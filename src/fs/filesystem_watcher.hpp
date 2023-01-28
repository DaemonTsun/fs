
#pragma once

#include "shl/number_types.hpp"
#include "shl/enum_flag.hpp"

namespace fs
{
enum class watcher_event_type : u8
{
    None     = 0,
    Removed  = 1 << 0,
    Created  = 1 << 1,
    Modified = 1 << 2,

    /*
    Opened,
    Closed,
    AttributesChanged,
    Renamed / moved,
    */
};

ENUM_CLASS_FLAG_OPS(watcher_event_type);

typedef void (*watcher_callback_f)(const char *path, fs::watcher_event_type event);

struct filesystem_watcher;

void create_filesystem_watcher(fs::filesystem_watcher **out, fs::watcher_callback_f callback);

void watch_file(fs::filesystem_watcher *watcher, const char *path);
void unwatch_file(fs::filesystem_watcher *watcher, const char *path);

// TODO: watch directory

void start_filesystem_watcher(fs::filesystem_watcher *watcher);
void stop_filesystem_watcher(fs::filesystem_watcher *watcher);

void destroy_filesystem_watcher(fs::filesystem_watcher *watcher);
}
