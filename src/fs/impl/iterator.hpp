
// used internally, you don't need to include this to use the iterators.

#include "shl/platform.hpp"
#include "shl/scratch_buffer.hpp"
#include "shl/enum_flag.hpp"
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
// "item" may be slightly misleading, but it is the iteration object
// that is given in the for_path loop and the object the user interacts
// with. "item" is misleading because there is only one of them.
struct fs_iterator_item
{
    fs::filesystem_type type;
    fs::const_fs_string path;

#if Windows
    WIN32_FIND_DATA *find_data;
#elif Linux
    dirent64 *dirent;
#endif
};

// maybe "directory detail" would be a better name, this struct contains
// a handle to a directory for iterating, and multiple of these are used
// in a recursive iterator.
struct fs_iterator_detail
{
#if Windows
    HANDLE find_handle;
    // find_data is here because when iterating recursively we don't want to
    // forget the directory being iterated on.
    WIN32_FIND_DATA find_data;
    bool at_end;
#elif Linux
    scratch_buffer<DIRENT_STACK_BUFFER_SIZE> buffer;
    int fd;
    s64 dirent_size;
    s64 dirent_offset;
#endif
};

bool init(fs::fs_iterator_detail *detail, fs::const_fs_string pth, error *err = nullptr);
bool init(fs::fs_iterator_detail *detail, fs::const_fs_string pth, void *extra, error *err = nullptr);
bool free(fs::fs_iterator_detail *detail, error *err = nullptr);

struct fs_iterator
{
    fs::const_fs_string target_path;
    fs::path path_it; // basically target_path with each element attached to it
    fs::fs_iterator_item current_item;

    fs::fs_iterator_detail _detail;
};

bool _init(fs::fs_iterator *it, fs::const_fs_string pth, error *err);

template<typename T>
auto init(fs::fs_iterator *it, T pth, error *err = nullptr)
    -> decltype(fs::_init(it, ::to_const_string(fs::get_platform_string(pth)), err))
{
    auto pth_str = fs::get_platform_string(pth);
    auto ret = fs::_init(it, ::to_const_string(pth_str), err);

    if constexpr (needs_conversion(T))
        free(&pth_str);

    return ret;
}

bool free(fs::fs_iterator *it, error *err = nullptr);

fs::fs_iterator_item *_iterate(fs::fs_iterator *it, fs::iterate_option opt = fs::iterate_option::None, error *err = nullptr);

// recursive
struct fs_recursive_iterator_item : public fs_iterator_item
{
    s32 depth;
    bool recurse;
    bool _advance; // used internally
};

struct fs_recursive_iterator
{
    fs::const_fs_string target_path;
    fs::path path_it;
    fs::fs_recursive_iterator_item current_item;

    // one detail per recursion _depth_.
    array<fs::fs_iterator_detail> _detail_stack;
};

bool _init(fs::fs_recursive_iterator *it, fs::const_fs_string pth, fs::iterate_option opts, error *err);

template<typename T>
auto init(fs::fs_recursive_iterator *it, T pth, fs::iterate_option opts = fs::iterate_option::None, error *err = nullptr)
    -> decltype(fs::_init(it, ::to_const_string(fs::get_platform_string(pth)), opts, err))
{
    auto pth_str = fs::get_platform_string(pth);
    auto ret = fs::_init(it, ::to_const_string(pth_str), opts, err);

    if constexpr (needs_conversion(T))
        free(&pth_str);

    return ret;
}

bool free(fs::fs_recursive_iterator *it, error *err = nullptr);

fs::fs_recursive_iterator_item *_iterate(fs::fs_recursive_iterator *it, fs::iterate_option opt = fs::iterate_option::None, error *err = nullptr);
}

// macros

#include "shl/macros.hpp"

#define for_path_Func(Func, Item_Var, Pth, Opts, Err, ...)\
    if (fs::fs_iterator Item_Var##_it; true)\
    if (defer { fs::free(&Item_Var##_it); }; fs::init(&Item_Var##_it, Pth, (Err)))\
    for (fs::fs_iterator_item *Item_Var = Func(&Item_Var##_it __VA_OPT__(,) __VA_ARGS__, (Opts), (Err));\
         Item_Var != nullptr;\
         Item_Var = Func(&Item_Var##_it __VA_OPT__(,) __VA_ARGS__, (Opts), (Err)))

#define for_path_IPOE(Item_Var, Pth, Opt, Err) for_path_Func(fs::_iterate, Item_Var, Pth, Opt, Err)
#define for_path_IPO(Item_Var, Pth, Opt)       for_path_IPOE(Item_Var, Pth, Opt, nullptr)
#define for_path_IP(Item_Var, Pth)             for_path_IPO(Item_Var, Pth, fs::iterate_option::None)

#define for_path(...) GET_MACRO3(__VA_ARGS__, for_path_IPOE, for_path_IPO, for_path_IP)(__VA_ARGS__)

#define for_path_type_IPOE(Type, Item_Var, Pth, Opt, Err)\
    for_path_Func(fs::_iterate, Item_Var, Pth, ((Opt) | fs::iterate_option::QueryType), Err)\
    if (Item_Var->type == (Type))

#define for_path_type_IPO(Type, Item_Var, Pth, Opt)       for_path_type_IPOE(Type, Item_Var, Pth, Opt, nullptr)
#define for_path_type_IP(Type, Item_Var, Pth)             for_path_type_IPO(Type, Item_Var, Pth, fs::iterate_option::None)

#define for_path_type(...) GET_MACRO4(__VA_ARGS__, for_path_type_IPOE, for_path_type_IPO, for_path_type_IP)(__VA_ARGS__)
#define for_path_files(...)       for_path_type(fs::filesystem_type::File, __VA_ARGS__)
#define for_path_directories(...) for_path_type(fs::filesystem_type::Directory, __VA_ARGS__)

// recursive macros
#define for_recursive_path_Func(Func, Item_Var, Pth, Opts, Err, ...)\
    if (fs::fs_recursive_iterator Item_Var##_it; true)\
    if (defer { fs::free(&Item_Var##_it); }; fs::init(&Item_Var##_it, Pth, (Opts), (Err)))\
    for (fs::fs_recursive_iterator_item *Item_Var = Func(&Item_Var##_it __VA_OPT__(,) __VA_ARGS__, (Opts), (Err));\
         Item_Var != nullptr;\
         Item_Var = Func(&Item_Var##_it __VA_OPT__(,) __VA_ARGS__, (Opts), (Err)))

#define for_recursive_path_IPOE(Item_Var, Pth, Opt, Err) for_recursive_path_Func(fs::_iterate, Item_Var, Pth, Opt, Err)
#define for_recursive_path_IPO(Item_Var, Pth, Opt)       for_recursive_path_IPOE(Item_Var, Pth, Opt, nullptr)
#define for_recursive_path_IP(Item_Var, Pth)             for_recursive_path_IPO(Item_Var, Pth, fs::iterate_option::None)

#define for_recursive_path(...) GET_MACRO3(__VA_ARGS__, for_recursive_path_IPOE, for_recursive_path_IPO, for_recursive_path_IP)(__VA_ARGS__)

#define for_recursive_path_type_IPOE(Type, Item_Var, Pth, Opt, Err)\
    for_recursive_path_Func(fs::_iterate, Item_Var, Pth, ((Opt) | fs::iterate_option::QueryType), Err)\
    if (Item_Var->type == (Type))

#define for_recursive_path_type_IPO(Type, Item_Var, Pth, Opt)       for_recursive_path_type_IPOE(Type, Item_Var, Pth, Opt, nullptr)
#define for_recursive_path_type_IP(Type, Item_Var, Pth)             for_recursive_path_type_IPO(Type, Item_Var, Pth, fs::iterate_option::None)

#define for_recursive_path_type(...) GET_MACRO4(__VA_ARGS__, for_recursive_path_type_IPOE, for_recursive_path_type_IPO, for_recursive_path_type_IP)(__VA_ARGS__)
#define for_recursive_path_files(...)       for_recursive_path_type(fs::filesystem_type::File, __VA_ARGS__)
#define for_recursive_path_directories(...) for_recursive_path_type(fs::filesystem_type::Directory, __VA_ARGS__)

