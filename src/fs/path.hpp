
/* path.hpp

v0.8
Path and filesystem functions.

All structs and functions in this library are in the fs namespace.

fs::path is a struct similar in structure to a shl string.
Depending on the platform and compile flags, fs::path may hold plain
characters (char/c8) or wide characters (wchar_t/c16/c32), the type used can be obtained
via fs::path_char_t or shl sys_char.

fs::path is used to represent filesystem paths and run filesystem operations
on.

Example: Creating a file named "myfile.txt" in the home directory.

    fs::path p{};
    fs::get_home_path(&p);

    fs::path_append(&p, "myfile.txt");
    fs::touch(&p);
    fs::free(&p);


For convenience, the path_new function creates a new path and optionally resolves
environment variables, e.g.:

    fs::path p = fs::path_new("$HOME/myfile.txt");
    ...
    fs::free(&p);

NOTE: path_new always allocates a new path, so free the variable before reassigning.

::to_const_string works with fs::path and yields a const_fs_string (const_string_base<sys_char>)
of the path.

Note: fs::path has a size member, but because fs uses a lot of
standard library functions / syscalls (e.g. stat), which do not
check for size, a null character is required to terminate a
fs::path.

Most functions accept strings of different character types, even
character types encodings not native to the platform, e.g. wchar_t on
Linux or char on a Unicode Windows.
In this case, the strings will first be converted to the native format,
using e.g. wcstombs or mbstowcs.

Most functions returning a boolean will set the error code and message when
returning false if an error occurred. An error code that is not 0 indicates an error.

Test cases for every function are located at tests/path_tests.cpp. Refer to them
to see how each function is used.

----------
Iterating:
----------

There are multiple macros defined in fs/impl/iterator.hpp (included in this file)
for iterating paths with different options. tests/path_tests.cpp lists multiple
examples on how these are used.

The most basic for_path macro iterates a directory, e.g.:
    my_dir
    |- file1
    |- file2
    |- my_dir2
       |- file3

        for_path(it, "my_dir")
            printf("%s\n", it->path.c_str);

    Prints the following:
        file1
        file2
        my_dir2


    Recursive iterating:

        for_recursive_path(it, "my_dir")
            printf("%s\n", it->path.c_str);

    Prints the following:
        my_dir/file1
        my_dir/file2
        my_dir/my_dir2
        my_dir/my_dir2/file3


For non-recursive iterating, the iteration item ("it" in for_path(it, ...)) has
the following members:

    fs::filesystem_type type;
    fs::const_fs_string path;

    as well as platform-specific members. See fs/impl/iterator.hpp for details.

Recursive iteration items have additional members:

    u32 depth;    // recursion depth, starts at 0
    bool recurse; // whether or not to recurse into the current item
    
for_path and for_recursive_path take optional 3rd and 4th parameters:
fs::iterate_option and error.
See fs/common.hpp for iteration options.

----------
Functions:
----------
( * is a pointer, Path is usually fs::path, PathStr is anything that can be converted
to a const_fs_string, FSInfo is fs::filesystem_info).

init(*Path) initializes an empty Path.
init(*Path, str) initializes Path to a copy of str. Does not normalize Path.
init(*Path, *Path2) initializes Path to a copy of Path2.

path_set(*Path, str) sets Path to a copy of str. Does not normalize Path.
path_set(*Path, *Path2) sets Path to a copy of Path2.

path_new(PathStr, ResolveVariables = true, VariableAliases = true)
    Returns a new fs::path which contains a copy of PathStr.
    If ResolveVariables is true, resolves any environment variables denoted as
    "$Var", e.g. "$HOST.txt".
    If VariableAliases is true, also replaces some variables that are not
    environment variables with the contents of certain other environment
    variables. See shl/environment.hpp for details.

free(*Path) frees the memory of Path.

hash(*Path) returns a hash of the Path string. Note that two equivalent paths may produce
            different hashes. Use fs::are_equivalent(Path1, Path2) to check if two paths
            are equivalent.

query_filesystem(PathStr, *Out, FollowSymlinks = true, Flags = fs::query_flag_default[, *err])
    Queries filesystem information from PathStr. To get information about symlinks,
    set FollowSymlinks to false.
    The flags determine what information is queried and what information is written
    to *Out, see fs/common.hpp for documentation on fs::filesystem_info and which
    flags may be used.
    Returns whether or not the function succeeded.

query_filesystem(Handle, *Out, Flags[, *err])
    Queries filesystem information about an open I/O Handle. 
    The flags determine what information is queried and what information is written
    to *Out, see fs/common.hpp for documentation on fs::filesystem_info and which
    flags may be used.
    Returns whether or not the function succeeded.

get_filesystem_type(PathStr, *Out, FollowSymlinks = true[, *err])
    Queries the filesystem type of the PathStr and writes the result to Out.
    Returns whether or not the function succeeded.

get_filesystem_type(Handle, *Out[, *err])
    Queries the filesystem type of the I/O Handle and writes the result to Out.
    Returns whether or not the function succeeded.

get_filesystem_type(*FSInfo)
    Returns the type of the filesystem_info FSInfo if the type was queried with
    fs::query_filesystem.

NOTE Windows: as of version 0.8, permissions are not yet implemented.
get_permissions(PathStr, *Out, FollowSymlinks = true[, *err])
    Queries the filesystem permissions of the PathStr and writes the result to Out.
    Returns whether or not the function succeeded.

get_permissions(Handle, *Out[, *err])
    Queries the filesystem permissions of the I/O Handle and writes the result to Out.
    Returns whether or not the function succeeded.

get_permissions(*FSInfo)
    Returns the permissions of the filesystem_info FSInfo if the permissions were queried with
    fs::query_filesystem.

set_permissions(PathStr, Perms, FollowSymlinks = true[, *err])
    Sets the filesystem permissions of the PathStr to Perms.
    Returns whether or not the function succeeded.

set_permissions(Handle, Perms[, *err])
    Sets the filesystem permissions of the PathStr to Perms.
    Returns whether or not the function succeeded.

exists(PathStr, FollowSymlinks = true[, *err])
    Returns 1 if PathStr exists, 0 if PathStr does not exist and -1 if an error occurred.

is_X(PathStr, FollowSymlinks = true[, *err]) (replace X with file, pipe, directory, symlink, ...)
    Returns whether or not the type of the PathStr is X.

is_X(Handle[, *err]) (replace X with file, pipe, directory, symlink, ...)
    Returns whether or not the type of the I/O Handle is X.

is_X_info(*FSinfo) (replace X with file, pipe, directory, symlink, ...)
    Returns whether or not the type of the FSinfo is X, if FSinfo was queried with type.

is_absolute(PathStr)
    Returns whether or not PathStr is an absolute path.

is_relative(PathStr)
    Returns whether or not PathStr is a relative path.

are_equivalent(PathStr1, PathStr2, FollowSymlinks = true[, *err])
    Returns whether or not PathStr1 and PathStr2 refer to the same file (or directory, ...) on
    the same filesystem.

are_equivalent_infos(*FSInfo1, *FSInfo2)
    Returns whether or not the filesystem_infos are equivalent.
    Only works when both FSInfos were queried with fs::query_flag::Id.

filename(ConstString)
filename(*Path)
    Returns the filename of Path, or an empty const_string if there is none.
    Does not copy the filename and instead returns a slice to the memory within Path
    which points to the filename.

is_dot(PathStr)
    Returns whether or not PathStr's filename is '.'. Specifically checks the filename, so
    e.g. "/abc/." returns true.

is_dot_dot(PathStr)
    Returns whether or not PathStr's filename is '..'. Specifically checks the filename, so
    e.g. "/abc/.." returns true.

is_dot_or_dot_dot(PathStr)
    Returns whether or not PathStr's filename is '.' or '..'. Specifically checks the filename, so
    e.g. "/abc/." or "/def/.." return true.

file_extension(ConstString)
file_extension(*Path)
    Returns the file extension of Path, or an empty const_string if there is none.
    This returns the full filename for "dotfiles", i.e. files that have no name, only an
    extension.
    Returns an empty const_string for '.' and '..'.

parent_path_segment(ConstString)
parent_path_segment(*Path)
    Returns the parent path segment of the Path, or an empty const_string if there is none.
    Returns Path is Path is a root.
    Does not copy the parent path segment and instead returns a slice to the memory within Path
    which points to the parent path.
    See tests/path_tests.cpp for a comprehensive list of examples.

root(ConstString)
root(*Path)
    Returns the root of the Path, or an empty const_string if there is none.
    Returns Path is Path is a root.
    Does not copy the root and instead returns a slice to the memory within Path
    which points to the root.
    See tests/path_tests.cpp for a comprehensive list of examples.

replace_filename(*Path, NewName)
    If Path has a filename, replaces that filename with NewName.
    NewName is anything that can be converted to a const_fs_string.

path_segments(PathStr, *OutArray)
path_segments(*Path, *OutArray)
    Fills OutArray (a shl array<const_fs_string>) with every "segment" in the path.
    The segments are slices into Path, no copies of Path are made.
    A segment is either the root, a directory or the filename. Filename extensions
    are not a segment. The root is a single segment, even if it contains multiple
    path separators.
    See tests/path_tests.cpp for a comprehensive list of examples.

* The following functions ending in "_path" create a new fs::path object (or
write to an existing one).

parent_path(PathStr)
    Returns a new fs::path that is the parent path of PathStr.

parent_path(PathStr, *OutPath)
    Sets the fs::path OutPath to the parent path of PathStr.

longest_existing_path(PathStr)
    Returns a new fs::path that is the longest path that exists (and is accessible)
    of PathStr.

longest_existing_path(PathStr, *OutPath)
    Sets the fs::path OutPath to the longest path that exists (and is accessible)
    of PathStr.

normalize(*Path)
    Normalizes the path. Normalization follows rules loosely following
    the algorithm described in https://en.cppreference.com/w/cpp/filesystem/path :
    - An empty path stays an empty path
    - When possible, replace non-native directory separators with the native
      directory separators (read: replace / with \ on Windows)
    - Remove ./
    - Remove <xyz>/..[/]
    - Remove any ..[/] directly after root
    - Remove trailing directory separators
    - If after all these steps the path ends up empty, set path to "."

    This function does not call any operating system functions to check the
    existence, validity or accessibility of the path, i.e. all operations
    in this function are done on the Path data, not on the filesystem.
    See tests/path_tests.cpp for a comprehensive list of examples.

absolute_path(PathStr[, *err])
    Returns a new fs::path that is the absolute path of PathStr.

absolute_path(PathStr, *OutPath[, *err])
    Sets the fs::path OutPath to the absolute path of PathStr.
    Returns whether or not the function succeeded.
    
canonical_path(PathStr[, *err])
    Returns a new fs::path that is the canonical path of PathStr.
    A canonical path is a path that contains no relative segments (. or ..) and
    no symlinks. PathStr must be an accessible and existing path, canonical_path
    does not work on paths that do not exist.

canonical_path(PathStr, *OutPath[, *err])
    Sets the fs::path OutPath to the canonical path of PathStr.
    A canonical path is a path that contains no relative segments (. or ..) and
    no symlinks. PathStr must be an accessible and existing path, canonical_path
    does not work on paths that do not exist.
    Returns whether or not the function succeeded.

weakly_canonical_path(PathStr[, *err])
    Returns a new fs::path that is the canonical path of PathStr.
    A canonical path is a path that contains no relative segments (. or ..) and
    no symlinks. Unlike canonical_path, PathStr does not need to exist.

weakly_canonical_path(PathStr, *OutPath[, *err])
    Sets the fs::path OutPath to the canonical path of PathStr.
    A canonical path is a path that contains no relative segments (. or ..) and
    no symlinks. Unlike canonical_path, PathStr does not need to exist.
    Returns whether or not the function succeeded.

get_symlink_target(PathStr, *OutPath[, *err])
    Sets the fs::path OutPath to the target of the symlink that PathStr points
    to.
    Returns whether or not the function succeeded.

get_current_path(*OutPath[, *err])
    Sets the fs::path OutPath to the current working directory.
    Returns whether or not the function succeeded.

set_current_path(PathStr[, *err])
    Sets the current working directory to PathStr.
    Returns whether or not the function succeeded.

path_append(*Path, StrSegment)
    Appends StrSegment to Path, inserting a fs::path_separator before StrSegment if
    Path does not end in a path separator.
    If StrSegment is absolute, sets Path to StrSegment.
    See tests/path_tests.cpp for a comprehensive list of examples.

path_concat(*Path, StrSegment)
    Concatenates StrSegment to the end of Path, regardless of either contents.

relative_path(FromPathStr, ToPathStr, *OutPath)
    Sets OutPath to the relative path from FromPathStr to ToPathStr.
    See tests/path_tests.cpp for a comprehensive list of examples.

-------------------------------
Filesystem modifying functions:
-------------------------------

touch(PathStr, Permissions = User[, *err])
    Creates a file at PathStr with permissions Permissions. If a file already exists
    at PathStr, its access and modification times are set to the current timestamp.
    Returns whether or not the function succeeded.

copy_file(FromPathStr, ToPathStr, Options = fs::copy_file_option::OverwriteExisting[, *err])
    Copies a _file_ from FromPathStr to ToPathStr. Option determines how the
    function behaves, specifically with existing ToPathStr files.

    fs::copy_file_option::None does not overwrite existing files and will report such as an error.
    fs::copy_file_option::OverwriteExisting (default) overwrites existing files.
    fs::copy_file_option::SkipExisting does not overwrite existing files silently (no error).
    fs::copy_file_option::UpdateExisting overwrites existing files only if FromPathStr has a
                                         newer _modification_ time than ToPathStr.
                                         
    Returns whether or not the function succeeded.
    See tests/path_tests.cpp for a comprehensive list of examples.

copy_directory(FromPathStr, ToPathStr, MaxDepth, Options = fs::copy_file_option::OverwriteExisting[, *err])
    (Recursively) copies a directory from FromPathStr to ToPathStr.
    MaxDepth determines the deepest subdirectories to be copied:

    -1 MaxDepth = unlimited subdirectories,
    0 = only the root directory, no subdirectories,
    1 = only the root directory and its immediate subdirectories, ...

    Options determines how the function handles file collisions, see copy_file for details.
    Returns whether or not the function succeeded.
    See tests/path_tests.cpp for a comprehensive list of examples.

copy(FromPathStr, ToPathStr, MaxDepth, Options = fs::copy_file_option::OverwriteExisting[, *err])
    Copies files and directories from FromPathStr to ToPathStr.
    Same options as copy_file and copy_directory.
    Returns whether or not the function succeeded.
    See tests/path_tests.cpp for a comprehensive list of examples.

create_directory(PathStr, Permissions = User[, *err])
    Creates a directory at PathStr with permissions Permissions.
    Returns whether or not the function succeeded, also returns true if a directory
    already exists at PathStr.
    Fails when attempting to create a directory whose parent does not exist (i.e. when
    attempting to create multiple directories), to do this use create_directories instead.

create_directories(PathStr, Permissions = User[, *err])
    Creates a directory at PathStr and all its parents with permissions Permissions.
    Returns whether or not the function succeeded, also returns true if a directory
    already exists at PathStr.

create_hard_link(TargetPathStr, LinkPathStr[, *err])
    Creates a link at LinkPathStr which is a hard link to TargetPathStr.
    Returns whether or not the function succeeded.

create_symlink(TargetPathStr, LinkPathStr[, *err])
    Creates a link at LinkPathStr which is a symbolic link to TargetPathStr.
    Returns whether or not the function succeeded.

move(FromPathStr, ToPathStr[, *err])
    Moves (renames) a file or directory from FromPathStr to ToPathStr.
    Overwrites existing files.
    Returns whether or not the function succeeded.

remove_file(PathStr[, *err])
    Removes the _file_ at PathStr. On Linux, also removes symbolic links.
    Returns whether or not the function succeeded.

remove_symlink(PathStr[, *err])
    Removes the symlink (not its target) at PathStr.
    Returns whether or not the function succeeded.

remove_empty_directory(PathStr[, *err])
    Removes an empty directory at PathStr. Fails if the directory is not empty.
    Returns whether or not the function succeeded.

remove_directory(PathStr[, *err])
    Removes a directory and all its descendants. Stops on the first error.
    Returns whether or not the function succeeded.

remove(PathStr[, *err])
    Removes a file, symlink or directory (and all its descendants) at PathStr.
    Returns whether or not the function succeeded.

get_children_names(PathStr, *OutPathArray[, *err])
    Appends the names of all direct children of the directory at PathStr
    to OutPathArray. The OutPathArray holds fs::paths, and as such the paths
    must be freed.
    Does not include . or .. .
    Returns the number of items added, or -1 on error.

get_children_fullpaths(PathStr, *OutPathArray[, *err])
    Appends the full paths of all direct children of the directory at PathStr
    to OutPathArray. The OutPathArray holds fs::paths, and as such the paths
    must be freed.
    Does not include . or .. .
    Returns the number of items added, or -1 on error.

get_all_descendants_paths(PathStr, *OutPathArray[, *err])
    Appends the relative (to PathStr) paths of all descendants of the
    directory at PathStr to OutPathArray. The OutPathArray holds fs::paths,
    and as such the paths must be freed.
    Does not include . or .. .
    Returns the number of items added, or -1 on error.

get_all_descendants_fullpaths(PathStr, *OutPathArray[, *err])
    Appends the full paths of all descendants of the directory at PathStr
    to OutPathArray. The OutPathArray holds fs::paths, and as such the
    paths must be freed.
    Does not include . or .. .
    Returns the number of items added, or -1 on error.

get_children_count(PathStr[, *err])
    Returns the number of direct children of the directory at PathStr, or -1 on error.
    Does not include . or .. .

get_descendant_count(PathStr[, *err])
    Returns the number of all descendants of the directory at PathStr, or -1 on error.
    Does not include . or .. .

--------------
Special paths:
--------------

get_home_path(*OutPath[, *err])
    Sets OutPath to the home directory of the user executing the process.
    Returns whether or not the function succeeded.

get_executable_path(*OutPath[, *err])
    Sets OutPath to the full path of the executing executable, including the
    executable name.
    Returns whether or not the function succeeded.

get_executable_directory_path(*OutPath[, *err])
    Sets OutPath to the full path of the folder where the executing
    executable is in.
    Returns whether or not the function succeeded.

get_preference_path(*OutPath[, AppString, OrgString, *err])
    Sets OutPath to the system specific app preference path, optionally
    including the application name AppString and the organization or company
    name OrgString.
    The directory will be created if it does not exist.
    Returns whether or not the function succeeded.

    Preference path is e.g. /home/<user>/.local/share/[org/][app/]
                         or C:/Users/<user>/AppData/Roaming/[org/][app/]

get_temporary_path(*OutPath[, *err])
    Sets OutPath to a temporary path as provided by the operating system.
    Returns whether or not the function succeeded.
*/

#pragma once

#include "shl/io.hpp"
#include "shl/hash.hpp"
#include "shl/platform.hpp"
#include "shl/error.hpp"
#include "shl/type_functions.hpp"
#include "shl/allocator.hpp"

#include "fs/common.hpp"
#include "fs/convert.hpp"

namespace fs
{
struct path
{
    typedef fs::path_char_t value_type;

    value_type *data;
    s64 size;
    s64 reserved_size;
    ::allocator allocator;

    operator const value_type*() const;

    const value_type *c_str() const;
};
}

fs::const_fs_string to_const_string(const fs::path_char_t *path);
fs::const_fs_string to_const_string(const fs::path_char_t *path, s64 size);
template<u64 N> fs::const_fs_string to_const_string(const fs::path_char_t path[N]) { return fs::const_fs_string{path, (s64)N}; }
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
sys_string _get_platform_string(::const_string_base<C> str)
{
    sys_string ret{};

    if constexpr (needs_conversion(C))
        ::string_set(&ret, str);
    else
    {
        ret.data = const_cast<C*>(str.c_str);
        ret.size = str.size;
        ret.reserved_size = str.size;
    }

    return ret;
}

/* FYI: we do this auto -> decltype to get better compiler errors when passing
        something to this function that has no to_const_string overload.
*/
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
        if constexpr (needs_conversion(T)) free(&pth_str);                                              \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
        auto ret = Func(::to_const_string(pth_str) __VA_OPT__(,) __VA_ARGS__);                          \
                                                                                                        \
        if constexpr (needs_conversion(T)) free(&pth_str);                                              \
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
        if constexpr (needs_conversion(T1)) free(&pth_str1);                                                                                    \
        if constexpr (needs_conversion(T2)) free(&pth_str2);                                                                                    \
    }                                                                                                                                           \
    else                                                                                                                                        \
    {                                                                                                                                           \
        auto ret = Func(::to_const_string(pth_str1), ::to_const_string(pth_str2) __VA_OPT__(,) __VA_ARGS__);                                    \
                                                                                                                                                \
        if constexpr (needs_conversion(T1)) free(&pth_str1);                                                                                    \
        if constexpr (needs_conversion(T2)) free(&pth_str2);                                                                                    \
                                                                                                                                                \
        return ret;                                                                                                                             \
    }                                                                                                                                           \
}

void init(fs::path *path);
void init(fs::path *path, const c8  *str);
void init(fs::path *path, const c8  *str, s64 size);
void init(fs::path *path, const c16 *str);
void init(fs::path *path, const c16 *str, s64 size);
void init(fs::path *path, const c32 *str);
void init(fs::path *path, const c32 *str, s64 size);
void init(fs::path *path, const_string    str);
void init(fs::path *path, const_u16string str);
void init(fs::path *path, const_u32string str);
void init(fs::path *path, const fs::path *other);

void free(fs::path *path);

void path_set(fs::path *pth, const c8  *new_path);
void path_set(fs::path *pth, const c8  *new_path, s64 size);
void path_set(fs::path *pth, const c16 *new_path);
void path_set(fs::path *pth, const c16 *new_path, s64 size);
void path_set(fs::path *pth, const c32 *new_path);
void path_set(fs::path *pth, const c32 *new_path, s64 size);
void path_set(fs::path *pth, const_string    new_path);
void path_set(fs::path *pth, const_u16string new_path);
void path_set(fs::path *pth, const_u32string new_path);
void path_set(fs::path *pth, const fs::path *new_path);

fs::path _path_new(fs::const_fs_string pth, bool resolve_variables, bool variable_aliases);

template<typename T>
auto path_new(T pth, bool resolve_variables = true, bool variable_aliases = true)
    define_fs_conversion_body(fs::_path_new, pth, resolve_variables, variable_aliases)

bool operator==(const fs::path &lhs, const fs::path &rhs);
bool operator!=(const fs::path &lhs, const fs::path &rhs);

bool query_filesystem(io_handle h, fs::filesystem_info *out, fs::query_flag flags, error *err = nullptr);
bool _query_filesystem(fs::const_fs_string pth, fs::filesystem_info *out, bool follow_symlinks, fs::query_flag flags, error *err);

// type T is anything that can be converted to fs::const_fs_string
template<typename T>
auto query_filesystem(T pth, fs::filesystem_info *out, bool follow_symlinks = true, fs::query_flag flags = fs::query_flag_default, error *err = nullptr)
    define_fs_conversion_body(fs::_query_filesystem, pth, out, follow_symlinks, flags, err)

fs::filesystem_type get_filesystem_type(const fs::filesystem_info *info);

bool get_filesystem_type(io_handle h, fs::filesystem_type *out, error *err = nullptr);
bool _get_filesystem_type(fs::const_fs_string pth, fs::filesystem_type *out, bool follow_symlinks, error *err);
template<typename T>
auto get_filesystem_type(T pth, fs::filesystem_type *out, bool follow_symlinks = true, error *err = nullptr)
    define_fs_conversion_body(fs::_get_filesystem_type, pth, out, follow_symlinks, err);

s64 get_file_size(const fs::filesystem_info *info);

bool get_file_size(io_handle h, s64 *out, error *err = nullptr);
bool _get_file_size(fs::const_fs_string pth, s64 *out, bool follow_symlinks, error *err);
template<typename T>
auto get_file_size(T pth, s64 *out, bool follow_symlinks = true, error *err = nullptr)
    define_fs_conversion_body(fs::_get_file_size, pth, out, follow_symlinks, err);

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
    -> decltype(fs::query_filesystem(Pth, (fs::filesystem_info*)nullptr, follow_symlinks, fs::query_flag::Type, err))\
{\
    fs::filesystem_info info{};\
\
    if (!fs::query_filesystem(Pth, &info, follow_symlinks, fs::query_flag::Type, err))\
        return false;\
\
    return InfoFunc(&info);\
}

// T is anything that can be passed to query_filesystem, i.e. strings and paths
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

bool _is_absolute(fs::const_fs_string pth);
bool _is_relative(fs::const_fs_string pth);

template<typename T> auto is_absolute(T pth) define_fs_conversion_body(fs::_is_absolute, pth)
template<typename T> auto is_relative(T pth) define_fs_conversion_body(fs::_is_relative, pth)

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

/* this differs from std::filesystem::path::extension:
filenames that have no stem (only an extension) are treated exactly like that:
no name, only extension, and as such file_extension returns the full filename
for filenames with no stem.

. and .. are treated the same as std::filesystem::path::extension:
file_extension will return empty strings (not nullptr) in these cases.
*/
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
        free(&pth_str);
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
void path_append(fs::path *out, const c8  *seg);
void path_append(fs::path *out, const c16 *seg);
void path_append(fs::path *out, const c32 *seg);
void path_append(fs::path *out, const_string    seg);
void path_append(fs::path *out, const_u16string seg);
void path_append(fs::path *out, const_u32string seg);
void path_append(fs::path *out, const fs::path *to_append);

// out = pth + seg
// does not add a directory separator
void path_concat(fs::path *out, const c8  *seg);
void path_concat(fs::path *out, const c16 *seg);
void path_concat(fs::path *out, const c32 *seg);
void path_concat(fs::path *out, const_string    seg);
void path_concat(fs::path *out, const_u16string seg);
void path_concat(fs::path *out, const_u32string seg);
void path_concat(fs::path *out, const fs::path *to_concat);

void _relative_path(fs::const_fs_string from, fs::const_fs_string to, fs::path *out);

template<typename T1, typename T2>
auto relative_path(T1 from_path, T2 to_path, fs::path *out)
    define_fs_conversion_body2(fs::_relative_path, from_path, to_path, out)

// modification operations

bool _touch(fs::const_fs_string pth, fs::permission perms, error *err);
template<typename T> auto touch(T pth, fs::permission perms = fs::permission::User, error *err = nullptr) define_fs_conversion_body(fs::_touch, pth, perms, err)

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

// user home directory
bool get_home_path(fs::path *out, error *err = nullptr);

// location of the executable
bool get_executable_path(fs::path *out, error *err = nullptr);
// convenience, basically parent_path of get_executable_path
bool get_executable_directory_path(fs::path *out, error *err = nullptr);

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
        free(&pth_str1);

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

    if constexpr (needs_conversion(T1)) free(&pth_str1);
    if constexpr (needs_conversion(T2)) free(&pth_str2);

    return ret;
}

// /tmp
bool get_temporary_path(fs::path *out, error *err);
}

hash_t hash(const fs::path *pth);

#include "fs/impl/iterator.hpp"
