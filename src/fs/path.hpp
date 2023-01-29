
#pragma once

#include "shl/hash.hpp"

namespace fs
{
struct path
{
    struct _path;
    _path *ptr;

    path();
    path(const char *pth);
    path(const wchar_t *pth);
    path(const fs::path &other);
    path(fs::path &&other);

    ~path();

    path &operator=(const fs::path &other);
    path &operator=(fs::path &&other);
    operator const char*() const;

    const char *c_str() const;
};

bool operator==(const fs::path &lhs, const fs::path &rhs);

hash_t hash(const fs::path *pth);

void set_path(fs::path *pth, const char *new_path);
void set_path(fs::path *pth, const wchar_t *new_path);

bool exists(const fs::path *pth);
bool is_file(const fs::path *pth);
bool is_directory(const fs::path *pth);
bool is_absolute(const fs::path *pth);
bool is_relative(const fs::path *pth);
bool are_equivalent(const fs::path *pth1, const fs::path *pth2);

const char *filename(const fs::path *pth);
const char *extension(const fs::path *pth);
void parent_path(const fs::path *pth, fs::path *out);

// out = pth / seg
// basically adds a directory separator and the segment to the path
void append_path(fs::path *out, const char *seg);
void append_path(fs::path *out, const wchar_t *seg);
void append_path(const fs::path *pth, const char *seg, fs::path *out);
void append_path(const fs::path *pth, const wchar_t *seg, fs::path *out);

// out = pth + seg
// does not add a directory separator
void concat_path(fs::path *out, const char *seg);
void concat_path(fs::path *out, const wchar_t *seg);
void concat_path(const fs::path *pth, const char *seg, fs::path *out);
void concat_path(const fs::path *pth, const wchar_t *seg, fs::path *out);

void canonical_path(const fs::path *pth, fs::path *out);
void weakly_canonical_path(const fs::path *pth, fs::path *out);
void absolute_path(const fs::path *pth, fs::path *out);
void absolute_canonical_path(const fs::path *pth, fs::path *out); // often used convenience
void relative_path(const fs::path *from, const fs::path *to, fs::path *out);
void proximate_path(const fs::path *from, const fs::path *to, fs::path *out);

void copy(const fs::path *from, const fs::path *to);
bool create_directory(const fs::path *pth);
bool create_directories(const fs::path *pth);
void create_hard_symlink(const fs::path *target, const fs::path *link);
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

// AppData, .local/share, etc
void get_preference_path(fs::path *out, const char *app = nullptr, const char *org = nullptr);

// /tmp
void get_temporary_path(fs::path *out);
}
