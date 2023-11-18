
/* path.hpp
 *
 * alternative to std::filesystem::path (or, right now, a bad wrapper
 * around them) with a better interface.
 *
 * fs::set_path(path) set a new path.
 *
 * exists(path) checks if the given path exists
 * is_file(path) checks if the given path is a file
 * is_directory(path) checks if the given path is a directory
 * is_absolute(path) checks if the given path is absolute
 * is_relative(path) checks if the given path is relative
 * are_equivalent(path1, path2) checks if path1 and path2 point to the same filesystem entry
 *
 * filename(path) returns a char pointer to the filename of the path.
 *                the pointer is invalidated when the path changes.
 * extension(path) returns a char pointer to the the extension
 *                 of the file of the path.
 *                 the pointer is invalidated when the path changes.
 *                                
 ************************************
 * path modification / related paths:
 *
 * parent_path(path) sets path to its parent path.
 * parent_path(path, out) sets out to the parent path of path.
 *
 * append_path(path, seg) adds the segment seg to path, such that path = path / seg
 * append_path(path, seg, out) out = path / seg
 *
 * concat_path(path, seg) concatenates seg to path, so that path = path seg
 * concat_path(path, seg, out) out = path seg.
 * 
 * canonical_path(path) sets path to its canonical path.
 * canonical_path(path, out) sets out the the canonical path of path.
 *
 * weakly_canonical_path(path) sets path to its weakly canonical path.
 * weakly_canonical_path(path, out) sets out to the weakly canonical path of path.
 *
 * absolute_path(path) makes path absolute
 * absolute_path(path, out) sets out to the absolute path of path.
 *
 * absolute_canonical_path(path) combines canonical_path and absolute_path
 * absolute_canonical_path(path, out) combines canonical_path and absolute_path
 *
 * relative_path(from_path, to_path, out) sets out to the relative path from from_path
 *                                        to to_path, so that adding out to from_path
 *                                        yields to_path.
 *
 * proximate_path(from_path, to_path, out) c++ proximate_path
 *
 *************************
 * filesystem modification
 *
 * copy(from_path, to_path) copies the filesystem entry from from_path to to_path.
 *
 * create_directory(path) creates a directory and returns whether it was successful or not.
 *                        does not create parents.
 *
 * create_directories(path) creates all directories until path and returns whether
 *                          it was successful or not.
 *                          does create parents.
 *
 * create_hard_link(target_path, link_path) creates a hard link at link_path to target_path.
 * create_file_symlink(target_path, link_path) creates a file symlink at link_path
 *                                             to target_path.
 * create_directory_symlink(target_path, link_path) creates a directory symlink at link_path
 *                                             to target_path.
 * move(from_path, to_path) moves the filesystem entry from from_path to to_path.
 *
 * remove(path) removes the filesystem entry at path and returns whether it was
 *              successful or not.
 * remove_all(path) removes the filesystem entry, including children,
 *                  at path and returns whether it was successful or not.
 *
 ***************
 * special paths
 *
 * get_current_path(out) sets out to the current working directory.
 * set_current_path(path) sets the current working directory to path.
 *
 * get_executable_path(out) sets out to the path of the running executable.
 * get_executable_directory_path(out) sets out to the path where the running
 *                                    executable is located at.
 *
 * get_preference_path(out, app = nullptr, org = nullptr)
 *     gets the local preference path.
 *     on Windows, this is likely AppData\Local\[Org\]App\.
 *     on Linux, this is .local/share/[org/]app/.
 *              
 * get_temporary_path(out) gets a temporary directory path.
 */

#pragma once

#include "shl/hash.hpp"
#include "shl/string.hpp"
#include "shl/platform.hpp"

namespace fs
{
#if Windows
typedef wchar_t path_char_t;
constexpr const path_char_t path_separator = L'\\';
#else
typedef char path_char_t;
constexpr const path_char_t path_separator = '/';
#endif

struct path
{
    typedef fs::path_char_t value_type;

    value_type *data;
    u64 size;
    u64 reserved_size;

    operator const value_type*() const;

    const value_type *c_str() const;
};

void init(fs::path *path);
void init(fs::path *path, const char    *str);
void init(fs::path *path, const wchar_t *str);
void init(fs::path *path, const_string   str);
void init(fs::path *path, const_wstring  str);
void init(fs::path *path, const fs::path *other);

void free(fs::path *path);

void set_path(fs::path *pth, const char    *new_path);
void set_path(fs::path *pth, const wchar_t *new_path);
void set_path(fs::path *pth, const_string   new_path);
void set_path(fs::path *pth, const_wstring  new_path);
void set_path(fs::path *pth, const fs::path *new_path);

/*
bool operator==(const fs::path &lhs, const fs::path &rhs);

// see append_path
fs::path  operator/ (const fs::path &lhs, const char    *seg);
fs::path  operator/ (const fs::path &lhs, const wchar_t *seg);
fs::path  operator/ (const fs::path &lhs, const_string   seg);
fs::path  operator/ (const fs::path &lhs, const_wstring  seg);

fs::path &operator/=(fs::path &lhs, const char    *seg);
fs::path &operator/=(fs::path &lhs, const wchar_t *seg);
fs::path &operator/=(fs::path &lhs, const_string   seg);
fs::path &operator/=(fs::path &lhs, const_wstring  seg);

// see concat_path
fs::path  operator+ (const fs::path &lhs, const char    *seg);
fs::path  operator+ (const fs::path &lhs, const wchar_t *seg);
fs::path  operator+ (const fs::path &lhs, const_string   seg);
fs::path  operator+ (const fs::path &lhs, const_wstring  seg);

fs::path &operator+=(fs::path &lhs, const char    *seg);
fs::path &operator+=(fs::path &lhs, const wchar_t *seg);
fs::path &operator+=(fs::path &lhs, const_string   seg);
fs::path &operator+=(fs::path &lhs, const_wstring  seg);

hash_t hash(const fs::path *pth);

bool exists(const fs::path *pth);
bool is_file(const fs::path *pth);
bool is_directory(const fs::path *pth);
bool is_absolute(const fs::path *pth);
bool is_relative(const fs::path *pth);
bool are_equivalent(const fs::path *pth1, const fs::path *pth2);

const char *filename(const fs::path *pth);
const char *extension(const fs::path *pth);
void parent_path(fs::path *out);
void parent_path(const fs::path *pth, fs::path *out);

// out = pth / seg
// basically adds a directory separator and the segment to the path
void append_path(fs::path *out, const char    *seg);
void append_path(fs::path *out, const wchar_t *seg);
void append_path(fs::path *out, const_string   seg);
void append_path(fs::path *out, const_wstring  seg);
void append_path(const fs::path *pth, const char    *seg, fs::path *out);
void append_path(const fs::path *pth, const wchar_t *seg, fs::path *out);
void append_path(const fs::path *pth, const_string   seg, fs::path *out);
void append_path(const fs::path *pth, const_wstring  seg, fs::path *out);

// out = pth + seg
// does not add a directory separator
void concat_path(fs::path *out, const char    *seg);
void concat_path(fs::path *out, const wchar_t *seg);
void concat_path(fs::path *out, const_string   seg);
void concat_path(fs::path *out, const_wstring  seg);
void concat_path(const fs::path *pth, const char    *seg, fs::path *out);
void concat_path(const fs::path *pth, const wchar_t *seg, fs::path *out);
void concat_path(const fs::path *pth, const_string   seg, fs::path *out);
void concat_path(const fs::path *pth, const_wstring  seg, fs::path *out);

void canonical_path(fs::path *out);
void canonical_path(const fs::path *pth, fs::path *out);
void weakly_canonical_path(fs::path *out);
void weakly_canonical_path(const fs::path *pth, fs::path *out);
void absolute_path(fs::path *out);
void absolute_path(const fs::path *pth, fs::path *out);
// often used convenience
void absolute_canonical_path(fs::path *out);
void absolute_canonical_path(const fs::path *pth, fs::path *out);
void relative_path(const fs::path *from, const fs::path *to, fs::path *out);
void proximate_path(const fs::path *from, const fs::path *to, fs::path *out);

// copies what the first path is pointing to to the location of the second path
void copy(const fs::path *from, const fs::path *to);
// does not create parents
bool create_directory(const fs::path *pth);
// creates parents as well
bool create_directories(const fs::path *pth);
void create_hard_link(const fs::path *target, const fs::path *link);
void create_file_symlink(const fs::path *target, const fs::path *link);
void create_directory_symlink(const fs::path *target, const fs::path *link);
void move(const fs::path *from, const fs::path *to);
bool remove(const fs::path *pth);
bool remove_all(const fs::path *pth);

////////////////////////
// getting special paths
////////////////////////

// current working directory
void get_current_path(fs::path *out);
void set_current_path(const fs::path *pth);

// location of the executable
void get_executable_path(fs::path *out);
// convenience, basically parent_path of get_executable_path
void get_executable_directory_path(fs::path *out); 

// AppData, .local/share, etc
void get_preference_path(fs::path *out, const char *app = nullptr, const char *org = nullptr);

// /tmp
void get_temporary_path(fs::path *out);
*/
}

// these allocate memory
fs::path operator ""_path(const char    *, u64);
fs::path operator ""_path(const wchar_t *, u64);

const_string_base<fs::path_char_t> to_const_string(const fs::path *path);
