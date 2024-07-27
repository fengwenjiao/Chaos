#include <vector>

#include "constellation_api.h"
#include "constellation.h"

using namespace constellation;

int ConstelTrainerHandleCreate(const char* name, ConstelTrainerHandle* handle) {
  API_BEGIN();
  *handle = new ConstelTrainer();
  API_END();
}

int ConstelTrainerHandleFree(ConstelTrainerHandle handle) {
  API_BEGIN();
  delete static_cast<ConstelTrainer*>(handle);
  API_END();
}

int ConstellationCArrayCreateDefault(ConstellationCArrayHandle* handle, int size, int dtype) {
  API_BEGIN();
  *handle = new CArray(size, dtype);
  API_END();
}

int ConstellationCArrayCreate(ConstellationCArrayHandle* handle, void* data, int size, int dtype) {
  API_BEGIN();
  *handle = new CArray(data, size, dtype);
  API_END();
}

int ConstellationCArrayFree(ConstellationCArrayHandle handle) {
  API_BEGIN();
  delete static_cast<CArray*>(handle);
  API_END();
}

int ConstellationTrainerPushPull(ConstelTrainerHandle handle,
                                 uint32_t key_num,
                                 const int* keys_in,
                                 uint32_t key_num_out,
                                 const int* keys_out,
                                 ConstellationCArrayHandle* values,
                                 ConstellationCArrayHandle* outs) {
  API_BEGIN();
  std::vector<int> keys_in_vec(keys_in, keys_in + key_num);
  std::vector<int> keys_out_vec(keys_out, keys_out + key_num_out);
  if (keys_in_vec.size() != keys_out_vec.size()) {
    throw std::runtime_error("Have different size of keys_in and keys_out");
  }
  std::vector<CArray> values_vec(keys_in_vec.size());
  std::vector<CArray*> outs_vec(keys_out_vec.size());
  for (uint32_t i = 0; i < key_num; ++i) {
    values_vec[i] = *static_cast<CArray*>(values[i]);
  }
  for (uint32_t i = 0; i < key_num_out; ++i) {
    outs_vec[i] = static_cast<CArray*>(outs[i]);
  }
  static_cast<ConstelTrainer*>(handle)->PushPull(keys_in_vec, values_vec, outs_vec);

  API_END();
}

int ConstellationTrainerInit(ConstelTrainerHandle handle,
                             uint32_t key_num,
                             const int* keys_in,
                             ConstellationCArrayHandle* values) {
  API_BEGIN();
  std::vector<int> keys_in_vec(keys_in, keys_in + key_num);
  std::vector<CArray*> values_vec(keys_in_vec.size());
  for (uint32_t i = 0; i < key_num; ++i) {
    values_vec[i] = static_cast<CArray*>(values[i]);
  }
  static_cast<ConstelTrainer*>(handle)->Init(keys_in_vec, values_vec);

  API_END();
}

int ConstellationTrainerRank(ConstelTrainerHandle handle, int* rank) {
  API_BEGIN();
  *rank = static_cast<ConstelTrainer*>(handle)->myRank();
  API_END();
}

int ConstellationTrainerNumTrainers(ConstelTrainerHandle handle, int* num) {
  API_BEGIN();
  *num = static_cast<ConstelTrainer*>(handle)->NumTrainers();
  API_END();
}

int ConstelControllerHandleCreate(const char* name, ConstelControllerHandle* handle) {
  API_BEGIN();
  *handle = new ConstelController();
  API_END();
}

int ConstelControllerHandleFree(ConstelControllerHandle handle) {
  API_BEGIN();
  delete static_cast<ConstelController*>(handle);
  API_END();
}

int ConstellationControllerRun(ConstelControllerHandle handle){
  API_BEGIN();
  static_cast<ConstelController*>(handle)->run();
  API_END();
}

int ConstellationIsTrainer(int * is_worker){
  API_BEGIN();
  *is_worker = constellation::is_trainer();
  API_END();
}
