
/* filesystem_watcher.hpp

Watch over directories and files.
The filesystem_watcher takes a callback function and runs in another thread
after starting.

usage:

   void callback_function(const char *path, fs::watcher_event_type event)
   { ... }

   fs::filesystem_watcher *watcher;
   fs::create_filesystem_watcher(&watcher, callback_function);

   fs::watch_file(watcher, "/etc/shadow");

   fs::start_filesystem_watcher(watcher);

   // do other things

   fs::stop_filesystem_watcher(watcher);
   fs::destroy_filesystem_watcher(watcher);
 */

#pragma once

#include "shl/number_types.hpp"
#include "shl/enum_flag.hpp"

#include "fs/path.hpp"

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

typedef void (*watcher_callback_f)(fs::const_fs_string path, fs::watcher_event_type event);

struct filesystem_watcher;

fs::filesystem_watcher *filesystem_watcher_create(fs::watcher_callback_f callback, error *err = nullptr);
bool filesystem_watcher_destroy(fs::filesystem_watcher *watcher, error *err = nullptr);

bool filesystem_watcher_watch_file(fs::filesystem_watcher *watcher,   fs::const_fs_string path, error *err = nullptr);
bool filesystem_watcher_unwatch_file(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err = nullptr);
bool filesystem_watcher_unwatch_all(fs::filesystem_watcher *watcher, error *err = nullptr);

// TODO: watch directory

bool filesystem_watcher_has_events(fs::filesystem_watcher *watcher, error *err);
bool filesystem_watcher_process_events(fs::filesystem_watcher *watcher, error *err);
}
