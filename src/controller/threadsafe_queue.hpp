#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace constellation {

template <typename T>
class ThreadSafeQueue {
 private:
  std::queue<T> queue;
  mutable std::mutex mutex;
  std::condition_variable cond;

 public:
  void push(T value) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(value);
    cond.notify_one();
  }

  T pop() {
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock, [this] { return !queue.empty(); });
    T value = queue.front();
    queue.pop();
    return value;
  }
};

}  // namespace constellation
