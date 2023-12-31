
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

bool _get_next_dirents(fs::fs_iterator_detail *detail, fs::fs_error *err)
{
    int errcode = 0;

    while (detail->buffer.size < DIRENT_ALLOC_MAX_SIZE)
    {
        detail->dirent_size = ::getdents64(detail->fd, detail->buffer.data, detail->buffer.size);

        if (detail->dirent_size != -1)
            break;

        errcode = errno;

        if (errcode != EINVAL)
            break;

        ::grow_by(&detail->buffer, DIRENT_ALLOC_GROWTH_FACTOR);
    }

    if (detail->dirent_size < 0)
    {
        set_fs_error(err, errcode, ::strerror(errcode));
        return false;
    }

    if (detail->buffer.size >= DIRENT_ALLOC_MAX_SIZE)
    {
        set_fs_error(err, EINVAL, "not enough memory in dirent buffer");
        return false;
    }

    detail->dirent_offset = 0;

    return true;
}

bool fs::init(fs::fs_iterator_detail *detail, fs::const_fs_string pth, fs::fs_error *err)
{
    assert(detail != nullptr);

#if Linux
    ::init(&detail->buffer);

    detail->fd = ::open(pth.c_str, O_RDONLY | O_DIRECTORY);

    if (detail->fd == -1)
    {
        set_fs_errno_error(err);
        return false;
    }

    detail->dirent_size = 0;
    detail->dirent_offset = 0;
#endif

    return true;
}

void fs::free(fs::fs_iterator_detail *detail)
{
    assert(detail != nullptr);

    ::free(&detail->buffer);

    if (detail->fd != -1)
        ::close(detail->fd);

    detail->fd = -1;
}

bool fs::_init(fs::fs_iterator *it, fs::const_fs_string pth, fs::fs_error *err)
{
    assert(it != nullptr);

    it->target_path = pth;
    fs::init(&it->path_it, pth);

    if (!fs::init(&it->_detail, pth, err))
        return false;

    if (!_get_next_dirents(&it->_detail, err))
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
    fs::free(&it->_detail);
}

template<fs::iterate_option BakeOpts>
fs::fs_iterator_item *_iterate(fs::fs_iterator *it, fs::iterate_option opts, fs::fs_error *err)
{
    if (it->_detail.dirent_size == 0)
        return nullptr;

    if (it->_detail.dirent_offset >= it->_detail.dirent_size)
    {
        if (!_get_next_dirents(&it->_detail, err))
            return nullptr;
    }

    if (it->_detail.dirent_size == 0)
        return nullptr;

    it->current_item.dirent = (dirent64*)(it->_detail.buffer.data + it->_detail.dirent_offset);
    fs::filesystem_type current_type = (fs::filesystem_type)(it->current_item.dirent->type << 12);
    const char *name = ((char*)it->current_item.dirent) + offsetof(dirent64, type) + 1;

    while (fs::is_dot_or_dot_dot(name))
    {
        it->_detail.dirent_offset += it->current_item.dirent->record_size;

        if (it->_detail.dirent_offset >= it->_detail.dirent_size)
        {
            if (!_get_next_dirents(&it->_detail, err))
                return nullptr;
        }

        if (it->_detail.dirent_size == 0)
            return nullptr;

        it->current_item.dirent = (dirent64*)(it->_detail.buffer.data + it->_detail.dirent_offset);
        current_type = (fs::filesystem_type)(it->current_item.dirent->type << 12);
        name = ((char*)it->current_item.dirent) + offsetof(dirent64, type) + 1;
    }

    it->current_item.type = current_type;

    // In memory, the name of the entry is directly after the type of a dirent64 struct.
    // The name is null terminated.

    if constexpr (is_flag_set(BakeOpts, fs::iterate_option::Fullpaths))
    {
        fs::replace_filename(&it->path_it, to_const_string(name));

        it->current_item.path = ::to_const_string(&it->path_it);
    }
    else
    {
        it->current_item.path = ::to_const_string(name);
    }

    it->_detail.dirent_offset += it->current_item.dirent->record_size;

    return &it->current_item;
}

fs::fs_iterator_item *fs::_iterate(fs::fs_iterator *it, fs::iterate_option opts, fs::fs_error *err)
{
    if (is_flag_set(opts, fs::iterate_option::Fullpaths))
        return ::_iterate<fs::iterate_option::Fullpaths>(it, opts, err);
    else
        return ::_iterate<fs::iterate_option::None>(it, opts, err);
}

// recursive iteration
bool fs::_init(fs::fs_recursive_iterator *it, fs::const_fs_string pth, fs::iterate_option opts, fs::fs_error *err)
{
    assert(it != nullptr);

    it->target_path = pth;
    fs::init(&it->path_it);
    it->current_item.dirent = nullptr;
    it->current_item.type = fs::filesystem_type::Unknown;
    it->current_item.recurse = false;
    it->current_item._advance = false;

    // we allocate 2, we could allocate more but for shallow directory structures
    // this would be wasteful (an iterator_item is somewhat big), but only
    // allocating 1 would mean if any directory iterated on has any subfolders
    // (which is the whole point of iterating recursively...), we would have to move
    // the memory of the first iterator_item upon growing, so 2 it is.
    // ideally we would know the deepest depth of subdirectories and submit it via
    // parameter or something, but this will do for now.
    ::init(&it->_detail_stack, 2);
    it->_detail_stack.size = 1;

    if (!fs::init(it->_detail_stack.data, pth, err))
        return false;

    if (!_get_next_dirents(it->_detail_stack.data, err))
        return false;

    if (is_flag_set(opts, fs::iterate_option::Fullpaths))
    {
        it->path_it = fs::canonical_path(pth);
    }
    else
        fs::set_path(&it->path_it, pth);

    fs::append_path(&it->path_it, ".");

    return true;
}

void fs::free(fs::fs_recursive_iterator *it)
{
    assert(it != nullptr);

    fs::free(&it->path_it);
    ::free<true>(&it->_detail_stack);
}

// #include "shl/print.hpp"
#define tprint(...)

#define _while_done_with_directory_go_up()\
{\
    while (stack->size > 0 && detail->dirent_size == 0)\
    {\
        fs::free(detail);\
        stack->size -= 1;\
        \
        if (stack->size == 0)\
            break;\
        \
        detail_idx -= 1;\
        detail = stack->data + detail_idx;\
        it->path_it.size = fs::parent_path_segment(&it->path_it).size;\
        it->path_it.data[it->path_it.size] = '\0';\
        \
        if constexpr (is_flag_set(BakeOpts, fs::iterate_option::ChildrenFirst))\
        {\
            assert(detail->dirent_offset < detail->dirent_size);\
            dirent64 *dirent = (dirent64*)(detail->buffer.data + detail->dirent_offset);\
            it->current_item.dirent = dirent;\
            it->current_item.type = (fs::filesystem_type)(dirent->type << 12);\
            it->current_item.path = ::to_const_string(&it->path_it);\
            it->current_item.depth = detail_idx;\
            detail->dirent_offset += it->current_item.dirent->record_size;\
            return &it->current_item;\
        }\
        else\
        {\
            if (detail->dirent_offset >= detail->dirent_size\
             && !_get_next_dirents(detail, err))\
            {\
                if (is_flag_set(opts, fs::iterate_option::StopOnError))\
                    return nullptr;\
                else\
                {\
                    detail->dirent_size = 0;\
                    continue;\
                }\
            }\
        }\
    }\
}

#define _regenerate_scratch_buffers(Stack)\
{\
    for_array(_det, Stack)\
    if (_det->buffer.size <= DIRENT_STACK_BUFFER_SIZE)\
        _det->buffer.data = _det->buffer.stack_buffer;\
}

template<fs::iterate_option BakeOpts>
fs::fs_recursive_iterator_item *_recursive_iterate(fs::fs_recursive_iterator *it, fs::iterate_option opts, fs::fs_error *err)
{
    tprint("iterate, stack size: %\n", it->_detail_stack.size);
    array<fs::fs_iterator_detail> *stack = &it->_detail_stack;

    // if recursion is on add the subdirectory onto the stack
    if (it->current_item.recurse)
    {
        tprint("  recursing into %\n", it->current_item.path);
        it->current_item.recurse = false;

        fs::fs_iterator_detail *subdir = ::add_at_end(stack);

        if (!fs::init(subdir, it->current_item.path, err)
         || !_get_next_dirents(subdir, err))
        {
            tprint("  recursing into % failed\n", it->current_item.path);
            if (is_flag_set(opts, fs::iterate_option::StopOnError))
                return nullptr;
            else
            {
                stack->size -= 1;

                if constexpr (is_flag_set(BakeOpts, fs::iterate_option::ChildrenFirst))
                    it->current_item._advance = true;
            }
        }
        else
            fs::append_path(&it->path_it, ".");

        // I suppose this is a design flaw, but that's what you
        // get when you mess with hacky things like scratch buffers.
        _regenerate_scratch_buffers(stack);
    }

    tprint("  path_it: %\n", ::to_const_string(it->path_it));

    if (stack->size == 0)
    {
        tprint("  empty stack\n");
        return nullptr;
    }

    // deepest subdirectory
    u64 detail_idx = stack->size - 1;
    fs::fs_iterator_detail *detail = stack->data + detail_idx;

    tprint("  dirent size: %, dirent offset %\n", detail->dirent_size, detail->dirent_offset);

    _while_done_with_directory_go_up();

    // we're done if stack is empty
    if (stack->size == 0)
    {
        tprint("  empty stack (2)\n");
        return nullptr;
    }

    tprint("  settled on idx %\n", detail_idx);

    if constexpr (is_flag_set(BakeOpts, fs::iterate_option::ChildrenFirst))
    {
        if (it->current_item._advance)
        {
            it->current_item._advance = false;
            it->current_item.dirent = (dirent64*)(detail->buffer.data + detail->dirent_offset);
            detail->dirent_offset += it->current_item.dirent->record_size;
        }
    }

    dirent64 *dirent = nullptr;
    fs::filesystem_type current_type;
    // just the name of the entry within a subdirectory, not full path
    const char *name = nullptr;

    while (true)
    {
        if (detail->dirent_offset >= detail->dirent_size
         && !_get_next_dirents(detail, err))
        {
            if (is_flag_set(opts, fs::iterate_option::StopOnError))
                return nullptr;
            else
                detail->dirent_size = 0;
        }

        _while_done_with_directory_go_up();

        if (stack->size == 0)
            return nullptr;

        if (detail->dirent_offset >= detail->dirent_size)
            continue;

        dirent = (dirent64*)(detail->buffer.data + detail->dirent_offset);
        it->current_item.dirent = dirent;
        name = ((char*)dirent) + offsetof(dirent64, type) + 1;
        tprint("    detail idx %, size %, offset % - %\n", detail_idx, detail->dirent_size, detail->dirent_offset, name);
        current_type = (fs::filesystem_type)(dirent->type << 12);

        if (fs::is_dot_or_dot_dot(name))
        {
            detail->dirent_offset += dirent->record_size;
            continue;
        }

        break;
    }

    it->current_item.type = current_type;

    fs::replace_filename(&it->path_it, ::to_const_string(name));
    it->current_item.path = ::to_const_string(&it->path_it);
    tprint("  current item path: %\n", it->current_item.path);

    it->current_item.depth = detail_idx;
    it->current_item.recurse = false;
    it->current_item._advance = false;

    if (it->current_item.type == fs::filesystem_type::Directory
     || (it->current_item.type == fs::filesystem_type::Symlink
        && is_flag_set(opts, fs::iterate_option::FollowSymlinks)
        && fs::is_directory(it->current_item.path, true)))
            it->current_item.recurse = true;

    if constexpr (is_flag_set(BakeOpts, fs::iterate_option::ChildrenFirst))
    {
        if (it->current_item.recurse)
            return ::_recursive_iterate<fs::iterate_option::ChildrenFirst>(it, opts, err);
        else
            it->current_item._advance = true;
    }
    else
    {
        detail->dirent_offset += it->current_item.dirent->record_size;
    }

    return &it->current_item;
}

fs::fs_recursive_iterator_item *fs::_iterate(fs::fs_recursive_iterator *it, fs::iterate_option opts, fs::fs_error *err)
{
    if (is_flag_set(opts, fs::iterate_option::ChildrenFirst))
        return ::_recursive_iterate<fs::iterate_option::ChildrenFirst>(it, opts, err);
    else
        return ::_recursive_iterate<fs::iterate_option::None>(it, opts, err);
}
