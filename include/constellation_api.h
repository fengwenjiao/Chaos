#ifndef CONSTELLATION_CONSTELLATION_API_H
#define CONSTELLATION_CONSTELLATION_API_H

#include <cstdint>
#include <memory>

#ifdef __cplusplus
#define DEFAULT(x) = x
#else
#define DEFAULT(x)
#endif

#define API_BEGIN() try {
#define API_END()                                        \
  }                                                      \
  catch (const std::exception& e) {                      \
    std::cerr << "Exception: " << e.what() << std::endl; \
    return -1;                                           \
  };                                                     \
                                                         \
  return 0;

// type definitions
typedef void* ConstelTrainerHandle;
typedef void* ConstelControllerHandle;
typedef void* ConstellationCArrayHandle;
typedef void* ConstellationCArrayDataPtrType;

class CtypesInfoBuffer {
 public:
  static inline CtypesInfoBuffer* Get() {
    return Get(nullptr).get();
  }

  std::vector<int>& GetKeysToMigrate() {
    return keys_to_migrate_;
  }

  ~CtypesInfoBuffer() = default;

 private:
  CtypesInfoBuffer() {}
  static inline std::shared_ptr<CtypesInfoBuffer> Get(void*) {
    static std::shared_ptr<CtypesInfoBuffer> _prt(new CtypesInfoBuffer());
    return _prt;
  }
  std::vector<int> keys_to_migrate_;
};

#ifdef __cplusplus
extern "C" {
#endif

/** @brief create a trainer handle
 * @param name - the name of the trainer
 * @param handle - the handle of the trainer
 * @return 0 - success, -1 - failure
 */
int ConstelTrainerHandleCreate(const char* name, ConstelTrainerHandle* handle);

/** @brief free the trainer handle
 * @param handle - the handle of the trainer
 * @return 0 - success, -1 - failure
 */
int ConstelTrainerHandleFree(ConstelTrainerHandle handle);

//////////// CArray ////////
/** @brief  create a CArray handle
 * @param handle - the handle of the CArray
 * @param size - the size of the CArray
 * @param dtype - the data type of the CArray
 * @return 0 - success, -1 - failure
 */
int ConstellationCArrayCreateDefault(ConstellationCArrayHandle* handle,
                                     uint64_t size DEFAULT(0),
                                     int dtype DEFAULT(0));

/** @brief create a CArray handle
 * @param handle - the handle of the CArray
 * @param data - the data of the CArray, must be allocated by the caller.
 * Usually borrowed from other AI framework, such as torch.tensor
 * @param size - the size of the CArray
 * @param dtype - the data type of the CArray
 * @return 0 - success, -1 - failure
 */
int ConstellationCArrayCreate(ConstellationCArrayHandle* handle,
                              void* data,
                              uint64_t size,
                              int dtype DEFAULT(0));

/** @brief free the CArray handle
 * @param handle - the handle of the CArray
 * @return 0 - success, -1 - failure
 */
int ConstellationCArrayFree(ConstellationCArrayHandle handle);

int ConstellationCArrayDataPtr(ConstellationCArrayHandle* handle,
                               ConstellationCArrayDataPtrType* data);

////// Trainer //////
/** @brief push and pull the data from the trainer
 * @param handle - the handle of the trainer
 * @param key_num - the number of keys to push and pull
 * @param keys_in - the keys to push
 * @param key_num_out - the number of keys to pull
 * @param keys_out - the keys to pull
 * @param values - the values to push
 * @param outs - the values to pull
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerPushPull(ConstelTrainerHandle handle,
                                 uint32_t key_num,
                                 const int* keys_in,
                                 uint32_t key_num_out,
                                 const int* keys_out,
                                 ConstellationCArrayHandle* values,
                                 ConstellationCArrayHandle* outs);

/** @brief init the trainer
 * @param handle - the handle of the trainer
 * @param key_num - the number of keys to init
 * @param keys_in - the keys to init
 * @param values - the values to init
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerInit(ConstelTrainerHandle handle,
                             uint32_t key_num,
                             const int* keys_in,
                             ConstellationCArrayHandle* values);

/** @brief recv the data from the trainer
 * @param handle - the handle of the trainer
 * @param keys_in - the keys to recv
 * @param values - the values to recv
 * @param num - the number of keys to recv
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerRecv(ConstelTrainerHandle handle,
                             const int* keys_in,
                             ConstellationCArrayHandle* values,
                             uint32_t num);

/** @brief check if the trainer is scale
 * @param handle - the handle of the trainer
 * @param is_scale - the flag to indicate if the trainer is scale
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerIsScale(ConstelTrainerHandle handle, int* is_scale);

/** @brief get the node transport topology of the trainer
 * @param handle - the handle of the trainer
 * @param parent - the parent of the trainer
 * @param children - the children of the trainer
 * @param children_size - the size of the children
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerGetNodeTransTopo(ConstelTrainerHandle handle,
                                         char* buffer,
                                         uint32_t size);

/** @brief get the timestamp of the trainer
 * @param handle - the handle of the trainer
 * @param timestamp - the timestamp of the trainer
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerGetTimestamp(ConstelTrainerHandle handle,
                                     uint32_t* timestamp);

/** @brief send the ready signal to the trainer
 * @param handle - the handle of the trainer
 * @return 0 - success, -1 - failure
 */
int ConstelTrainerNotifyReadyAndWait(ConstelTrainerHandle handle,
                                     const int need_sycn_model,
                                     const int* keys DEFAULT(nullptr),
                                     const uint64_t* lens DEFAULT(nullptr),
                                     const int key_num DEFAULT(0));

/** @brief end the batch of the trainer
 * @param handle - the handle of the trainer
 * @param keys_to_migrate - the keys to migrate
 * to tell the trainer which keys to migrate
 * @param key_num - the number of keys to migrate
 * @return 0 - success, -1 - failure
 */
int ConstelTrainerBatchEnd(ConstelTrainerHandle handle,
                           int* keys_size DEFAULT(nullptr));

/** @brief get the keys to migrate, must be called after ConstelTrainerBatchEnd
 * @param keys - the keys to migrate
 * @param keys_size - the size of the keys
 * @return 0 - success, -1 - failure
 */
int ConstelTrainerGetKeysToMigrate(int* keys, int keys_size);

/** @brief migrate the keys and values to the trainer
 * @param handle - the handle of the trainer
 * @param key_num - the number of keys to migrate
 * @param keys_in - the keys to migrate
 * @param values - the values to migrate
 * @return 0 - success, -1 - failure
 */
int ConstelTrainerMigrate(ConstelTrainerHandle handle,
                          uint32_t key_num,
                          const int* keys_in,
                          ConstellationCArrayHandle* values);

/** @brief get the rank of the trainer
 * @param handle - the handle of the trainer
 * @param rank - the rank of the trainer
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerRank(ConstelTrainerHandle handle, int* rank);

/** @brief get the id of the trainer
 * @param handle - the handle of the trainer
 * @param myid - the id of the trainer
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerMyid(ConstelTrainerHandle handle, int* myid);

/** @brief get the number of trainers
 * @param handle - the handle of the trainer
 * @param num - the number of trainers
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerNumTrainers(ConstelTrainerHandle handle, int* num);

////////Controller ////////
/** @brief create a controller handle
 * @param thinker_name - the name of the thinker
 * @param handle - the handle of the controller
 * @return 0 - success, -1 - failure
 */
int ConstelControllerHandleCreate(ConstelControllerHandle* handle,
                                  const char* thinker_name = nullptr);

int ConstelControllerSetThinker(ConstelControllerHandle handle,
                                const char* thinker_name);

/** @brief free the controller handle
 * @param handle - the handle of the controller
 * @return 0 - success, -1 - failure
 */
int ConstelControllerHandleFree(ConstelControllerHandle handle);

/** @brief run the controller
 * @param handle - the handle of the controller
 * @return 0 - success, -1 - failure
 */
int ConstellationControllerRun(ConstelControllerHandle handle);

/////// Info //////
/** @brief check if the current process is a worker
 * @param is_worker - the flag to indicate if the current process is a worker
 * @return
 */
int ConstellationIsTrainer(int* is_worker);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // CONSTELLATION_CONSTELLATION_API_H
