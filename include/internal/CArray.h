#ifndef CONSTELLATION_CARRAY_H
#define CONSTELLATION_CARRAY_H

#include <memory>

namespace constellation {
// reffer to byteps: https://github.com/bytedance/byteps/blob/master/byteps/common/common.h
// TODO: Add more data type
enum class ConstelDataType {
  CONSTEL_FLOAT32 = 0,
};

struct CArray {
  struct DataTrunk {
    char* dptr_;
    size_t size_{0};

    DataTrunk(size_t size = 0) : size_(size) {
      if (size == 0) {
        dptr_ = nullptr;
        return;
      }
      dptr_ = new char[size];
      if (!dptr_) {
        std::runtime_error("CArray: out of memory");
      }
    }
    ~DataTrunk() {
      if (dptr_) {
        delete[] dptr_;
        dptr_ = nullptr;
      }
    }
    DataTrunk(DataTrunk&&) = delete;
    DataTrunk(const DataTrunk&) = delete;
    DataTrunk& operator=(DataTrunk&&) = delete;
    DataTrunk& operator=(const DataTrunk&) = delete;
  };

  std::shared_ptr<DataTrunk> sptr_;
  int dtype;

  explicit CArray() : dtype(0), sptr_(nullptr) {}
  explicit CArray(size_t size) : dtype(0), sptr_(std::make_shared<DataTrunk>(size)) {}
  CArray(CArray&& other) = default;
  CArray(const CArray& other) = default;
  CArray& operator=(CArray&& other) = default;
  CArray& operator=(const CArray& other) = default;

  bool isNone() const {
    return sptr_ == nullptr || sptr_->dptr_ == nullptr || sptr_->size_ == 0;
  }
  inline char* data() const {
    return sptr_->dptr_;
  }
  inline size_t size() const {
    return sptr_->size_;
  }

  inline const std::shared_ptr<DataTrunk>& ptr() const {
    return sptr_;
  }

  void CopyFrom(const CArray& other) {
    // TODO:use OMP to accelerate
    if (other.data() && other.size()) {
      if (this->size() != other.size())
        throw std::runtime_error("CArray: size mismatch");
      memcpy(this->data(), other.data(), other.size());
      this->dtype = other.dtype;
    }
  }
  void CopyFrom(const void* data, size_t size) const {
    if (this->size() != size) {
      throw std::runtime_error("CArray: size mismatch");
    }
    if (data && size) {
      memcpy(this->data(), data, size);
    }
  }
};

}  // namespace constellation

#endif  // CONSTELLATION_CARRAY_H
