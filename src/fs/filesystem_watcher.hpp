
/* filesystem_watcher.hpp

Watch over directories and files.

Example usage:

    void callback(fs::watcher_event *e)
    {
        printf("%d %s %p\n", value(e->event), e->path.c_str);
    }
    ...

    fs::filesystem_watcher *watcher;
    watcher = fs::filesystem_watcher_create(callback);

    fs::filesystem_watcher_watch(watcher, "some_directory");

    while (true)
    {
        if (fs::filesystem_watcher_has_events(watcher))
            fs::filesystem_watcher_process_events(watcher);

        // if loop condition, etc
        break;
    }

    fs::filesystem_watcher_destroy(watcher);

Types:

enum class watcher_event_type
    The filesystem event type.
    
struct filesystem_watcher
    Opaque type, manages all the memory and handles necessary for watching
    files and directories.

typedef void (*watcher_callback_f)(watcher_event)
    The callback function type for a filesystem_watcher.


Functions:

filesystem_watcher_create(Callback[, error])
    Creates a new filesystem_watcher and returns it. Callback is stored in the
    watcher and is called whenever filesystem_watcher_process_events is called
    and has events to process.
    On error, returns nullptr.

filesystem_watcher_destroy(Watcher[, error])
    Destroys the given filesystem watcher Watcher and frees up any memory used
    and closes any OS handles.
    Returns whether or not the function succeeded.

filesystem_watcher_watch_file(Watcher, Path, Filter = All, Userdata = nullptr[, error])
    Adds the file at Path to Watcher to be watched.
    Watcher will report events (via the Callback when calling
    filesystem_watcher_process_events) if they match the event filter
    Filter (watcher_event_type).
    If the file at Path is already being watched, Filter will be added
    to the existing filter.
    Returns whether or not the function succeeded.

filesystem_watcher_unwatch_file(Watcher, Path[, error])
    Stops watching the file at Path from Watcher.
    Returns whether or not the function succeeded.

filesystem_watcher_watch_directory(Watcher, Path, Filter = All, Userdata = nullptr[, error])
    Adds the directory at Path to Watcher to be watched.
    Watcher will report events (via the Callback when calling
    filesystem_watcher_process_events) if they match the event filter
    Filter (watcher_event_type).
    If the directory at Path is already being watched, Filter will be added
    to the existing filter.
    Returns whether or not the function succeeded.

filesystem_watcher_unwatch_directory(Watcher, Path[, error])
    Stops watching the directory at Path from Watcher.
    Returns whether or not the function succeeded.

filesystem_watcher_watch(Watcher, Path, Filter = All, Userdata = nullptr[, error])
    If Path points to a directory, returns
    filesystem_watcher_watch_directory(Watcher, Path, Filter, error),
    otherwise, returns
    filesystem_watcher_watch_file(Watcher, Path, Filter, error).

filesystem_watcher_unwatch(Watcher, Path[, error])
    If Path points to a directory, returns
    filesystem_watcher_unwatch_directory(Watcher, Path, error),
    otherwise, returns
    filesystem_watcher_unwatch_file(Watcher, Path, error).

filesystem_watcher_unwatch_all(Watcher[, error])
    Stops watching all watched files and directories. 
    Returns whether or not the function succeeded.

filesystem_watcher_has_events(Watcher[, error])
    Returns whether there are any filesystem events to be processed.
    NOTE: Does not check whether any of the available events match the filters
    in Watcher, so it is possible that filesystem_watcher_has_events returns true
    but filesystem_watcher_process_events does not call Callback with any events.

filesystem_watcher_process_events(watcher[, error])
    Processes any events since the last call to filesystem_watcher_process_events.
    The watcher callback will be called for any event that matches the filter of
    watched files or directories.
    Returns whether or not the function succeeded.
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

    All = Removed | Created | Modified | MovedFrom | MovedTo
};

struct watcher_event
{
    fs::const_fs_string path;
    fs::watcher_event_type event;
    void *userdata;
};

enum_flag(watcher_event_type);

typedef void (*watcher_callback_f)(fs::watcher_event *event);

struct filesystem_watcher;

fs::filesystem_watcher *filesystem_watcher_create(fs::watcher_callback_f callback, error *err = nullptr);
bool filesystem_watcher_destroy(fs::filesystem_watcher *watcher, error *err = nullptr);

// file
bool _filesystem_watcher_watch_file(fs::filesystem_watcher *watcher, fs::const_fs_string path, fs::watcher_event_type filter, void *userdata, error *err);

template<typename T>
auto filesystem_watcher_watch_file(fs::filesystem_watcher *watcher, T pth, fs::watcher_event_type filter = fs::watcher_event_type::All, void *userdata = nullptr, error *err = nullptr)
    define_fs_watcher_body(fs::_filesystem_watcher_watch_file, watcher, pth, filter, userdata, err)

bool _filesystem_watcher_unwatch_file(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err);

template<typename T>
auto filesystem_watcher_unwatch_file(fs::filesystem_watcher *watcher, T pth, error *err = nullptr)
    define_fs_watcher_body(fs::_filesystem_watcher_unwatch_file, watcher, pth, err)

// directory
bool _filesystem_watcher_watch_directory(fs::filesystem_watcher *watcher, fs::const_fs_string path, fs::watcher_event_type filter, void *userdata, error *err);

template<typename T>
auto filesystem_watcher_watch_directory(fs::filesystem_watcher *watcher, T pth, fs::watcher_event_type filter = fs::watcher_event_type::All, void *userdata = nullptr, error *err = nullptr)
    define_fs_watcher_body(fs::_filesystem_watcher_watch_directory, watcher, pth, filter, userdata, err)

bool _filesystem_watcher_unwatch_directory(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err);

template<typename T>
auto filesystem_watcher_unwatch_directory(fs::filesystem_watcher *watcher, T pth, error *err = nullptr)
    define_fs_watcher_body(fs::_filesystem_watcher_unwatch_directory, watcher, pth, err)

// unspecific
bool _filesystem_watcher_watch(fs::filesystem_watcher *watcher, fs::const_fs_string path, fs::watcher_event_type filter, void *userdata, error *err);

template<typename T>
auto filesystem_watcher_watch(fs::filesystem_watcher *watcher, T pth, fs::watcher_event_type filter = fs::watcher_event_type::All, void *userdata = nullptr, error *err = nullptr)
    define_fs_watcher_body(fs::_filesystem_watcher_watch, watcher, pth, filter, userdata, err)

bool _filesystem_watcher_unwatch(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err);

template<typename T>
auto filesystem_watcher_unwatch(fs::filesystem_watcher *watcher, T pth, error *err = nullptr)
    define_fs_watcher_body(fs::_filesystem_watcher_unwatch, watcher, pth, err)

// all
bool filesystem_watcher_unwatch_all(fs::filesystem_watcher *watcher, error *err = nullptr);

bool filesystem_watcher_has_events(fs::filesystem_watcher *watcher, error *err = nullptr);
bool filesystem_watcher_process_events(fs::filesystem_watcher *watcher, error *err = nullptr);

#undef define_fs_watcher_body
}
