#ifndef BASE_H
#define BASE_H
namespace moniter
{
    /**
     * \brief the signal of gather static info of all clients 
     * mixed info can  be combined:
     * - kSignalStatic + kSignalDynamic means gather dynamic and sattic info at the same time
     * */
    static const int kSignalStatic = 1;
    /**
     * \brief the signal of gather dynamic info of all clients 
     */
    static const int kSignalDynamic = 2;
    /**
     * \brief the signal of gather bandwidth info of all clients 
     */
    static const int kSignalBandwidth = 4;
} // namespace base
#endif // BASE_H_