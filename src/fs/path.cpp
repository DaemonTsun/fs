
// v1.1

#include <assert.h>

#if Windows
#include <windows.h>
#include <direct.h> // _wgetcwd
#else
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#endif

#include <stdlib.h>

#include "shl/memory.hpp"
#include "shl/defer.hpp"
#include "shl/error.hpp"

#include "fs/path.hpp"


// constants

// These sizes are used as the default when querying paths with e.g. getcwd.
// Path sizes quadruple until reaching PATH_ALLOC_MAX_SIZE
#define PATH_ALLOC_MIN_SIZE 255
#define PATH_ALLOC_MAX_SIZE 65535





#if Windows
// PC stands for PATH_CHAR
#define PC_LIT(c) L##c

#define platform_getcwd ::_wgetcwd
#define platform_chdir  ::_chdir
#else
#define PC_LIT(c) c

#define platform_getcwd ::getcwd
#define platform_chdir  ::chdir

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

bool fs::operator< (fs::filesystem_timestamp lhs, fs::filesystem_timestamp rhs) 
{
    if (lhs.tv_sec < rhs.tv_sec)
        return true;

    if (lhs.tv_sec > rhs.tv_sec)
        return false;

    return lhs.tv_nsec < rhs.tv_nsec;
}

bool fs::operator> (fs::filesystem_timestamp lhs, fs::filesystem_timestamp rhs) { return rhs < lhs; }
bool fs::operator<=(fs::filesystem_timestamp lhs, fs::filesystem_timestamp rhs) { return !(rhs < lhs); }
bool fs::operator>=(fs::filesystem_timestamp lhs, fs::filesystem_timestamp rhs) { return !(lhs < rhs); }

#endif

#define PC_DOT PC_LIT('.')
#define PC_NUL PC_LIT('\0')

#define as_array_ptr(x)     (::array<fs::path_char_t>*)(x)
#define as_string_ptr(x)    (::string_base<fs::path_char_t>*)(x)
#define empty_fs_string     fs::const_fs_string{PC_LIT(""), 0}

// conversion helpers
template<typename T>
struct _converted_string
{
    T *data;
    u64 size;
};

template<typename C>
inline const_string_base<C> to_const_string(_converted_string<C> str)
{
    return const_string_base<C>{str.data, str.size};
}

typedef _converted_string<fs::path_char_t> platform_converted_string;

inline fs::path _converted_string_to_path(platform_converted_string str)
{
    assert(str.data != nullptr);
    assert(str.size != (size_t)-1);
    return fs::path{.data = str.data, .size = str.size, .reserved_size = str.size};
}

_converted_string<char> _convert_string(const wchar_t *wcstring, u64 wchar_count)
{
    _converted_string<char> ret;
    u64 sz = (wchar_count + 1) * sizeof(char);
    ret.data = (char*)::allocate_memory(sz);

    ::fill_memory(ret.data, 0, sz);

    ret.size = ::wcstombs(ret.data, wcstring, wchar_count * sizeof(wchar_t));

    return ret;
}

_converted_string<char> _convert_string(const wchar_t *cstring)
{
    return _convert_string(cstring, ::string_length(cstring));
}

_converted_string<char> _convert_string(const_wstring cstring)
{
    return _convert_string(cstring.c_str, cstring.size);
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

_converted_string<wchar_t> _convert_string(const char *cstring)
{
    return _convert_string(cstring, ::string_length(cstring));
}

_converted_string<wchar_t> _convert_string(const_string cstring)
{
    return _convert_string(cstring.c_str, cstring.size);
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
    platform_converted_string converted = ::_convert_string(str.c_str, str.size);

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
    platform_converted_string converted = ::_convert_string(str.c_str, str.size);

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
    platform_converted_string converted = ::_convert_string(new_path.c_str, new_path.size);

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
    platform_converted_string converted = ::_convert_string(new_path.c_str, new_path.size);

    assert(converted.data != nullptr);
    assert(converted.size != (size_t)-1);

    ::set_string(as_string_ptr(pth), converted.data, converted.size);

    ::free_memory(converted.data);
#endif
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

int fs::exists(const fs::path *pth, bool follow_symlinks, fs::fs_error *err)
{
    assert(pth != nullptr);

#if Windows
    // TODO: implement
    return -1;
#else
    int flags = 0;

    if (!follow_symlinks)
        flags |= AT_SYMLINK_NOFOLLOW;

    if (::faccessat(AT_FDCWD, pth->data, F_OK, flags) == 0)
        return 1;

    if (errno == ENOENT)
        return 0;

    set_fs_errno_error(err);

#endif

    return -1;
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

    // for relative or invalid paths, root is empty string
    auto rt = fs::root(pth);

    return rt.size > 0;
}

bool fs::is_relative(const fs::path *pth, fs::fs_error *err)
{
    return !fs::is_absolute(pth, err);
}

bool fs::are_equivalent(const fs::path *pth1, const fs::path *pth2, bool follow_symlinks, fs::fs_error *err)
{
    assert(pth1 != nullptr);
    assert(pth2 != nullptr);

    if (pth1 == pth2)
        return true;

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

    if (_is_dot_filename(fname) || _is_dot_dot_filename(fname))
        return empty_fs_string;

    const fs::path_char_t *found = ::strrchr(fname.c_str, '.');

    if (found == nullptr)
        return empty_fs_string;

    return fs::const_fs_string{found, (u64)((pth->data + pth->size) - found)};
}

fs::const_fs_string fs::parent_path_segment(const fs::path *pth)
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

fs::const_fs_string fs::root(const fs::path *pth)
{
    assert(pth != nullptr);

#if Windows
    // TODO: implement, check for unc, check for long unc (\\?\..., \\?\UNC\...)

    return empty_fs_string;
#else
    if (pth->size == 0 || pth->data[0] != fs::path_separator)
        return empty_fs_string;

    return fs::const_fs_string{pth->data, 1};
#endif
}

void fs::path_segments(const fs::path *pth, array<fs::const_fs_string> *out)
{
    assert(pth != nullptr);
    assert(out != nullptr);

    ::clear(out);

    auto seg = fs::root(pth);

    if (seg.size > 0)
        ::add_at_end(out, seg);

    u64 start = seg.size;
    u64 i = start;

    while (i < pth->size)
    {
        if (pth->data[i] != fs::path_separator)
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

        seg.c_str = pth->data + start;
        seg.size = i - start;

        ::add_at_end(out, seg);

        i += 1;
        start = i;
    }

    if (start == i)
        return;

    seg.c_str = pth->data + start;
    seg.size = i - start;

    ::add_at_end(out, seg);
}

fs::path fs::parent_path(const fs::path *pth)
{
    assert(pth != nullptr);

    fs::path ret{};
    fs::parent_path(pth, &ret);
    return ret;
}

void fs::parent_path(const fs::path *pth, fs::path *out)
{
    assert(pth != nullptr);

    fs::const_fs_string parent = fs::parent_path_segment(pth);

    ::set_string(as_string_ptr(out), parent);
}

fs::path fs::longest_existing_path(const fs::path *pth)
{
    assert(pth != nullptr);

    fs::path existing_slice{};
    fs::longest_existing_path(pth, &existing_slice);
    return existing_slice;
}

void fs::longest_existing_path(const fs::path *pth, fs::path *out)
{
    assert(pth != nullptr);

    fs::set_path(out, pth);
    fs::normalize(out);

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

fs::path fs::absolute_path(const fs::path *pth, fs::fs_error *err)
{
    assert(pth != nullptr);

    fs::path ret{};
    fs::absolute_path(pth, &ret, err);
    return ret;
}

bool fs::absolute_path(const fs::path *pth, fs::path *out, fs::fs_error *err)
{
    assert(pth != nullptr);
    assert(out != nullptr);
    assert(pth != out);

    if (!fs::is_absolute(pth))
    {
        if (!fs::get_current_path(out, err))
            return false;

        fs::append_path(out, to_const_string(pth));
    }
    else
        fs::set_path(out, pth);

    return true;
}

fs::path fs::canonical_path(const fs::path *pth, fs::fs_error *err)
{
    assert(pth != nullptr);

    fs::path ret{};
    fs::canonical_path(pth, &ret, err);
    return ret;
}

bool fs::canonical_path(const fs::path *pth, fs::path *out, fs::fs_error *err)
{
    assert(pth != nullptr);
    assert(out != nullptr);
    assert(pth != out);

#if Windows
    // TODO: PathCchCanonicalizeEx or, if not supported, _fullpath

    return false;
#else

    // TODO: replace with a real realpath, not a fake one.
    // Problems with ::realpath: allocates its own buffer,
    // and the buffer can only be up to PATH_MAX bytes long.
    char *npath = ::realpath(pth->data, nullptr);

    if (npath == nullptr)
    {
        set_fs_errno_error(err);
        return false;
    }

    fs::free(out);
    out->data = npath;
    out->size = ::string_length(npath);
    out->reserved_size = out->size;

    return true;
#endif
}

fs::path fs::weakly_canonical_path(const fs::path *pth, fs::fs_error *err)
{
    assert(pth != nullptr);

    fs::path ret{};
    fs::weakly_canonical_path(pth, &ret, err);
    return ret;
}

bool fs::weakly_canonical_path(const fs::path *pth, fs::path *out, fs::fs_error *err)
{
    assert(pth != nullptr);
    assert(out != nullptr);
    assert(pth != out);

    if (pth->size == 0)
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

bool fs::get_current_path(fs::path *out, fs::fs_error *err)
{
    assert(out != nullptr);

    out->size = 0;
    
    string_base<fs::path_char_t> *outs = as_string_ptr(out);

    if (outs->reserved_size < PATH_ALLOC_MIN_SIZE)
        ::string_reserve(outs, PATH_ALLOC_MIN_SIZE);

    fs::path_char_t *ret = platform_getcwd(out->data, out->reserved_size);

    while (ret == nullptr && (errno == ERANGE))
    {
        if (outs->reserved_size >= PATH_ALLOC_MAX_SIZE)
        {
            set_fs_errno_error(err);
            return false;
        }

        ::string_reserve(outs, outs->reserved_size << 2);
        ret = platform_getcwd(out->data, out->reserved_size);
    }

    if (ret == nullptr)
    {
        set_fs_errno_error(err);
        return false;
    }

    out->size = ::string_length(out->data);

    return true;
}

bool fs::set_current_path(const fs::path *pth, fs::fs_error *err)
{
    assert(pth != nullptr);

    if (platform_chdir(pth->data) == -1)
    {
        set_fs_errno_error(err);
        return false;
    }

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

void fs::append_path(fs::path *out, const_string   seg)
{
    fs::path to_append{}; 

#if Windows
    platform_converted_string converted = ::_convert_string(seg);
    to_append = ::_converted_string_to_path(converted);

    fs::append_path(out, &to_append);

    ::free_memory(converted.data);
#else
    to_append.data = (fs::path_char_t*)seg.c_str;
    to_append.size = seg.size;
    to_append.reserved_size = seg.size;
    fs::append_path(out, &to_append);
#endif
}

void fs::append_path(fs::path *out, const_wstring  seg)
{
    fs::path to_append{}; 

#if Windows
    to_append.data = (fs::path_char_t*)seg.c_str;
    to_append.size = seg.size;
    to_append.reserved_size = seg.size;
    fs::append_path(out, &to_append);
#else
    platform_converted_string converted = ::_convert_string(seg);
    to_append = ::_converted_string_to_path(converted);

    fs::append_path(out, &to_append);

    ::free_memory(converted.data);
#endif
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

void fs::concat_path(fs::path *out, const_string   seg)
{
#if Windows
    platform_converted_string converted = ::_convert_string(seg);

    fs::concat_path(out, ::to_const_string(converted));

    ::free_memory(converted.data);
#else
    ::reserve(as_array_ptr(out), out->size + seg.size + 1);
    ::append_string(as_string_ptr(out), seg);
#endif
}

void fs::concat_path(fs::path *out, const_wstring  seg)
{
#if Windows
    ::reserve(as_array_ptr(out), out->size + seg.size + 1);
    ::append_string(as_string_ptr(out), seg);
#else
    platform_converted_string converted = ::_convert_string(seg);

    fs::concat_path(out, ::to_const_string(converted));

    ::free_memory(converted.data);
#endif
}

void fs::concat_path(fs::path *out, const fs::path *to_concat)
{
    assert(out != nullptr);
    assert(to_concat != nullptr);

    fs::concat_path(out, to_const_string(to_concat));
}

void fs::relative_path(const fs::path *from, const fs::path *to, fs::path *out)
{
    assert(from != nullptr);
    assert(to != nullptr);
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
        to_append.size = (to->data - to_append.c_str) + to->size;

        assert(to_append.c_str != nullptr);
        fs::append_path(out, to_append);
    }
}

bool fs::touch(const fs::path *pth, fs::permission perms, fs::fs_error *err)
{
    assert(pth != nullptr);

#if Windows
    // TODO: implement
#else
    int fd = ::open(pth->data, O_CREAT | O_WRONLY, (::mode_t)perms);

    if (fd == -1)
    {
        set_fs_errno_error(err);
        return false;
    }

    defer { ::close(fd); };

    // passing nullptr sets change & mod time to current time
    if (::futimens(fd, nullptr) == -1)
    {
        set_fs_errno_error(err);
        return false;
    }

#endif

    return true;
}

bool fs::copy_file(const fs::path *from, const fs::path *to, fs::copy_file_option opt, fs::fs_error *err)
{
    assert(from != nullptr);
    assert(to != nullptr);

#if Windows
    // TODO: implement
#else
    int from_fd = 0;
    int to_fd = 0;
    struct statx from_info{};
    unsigned int statx_mask = STATX_SIZE | STATX_MODE;
    unsigned int open_from_flags = O_RDONLY;
    unsigned int open_to_flags = O_CREAT | O_WRONLY | O_TRUNC;

    if (opt != fs::copy_file_option::OverwriteExisting)
        open_to_flags |= O_EXCL;

    if (opt == fs::copy_file_option::UpdateExisting)
        statx_mask |= STATX_MTIME;

    from_fd = ::open(from->data, open_from_flags);

    if (from_fd == -1)
    {
        set_fs_errno_error(err);
        return false;
    }

    defer { ::close(from_fd); };

    if (::statx(from_fd, "", AT_EMPTY_PATH, statx_mask, &from_info) != 0)
    {
        set_fs_errno_error(err);
        return false;
    }

    to_fd = ::open(to->data, open_to_flags, from_info.stx_mode);

    if (to_fd == -1)
    {
        if ((opt == fs::copy_file_option::None)
         || (errno != EEXIST))
        {
            set_fs_errno_error(err);
            return false;
        }

        if (opt == fs::copy_file_option::SkipExisting)
            return true;

        // check change time
        int tmp_fd = ::open(to->data, O_RDONLY);
        struct statx tmp_info;

        if (tmp_fd == -1)
        {
            set_fs_errno_error(err);
            return false;
        }

        defer { ::close(tmp_fd); };

        if (::statx(tmp_fd, "", AT_EMPTY_PATH, STATX_MTIME, &tmp_info) != 0)
        {
            set_fs_errno_error(err);
            return false;
        }

        if (tmp_info.stx_mtime >= from_info.stx_mtime)
            return true;

        open_to_flags &= (~O_EXCL);
        to_fd = ::open(to->data, open_to_flags, from_info.stx_mode);

        if (to_fd == -1)
        {
            set_fs_errno_error(err);
            return false;
        }
    }
    
    defer { ::close(to_fd); };

    if (::sendfile(to_fd, from_fd, nullptr, from_info.stx_size) == -1)
    {
        set_fs_errno_error(err);
        return false;
    }
#endif

    return true;
}

// TODO: implement copy_directory

bool fs::create_directory(const fs::path *pth, fs::permission perms, fs::fs_error *err)
{
    assert(pth != nullptr);

#if Windows
    // TODO: implement
    return false;
#else
    if (::mkdir(pth->data, (::mode_t)perms) != -1)
        return true;

    // we have to check for EEXIST because if it already exists and it's a directory
    // then we return true (failed successfully), but if it exists and is not a
    // directory then we yield the correct error and return false.
    int _errcode = errno;

    if (_errcode != EEXIST)
    {
        set_fs_error(err, _errcode, ::strerror(_errcode));
        return false;
    }

    fs::filesystem_info info;

    if (!fs::get_filesystem_info(pth, &info, false, STATX_TYPE, err))
        return false;

    if (!S_ISDIR(info.stx_mode))
    {
        set_fs_error(err, _errcode, ::strerror(_errcode));
        return false;
    }
#endif

    return true;
}

bool fs::create_directories(const fs::path *pth, fs::permission perms, fs::fs_error *err)
{
    assert(pth != nullptr);

    return false;
}

#if 0
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

fs::const_fs_string to_const_string(const fs::path &path)
{
    assert(path != nullptr);
    return fs::const_fs_string{path.data, path.size};
}
