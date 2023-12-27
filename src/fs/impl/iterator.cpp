
#include <assert.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stddef.h> // offsetof

#include "shl/fixed_array.hpp"
#include "shl/scratch_buffer.hpp"
#include "fs/path.hpp"

#define as_array_ptr(x)     (::array<fs::path_char_t>*)(x)
#define as_string_ptr(x)    (::string_base<fs::path_char_t>*)(x)

ssize_t getdents64(int fd, void *buf, size_t buf_size)
{
    return syscall(SYS_getdents64, fd, buf, buf_size);
}

bool _get_next_dirents(fs::fs_iterator *it, fs::fs_error *err)
{
    int errcode = 0;

    while (it->buffer.size < DIRENT_ALLOC_MAX_SIZE)
    {
        it->dirent_size = ::getdents64(it->fd, it->buffer.data, it->buffer.size);

        if (it->dirent_size != -1)
            break;

        errcode = errno;

        if (errcode != EINVAL)
            break;

        ::grow_by(&it->buffer, DIRENT_ALLOC_GROWTH_FACTOR);
    }

    if (it->dirent_size < 0)
    {
        set_fs_error(err, errcode, ::strerror(errcode));
        return false;
    }

    if (it->buffer.size >= DIRENT_ALLOC_MAX_SIZE)
    {
        set_fs_error(err, EINVAL, "not enough memory in dirent buffer");
        return false;
    }

    it->dirent_offset = 0;

    return true;
}

bool fs::_init(fs::fs_iterator *it, fs::const_fs_string pth, fs::fs_error *err)
{
    assert(it != nullptr);

    it->target_path = pth;
    fs::init(&it->path_it, pth);

#if Linux
    ::init(&it->buffer);
    it->fd = -1;

    it->fd = ::open(pth.c_str, O_RDONLY | O_DIRECTORY);

    if (it->fd == -1)
    {
        set_fs_errno_error(err);
        return false;
    }
#endif

    if (!_get_next_dirents(it, err))
        return false;

    fs::path tmp = fs::canonical_path(&it->path_it);
    fs::free(&it->path_it);
    it->path_it = tmp;

    fs::append_path(&it->path_it, ".");

    return true;
}

void fs::free(fs_iterator *it)
{
    assert(it != nullptr);

    fs::free(&it->path_it);
    ::free(&it->buffer);

    if (it->fd != -1)
        ::close(it->fd);

    it->fd = -1;
}

template<fs::iterate_option Opts, bool UseTypeFilter>
fs::fs_iterator_item *_iterate(fs::fs_iterator *it, int type_filter, fs::fs_error *err)
{
    if (it->dirent_size == 0)
        return nullptr;

    if (it->dirent_offset >= it->dirent_size)
    {
        if (!_get_next_dirents(it, err))
            return nullptr;
    }

    if (it->dirent_size == 0)
        return nullptr;

    it->current_item.dirent = (dirent64*)(it->buffer.data + it->dirent_offset);
    fs::filesystem_type current_type = (fs::filesystem_type)(it->current_item.dirent->type << 12);

    if constexpr (UseTypeFilter)
    {
        fs::filesystem_type target_type = (fs::filesystem_type)type_filter;

        while (current_type != target_type)
        {
            it->dirent_offset += it->current_item.dirent->record_size;

            if (it->dirent_offset >= it->dirent_size)
            {
                if (!_get_next_dirents(it, err))
                    return nullptr;
            }

            if (it->dirent_size == 0)
                return nullptr;

            it->current_item.dirent = (dirent64*)(it->buffer.data + it->dirent_offset);
            current_type = (fs::filesystem_type)(it->current_item.dirent->type << 12);
        }
    }

    it->current_item.type = current_type;

    // in memory, the name of the entry is directly after the type of a dirent64 struct.
    // the name is null terminated.
    const char *name = ((char*)it->current_item.dirent) + offsetof(dirent64, type) + 1;

    if constexpr (is_flag_set(Opts, fs::iterate_option::Fullpaths))
    {
        auto parent_seg = fs::parent_path_segment(&it->path_it);
        auto name_s = ::to_const_string(name);

        u64 cutoff = parent_seg.size + 1 + name_s.size;
        ::insert_range(as_array_ptr(&it->path_it), parent_seg.size + 1, name_s.c_str, name_s.size);
        ::reserve(as_array_ptr(&it->path_it), cutoff + 1);
        it->path_it.data[cutoff] = '\0';

        it->current_item.path = ::to_const_string(&it->path_it);
    }
    else
    {
        it->current_item.path = ::to_const_string(name);
    }

    it->dirent_offset += it->current_item.dirent->record_size;

    return &it->current_item;
}

constexpr inline fs::iterate_option to_iterate_opt(int x)
{
    return static_cast<fs::iterate_option>(x);
}

typedef fs::fs_iterator_item *(*_iterate_func)(fs::fs_iterator *, int, fs::fs_error *);
// we do this to bake all the options at compile time. C++ is a beautiful thing.
constexpr fixed_array _iterate_functions = {
    ::_iterate<to_iterate_opt(0b000), false>,
    ::_iterate<to_iterate_opt(0b001), false>,
    ::_iterate<to_iterate_opt(0b010), false>,
    ::_iterate<to_iterate_opt(0b011), false>,
    ::_iterate<to_iterate_opt(0b100), false>,
    ::_iterate<to_iterate_opt(0b101), false>,
    ::_iterate<to_iterate_opt(0b110), false>,
    ::_iterate<to_iterate_opt(0b111), false>,
    ::_iterate<to_iterate_opt(0b000), true>,
    ::_iterate<to_iterate_opt(0b001), true>,
    ::_iterate<to_iterate_opt(0b010), true>,
    ::_iterate<to_iterate_opt(0b011), true>,
    ::_iterate<to_iterate_opt(0b100), true>,
    ::_iterate<to_iterate_opt(0b101), true>,
    ::_iterate<to_iterate_opt(0b110), true>,
    ::_iterate<to_iterate_opt(0b111), true>,
};

fs::fs_iterator_item *fs::_iterate(fs::fs_iterator *it, fs::iterate_option opt, fs::fs_error *err)
{
    return ::_iterate_functions[(int)opt](it, -1, err);
}

fs::fs_iterator_item *fs::_iterate_type(fs::fs_iterator *it, int type_filter, fs::iterate_option opt, fs::fs_error *err)
{
    return ::_iterate_functions[(int)opt + ::_iterate_functions.size / 2](it, -1, err);
}

