
/* path.hpp
 TODO: write new docs
 */

#pragma once

#include "shl/hash.hpp"
#include "shl/string.hpp"
#include "shl/platform.hpp"

#include "fs/fs_error.hpp"

#if Windows

#else
// I doubt there's any way around including this, stat is a syscall and
// the struct can change depending on the system.
#include <sys/stat.h>
#endif

namespace fs
{
#if Windows

typedef wchar_t path_char_t;
constexpr const path_char_t path_separator = L'\\';

struct filesystem_info {}; // TODO: define

#else
// Linux and others

typedef char path_char_t;
constexpr const path_char_t path_separator = '/';

typedef struct stat filesystem_info; 

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

bool operator==(const fs::path &lhs, const fs::path &rhs);

hash_t hash(const fs::path *pth);

bool exists(const fs::path *pth, fs::fs_error *err = nullptr);

bool is_file(const fs::path *pth, fs::fs_error *err = nullptr);
bool is_pipe(const fs::path *pth, fs::fs_error *err = nullptr);
bool is_block_device(const fs::path *pth, fs::fs_error *err = nullptr);
bool is_special_character_file(const fs::path *pth, fs::fs_error *err = nullptr);
bool is_socket(const fs::path *pth, fs::fs_error *err = nullptr);
bool is_symlink(const fs::path *pth, fs::fs_error *err = nullptr);
bool is_directory(const fs::path *pth, fs::fs_error *err = nullptr);
bool is_other(const fs::path *pth, fs::fs_error *err = nullptr);

bool is_absolute(const fs::path *pth, fs::fs_error *err = nullptr);
bool is_relative(const fs::path *pth, fs::fs_error *err = nullptr);
bool are_equivalent(const fs::path *pth1, const fs::path *pth2, fs::fs_error *err = nullptr);

/*
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
