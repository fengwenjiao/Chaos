#include "constellation_trainer.h"

#include <functional>

namespace constellation {

void ConstelTrainer::NotifyReadyAndWait() {
  // send ready signal to controller
  // should be called before other functions(e.g. pushpull, broadcast)
  if (!is_ctx_ready_.load()) {
    int head = static_cast<int>(kControllerSignal::kNodeReadySignal);
    int my_id = ps::Postoffice::Get()->GetMyID();
    std::string body = std::to_string(my_id);
    // no need wait
    trainer_->Request(head, body, ps::kScheduler);
    while (!is_ctx_ready_.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}

void ConstelTrainer::Broadcast(std::vector<int>& keys, std::vector<CArray*>& vals_init) {
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

void ConstelTrainer::PushPull(std::vector<int>& keys,
                              std::vector<CArray>& vals_push,
                              std::vector<CArray*>& vals_pull) {
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
  engine_ = new Engine(num_thread);
  using namespace std::placeholders;
  engine_->set_data_handle(std::bind(&ConstelTrainer::ProcessPushData, this, _1, _2, _3));
  //  engine_->set_messure_func();
  engine_->Start();
}

void ConstelTrainer::ProcessPushData(const int key,
                                     const EngineTaskData& data,
                                     Engine::ReturnOnAgg& rt) {
  auto& update = data.update_buf;
  auto update_buf = GetUpdateBuf(key);
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
        update_buf->merged = CArray(update.merged.size());  // alloc the space
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
          trainer_->Wait(init_waiting_ts_[key]);
        }
        // response
        for (const auto& meta : update_buf->request_meta) {
          ps::KVPairs<char> pairs;
          pairs.keys.push_back(key);
          pairs.vals = ps::SArray<char>(update_buf->merged);
          pairs.lens.push_back(static_cast<int>(update_buf->merged.size()));
          trainer_->Response(meta, pairs);
        }

        rt(0);
      }
      break;
    }
    case TaskTypeEnum::kPushPull: {
      if (update_buf->num == 1) {
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
        update_buf->shouldReset = true;
        // send to father
        // TODO: int dtype = what...
        if (isRootNode()) {
          // for root node, no need to send, just rt(0)
          rt(0);
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
          int ts = trainer_->ZPushPull(keys, *vals, vals, lens, cmd, [vals, lens]() {
            delete vals;
            delete lens;
          });
          rt(ts);
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
      tick.Decode(body);
      auto fut_timestamp = tick.timestamp;
      auto& transtopo = tick.transtopo;
      if (fut_timestamp == 0) {
        // sycn add stage, update right now
        int my_id = ps::Postoffice::Get()->GetMyID();
        auto it = transtopo.find(my_id);
        CHECK(it != transtopo.end()) << "Node " << my_id << " is not in the transtopo";
        // update Postoffice transtopo
        auto& local_transtopo = it->second;
        ps::Postoffice::Get()->UpdateLocalTrans(local_transtopo.getParent(),
                                                local_transtopo.getChildren());
        auto& topo = GetNodeTransTopo();
        topo = local_transtopo;
        this->is_scale_ = false;
        // notify main thread
        this->is_ctx_ready_.store(true);
      } else {
        // async add stage, set alarm
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
    default:
      LOG(FATAL) << "Unknown request type";
  }
}

}  // namespace constellation