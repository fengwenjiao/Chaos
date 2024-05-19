#ifndef _CONSTELLATION_TRAINER_H_
#define _CONSTELLATION_TRAINER_H_

#include "./constellation_commons.h"
#include "ps/ps.h"
#include <atomic>

namespace constellation {

template <typename T>
struct CArray {
  T* data;
  size_t size;
  bool is_owner;
  int dtype;  // if use compressed grad, check byteps and there more things to do
  explicit CArray() : data(nullptr), size(0), is_owner(false) {}
  explicit CArray(size_t size) : data(new T[size]), size(size), is_owner(true) {}
  CArray(T* data, size_t size, bool is_owner = false)
      : data(data), size(size), is_owner(is_owner) {}
  ~CArray() {
    if (is_owner) {
      delete[] data;
    }
  }
};

class ConstelTrainer {
 public:
  explicit ConstelTrainer() {
    ps::StartAsync(0, "ConstelTrainer");
    this->trainer_ = new ps::KVTrainer<char>(0, 0);  // app_id, customer_id
    using namespace std::placeholders;
    static_cast<ps::SimpleApp*>(this->trainer_)
        ->set_request_handle(std::bind(&ConstelTrainer::RequestHandle, this, _1, _2));
    this->trainer_->set_request_handle(std::bind(&ConstelTrainer::DataHandle, this, _1, _2, _3));
  }
  ~ConstelTrainer() {
    ps::Finalize(0, false);
    delete this->trainer_;
  }
  void Init(int key, const CArray<char>* val);

  void Push(int key, const CArray<char>& val);

  void Pull(int key, CArray<char>* val);

  void RequestHandle(const ps::SimpleData& recved, ps::SimpleApp* app);

  void DataHandle(const ps::KVMeta& req_meta,
                  const ps::KVPairs<char>& req_data,
                  ps::KVTrainer<char>* trainer);

 private:
  ScaleClock clock_;
  ps::KVTrainer<char>* trainer_;

  std::atomic<bool> is_ctx_ready_{false};

  bool is_scale_ = true;
};
}  // namespace constellation

#endif  // _CONSTELLATION_TRAINER_H_