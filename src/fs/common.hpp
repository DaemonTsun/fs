
/* common.hpp

Filesystem structs, enums, constants, etc.

typedef path_char_t:
    On Linux, char.
    On Windows, wchar_t if UNICODE is set (default), char otherwise.

constant path_char_t path_separator:
    The default path separator used by the platform.

enum fs::filesystem_type:
    The type of filesystem entries, may be obtained with fs::get_filesystem_type
    or in iterations of directories with for_path(item, path) { item->type }.
    The values, and which values are available, depend on the platform, for
    instance on Linux, filesystem_type::Socket names a valid filesystem_type,
    on Windows, filesystem_type::Socket does not exist.

    The following types exist on all platforms:
        - Unknown
        - File
        - Directory
        - Symlink

enum fs::permission:
    Bitmask flags for permissions, based on POSIX permissions.

enum fs::copy_file_option:
    Enum to determine how to copy files:

    None:              Reports an error when destination exists.
    OverwriteExisting: (default) overwrites existing destination.
    UpdateExisting:    Overwrites existing destination ONLY if destination is older than source,
                       but does not report an error in either case.
                       Looks at MODIFICATION time.
    SkipExisting:      Skips any existing destination files.

enum fs::iterate_option:
    Bitmask flags that change the behavior of path iterators. Values:

    None:           Does not follow symlinks and does not stop on errors.
    FollowSymlinks: Follows directory symlinks.
    StopOnError:    Stop on first error.
    Fullpaths:      Yields full paths in item->path. Does consume more memory.
    ChildrenFirst:  When recursively iterating, iterates all children of a
                    directory before iterating the directory.
                    Does nothing when not iterating recursively.
    QueryType:      Whether or not to query type information.
                    Because you get type information on dirent (Linux
                    implementation) anyway, this flag does nothing on
                    Linux.

struct fs::filesystem_info:
    A struct containing specific filesystem information about a given path.
    May be obtained using fs::query_filesystem using flags to query
    different types of information, such as attributes, unique identifier,
    permissions, change and access times, etc..

    On Linux, fs::filesystem_info is identical in structure to a statx struct.
    On Windows, fs::filesystem_info contains different union members for
    different operations.

    fs::query_filesystem signature:

    fs::query_filesystem(Path, *Out, FollowSymlinks, Flags[, err])

    The underlying syscalls or operating functions depend on the Flags,
    although on Linux this will just be statx, but on Windows different
    functions will be called to fill the filesystem_info with data.
    Depending on the flag, different fields of the struct will be written to,
    meaning other fields or union members not written to should not be accessed.

    The following flags are supported, writing to the following fields:

    FS_QUERY_PERMISSIONS:
        Queries the permissions of the given path.
        On Linux, equivalent to STATX_MODE, writing to filesystem_info.stx_mode.
        On Windows, as of 0.8, not implemented. TODO: implement

    FS_QUERY_TYPE:
        Queries the type of the given path.
        On Linux, equivalent to STATX_TYPE, writing to filesystem_info.stx_mode
        (stx_mode contains both access rights and the type, see statx documentation).
        On Windows, does multiple checks on the given path and writes the type
        to filesystem_info.detail.type (fs::filesystem_type).

    FS_QUERY_ID:
        Queries the unique file ID of the given path.
        On Linux, equivalent to STATX_INO, writing to filesystem_info.stx_ino,
        stx_dev_major and stx_dev_minor.
        On Windows, uses GetFileInformationByHandle to set
        filesystem_info.detail.id_info.VolumeSerialNumber and
        filesystem_info.detail.id_info.FileId.

    FS_QUERY_SIZE:
        Queries the size of a file or pipe.
        If the given path does not exist or is not a file or pipe, the size
        will be undefined.

    FS_QUERY_FILE_TIMES:
        Queries the last access time, modification time, change time and
        creation time of the given path.
        On Linux, equivalent to STATX_BTIME | STATX_ATIME | STATX_MTIME | STATX_CTIME,
        Writing to filesystem_info.stx_btime, stx_atime, stx_mtime and stx_ctime.
        On Windows, uses GetFileInformationByHandleEx to query the file times
        and sets filesystem_info.detail.file_times.creation_time, .last_access_time,
        .last_write_time and .change_time.
        
    FS_QUERY_DEFAULT_FLAGS:
        The default flag when omitting specific flags, querying some or all
        information available.
        On Linux, equivalent to STATX_BASIC_STATS | STATX_BTIME, setting
        most fields in the fs::filesystem_info struct.
        On Windows, calls GetFileInformationByHandleEx with FileBasicInfo, setting
        filesystem_info.detail.basic_info.

 */

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

struct windows_file_times
{
    u64 creation_time;
    u64 last_access_time;
    u64 last_write_time;
    u64 change_time;
};

struct filesystem_info
{
    union _info_detail
    {
        FILE_BASIC_INFO         basic_info;     // FileBasicInfo, includes times
        FILE_ATTRIBUTE_TAG_INFO attribute_info; // FileAttributeTagInfo
        FILE_ID_INFO            id_info;        // FileIdInfo
        windows_file_times      file_times;
        int                     permissions;
        fs::filesystem_type     type;
        s64                     size;
    } detail;
};

enum class query_flag
{
    Permissions = 0x1000,
    Type        = 0x1001,
    Id          = 0x1002,
    FileTimes   = 0x1003,
    Size        = 0x1004
};

enum_flag(query_flag);
constexpr const query_flag query_flag_default = (query_flag)FileBasicInfo;

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

enum class query_flag
{
    Type        = 0x001, // STATX_TYPE
    Permissions = 0x002, // STATX_MODE
    Id          = 0x100, // STATX_INO
    Size        = 0x200, // STATX_SIZE
    FileTimes   = 0x8e0, // STATX_BTIME | STATX_ATIME | STATX_MTIME | STATX_CTIME
};

enum_flag(query_flag);
// STATX_BASIC_STATS | STATX_BTIME
constexpr const query_flag query_flag_default = (query_flag)0xfff;

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

enum class copy_file_option
{
    None,               // reports an error when destination exists.
    OverwriteExisting,  // (default) overwrites existing destination.
    UpdateExisting,     // overwrites existing destination ONLY if destination is older than source,
                        // but does not report an error in either case.
                        // looks at MODIFICATION time.
    SkipExisting        // skips any existing destination files.
    // TODO: UpdateOnly? might be useful
};

enum class iterate_option : u8
{
    None            = 0x00, // Does not follow symlinks and does not stop on errors.
    FollowSymlinks  = 0x01, // Follows directory symlinks.
    StopOnError     = 0x02, // Stop on first error.
    Fullpaths       = 0x04, // Yields full paths in item->path. does consume more memory.
    ChildrenFirst   = 0x08, // When recursively iterating, iterates all children of a
                            // directory before iterating the directory.
                            // Does nothing when not iterating recursively.
    QueryType       = 0x10, // Whether or not to query type information.
                            // Because you get type information on dirent (Linux
                            // implementation) anyway, this flag does nothing on
                            // Linux.
};

enum_flag(iterate_option);
}

