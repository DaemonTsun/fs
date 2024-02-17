
#include "shl/platform.hpp"

#if Windows
#include <windows.h>

#include <assert.h>
#include <stddef.h> // offsetof
#include <stdio.h> // TODO: remove

#include "shl/fixed_array.hpp"
#include "shl/scratch_buffer.hpp"
#include "shl/memory.hpp"
#include "fs/path.hpp"

#define as_array_ptr(x)     (::array<fs::path_char_t>*)(x)
#define as_string_ptr(x)    (::string_base<fs::path_char_t>*)(x)


bool fs::init(fs::fs_iterator_detail *detail, fs::const_fs_string pth, error *err)
{
    // should always trigger an assert
    return fs::init(detail, pth, nullptr, err);
}

bool fs::init(fs::fs_iterator_detail *detail, fs::const_fs_string pth, void *extra, error *err)
{
    assert(detail != nullptr);
    assert(extra != nullptr);

    detail->find_handle = ::FindFirstFileEx(pth.c_str, 
                                            FindExInfoBasic,
                                            (LPWIN32_FIND_DATA)extra,
                                            FindExSearchNameMatch,
                                            nullptr,
                                            0);

    if (detail->find_handle == INVALID_HANDLE_VALUE)
    {
        set_GetLastError_error(err);
        return false;
    }

    return true;
}

bool fs::free(fs::fs_iterator_detail *detail, error *err)
{
    assert(detail != nullptr);

    if (detail->find_handle != INVALID_HANDLE_VALUE)
    {
        if (!::FindClose(detail->find_handle))
        {
            set_GetLastError_error(err);
            return false;
        }

        detail->find_handle = INVALID_HANDLE_VALUE;
    }

    return true;
}

bool fs::_init(fs::fs_iterator *it, fs::const_fs_string pth, error *err)
{
    assert(it != nullptr);

    it->target_path = pth;
    it->path_it = fs::canonical_path(pth);
    
    fs::append_path(&it->path_it, SYS_CHAR("*"));

    if (!fs::init(&it->_detail, to_const_string(&it->path_it), (void*)(&it->current_item.find_data), err))
        return false;

    fs::replace_filename(&it->path_it, ".");

    return true;
}

bool fs::free(fs_iterator *it, error *err)
{
    assert(it != nullptr);

    fs::free(&it->path_it);
    return fs::free(&it->_detail, err);
}

template<fs::iterate_option BakeOpts>
fs::fs_iterator_item *_iterate(fs::fs_iterator *it, fs::iterate_option opts, error *err)
{
    if (!::FindNextFile(it->_detail.find_handle, &it->current_item.find_data))
        return nullptr;

    // ignore . and ..
    while (fs::is_dot_or_dot_dot(it->current_item.find_data.cFileName))
        if (!::FindNextFile(it->_detail.find_handle, &it->current_item.find_data))
            return nullptr;

    const sys_char *name = it->current_item.find_data.cFileName;

    it->current_item.path = ::to_const_string(name);

    // unfortunately, we need the full path to query the type of the item since
    // FindFirstFile(Ex) / FindNextFile does not provide the type of the iterated
    // item.
    if constexpr (is_flag_set(BakeOpts, fs::iterate_option::QueryType)
               || is_flag_set(BakeOpts, fs::iterate_option::Fullpaths))
    {
        fs::replace_filename(&it->path_it, ::to_const_string(name));

        if constexpr (is_flag_set(BakeOpts, fs::iterate_option::Fullpaths))
            it->current_item.path = ::to_const_string(&it->path_it);

        if constexpr (is_flag_set(BakeOpts, fs::iterate_option::QueryType))
        {
            if (!fs::get_filesystem_type(it->path_it, &it->current_item.type, false, err))
                it->current_item.type = fs::filesystem_type::Unknown;
        }
    }

    return &it->current_item;
}

fs::fs_iterator_item *fs::_iterate(fs::fs_iterator *it, fs::iterate_option opts, error *err)
{
    if (is_flag_set(opts, fs::iterate_option::Fullpaths))
    {
        if (is_flag_set(opts, fs::iterate_option::QueryType))
            return ::_iterate<fs::iterate_option::Fullpaths | fs::iterate_option::QueryType>(it, opts, err);
        else
            return ::_iterate<fs::iterate_option::Fullpaths>(it, opts, err);
    }
    else
    {
        if (is_flag_set(opts, fs::iterate_option::QueryType))
            return ::_iterate<fs::iterate_option::QueryType>(it, opts, err);
        else
            return ::_iterate<fs::iterate_option::None>(it, opts, err);
    }
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

bool fs::free(fs::fs_recursive_iterator *it, error *err)
{
    assert(it != nullptr);

    fs::free(&it->path_it);

    bool all_ok = true;

    for_array(stck, &it->_detail_stack)
    {
        if (!fs::free(stck, err))
            all_ok = false;
    }

    return all_ok;
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
