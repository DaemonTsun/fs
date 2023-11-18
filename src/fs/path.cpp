
// v1.1

#include <assert.h>

#if Windows
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <string.h>
#endif

#include <stdlib.h>

#include "shl/memory.hpp"
#include "shl/error.hpp"

#include "fs/path.hpp"

#if Windows
#define PATH_CHAR_LIT(c) L##c
#else
#define PATH_CHAR_LIT(c) c
#endif

// conversion helpers
#define as_array_ptr(x)     (::array<fs::path_char_t>*)(x)
#define as_string_ptr(x)    (::string_base<fs::path_char_t>*)(x)

struct _mbstring
{
    char *data;
    u64 size;
};

_mbstring wcstring_to_cstring(const wchar_t *wcstring, u64 wchar_count)
{
    _mbstring ret;
    u64 sz = wchar_count * sizeof(char);
    ret.data = (char*)::allocate_memory(sz);

    ::fill_memory(ret.data, 0, sz);

    ret.size = ::wcstombs(ret.data, wcstring, wchar_count * sizeof(wchar_t));

    return ret;
}

struct _wstring
{
    wchar_t *data;
    u64 size;
};

_wstring cstring_to_wcstring(const char *cstring, u64 char_count)
{
    _wstring ret;
    u64 sz = char_count * sizeof(wchar_t);
    ret.data = (wchar_t*)::allocate_memory(sz);

    ::fill_memory(ret.data, 0, sz);

    ret.size = ::mbstowcs(ret.data, cstring, char_count * sizeof(char));

    return ret;
}

// path functions

const fs::path_char_t *fs::path::c_str() const
{
    return this->data;
}

void fs::init(fs::path *path)
{
    assert(path != nullptr);

    ::init(as_string_ptr(path));
}

void fs::init(fs::path *path, const char    *str)
{
    fs::init(path, ::to_const_string(str));
}

void fs::init(fs::path *path, const wchar_t *str)
{
    fs::init(path, ::to_const_string(str));
}

void _path_init(fs::path *path, const fs::path_char_t *str, u64 size)
{
    ::init(as_string_ptr(path), str, size);
}

void fs::init(fs::path *path, const_string   str)
{
    assert(path != nullptr);
    assert(str.c_str != nullptr);

#if Windows
    _wstring converted = ::cstring_to_wcstring(str.c_str, str.size);

    assert(converted.data != nullptr);
    assert(converted.size != (size_t)-1);

    _path_init(path, converted.data, converted.size);

    ::free_memory(converted.data);
#else
    _path_init(path, str.c_str, str.size);
#endif
}

void fs::init(fs::path *path, const_wstring  str)
{
    assert(path != nullptr);
    assert(str.c_str != nullptr);

#if Windows
    _path_init(path, str.c_str, str.size);
#else
    _mbstring converted = ::wcstring_to_cstring(str.c_str, str.size);

    assert(converted.data != nullptr);
    assert(converted.size != (size_t)-1);

    _path_init(path, converted.data, converted.size);

    ::free_memory(converted.data);
#endif
}

void fs::init(fs::path *path, const fs::path *other)
{
    assert(other != nullptr);

    fs::init(path, ::to_const_string(other));
}

void fs::free(fs::path *path)
{
    assert(path != nullptr);

    ::free(as_string_ptr(path));
}

void fs::set_path(fs::path *pth, const char    *new_path)
{
    fs::set_path(pth, ::to_const_string(new_path));
}

void fs::set_path(fs::path *pth, const wchar_t *new_path)
{
    fs::set_path(pth, ::to_const_string(new_path));
}

void fs::set_path(fs::path *pth, const_string   new_path)
{
#if Windows
    _wstring converted = ::wcstring_to_cstring(new_path.c_str, new_path.size);

    assert(converted.data != nullptr);
    assert(converted.size != (size_t)-1);

    ::set_string(as_string_ptr(pth), converted.data, converted.size);

    ::free_memory(converted.data);
#else
    set_string(as_string_ptr(pth), new_path);
#endif
}

void fs::set_path(fs::path *pth, const_wstring  new_path)
{
#if Windows
    set_string(as_string_ptr(pth), new_path);
#else
    _mbstring converted = ::wcstring_to_cstring(new_path.c_str, new_path.size);

    assert(converted.data != nullptr);
    assert(converted.size != (size_t)-1);

    ::set_string(as_string_ptr(pth), converted.data, converted.size);

    ::free_memory(converted.data);
#endif
}

void fs::set_path(fs::path *pth, const fs::path *new_path)
{
    set_string(as_string_ptr(pth), to_const_string(new_path));
}

#if 0
bool fs::operator==(const fs::path &lhs, const fs::path &rhs)
{
    return lhs.ptr->data == rhs.ptr->data;
}

// append operators
#define _APPEND_OPERATOR_BODY(...)\
    {\
        fs::path ret = lhs;\
        fs::append_path(&ret, __VA_ARGS__ seg);\
        return ret;\
    }

fs::path  fs::operator/ (const fs::path &lhs, const char    *seg) _APPEND_OPERATOR_BODY()
fs::path  fs::operator/ (const fs::path &lhs, const wchar_t *seg) _APPEND_OPERATOR_BODY()
fs::path  fs::operator/ (const fs::path &lhs, const_string   seg) _APPEND_OPERATOR_BODY()
fs::path  fs::operator/ (const fs::path &lhs, const_wstring  seg) _APPEND_OPERATOR_BODY()
fs::path  fs::operator/ (const fs::path &lhs, const string  &seg) _APPEND_OPERATOR_BODY(&)
fs::path  fs::operator/ (const fs::path &lhs, const wstring &seg) _APPEND_OPERATOR_BODY(&)
fs::path  fs::operator/ (const fs::path &lhs, const string  *seg) _APPEND_OPERATOR_BODY()
fs::path  fs::operator/ (const fs::path &lhs, const wstring *seg) _APPEND_OPERATOR_BODY()

#define _APPEND_ASSIGNMENT_OPERATOR_BODY(...)\
    {\
        fs::append_path(&lhs, __VA_ARGS__ seg);\
        return lhs;\
    }

fs::path &fs::operator/=(fs::path &lhs, const char    *seg) _APPEND_ASSIGNMENT_OPERATOR_BODY()
fs::path &fs::operator/=(fs::path &lhs, const wchar_t *seg) _APPEND_ASSIGNMENT_OPERATOR_BODY()
fs::path &fs::operator/=(fs::path &lhs, const_string   seg) _APPEND_ASSIGNMENT_OPERATOR_BODY()
fs::path &fs::operator/=(fs::path &lhs, const_wstring  seg) _APPEND_ASSIGNMENT_OPERATOR_BODY()
fs::path &fs::operator/=(fs::path &lhs, const string  &seg) _APPEND_ASSIGNMENT_OPERATOR_BODY(&)
fs::path &fs::operator/=(fs::path &lhs, const wstring &seg) _APPEND_ASSIGNMENT_OPERATOR_BODY(&)
fs::path &fs::operator/=(fs::path &lhs, const string  *seg) _APPEND_ASSIGNMENT_OPERATOR_BODY()
fs::path &fs::operator/=(fs::path &lhs, const wstring *seg) _APPEND_ASSIGNMENT_OPERATOR_BODY()

// concat operators
#define _CONCAT_OPERATOR_BODY(...)\
    {\
        fs::path ret = lhs;\
        fs::concat_path(&ret, __VA_ARGS__ seg);\
        return ret;\
    }

fs::path  fs::operator+ (const fs::path &lhs, const char    *seg) _CONCAT_OPERATOR_BODY()
fs::path  fs::operator+ (const fs::path &lhs, const wchar_t *seg) _CONCAT_OPERATOR_BODY()
fs::path  fs::operator+ (const fs::path &lhs, const_string   seg) _CONCAT_OPERATOR_BODY()
fs::path  fs::operator+ (const fs::path &lhs, const_wstring  seg) _CONCAT_OPERATOR_BODY()
fs::path  fs::operator+ (const fs::path &lhs, const string  &seg) _CONCAT_OPERATOR_BODY(&)
fs::path  fs::operator+ (const fs::path &lhs, const wstring &seg) _CONCAT_OPERATOR_BODY(&)
fs::path  fs::operator+ (const fs::path &lhs, const string  *seg) _CONCAT_OPERATOR_BODY()
fs::path  fs::operator+ (const fs::path &lhs, const wstring *seg) _CONCAT_OPERATOR_BODY()

#define _CONCAT_ASSIGNMENT_OPERATOR_BODY(...)\
    {\
        fs::concat_path(&lhs, __VA_ARGS__ seg);\
        return lhs;\
    }

fs::path &fs::operator+=(fs::path &lhs, const char    *seg) _CONCAT_ASSIGNMENT_OPERATOR_BODY()
fs::path &fs::operator+=(fs::path &lhs, const wchar_t *seg) _CONCAT_ASSIGNMENT_OPERATOR_BODY()
fs::path &fs::operator+=(fs::path &lhs, const_string   seg) _CONCAT_ASSIGNMENT_OPERATOR_BODY()
fs::path &fs::operator+=(fs::path &lhs, const_wstring  seg) _CONCAT_ASSIGNMENT_OPERATOR_BODY()
fs::path &fs::operator+=(fs::path &lhs, const string  &seg) _CONCAT_ASSIGNMENT_OPERATOR_BODY(&)
fs::path &fs::operator+=(fs::path &lhs, const wstring &seg) _CONCAT_ASSIGNMENT_OPERATOR_BODY(&)
fs::path &fs::operator+=(fs::path &lhs, const string  *seg) _CONCAT_ASSIGNMENT_OPERATOR_BODY()
fs::path &fs::operator+=(fs::path &lhs, const wstring *seg) _CONCAT_ASSIGNMENT_OPERATOR_BODY()

hash_t fs::hash(const fs::path *pth)
{
    const char *cstr = pth->c_str();
    return hash_data(cstr, strlen(cstr));
}

void fs::set_path(fs::path *pth, const char *new_path)
{
    pth->ptr->data.assign(new_path);
}

void fs::set_path(fs::path *pth, const wchar_t *new_path)
{
    pth->ptr->data.assign(new_path);
}

void fs::set_path(fs::path *pth, const_string new_path)
{
    pth->ptr->data.assign(new_path.c_str);
}

void fs::set_path(fs::path *pth, const_wstring new_path)
{
    pth->ptr->data.assign(new_path.c_str);
}

void fs::set_path(fs::path *pth, const string *new_path)
{
    pth->ptr->data.assign(new_path->data.data);
}

void fs::set_path(fs::path *pth, const wstring *new_path)
{
    pth->ptr->data.assign(new_path->data.data);
}

bool fs::exists(const fs::path *pth)
{
    return std::filesystem::exists(pth->ptr->data);
}

bool fs::is_file(const fs::path *pth)
{
    return std::filesystem::is_regular_file(pth->ptr->data);
}

bool fs::is_directory(const fs::path *pth)
{
    return std::filesystem::is_directory(pth->ptr->data);
}

bool fs::is_absolute(const fs::path *pth)
{
    return pth->ptr->data.is_absolute();
}

bool fs::is_relative(const fs::path *pth)
{
    return pth->ptr->data.is_relative();
}

bool fs::are_equivalent(const fs::path *pth1, const fs::path *pth2)
{
    return std::filesystem::equivalent(pth1->ptr->data, pth2->ptr->data);
}

const char *fs::filename(const fs::path *pth)
{
    const char *cstr = pth->c_str();
    const char *ret;

#if Windows
    ret = strrchr(cstr, '\\');
#else
    ret = strrchr(cstr, '/');
#endif

    if (ret == nullptr)
        ret = cstr;
    else
        ret++;

    return ret;
}

const char *fs::extension(const fs::path *pth)
{
    const char *ret = pth->c_str();

    ret = strrchr(ret, '.');

    if (ret == nullptr)
        ret = "";

    return ret;
}

void fs::parent_path(fs::path *out)
{
    fs::parent_path(out, out);
}

void fs::parent_path(const fs::path *pth, fs::path *out)
{
    out->ptr->data = pth->ptr->data.parent_path();
}

void fs::append_path(fs::path *out, const char    *seg)
{
    out->ptr->data.append(seg);
}

void fs::append_path(fs::path *out, const wchar_t *seg)
{
    out->ptr->data.append(seg);
}

void fs::append_path(fs::path *out, const_string   seg)
{
    fs::append_path(out, seg.c_str);
}

void fs::append_path(fs::path *out, const_wstring  seg)
{
    fs::append_path(out, seg.c_str);
}

void fs::append_path(fs::path *out, const string  *seg)
{
    fs::append_path(out, to_const_string(seg));
}

void fs::append_path(fs::path *out, const wstring *seg)
{
    fs::append_path(out, to_const_string(seg));
}

void fs::append_path(const fs::path *pth, const char    *seg, fs::path *out)
{
    out->ptr->data = pth->ptr->data / seg;
}

void fs::append_path(const fs::path *pth, const wchar_t *seg, fs::path *out)
{
    out->ptr->data = pth->ptr->data / seg;
}

void fs::append_path(const fs::path *pth, const_string   seg, fs::path *out)
{
    fs::append_path(pth, seg.c_str, out);
}

void fs::append_path(const fs::path *pth, const_wstring  seg, fs::path *out)
{
    fs::append_path(pth, seg.c_str, out);
}

void fs::append_path(const fs::path *pth, const string  *seg, fs::path *out)
{
    fs::append_path(pth, to_const_string(seg), out);
}

void fs::append_path(const fs::path *pth, const wstring *seg, fs::path *out)
{
    fs::append_path(pth, to_const_string(seg), out);
}

void fs::concat_path(fs::path *out, const char    *seg)
{
    out->ptr->data.concat(seg);
}

void fs::concat_path(fs::path *out, const wchar_t *seg)
{
    out->ptr->data.concat(seg);
}

void fs::concat_path(fs::path *out, const_string   seg)
{
    fs::concat_path(out, seg.c_str);
}

void fs::concat_path(fs::path *out, const_wstring  seg)
{
    fs::concat_path(out, seg.c_str);
}

void fs::concat_path(fs::path *out, const string  *seg)
{
    fs::concat_path(out, to_const_string(seg));
}

void fs::concat_path(fs::path *out, const wstring *seg)
{
    fs::concat_path(out, to_const_string(seg));
}

void fs::concat_path(const fs::path *pth, const char    *seg, fs::path *out)
{
    out->ptr->data = pth->ptr->data;
    out->ptr->data.concat(seg);
}

void fs::concat_path(const fs::path *pth, const wchar_t *seg, fs::path *out)
{
    out->ptr->data = pth->ptr->data;
    out->ptr->data.concat(seg);
}

void fs::concat_path(const fs::path *pth, const_string   seg, fs::path *out)
{
    fs::concat_path(pth, seg.c_str, out);
}

void fs::concat_path(const fs::path *pth, const_wstring  seg, fs::path *out)
{
    fs::concat_path(pth, seg.c_str, out);
}

void fs::concat_path(const fs::path *pth, const string  *seg, fs::path *out)
{
    fs::concat_path(pth, to_const_string(seg), out);
}

void fs::concat_path(const fs::path *pth, const wstring *seg, fs::path *out)
{
    fs::concat_path(pth, to_const_string(seg), out);
}

void fs::canonical_path(fs::path *out)
{
    fs::canonical_path(out, out);
}

void fs::canonical_path(const fs::path *pth, fs::path *out)
{
    out->ptr->data = std::filesystem::canonical(pth->ptr->data);
}

void fs::weakly_canonical_path(fs::path *out)
{
    fs::weakly_canonical_path(out, out);
}

void fs::weakly_canonical_path(const fs::path *pth, fs::path *out)
{
    out->ptr->data = std::filesystem::weakly_canonical(pth->ptr->data);
}

void fs::absolute_path(fs::path *out)
{
    fs::absolute_path(out, out);
}

void fs::absolute_path(const fs::path *pth, fs::path *out)
{
    out->ptr->data = std::filesystem::absolute(pth->ptr->data);
}

void fs::absolute_canonical_path(fs::path *out)
{
    fs::absolute_canonical_path(out, out);
}

void fs::absolute_canonical_path(const fs::path *pth, fs::path *out)
{
    out->ptr->data = std::filesystem::absolute(std::filesystem::canonical(pth->ptr->data));
}

void fs::relative_path(const fs::path *from, const fs::path *to, fs::path *out)
{
    out->ptr->data = std::filesystem::relative(to->ptr->data, from->ptr->data);
}

void fs::proximate_path(const fs::path *from, const fs::path *to, fs::path *out)
{
    out->ptr->data = std::filesystem::proximate(to->ptr->data, from->ptr->data);
}

void fs::copy(const fs::path *from, const fs::path *to)
{
    std::filesystem::copy(from->ptr->data, to->ptr->data);
}

bool fs::create_directory(const fs::path *pth)
{
    return std::filesystem::create_directory(pth->ptr->data);
}

bool fs::create_directories(const fs::path *pth)
{
    return std::filesystem::create_directories(pth->ptr->data);
}

void fs::create_hard_link(const fs::path *target, const fs::path *link)
{
    std::filesystem::create_hard_link(target->ptr->data, link->ptr->data);
}

void fs::create_file_symlink(const fs::path *target, const fs::path *link)
{
    std::filesystem::create_symlink(target->ptr->data, link->ptr->data);
}

void fs::create_directory_symlink(const fs::path *target, const fs::path *link)
{
    std::filesystem::create_directory_symlink(target->ptr->data, link->ptr->data);
}

void fs::move(const fs::path *from, const fs::path *to)
{
    std::filesystem::rename(from->ptr->data, to->ptr->data);
}

bool fs::remove(const fs::path *pth)
{
    return std::filesystem::remove(pth->ptr->data);
}

bool fs::remove_all(const fs::path *pth)
{
    return std::filesystem::remove_all(pth->ptr->data);
}

void fs::get_current_path(fs::path *out)
{
    out->ptr->data = std::filesystem::current_path();
}

void fs::set_current_path(const fs::path *pth)
{
    std::filesystem::current_path(pth->ptr->data);
}

void fs::get_executable_path(fs::path *out)
{
#if Linux
    char pth[PATH_MAX] = {0};

    if (readlink("/proc/self/exe", pth, PATH_MAX) < 0)
        throw_error("could not get executable path: %s", strerror(errno));

#elif Windows
    wchar_t pth[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, pth, MAX_PATH);
#else
    #error "unsupported"
#endif

    fs::set_path(out, pth);
}

void fs::get_executable_directory_path(fs::path *out)
{
    fs::get_executable_path(out);
    fs::parent_path(out);
}

/* SDL zlib license
Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>
  
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
  
1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required. 
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
void fs::get_preference_path(fs::path *out, const char *app, const char *org)
{
    // THIS IS ALTERED.
    char *retval = nullptr;

#if Linux
    char buf[PATH_MAX] = {0};

    const char *envr = getenv("XDG_DATA_HOME");
    const char *append;

    if (app == nullptr)
        app = "";

    if (org == nullptr)
        org = "";

    if (envr == NULL)
    {
        // You end up with "$HOME/.local/share/Name"
        envr = getenv("HOME");

        if (envr == NULL)
            throw_error("neither XDG_DATA_HOME nor HOME environment variables are defined");

        append = "/.local/share/";
    }
    else
        append = "/";

    size_t len = strlen(envr);

    if (envr[len - 1] == '/')
        append += 1;

    len += strlen(append) + strlen(org) + strlen(app) + 3;

    if (*org)
        snprintf(buf, len, "%s%s%s/%s/", envr, append, org, app);
    else
        snprintf(buf, len, "%s%s%s/", envr, append, app);

    // recursively create the directories
    for (char *ptr = buf + 1; *ptr; ptr++)
    {
        if (*ptr == '/')
        {
            *ptr = '\0';

            if (mkdir(buf, 0700) != 0 && errno != EEXIST)
                throw_error("couldn't create directory '%s': '%s'", buf, strerror(errno));

            *ptr = '/';
        }
    }

    if (mkdir(retval, 0700) != 0 && errno != EEXIST)
        throw_error("couldn't create directory '%s': '%s'", buf, strerror(errno));

    retval = buf;

#elif Windows

#if 0
    WCHAR path[MAX_PATH];
    WCHAR *worg = NULL;
    WCHAR *wapp = NULL;
    size_t new_wpath_len = 0;
    BOOL api_result = FALSE;

    if (app == NULL)
        app = "";

    if (org == NULL)
        org = "";

    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, path))) {
        WIN_SetError("could not locate our prefpath");
        return NULL;
    }

    worg = WIN_UTF8ToStringW(org);

    if (worg == NULL)
        throw_error("out of memory");

    wapp = WIN_UTF8ToStringW(app);

    if (wapp == NULL)
    {
        free(worg);
        throw_error("out of memory");
    }

    new_wpath_len = string_length(worg) + string_length(wapp) + string_length(path) + 3;

    if ((new_wpath_len + 1) > MAX_PATH)
    {
        free(worg);
        free(wapp);
        WIN_SetError("path too long");
        return NULL;
    }

    if (*worg)
    {
        wcslcat(path, L"\\", arraysize(path));
        wcslcat(path, worg, arraysize(path));
    }

    free(worg);

    api_result = CreateDirectoryW(path, NULL);
    
    if (api_result == FALSE)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            free(wapp);
            WIN_SetError("Couldn't create a prefpath.");
            return NULL;
        }
    }

    wcslcat(path, L"\\", arraysize(path));
    wcslcat(path, wapp, arraysize(path));
    free(wapp);

    api_result = CreateDirectoryW(path, NULL);

    if (api_result == FALSE)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            WIN_SetError("Couldn't create a prefpath.");
            return NULL;
        }
    }

    wcslcat(path, L"\\", arraysize(path));

    retval = WIN_StringToUTF8W(path);
#endif

#else
#error "unsupported platform"
#endif
    
    if (retval == nullptr)
        throw_error("could not get preference path");

    fs::set_path(out, retval);
}

void fs::get_temporary_path(fs::path *out)
{
    out->ptr->data = std::filesystem::temp_directory_path();
}

#endif

fs::path operator ""_path(const char    *pth, u64 size)
{
    fs::path ret;
    fs::init(&ret, ::to_const_string(pth, size));
    return ret;
}

fs::path operator ""_path(const wchar_t *pth, u64 size)
{
    fs::path ret;
    fs::init(&ret, ::to_const_string(pth, size));
    return ret;
}

const_string_base<fs::path_char_t> to_const_string(const fs::path *path)
{
    assert(path != nullptr);
    return const_string_base<fs::path_char_t>{path->data, path->size};
}
