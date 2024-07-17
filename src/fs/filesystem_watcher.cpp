
//#include "shl/print.hpp"
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
    io_handle fd;
#endif

    // TODO: filter
};

#define WATCHER_BUFFER_SIZE          256
#define WATCHER_BUFFER_GROWTH_FACTOR   4
#define WATCHER_BUFFER_MAX_SIZE    65535

struct _watched_directory
{
    fs::path path;
    
#if Windows
    hash_table<wstring, _watched_file> watched_files;
    io_handle handle;
    OVERLAPPED overlapped;
    scratch_buffer<WATCHER_BUFFER_SIZE> event_buffer;
#elif Linux
    hash_table<fs::path, _watched_file> watched_files;
    /* On Linux, inotify will report changes on both the
       watched directory and it's descendants. If we only
       care about files, only_watch_files will be true.
    */
    io_handle fd;
    bool only_watch_files;
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

    if (mask & FILE_ACTION_ADDED)    set_flag(ret, fs::watcher_event_type::Created);
    if (mask & FILE_ACTION_MODIFIED) set_flag(ret, fs::watcher_event_type::Modified);
    if (mask & FILE_ACTION_REMOVED)  set_flag(ret, fs::watcher_event_type::Removed);
    
    if (mask & FILE_ACTION_RENAMED_NEW_NAME) set_flag(ret, fs::watcher_event_type::MovedTo);
    if (mask & FILE_ACTION_RENAMED_OLD_NAME) set_flag(ret, fs::watcher_event_type::MovedFrom);
    
    return ret;
}

void _process_event(fs::filesystem_watcher *watcher, _watched_directory *dir, FILE_NOTIFY_INFORMATION *event)
{
    fs::watcher_event_type type = mask_to_event(event->Action);
    const_wstring fname{(wchar_t*)event->FileName, event->FileNameLength / sizeof(wchar_t)};

    _watched_file *watched = ::search_by_hash(&dir->watched_files, ::hash(fname));

    /*
    tprint("looking for %, len %, hash %x\n", fname, fname.size, ::hash(fname));
    for_hash_table(k, wfile, e, &dir->watched_files)
        tprint("watching: % (%x)\n", wfile->name, e->hash);
    */

    if (watched == nullptr)
        return;

    // TODO: check filter
    fs::set_path(&watcher->iterator_path, dir->path);
    fs::append_path(&watcher->iterator_path, fname);
    watcher->callback(to_const_string(watcher->iterator_path), type);
}
#elif Linux
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
    fs::watcher_event_type type = mask_to_event(event->mask);

    // TODO: remove
    if (event->mask == IN_IGNORED)    return;
    if (event->mask == IN_Q_OVERFLOW) return;
    if (event->mask == IN_UNMOUNT)    return;

    /*
    if (event->mask & IN_ISDIR)         printf("IN_ISDIR ");
    if (event->mask & IN_ACCESS)        printf("IN_ACCESS ");
    if (event->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
    if (event->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
    if (event->mask & IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
    if (event->mask & IN_CREATE)        printf("IN_CREATE ");
    if (event->mask & IN_DELETE)        printf("IN_DELETE ");
    if (event->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
    if (event->mask & IN_MODIFY)        printf("IN_MODIFY ");
    if (event->mask & IN_MOVE_SELF)     printf("IN_MOVE_SELF ");
    if (event->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
    if (event->mask & IN_MOVED_TO)      printf("IN_MOVED_TO ");
    if (event->mask & IN_OPEN)          printf("IN_OPEN ");

    if (event->len > 0)
        printf("   name = %s\n", event->name);
    */

    fs::path *it = &watcher->iterator_path;
    it->size = 0;

    for_hash_table(pth, dir, &watcher->watched_directories)
    {
        if (dir->fd == event->watched_fd)
        {
            if (dir->only_watch_files)
            {
                if (event->name_length <= 0)
                    // we only care for modified files
                    continue;

                fs::set_path(it, &dir->path);
                fs::append_path(it, inotify_event_name(event));

                if (!contains(&dir->watched_files, it))
                    continue;

                
            }
            else
            {
                // TODO: directory events
            }
        }
        else
        {
            _watched_file *file = nullptr;

            for_hash_table(wf, &dir->watched_files)
            if (wf->fd == event->watched_fd)
            {
                file = wf;
                break;
            }

            if (file == nullptr)
                continue;

            // TODO: filter
            fs::set_path(it, file->path);
            break;
        }
    }

    if (type == fs::watcher_event_type::None || it->size <= 0)
        return;

    watcher->callback(to_const_string(it), type);
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

bool fs::_filesystem_watcher_watch_file(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err)
{
    // TODO: add event filter
    assert(watcher != nullptr);

#if Windows
    fs::path fcanon{};
    fs::canonical_path(path, &fcanon);
    defer { fs::free(&fcanon); };

    fs::path parent_path{};
    fs::set_path(&parent_path, fs::parent_path_segment(&fcanon));

    _watched_directory *watched_parent = ::search(&watcher->watched_directories, &parent_path);

    if (watched_parent == nullptr)
    {
        io_handle dirh = CreateFile(parent_path.c_str(),
                                    GENERIC_READ | FILE_LIST_DIRECTORY,
                                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                    nullptr);

        if (dirh == INVALID_IO_HANDLE)
        {
            fs::free(&parent_path);
            set_GetLastError_error(err);
            return false;
        }

        watched_parent = ::add_element_by_key(&watcher->watched_directories, &parent_path);
        assert(watched_parent != nullptr);
        _regenerate_scratch_buffers(&watcher->watched_directories);
        fill_memory(watched_parent, 0);
        ::init(&watched_parent->watched_files);
        ::init(&watched_parent->event_buffer);
        watched_parent->path = parent_path;
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
    }
    else
        fs::free(&parent_path);

    // We need a wstring for ReadDirectoryChangesW...
    fs::const_fs_string fname = fs::filename(path);
    wstring wfname{};
    set_string(&wfname, fname);

    tprint("watching %, size %, hash %x\n", wfname, wfname.size, ::hash(wfname));

    _watched_file *watched = ::search(&watched_parent->watched_files, &wfname);

    if (watched != nullptr)
    {
        // TODO: update filter
        ::free(&wfname);
        return true;
    }

    watched = ::add_element_by_key(&watched_parent->watched_files, &wfname);
    assert(watched != nullptr);

    // TODO: add filter
    watched->name = wfname;

    return true;

#elif Linux
    fs::path fcanon{};
    fs::canonical_path(path, &fcanon);

    fs::path parent_path{};
    fs::set_path(&parent_path, fs::parent_path_segment(&fcanon));
    
    _watched_directory *watched_parent = ::search(&watcher->watched_directories, &parent_path);

    if (watched_parent == nullptr)
    {
        sys_int ifd = ::inotify_add_watch(watcher->inotify_fd, parent_path.c_str(), IN_ALL_EVENTS);

        if (ifd < 0)
        {
            set_error_by_code(err, -ifd);
            fs::free(&parent_path);
            fs::free(&fcanon);
            return false;
        }

        watched_parent = ::add_element_by_key(&watcher->watched_directories, &parent_path);
        assert(watched_parent != nullptr);
        ::init(&watched_parent->watched_files);
        watched_parent->path = parent_path;
        watched_parent->only_watch_files = true;
        watched_parent->fd = ifd;
    }
    else
        fs::free(&parent_path); // already stored in watched_parent->path

    assert(watched_parent != nullptr);

    _watched_file *watched = ::search(&watched_parent->watched_files, &fcanon);

    if (watched != nullptr)
    {
        // TODO: update filter
        fs::free(&fcanon);
        return true;
    }

    sys_int ffd = ::inotify_add_watch(watcher->inotify_fd, fcanon.c_str(), IN_ALL_EVENTS);

    if (ffd < 0)
    {
        set_error_by_code(err, -ffd);
        fs::free(&fcanon);
        return false;
    }

    watched = ::add_element_by_key(&watched_parent->watched_files, &fcanon);
    assert(watched != nullptr);

    watched->path = fcanon;
    watched->fd = ffd;
    // TODO: add filter

    return true;
#endif
}

bool fs::_filesystem_watcher_unwatch_file(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err)
{
    assert(watcher != nullptr);

#if Windows
    fs::path fcanon{};
    fs::canonical_path(path, &fcanon);
    defer { fs::free(&fcanon); };

    fs::path parent_path{};
    fs::set_path(&parent_path, fs::parent_path_segment(&fcanon));
    defer { fs::free(&parent_path); };

    _watched_directory *watched_parent = ::search(&watcher->watched_directories, &parent_path);

    if (watched_parent == nullptr)
        return false;

    fs::const_fs_string fname = fs::filename(path);
    wstring wfname{};
    set_string(&wfname, fname);
    defer { ::free(&wfname); };

    _watched_file *watched = ::search(&watched_parent->watched_files, &wfname);

    if (watched == nullptr)
        return false;

    ::free(&watched->name);
    ::remove_element_by_key(&watched_parent->watched_files, &wfname);

    if (watched_parent->watched_files.size == 0)
    {
        bool ok = CloseHandle(watched_parent->handle);

        if (!ok)
        {
            set_GetLastError_error(err);
            return false;
        }

        CloseHandle(watched_parent->overlapped.hEvent);

        fs::free(&watched_parent->path);
        ::free(&watched_parent->event_buffer);
        ::remove_element_by_key(&watcher->watched_directories, &parent_path);
    }

    return true;
#elif Linux
    fs::path fcanon{};
    fs::canonical_path(path, &fcanon);
    defer { fs::free(&fcanon); };

    fs::path parent_path{};
    fs::set_path(&parent_path, fs::parent_path_segment(&fcanon));
    defer { fs::free(&parent_path); };

    _watched_directory *watched_parent = ::search(&watcher->watched_directories, &parent_path);

    if (watched_parent == nullptr)
        return false;

    _watched_file *watched = ::search(&watched_parent->watched_files, &fcanon);

    if (watched == nullptr)
        return false;

    sys_int ret = inotify_rm_watch(watcher->inotify_fd, watched->fd);

    if (ret < 0)
    {
        set_error_by_code(err, -ret);
        return false;
    }

    fs::free(&watched->path);
    remove_element_by_key(&watched_parent->watched_files, &fcanon);

    if (watched_parent->only_watch_files
     && watched_parent->watched_files.size == 0)
    {
        ret = inotify_rm_watch(watcher->inotify_fd, watched_parent->fd);

        if (ret < 0)
        {
            set_error_by_code(err, -ret);
            return false;
        }

        fs::free(&watched_parent->path);
        remove_element_by_key(&watcher->watched_directories, &parent_path);
    }

    return true;
#else
    #error "Unsupported"
#endif
}

bool fs::filesystem_watcher_unwatch_all(fs::filesystem_watcher *watcher, error *err)
{
    assert(watcher != nullptr);

#if Windows
    bool ok = true;
    bool all_ok = true;

    for_hash_table(watched_dir, &watcher->watched_directories)
    {
        for_hash_table(watched, &watched_dir->watched_files)
            ::free(&watched->name);

        ::free(&watched_dir->watched_files);

        ok = CloseHandle(watched_dir->handle);

        if (!ok)
        {
            all_ok = false;
            set_GetLastError_error(err);
        }

        CloseHandle(watched_dir->overlapped.hEvent);
        fs::free(&watched_dir->path);
        ::free(&watched_dir->event_buffer);
    }

    ::clear(&watcher->watched_directories);

    return all_ok;
#elif Linux
    sys_int ret = 0;
    bool ok = true;

    for_hash_table(watched_dir, &watcher->watched_directories)
    {
        for_hash_table(watched, &watched_dir->watched_files)
        {
            ret = inotify_rm_watch(watcher->inotify_fd, watched->fd);

            if (ret < 0)
            {
                ok = false;
                set_error_by_code(err, -ret);
            }

            fs::free(&watched->path);
        }

        ::free(&watched_dir->watched_files);

        ret = inotify_rm_watch(watcher->inotify_fd, watched_dir->fd);

        if (ret < 0)
        {
            ok = false;
            set_error_by_code(err, -ret);
        }

        fs::free(&watched_dir->path);
    }

    ::clear(&watcher->watched_directories);

    return ok;
#endif
}

bool fs::filesystem_watcher_has_events(fs::filesystem_watcher *watcher, error *err)
{
    assert(watcher != nullptr);

#if Windows
    bool ok = false;

    for_hash_table(dir, &watcher->watched_directories)
    {
        ok = WaitForSingleObject(dir->overlapped.hEvent, 0) == WAIT_OBJECT_0;

        // tprint("%\n", ok);

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

    s64 bytes = 0;
    bool ok = false;

#if Windows
    for_hash_table(dir, &watcher->watched_directories)
    {
        s32 ret = WaitForSingleObject(dir->overlapped.hEvent, 0);

        tprint("%\n", ret);

        if (ret != WAIT_OBJECT_0)
            continue;

        GetOverlappedResult(dir->handle, &dir->overlapped, (LPDWORD)&bytes, false);
        ResetEvent(&dir->overlapped.hEvent);

        auto *buffer = &dir->event_buffer;

        // we call this here before checking the previous result to
        // immediately start watching for new changes.
        ok = ReadDirectoryChangesW(dir->handle,
                                   buffer->data,
                                   (DWORD)buffer->size,
                                   false,
                                   WATCH_EVENTS,
                                   nullptr,
                                   &dir->overlapped,
                                   nullptr
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
            continue;
        }

        if (!ok)
        {
            set_GetLastError_error(err);
            break;
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
    }

    return ok;
#elif Linux
    s64 length = -1;
    s64 i = 0;

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
