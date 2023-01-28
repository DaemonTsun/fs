
#pragma once

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

void set_path(fs::path *pth, const char *new_path);
void set_path(fs::path *pth, const wchar_t *new_path);

bool exists(const fs::path *pth);
bool is_file(const fs::path *pth);
bool is_directory(const fs::path *pth);
bool is_absolute(const fs::path *pth);
bool is_relative(const fs::path *pth);

const char *filename(const fs::path *pth);
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
}
