
#include "fs/common.hpp"

#if Linux
bool fs::operator< (fs::filesystem_timestamp lhs, fs::filesystem_timestamp rhs) 
{
    if (lhs.tv_sec < rhs.tv_sec)
        return true;

    if (lhs.tv_sec > rhs.tv_sec)
        return false;

    return lhs.tv_nsec < rhs.tv_nsec;
}

bool fs::operator> (fs::filesystem_timestamp lhs, fs::filesystem_timestamp rhs) { return rhs < lhs; }
bool fs::operator<=(fs::filesystem_timestamp lhs, fs::filesystem_timestamp rhs) { return !(rhs < lhs); }
bool fs::operator>=(fs::filesystem_timestamp lhs, fs::filesystem_timestamp rhs) { return !(lhs < rhs); }
#endif
