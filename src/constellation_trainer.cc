#include "constellation_trainer.h"
#include <functional>

namespace constellation {

void ConstelTrainer::Init(int key, const CArray* val) {
  // send ready signal to controller
  int head = static_cast<int>(kControllerSignal::kNodeReadySignal);
  int my_id = ps::Postoffice::Get()->GetMyID();
  std::string body = std::to_string(my_id);
  trainer_->Wait(trainer_->Request(head, body, ps::kScheduler));
  // wait until receive the transtopo from controller
  while (!is_ctx_ready_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  // TODO :model init
  // TODO: init pushpull_buf_
}

void ConstelTrainer::PushPull(std::vector<int>& keys,
                              std::vector<CArray>& vals_push,
                              std::vector<CArray*>& vals_pull) {
  std::vector<int> ts_arr;
  int size = keys.size();
  std::unordered_map<int, int> res_map;
  std::vector<UpdateBuf> vals(size);
  // TODO: need consider merge the key-value if there more than one gpus
  for (size_t i = 0; i < size; i++) {
    int key = keys[i];
    // submit the key-value, and get the timestamp which the corresponding merged value is pushed to
    // the father node
    auto& updt = vals[i];
    updt.merged = vals_push[i];
  }
  engine_->PushAndWait(std::move(keys), std::move(vals), res_map);
  // TODO: for root node, no need to Wait
  for (auto& it : res_map) {
    trainer_->Wait(it.second);
    // 1. 对于所有的meta 还有没记录, 是在recive的时候记录的
    // 2. 用记录的meta去回复子节点，带上pull回来的数据
  }
  // TODO: refer to update_buf, get the meta and send the aggregated value to the son node
}

int ConstelTrainer::SimplePushPullDefault(int key, const CArray& val) {
  return 0;
}

void ConstelTrainer::InitEngine(size_t num_thread = 2) {
  engine_ = new Engine(num_thread);
  using namespace std::placeholders;
  engine_->set_data_handle(std::bind(&ConstelTrainer::ProcessPushData, this, _1, _2, _3));
  //  engine_->set_messure_func();
}

void ConstelTrainer::ProcessPushData(const int key,
                                     const UpdateBuf& data,
                                     Engine::ReturnOnAgg& rt) {
  auto update_buf = GetUpdateBuf(key);
  if (update_buf->num == 0) {
    // copy the val into merged
    update_buf->merged.CopyFrom(data.merged);
    update_buf->num++;
  } else {
    // TODO: Use Template to replace this
    auto dst = reinterpret_cast<float*>(update_buf->merged.data());
    auto src = reinterpret_cast<float*>(update_buf->merged.data());
    // TODO: Use omp
    CHECK_EQ(update_buf->merged.size(), data.merged.size());
    for (size_t i = 0; i < update_buf->merged.size() / sizeof(float); i++) {
      dst[i] += src[i];
    }
  }
  update_buf->num++;
  if (!data.request_meta.empty()) {
    update_buf->request_meta.push_back(data.request_meta[0]);
  }
  if (update_buf->num == ps::Postoffice::Get()->GetMyChildren().size() + 1) {
    // send to father
    // TODO: int dtype = what...
    ps::SArray<char> vals(update_buf->merged.data(), update_buf->merged.size(), false);
    auto key_t = static_cast<ps::Key>(key);
    ps::SArray<ps::Key> keys(&key_t, 1, false);
    auto len_t = static_cast<int>(update_buf->merged.size());
    ps::SArray<int> lens(&len_t, 1, false);
    int cmd = GetCommandType(RequestType::kDefaultPushPull, update_buf->merged.dtype);
    // TODO: for root node, no need to send, just rt(0)
    // TODO: add pull callback(refactor the return impl in engine)
    int ts = trainer_->ZPush(keys, vals, lens, cmd);
    rt(ts);
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
  UpdateBuf updt;
  switch (type.requestType) {
    case RequestType::kDefaultPushPull:
      updt.request_meta.push_back(req_meta);
      updt.merged = CArray(req_data.lens[0]);
      // 先数据拷贝一份，有优化空间
      updt.merged.CopyFrom((void*)req_data.vals.data(), req_data.lens[0]);
      // 收的逻辑还没有写
      // 存meta，meta的管理（还没想好）
      break;
    default:
      LOG(FATAL) << "Unkown request type";
  }
}

}  // namespace constellation