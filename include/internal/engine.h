//
// Created by deemo on 5/31/24.
//

#ifndef CONSTELLATION_ENGINE_H_
#define CONSTELLATION_ENGINE_H_

#include "ps/internal/threadsafe_queue.h"
#include <vector>
#include <functional>
#include <thread>
#include <unordered_set>
#include <future>

namespace constellation {

template <typename Data, typename ResType>
class ConstelAggEngine {
 public:
  class ReturnOnAgg {
   public:
    inline void operator()(const ResType& res) {
      return_handle_(engine_, this->id, res);
    }

   private:
    friend class ConstelAggEngine;
    int id;
    ConstelAggEngine* engine_;
    std::function<void(ConstelAggEngine*, const int, const ResType)> return_handle_;
  };

  using DataHandle = std::function<void(int, Data, ReturnOnAgg)>;

  using MessureFunc = std::function<int(int, Data)>;

  void set_data_handle(const DataHandle& handle) {
    this->datahandle_ = handle;
  }

  void set_messure_func(const MessureFunc& func) {
    this->messure_func_ = func;
  }

  explicit ConstelAggEngine(size_t num_threads = 2)
      : num_threads_(num_threads), is_running_(false) {}
  ConstelAggEngine(const ConstelAggEngine&) = delete;
  ConstelAggEngine(ConstelAggEngine&&) = delete;

  ConstelAggEngine& operator=(const ConstelAggEngine&) = delete;

  ~ConstelAggEngine() {
    if (is_running_)
      this->Stop();
  }
  inline ReturnOnAgg CreateReturnCallBack(int id) {
    ReturnOnAgg callback;
    callback.engine_ = this;
    using namespace std::placeholders;
    callback.return_handle_ = std::bind(&ConstelAggEngine::CallBackReturnHandle, this, _1, _2, _3);
  }

  void CallBackReturnHandle(ConstelAggEngine* engine, const int id, const ResType& res) {
    auto it = std::find(engine->expected_ids_.begin(), engine->expected_ids_.end(), id);
    if (it != engine->expected_ids_.end()) {
      engine->ret_[id] = res;
    }
  }

  void Start() {
    is_running_ = true;
    for (size_t i = 0; i < num_threads_; ++i) {
      auto queue = std::make_unique<ps::ThreadsafeQueue<Task>>();
      queues_.emplace_back(std::move(queue));
      threads_.emplace_back([this, i]() {
        while (true) {
          Task task;
          this->queues_[i]->WaitAndPop(&task);
          if (!task)
            break;
          task();
        }
      });
    }
  }
  void Stop() {
    is_running_ = false;
    for (auto& queue : queues_) {
      queue->Push(Task());
    }
    for (auto& thread : threads_) {
      if (thread.joinable())
        thread.join();
    }
  }
  void PushAndWait(std::vector<int>&& ids,
                   std::vector<Data>&& data,
                   std::unordered_map<int, Data>* res) {
    expected_ids_ = std::unordered_set<int>(ids.begin(), ids.end());
    res->clear();
    for (auto i : expected_ids_) {
      res[i] = Data();
    }

    ret_ = res;
    CHECK_EQ(ret_->size(), expected_ids_.size());

    PushAsync(std::move(ids), std::move(data));
  }
  void PushAsync(std::vector<int>&& ids, std::vector<Data>&& data) {
    CHECK_EQ(ids.size(), data.size());
    for (size_t i = 0; i < ids.size(); ++i) {
      int id = ids[i];
      auto d = std::make_shared<Data>(std::move(data[i]));
      size_t tid = GetWorkerId(id, &d);
      queues_[tid]->Push([this, d, id]() { this->datahandle_(id, *d); });
    }
  }

 private:
  using Task = std::function<void()>;

  size_t GetWorkerId(int id, const Data* data = nullptr) {
    return GetWorkerIdDefault(id, data);
  }
  size_t GetWorkerIdDefault(int id, const Data* data = nullptr) {
    std::lock_guard<std::mutex> lock(thread_id_mu_);
    if (thread_id_map_.find(id) != thread_id_map_.end()) {
      return thread_id_map_[id];
    }

    if (load_map_.empty()) {
      for (int i = 0; i < num_threads_; ++i) {
        load_map_[i] = 0;
      }
    }
    // find the least loaded thread
    int taskload = (data == nullptr || messure_func_ == nullptr) ? 1 : messure_func_(id, *data);
    int min_load = std::numeric_limits<int>::max();
    size_t min_load_id = 0;
    for (auto& load : load_map_) {
      if (load.second < min_load) {
        min_load = load.second;
        min_load_id = load.first;
      }
    }
    load_map_[min_load_id] += taskload;
    thread_id_map_[id] = min_load_id;
    return min_load_id;
  }

  DataHandle datahandle_;
  MessureFunc messure_func_;

  std::mutex thread_id_mu_;
  std::unordered_map<int, size_t> thread_id_map_;
  std::unordered_map<size_t, int> load_map_;

  std::unordered_map<int, Data>* ret_;

  std::unordered_set<int> expected_ids_;

  bool is_running_;
  size_t num_threads_;
  std::vector<std::unique_ptr<ps::ThreadsafeQueue<Task>>> queues_;
  std::vector<std::thread> threads_;
};
}  // namespace constellation

#endif  // CONSTELLATION_ENGINE_H_
