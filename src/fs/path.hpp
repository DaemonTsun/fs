
/* path.hpp
 TODO: write new docs

Note: fs::path has a size member, but because fs uses a lot of
standard library functions / syscalls (e.g. stat), which do not
check for size, a null character is required to terminate a
fs::path.

 */

#pragma once

#include "shl/io.hpp"
#include "shl/hash.hpp"
#include "shl/platform.hpp"
#include "shl/error.hpp"
#include "shl/type_functions.hpp"

#include "fs/common.hpp"
#include "fs/convert.hpp"

namespace fs
{
struct path
{
    typedef fs::path_char_t value_type;

    value_type *data;
    u64 size;
    u64 reserved_size;

    operator const value_type*() const;

    const value_type *c_str() const;
};
}

fs::const_fs_string to_const_string(const fs::path_char_t *path);
fs::const_fs_string to_const_string(const fs::path_char_t *path, u64 size);
template<u64 N> fs::const_fs_string to_const_string(const fs::path_char_t path[N]) { return fs::const_fs_string{path, N}; }
fs::const_fs_string to_const_string(const fs::path *path);
fs::const_fs_string to_const_string(const fs::path &path);
fs::const_fs_string to_const_string(fs::const_fs_string path);

namespace fs
{
// this function returns a string that's always using the character
// type for paths on the current system.
// This will allocate memory if needs_conversion(C) is true, in which case
// it needs to be freed by calling fs::free(&return value of this function).
template<typename C>
fs::platform_converted_string _get_platform_string(::const_string_base<C> str)
{
    if constexpr (needs_conversion(C))
        return fs::convert_string(str);
    else
        return fs::converted_string<C>{const_cast<C*>(str.c_str), str.size};
}

template<typename T>
auto get_platform_string(T str)
    -> decltype(fs::_get_platform_string(::to_const_string(str)))
{
    return fs::_get_platform_string(::to_const_string(str));
}

#define define_fs_conversion_body(Func, Pth, ...)                                                       \
-> decltype(Func(::to_const_string(fs::get_platform_string(Pth)) __VA_OPT__(,) __VA_ARGS__))            \
{                                                                                                       \
    auto pth_str = fs::get_platform_string(Pth);                                                        \
                                                                                                        \
    if constexpr (is_same(void, decltype(Func(::to_const_string(pth_str) __VA_OPT__(,) __VA_ARGS__))))  \
    {                                                                                                   \
        Func(::to_const_string(pth_str) __VA_OPT__(,) __VA_ARGS__);                                     \
                                                                                                        \
        if constexpr (needs_conversion(T))                                                              \
            fs::free(&pth_str);                                                                         \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
        auto ret = Func(::to_const_string(pth_str) __VA_OPT__(,) __VA_ARGS__);                          \
                                                                                                        \
        if constexpr (needs_conversion(T))                                                              \
            fs::free(&pth_str);                                                                         \
                                                                                                        \
        return ret;                                                                                     \
    }                                                                                                   \
}

#define define_fs_conversion_body2(Func, Pth1, Pth2, ...)                                                                                       \
-> decltype(Func(::to_const_string(fs::get_platform_string(Pth1)), ::to_const_string(fs::get_platform_string(Pth2)) __VA_OPT__(,) __VA_ARGS__)) \
{                                                                                                                                               \
    auto pth_str1 = fs::get_platform_string(Pth1);                                                                                              \
    auto pth_str2 = fs::get_platform_string(Pth2);                                                                                              \
                                                                                                                                                \
    if constexpr (is_same(void, decltype(Func(::to_const_string(pth_str1), ::to_const_string(pth_str2) __VA_OPT__(,) __VA_ARGS__))))            \
    {                                                                                                                                           \
        Func(::to_const_string(pth_str1), ::to_const_string(pth_str2) __VA_OPT__(,) __VA_ARGS__);                                               \
                                                                                                                                                \
        if constexpr (needs_conversion(T1))                                                                                                     \
            fs::free(&pth_str1);                                                                                                                \
                                                                                                                                                \
        if constexpr (needs_conversion(T2))                                                                                                     \
            fs::free(&pth_str2);                                                                                                                \
    }                                                                                                                                           \
    else                                                                                                                                        \
    {                                                                                                                                           \
        auto ret = Func(::to_const_string(pth_str1), ::to_const_string(pth_str2) __VA_OPT__(,) __VA_ARGS__);                                    \
                                                                                                                                                \
        if constexpr (needs_conversion(T1))                                                                                                     \
            fs::free(&pth_str1);                                                                                                                \
                                                                                                                                                \
        if constexpr (needs_conversion(T2))                                                                                                     \
            fs::free(&pth_str2);                                                                                                                \
                                                                                                                                                \
        return ret;                                                                                                                             \
    }                                                                                                                                           \
}

void init(fs::path *path);
void init(fs::path *path, const char    *str);
void init(fs::path *path, const char    *str, u64 size);
void init(fs::path *path, const wchar_t *str);
void init(fs::path *path, const wchar_t *str, u64 size);
void init(fs::path *path, const_string   str);
void init(fs::path *path, const_wstring  str);
void init(fs::path *path, const fs::path *other);

void free(fs::path *path);

void set_path(fs::path *pth, const char    *new_path);
void set_path(fs::path *pth, const char    *new_path, u64 size);
void set_path(fs::path *pth, const wchar_t *new_path);
void set_path(fs::path *pth, const wchar_t *new_path, u64 size);
void set_path(fs::path *pth, const_string   new_path);
void set_path(fs::path *pth, const_wstring  new_path);
void set_path(fs::path *pth, const fs::path *new_path);

bool operator==(const fs::path &lhs, const fs::path &rhs);

hash_t hash(const fs::path *pth);

bool get_filesystem_info(io_handle h, fs::filesystem_info *out, int flags, error *err = nullptr);
bool _get_filesystem_info(fs::const_fs_string pth, fs::filesystem_info *out, bool follow_symlinks, int flags, error *err);

// type T is anything that can be converted to fs::const_fs_string
template<typename T>
auto get_filesystem_info(T pth, fs::filesystem_info *out, bool follow_symlinks = true, int flags = FS_QUERY_DEFAULT_FLAGS, error *err = nullptr)
    define_fs_conversion_body(fs::_get_filesystem_info, pth, out, follow_symlinks, flags, err)

fs::filesystem_type get_filesystem_type(const fs::filesystem_info *info);

bool get_filesystem_type(io_handle h, fs::filesystem_type *out, error *err = nullptr);
bool _get_filesystem_type(fs::const_fs_string pth, fs::filesystem_type *out, bool follow_symlinks, error *err);
template<typename T>
auto get_filesystem_type(T pth, fs::filesystem_type *out, bool follow_symlinks = true, error *err = nullptr)
    define_fs_conversion_body(fs::_get_filesystem_type, pth, out, follow_symlinks, err);

fs::permission get_permissions(const fs::filesystem_info *info);

bool get_permissions(io_handle h, fs::permission *out, error *err = nullptr);
bool _get_permissions(fs::const_fs_string pth, fs::permission *out, bool follow_symlinks, error *err);
template<typename T>
auto get_permissions(T pth, fs::permission *out, bool follow_symlinks = true, error *err = nullptr)
    define_fs_conversion_body(fs::_get_permissions, pth, out, follow_symlinks, err);

bool set_permissions(io_handle h, fs::permission perms, error *err = nullptr);
bool _set_permissions(fs::const_fs_string pth, fs::permission perms, bool follow_symlinks, error *err);
template<typename T>
auto set_permissions(T pth, fs::permission perms, bool follow_symlinks = true, error *err = nullptr)
    define_fs_conversion_body(fs::_set_permissions, pth, perms, follow_symlinks, err);

// 0 = doesn't exist, 1 = exists, -1 = error
int _exists(fs::const_fs_string pth, bool follow_symlinks, error *err);

// type T is anything that can be converted to fs::const_fs_string
template<typename T>
auto exists(T pth, bool follow_symlinks = true, error *err = nullptr)
    define_fs_conversion_body(fs::_exists, pth, follow_symlinks, err)

bool is_file_info(const fs::filesystem_info *info);
bool is_pipe_info(const fs::filesystem_info *info);
bool is_block_device_info(const fs::filesystem_info *info);
bool is_special_character_file_info(const fs::filesystem_info *info);
bool is_socket_info(const fs::filesystem_info *info);
bool is_symlink_info(const fs::filesystem_info *info);
bool is_directory_info(const fs::filesystem_info *info);
bool is_other_info(const fs::filesystem_info *info);

#define define_path_is_type_body(InfoFunc, Pth)\
    -> decltype(fs::get_filesystem_info(Pth, (fs::filesystem_info*)nullptr, follow_symlinks, FS_QUERY_TYPE, err))\
{\
    fs::filesystem_info info{};\
\
    if (!fs::get_filesystem_info(Pth, &info, follow_symlinks, FS_QUERY_TYPE, err))\
        return false;\
\
    return InfoFunc(&info);\
}

// T is anything that can be passed to get_filesystem_info, i.e. strings and paths
template<typename T> auto is_file(T pth, bool follow_symlinks = true, error *err = nullptr) define_path_is_type_body(fs::is_file_info, pth)
template<typename T> auto is_pipe(T pth, bool follow_symlinks = true, error *err = nullptr) define_path_is_type_body(fs::is_pipe_info, pth)
template<typename T> auto is_block_device(T pth, bool follow_symlinks = true, error *err = nullptr) define_path_is_type_body(fs::is_block_device_info, pth)
template<typename T> auto is_special_character_file(T pth, bool follow_symlinks = true, error *err = nullptr) define_path_is_type_body(fs::is_special_character_file_info, pth)
template<typename T> auto is_socket(T pth, bool follow_symlinks = true, error *err = nullptr) define_path_is_type_body(fs::is_socket_info, pth)
// is_symlink has follow_symlinks = false for obvious reasons
template<typename T> auto is_symlink(T pth, bool follow_symlinks = false, error *err = nullptr) define_path_is_type_body(fs::is_symlink_info, pth)
template<typename T> auto is_directory(T pth, bool follow_symlinks = true, error *err = nullptr) define_path_is_type_body(fs::is_directory_info, pth)
template<typename T> auto is_other(T pth, bool follow_symlinks = true, error *err = nullptr) define_path_is_type_body(fs::is_other_info, pth)

bool is_file(io_handle h, error *err = nullptr);
bool is_pipe(io_handle h, error *err = nullptr);
bool is_block_device(io_handle h, error *err = nullptr);
bool is_special_character_file(io_handle h, error *err = nullptr);
bool is_socket(io_handle h, error *err = nullptr);
bool is_symlink(io_handle h, error *err = nullptr);
bool is_directory(io_handle h, error *err = nullptr);
bool is_other(io_handle h, error *err = nullptr);

bool _is_absolute(fs::const_fs_string pth, error *err);
bool _is_relative(fs::const_fs_string pth, error *err);

template<typename T> auto is_absolute(T pth, error *err = nullptr) define_fs_conversion_body(fs::_is_absolute, pth, err)
template<typename T> auto is_relative(T pth, error *err = nullptr) define_fs_conversion_body(fs::_is_relative, pth, err)

bool are_equivalent_infos(const fs::filesystem_info *info1, const fs::filesystem_info *info2);

bool _are_equivalent(fs::const_fs_string pth1, fs::const_fs_string pth2, bool follow_symlinks = true, error *err = nullptr);

// type T1 and T2 are anything that can be converted to fs::const_fs_string,
// but do not have to be the same type.
template<typename T1, typename T2>
auto are_equivalent(T1 pth1, T2 pth2, bool follow_symlinks = true, error *err = nullptr)
    define_fs_conversion_body2(fs::_are_equivalent, pth1, pth2, follow_symlinks, err)

fs::const_fs_string filename(fs::const_fs_string pth);
fs::const_fs_string filename(const fs::path *pth);

// .
bool _is_dot(fs::const_fs_string pth);
// ..
bool _is_dot_dot(fs::const_fs_string pth);
// . or ..
bool _is_dot_or_dot_dot(fs::const_fs_string pth);

template<typename T> auto is_dot(T pth) define_fs_conversion_body(fs::_is_dot, pth)
template<typename T> auto is_dot_dot(T pth) define_fs_conversion_body(fs::_is_dot_dot, pth)
template<typename T> auto is_dot_or_dot_dot(T pth) define_fs_conversion_body(fs::_is_dot_or_dot_dot, pth)

// this differs from std::filesystem::path::extension:
// filenames that have no stem (only an extension) are treated exactly like that:
// no filename, only extension, and as such file_extension returns the full filename
// for filenames with no stem.
//
// . and .. are treated the same as std::filesystem::path::extension:
// file_extension will return empty strings (not nullptr) in these cases.
fs::const_fs_string file_extension(fs::const_fs_string pth);
fs::const_fs_string file_extension(const fs::path *pth);
fs::const_fs_string parent_path_segment(fs::const_fs_string pth);
fs::const_fs_string parent_path_segment(const fs::path *pth);
fs::const_fs_string root(fs::const_fs_string pth);
fs::const_fs_string root(const fs::path *pth);

void _replace_filename(fs::path *out, fs::const_fs_string newname);

template<typename T>
void replace_filename(fs::path *out, T newname)
{
    auto pth_str = fs::get_platform_string(newname);

    _replace_filename(out, ::to_const_string(pth_str));

    if constexpr (needs_conversion(T))
        fs::free(&pth_str);
}

void path_segments(fs::const_fs_string pth, array<fs::const_fs_string> *out);
void path_segments(const fs::path *pth, array<fs::const_fs_string> *out);

fs::path _parent_path(fs::const_fs_string pth);
void     _parent_path(fs::const_fs_string pth, fs::path *out);
template<typename T> auto parent_path(T pth) define_fs_conversion_body(fs::_parent_path, pth)
template<typename T> auto parent_path(T pth, fs::path *out) define_fs_conversion_body(fs::_parent_path, pth, out)

fs::path _longest_existing_path(fs::const_fs_string pth);
void     _longest_existing_path(fs::const_fs_string pth, fs::path *out);
template<typename T> auto longest_existing_path(T pth) define_fs_conversion_body(fs::_longest_existing_path, pth)
template<typename T> auto longest_existing_path(T pth, fs::path *out) define_fs_conversion_body(fs::_longest_existing_path, pth, out)

void normalize(fs::path *pth);

fs::path _absolute_path(fs::const_fs_string pth, error *err);
bool     _absolute_path(fs::const_fs_string pth, fs::path *out, error *err);
template<typename T> auto absolute_path(T pth, error *err = nullptr) define_fs_conversion_body(fs::_absolute_path, pth, err);
template<typename T> auto absolute_path(T pth, fs::path *out, error *err = nullptr) define_fs_conversion_body(fs::_absolute_path, pth, out, err);

fs::path _canonical_path(fs::const_fs_string pth, error *err);
bool     _canonical_path(fs::const_fs_string pth, fs::path *out, error *err);
template<typename T> auto canonical_path(T pth, error *err = nullptr) define_fs_conversion_body(fs::_canonical_path, pth, err);
template<typename T> auto canonical_path(T pth, fs::path *out, error *err = nullptr) define_fs_conversion_body(fs::_canonical_path, pth, out, err);

fs::path _weakly_canonical_path(fs::const_fs_string pth, error *err);
bool     _weakly_canonical_path(fs::const_fs_string pth, fs::path *out, error *err);
template<typename T> auto weakly_canonical_path(T pth, error *err = nullptr) define_fs_conversion_body(fs::_weakly_canonical_path, pth, err);
template<typename T> auto weakly_canonical_path(T pth, fs::path *out, error *err = nullptr) define_fs_conversion_body(fs::_weakly_canonical_path, pth, out, err);

bool _get_symlink_target(fs::const_fs_string pth, fs::path *out, error *err);
template<typename T> auto get_symlink_target(T pth, fs::path *out, error *err = nullptr) define_fs_conversion_body(fs::_get_symlink_target, pth, out, err);

bool get_current_path(fs::path *out, error *err = nullptr);

bool _set_current_path(fs::const_fs_string pth, error *err);
template<typename T> auto set_current_path(T pth, error *err = nullptr) define_fs_conversion_body(fs::_set_current_path, pth, err)

// out = pth / seg
void append_path(fs::path *out, const char    *seg);
void append_path(fs::path *out, const wchar_t *seg);
void append_path(fs::path *out, const_string   seg);
void append_path(fs::path *out, const_wstring  seg);
void append_path(fs::path *out, const fs::path *to_append);

// out = pth + seg
// does not add a directory separator
void concat_path(fs::path *out, const char    *seg);
void concat_path(fs::path *out, const wchar_t *seg);
void concat_path(fs::path *out, const_string   seg);
void concat_path(fs::path *out, const_wstring  seg);
void concat_path(fs::path *out, const fs::path *to_concat);

void _relative_path(fs::const_fs_string from, fs::const_fs_string to, fs::path *out);

template<typename T1, typename T2>
auto relative_path(T1 from_path, T2 to_path, fs::path *out)
    define_fs_conversion_body2(fs::_relative_path, from_path, to_path, out)

// modification operations

bool _touch(fs::const_fs_string pth, fs::permission perms, error *err);
template<typename T> auto touch(T pth, fs::permission perms = fs::permission::User, error *err = nullptr) define_fs_conversion_body(fs::_touch, pth, perms, err)

enum class copy_file_option
{
    None,               // reports an error when destination exists.
    OverwriteExisting,  // (default) overwrites existing destination.
    UpdateExisting,     // overwrites existing destination ONLY if destination is older than source,
                        // but does not report an error in either case.
                        // looks at MODIFICATION time.
    SkipExisting        // skips any existing destination files.
    // UpdateOnly? might be useful
};

bool _copy_file(fs::const_fs_string from, fs::const_fs_string to, fs::copy_file_option opt, error *err);

template<typename T1, typename T2>
auto copy_file(T1 from, T2 to, fs::copy_file_option opt = fs::copy_file_option::OverwriteExisting, error *err = nullptr)
    define_fs_conversion_body2(fs::_copy_file, from, to, opt, err)

bool _copy_directory(fs::const_fs_string from, fs::const_fs_string to, int max_depth, fs::copy_file_option opt, error *err);

// -1 max depth = everything
// 0 = only current directory
// 1 = current directory and 1st level subdirectories
// 2 = ...
template<typename T1, typename T2>
auto copy_directory(T1 from, T2 to, int max_depth = -1, fs::copy_file_option opt = fs::copy_file_option::OverwriteExisting, error *err = nullptr)
    define_fs_conversion_body2(fs::_copy_directory, from, to, max_depth, opt, err)

// copies files and directories, doesn't matter what you give it
bool _copy(fs::const_fs_string from, fs::const_fs_string to, int max_depth, fs::copy_file_option opt, error *err);

template<typename T1, typename T2>
auto copy(T1 from, T2 to, int max_depth = -1, fs::copy_file_option opt = fs::copy_file_option::OverwriteExisting, error *err = nullptr)
    define_fs_conversion_body2(fs::_copy, from, to, max_depth, opt, err)

// does not create parents
bool _create_directory(fs::const_fs_string pth, fs::permission perms, error *err);

template<typename T>
auto create_directory(T pth, fs::permission perms = fs::permission::User, error *err = nullptr)
    define_fs_conversion_body(fs::_create_directory, pth, perms, err)

// creates parents as well
bool _create_directories(fs::const_fs_string pth, fs::permission perms, error *err);

template<typename T>
auto create_directories(T pth, fs::permission perms = fs::permission::User, error *err = nullptr)
    define_fs_conversion_body(fs::_create_directories, pth, perms, err)

bool _create_hard_link(fs::const_fs_string target, fs::const_fs_string link, error *err);
template<typename T1, typename T2> auto create_hard_link(T1 target, T2 link, error *err = nullptr) define_fs_conversion_body2(fs::_create_hard_link, target, link, err)

bool _create_symlink(fs::const_fs_string target, fs::const_fs_string link, error *err);
template<typename T1, typename T2> auto create_symlink(T1 target, T2 link, error *err = nullptr) define_fs_conversion_body2(fs::_create_symlink, target, link, err)

bool _move(fs::const_fs_string src, fs::const_fs_string dest, error *err = nullptr);
template<typename T1, typename T2> auto move(T1 src, T2 dest, error *err = nullptr) define_fs_conversion_body2(fs::_move, src, dest, err)

bool _remove_file(fs::const_fs_string pth, error *err);
template<typename T> auto remove_file(T pth, error *err = nullptr) define_fs_conversion_body(fs::_remove_file, pth, err)

bool _remove_symlink(fs::const_fs_string pth, error *err);
template<typename T> auto remove_symlink(T pth, error *err = nullptr) define_fs_conversion_body(fs::_remove_symlink, pth, err)

// removes single empty directory
bool _remove_empty_directory(fs::const_fs_string pth, error *err);
template<typename T> auto remove_empty_directory(T pth, error *err = nullptr) define_fs_conversion_body(fs::_remove_empty_directory, pth, err)

// removes all children as well
bool _remove_directory(fs::const_fs_string pth, error *err);
template<typename T> auto remove_directory(T pth, error *err = nullptr) define_fs_conversion_body(fs::_remove_directory, pth, err)

// removes anything. does not return false when removing non-existent things.
bool _remove(fs::const_fs_string pth, error *err);
template<typename T> auto remove(T pth, error *err = nullptr) define_fs_conversion_body(fs::_remove, pth, err)

// gets the paths to children of one directory, not subdirectories
s64 _get_children(fs::const_fs_string pth, array<fs::path> *children, fs::iterate_option options, error *err);

template<typename T> auto get_children_names(T pth, array<fs::path> *children, error *err = nullptr)
    define_fs_conversion_body(fs::_get_children, pth, children, fs::iterate_option::StopOnError, err)

template<typename T> auto get_children_fullpaths(T pth, array<fs::path> *children, error *err = nullptr)
    define_fs_conversion_body(fs::_get_children, pth, children, fs::iterate_option::Fullpaths | fs::iterate_option::StopOnError, err)

// gets subdirectories too
s64 _get_all_descendants(fs::const_fs_string pth, array<fs::path> *descendants, fs::iterate_option options, error *err);

template<typename T> auto get_all_descendants_paths(T pth, array<fs::path> *descendants, error *err = nullptr)
    define_fs_conversion_body(fs::_get_all_descendants, pth, descendants, fs::iterate_option::StopOnError, err)

template<typename T> auto get_all_descendants_fullpaths(T pth, array<fs::path> *descendants, error *err = nullptr)
    define_fs_conversion_body(fs::_get_all_descendants, pth, descendants, fs::iterate_option::Fullpaths | fs::iterate_option::StopOnError, err)

// direct children, -1 = error
s64 _get_children_count(fs::const_fs_string pth, fs::iterate_option options, error *err);

template<typename T> auto get_children_count(T pth, error *err = nullptr)
    define_fs_conversion_body(fs::_get_children_count, pth, fs::iterate_option::StopOnError, err)

s64 _get_descendant_count(fs::const_fs_string pth, fs::iterate_option options, error *err);

template<typename T> auto get_descendant_count(T pth, error *err = nullptr)
    define_fs_conversion_body(fs::_get_descendant_count, pth, fs::iterate_option::StopOnError, err)

// maybe add something along the lines of get_file_count

////////////////////////
// getting special paths
////////////////////////

// location of the executable
bool get_executable_path(fs::path *out, error *err);
// convenience, basically parent_path of get_executable_path
bool get_executable_directory_path(fs::path *out, error *err);

// AppData, .local/share, etc
// will also create the folder if it doesn't exist.
bool _get_preference_path(fs::path *out, const_fs_string app, const_fs_string org, error *err = nullptr);
bool get_preference_path(fs::path *out, error *err = nullptr);

template<typename T>
auto get_preference_path(fs::path *out, T app, error *err = nullptr)
-> decltype(fs::_get_preference_path(out, ::to_const_string(fs::get_platform_string(app)),
                                          const_fs_string{SYS_CHAR(""), 0},
                                          err))
{
    auto pth_str1 = fs::get_platform_string(app);
    auto ret = fs::_get_preference_path(out, ::to_const_string(pth_str1), const_fs_string{SYS_CHAR(""), 0}, err);

    if constexpr (needs_conversion(T))
        fs::free(&pth_str1);

    return ret;
}

template<typename T1, typename T2>
auto get_preference_path(fs::path *out, T1 app, T2 org, error *err = nullptr)
-> decltype(fs::_get_preference_path(out, ::to_const_string(fs::get_platform_string(app)),
                                          ::to_const_string(fs::get_platform_string(org)),
                                          err))
{
    auto pth_str1 = fs::get_platform_string(app);
    auto pth_str2 = fs::get_platform_string(org);

    auto ret = fs::_get_preference_path(out, ::to_const_string(pth_str1), ::to_const_string(pth_str2), err);

    if constexpr (needs_conversion(T1))
        fs::free(&pth_str1);

    if constexpr (needs_conversion(T2))
        fs::free(&pth_str2);

    return ret;
}

// /tmp
bool get_temporary_path(fs::path *out, error *err);
}

// these allocate memory
fs::path operator ""_path(const char    *, u64);
fs::path operator ""_path(const wchar_t *, u64);

#include "fs/impl/iterator.hpp"
