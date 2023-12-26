
// used internally, you don't need to include this to use the iterators.

#include "shl/platform.hpp"
#include "shl/scratch_buffer.hpp"
#include "shl/defer.hpp"

#if Linux
struct dirent64
{
    u64  inode;
    s64  offset;
    u16  record_size;
    u8   type;
};
#endif

namespace fs
{
struct fs_iterator_item
{
    fs::filesystem_type type;
    fs::const_fs_string path;

#if Linux
    dirent64 *dirent;
#endif
};

struct fs_iterator
{
    fs::const_fs_string target_path;
    fs::path path_it; // basically target_path with each element attached to it

#if Linux
    scratch_buffer<DIRENT_STACK_BUFFER_SIZE> buffer;
    int fd;
    s64 dirent_size;
    s64 dirent_offset;
#endif

    fs::fs_iterator_item current_item;
};

bool _init(fs::fs_iterator *it, fs::const_fs_string pth, fs::fs_error *err);

template<typename T>
auto init(fs::fs_iterator *it, T pth, fs::fs_error *err = nullptr)
    -> decltype(fs::_init(it, ::to_const_string(fs::get_platform_string(pth)), err))
{
    auto pth_str = fs::get_platform_string(pth);
    auto ret = fs::_init(it, ::to_const_string(pth_str), err);

    if constexpr (needs_conversion(T))
        fs::free(&pth_str);

    return ret;
}

void free(fs::fs_iterator *it);

fs::fs_iterator_item *_iterate(fs::fs_iterator *it, fs::fs_error *err = nullptr);
fs::fs_iterator_item *_iterate_fullpath(fs::fs_iterator *it, fs::fs_error *err = nullptr);
fs::fs_iterator_item *_iterate_type(fs::fs_iterator *it, int type_filter, fs::fs_error *err = nullptr);
fs::fs_iterator_item *_iterate_fullpath_type(fs::fs_iterator *it, int type_filter, fs::fs_error *err = nullptr);
}

// macros

#include "shl/macros.hpp"

#define for_path_Func(Func, Item_Var, Pth, Err, ...)\
    if (fs::fs_iterator Item_Var##_it; true)\
    if (defer { fs::free(&Item_Var##_it); }; fs::init(&Item_Var##_it, Pth, (Err)))\
    for (fs::fs_iterator_item *Item_Var = Func(&Item_Var##_it __VA_OPT__(,) __VA_ARGS__, (Err));\
         Item_Var != nullptr;\
         Item_Var = Func(&Item_Var##_it __VA_OPT__(,) __VA_ARGS__, (Err)))

#define for_path_IPE(Item_Var, Pth, Err) for_path_Func(fs::_iterate, Item_Var, Pth, Err)
#define for_path_IP(Item_Var, Pth)       for_path_Func(fs::_iterate, Item_Var, Pth, nullptr)

#define for_path(...) GET_MACRO2(__VA_ARGS__, for_path_IPE, for_path_IP)(__VA_ARGS__)

#define for_fullpath_IPE(Item_Var, Pth, Err) for_path_Func(fs::_iterate_fullpath, Item_Var, Pth, Err)
#define for_fullpath_IP(Item_Var, Pth)       for_path_Func(fs::_iterate_fullpath, Item_Var, Pth, nullptr)

#define for_fullpath(...) GET_MACRO2(__VA_ARGS__, for_fullpath_IPE, for_fullpath_IP)(__VA_ARGS__)

#define for_path_type_IPE(Type, Item_Var, Pth, Err) for_path_Func(fs::_iterate_type, Item_Var, Pth, Err, (int)Type)
#define for_path_type_IP(Type, Item_Var, Pth)       for_path_Func(fs::_iterate_type, Item_Var, Pth, nullptr, (int)Type)

#define for_path_type(...) GET_MACRO3(__VA_ARGS__, for_path_type_IPE, for_path_type_IP)(__VA_ARGS__)
#define for_path_files(...)       for_path_type(fs::filesystem_type::File, __VA_ARGS__)
#define for_path_directories(...) for_path_type(fs::filesystem_type::Directory, __VA_ARGS__)

#define for_fullpath_type_IPE(Type, Item_Var, Pth, Err) for_path_Func(fs::_iterate_fullpath_type, Item_Var, Pth, Err, (int)Type)
#define for_fullpath_type_IP(Type, Item_Var, Pth)       for_path_Func(fs::_iterate_fullpath_type, Item_Var, Pth, nullptr, (int)Type)

#define for_fullpath_type(...) GET_MACRO3(__VA_ARGS__, for_fullpath_type_IPE, for_fullpath_type_IP)(__VA_ARGS__)
#define for_fullpath_files(...)       for_fullpath_type(fs::filesystem_type::File, __VA_ARGS__)
#define for_fullpath_directories(...) for_fullpath_type(fs::filesystem_type::Directory, __VA_ARGS__)
