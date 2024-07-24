#ifndef CONSTELLATION_CONSTELLATION_API_H
#define CONSTELLATION_CONSTELLATION_API_H

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

#ifdef __cplusplus
extern "C" {
#endif

/* *brief: create a trainer handle
 * *param: name - the name of the trainer
 * *param: handle - the handle of the trainer
 * *return: 0 - success, -1 - failure
 */
int ConstelTrainerHandleCreate(const char* name, ConstelTrainerHandle* handle);

/* *brief: free the trainer handle
 * *param: handle - the handle of the trainer
 * *return: 0 - success, -1 - failure
 */
int ConstelTrainerHandleFree(ConstelTrainerHandle* handle);

#ifdef __cplusplus
}
#endif

#endif  // CONSTELLATION_CONSTELLATION_API_H
