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
  static_cast<ConstelTrainer*>(handle)->Broadcast(keys_in_vec, values_vec);

  API_END();
}

int ConstellationTrainerRecv(ConstelTrainerHandle handle,
                             const int* keys_in,
                             ConstellationCArrayHandle* values,
                             uint32_t num) {
  API_BEGIN();
  std::vector<int> keys_in_vec(keys_in, keys_in + num);
  std::vector<CArray*> values_vec(num);
  for (uint32_t i = 0; i < num; ++i) {
    values_vec[i] = static_cast<CArray*>(values[i]);
  }
  static_cast<ConstelTrainer*>(handle)->Recv(keys_in_vec, values_vec);
  API_END();
}

int ConstellationTrainerIsScale(ConstelTrainerHandle handle, int* is_scale) {
  API_BEGIN();
  *is_scale = static_cast<ConstelTrainer*>(handle)->is_scale() ? 1 : 0;
  API_END();
}

int ConstelTrainerNotifyReadyAndWait(ConstelTrainerHandle handle,
                                     const int need_sycn_model,
                                     const int* keys,
                                     const uint64_t* lens,
                                     const int key_num) {
  API_BEGIN();
  std::vector<int> keys_vec;
  std::vector<uint64_t> lens_vec;
  if (key_num != 0) {
    keys_vec.assign(keys, keys + key_num);
    lens_vec.assign(lens, lens + key_num);
  }
  bool need_sycn_model_bool = need_sycn_model == 1 ? true : false;
  static_cast<ConstelTrainer*>(handle)->NotifyReadyAndWait(
      need_sycn_model_bool, keys_vec, lens_vec);
  API_END();
}

int ConstelTrainerBatchEnd(ConstelTrainerHandle handle, int* keys_size) {
  API_BEGIN();
  auto& keys_to_migrate = CtypesInfoBuffer::Get()->GetKeysToMigrate();
  static_cast<ConstelTrainer*>(handle)->BatchEnd(&keys_to_migrate);
  *keys_size = keys_to_migrate.size();
  API_END();
}

int ConstelTrainerGetKeysToMigrate(int* keys, const int keys_size) {
  API_BEGIN();
  auto& keys_to_migrate = CtypesInfoBuffer::Get()->GetKeysToMigrate();
  if (keys_size < keys_to_migrate.size()) {
    throw std::runtime_error(
        "keys_size is smaller than keys_to_migrate.size(). Please call ConstelTrainerBatchEnd "
        "to get the correct size of keys_to_migrate.");
  }
  for (size_t i = 0; i < keys_to_migrate.size(); ++i) {
    keys[i] = keys_to_migrate[i];
  }
  API_END();
}

int ConstelTrainerMigrate(ConstelTrainerHandle handle,
                          uint32_t key_num,
                          const int* keys_in,
                          ConstellationCArrayHandle* values) {
  API_BEGIN();
  std::vector<int> keys_in_vec(keys_in, keys_in + key_num);
  std::vector<CArray> values_vec(key_num);
  for (uint32_t i = 0; i < key_num; ++i) {
    values_vec[i] = *static_cast<CArray*>(values[i]);
  }
  static_cast<ConstelTrainer*>(handle)->Migrate(keys_in_vec, values_vec);
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

int ConstellationControllerRun(ConstelControllerHandle handle) {
  API_BEGIN();
  static_cast<ConstelController*>(handle)->run();
  API_END();
}

int ConstellationIsTrainer(int* is_worker) {
  API_BEGIN();
  *is_worker = constellation::is_trainer();
  API_END();
}
