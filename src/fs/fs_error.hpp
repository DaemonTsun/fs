

#pragma once

#include "shl/error.hpp"

namespace fs
{
struct fs_error : public ::error
{
    // fs info?
};

#define get_fs_error(ERR, FMT, ...) \
    do { if (ERR != nullptr) { *ERR = fs::fs_error{.what = ::format_error(FMT __VA_OPT__(,) __VA_ARGS__), .file = __FILE__, .line = __LINE__}; } } while (0)
}
