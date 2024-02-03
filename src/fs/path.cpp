
// v1.1

#include <assert.h>
#include "shl/print.hpp"

#include "shl/string.hpp"
#include "shl/platform.hpp"

#if Windows
#include <windows.h>
#include <direct.h>
#else
// ---------- LINUX ----------
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h> // syscall and faccessat
#include <string.h> // strerror
#include <errno.h>
#include <fcntl.h>

// some syscalls
// from sys/sendfile.h
int _sendfile(int out_fd, int in_fd, off_t *offset, size_t count)
{
    return ::syscall(SYS_sendfile, out_fd, in_fd, offset, count);
}

// from stdio.h
int _rename(const char *oldpath, const char *newpath)
{
    return ::syscall(SYS_rename, oldpath, newpath);
}
#endif

#include <stdlib.h>

#include "shl/memory.hpp"
#include "shl/defer.hpp"
#include "shl/error.hpp"

#include "fs/path.hpp"

#define empty_fs_string     fs::const_fs_string{SYS_CHAR(""), 0}

#if Windows
// PC stands for PATH_CHAR
#define PC_LIT(c) SYS_CHAR(c)

bool _get_windows_handle_from_path(fs::const_fs_string pth, bool follow_symlinks, io_handle *out, error *err = nullptr)
{
    int flags = FILE_FLAG_BACKUP_SEMANTICS; // for directories

    if (!follow_symlinks)
        flags |= FILE_FLAG_OPEN_REPARSE_POINT; // for symlinks

    io_handle ret = INVALID_HANDLE_VALUE;

    ret = CreateFile(pth.c_str,
                     0,
                     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                     nullptr,
                     OPEN_EXISTING,
                     flags,
                     nullptr);

    /*
    if (ret != INVALID_HANDLE_VALUE)
        break;

    if (GetLastError() != ERROR_PIPE_BUSY)
        break;

    if (!WaitNamedPipe(pth.c_str, 10000))
        break;
    */

    if (ret == INVALID_HANDLE_VALUE || ret == INVALID_IO_HANDLE)
    {
        set_GetLastError_error(err);
        return false;
    }

    *out = ret;

    return true;
}

#define GetWindowsHandleFromPath(Name, Pth, FollowSymlinks, Err)\
    io_handle Name;\
    if (!_get_windows_handle_from_path(Pth, FollowSymlinks, &Name, Err))\
        return false;

bool CloseWindowsPathHandle(io_handle h, error *err = nullptr)
{
    if (!CloseHandle(h))
    {
        set_GetLastError_error(err);
        return false;
    }

    return true;
}

// Windows supports both / and \, but \ is preferred.
inline bool _is_directory_separator(sys_char c)
{
    return (c == SYS_CHAR('/')) || (c == SYS_CHAR('\\'));
}

inline bool _is_special_unc(fs::const_fs_string s)
{
    return ((compare_strings(s, SYS_CHAR(".")) == 0)
         || (compare_strings(s, SYS_CHAR("?")) == 0));
}

inline bool _parse_drive_letter(const sys_char *c, s64 size, s64 start, s64 *end)
{
    // need at least <letter><colon>
    if (size - start < 2)
        return false;

    if (is_alpha(c[start]) && (c[start+1] == SYS_CHAR(':')))
    {
        start += 2;

        if ((start < size) && _is_directory_separator(c[start]))
            start += 1;
    }
    else
        return false;

    *end = start;
    return true;
}

inline bool _parse_unc_segment(const sys_char *c, s64 size, s64 start, fs::const_fs_string *out)
{
    if (size - start < 1)
    {
        *out = empty_fs_string;
        return false;
    }

    s64 i = start;

    while (i < size && !_is_directory_separator(c[i]))
        i++;

    out->c_str = c + start;
    out->size = i - start;

    return true;
}

#else // Linux
#define PC_LIT(c) c

bool operator<(statx_timestamp lhs, statx_timestamp rhs)
{
    if (lhs.tv_sec < rhs.tv_sec)
        return true;

    if (lhs.tv_sec > rhs.tv_sec)
        return false;

    return lhs.tv_nsec < rhs.tv_nsec;
}

bool operator>(statx_timestamp lhs, statx_timestamp rhs)  { return rhs < lhs; }
bool operator<=(statx_timestamp lhs, statx_timestamp rhs) { return !(rhs < lhs); }
bool operator>=(statx_timestamp lhs, statx_timestamp rhs) { return !(lhs < rhs); }

#endif

#define PC_DOT PC_LIT('.')
#define PC_NUL PC_LIT('\0')

#define as_array_ptr(x)     (::array<fs::path_char_t>*)(x)
#define as_string_ptr(x)    (::string_base<fs::path_char_t>*)(x)

// conversion helpers
inline fs::path _converted_string_to_path(fs::platform_converted_string str)
{
    assert(str.data != nullptr);
    assert(str.size != (size_t)-1);
    return fs::path{.data = str.data, .size = str.size, .reserved_size = str.size};
}


inline bool _is_dot_filename(fs::const_fs_string str)
{
    return str.size == 1
        && str.c_str[0] == PC_DOT;
}

inline bool _is_dot_dot_filename(fs::const_fs_string str)
{
    return str.size == 2
        && str.c_str[0] == PC_DOT
        && str.c_str[1] == PC_DOT;
}

fs::const_fs_string to_const_string(const fs::path_char_t *path)
{
    assert(path != nullptr);
    return fs::const_fs_string{path, string_length(path)};
}

fs::const_fs_string to_const_string(const fs::path_char_t *path, u64 size)
{
    assert(path != nullptr);
    return fs::const_fs_string{path, size};
}

fs::const_fs_string to_const_string(const fs::path *path)
{
    assert(path != nullptr);
    return fs::const_fs_string{path->data, path->size};
}

fs::const_fs_string to_const_string(const fs::path &path)
{
    return fs::const_fs_string{path.data, path.size};
}

fs::const_fs_string to_const_string(fs::const_fs_string path)
{
    return path;
}

// path functions

fs::path::operator const fs::path_char_t* () const
{
    return this->data;
}

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

void fs::init(fs::path *path, const char    *str, u64 size)
{
    fs::init(path, ::to_const_string(str, size));
}

void fs::init(fs::path *path, const wchar_t *str)
{
    fs::init(path, ::to_const_string(str));
}

void fs::init(fs::path *path, const wchar_t *str, u64 size)
{
    fs::init(path, ::to_const_string(str, size));
}

void _path_init(fs::path *path, const fs::path_char_t *str, u64 size)
{
    ::init(as_string_ptr(path), str, size);
}

template<typename C>
void _init(fs::path *path, const_string_base<C> str)
{
    assert(path != nullptr);
    assert(str.c_str != nullptr);

    if constexpr (needs_conversion(C))
    {
        fs::platform_converted_string converted = fs::convert_string(str.c_str, str.size);

        assert(converted.data != nullptr);
        assert(converted.size != (size_t)-1);

        ::init(as_string_ptr(path), converted.data, converted.size);

        fs::free(&converted);
    }
    else
        ::init(as_string_ptr(path), str.c_str, str.size);
}

void fs::init(fs::path *path, const_string   str)
{
    return _init(path, str);
}

void fs::init(fs::path *path, const_wstring   str)
{
    return _init(path, str);
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

void fs::set_path(fs::path *pth, const char    *new_path, u64 size)
{
    fs::set_path(pth, ::to_const_string(new_path, size));
}

void fs::set_path(fs::path *pth, const wchar_t *new_path)
{
    fs::set_path(pth, ::to_const_string(new_path));
}

void fs::set_path(fs::path *pth, const wchar_t *new_path, u64 size)
{
    fs::set_path(pth, ::to_const_string(new_path, size));
}

template<typename C>
void _set_path(fs::path *pth, const_string_base<C> new_path)
{
    assert(pth != nullptr);

    if constexpr (needs_conversion(C))
    {
        fs::platform_converted_string converted = fs::convert_string(new_path.c_str, new_path.size);

        assert(converted.data != nullptr);
        assert(converted.size != (size_t)-1);

        ::set_string(as_string_ptr(pth), converted.data, converted.size);

        fs::free(&converted);
    }
    else
    {
        ::set_string(as_string_ptr(pth), new_path);
    }
}

void fs::set_path(fs::path *pth, const_string   new_path)
{
    _set_path(pth, new_path);
}

void fs::set_path(fs::path *pth, const_wstring  new_path)
{
    _set_path(pth, new_path);
}

void fs::set_path(fs::path *pth, const fs::path *new_path)
{
    assert(pth != nullptr);
    assert(new_path != nullptr);
    assert(pth != new_path);

    ::set_string(as_string_ptr(pth), to_const_string(new_path));
}

bool fs::operator==(const fs::path &lhs, const fs::path &rhs)
{
    return ::compare_strings(::to_const_string(&lhs), ::to_const_string(&rhs)) == 0;
}

hash_t fs::hash(const fs::path *pth)
{
    return hash_data(pth->data, pth->size * sizeof(fs::path_char_t));
}

bool fs::get_filesystem_info(io_handle h, fs::filesystem_info *out, int flags, error *err)
{
    assert(out != nullptr);

#if Windows
    if (flags >= 0x1000)
    {
        switch (flags)
        {
        case FS_QUERY_TYPE:
            return fs::get_filesystem_type(h, &out->detail.type, err);
        default:
            return false;
        }
    }
    else
    {
        if (!GetFileInformationByHandleEx(h,
                                          (FILE_INFO_BY_HANDLE_CLASS)flags,
                                          &out,
                                          sizeof(fs::filesystem_info)))
        {
            set_GetLastError_error(err);
            return false;
        }
    }

    return true;
#else
    if (::statx(h, "", AT_EMPTY_PATH, flags /* mask */, (struct statx*)out) != 0)
    {
        set_errno_error(err);
        return false;
    }
    
    return true;
#endif
}

bool fs::_get_filesystem_info(fs::const_fs_string pth, fs::filesystem_info *out, bool follow_symlinks, int flags, error *err)
{
    assert(out != nullptr);

#if Windows
    GetWindowsHandleFromPath(h, pth, follow_symlinks, err);

    if (!fs::get_filesystem_info(h, out, flags, err))
    {
        CloseWindowsPathHandle(h);
        return false;
    }

    if (!CloseWindowsPathHandle(h, err))
        return false;

    return true;
#else
    int statx_flags = 0;

    if (!follow_symlinks)
        statx_flags |= AT_SYMLINK_NOFOLLOW;

    if (::statx(AT_FDCWD, pth.c_str, statx_flags, flags /* mask */, (struct statx*)out) == 0)
        return true;
    
    set_errno_error(err);

    return false;
#endif
}

int fs::_exists(fs::const_fs_string pth, bool follow_symlinks, error *err)
{
#if Windows
    io_handle h;
    error _err{};

    bool ok = _get_windows_handle_from_path(pth, follow_symlinks, &h, &_err);

    if (err != nullptr)
        *err = _err;
    
    if (!ok)
    {
        if (_err.error_code == ERROR_FILE_NOT_FOUND)
            return 0;

        return -1;
    }

    if (!CloseWindowsPathHandle(h))
        return -1;

    return 1;
#else
    int flags = 0;

    if (!follow_symlinks)
        flags |= AT_SYMLINK_NOFOLLOW;

    if (::faccessat(AT_FDCWD, pth.c_str, F_OK, flags) == 0)
        return 1;

    if (errno == ENOENT)
        return 0;

    set_errno_error(err);

    return -1;
#endif
}

fs::filesystem_type fs::get_filesystem_type(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return info->detail.type;
#else
    return (fs::filesystem_type)(info->stx_mode & S_IFMT);
#endif
}

bool fs::get_filesystem_type(io_handle h, fs::filesystem_type *out, error *err)
{
    assert(out != nullptr);

#if Windows
    int ptype = GetFileType(h);

    switch (ptype)
    {
    case FILE_TYPE_PIPE:
        *out = fs::filesystem_type::Pipe;
        return true;
    
    case FILE_TYPE_DISK:
    {
        // may still be a directory or a file
        FILE_ATTRIBUTE_TAG_INFO inf;

        if (!GetFileInformationByHandleEx(h, FileAttributeTagInfo, &inf, sizeof(inf)))
        {
            set_GetLastError_error(err);
            return false;
        }

        int attrs = (int)inf.FileAttributes;

        if (attrs & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            *out = fs::filesystem_type::Symlink;
            return true;
        }

        if (attrs & FILE_ATTRIBUTE_DIRECTORY)
        {
            *out = fs::filesystem_type::Directory;
            return true;
        }

        *out = fs::filesystem_type::File;
        return true;
    }

    case FILE_TYPE_CHAR:
        *out = fs::filesystem_type::CharacterFile;
        return true;

    case FILE_TYPE_UNKNOWN:
    {
        set_GetLastError_error(err);
        return false;
    }
    }

    *out = fs::filesystem_type::Unknown;
    return true;
#else
    fs::filesystem_info info;

    if (!fs::get_filesystem_info(h, &info, STATX_TYPE, err))
        return false;

    *out = (fs::filesystem_type)(info.stx_mode & S_IFMT);

    return true;
#endif
}

bool fs::_get_filesystem_type(fs::const_fs_string pth, fs::filesystem_type *out, bool follow_symlinks, error *err)
{
    assert(out != nullptr);

#if Windows
    GetWindowsHandleFromPath(h, pth, follow_symlinks, err);

    if (!fs::get_filesystem_type(h, out, err))
    {
        CloseWindowsPathHandle(h);
        return false;
    }

    if (!CloseWindowsPathHandle(h, err))
        return false;

    return true;
#else
    fs::filesystem_info info;

    if (!fs::_get_filesystem_info(pth, &info, follow_symlinks, STATX_TYPE, err))
        return false;

    *out = (fs::filesystem_type)(info.stx_mode & S_IFMT);

    return true;
#endif
}

fs::permission fs::get_permissions(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    // TODO: implement ACLs at some point
    // turns out not even C++ std::filesystem implements this, and
    // windows ACL api is a huge giant mess with a truly abysmal
    // documentation, so this will be implemented at some other point.
    return fs::permission::None;
#else
    return (fs::permission)(info->stx_mode & ~S_IFMT);
#endif
}

bool fs::get_permissions(io_handle h, fs::permission *out, error *err)
{
#if Windows
    // TODO: implement, see above
    return false;
#else
    fs::filesystem_info info;

    if (!fs::get_filesystem_info(h, &info, STATX_MODE, err))
        return false;

    *out = (fs::permission)(info.stx_mode & ~S_IFMT);

    return true;
#endif
}

bool fs::_get_permissions(fs::const_fs_string pth, fs::permission *out, bool follow_symlinks, error *err)
{
    assert(out != nullptr);

#if Windows
    // TODO: implement, see get_permissions
    return false;
#else
    fs::filesystem_info info;

    if (!fs::_get_filesystem_info(pth, &info, follow_symlinks, STATX_MODE, err))
        return false;

    *out = (fs::permission)(info.stx_mode & ~S_IFMT);

    return true;
#endif
}

bool fs::set_permissions(io_handle h, fs::permission perms, error *err)
{
#if Windows
    // TODO: implement, see get_permissions
    return false;
#else
    if (::fchmod(h, (::mode_t)perms) == -1)
    {
        set_errno_error(err);
        return false;
    }

    return true;
#endif
}

bool fs::_set_permissions(fs::const_fs_string pth, fs::permission perms, bool follow_symlinks, error *err)
{
#if Windows
    // TODO: implement, see get_permissions
    return false;
#else
    int flags = 0;

    if (!follow_symlinks)
        flags |= AT_SYMLINK_NOFOLLOW;

    if (::fchmodat(AT_FDCWD, pth.c_str, (::mode_t)perms, flags) == -1)
    {
        set_errno_error(err);
        return false;
    }

    return true;
#endif
}

bool fs::is_file_info(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return info->detail.type == fs::filesystem_type::File;
#else

    return S_ISREG(info->stx_mode);
#endif
}

bool fs::is_pipe_info(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return info->detail.type == fs::filesystem_type::Pipe;
#else

    return S_ISFIFO(info->stx_mode);
#endif
}

bool fs::is_block_device_info(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return false;
#else

    return S_ISBLK(info->stx_mode);
#endif
}

bool fs::is_special_character_file_info(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return info->detail.type == fs::filesystem_type::CharacterFile;
#else

    return S_ISCHR(info->stx_mode);
#endif
}

bool fs::is_socket_info(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return false;
#else

    return S_ISSOCK(info->stx_mode);
#endif
}

bool fs::is_symlink_info(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return info->detail.type == fs::filesystem_type::Symlink;
#else

    return S_ISLNK(info->stx_mode);
#endif
}

bool fs::is_directory_info(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return info->detail.type == fs::filesystem_type::Directory;
#else

    return S_ISDIR(info->stx_mode);
#endif
}

bool fs::is_other_info(const fs::filesystem_info *info)
{
    assert(info != nullptr);

#if Windows
    return info->detail.type == fs::filesystem_type::Unknown;

#else
    // if only normal flags are set, this is not Other.
    return (info->stx_mode & (~(S_IFMT))) != 0;
#endif
}

bool fs::is_file(io_handle h, error *err)
{
    fs::filesystem_type typ;

    if (!fs::get_filesystem_type(h, &typ, err))
        return false;

    return typ == fs::filesystem_type::File;
}

bool fs::is_pipe(io_handle h, error *err)
{
    fs::filesystem_type typ;

    if (!fs::get_filesystem_type(h, &typ, err))
        return false;

    return typ == fs::filesystem_type::Pipe;
}

bool fs::is_block_device(io_handle h, error *err)
{
#if Windows
    return false;
#else
    fs::filesystem_type typ;

    if (!fs::get_filesystem_type(h, &typ, err))
        return false;

    return typ == fs::filesystem_type::BlockDevice;
#endif
}

bool fs::is_special_character_file(io_handle h, error *err)
{
#if Windows
    return false;
#else
    fs::filesystem_type typ;

    if (!fs::get_filesystem_type(h, &typ, err))
        return false;

    return typ == fs::filesystem_type::CharacterFile;
#endif
}

bool fs::is_socket(io_handle h, error *err)
{
#if Windows
    return false;
#else
    fs::filesystem_type typ;

    if (!fs::get_filesystem_type(h, &typ, err))
        return false;

    return typ == fs::filesystem_type::Socket;
#endif
}

bool fs::is_symlink(io_handle h, error *err)
{
    fs::filesystem_type typ;

    if (!fs::get_filesystem_type(h, &typ, err))
        return false;

    return typ == fs::filesystem_type::Symlink;
}

bool fs::is_directory(io_handle h, error *err)
{
    fs::filesystem_type typ;

    if (!fs::get_filesystem_type(h, &typ, err))
        return false;

    return typ == fs::filesystem_type::Directory;
}

bool fs::is_other(io_handle h, error *err)
{
    fs::filesystem_type typ;

    if (!fs::get_filesystem_type(h, &typ, err))
        return false;

    return typ == fs::filesystem_type::Unknown;
}

bool fs::_is_absolute(fs::const_fs_string pth, error *err)
{
    // for relative or invalid paths, root is empty string
    auto rt = fs::root(pth);

    return rt.size > 0;
}

bool fs::_is_relative(fs::const_fs_string pth, error *err)
{
    return !fs::_is_absolute(pth, err);
}

bool fs::are_equivalent_infos(const fs::filesystem_info *info1, const fs::filesystem_info *info2)
{
    assert(info1 != nullptr);
    assert(info2 != nullptr);

    if (info1 == info2)
        return true;

#if Windows
    // TODO: implement
    return false;
#else
    return (info1->stx_ino       == info2->stx_ino)
        && (info1->stx_dev_major == info2->stx_dev_major)
        && (info1->stx_dev_minor == info2->stx_dev_minor);
#endif
}

bool fs::_are_equivalent(fs::const_fs_string pth1, fs::const_fs_string pth2, bool follow_symlinks, error *err)
{
    if (pth1 == pth2)
        return true;

    fs::filesystem_info info1;
    fs::filesystem_info info2;

    int info_flags = FS_QUERY_ID;

    if (!fs::get_filesystem_info(pth1, &info1, follow_symlinks, info_flags, err))
        return false;

    if (!fs::get_filesystem_info(pth2, &info2, follow_symlinks, info_flags, err))
        return false;

    return fs::are_equivalent_infos(&info1, &info2);
}

fs::const_fs_string fs::filename(fs::const_fs_string pth)
{
    s64 found = ::last_index_of(pth, fs::path_separator);

    if (found == -1)
        found = 0;
    else
        found++;

    pth.c_str += found;
    pth.size -= found;
    return pth;
}

fs::const_fs_string fs::filename(const fs::path *pth)
{
    assert(pth != nullptr);

    return fs::filename(to_const_string(pth));
}

bool fs::_is_dot(fs::const_fs_string pth)
{
    return ::_is_dot_filename(fs::filename(pth));
}

bool fs::_is_dot_dot(fs::const_fs_string pth)
{
    return ::_is_dot_dot_filename(fs::filename(pth));
}

bool fs::_is_dot_or_dot_dot(fs::const_fs_string pth)
{
    auto fname = fs::filename(pth);
    return ::_is_dot_filename(fname) || ::_is_dot_dot_filename(fname);
}

fs::const_fs_string fs::file_extension(fs::const_fs_string pth)
{
    auto fname = fs::filename(pth);

    // to conform to std::filesystem, simply check if first character
    // of the filename is a '.'.

    if (fname.size == 0)
        return empty_fs_string;

    if (_is_dot_filename(fname) || _is_dot_dot_filename(fname))
        return empty_fs_string;

    s64 found = ::last_index_of(fname, PC_LIT('.'));

    if (found == -1)
        return empty_fs_string;

    fname.c_str += found;
    fname.size -= found;
    return fname;
}

fs::const_fs_string fs::file_extension(const fs::path *pth)
{
    assert(pth != nullptr);

    return fs::file_extension(to_const_string(pth));
}

fs::const_fs_string fs::parent_path_segment(fs::const_fs_string pth)
{
    s64 last_sep = ::last_index_of(pth, fs::path_separator);

    if (last_sep == -1)
        return empty_fs_string;

#if Windows
    // TODO: implement check for root
#else
    s64 first_sep = ::index_of(pth, fs::path_separator);

    if (first_sep == last_sep
     && first_sep == 0)
        return fs::const_fs_string{pth.c_str, 1};
#endif

    return fs::const_fs_string{pth.c_str, (u64)last_sep};
}

fs::const_fs_string fs::parent_path_segment(const fs::path *pth)
{
    assert(pth != nullptr);

    return fs::parent_path_segment(to_const_string(pth));
}

fs::const_fs_string fs::root(fs::const_fs_string pth)
{
#if Windows
    // please refer to this link for details
    // https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats

    if (pth.size == 0)
        return empty_fs_string;

    if (pth.size == 1)
    {
        if (_is_directory_separator(pth[0]))
            return pth;
        else
            return empty_fs_string;
    }

    // e.g. /abc/def -> /
    if (_is_directory_separator(pth[0]) && !(_is_directory_separator(pth[1])))
        return fs::const_fs_string{pth.c_str, 1};

    s64 i = 0;

    // e.g. C:/abc -> C:/
    if (_parse_drive_letter(pth.c_str, (s64)pth.size, 0, &i))
        return fs::const_fs_string{pth.c_str, (u64)i};

    // if path is not / or drive letter, path must begin with
    // two path separators (UNC path), otherwise it has no root.
    if (!_is_directory_separator(pth[0]) && !(_is_directory_separator(pth[1])))
        return empty_fs_string;

    fs::const_fs_string seg1{};
    fs::const_fs_string seg2{};

    s64 offset = 2;
    bool ok1 = _parse_unc_segment(pth.c_str, pth.size, offset, &seg1);

    if (!ok1)
        return empty_fs_string;

    offset += seg1.size + 1;
    bool ok2 = ok1 && _parse_unc_segment(pth.c_str, pth.size, offset, &seg2);

#define _include_last_sep(Seg)\
    len = (Seg.c_str - pth.c_str) + Seg.size;\
    if (len < pth.size && _is_directory_separator(pth[len]))\
        len += 1;

    s64 len = 0;

    // single segment UNC path
    if (!ok2)
    {
        _include_last_sep(seg1);
        return fs::const_fs_string{pth.c_str, (u64)len};
    }

    // if its not . or ?, we can safely just return the first two segments
    if (!_is_special_unc(seg1))
    {
        _include_last_sep(seg2);
        return fs::const_fs_string{pth.c_str, (u64)len};
    }

    // not "UNC" UNC path
    if (compare_strings(seg2, SYS_CHAR("UNC")) != 0)
    {
        _include_last_sep(seg2);
        return fs::const_fs_string{pth.c_str, (u64)len};
    }

    offset += seg2.size + 1;
    bool ok3 = _parse_unc_segment(pth.c_str, pth.size, offset, &seg1);

    if (!ok3)
    {
        _include_last_sep(seg2);
        return fs::const_fs_string{pth.c_str, (u64)len};
    }

    offset += seg1.size + 1;
    bool ok4 = _parse_unc_segment(pth.c_str, pth.size, offset, &seg2);

    if (!ok4)
    {
        _include_last_sep(seg1);
        return fs::const_fs_string{pth.c_str, (u64)len};
    }

    _include_last_sep(seg2);
    return fs::const_fs_string{pth.c_str, (u64)len};

#undef _include_last_sep
#else
    if (pth.size == 0 || pth.c_str[0] != fs::path_separator)
        return empty_fs_string;

    return fs::const_fs_string{pth.c_str, 1};
#endif
}

fs::const_fs_string fs::root(const fs::path *pth)
{
    assert(pth != nullptr);

    return fs::root(to_const_string(pth));
}

void fs::replace_filename(fs::path *out, fs::const_fs_string newname)
{
    assert(out != nullptr);

    auto parent_seg = fs::parent_path_segment(out);
    u64 start = 0;
    u64 cutoff = newname.size;

    if (parent_seg == fs::root(out))
    {
        start = parent_seg.size;
        cutoff += start;
    }
    else if (parent_seg.size != 0)
    {
        // +1 for directory separator
        start = parent_seg.size + 1;
        cutoff += start;
    }

    ::reserve(as_array_ptr(out), cutoff + 1);
    ::insert_range(as_array_ptr(out), start, newname.c_str, newname.size);

    out->size = cutoff;
    out->data[cutoff] = PC_NUL;
}

void fs::path_segments(fs::const_fs_string pth, array<fs::const_fs_string> *out)
{
    assert(out != nullptr);

    ::clear(out);

    auto seg = fs::root(pth);

    if (seg.size > 0)
        ::add_at_end(out, seg);

    u64 start = seg.size;
    u64 i = start;

    while (i < pth.size)
    {
        if (pth.c_str[i] != fs::path_separator)
        {
            i += 1;
            continue;
        }

        // no empty segments
        if (start == i)
        {
            i += 1;
            continue;
        }

        seg.c_str = pth.c_str + start;
        seg.size = i - start;

        ::add_at_end(out, seg);

        i += 1;
        start = i;
    }

    if (start == i)
        return;

    seg.c_str = pth.c_str + start;
    seg.size = i - start;

    ::add_at_end(out, seg);
}

void fs::path_segments(const fs::path *pth, array<fs::const_fs_string> *out)
{
    assert(pth != nullptr);
    assert(out != nullptr);

    fs::path_segments(to_const_string(pth), out);
}

fs::path fs::_parent_path(fs::const_fs_string pth)
{
    fs::path ret{};
    fs::_parent_path(pth, &ret);
    return ret;
}

void fs::_parent_path(fs::const_fs_string pth, fs::path *out)
{
    assert(out != nullptr);

    fs::const_fs_string parent = fs::parent_path_segment(pth);

    ::set_string(as_string_ptr(out), parent);
}

fs::path fs::_longest_existing_path(fs::const_fs_string pth)
{
    fs::path existing_slice{};
    fs::_longest_existing_path(pth, &existing_slice);
    return existing_slice;
}

void fs::_longest_existing_path(fs::const_fs_string pth, fs::path *out)
{
    assert(out != nullptr);

    fs::set_path(out, pth);

    u64 i = 0;

    fs::const_fs_string rt = fs::root(out);

    while (out->size > rt.size)
    {
        if (fs::exists(out) == 1)
            break;

        i = out->size - 1;

        while (i > rt.size && out->data[i] != fs::path_separator)
            i--;

        out->size = i;
        out->data[i] = PC_NUL;
    }
}

// The algorithm for normalizing is here
// https://en.cppreference.com/w/cpp/filesystem/path
void fs::normalize(fs::path *pth)
{
    // TODO: account for windows bs

    if (pth == nullptr)
        return;

    if (pth->size == 0)
        return;

    u64 i = 0;
    u64 j = 0;

    // Simplify multiple separators into one
    // e.g. /a/////b -> /a/b
    while (pth->size > 0 && (i < pth->size - 1))
    {
        if (pth->data[i] != fs::path_separator)
        {
            i += 1;
            continue;
        }

        j = i + 1;

        while (pth->data[j] == fs::path_separator)
            j += 1;

        if (j == i + 1)
        {
            i += 1;
            continue;
        }

        assert(j > i + 1);
        ::remove_elements(as_array_ptr(pth), i, (j - i) - 1);

        i += 1;
    }

    pth->data[pth->size] = PC_NUL;

    // remove ./

    i = 0;
    j = 0;

    while (pth->size > 0 && (i < pth->size - 1))
    {
        if (pth->data[i] == PC_DOT
         && pth->data[i + 1] == fs::path_separator)
        {
            if ((i == 0) || (pth->data[i - 1] == fs::path_separator))
                ::remove_elements(as_array_ptr(pth), i, 2);
            else
                i += 1;
        }
        else
            i += 1;
    }

    pth->data[pth->size] = PC_NUL;

    // remove <dir>/..[/]

    i = 0;
    u64 filename_start = 0;

    // 3 to account for "/.."
    while (pth->size > 3 && (i < pth->size - 3))
    {
        if (pth->data[i] == fs::path_separator)
        {
            i += 1;
            filename_start = i;

            while ((i < pth->size - 3)
                && pth->data[i] != fs::path_separator)
                i += 1;

            if (i >= pth->size - 3)
                break;

            if (pth->data[i] == fs::path_separator
             && pth->data[i + 1] == PC_DOT
             && pth->data[i + 2] == PC_DOT
             && (i + 3 >= pth->size || pth->data[i + 3] == fs::path_separator))
            {
                i += 3;

                while (i < pth->size && pth->data[i] == fs::path_separator)
                    i += 1;

                ::remove_elements(as_array_ptr(pth), filename_start, i - filename_start);
                pth->data[pth->size] = PC_NUL;

                // we set i to 0 here because of ../..
                i = 0;
                continue;
            }
        }
        else
            i += 1;
    }

    pth->data[pth->size] = PC_NUL;

    // remove trailing .. after root
    auto rt = fs::root(pth);

    if (rt.size > 0)
    {
        i = rt.size;
        j = 0;

        if ((pth->size >= rt.size + 2) && i < pth->size - 1)
        {
            if (pth->data[i]     == PC_DOT
             && pth->data[i + 1] == PC_DOT)
            {
                j = i;
                i += 2;

                while (i < pth->size)
                {
                    if (pth->data[i] == fs::path_separator)
                    {
                        i += 1;
                        continue;
                    }

                    if (i < pth->size - 1
                     && pth->data[i]     == PC_DOT
                     && pth->data[i + 1] == PC_DOT)
                    {
                        if (i + 2 >= pth->size)
                        {
                            i += 2;
                            continue;
                        }
                        else if (pth->data[i + 2] == fs::path_separator)
                        {
                            i += 2;
                            continue;
                        }
                        else
                            break;
                    }
                }

                assert(i > j);
                assert(i <= pth->size);
                ::remove_elements(as_array_ptr(pth), j, i - j);
            }
        }
    }

    // remove the last /. (not in the rules)
    if (pth->size > rt.size)
    {
        i = pth->size - 1;

        if (i > 0
         && pth->data[i] == PC_DOT
         && pth->data[i - 1] == fs::path_separator)
            ::remove_elements(as_array_ptr(pth), i, 1);

        pth->data[pth->size] = PC_NUL;
    }

    // remove the last / (not in the rules)
    if (pth->size > rt.size)
    {
        i = pth->size - 1;

        if (pth->data[i] == fs::path_separator)
            ::remove_elements(as_array_ptr(pth), i, 1);

        pth->data[pth->size] = PC_NUL;
    }

    pth->data[pth->size] = PC_NUL;

    if (pth->size == 0)
    {
        ::reserve(as_array_ptr(pth), 2);
        pth->data[0] = PC_DOT;
        pth->size = 1;
        pth->data[pth->size] = PC_NUL;
    }
}

fs::path fs::_absolute_path(fs::const_fs_string pth, error *err)
{
    fs::path ret{};
    fs::_absolute_path(pth, &ret, err);
    return ret;
}

bool fs::_absolute_path(fs::const_fs_string pth, fs::path *out, error *err)
{
    assert(out != nullptr);
    assert(pth.c_str != out->data);

    if (!fs::is_absolute(pth))
    {
        if (!fs::get_current_path(out, err))
            return false;

        fs::append_path(out, pth);
    }
    else
        fs::set_path(out, pth);

    return true;
}

fs::path fs::_canonical_path(fs::const_fs_string pth, error *err)
{
    fs::path ret{};
    fs::canonical_path(pth, &ret, err);
    return ret;
}

bool fs::_canonical_path(fs::const_fs_string pth, fs::path *out, error *err)
{
    assert(out != nullptr);
    assert(pth.c_str != out->data);

#if Windows
    // TODO: PathCchCanonicalizeEx or, if not supported, _fullpath

    return false;
#else

    // TODO: replace with a real realpath, not a fake one.
    // Problems with ::realpath: allocates its own buffer,
    // and the buffer can only be up to PATH_MAX bytes long.
    char *npath = ::realpath(pth.c_str, nullptr);

    if (npath == nullptr)
    {
        set_errno_error(err);
        return false;
    }

    fs::free(out);
    out->data = npath;
    out->size = ::string_length(npath);
    out->reserved_size = out->size;

    return true;
#endif
}

fs::path fs::_weakly_canonical_path(fs::const_fs_string pth, error *err)
{
    fs::path ret{};
    fs::weakly_canonical_path(pth, &ret, err);
    return ret;
}

bool fs::_weakly_canonical_path(fs::const_fs_string pth, fs::path *out, error *err)
{
    assert(out != nullptr);
    assert(pth.c_str != out->data);

    if (pth.size == 0)
    {
        fs::set_path(out, pth);
        return true;
    }

    out->size = 0;

    if (!fs::absolute_path(pth, out, err))
        return false;

    fs::normalize(out);

    if (out->size == 0)
        return true;

    auto rt = fs::root(out);

    if (rt.size == 0 || rt.size == out->size)
        return true;

    fs::path existing_slice{};
    fs::longest_existing_path(out, &existing_slice);
    defer { fs::free(&existing_slice); };

    if (existing_slice.size <= rt.size)
        return true;

    // existing slice must exist now
    fs::path old_path{};
    fs::set_path(&old_path, out);
    defer { fs::free(&old_path); };
    
    if (!fs::canonical_path(&existing_slice, out, err))
        return false;

    fs::const_fs_string rest{.c_str = old_path.data + existing_slice.size,
                             .size  = old_path.size - existing_slice.size};

    if (rest.size == 0)
        return true;

    if (rest.c_str[0] == fs::path_separator)
    {
        rest.c_str += 1;
        rest.size  -= 1;
    }

    fs::append_path(out, rest);

    return true;
}

bool fs::_get_symlink_target(fs::const_fs_string pth, fs::path *out, error *err)
{
    assert(out != nullptr);

#if Windows
    // TODO: implement
    return false;
#else
    scratch_buffer<1024> buf{};
    ::init(&buf);
    defer { ::free(&buf); };

    ssize_t retsize = 0;

    // PATH_ALLOC_MAX_SIZE is much bigger than MAX_PATH
    while (buf.size < PATH_ALLOC_MAX_SIZE)
    {
        retsize = ::readlink(pth.c_str, buf.data, buf.size);

        if (retsize < 0)
        {
            set_errno_error(err);
            return false;
        }

        // disgusting
        if (retsize == buf.size)
        {
            ::grow(&buf);
            continue;
        }

        break;
    }

    if (buf.size >= PATH_ALLOC_MAX_SIZE)
    {
        set_error(err, EINVAL, ::strerror(EINVAL));
        return false;
    }

    fs::set_path(out, buf.data, retsize);

    return true;
#endif
}

bool fs::get_current_path(fs::path *out, error *err)
{
    assert(out != nullptr);

    out->size = 0;
    
    string_base<fs::path_char_t> *outs = as_string_ptr(out);

    if (outs->reserved_size < PATH_ALLOC_MIN_SIZE)
        ::string_reserve(outs, PATH_ALLOC_MIN_SIZE);

#if Windows
    int bufsize = GetCurrentDirectory((u32)out->reserved_size, out->data);

    while (true)
    {
        if (bufsize == 0)
        {
            set_GetLastError_error(err);
            return false;
        }

        if (bufsize > out->reserved_size)
        {
            ::string_reserve(outs, bufsize);
            bufsize = GetCurrentDirectory((u32)out->reserved_size, out->data);
            continue;
        }

        break;
    }

    out->size = bufsize;

    return true;
#else
    fs::path_char_t *ret = ::getcwd(out->data, out->reserved_size);

    while (ret == nullptr && (errno == ERANGE))
    {
        if (outs->reserved_size >= PATH_ALLOC_MAX_SIZE)
        {
            set_errno_error(err);
            return false;
        }

        ::string_reserve(outs, outs->reserved_size << 2);
        ret = ::getcwd(out->data, out->reserved_size);
    }

    if (ret == nullptr)
    {
        set_errno_error(err);
        return false;
    }

    out->size = ::string_length(out->data);

    return true;
#endif
}

bool fs::_set_current_path(fs::const_fs_string pth, error *err)
{
#if Windows
    if (!::SetCurrentDirectory(pth.c_str))
    {
        set_GetLastError_error(err);
        return false;
    }
#else
    if (::chdir(pth.c_str) == -1)
    {
        set_errno_error(err);
        return false;
    }
#endif

    return true;
}

void fs::append_path(fs::path *out, const char    *seg)
{
    return fs::append_path(out, ::to_const_string(seg));
}

void fs::append_path(fs::path *out, const wchar_t *seg)
{
    return fs::append_path(out, ::to_const_string(seg));
}

template<typename C>
void _append_string(fs::path *out, const_string_base<C> seg)
{
    fs::path to_append{}; 

    if constexpr (needs_conversion(C))
    {
        fs::platform_converted_string converted = fs::convert_string(seg);
        to_append = ::_converted_string_to_path(converted);

        fs::append_path(out, &to_append);

        fs::free(&converted);
    }
    else
    {
        to_append.data = (fs::path_char_t*)seg.c_str;
        to_append.size = seg.size;
        to_append.reserved_size = seg.size;
        fs::append_path(out, &to_append);
    }
}

void fs::append_path(fs::path *out, const_string   seg)
{
    _append_string(out, seg);
}

void fs::append_path(fs::path *out, const_wstring  seg)
{
    _append_string(out, seg);
}

void fs::append_path(fs::path *out, const fs::path *to_append)
{
    assert(out != nullptr);
    assert(to_append != nullptr);

    if (to_append->size == 0)
        return;

    if (out->size == 0)
    {
        fs::set_path(out, to_append);
        return;
    }

    if (fs::is_absolute(to_append))
    {
        fs::set_path(out, to_append);
        return;
    }

    u64 append_from = 0;
    u64 extra_size_needed = to_append->size + 1;
    bool add_separator = false;

    fs::path_char_t end_of_out = out->data[out->size - 1];
    fs::path_char_t start_of_append = to_append->data[0];

    if (end_of_out == fs::path_separator
     && start_of_append == fs::path_separator)
    {
        // if there's a separator at the end of the out path as well
        // as the front of the path to add, don't add the one from
        // the path to add.
        append_from = 1;
        extra_size_needed -= 1;
    }
    else if (end_of_out != fs::path_separator
          && start_of_append != fs::path_separator)
    {
        // or if neither is a separator, we have to add one...
        add_separator = true;
        extra_size_needed += 1; // can separators be larger than 1 character?
    }
    
    ::reserve(as_array_ptr(out), out->size + extra_size_needed);

    if (add_separator)
    {
        out->data[out->size] = fs::path_separator;
        out->size += 1;
    }

    ::append_string(as_string_ptr(out), fs::const_fs_string{to_append->data + append_from, to_append->size - append_from});
}

void fs::concat_path(fs::path *out, const char    *seg)
{
    return fs::concat_path(out, ::to_const_string(seg));
}

void fs::concat_path(fs::path *out, const wchar_t *seg)
{
    return fs::concat_path(out, ::to_const_string(seg));
}

template<typename C>
void _concat_path(fs::path *out, const_string_base<C> seg)
{
    assert(out != nullptr);

    if constexpr (needs_conversion(C))
    {
        fs::platform_converted_string converted = fs::convert_string(seg);

        _concat_path(out, ::to_const_string(converted));

        fs::free(&converted);
    }
    else
    {
        ::reserve(as_array_ptr(out), out->size + seg.size + 1);
        ::append_string(as_string_ptr(out), seg);
    }
}

void fs::concat_path(fs::path *out, const_string   seg)
{
    _concat_path(out, seg);
}

void fs::concat_path(fs::path *out, const_wstring  seg)
{
    _concat_path(out, seg);
}

void fs::concat_path(fs::path *out, const fs::path *to_concat)
{
    assert(out != nullptr);
    assert(to_concat != nullptr);

    fs::concat_path(out, to_const_string(to_concat));
}

void fs::_relative_path(fs::const_fs_string from, fs::const_fs_string to, fs::path *out)
{
    assert(out != nullptr);

    fs::set_path(out, PC_LIT(""));

    auto rt_from = fs::root(from);
    auto rt_to = fs::root(to);

    if (rt_from != rt_to)
        return;

    array<fs::const_fs_string> from_segs{};
    array<fs::const_fs_string> to_segs{};

    defer { ::free(&from_segs); };
    defer { ::free(&to_segs); };

    fs::path_segments(from, &from_segs);
    fs::path_segments(to,   &to_segs);

    u64 i = 0;

    while (i < from_segs.size && i < to_segs.size)
    {
        if (from_segs[i] != to_segs[i])
            break;

        i += 1;
    }

    if (i == from_segs.size && i == to_segs.size)
    {
        fs::set_path(out, PC_LIT("."));
        return;
    }

    s64 n = 0;
    u64 j = i;
    
    while (j < from_segs.size)
    {
        if (_is_dot_dot_filename(from_segs[j]))
            n -= 1;
        else if (!_is_dot_filename(from_segs[j]))
            n += 1;

        j += 1;
    }

    if (n < 0)
        return;

    if (n == 0 && i >= to_segs.size)
    {
        fs::set_path(out, PC_LIT("."));
        return;
    }

    while (n > 0)
    {
        fs::append_path(out, PC_LIT(".."));
        n -= 1;
    }

    if (i < to_segs.size)
    {
        fs::const_fs_string to_append = to_segs[i];
        to_append.size = (to.c_str - to_append.c_str) + to.size;

        assert(to_append.c_str != nullptr);
        fs::append_path(out, to_append);
    }
}

bool fs::_touch(fs::const_fs_string pth, fs::permission perms, error *err)
{
#if Windows
    // TODO: implement
#else
    int fd = ::open(pth.c_str, O_CREAT | O_WRONLY, (::mode_t)perms);

    if (fd == -1)
    {
        set_errno_error(err);
        return false;
    }

    defer { ::close(fd); };

    // passing nullptr sets change & mod time to current time
    if (::futimens(fd, nullptr) == -1)
    {
        set_errno_error(err);
        return false;
    }

#endif

    return true;
}

bool fs::_copy_file(fs::const_fs_string from, fs::const_fs_string to, fs::copy_file_option opt, error *err)
{
#if Windows
    // TODO: implement
    return false;
#else
    int from_fd = 0;
    int to_fd = 0;
    fs::filesystem_info from_info{};
    unsigned int statx_mask = STATX_SIZE | STATX_MODE;
    unsigned int open_from_flags = O_RDONLY;
    unsigned int open_to_flags = O_CREAT | O_WRONLY | O_TRUNC;

    if (opt != fs::copy_file_option::OverwriteExisting)
        open_to_flags |= O_EXCL; // O_EXCL to ensure creating, or error when already exists

    if (opt == fs::copy_file_option::UpdateExisting)
        statx_mask |= STATX_MTIME;

    from_fd = ::open(from.c_str, open_from_flags);

    if (from_fd == -1)
    {
        set_errno_error(err);
        return false;
    }

    defer { ::close(from_fd); };

    if (!fs::get_filesystem_info(from_fd, &from_info, statx_mask, err))
        return false;

    to_fd = ::open(to.c_str, open_to_flags, from_info.stx_mode);

    if (to_fd == -1)
    {
        if ((opt == fs::copy_file_option::None)
         || (errno != EEXIST))
        {
            set_errno_error(err);
            return false;
        }

        if (opt == fs::copy_file_option::SkipExisting)
            return true;

        // check change time
        int tmp_fd = ::open(to.c_str, O_RDONLY);
        fs::filesystem_info tmp_info{};

        if (tmp_fd == -1)
        {
            set_errno_error(err);
            return false;
        }

        defer { ::close(tmp_fd); };

        if (!fs::get_filesystem_info(tmp_fd, &tmp_info, STATX_MTIME, err))
            return false;

        if (tmp_info.stx_mtime >= from_info.stx_mtime)
            return true;

        open_to_flags &= (~O_EXCL);
        to_fd = ::open(to.c_str, open_to_flags, from_info.stx_mode);

        if (to_fd == -1)
        {
            set_errno_error(err);
            return false;
        }
    }
    
    defer { ::close(to_fd); };

    if (::_sendfile(to_fd, from_fd, nullptr, from_info.stx_size) == -1)
    {
        set_errno_error(err);
        return false;
    }

    return true;
#endif
}

// only the directory, not children
bool _copy_single_directory(fs::const_fs_string from, fs::const_fs_string to, fs::copy_file_option opt, error *err)
{
    unsigned int flags = 0;

#if Linux
    flags = STATX_MODE | STATX_TYPE;
#endif

    fs::filesystem_info from_info;

    // TODO: symlinks?
    if (!fs::get_filesystem_info(from, &from_info, true, flags, err))
        return false;

    fs::filesystem_type from_type = fs::get_filesystem_type(&from_info);

    if (from_type != fs::filesystem_type::Directory)
    {
        set_error(err, EEXIST, ::strerror(EEXIST));
        return false;
    }

    error _err{};
    bool ok = fs::create_directory(to, fs::get_permissions(&from_info), &_err);

    if (err != nullptr)
        *err = _err;

    if (!ok)
        return false;

    // create_directory returns true even if target exists as directory,
    // so we check again.
    if (_err.error_code == EEXIST && opt == fs::copy_file_option::None)
        return false;

    return true;
}

template<bool CheckDepth>
bool _copy_directory(fs::const_fs_string from, fs::const_fs_string to, int max_depth, fs::copy_file_option opt, error *err)
{
    if (!::_copy_single_directory(from, to, opt, err))
        return false;

    fs::path from_abs = fs::_canonical_path(from, nullptr);
    fs::path path_it{};
    fs::const_fs_string attachment;
    fs::_relative_path(fs::parent_path_segment(&from_abs), to, &path_it);

    fs::free(&from_abs);

    u64 base_length = path_it.size;
    u64 attachment_length = from.size + 1;

    defer { fs::free(&path_it); };

    // TODO: iterate options in parameters?
    for_recursive_path(item, from, fs::iterate_option::StopOnError, err)
    {
        path_it.size = base_length;
        path_it.data[path_it.size] = PC_NUL;
        attachment = item->path;
        assert(attachment.size >= attachment_length);
        attachment.size -= attachment_length;
        attachment.c_str += attachment_length;
        fs::append_path(&path_it, attachment);

        if constexpr (CheckDepth)
        {
            if (item->depth == max_depth)
                item->recurse = false;
        }

        if (item->type == fs::filesystem_type::Directory)
        {
            if (!::_copy_single_directory(item->path, ::to_const_string(path_it), opt, err))
                return false;
        }
        else
        {
            if (!fs::_copy_file(item->path, ::to_const_string(path_it), opt, err))
                return false;
        }
    }

    return true;
}

bool fs::_copy_directory(fs::const_fs_string from, fs::const_fs_string to, int max_depth, fs::copy_file_option opt, error *err)
{
    if (max_depth < 0)
        return ::_copy_directory<false>(from, to, max_depth, opt, err);
    else
        return ::_copy_directory<true>(from, to, max_depth, opt, err);
}

bool fs::_copy(fs::const_fs_string from, fs::const_fs_string to, int max_depth, fs::copy_file_option opt, error *err)
{
    fs::filesystem_type from_type;

    if (!fs::get_filesystem_type(from, &from_type, true, err))
        return false;

    if (from_type == fs::filesystem_type::Directory)
        return fs::_copy_directory(from, to, max_depth, opt, err);
    else
        return fs::_copy_file(from, to, opt, err);
}

bool fs::_create_directory(fs::const_fs_string pth, fs::permission perms, error *err)
{
#if Windows
    // TODO: implement
    return false;
#else
    if (::mkdir(pth.c_str, (::mode_t)perms) != -1)
        return true;

    // we have to check for EEXIST because if it already exists and it's a directory
    // then we return true (failed successfully), but if it exists and is not a
    // directory then we yield the correct error and return false.
    int _errcode = errno;
    set_error(err, _errcode, ::strerror(_errcode));

    if (_errcode != EEXIST)
        return false;

    fs::filesystem_info info;

    if (!fs::get_filesystem_info(pth, &info, false, STATX_TYPE, err))
        return false;

    if (!S_ISDIR(info.stx_mode))
        return false;

    return true;
#endif
}

bool fs::_create_directories(fs::const_fs_string pth, fs::permission perms, error *err)
{
    fs::path longest_part{};
    fs::longest_existing_path(pth, &longest_part);
    defer { fs::free(&longest_part); };

    array<fs::const_fs_string> segs{};
    fs::path_segments(pth, &segs);
    defer { ::free(&segs); };

    u64 i = 0;

    // find the first segment after longest_part that doesn't exist
    if (longest_part.size > 0)
    while (i < segs.size)
    {
        u64 offset = (segs[i].c_str - pth.c_str);

        if (offset > longest_part.size)
            break;

        i += 1;
    }

    // we do this to make sure longest_part is a directory
    if (longest_part.size > 0
     && !fs::create_directory(&longest_part, perms, err))
        return false;

    while (i < segs.size)
    {
        fs::append_path(&longest_part, segs[i]);

        if (!fs::create_directory(&longest_part, perms, err))
            return false;

        i += 1;
    }

    return true;
}

bool fs::_create_hard_link(fs::const_fs_string target, fs::const_fs_string link, error *err)
{
#if Windows
    // TODO: implement
#else
    if (::link(target.c_str, link.c_str) == -1)
    {
        set_errno_error(err);
        return false;
    }
#endif

    return true;
}

bool fs::_create_symlink(fs::const_fs_string target, fs::const_fs_string link, error *err)
{
#if Windows
    // TODO: implement
#else
    if (::symlink(target.c_str, link.c_str) == -1)
    {
        set_errno_error(err);
        return false;
    }
#endif

    return true;
}

bool fs::_move(fs::const_fs_string src, fs::const_fs_string dest, error *err)
{
#if Windows
    // TODO: implement
#else
    if (::_rename(src.c_str, dest.c_str) == -1)
    {
        set_errno_error(err);
        return false;
    }
#endif

    return true;
}

bool fs::_remove_file(fs::const_fs_string pth, error *err)
{
#if Windows
    // TODO: implement
#else
    if (::unlink(pth.c_str) == -1)
    {
        set_errno_error(err);
        return false;
    }
#endif

    return true;
}

bool fs::_remove_empty_directory(fs::const_fs_string pth, error *err)
{
#if Windows
    // TODO: implement
#else
    if (::rmdir(pth.c_str) == -1)
    {
        set_errno_error(err);
        return false;
    }
#endif

    return true;
}

bool fs::_remove_directory(fs::const_fs_string pth, error *err)
{
    fs::iterate_option opts = fs::iterate_option::Fullpaths
                            | fs::iterate_option::StopOnError
                            | fs::iterate_option::ChildrenFirst;

    for_recursive_path(item, pth, opts, err)
    {
        switch (item->type)
        {
        case fs::filesystem_type::File:
        case fs::filesystem_type::Symlink:
        case fs::filesystem_type::Pipe:
        {
            if (!fs::remove_file(item->path, err))
                return false;

            break;
        }
        case fs::filesystem_type::Directory:
        {
            if (!fs::remove_empty_directory(item->path, err))
                return false;

            break;
        }
        case fs::filesystem_type::Unknown:
        case fs::filesystem_type::CharacterFile:
        default:
            return false;
        }
    }

    if (!fs::remove_empty_directory(pth, err))
        return false;

    if (err == nullptr)
        return true;

    return err->error_code == 0;
}

bool fs::_remove(fs::const_fs_string pth, error *err)
{
    fs::filesystem_type fstype;

    if (!fs::_get_filesystem_type(pth, &fstype, false, err))
    {
        if (errno == ENOENT)
            return true; // don't care about nonexisting things

        return false;
    }

    switch (fstype)
    {
    case fs::filesystem_type::Directory:
        return fs::remove_directory(pth, err);
    case fs::filesystem_type::Unknown:
    case fs::filesystem_type::File:
    case fs::filesystem_type::Symlink:
    case fs::filesystem_type::Pipe:
    case fs::filesystem_type::CharacterFile:
    default:
        return fs::remove_file(pth, err);
    }
}

s64 fs::_get_children(fs::const_fs_string pth, array<fs::path> *children, fs::iterate_option opts, error *err)
{
    assert(children != nullptr);

    s64 count = 0;
    error _err{};

    for_path(child, pth, opts, &_err)
    {
        fs::path *cp = ::add_at_end(children);
        fs::init(cp);
        fs::set_path(cp, child->path);
        count += 1;
    }

    if (_err.error_code != 0)
    {
        if (err != nullptr)
            *err = _err;

        count = -1;
    }

    return count;
}

s64 fs::_get_all_descendants(fs::const_fs_string pth, array<fs::path> *descendants, fs::iterate_option opts, error *err)
{
    assert(descendants != nullptr);

    s64 count = 0;
    error _err{};

    for_recursive_path(desc, pth, opts, &_err)
    {
        fs::path *cp = ::add_at_end(descendants);
        fs::init(cp);
        fs::set_path(cp, desc->path);
        count += 1;
    }

    if (_err.error_code != 0)
    {
        if (err != nullptr)
            *err = _err;

        count = -1;
    }

    return count;
}

s64 fs::_get_children_count(fs::const_fs_string pth, fs::iterate_option opts, error *err)
{
    s64 count = 0;
    error _err{};

    for_path(child, pth, opts, &_err)
        count += 1;

    if (_err.error_code != 0)
    {
        if (err != nullptr)
            *err = _err;

        count = -1;
    }

    return count;
}

s64 fs::_get_descendant_count(fs::const_fs_string pth, fs::iterate_option opts, error *err)
{
    s64 count = 0;
    error _err{};

    for_recursive_path(child, pth, opts, &_err)
        count += 1;

    if (_err.error_code != 0)
    {
        if (err != nullptr)
            *err = _err;

        count = -1;
    }

    return count;
}

bool fs::get_executable_path(fs::path *out, error *err)
{
    assert(out != nullptr);
#if Linux
    return fs::get_symlink_target("/proc/self/exe"_cs, out, err);

#elif Windows
    sys_char pth[MAX_PATH] = {0};

    if (GetModuleFileName(NULL, pth, MAX_PATH) == 0)
    {
        set_GetLastError_error(err);
        return false;
    }

    fs::set_path(out, pth);
    return true;
#else
    #error "unsupported"
#endif
}

bool fs::get_executable_directory_path(fs::path *out, error *err)
{
    assert(out != nullptr);

    if (!fs::get_executable_path(out, err))
        return false;

    auto seg = fs::parent_path_segment(out);
    out->size = seg.size;
    return true;
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
bool fs::get_preference_path(fs::path *out, const char *app, const char *org, error *err)
{
    // THIS IS ALTERED.
#if Linux
    out->size = 0;

    const char *envr = ::getenv("XDG_DATA_HOME");
    const char *append;

    if (app == nullptr)
        app = "";

    if (org == nullptr)
        org = "";

    if (envr == nullptr)
    {
        // You end up with "$HOME/.local/share/Name"
        envr = getenv("HOME");

        if (envr == nullptr)
        {
            set_error(err, 0, "neither XDG_DATA_HOME nor HOME environment variables are defined");
            return false;
        }

        append = "/.local/share";
    }
    else
        append = "/";

    u64 len = ::string_length(envr);

    assert(len > 0);
    if (envr[len - 1] == fs::path_separator)
        append += 1;

    len += string_length(append) + string_length(org) + string_length(app) + 3;

    fs::concat_path(out, envr);
    fs::concat_path(out, append);

    if (*org != '\0')
        fs::append_path(out, org);

    fs::append_path(out, app);

    if (!fs::create_directories(out, fs::permission::User, err))
        return false;

    return true;
#elif Windows
    // TODO: implement below
    return false;
#if 0
    char *retval = nullptr;

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
}

bool fs::get_temporary_path(fs::path *out, error *err)
{
    assert(out != nullptr);

#if Linux
    const char *envr = ::getenv("TMPDIR");
    if (envr == nullptr) envr = ::getenv("TMP");
    if (envr == nullptr) envr = ::getenv("TEMP");
    if (envr == nullptr) envr = ::getenv("TEMPDIR");

    if (envr == nullptr)
        fs::set_path(out, "/tmp");
    else
        fs::set_path(out, envr);

    return true;
#else
    // TODO: implement
#endif
    return false;
}

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
