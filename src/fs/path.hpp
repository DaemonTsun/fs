
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

const char *filename(fs::path *pth);

// location of the executable
void get_executable_path(fs::path *out);

// AppData, .local/share, etc
void get_preference_path(fs::path *out, const char *app = nullptr, const char *org = nullptr);
}
