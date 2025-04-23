#ifndef _CONSTELLATION_H_
#define _CONSTELLATION_H_
#include "ps/ps.h"

#include "./constellation_controller.h"
#include "./constellation_trainer.h"
namespace constellation {
bool is_trainer() {
  const char* role_str = ps::Environment::Get()->find("DMLC_ROLE");
  return (role_str == nullptr) || (!strcmp(role_str, "trainer"));
}
}  // namespace constellation

#endif  // _CONSTELLATION_H_