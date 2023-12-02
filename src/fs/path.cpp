
// v1.1

#include <assert.h>

#if Windows
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#endif

#include <stdlib.h>

#include "shl/memory.hpp"
#include "shl/error.hpp"

#include "fs/path.hpp"

#if Windows
// PC stands for PATH_CHAR
#define PC_LIT(c) L##c
#else
#define PC_LIT(c) c

#endif

#define as_array_ptr(x)     (::array<fs::path_char_t>*)(x)
#define as_string_ptr(x)    (::string_base<fs::path_char_t>*)(x)
#define empty_fs_string fs::const_fs_string{PC_LIT(""), 0}

// conversion helpers
template<typename T>
struct _converted_string
{
    T *data;
    u64 size;
};

_converted_string<char> _convert_string(const wchar_t *wcstring, u64 wchar_count)
{
    _converted_string<char> ret;
    u64 sz = (wchar_count + 1) * sizeof(char);
    ret.data = (char*)::allocate_memory(sz);

    ::fill_memory(ret.data, 0, sz);

    ret.size = ::wcstombs(ret.data, wcstring, wchar_count * sizeof(wchar_t));

    return ret;
}

_converted_string<wchar_t> _convert_string(const char *cstring, u64 char_count)
{
    _converted_string<wchar_t> ret;
    u64 sz = (char_count + 1) * sizeof(wchar_t);
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
    _converted_string<wchar_t> converted = ::_convert_string(str.c_str, str.size);

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
    _converted_string<char> converted = ::_convert_string(str.c_str, str.size);

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
    assert(pth != nullptr);

#if Windows
    _converted_string converted = ::_convert_string(new_path.c_str, new_path.size);

    assert(converted.data != nullptr);
    assert(converted.size != (size_t)-1);

    ::set_string(as_string_ptr(pth), converted.data, converted.size);

    ::free_memory(converted.data);
#else
    ::set_string(as_string_ptr(pth), new_path);
#endif
}

void fs::set_path(fs::path *pth, const_wstring  new_path)
{
    assert(pth != nullptr);

#if Windows
    ::set_string(as_string_ptr(pth), new_path);
#else
    _converted_string converted = ::_convert_string(new_path.c_str, new_path.size);

    assert(converted.data != nullptr);
    assert(converted.size != (size_t)-1);

    ::set_string(as_string_ptr(pth), converted.data, converted.size);

    ::free_memory(converted.data);
#endif
}

void fs::set_path(fs::path *pth, const fs::path *new_path)
{
    ::set_string(as_string_ptr(pth), to_const_string(new_path));
}

bool fs::operator==(const fs::path &lhs, const fs::path &rhs)
{
    return ::compare_strings(::to_const_string(&lhs), ::to_const_string(&rhs));
}

hash_t fs::hash(const fs::path *pth)
{
    return hash_data(pth->data, pth->size * sizeof(fs::path_char_t));
}

bool fs::get_filesystem_info(const fs::path *pth, fs::filesystem_info *out, bool follow_symlinks, int flags, fs::fs_error *err)
{
    assert(pth != nullptr);
    assert(out != nullptr);

#if Windows
    return false;

#else
    int statx_flags = 0;

    if (!follow_symlinks)
        statx_flags |= AT_SYMLINK_NOFOLLOW;

    if (::statx(AT_FDCWD, pth->data, statx_flags, flags /* mask */, (struct statx*)out) == 0)
        return true;
    
    set_fs_errno_error(err);
#endif

    return false;
}

bool fs::exists(const fs::path *pth, bool follow_symlinks, fs::fs_error *err)
{
    assert(pth != nullptr);

#if Windows
    return false;
#else
    int flags = 0;

    if (!follow_symlinks)
        flags |= AT_SYMLINK_NOFOLLOW;

    if (::faccessat(AT_FDCWD, pth->data, F_OK, flags) == 0)
        return true;

    set_fs_errno_error(err);

#endif

    return false;
}

fs::filesystem_type get_filesystem_type(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows

#else
    return (fs::filesystem_type)(info->stx_mode & S_IFMT);
#endif

    return fs::filesystem_type::Unknown;
}

bool fs::is_file(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    // TODO: implement
    return false;
#else

    return S_ISREG(info->stx_mode);
#endif
}

bool fs::is_pipe(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return false;
#else

    return S_ISFIFO(info->stx_mode);
#endif
}

bool fs::is_block_device(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return false;
#else

    return S_ISBLK(info->stx_mode);
#endif
}

bool fs::is_special_character_file(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return false;
#else

    return S_ISCHR(info->stx_mode);
#endif
}

bool fs::is_socket(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return false;
#else

    return S_ISSOCK(info->stx_mode);
#endif
}

bool fs::is_symlink(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    // TODO: implement
    return false;
#else

    return S_ISLNK(info->stx_mode);
#endif
}

bool fs::is_directory(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    // TODO: implement
    return false;
#else

    return S_ISDIR(info->stx_mode);
#endif
}

bool fs::is_other(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    // TODO: implement
    return false;

#else
    // if only normal flags are set, this is not Other.
    return (info->stx_mode & (~(S_IFMT))) != 0;
#endif
}

#define define_is_fs_type(FUNC)\
bool fs::FUNC(const fs::path *pth, bool follow_symlinks, fs::fs_error *err)\
{\
    assert(pth != nullptr);\
    fs::filesystem_info info;\
\
    if (!fs::get_filesystem_info(pth, &info, follow_symlinks, STATX_TYPE, err))\
        return false;\
\
    return fs::FUNC(&info);\
}

define_is_fs_type(is_file)
define_is_fs_type(is_pipe)
define_is_fs_type(is_block_device)
define_is_fs_type(is_socket)
define_is_fs_type(is_symlink)
define_is_fs_type(is_directory)
define_is_fs_type(is_other)


bool fs::is_absolute(const fs::path *pth, fs::fs_error *err)
{
    assert(pth != nullptr);

#if Windows

#else

    if (pth->size == 0)
        return false;

    // lol
    return pth->data[0] == fs::path_separator;
#endif

    return false;
}

bool fs::is_relative(const fs::path *pth, fs::fs_error *err)
{
    return !fs::is_absolute(pth, err);
}

bool fs::are_equivalent(const fs::path *pth1, const fs::path *pth2, bool follow_symlinks, fs::fs_error *err)
{
    assert(pth1 != nullptr);
    assert(pth2 != nullptr);

    if (::compare_strings(as_string_ptr(pth1), as_string_ptr(pth2)) == 0)
        return true;

    fs::filesystem_info info1;
    fs::filesystem_info info2;
    fs::fs_error err1;
    fs::fs_error err2;

    int info_flags = 0;

#if Windows
#else
    info_flags = STATX_INO;
#endif

    bool ok1 = fs::get_filesystem_info(pth1, &info1, follow_symlinks, info_flags, &err1);

    if (!ok1)
    {
        if (err != nullptr)
            *err = err1;

        return false;
    }

    bool ok2 = fs::get_filesystem_info(pth2, &info2, follow_symlinks, info_flags, &err2);

    if (!ok2)
    {
        if (err != nullptr)
            *err = err2;

        return false;
    }

#if Windows
    // TODO: implement
    return false;
#else
    return (info1.stx_ino == info2.stx_ino)
        && (info1.stx_dev_major == info2.stx_dev_major)
        && (info1.stx_dev_minor == info2.stx_dev_minor);
#endif
}

fs::const_fs_string fs::filename(const fs::path *pth)
{
    assert(pth != nullptr);

    const fs::path_char_t *cstr = pth->data;
    const fs::path_char_t *found;

    found = ::strrchr(cstr, fs::path_separator);

    if (found == nullptr)
        found = cstr;
    else
        found++;

    return fs::const_fs_string{found, (u64)((pth->data + pth->size) - found)};
}

fs::const_fs_string fs::file_extension(const fs::path *pth)
{
    assert(pth != nullptr);

    auto fname = fs::filename(pth);

    // to conform to std::filesystem, simply check if first character
    // of the filename is a '.'.

    if (fname.size == 0)
        return empty_fs_string;

    if ((fname.size == 1 && fname.c_str[0] == PC_LIT('.'))
     || (fname.size == 2 && fname.c_str[0] == PC_LIT('.') && fname.c_str[1] == PC_LIT('.')))
        return empty_fs_string;

    const fs::path_char_t *found = ::strrchr(fname.c_str, '.');

    if (found == nullptr)
        return empty_fs_string;

    return fs::const_fs_string{found, (u64)((pth->data + pth->size) - found)};
}

fs::const_fs_string fs::parent_path(const fs::path *pth)
{
    assert(pth != nullptr);

    const fs::path_char_t *last_sep = ::strrchr(pth->data, fs::path_separator);

    if (last_sep == nullptr)
        return empty_fs_string;

    const fs::path_char_t *first_sep = ::strchr(pth->data, fs::path_separator);

#if Windows
    // TODO: implement check for root
#else
    if (first_sep == last_sep
     && first_sep == pth->data)
        return fs::const_fs_string{pth->data, 1};
#endif

    return fs::const_fs_string{pth->data, (u64)(last_sep - pth->data)};
}

#if 0
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

    if (envr[len - 1] == fs::path_separator)
        append += 1;

    len += strlen(append) + strlen(org) + strlen(app) + 3;

    if (*org)
        snprintf(buf, len, "%s%s%s/%s/", envr, append, org, app);
    else
        snprintf(buf, len, "%s%s%s/", envr, append, app);

    // recursively create the directories
    for (char *ptr = buf + 1; *ptr; ptr++)
    {
        if (*ptr == fs::path_separator)
        {
            *ptr = '\0';

            if (mkdir(buf, 0700) != 0 && errno != EEXIST)
                throw_error("couldn't create directory '%s': '%s'", buf, strerror(errno));

            *ptr = fs::path_separator;
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

fs::const_fs_string to_const_string(const fs::path *path)
{
    assert(path != nullptr);
    return fs::const_fs_string{path->data, path->size};
}
