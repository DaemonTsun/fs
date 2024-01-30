
#include "shl/platform.hpp"

#if Windows
#include <windows.h>

#include <assert.h>
#include <stddef.h> // offsetof

#include "shl/fixed_array.hpp"
#include "shl/scratch_buffer.hpp"
#include "fs/path.hpp"

#define as_array_ptr(x)     (::array<fs::path_char_t>*)(x)
#define as_string_ptr(x)    (::string_base<fs::path_char_t>*)(x)


bool fs::init(fs::fs_iterator_detail *detail, fs::const_fs_string pth, error *err)
{
    assert(detail != nullptr);

    // TODO: implement
    return true;
}

void fs::free(fs::fs_iterator_detail *detail)
{
    assert(detail != nullptr);

    // TODO: implement
}

bool fs::_init(fs::fs_iterator *it, fs::const_fs_string pth, error *err)
{
    assert(it != nullptr);

    it->target_path = pth;
    fs::init(&it->path_it, pth);

    if (!fs::init(&it->_detail, pth, err))
        return false;

    // TODO: items

    fs::path tmp = fs::canonical_path(&it->path_it);
    fs::free(&it->path_it);
    it->path_it = tmp;

    // fs::append_path(&it->path_it, ".");

    return true;
}

void fs::free(fs_iterator *it)
{
    assert(it != nullptr);

    fs::free(&it->path_it);
    fs::free(&it->_detail);
}

template<fs::iterate_option BakeOpts>
fs::fs_iterator_item *_iterate(fs::fs_iterator *it, fs::iterate_option opts, error *err)
{
    // TODO: implement
    return &it->current_item;
}

fs::fs_iterator_item *fs::_iterate(fs::fs_iterator *it, fs::iterate_option opts, error *err)
{
    if (is_flag_set(opts, fs::iterate_option::Fullpaths))
        return ::_iterate<fs::iterate_option::Fullpaths>(it, opts, err);
    else
        return ::_iterate<fs::iterate_option::None>(it, opts, err);
}

// recursive iteration
bool fs::_init(fs::fs_recursive_iterator *it, fs::const_fs_string pth, fs::iterate_option opts, error *err)
{
    assert(it != nullptr);

    it->target_path = pth;
    fs::init(&it->path_it);
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

    // TODO: get initial items

    if (is_flag_set(opts, fs::iterate_option::Fullpaths))
    {
        if (!fs::canonical_path(pth, &it->path_it, err))
            return false;
    }
    else
        fs::set_path(&it->path_it, pth);

    // fs::append_path(&it->path_it, ".");

    return true;
}

void fs::free(fs::fs_recursive_iterator *it)
{
    assert(it != nullptr);

    fs::free(&it->path_it);
    ::free<true>(&it->_detail_stack);
}

template<fs::iterate_option BakeOpts>
fs::fs_recursive_iterator_item *_recursive_iterate(fs::fs_recursive_iterator *it, fs::iterate_option opts, error *err)
{
    // TODO: implement
    return &it->current_item;
}

fs::fs_recursive_iterator_item *fs::_iterate(fs::fs_recursive_iterator *it, fs::iterate_option opts, error *err)
{
    if (is_flag_set(opts, fs::iterate_option::ChildrenFirst))
        return ::_recursive_iterate<fs::iterate_option::ChildrenFirst>(it, opts, err);
    else
        return ::_recursive_iterate<fs::iterate_option::None>(it, opts, err);
}

#endif // if Windows
