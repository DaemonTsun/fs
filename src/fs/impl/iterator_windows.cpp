
#include "shl/platform.hpp"

#if Windows
#include <windows.h>

#include <assert.h>
#include <stddef.h> // offsetof

// #include "shl/print.hpp"
#define tprint(...)

#include "shl/fixed_array.hpp"
#include "shl/scratch_buffer.hpp"
#include "shl/memory.hpp"
#include "fs/path.hpp"

#define as_array_ptr(x)     (::array<fs::path_char_t>*)(x)
#define as_string_ptr(x)    (::string_base<fs::path_char_t>*)(x)

bool _get_next_item(fs::fs_iterator_detail *detail, error *err)
{
    if (!::FindNextFile(detail->find_handle, &detail->find_data))
    {
        int errcode = GetLastError();

        if (errcode != ERROR_NO_MORE_FILES)
            set_error(err, errcode, _windows_error_message(errcode));
        else
            detail->at_end = true;

        return false;
    }

    return true;
}

#define _query_item_type(Path, ItemPtr)\
    if (!fs::get_filesystem_type(Path, &((ItemPtr)->type), false, err))\
        it->current_item.type = fs::filesystem_type::Unknown;

bool fs::init(fs::fs_iterator_detail *detail, fs::const_fs_string pth, error *err)
{
    assert(detail != nullptr);

    detail->find_data = {};
    detail->find_handle = ::FindFirstFileEx(pth.c_str, 
                                            FindExInfoBasic,
                                            &detail->find_data,
                                            FindExSearchNameMatch,
                                            nullptr,
                                            0);

    if (detail->find_handle == INVALID_HANDLE_VALUE)
    {
        detail->at_end = true;
        set_GetLastError_error(err);
        return false;
    }

    detail->at_end = false;

    return true;
}

bool fs::init(fs::fs_iterator_detail *detail, fs::const_fs_string pth, [[maybe_unused]] void *extra, error *err)
{
    return fs::init(detail, pth, err);
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

    detail->at_end = true;

    return true;
}

bool fs::_init(fs::fs_iterator *it, fs::const_fs_string pth, error *err)
{
    assert(it != nullptr);

    it->target_path = pth;
    fs::init(&it->path_it);

    if (!fs::canonical_path(pth, &it->path_it, err))
        return false;
    
    // * necessary for FindFirstFile(Ex) pattern
    fs::append_path(&it->path_it, SYS_CHAR("*"));

    if (!fs::init(&it->_detail, to_const_string(&it->path_it), err))
        return false;

    it->current_item.find_data = &it->_detail.find_data;

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
    if (!_get_next_item(&it->_detail, err) && is_flag_set(opts, fs::iterate_option::StopOnError))
        return nullptr;

    // ignore . and ..
    while (!it->_detail.at_end && fs::is_dot_or_dot_dot(it->current_item.find_data->cFileName))
        if (!_get_next_item(&it->_detail, err))
            return nullptr;

    if (it->_detail.at_end)
        return nullptr;

    const sys_char *name = it->current_item.find_data->cFileName;

    it->current_item.path = ::to_const_string(name);

    // unfortunately, we need the full path to query the type of the item since
    // FindFirstFile(Ex) / FindNextFile does not provide the type of the iterated
    // item (and we can't query the type with just the name).
    if constexpr (is_flag_set(BakeOpts, fs::iterate_option::QueryType)
               || is_flag_set(BakeOpts, fs::iterate_option::Fullpaths))
    {
        fs::replace_filename(&it->path_it, ::to_const_string(name));

        if constexpr (is_flag_set(BakeOpts, fs::iterate_option::Fullpaths))
            it->current_item.path = ::to_const_string(&it->path_it);

        if constexpr (is_flag_set(BakeOpts, fs::iterate_option::QueryType))
            _query_item_type(it->path_it, &it->current_item);
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

    // see iterator_linux.cpp why 2
    ::init(&it->_detail_stack, 2);
    it->_detail_stack.size = 1;

    if (is_flag_set(opts, fs::iterate_option::Fullpaths))
    {
        if (!fs::canonical_path(pth, &it->path_it, err))
            return false;
    }
    else
        fs::set_path(&it->path_it, pth);
    
    fs::append_path(&it->path_it, SYS_CHAR("*"));

    if (!fs::init(it->_detail_stack.data, to_const_string(&it->path_it), err))
        return false;

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

#define _while_done_with_directory_go_up()\
{\
    while (stack->size > 0 && detail->at_end)\
    {\
        tprint(L"  goin up\n");\
        fs::free(detail);\
        stack->size -= 1;\
        \
        if (stack->size == 0)\
            break;\
        \
        detail_idx -= 1;\
        detail = stack->data + detail_idx;\
        it->path_it.size = fs::parent_path_segment(&it->path_it).size;\
        it->path_it.data[it->path_it.size] = SYS_CHAR('\0');\
        \
        if constexpr (is_flag_set(BakeOpts, fs::iterate_option::ChildrenFirst))\
        {\
            it->current_item.find_data = &detail->find_data;\
            it->current_item.path = ::to_const_string(&it->path_it);\
            _query_item_type(it->path_it, &it->current_item);\
            return &it->current_item;\
        }\
    }\
}

/*
        else\
        {\
            error _tmp_err{};\
            if (!_get_next_item(detail, &_tmp_err))\
            {\
                if (_tmp_err.error_code != 0 &&\
                    is_flag_set(opts, fs::iterate_option::StopOnError))\
                {\
                    if (err) *err = _tmp_err;\
                    return nullptr;\
                }\
            }\
        }
*/

template<fs::iterate_option BakeOpts>
fs::fs_recursive_iterator_item *_recursive_iterate(fs::fs_recursive_iterator *it, fs::iterate_option opts, error *err)
{
    static int DEPTH = 0;
    DEPTH++;

    if (DEPTH > 5)
        return nullptr;

    array<fs::fs_iterator_detail> *stack = &it->_detail_stack;

    // if recursion is on, add the subdirectory onto the stack
    if (it->current_item.recurse)
    {
        it->current_item.path.size += 2;
        tprint(L"  recursing into %\n", it->current_item.path);
        it->current_item.recurse = false;

        fs::fs_iterator_detail *subdir = ::add_at_end(stack);
        subdir->find_handle = INVALID_HANDLE_VALUE;

        if (!fs::init(subdir, it->current_item.path, err))
        {
            tprint(L"  recursing into % failed: %\n", it->current_item.path, err->error_code);

            if (is_flag_set(opts, fs::iterate_option::StopOnError))
                return nullptr;
            else
            {
                stack->size -= 1;

                if constexpr (is_flag_set(BakeOpts, fs::iterate_option::ChildrenFirst))
                    it->current_item._advance = true;
            }
        }
        /*
        else
            fs::append_path(&it->path_it, SYS_CHAR("*"));
        */

        tprint(L"  recursed first entry %\n", subdir->find_data.cFileName);
    }

    tprint(L"  path_it: %\n", ::to_const_string(it->path_it));

    if (stack->size == 0)
    {
        tprint(L"  empty stack\n");
        return nullptr;
    }

    // deepest subdirectory
    u64 detail_idx = stack->size - 1;
    fs::fs_iterator_detail *detail = stack->data + detail_idx;

    _while_done_with_directory_go_up();

    // we're done if stack is now empty
    if (stack->size == 0)
    {
        tprint(L"  empty stack (2)\n");
        return nullptr;
    }

    tprint(L"  settled on idx %\n", detail_idx);

    /*
    if constexpr (is_flag_set(BakeOpts, fs::iterate_option::ChildrenFirst))
    {
        if (it->current_item._advance)
        {
            it->current_item._advance = false;
            it->current_item.dirent = (dirent64*)(detail->buffer.data + detail->dirent_offset);
            detail->dirent_offset += it->current_item.dirent->record_size;
        }
    }
    */

    const sys_char *name = detail->find_data.cFileName;

    // skip to the first non-dot-or-dot-dot file
    while (fs::is_dot_or_dot_dot(name))
    {
        _while_done_with_directory_go_up();

        if (stack->size == 0)
            return nullptr;

        if (!_get_next_item(detail, err))
            continue;

        name = detail->find_data.cFileName;
        it->current_item.find_data = &detail->find_data;

        tprint(L"    detail idx %, %\n", detail_idx, name);
    }

    if (name == nullptr)
        return nullptr;

    fs::replace_filename(&it->path_it, ::to_const_string(name));
    it->current_item.path = ::to_const_string(&it->path_it);
    tprint(L"  current item path: %\n", it->current_item.path);

    it->current_item.depth = (u32)detail_idx;
    it->current_item.recurse = false;
    // it->current_item._advance = false;

    _query_item_type(it->path_it, &it->current_item);

    tprint("types: % %\n", (int)fs::filesystem_type::Directory, (int)it->current_item.type);

    if (it->current_item.type == fs::filesystem_type::Directory
     || (it->current_item.type == fs::filesystem_type::Symlink
        && is_flag_set(opts, fs::iterate_option::FollowSymlinks)
        && fs::is_directory(it->current_item.path, true)))
    {
        it->current_item.recurse = true;
        fs::append_path(&it->path_it, SYS_CHAR("*"));
    }

    if constexpr (is_flag_set(BakeOpts, fs::iterate_option::ChildrenFirst))
    {
        if (it->current_item.recurse)
            // TODO: split this function and remove recursion, this is really bad!!!
            return ::_recursive_iterate<fs::iterate_option::ChildrenFirst>(it, opts, err);
        // else
        //  it->current_item._advance = true;
    }

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
