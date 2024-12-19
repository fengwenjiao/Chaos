#include "constellation_trainer.h"

#include "../utils/serilite.hpp"
#include "engine.hpp"

#if CONS_NETWORK_AWARE
#include "clusterRM/smq.h"
#endif

#include <functional>
#include <unordered_set>

namespace constellation {

ConstelTrainer::ConstelTrainer() {
  ps::StartAsync(0, "ConstelTrainer");
  this->trainer_ = new ps::KVTrainer<char>(0, 0);  // app_id, customer_id
  using namespace std::placeholders;
  static_cast<ps::SimpleApp*>(this->trainer_)
      ->set_request_handle(std::bind(&ConstelTrainer::RequestHandle, this, _1, _2));
  this->trainer_->set_request_handle(std::bind(&ConstelTrainer::DataHandle, this, _1, _2, _3));

#if CONS_NETWORK_AWARE
  test_client_ = std::make_unique<moniter::Smq>(ps::Postoffice::Get()->getSchedulerHost());
  // auto& ip2nodes = ps::Postoffice::Get()->GetIp2Nodes();
  // std::vector<std::string> ips;
  // for (const auto& ip_nodes : ip2nodes) {
  //   ips.push_back(ip_nodes.first);
  // }
  // test_client_->set_neighbors_(ips);
  test_client_thread_ = std::make_unique<std::thread>(
      [this] { test_client_->start_client(ps::Postoffice::Get()->GetMyID()); });
#endif
  //  init engine
  InitEngine(2);
}

ConstelTrainer::~ConstelTrainer() {
  ps::Finalize(0, false);
  delete this->trainer_;
  this->trainer_ = nullptr;
  engine_->Stop();
  delete engine_;
#if CONS_NETWORK_AWARE
  test_client_->stop_smq();
  test_client_thread_->join();
#endif
}

void ConstelTrainer::NotifyReadyAndWait(bool need_sycn_model,
                                        const std::vector<int> keys,
                                        const std::vector<uint64_t> lens) {
  // send ready signal to controller
  // should be called before other functions(e.g. pushpull, broadcast)
  if (!is_ctx_ready_.load()) {
    int head = static_cast<int>(kControllerSignal::kNodeReadySignal);
    int my_id = ps::Postoffice::Get()->GetMyID();
    if (need_sycn_model) {
      CHECK(!keys.empty()) << "keys should not be empty when need_sycn_model is true";
      CHECK(!lens.empty()) << "lens should not be empty when need_sycn_model is true";
      CHECK_EQ(keys.size(), lens.size()) << "keys and lens should have the same size";
      for (size_t i = 0; i < keys.size(); i++) {
        model_info_[keys[i]] = lens[i];
        model_size_ += lens[i];
      }
    }
    auto body =
        ReadySignalBody{my_id, std::move(need_sycn_model), std::move(keys), std::move(lens)};
    std::string body_str = serilite::serialize(body).as_string();
    // no need wait
    trainer_->Request(head, body_str, ps::kScheduler);
    while (!is_ctx_ready_.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}

bool ConstelTrainer::BatchEnd(std::vector<int>* keys_to_migrate) {
  auto timestamp = this->clock_.getLocalTimestamp() + 1;
  auto is_ticked = this->clock_.clockTick();
  if (isRootNode()) {
    NotifySchedulerUpdateClock(timestamp);
  }
  if (keys_to_migrate) {
    keys_to_migrate->clear();
  }
  model_sync_conf_.Clear();
  if (!is_ticked) {
    // no alarm
    clock_.unlock();
    return true;
  }

  // there is a alarm
  auto& transtopo = this->clock_.ticks_[timestamp].transtopo;
  auto& model_sync_conf = this->clock_.ticks_[timestamp].model_sync_conf;
  int my_id = ps::Postoffice::Get()->GetMyID();
  auto it = transtopo.find(my_id);
  if (it == transtopo.end()) {
    clock_.unlock();
    return false;
  }
  // update Postoffice transtopo
  auto& local_transtopo = it->second;
  this->SetNodeTransTopo(local_transtopo);
  clock_.unlock();  // TODO: to be improved

  if (!model_sync_conf.paths.empty()) {
    PS_VLOG(2) << "BatchEnd Model Sync Conf: " << model_sync_conf.debug_string();
    // need migrate the kvs
    if (keys_to_migrate) {
      std::unordered_set<int> keys_to_migrate_set;
      for (const auto& kvslice_vec : model_sync_conf.kvslices) {
        for (const auto& kvslice : kvslice_vec) {
          if (kvslice.is_full()) {
            for (int key = kvslice.key_begin; key < kvslice.key_end; key++) {
              keys_to_migrate_set.insert(key);
            }
          } else {
            keys_to_migrate_set.insert(kvslice.key_begin);
          }
        }
      }
      keys_to_migrate->assign(keys_to_migrate_set.begin(), keys_to_migrate_set.end());
    }
    model_sync_conf_ = std::move(model_sync_conf);
  }
  this->clock_.removeTick(timestamp);

  return true;
}

void ConstelTrainer::Migrate(const std::vector<int>& keys, const std::vector<CArray>& vals) {
  CHECK_EQ(keys.size(), vals.size());
  int size = this->model_sync_conf_.paths.size();
  for (size_t i = 0; i < size; i++) {
    const auto& path = this->model_sync_conf_.paths[i];
    if (path.size() < 2 || path[0] != ps::Postoffice::Get()->GetMyID()) {
      continue;
    }
    ModelSycnConf model_sync_conf;
    model_sync_conf.target_node_id = {path.back()};
    model_sync_conf.paths = {std::vector<int>(path.begin() + 1, path.end())};
    for (const auto& kvslice : this->model_sync_conf_.kvslices[i]) {
      if (kvslice.is_full()) {
        // send the full kv-pair
        for (int key = kvslice.key_begin; key < kvslice.key_end; key++) {
          auto it = std::find(keys.begin(), keys.end(), key);
          CHECK(it != keys.end()) << "key " << key << " is not in the keys";
          int idx = std::distance(keys.begin(), it);

          int cmd = GetCommandType(RequestType::kModelSync, vals[idx].dtype);

          // auto vals_s = new ps::SArray<char>(vals[idx]);
          auto vals_s = new ps::SArray<char>(vals[idx].data(), vals[idx].size(), false);
          auto key_t = static_cast<ps::Key>(key);
          ps::SArray<ps::Key> keys({key_t});
          auto len_t = static_cast<int>(vals[idx].size());
          auto lens = new ps::SArray<int>;
          lens->push_back(len_t);

          auto slice_info = KVSlice{key, key + 1};
          model_sync_conf.kvslices = {{slice_info}};

          auto str = serilite::serialize(model_sync_conf).as_string();
          int ts = trainer_->ZMove(path[1], keys, *vals_s, *lens, str, cmd, [vals_s, lens]() {
            delete vals_s;
            delete lens;
          });
          PS_VLOG(2) << "Begin to Migrate : " << model_sync_conf.debug_string();
        }
      } else {
        int key = kvslice.key_begin;
        CHECK(std::find(keys.begin(), keys.end(), key) != keys.end())
            << "key " << key << " is not in the keys";
        int idx = std::distance(keys.begin(), std::find(keys.begin(), keys.end(), key));

        int cmd = GetCommandType(RequestType::kModelSync, vals[idx].dtype);

        // auto vals_s = new ps::SArray<char>(vals[idx].data() + kvslice.slice, kvslice.slice_len);
        auto vals_s =
            new ps::SArray<char>(vals[idx].data() + kvslice.slice, kvslice.slice_len, false);
        auto key_t = static_cast<ps::Key>(key);
        ps::SArray<ps::Key> keys({key_t});
        auto len_t = static_cast<int>(kvslice.slice_len);
        auto lens = new ps::SArray<int>;
        lens->push_back(len_t);

        auto slice_info = KVSlice{key, kvslice.slice, kvslice.slice_len};
        model_sync_conf.kvslices = {{slice_info}};
        auto str = serilite::serialize(model_sync_conf).as_string();
        // LOG(INFO) << "Migrate key: " << key << " to node: " << path.back();
        int ts = trainer_->ZMove(path[1], keys, *vals_s, *lens, str, cmd, [vals_s, lens]() {
          delete vals_s;
          delete lens;
        });
        PS_VLOG(2) << "Begin to Migrate : " << model_sync_conf.debug_string();
      }
    }
  }
}

void ConstelTrainer::NotifySchedulerUpdateClock(uint32_t timestamp) {
  // if (timestamp % 5 != 0) {
  //   return;
  // }
  int head = static_cast<int>(kControllerSignal::kUpdateClockSignal);
  int my_id = ps::Postoffice::Get()->GetMyID();
  std::string body = std::to_string(my_id) + "," + std::to_string(timestamp);
  // no need wait
  trainer_->Request(head, body, ps::kScheduler);
}

void ConstelTrainer::Broadcast(const std::vector<int>& keys,
                               const std::vector<CArray*>& vals_init) {
  int size = keys.size();
  CHECK_EQ(vals_init.size(), size);

  // TODO :model init
  std::vector<EngineTaskData> vals;
  if (isRootNode()) {
    vals.resize(size, {UpdateBuf(), TaskTypeEnum::kBroadcastDefault, true});
  } else {
    vals.resize(size, {UpdateBuf(), TaskTypeEnum::kBroadcastDefault, false});
  }
  for (size_t i = 0; i < size; i++) {
    vals[i].update_buf.merged = *vals_init[i];
  }
  engine_->PushAndWait(keys, std::move(vals), nullptr);

  // copy the update buf to vals
  for (size_t i = 0; i < size; i++) {
    int key = keys[i];
    vals_init[i]->CopyFrom(GetUpdateBuf(key)->merged);
  }
}

void ConstelTrainer::Recv(const std::vector<int>& keys, const std::vector<CArray*>& vals) {
  std::unique_lock<std::mutex> lock(model_sync_mu_);
  wait_recv_keys_ = const_cast<std::vector<int>*>(&keys);
  wait_recv_vals_ = const_cast<std::vector<CArray*>*>(&vals);
  model_sync_cv_.wait(lock, [this] { return this->is_model_sync_.load(); });
  wait_recv_keys_ = nullptr;
  wait_recv_vals_ = nullptr;
}

void ConstelTrainer::PushPull(const std::vector<int>& keys,
                              const std::vector<CArray>& vals_push,
                              const std::vector<CArray*>& vals_pull) {
  int size = keys.size();
  CHECK_EQ(vals_push.size(), size);
  CHECK_EQ(vals_pull.size(), size);
  std::unordered_map<int, int> res_map;
  std::vector<EngineTaskData> vals(size, {UpdateBuf(), TaskTypeEnum::kPushPull, true});
  // TODO: need consider merge the key-value if there more than one gpus
  for (size_t i = 0; i < size; i++) {
    // submit the key-value, and get the timestamp which the corresponding merged value is pushed to
    // the father node
    auto& update = vals[i].update_buf;
    update.merged = vals_push[i];
  }
  auto now = clock_.getLocalTimestamp();
  cached_kv_mu_.lock();
  if (cached_kv_.find(now) != cached_kv_.end()) {
    size_t size = cached_kv_[now].size();
    auto keys_ = std::vector<int>(size);
    auto datas_ = std::vector<EngineTaskData>(size);
    size_t i = 0;
    for (auto& data : cached_kv_[now]) {
      keys_[i] = data.first;
      datas_[i] = data.second;
      i++;
    }
    engine_->PushAsync(keys_, std::move(datas_));
    cached_kv_.erase(now);
  }
  cached_kv_mu_.unlock();
  engine_->PushAndWait(keys, std::move(vals), &res_map);
  for (auto& it : res_map) {
    int key = it.first;
    auto key_it = std::find_if(keys.begin(), keys.end(), [key](int k) { return k == key; });
    CHECK_NE(key_it, keys.end());
    int i = std::distance(keys.begin(), key_it);
    if (it.second > 0) {
      // for root node,  no need to Wait
      trainer_->Wait(it.second);
    }
    // put pullback data to vals_pull from the update buf
    auto* buf = GetUpdateBuf(key);
    vals_pull[i]->CopyFrom(buf->merged);
    // 2. 用记录的meta去回复子节点，带上pull回来的数据
    for (const auto& meta : buf->request_meta) {
      ps::KVPairs<char> pairs;
      pairs.keys.push_back(key);
      pairs.vals = ps::SArray<char>(buf->merged);
      auto len = static_cast<int>(buf->merged.size());  // bytes
      pairs.lens = {len};
      trainer_->Response(meta, pairs);
    }
  }
}

int ConstelTrainer::SimplePushPullDefault(int key, const CArray& val) {
  return 0;
}

void ConstelTrainer::InitEngine(size_t num_thread = 8) {
  engine_ = new EngineType(num_thread);
  using namespace std::placeholders;
  engine_->set_data_handle(std::bind(&ConstelTrainer::ProcessPushData, this, _1, _2, _3));
  //  engine_->set_messure_func();
  engine_->Start();
}

void ConstelTrainer::ProcessPushData(const int key,
                                     const EngineTaskData& data,
                                     std::shared_ptr<ReturnOnAgg<EngineTaskData, int>> rt) {
  auto& update = data.update_buf;
  auto update_buf = GetUpdateBuf(key);
  auto now = clock_.getLocalTimestamp();
  if (!update.request_meta.empty() && update.request_meta[0].extra.size() > 0 &&
      data.type == TaskTypeEnum::kPushPull) {
    uint32_t timestamp = std::stoi(update.request_meta[0].extra);
    if (timestamp > now) {
      std::lock_guard<std::mutex> lock(cached_kv_mu_);
      cached_kv_[timestamp].push_back({key, data});
      return;
    }
  }
  int all_recved = ps::Postoffice::Get()->GetMyChildren().size() + 1;
  auto& tasktype = data.type;
  if (update_buf->shouldReset)
    update_buf->ResetUpdateBuf();
  update_buf->num++;
  if (!update.request_meta.empty()) {
    update_buf->request_meta.push_back(update.request_meta[0]);
  }
  switch (tasktype) {
    case TaskTypeEnum::kBroadcastDefault: {
      if (!update.merged.isNone()) {
        // init request from myself
        if (update_buf->merged.isNone() || update_buf->merged.size() != update.merged.size()) {
          update_buf->merged = CArray(update.merged.size());  // alloc the space
        }
        if (data.isFromRoot) {
          update_buf->merged.CopyFrom(update.merged);
          std::lock_guard<std::mutex> lock(init_waiting_ts_mu_);
          init_waiting_ts_[key] = -1;
        } else {
          // pull request to father
          auto key_t = ps::SArray<ps::Key>({static_cast<ps::Key>(key)});
          auto* len_t = new ps::SArray<int>({static_cast<int>(update_buf->merged.size())});
          // TODO: use another buff to recv
          auto* vals = new ps::SArray<char>(update_buf->merged);
          int cmd = GetCommandType(RequestType::kDefaultInit, update.merged.dtype);
          int ts = trainer_->ZPull(key_t, vals, len_t, cmd, [vals, len_t]() {
            delete vals;
            delete len_t;
          });

          {
            std::lock_guard<std::mutex> lock(init_waiting_ts_mu_);
            init_waiting_ts_[key] = ts;
          }
        }
      } else {
        // init request from sons
      }
      if (update_buf->num == all_recved) {
        update_buf->shouldReset = true;
        int ts;
        {
          std::lock_guard<std::mutex> lock(init_waiting_ts_mu_);
          ts = init_waiting_ts_[key];
        }
        if (ts != -1) {
          trainer_->Wait(ts);
        }
        // response
        for (const auto& meta : update_buf->request_meta) {
          ps::KVPairs<char> pairs;
          pairs.keys.push_back(key);
          pairs.vals = ps::SArray<char>(update_buf->merged);
          pairs.lens.push_back(static_cast<int>(update_buf->merged.size()));
          trainer_->Response(meta, pairs);
        }

        (*rt)(0);
      }
      break;
    }
    case TaskTypeEnum::kPushPull: {
      if (update_buf->num == 1) {
        if (update_buf->merged.isNone() || update_buf->merged.size() != update.merged.size()) {
          update_buf->merged = CArray(update.merged.size());  // alloc the space
        }
        // copy the val into merged
        update_buf->merged.CopyFrom(update.merged);
      } else {
        // TODO: Use Template to replace this
        auto dst = reinterpret_cast<float*>(update_buf->merged.data());
        auto src = reinterpret_cast<float*>(update.merged.data());
        // TODO: Use omp
        CHECK_EQ(update_buf->merged.size(), update.merged.size());
        for (size_t i = 0; i < update_buf->merged.size() / sizeof(float); i++) {
          dst[i] += src[i];
        }
      }

      if (update_buf->num == all_recved) {
        std::string dbg;
        for (const auto& meta : update_buf->request_meta) {
          dbg += std::to_string(meta.sender) + " ";
        }
        update_buf->shouldReset = true;
        // send to father
        // TODO: int dtype = what...
        if (isRootNode()) {
          // for root node, no need to send, just rt(0)
          (*rt)(0);
        } else {
          // ps::SArray vals's initialization will create the shared_ptr from the data ptr,
          // and CArray also hold the shared_ptr of the data, So here `deletable` must be false.
          // or may cause double free
          // TODO: use another buff to recv
          auto vals = new ps::SArray<char>(update_buf->merged);
          auto key_t = static_cast<ps::Key>(key);
          ps::SArray<ps::Key> keys({key_t});
          auto len_t = static_cast<int>(update_buf->merged.size());
          auto lens = new ps::SArray<int>;
          lens->push_back(len_t);
          int cmd = GetCommandType(RequestType::kDefaultPushPull, update_buf->merged.dtype);
          int ts = trainer_->ZPushPull(
              keys, *vals, vals, lens, cmd, std::to_string(now), [vals, lens]() {
                delete vals;
                delete lens;
              });
          (*rt)(ts);
        }
      }
      break;
    }
    default:
      LOG(FATAL) << "unsupported task type";
  }
}

void ConstelTrainer::RequestHandle(const ps::SimpleData& recved, ps::SimpleApp* app) {
  int sender = recved.sender;
  auto signal = static_cast<kControllerSignal>(recved.head);
  const std::string& body = recved.body;
  switch (signal) {
    // case : kControllerSignal::xxxx:
    //     break;
    case kControllerSignal::kUpdateTransTopoAnouce: {
      // recv new transtopo, set it to clock
      ScaleClock::Tick tick;
      serilite::deserialize(body, tick);
      auto fut_timestamp = tick.timestamp;
      auto& transtopo = tick.transtopo;
      if (fut_timestamp == 0 || !this->is_ctx_ready_.load()) {
        // for sycn add nodes or async add nodes
        //  sycn add stage, update right now
        this->is_scale_ = !fut_timestamp == 0;
        int my_id = ps::Postoffice::Get()->GetMyID();
        auto it = transtopo.find(my_id);
        if (fut_timestamp == 0) {
          CHECK(it != transtopo.end()) << "Node " << my_id << " is not in the transtopo";
        }
        if (it != transtopo.end()) {
          this->clock_.local_timestamp_ = fut_timestamp;
          auto& local_transtopo = it->second;
          this->SetNodeTransTopo(local_transtopo);
          PS_VLOG(2) << "Update transtopo: " << local_transtopo.debug_string();
          // notify main thread
          this->is_ctx_ready_.store(true);
        }
      } else {
        // for old nodes
        PS_VLOG(2) << "Add new tick: " << tick.debug_string();
        clock_.setAlarm(std::move(tick));
      }

      break;
    }
    default:
      LOG(FATAL) << "Unkown signal ";
      break;
  };
  app->Response(recved);
}

void ConstelTrainer::DataHandle(const ps::KVMeta& req_meta,
                                const ps::KVPairs<char>& req_data,
                                ps::KVTrainer<char>* trainer) {
  DataHandleType type = DepairDataHandleType(req_meta.cmd);
  EngineTaskData data{UpdateBuf(), TaskTypeEnum::kPushPull};
  ModelSycnConf model_sync_conf;
  auto& updt = data.update_buf;
  switch (type.requestType) {
    case RequestType::kDefaultPushPull:
      updt.request_meta.push_back(req_meta);
      updt.merged = CArray(req_data.lens[0]);
      // TODO: 先数据拷贝一份，有优化空间
      updt.merged.CopyFrom((void*)req_data.vals.data(), req_data.lens[0]);
      engine_->PushAsync({static_cast<int>(req_data.keys[0])}, {data});
      break;
    case RequestType::kDefaultInit:
      updt.request_meta.push_back(req_meta);
      data.type = TaskTypeEnum::kBroadcastDefault;
      //      updt.merged = CArray(req_data.lens[0]);
      //      updt.merged.CopyFrom((void*)req_data.vals.data(), req_data.lens[0]);
      engine_->PushAsync({static_cast<int>(req_data.keys[0])}, {data});
      break;
    case RequestType::kModelSync:
      if (req_meta.extra.empty()) {
        LOG(WARNING) << "ModelSync request has no extra";
        break;
      }
      serilite::deserialize(req_meta.extra, model_sync_conf);
      // check the model_sync_conf
      if (model_sync_conf.paths.empty() || model_sync_conf.kvslices.empty() ||
          model_sync_conf.paths[0][0] != ps::Postoffice::Get()->GetMyID()) {
        LOG(WARNING) << "ModelSync request is invalid";
        break;
      }
      if (model_sync_conf.paths[0].size() == 1 &&
          model_sync_conf.target_node_id[0] == ps::Postoffice::Get()->GetMyID()) {
        // recv the data
        std::unique_lock<std::mutex> lock(model_sync_mu_);
        while (wait_recv_keys_ == nullptr) {
          lock.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          lock.lock();
        }
        CHECK_EQ(wait_recv_keys_->size(), wait_recv_vals_->size());
        auto idx = std::distance(wait_recv_keys_->begin(),
                                 std::find(wait_recv_keys_->begin(),
                                           wait_recv_keys_->end(),
                                           model_sync_conf.kvslices[0][0].key_begin));
        if (idx < wait_recv_keys_->size()) {
          if (model_sync_conf.kvslices[0][0].is_full()) {
            // TODO: This can be zero copy
            wait_recv_vals_->at(idx)->CopyFrom(req_data.vals.data(), req_data.lens[0]);
          } else {
            wait_recv_vals_->at(idx)->CopyFrom(
                req_data.vals.data(), req_data.lens[0], model_sync_conf.kvslices[0][0].slice);
          }
          this->model_size_ -= req_data.lens[0];
          if (this->model_size_ == 0) {
            is_model_sync_.store(true);
            model_sync_cv_.notify_all();
          } else if (this->model_size_ < 0) {
            LOG(WARNING) << "model_size_ is less than 0";
          }
          PS_VLOG(2) << "recv the migrate data: " << model_sync_conf.debug_string()
                     << " kvpairs: " << " lens: "<< req_data.lens[0];
        } else {
          LOG(WARNING) << "Recv data is not in the waiting list";
        }

      } else {
        int next = model_sync_conf.paths[0][1];
        model_sync_conf.paths[0].erase(model_sync_conf.paths[0].begin());
        auto str = serilite::serialize(model_sync_conf).as_string();
        trainer_->ZMove(next, req_data.keys, req_data.vals, req_data.lens, str, req_meta.cmd);
        PS_VLOG(2) << "forward the migrate data: " << model_sync_conf.debug_string()
                      << " kvpairs: " <<" lens: "<< req_data.lens[0];
      }
      trainer_->Response(req_meta, {});
      break;
    default:
      LOG(FATAL) << "Unknown request type";
  }
}

}  // namespace constellation