//
// Created by DeEMO on 2024/7/15.
//

#ifndef CONSTELLATION_CARRAY_H
#define CONSTELLATION_CARRAY_H

struct CArray {
  // reffer to byteps: https://github.com/bytedance/byteps/blob/master/byteps/common/common.h
  // TODO: Add more data type
  enum class ConstelDateType {
    CONSTEL_FLOAT32 = 0,
  };
  struct DataTrunk {
    char* dptr_;
    size_t size_{0};

    DataTrunk(size_t size = 0) : size_(size) {
      if (size == 0) {
        dptr_ = nullptr;
        return;
      }
      dptr_ = new char[size];
      CHECK(dptr_);
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
      this->sptr_ = std::make_shared<DataTrunk>(other.size());
      memcpy(this->data(), other.data(), other.size());
      this->dtype = other.dtype;
    }
  }
  void CopyFrom(const void* data, size_t size) {
    if (this->size() != size) {
      throw "CArray::CopyFrom: size not match";
    }
    if (data && size) {
      memcpy(this->data(), data, size);
    }
  }
};

#endif  // CONSTELLATION_CARRAY_H
