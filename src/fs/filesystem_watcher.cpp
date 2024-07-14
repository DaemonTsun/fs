
#include "shl/print.hpp"
#include "shl/platform.hpp"
#include "shl/assert.hpp"
#include "fs/path.hpp"

#if Windows
#  include <windows.h>
#  warning "filesystem watcher currently unsupported on windows"
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
    fs::path path;
#if Linux
    io_handle fd;
#endif
};

struct _watched_directory
{
    fs::path path;
#if Linux
    io_handle fd;
#endif

    hash_table<fs::path, _watched_file> watched_files;
    bool only_watch_files;
};

#define WATCHER_BUFFER_SIZE          256
#define WATCHER_BUFFER_GROWTH_FACTOR   4
#define WATCHER_BUFFER_MAX_SIZE    65535

struct fs::filesystem_watcher
{
    watcher_callback_f callback;
    hash_table<fs::path, _watched_directory> watched_directories;
    fs::path iterator_path;
    scratch_buffer<WATCHER_BUFFER_SIZE> event_buffer;

#if Linux
    io_handle inotify_fd;
#endif
};

// linux specific watcher
#if Linux
/*
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
*/

fs::watcher_event_type mask_to_event(u32 mask)
{
    fs::watcher_event_type ret = fs::watcher_event_type::None;

    if (mask & IN_CREATE) set_flag(ret, fs::watcher_event_type::Created);
    if (mask & IN_MODIFY) set_flag(ret, fs::watcher_event_type::Modified);
    if (mask & (IN_DELETE | IN_DELETE_SELF)) set_flag(ret, fs::watcher_event_type::Removed);
    
    // not accurate, but as long as we're not emitting move events, this works
    if (mask & IN_MOVED_TO) set_flag(ret, fs::watcher_event_type::Created);
    if (mask & IN_MOVED_FROM) set_flag(ret, fs::watcher_event_type::Removed);
    
    return ret;
}

void _process_event(fs::filesystem_watcher *watcher, fs::path *it, inotify_event *event)
{
    fs::watcher_event_type type = fs::watcher_event_type::None;

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

                type = mask_to_event(event->mask);
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
            // fs::watcher_event_type type = mask_to_event(event->mask);
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
    ::init(&ret->event_buffer);
    fs::init(&ret->iterator_path);
    ret->callback = callback;

#if Linux
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
    ::free(&watcher->event_buffer);

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

bool fs::filesystem_watcher_watch_file(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err)
{
    // TODO: event filter
    assert(watcher != nullptr);

#if Linux
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

    return true;
#elif Windows
#else
    #error "Unsupported"
#endif
}

bool fs::filesystem_watcher_unwatch_file(fs::filesystem_watcher *watcher, fs::const_fs_string path, error *err)
{
    assert(watcher != nullptr);

#if Linux
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
#elif Windows
#else
    #error "Unsupported"
#endif
}

bool fs::filesystem_watcher_unwatch_all(fs::filesystem_watcher *watcher, error *err)
{
    assert(watcher != nullptr);

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
}

bool fs::filesystem_watcher_has_events(fs::filesystem_watcher *watcher, error *err)
{
    assert(watcher != nullptr);

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
}

bool fs::filesystem_watcher_process_events(fs::filesystem_watcher *watcher, error *err)
{
    assert(watcher != nullptr);

#if Linux
    auto *buffer = &watcher->event_buffer;
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

    while(i < length)
    {
        inotify_event *event = (inotify_event*)(buffer->data + i);
        _process_event(watcher, &watcher->iterator_path, event);
        i += sizeof(inotify_event) + event->name_length;
    }

    return true;
#endif
}
