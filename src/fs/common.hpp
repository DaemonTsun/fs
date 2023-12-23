
#pragma once

#include "shl/string.hpp"
#include "shl/platform.hpp"
#include "shl/enum_flag.hpp"
#include "shl/number_types.hpp"

namespace fs
{
#if Windows

typedef wchar_t path_char_t;
constexpr const path_char_t path_separator = L'\\';

struct filesystem_info {}; // TODO: define
#define FS_QUERY_DEFAULT_FLAGS 0
#define FS_QUERY_TYPE          0

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
#define FS_QUERY_DEFAULT_FLAGS 0xfff
#define FS_QUERY_TYPE          0x001 // STATX_TYPE

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
    Other           = 0007
};

enum_flag(permission);

// this will either be const_string or const_wstring
typedef const_string_base<fs::path_char_t> const_fs_string;
}
