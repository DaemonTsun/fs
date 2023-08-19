
// v1.0
#include "shl/platform.hpp"

#if Windows
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <string.h>
#endif

#include "shl/string.hpp"
#include "shl/error.hpp"

#include "fs/impl/path.hpp"

#if Windows
char *convert_wide_string(const wchar_t *input)
{
    static char *_buf = nullptr;
    constexpr size_t _buf_size = sizeof(wchar_t) * MAX_PATH;

    if (_buf == nullptr)
        _buf = (char*)malloc(_buf_size);

    memset(_buf, 0, _buf_size);

    wcstombs(_buf, input, _buf_size);

    return _buf;
}
#endif

fs::path::path()
{
    this->ptr = new fs::path::_path;
}

fs::path::path(const char *pth)
{
    this->ptr = new fs::path::_path{{pth}};
}

fs::path::path(const wchar_t *pth)
{
    this->ptr = new fs::path::_path{{pth}};
}

fs::path::path(const_string pth)
{
    this->ptr = new fs::path::_path{{pth.c_str}};
}

fs::path::path(const_wstring pth)
{
    this->ptr = new fs::path::_path{{pth.c_str}};
}

fs::path::path(const string *pth)
{
    this->ptr = new fs::path::_path{{pth->data.data}};
}

fs::path::path(const wstring *pth)
{
    this->ptr = new fs::path::_path{{pth->data.data}};
}

fs::path::path(const fs::path &other)
{
    this->ptr = new fs::path::_path{other.ptr->data};
}

fs::path::path(fs::path &&other)
{
    this->ptr = other.ptr;
    other.ptr = nullptr;
}

fs::path::~path()
{
    if (this->ptr != nullptr)
        delete this->ptr;

    this->ptr = nullptr;
}

fs::path &fs::path::operator=(const fs::path &other)
{
    this->ptr = new fs::path::_path{other.ptr->data};
    return *this;
}

fs::path &fs::path::operator=(fs::path &&other)
{
    this->ptr = other.ptr;
    other.ptr = nullptr;
    return *this;
}

fs::path::operator const char*() const
{
#if Windows
    return convert_wide_string(this->ptr->data.c_str());
#else
    return this->ptr->data.c_str();
#endif
}

const char *fs::path::c_str() const
{
#if Windows
    return convert_wide_string(this->ptr->data.c_str());
#else
    return this->ptr->data.c_str();
#endif
}

bool fs::operator==(const fs::path &lhs, const fs::path &rhs)
{
    return lhs.ptr->data == rhs.ptr->data;
}

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

void fs::append_path(fs::path *out, const char *seg)
{
    out->ptr->data.append(seg);
}

void fs::append_path(fs::path *out, const wchar_t *seg)
{
    out->ptr->data.append(seg);
}

void fs::append_path(const fs::path *pth, const char *seg, fs::path *out)
{
    out->ptr->data = pth->ptr->data / seg;
}

void fs::append_path(const fs::path *pth, const wchar_t *seg, fs::path *out)
{
    out->ptr->data = pth->ptr->data / seg;
}

void fs::concat_path(fs::path *out, const char *seg)
{
    out->ptr->data.concat(seg);
}

void fs::concat_path(fs::path *out, const wchar_t *seg)
{
    out->ptr->data.concat(seg);
}

void fs::concat_path(const fs::path *pth, const char *seg, fs::path *out)
{
    out->ptr->data = pth->ptr->data;
    out->ptr->data.concat(seg);
}

void fs::concat_path(const fs::path *pth, const wchar_t *seg, fs::path *out)
{
    out->ptr->data = pth->ptr->data;
    out->ptr->data.concat(seg);
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
