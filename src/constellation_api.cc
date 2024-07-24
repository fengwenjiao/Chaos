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