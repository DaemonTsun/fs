
/* path.hpp
 TODO: write new docs

Note: fs::path has a size member, but because fs uses a lot of 
standard library functions / syscalls (e.g. stat), which do not
check for size, a null character is required to terminate a
fs::path.

 */

#pragma once

#include "shl/hash.hpp"
#include "shl/string.hpp"
#include "shl/enum_flag.hpp"
#include "shl/platform.hpp"

#include "fs/fs_error.hpp"

namespace fs
{
#if Windows

typedef wchar_t path_char_t;
constexpr const path_char_t path_separator = L'\\';

struct filesystem_info {}; // TODO: define
#define FS_QUERY_DEFAULT_FLAGS 0

enum class filesystem_type
{
    Unknown = 0,
    File /* = ??? */,
    Directory,
    Symlink,
    // ??
};


#else
// Linux and others

typedef char path_char_t;
constexpr const path_char_t path_separator = '/';

struct filesystem_timestamp
{
    s64 tv_sec;
    u32 tv_nsec;
    s32 _pad;
};

bool operator< (filesystem_timestamp lhs, filesystem_timestamp rhs);
bool operator> (filesystem_timestamp lhs, filesystem_timestamp rhs);
bool operator<=(filesystem_timestamp lhs, filesystem_timestamp rhs);
bool operator>=(filesystem_timestamp lhs, filesystem_timestamp rhs);

// based on statx
struct filesystem_info {
	u32 stx_mask;
	u32 stx_blksize;
	u64 stx_attributes;
	u32 stx_nlink;
	u32 stx_uid;
	u32 stx_gid;
	u16 stx_mode;
	u16 _pad1;
	u64 stx_ino;
	u64 stx_size;
	u64 stx_blocks;
	u64 stx_attributes_mask;
	filesystem_timestamp stx_atime; // access time
    filesystem_timestamp stx_btime; // creation time
    filesystem_timestamp stx_ctime; // status change time
    filesystem_timestamp stx_mtime; // modification time
	u32 stx_rdev_major;
	u32 stx_rdev_minor;
	u32 stx_dev_major;
	u32 stx_dev_minor;
	u64 _unused[14];
};

// STATX_BASIC_STATS | STATX_BTIME
#define FS_QUERY_DEFAULT_FLAGS 0xfff

enum class filesystem_type : u16
{
    Unknown         = 0,
    File            = 0x8000,
    Directory       = 0x4000,
    Pipe            = 0x1000,
    BlockDevice     = 0x6000,
    CharacterFile   = 0x2000,
    Socket          = 0xc000,
    Symlink         = 0xa000
};

#endif

enum class permission : u16
{
    UserRead        = 0400,
    UserWrite       = 0200,
    UserExecute     = 0100,
    GroupRead       = 0040,
    GroupWrite      = 0020,
    GroupExecute    = 0010,
    OtherRead       = 0004,
    OtherWrite      = 0002,
    OtherExecute    = 0001,

    // Combined values
    None            = 0000,
    User            = 0700,
    Group           = 0070,
    Other           = 0007
};

enum_flag(permission);

// this will either be const_string or const_wstring
typedef const_string_base<fs::path_char_t> const_fs_string;

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

bool get_filesystem_info(const fs::path *pth, fs::filesystem_info *out, bool follow_symlinks = true, int flags = FS_QUERY_DEFAULT_FLAGS, fs::fs_error *err = nullptr);
fs::filesystem_type get_filesystem_type(const fs::filesystem_info *info);

// 0 = doesn't exist, 1 = exists, -1 = error
int exists(const fs::path *pth, bool follow_symlinks = true, fs::fs_error *err = nullptr);

bool is_file(const fs::filesystem_info *info);
bool is_pipe(const fs::filesystem_info *info);
bool is_block_device(const fs::filesystem_info *info);
bool is_special_character_file(const fs::filesystem_info *info);
bool is_socket(const fs::filesystem_info *info);
bool is_symlink(const fs::filesystem_info *info);
bool is_directory(const fs::filesystem_info *info);
bool is_other(const fs::filesystem_info *info);

// TODO: const char, const_string overloads
bool is_file(const fs::path *pth, bool follow_symlinks = true, fs::fs_error *err = nullptr);
bool is_pipe(const fs::path *pth, bool follow_symlinks = true, fs::fs_error *err = nullptr);
bool is_block_device(const fs::path *pth, bool follow_symlinks = true, fs::fs_error *err = nullptr);
bool is_special_character_file(const fs::path *pth, bool follow_symlinks = true, fs::fs_error *err = nullptr);
bool is_socket(const fs::path *pth, bool follow_symlinks = true, fs::fs_error *err = nullptr);
// is_symlink has follow_symlinks = false for obvious reasons
bool is_symlink(const fs::path *pth, bool follow_symlinks = false, fs::fs_error *err = nullptr);
bool is_directory(const fs::path *pth, bool follow_symlinks = true, fs::fs_error *err = nullptr);
bool is_other(const fs::path *pth, bool follow_symlinks = true, fs::fs_error *err = nullptr);

// TODO: const char, const_string overloads
bool is_absolute(const fs::path *pth, fs::fs_error *err = nullptr);
bool is_relative(const fs::path *pth, fs::fs_error *err = nullptr);
// TODO: const char, const_string, filesystem_info overloads
bool are_equivalent(const fs::path *pth1, const fs::path *pth2, bool follow_symlinks = true, fs::fs_error *err = nullptr);

fs::const_fs_string filename(const fs::path *pth);

// this differs from std::filesystem::path::extension:
// filenames that have no stem (only an extension) are treated exactly like that:
// no filename, only extension, and as such file_extension returns the full filename
// for filenames with no stem.
//
// . and .. are treated the same as std::filesystem::path::extension:
// file_extension will return empty strings (not nullptr) in these cases.
fs::const_fs_string file_extension(const fs::path *pth);
fs::const_fs_string parent_path_segment(const fs::path *pth);
fs::const_fs_string root(const fs::path *pth);

void path_segments(const fs::path *pth, array<fs::const_fs_string> *out);

fs::path parent_path(const fs::path *pth);
void parent_path(const fs::path *pth, fs::path *out);
fs::path longest_existing_path(const fs::path *pth);
void longest_existing_path(const fs::path *pth, fs::path *out);

void normalize(fs::path *pth);

fs::path absolute_path(const fs::path *pth, fs::fs_error *err = nullptr);
bool absolute_path(const fs::path *pth, fs::path *out, fs::fs_error *err = nullptr);

fs::path canonical_path(const fs::path *pth, fs::fs_error *err = nullptr);
bool canonical_path(const fs::path *pth, fs::path *out, fs::fs_error *err = nullptr);

fs::path weakly_canonical_path(const fs::path *pth, fs::fs_error *err = nullptr);
bool weakly_canonical_path(const fs::path *pth, fs::path *out, fs::fs_error *err = nullptr);

bool get_current_path(fs::path *out, fs::fs_error *err = nullptr);
bool set_current_path(const fs::path *pth, fs::fs_error *err = nullptr);

// out = pth / seg
void append_path(fs::path *out, const char    *seg);
void append_path(fs::path *out, const wchar_t *seg);
void append_path(fs::path *out, const_string   seg);
void append_path(fs::path *out, const_wstring  seg);
void append_path(fs::path *out, const fs::path *to_append);

// out = pth + seg
// does not add a directory separator
void concat_path(fs::path *out, const char    *seg);
void concat_path(fs::path *out, const wchar_t *seg);
void concat_path(fs::path *out, const_string   seg);
void concat_path(fs::path *out, const_wstring  seg);
void concat_path(fs::path *out, const fs::path *to_concat);

void relative_path(const fs::path *from, const fs::path *to, fs::path *out);

bool touch(const fs::path *pth, fs::permission perms = fs::permission::User, fs::fs_error *err = nullptr);

enum class copy_file_option
{
    None,               // reports an error when destination exists.
    OverwriteExisting,  // (default) overwrites existing destination.
    UpdateExisting,     // overwrites existing destination ONLY if destination is older than source,
                        // but does not report an error in either case.
                        // looks at MODIFICATION date.
    SkipExisting        // skips any existing destination files.
};

bool copy_file(const fs::path *from, const fs::path *to, fs::copy_file_option opt = fs::copy_file_option::OverwriteExisting, fs::fs_error *err = nullptr);
// TODO: add copy_directory
// bool copy_directory(const fs::path *from, const fs::path *to, fs::copy_file_option opt = fs::copy_file_option::OverwriteExisting, fs::fs_error *err = nullptr);

// does not create parents
bool create_directory(const fs::path *pth, fs::permission perms = fs::permission::User, fs::fs_error *err = nullptr);
// creates parents as well
bool create_directories(const fs::path *pth, fs::permission perms = fs::permission::User, fs::fs_error *err = nullptr);
/*
void create_hard_link(const fs::path *target, const fs::path *link);
void create_file_symlink(const fs::path *target, const fs::path *link);
void create_directory_symlink(const fs::path *target, const fs::path *link);
void move(const fs::path *from, const fs::path *to);
bool remove(const fs::path *pth);
bool remove_all(const fs::path *pth);

////////////////////////
// getting special paths
////////////////////////

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

fs::const_fs_string to_const_string(const fs::path *path);
fs::const_fs_string to_const_string(const fs::path &path);
