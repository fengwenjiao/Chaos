#ifndef MONITER_BASE_H
#define MONITER_BASE_H
namespace moniter {
static const int kSignalRegister = 0;
/**
 * \brief the signal of gather static info of all clients
 * mixed info can  be combined:
 * - kSignalStatic + kSignalDynamic means gather dynamic and sattic info at the
 * same time
 * */
static const int kSignalStatic = 1;
/**
 * \brief the signal of gather dynamic info of all clients
 */
static const int kSignalDynamic = 2;
/**
 * \brief the signal of gather network info of all clients
 */
static const int kSignalNetwork = 4;
}  // namespace moniter
#endif  // MONITER_BASE_H