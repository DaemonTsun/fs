
#pragma once

/* filesystem_watcher.hpp
 *
 * watch over directories and files.
 * the filesystem_watcher takes a callback function and runs in another thread
 * after starting.
 *
 * usage:
 *
 *    void callback_function(const char *path, fs::watcher_event_type event)
 *    { ... }
 *
 *    fs::filesystem_watcher *watcher;
 *    fs::create_filesystem_watcher(&watcher, callback_function);
 *
 *    fs::watch_file(watcher, "/etc/shadow");
 *
 *    fs::start_filesystem_watcher(watcher);
 *
 *    // do other things
 *
 *    fs::stop_filesystem_watcher(watcher);
 *    fs::destroy_filesystem_watcher(watcher);
 */

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
