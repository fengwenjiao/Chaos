#ifndef CONSTELLATION_CONSTELLATION_API_H
#define CONSTELLATION_CONSTELLATION_API_H

#include <cstdint>

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
                                     int size DEFAULT(0),
                                     int dtype DEFAULT(0));

/** @brief create a CArray handle
 * @param handle - the handle of the CArray
 * @param data - the data of the CArray, must be allocated by the caller. Usually borrowed from
 * other AI framework, such as torch.tensor
 * @param size - the size of the CArray
 * @param dtype - the data type of the CArray
 * @return 0 - success, -1 - failure
 */
int ConstellationCArrayCreate(ConstellationCArrayHandle* handle,
                              void* data,
                              int size,
                              int dtype DEFAULT(0));

/** @brief free the CArray handle
 * @param handle - the handle of the CArray
 * @return 0 - success, -1 - failure
 */
int ConstellationCArrayFree(ConstellationCArrayHandle handle);

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

/** @brief get the rank of the trainer
 * @param handle - the handle of the trainer
 * @param rank - the rank of the trainer
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerRank(ConstelTrainerHandle handle, int* rank);

/** @brief get the number of trainers
 * @param handle - the handle of the trainer
 * @param num - the number of trainers
 * @return 0 - success, -1 - failure
 */
int ConstellationTrainerNumTrainers(ConstelTrainerHandle handle, int* num);


////////Controller ////////
/** @brief create a controller handle
 * @param name - the name of the controller
 * @param handle - the handle of the controller
 * @return 0 - success, -1 - failure
 */
int ConstelControllerHandleCreate(const char* name, ConstelControllerHandle* handle);

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
int ConstellationIsTrainer(int * is_worker);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // CONSTELLATION_CONSTELLATION_API_H
