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

    DataTrunk(size_t size = 0) {
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
  size_t size_{0};
  int dtype;

  // if this carray get the data ptr from other struct such as torch.tensor,
  // just record the ptr and should not free it
  void* borrowed_data{nullptr};

  explicit CArray() : dtype(0), sptr_(nullptr) {}
  explicit CArray(size_t size, int dtype = 0)
      : dtype(dtype), size_(size), sptr_(std::make_shared<DataTrunk>(size)) {}
  // for other AI framework, such as torch.tensor
  explicit CArray(const void* data, size_t size, int dtype = 0)
      : borrowed_data(const_cast<void*>(data)), size_(size), dtype(dtype), sptr_(nullptr) {}
  CArray(CArray&& other) = default;
  CArray(const CArray& other) = default;
  CArray& operator=(CArray&& other) = default;
  CArray& operator=(const CArray& other) = default;

  bool isNone() const {
    return (sptr_ == nullptr || sptr_->dptr_ == nullptr) && size_ == 0;
  }
  inline char* data() const {
    if (borrowed_data) {
      return static_cast<char*>(borrowed_data);
    }
    return sptr_->dptr_;
  }
  inline size_t size() const {
    return size_;
  }

  inline const std::shared_ptr<DataTrunk>& ptr() const {
    if (borrowed_data) {
      throw std::runtime_error("Could not get shared ptr when the CArray is borrowed!");
    }
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

  void From(const CArray& other) {
    if (other.data() && other.size()) {
      if (this->size() != other.size()) {
        this->sptr_ = std::make_shared<DataTrunk>(other.size());
        memcpy(this->data(), other.data(), other.size());
        this->dtype = other.dtype;
        this->size_ = other.size();
      } else {
        this->CopyFrom(other);
      }
    }
  }
};

}  // namespace constellation

#endif  // CONSTELLATION_CARRAY_H
