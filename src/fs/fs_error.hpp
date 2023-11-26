
#pragma once

#include "shl/error.hpp"

namespace fs
{
struct fs_error
{
    const char *what;

#ifndef NDEBUG
    const char *file;
    unsigned long line;
#endif

    int error_code;
};

#ifndef NDEBUG
#define set_fs_error(ERR, Errno, MSG) \
    do { if ((ERR) != nullptr) { *(ERR) = fs::fs_error{.what = MSG, .file = __FILE__, .line = __LINE__, .error_code = Errno}; } } while (0)

#define set_fs_errno_error(ERR) \
    do { if ((ERR) != nullptr) { int _code = (errno); *(ERR) = fs::fs_error{.what = ::strerror(_code), .file = __FILE__, .line = __LINE__, .error_code = _code}; } } while (0)

#else
#define set_fs_error(ERR, Errno, MSG) \
    do { if ((ERR) != nullptr) { *(ERR) = fs::fs_error{.what = MSG, .error_code = Errno}; } } while (0)

#define set_fs_errno_error(ERR) \
    do { if ((ERR) != nullptr) { int _code = (errno); *(ERR) = fs::fs_error{.what = ::strerror(_code), .error_code = _code}; } } while (0)

#endif

#define format_fs_error(ERR, Errno, FMT, ...) \
    set_fs_error(ERR, Errno, ::format_error_message(FMT __VA_OPT__(,) __VA_ARGS__))
}
