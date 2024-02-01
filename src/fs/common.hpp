
#pragma once

#include "shl/string.hpp"
#include "shl/platform.hpp"
#include "shl/enum_flag.hpp"
#include "shl/number_types.hpp"

#if Windows
#include <windows.h>
#endif

// constants

// These sizes are used as the default when querying paths with e.g. getcwd.
// Path sizes quadruple until reaching PATH_ALLOC_MAX_SIZE
#define PATH_ALLOC_MIN_SIZE 255
#define PATH_ALLOC_MAX_SIZE 65535

// used in the buffer of getdents64 on linux
#define DIRENT_STACK_BUFFER_SIZE 256
#define DIRENT_ALLOC_GROWTH_FACTOR 4
#define DIRENT_ALLOC_MAX_SIZE 16777215

namespace fs
{
#if Windows

typedef sys_char path_char_t;
constexpr const path_char_t path_separator = SYS_CHAR('\\');

enum class filesystem_type
{
    Unknown         = FILE_TYPE_UNKNOWN,
    File            = FILE_TYPE_DISK, // pretty sure this is right
    Directory       = 6,
    Symlink         = 7,
    Pipe            = FILE_TYPE_PIPE,
    CharacterFile   = FILE_TYPE_CHAR,
};

struct filesystem_info
{
    union _info_detail
    {
        FILE_ATTRIBUTE_TAG_INFO attribute_info; // FileAttributeTagInfo
        FILE_ID_INFO            id_info;        // FileIdInfo
        int                     permissions;
        fs::filesystem_type     type;
    } detail;
};

#define FS_QUERY_DEFAULT_FLAGS  0
#define FS_QUERY_ID             FileIdInfo
#define FS_QUERY_PERMISSIONS    0x1000
#define FS_QUERY_TYPE           0x1001

#else
// Linux and others

typedef sys_char path_char_t;
constexpr const path_char_t path_separator = SYS_CHAR('/');

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
struct filesystem_info
{
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
#define FS_QUERY_DEFAULT_FLAGS  0xfff
#define FS_QUERY_TYPE           0x001 // STATX_TYPE
#define FS_QUERY_PERMISSIONS    0x002 // STATX_MODE
#define FS_QUERY_ID             0x100 // STATX_INO

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

#endif // Linux end

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
    Other           = 0007,
    All             = 0777
};

enum_flag(permission);

// this will either be const_string or const_wstring
typedef const_string_base<fs::path_char_t> const_fs_string;

enum class iterate_option : u8
{
    None            = 0b0000, // does not follow symlinks and does not stop on errors
    FollowSymlinks  = 0b0001, // follows directory symlinks
    StopOnError     = 0b0010, // stop on first error
    Fullpaths       = 0b0100, // yields full paths in item->path. does consume more memory.
    ChildrenFirst   = 0b1000  // when recursively iterating, iterates all children of a
                              // directory before iterating the directory.
                              // does nothing when not iterating recursively.
};

enum_flag(iterate_option);
}
