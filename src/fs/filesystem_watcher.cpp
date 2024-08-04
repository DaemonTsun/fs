
// #include "shl/print.hpp"
#define tprint(...)

#include "shl/platform.hpp"
#include "shl/assert.hpp"
#include "fs/path.hpp"

#if Windows
#  include <windows.h>
#elif Linux
#  include "shl/impl/linux/inotify.hpp"
#  include "shl/impl/linux/poll.hpp"
#  include "shl/impl/linux/io.hpp"
#  include "shl/impl/linux/error_codes.hpp"
#endif

#include "shl/io.hpp"

#include "shl/hash_table.hpp"
#include "shl/thread.hpp"
#include "shl/error.hpp"
#include "shl/scratch_buffer.hpp"

#include "fs/filesystem_watcher.hpp"

struct _watched_file
{
#if Windows
    /*
    this is always wstring because ReadDirectoryChanges only has a W variant,
    so it always yields wchar_t, and conversion with each event would be
    more expensive than converting once and storing the converted name here.
    */
    wstring name;
#elif Linux
    fs::path path;
    // io_handle fd;
#endif

    void *userdata;
    fs::watcher_event_type filter;
};

#if Windows
#define WATCHER_BUFFER_SIZE        16384
#define WATCHER_BUFFER_GROWTH_FACTOR   4
#define WATCHER_BUFFER_MAX_SIZE 16777215
#else
#define WATCHER_BUFFER_SIZE          256
#define WATCHER_BUFFER_GROWTH_FACTOR   4
#define WATCHER_BUFFER_MAX_SIZE    65535
#endif

struct _watched_directory
{
    fs::path path;
    void *userdata;
    bool only_watch_files;
    fs::watcher_event_type filter;
    
#if Windows
    hash_table<wstring, _watched_file> watched_files;
    io_handle handle;
    OVERLAPPED overlapped;
    // On Windows, we pretty much need an event buffer per overlapped instance,
    // otherwise ReadDirectoryChangesW would overwrite the same buffer.
    scratch_buffer<WATCHER_BUFFER_SIZE> event_buffer;
#elif Linux
    hash_table<fs::path, _watched_file> watched_files;
    io_handle fd;
#endif
};

struct fs::filesystem_watcher
{
    watcher_callback_f callback;
    hash_table<fs::path, _watched_directory> watched_directories;
    fs::path iterator_path;

#if Linux
    scratch_buffer<WATCHER_BUFFER_SIZE> event_buffer;
    io_handle inotify_fd;
#endif
};

#if Windows
#define WATCH_EVENTS (FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_ATTRIBUTES|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_LAST_ACCESS|FILE_NOTIFY_CHANGE_CREATION)

#define _regenerate_scratch_buffers(WatchedDirs)\
{\
    for_hash_table(_dir, (WatchedDirs))\
    if (_dir->event_buffer.size <= WATCHER_BUFFER_MAX_SIZE)\
        _dir->event_buffer.data = _dir->event_buffer.stack_buffer;\
}

fs::watcher_event_type mask_to_event(u32 mask)
{
    fs::watcher_event_type ret = fs::watcher_event_type::None;

    if (mask == FILE_ACTION_ADDED)    set_flag(ret, fs::watcher_event_type::Created);
    if (mask == FILE_ACTION_MODIFIED) set_flag(ret, fs::watcher_event_type::Modified);
    if (mask == FILE_ACTION_REMOVED)  set_flag(ret, fs::watcher_event_type::Removed);
    
    if (mask == FILE_ACTION_RENAMED_NEW_NAME) set_flag(ret, fs::watcher_event_type::MovedTo);
    if (mask == FILE_ACTION_RENAMED_OLD_NAME) set_flag(ret, fs::watcher_event_type::MovedFrom);
    
    return ret;
}

void _process_event(fs::filesystem_watcher *watcher, _watched_directory *dir, FILE_NOTIFY_INFORMATION *event)
{
    fs::watcher_event_type type = mask_to_event(event->Action);
    fs::path *it = &watcher->iterator_path;
    it->size = 0;
    const_wstring fname{(wchar_t*)event->FileName, event->FileNameLength / sizeof(wchar_t)};
    fs::watcher_event ev{};

    if (dir->only_watch_files)
    {
        _watched_file *file = ::search_by_hash(&dir->watched_files, ::hash(fname));

        if (file == nullptr)
            return;

        if ((file->filter & type) == fs::watcher_event_type::None)
            return;

        type &= file->filter;
        fs::set_path(it, dir->path);
        fs::append_path(it, fname);
        ev.userdata = file->userdata;
    }
    else
    {
        if ((dir->filter & type) == fs::watcher_event_type::None)
            return;

        type &= dir->filter;
        fs::set_path(it, &dir->path);
        fs::append_path(it, fname);
        ev.userdata = dir->userdata;
    }

    ev.path = to_const_string(it);
    ev.event = type;
    watcher->callback(&ev);
}
#elif Linux
static inline s64 _strlen_to_nullterm(const char *t)
{
    s64 ret = 0;
    while (*t != '\0')
    {
        ret += 1;
        t++;
    }

    return ret;
}

fs::watcher_event_type mask_to_event(u32 mask)
{
    fs::watcher_event_type ret = fs::watcher_event_type::None;

    if (mask & IN_CREATE) set_flag(ret, fs::watcher_event_type::Created);
    if (mask & IN_MODIFY) set_flag(ret, fs::watcher_event_type::Modified);
    if (mask & (IN_DELETE | IN_DELETE_SELF)) set_flag(ret, fs::watcher_event_type::Removed);
    
    if (mask & IN_MOVED_TO)   set_flag(ret, fs::watcher_event_type::MovedTo);
    if (mask & IN_MOVED_FROM) set_flag(ret, fs::watcher_event_type::MovedFrom);
    
    return ret;
}

void _process_event(fs::filesystem_watcher *watcher, inotify_event *event)
{
    if (event->name_length <= 0)
        return;

    fs::watcher_event_type type = mask_to_event(event->mask);

    if (event->mask == IN_IGNORED)    return;
    if (event->mask == IN_Q_OVERFLOW) return;
    if (event->mask == IN_UNMOUNT)    return;

    /*
    if (event->mask & IN_ISDIR)         tprint("IN_ISDIR ");
    if (event->mask & IN_ACCESS)        tprint("IN_ACCESS ");
    if (event->mask & IN_ATTRIB)        tprint("IN_ATTRIB ");
    if (event->mask & IN_CLOSE_NOWRITE) tprint("IN_CLOSE_NOWRITE ");
    if (event->mask & IN_CLOSE_WRITE)   tprint("IN_CLOSE_WRITE ");
    if (event->mask & IN_CREATE)        tprint("IN_CREATE ");
    if (event->mask & IN_DELETE)        tprint("IN_DELETE ");
    if (event->mask & IN_DELETE_SELF)   tprint("IN_DELETE_SELF ");
    if (event->mask & IN_MODIFY)        tprint("IN_MODIFY ");
    if (event->mask & IN_MOVE_SELF)     tprint("IN_MOVE_SELF ");
    if (event->mask & IN_MOVED_FROM)    tprint("IN_MOVED_FROM ");
    if (event->mask & IN_MOVED_TO)      tprint("IN_MOVED_TO ");
    if (event->mask & IN_OPEN)          tprint("IN_OPEN ");

    if (event->name_length > 0)
        tprint("   name = %s, %d\n", inotify_event_name(event), event->watched_fd);
    */

    fs::path *it = &watcher->iterator_path;
    it->size = 0;
    bool found = false;
    fs::watcher_event ev{};

    fs::const_fs_string fname{inotify_event_name(event), _strlen_to_nullterm(inotify_event_name(event))};

    for_hash_table(pth, dir, &watcher->watched_directories)
    {
        if (dir->fd != event->watched_fd)
            continue;

        if (dir->only_watch_files)
        {
            _watched_file *file = ::search_by_hash(&dir->watched_files, ::hash(fname));

            if (file == nullptr)
                continue;

            if ((file->filter & type) == fs::watcher_event_type::None)
                continue;

            found = true;
            type &= file->filter;
            fs::set_path(it, &dir->path);
            fs::append_path(it, fname);
            ev.userdata = file->userdata;
            break;
        }
        else
        {
            if ((dir->filter & type) == fs::watcher_event_type::None)
                continue;

            found = true;
            type &= dir->filter;
            fs::set_path(it, &dir->path);
            fs::append_path(it, fname);
            ev.userdata = dir->userdata;
            break;
        }
    }

    if (type == fs::watcher_event_type::None || it->size <= 0)
        return;

    if (!found)
        return;

    ev.path = to_const_string(it);
    ev.event = type;
    watcher->callback(&ev);
}
#endif

fs::filesystem_watcher *fs::filesystem_watcher_create(fs::watcher_callback_f callback, error *err)
{
    assert(callback != nullptr);
    fs::filesystem_watcher *ret = alloc<fs::filesystem_watcher>();

    fill_memory(ret, 0);
    ::init(&ret->watched_directories);
    fs::init(&ret->iterator_path);
    ret->callback = callback;

#if Linux
    ::init(&ret->event_buffer);
    ret->inotify_fd = ::inotify_init1(IN_NONBLOCK);

    if (ret->inotify_fd < 0)
    {
        set_error_by_code(err, -ret->inotify_fd);
        return nullptr;
    }
#endif

    return ret;
}

bool fs::filesystem_watcher_destroy(fs::filesystem_watcher *watcher, error *err)
{
    assert(watcher != nullptr);

    fs::filesystem_watcher_unwatch_all(watcher, err);
    ::free(&watcher->watched_directories);
    fs::free(&watcher->iterator_path);

#if Linux
    if (sys_int code = ::close(watcher->inotify_fd); code < 0)
    {
        set_error_by_code(err, -code);
        return false;
    }
#endif

    dealloc(watcher);

    return true;
}

static _watched_directory *_watch_directory(fs::filesystem_watcher *watcher, fs::path *dirpath, error *err)
{
    _watched_directory *watched_parent = nullptr;

#if Windows
    io_handle dirh = CreateFile(dirpath->c_str(),
                                GENERIC_READ | FILE_LIST_DIRECTORY,
                                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                nullptr,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                nullptr);

    if (dirh == INVALID_IO_HANDLE)
    {
        set_GetLastError_error(err);
        return nullptr;
    }
#elif Linux
    sys_int ifd = ::inotify_add_watch(watcher->inotify_fd, dirpath->c_str(), IN_ALL_EVENTS);

    if (ifd < 0)
    {
        set_error_by_code(err, -ifd);
        return nullptr;
    }
#endif

    watched_parent = ::add_element_by_key(&watcher->watched_directories, dirpath);
    assert(watched_parent != nullptr);
    fill_memory(watched_parent, 0);
    ::init(&watched_parent->watched_files);
    watched_parent->path = *dirpath;
    watched_parent->only_watch_files = true;

#if Windows
    ::init(&watched_parent->event_buffer);
    _regenerate_scratch_buffers(&watcher->watched_directories);
    watched_parent->handle = dirh;
    watched_parent->overlapped.hEvent = ::CreateEvent(nullptr, true, false, nullptr);
    
    // start watching
    ReadDirectoryChangesW(dirh,
                          watched_parent->event_buffer.data,
                          (DWORD)watched_parent->event_buffer.size,
                          false,
                          WATCH_EVENTS,
                          nullptr,
                          &watched_parent->overlapped,
                          nullptr
                          );
#elif Linux
    watched_parent->fd = ifd;
#endif

    return watched_parent;
}

bool fs::_filesystem_watcher_watch_file(fs::filesystem_watcher *watcher, fs::const_fs_string path, fs::watcher_event_type filter, void *userdata, error *err)
{
    assert(watcher != nullptr);

    fs::path fcanon{};
    defer { fs::free(&fcanon); };

    if (!fs::canonical_path(path, &fcanon, err))
        return false;

    fs::path parent_path{};
    fs::set_path(&parent_path, fs::parent_path_segment(&fcanon));
    
    _watched_directory *watched_parent = ::search(&watcher->watched_directories, &parent_path);

    if (watched_parent == nullptr)
    {
        watched_parent = _watch_directory(watcher, &parent_path, err);

        if (watched_parent == nullptr)
        {
            fs::free(&parent_path);
            return false;
        }
    }
    else
        fs::free(&parent_path);

    assert(watched_parent != nullptr);

#if Windows
    // We need a wstring for ReadDirectoryChangesW...
    fs::const_fs_string fname = fs::filename(path);
    wstring wfname{};
    set_string(&wfname, fname);

    _watched_file *watched = ::search(&watched_parent->watched_files, &wfname);

    if (watched != nullptr)
    {
        watched->filter |= filter;
        watched->userdata = userdata;
        ::free(&wfname);
        return true;
    }

    watched = ::add_element_by_key(&watched_parent->watched_files, &wfname);
    assert(watched != nullptr);

    watched->name = wfname;
    watched->filter = filter;
    watched->userdata = userdata;

    return true;

#elif Linux
    fs::const_fs_string fname = fs::filename(&fcanon);
    _watched_file *watched = ::search_by_hash(&watched_parent->watched_files, ::hash(fname));

    if (watched != nullptr)
    {
        watched->filter |= filter;
        watched->userdata = userdata;
        return true;
    }

    fs::path fpname{};
    fs::set_path(&fpname, fname);
    watched = ::add_element_by_key(&watched_parent->watched_files, &fpname);
    assert(watched != nullptr);

    watched->path = fpname;
    watched->filter = filter;
    watched->userdata = userdata;

    return true;
#endif
}

// make sure all watched files were freed
static bool _unwatch_directory(fs::filesystem_watcher *watcher, _watched_directory *dir, error *err)
{
#if Windows
    if (!CloseHandle(dir->handle))
    {
        set_GetLastError_error(err);
        return false;
    }

    if (!CloseHandle(dir->overlapped.hEvent))
    {
        set_GetLastError_error(err);
        return false;
    }

    ::free(&dir->event_buffer);

#elif Linux
    // don't think fd of directory could be negative but doesn't hurt to check
    if (dir->fd >= 0)
    {
        sys_int ret = inotify_rm_watch(watcher->inotify_fd, dir->fd);

        if (ret < 0)
        {
            set_error_by_code(err, -ret);
            return false;
        }
    }
#endif

    fs::free(&dir->path);
    ::free(&dir->watched_files);
    return true;
}

bool fs::_filesystem_watcher_unwatch_file(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err)
{
    assert(watcher != nullptr);

    fs::path fcanon{};
    defer { fs::free(&fcanon); };

    if (!fs::canonical_path(path, &fcanon, err))
        return false;

    fs::path parent_path{};
    fs::set_path(&parent_path, fs::parent_path_segment(&fcanon));
    defer { fs::free(&parent_path); };

    _watched_directory *watched_parent = ::search(&watcher->watched_directories, &parent_path);

    if (watched_parent == nullptr)
        return false;

    fs::const_fs_string fname = fs::filename(path);

#if Windows
    wstring wfname{};
    set_string(&wfname, fname);
    defer { ::free(&wfname); };

    _watched_file *watched = ::search(&watched_parent->watched_files, &wfname);

    if (watched == nullptr)
        return false;

    ::free(&watched->name);
    ::remove_element_by_key(&watched_parent->watched_files, &wfname);

#elif Linux
    _watched_file *watched = ::search_by_hash(&watched_parent->watched_files, ::hash(fname));

    if (watched == nullptr)
        return false;

    fs::free(&watched->path);
    remove_element_by_hash(&watched_parent->watched_files, ::hash(fname));
#endif

    if (watched_parent->only_watch_files
     && watched_parent->watched_files.size == 0)
    {
        if (!_unwatch_directory(watcher, watched_parent, err))
            return false;
        
        ::remove_element_by_key(&watcher->watched_directories, &parent_path);
    }

    return true;
}

bool fs::_filesystem_watcher_watch_directory(fs::filesystem_watcher *watcher, fs::const_fs_string path, fs::watcher_event_type filter, void *userdata, error *err)
{
    assert(watcher != nullptr);

    fs::path fcanon{};

    if (!fs::canonical_path(path, &fcanon, err))
        return false;

    if (!fs::is_directory(&fcanon, true, err))
    {
        fs::free(&fcanon);
        return false;
    }

    _watched_directory *watched_dir = ::search(&watcher->watched_directories, &fcanon);

    if (watched_dir == nullptr)
    {
        watched_dir = _watch_directory(watcher, &fcanon, err);

        if (watched_dir == nullptr)
        {
            fs::free(&fcanon);
            return false;
        }

        watched_dir->filter = filter;
    }
    else
    {
        fs::free(&fcanon);
        watched_dir->filter |= filter;
    }

    assert(watched_dir != nullptr);
    watched_dir->only_watch_files = false;
    watched_dir->userdata = userdata;

    return true;
}

bool fs::_filesystem_watcher_unwatch_directory(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err)
{
    assert(watcher != nullptr);

    fs::path fcanon{};
    defer { fs::free(&fcanon); };

    if (!fs::canonical_path(path, &fcanon, err))
        return false;

    if (!fs::is_directory(&fcanon, true, err))
        return false;

    _watched_directory *watched_dir = ::search(&watcher->watched_directories, &fcanon);

    if (watched_dir == nullptr)
        return false;

    watched_dir->only_watch_files = true;

    if (watched_dir->watched_files.size == 0)
    {
        if (!_unwatch_directory(watcher, watched_dir, err))
            return false;

        ::remove_element_by_key(&watcher->watched_directories, &fcanon);
    }

    return true;
}

bool fs::_filesystem_watcher_watch(fs::filesystem_watcher *watcher, fs::const_fs_string path, fs::watcher_event_type filter, void *userdata, error *err)
{
    if (fs::is_directory(path))
        return fs::_filesystem_watcher_watch_directory(watcher, path, filter, userdata, err);
    else
        return fs::_filesystem_watcher_watch_file(watcher, path, filter, userdata, err);
}

bool fs::_filesystem_watcher_unwatch(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err)
{
    if (fs::is_directory(path))
        return fs::_filesystem_watcher_unwatch_directory(watcher, path, err);
    else
        return fs::_filesystem_watcher_unwatch_file(watcher, path, err);
}

bool fs::filesystem_watcher_unwatch_all(fs::filesystem_watcher *watcher, error *err)
{
    assert(watcher != nullptr);

    bool ok = true;
    bool all_ok = true;

    for_hash_table(watched_dir, &watcher->watched_directories)
    {
        for_hash_table(watched, &watched_dir->watched_files)
        {
#if Windows
            ::free(&watched->name);
#elif Linux
            fs::free(&watched->path);
#endif
        }

        ok = _unwatch_directory(watcher, watched_dir, err);
    }

    ::clear(&watcher->watched_directories);

    return all_ok;
}

bool fs::filesystem_watcher_has_events(fs::filesystem_watcher *watcher, error *err)
{
    assert(watcher != nullptr);

#if Windows
    bool ok = false;

    for_hash_table(dir, &watcher->watched_directories)
    {
        u32 ret = WaitForSingleObject(dir->overlapped.hEvent, 0);
        ok = ret == WAIT_OBJECT_0;

        if (ok)
            break;
    }

    return ok;

#elif Linux
    poll_fd pfd{};
    pfd.fd = watcher->inotify_fd;
    pfd.events = POLLIN;

    sys_int ret = ::poll(&pfd, 1, 0);

    if (ret < 0)
    {
        set_error_by_code(err, -ret);
        return false;
    }

    // 0 = timeout
    // >0 = number of events ready
    return ret != 0;
#endif
}

bool fs::filesystem_watcher_process_events(fs::filesystem_watcher *watcher, error *err)
{
    assert(watcher != nullptr);

#if Windows
    s64 bytes = 0;
    bool ok = false;

    for_hash_table(dir, &watcher->watched_directories)
    {
        ok = true;
        s32 ret = WaitForSingleObject(dir->overlapped.hEvent, 0);

        tprint("single object ret: %\n", ret);

        if (ret != WAIT_OBJECT_0)
            continue;

        ok = GetOverlappedResult(dir->handle, &dir->overlapped, (LPDWORD)&bytes, false);

        if (!ok)
        {
            set_GetLastError_error(err);
            break;
        }

        ok = ResetEvent(dir->overlapped.hEvent);

        if (!ok)
        {
            set_GetLastError_error(err);
            break;
        }

        auto *buffer = &dir->event_buffer;

#define _start_next_ReadDirectoryChanges(Dir)\
        ReadDirectoryChangesW((Dir)->handle,\
                              (Dir)->event_buffer.data,\
                              (DWORD)(Dir)->event_buffer.size,\
                              false,\
                              WATCH_EVENTS,\
                              nullptr,\
                              &(Dir)->overlapped,\
                              nullptr\
                              );

        
        if (bytes == 0)
        {
            // not enough space
            if (buffer->size >= WATCHER_BUFFER_MAX_SIZE)
            {
                set_GetLastError_error(err);
                break;
            }

            tprint("growing %\n", buffer->size);
            ::grow_by(buffer, WATCHER_BUFFER_GROWTH_FACTOR);
            ok = _start_next_ReadDirectoryChanges(dir);

            if (!ok)
            {
                set_GetLastError_error(err);
                break;
            }

            continue;
        }

        s64 i = 0;

        while (i < bytes)
        {
            FILE_NOTIFY_INFORMATION *event = (FILE_NOTIFY_INFORMATION*)(buffer->data + i);
            _process_event(watcher, dir, event);
            i += event->NextEntryOffset;

            if (event->NextEntryOffset == 0)
                break;
        }

        ok = _start_next_ReadDirectoryChanges(dir);
        if (!ok)
        {
            set_GetLastError_error(err);
            break;
        }
    }

    return ok;
#elif Linux
    s64 length = -1;
    s64 i = 0;
    auto *buffer = &watcher->event_buffer;

    do
    {
        length = ::read(watcher->inotify_fd, buffer->data, buffer->size);
        
        if (length == -EINVAL)
        {
            if (buffer->size >= WATCHER_BUFFER_MAX_SIZE)
                break;

            tprint("growing %\n", buffer->size);
            ::grow_by(buffer, WATCHER_BUFFER_GROWTH_FACTOR);
            continue;
        }

        break;
    }
    while (length == -EINVAL);

    if (length < 0)
    {
        set_error_by_code(err, -length);
        return false;
    }

    if (length == 0)
        return true;

    i = 0;

    while (i < length)
    {
        inotify_event *event = (inotify_event*)(buffer->data + i);
        _process_event(watcher, event);
        i += sizeof(inotify_event) + event->name_length;
    }

    return true;
#endif
}
