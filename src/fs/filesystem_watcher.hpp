
/* filesystem_watcher.hpp

Watch over directories and files.

usage: TODO

TODO: docs
 */

#pragma once

#include "shl/number_types.hpp"
#include "shl/enum_flag.hpp"

#include "fs/path.hpp"

#define define_fs_watcher_body(Func, Watcher, Pth, ...)                                                 \
-> decltype(Func(Watcher, ::to_const_string(fs::get_platform_string(Pth)) __VA_OPT__(,) __VA_ARGS__))   \
{                                                                                                       \
    auto pth_str = fs::get_platform_string(Pth);                                                        \
                                                                                                        \
    if constexpr (is_same(void, decltype(Func(Watcher, ::to_const_string(pth_str) __VA_OPT__(,) __VA_ARGS__))))  \
    {                                                                                                   \
        Func(Watcher, ::to_const_string(pth_str) __VA_OPT__(,) __VA_ARGS__);                                     \
                                                                                                        \
        if constexpr (needs_conversion(T))                                                              \
            fs::free(&pth_str);                                                                         \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
        auto ret = Func(Watcher, ::to_const_string(pth_str) __VA_OPT__(,) __VA_ARGS__);                          \
                                                                                                        \
        if constexpr (needs_conversion(T))                                                              \
            fs::free(&pth_str);                                                                         \
                                                                                                        \
        return ret;                                                                                     \
    }                                                                                                   \
}

namespace fs
{
enum class watcher_event_type : u8
{
    None     = 0,
    Removed  = 1 << 0,
    Created  = 1 << 1,
    Modified = 1 << 2,

    MovedFrom = 1 << 3,
    MovedTo   = 1 << 4,

    /*
    Opened,
    Closed,
    AttributesChanged,
    */
};

ENUM_CLASS_FLAG_OPS(watcher_event_type);

typedef void (*watcher_callback_f)(fs::const_fs_string path, fs::watcher_event_type event);

struct filesystem_watcher;

fs::filesystem_watcher *filesystem_watcher_create(fs::watcher_callback_f callback, error *err = nullptr);
bool filesystem_watcher_destroy(fs::filesystem_watcher *watcher, error *err = nullptr);

bool _filesystem_watcher_watch_file(fs::filesystem_watcher *watcher,   fs::const_fs_string path, error *err);

template<typename T>
auto filesystem_watcher_watch_file(fs::filesystem_watcher *watcher, T pth, error *err = nullptr)
    define_fs_watcher_body(fs::_filesystem_watcher_watch_file, watcher, pth, err)

bool _filesystem_watcher_unwatch_file(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err);

template<typename T>
auto filesystem_watcher_watch_unwatch_file(fs::filesystem_watcher *watcher, T pth, error *err = nullptr)
    define_fs_watcher_body(fs::_filesystem_watcher_unwatch_file, watcher, pth, err)

bool filesystem_watcher_unwatch_all(fs::filesystem_watcher *watcher, error *err = nullptr);

// TODO: watch directory

bool filesystem_watcher_has_events(fs::filesystem_watcher *watcher, error *err);
bool filesystem_watcher_process_events(fs::filesystem_watcher *watcher, error *err);
}
