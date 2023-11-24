

#pragma once

namespace fs
{
struct fs_error
{
    const char *what;
    const char *file;
    unsigned long line;
    int error_code;
};

#define get_fs_error(ERR, Errno, FMT, ...) \
    do { if (ERR != nullptr) { *ERR = fs::fs_error{.what = ::format_error(FMT __VA_OPT__(,) __VA_ARGS__), .file = __FILE__, .line = __LINE__, .error_code = Errno}; } } while (0)

#define get_fs_errno_error(ERR) \
    do { if (ERR != nullptr) { int _code = (errno); *ERR = fs::fs_error{.what = strerror(_code), .file = __FILE__, .line = __LINE__, .error_code = _code}; } } while (0)
}
