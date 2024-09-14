
#include <t1/t1.hpp>
#include "shl/platform.hpp"
#include "shl/environment.hpp"

#if Linux
#include <sys/stat.h> // umask
#include <errno.h>

#define _ignore_return_value2(Name) [[maybe_unused]] auto _##Name = 
#define _ignore_return_value(Name) _ignore_return_value2(Name)
#define ignore_return_value _ignore_return_value(__LINE__) 
#endif

#include "shl/string.hpp"
#include "shl/file_stream.hpp"
#include "shl/pipe.hpp"
#include "shl/time.hpp" // for sleep
#include "shl/print.hpp"
#include "shl/sort.hpp"
#include "fs/path.hpp"

int path_comparer(const fs::path *a, const fs::path *b)
{
    return string_compare(to_const_string(a), to_const_string(b));
}

#define assert_equal_str(STR1, STR2)\
    assert_equal(string_compare(to_const_string(STR1), STR2), 0)

#if Windows
#define SANDBOX_DIR             u"C:\\Temp\\sandbox"
#define SANDBOX_TEST_DIR        SANDBOX_DIR u"\\dir"
#define SANDBOX_TEST_FILE       SANDBOX_DIR u"\\file"
#define SANDBOX_TEST_SYMLINK    SANDBOX_DIR u"\\symlink"
#define SANDBOX_TEST_SYMLINK_NO_TARGET   SANDBOX_DIR u"\\symlink2"
#define SANDBOX_TEST_PIPE       u"\\\\.\\Pipe\\_sandboxPipe"

#define SANDBOX_TEST_DIR2       SANDBOX_TEST_DIR  u"\\dir2"
#define SANDBOX_TEST_FILE2      SANDBOX_TEST_DIR2 u"\\file2"
#define SANDBOX_TEST_DIR_NO_PERMISSION  SANDBOX_TEST_DIR2 u"\\dir_noperm"
#define SANDBOX_TEST_FILE_IN_NOPERM_DIR SANDBOX_TEST_DIR_NO_PERMISSION u"\\file_noperm"

#else // linux sandbox dirs

#define SANDBOX_DIR             "/tmp/sandbox"
#define SANDBOX_TEST_DIR        SANDBOX_DIR "/dir"
#define SANDBOX_TEST_FILE       SANDBOX_DIR "/file"
#define SANDBOX_TEST_SYMLINK    SANDBOX_DIR "/symlink"
#define SANDBOX_TEST_SYMLINK_NO_TARGET   SANDBOX_DIR "/symlink2"
#define SANDBOX_TEST_PIPE       SANDBOX_DIR "/pipe"

#define SANDBOX_TEST_DIR2       SANDBOX_TEST_DIR "/dir2"
#define SANDBOX_TEST_FILE2      SANDBOX_TEST_DIR2 "/file2"
#define SANDBOX_TEST_DIR_NO_PERMISSION  SANDBOX_TEST_DIR2 "/dir_noperm"
#define SANDBOX_TEST_FILE_IN_NOPERM_DIR SANDBOX_TEST_DIR_NO_PERMISSION "/file_noperm"
#endif

define_test(path_set_sets_path)
{
    fs::path pth{};
    
    fs::path_set(&pth, "/abc");
    assert_equal_str(pth.data, SYS_CHAR("/abc"));
    fs::path_set(&pth, "/abc/def"); assert_equal_str(pth.data, SYS_CHAR("/abc/def"));
    fs::path_set(&pth, u"/abc///:def"); assert_equal_str(pth.data, SYS_CHAR("/abc///:def"));
    fs::path_set(&pth, u"C:/abc///:def"); assert_equal_str(pth.data, SYS_CHAR("C:/abc///:def"));

    fs::free(&pth);
}

define_test(path_new_creates_path_new)
{
    fs::path pth = fs::path_new("/home/etc");
    
    assert_equal_str(pth, SYS_CHAR("/home/etc"));

    fs::free(&pth);
}

define_test(path_new_creates_path_new_resolves_environment_variables)
{
    ::set_environment_variable(SYS_CHAR("MYPATHVAR"), SYS_CHAR("this is the value"));
    fs::path pth = fs::path_new("$MYPATHVAR/abc");
    
    assert_equal_str(pth, SYS_CHAR("this is the value/abc"));

    fs::free(&pth);
}

define_test(is_fs_type_tests)
{
    fs::path p{};

    // directory
    fs::path_set(&p, SANDBOX_TEST_DIR);
    assert_equal(fs::is_file(&p),         false);
    assert_equal(fs::is_block_device(&p), false);
    assert_equal(fs::is_symlink(&p),      false);
    assert_equal(fs::is_pipe(&p),         false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    true);

    // file
    fs::path_set(&p, SANDBOX_TEST_FILE);
    assert_equal(fs::is_file(&p),         true);
    assert_equal(fs::is_block_device(&p), false);
    assert_equal(fs::is_symlink(&p),      false);
    assert_equal(fs::is_pipe(&p),         false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    false);

    assert_equal(fs::is_file(SANDBOX_TEST_FILE), true);

    // symlink
    fs::path_set(&p, SANDBOX_TEST_SYMLINK);
    assert_equal(fs::is_file(&p),         true);
    assert_equal(fs::is_file(&p, false),  false);
    assert_equal(fs::is_block_device(&p), false);
    assert_equal(fs::is_symlink(&p),      true);
    assert_equal(fs::is_pipe(&p),         false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    false);

    fs::path_set(&p, SANDBOX_TEST_SYMLINK_NO_TARGET);
    assert_equal(fs::is_file(&p),         false);
    assert_equal(fs::is_file(&p, false),  false);
    assert_equal(fs::is_block_device(&p), false);
    assert_equal(fs::is_symlink(&p),      true);
    assert_equal(fs::is_pipe(&p),         false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    false);

    // pipe
    fs::path_set(&p, SANDBOX_TEST_PIPE);
    assert_equal(fs::is_pipe(&p),         true);
    assert_equal(fs::is_file(&p),         false);
    assert_equal(fs::is_block_device(&p), false);
    assert_equal(fs::is_symlink(&p),      false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    false);

    // block device
#if Linux
    fs::path_set(&p, "/dev/sda");
    assert_equal(fs::is_file(&p),         false);
    assert_equal(fs::is_block_device(&p), true);
    assert_equal(fs::is_symlink(&p),      false);
    assert_equal(fs::is_pipe(&p),         false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    false);
#endif

    fs::free(&p);
}

define_test(is_fs_type_tests_on_handles)
{
    file_stream strm;

    init(&strm, SANDBOX_TEST_FILE);

    assert_equal(fs::is_file(strm.handle), true);
    assert_equal(fs::is_symlink(strm.handle), false);
    assert_equal(fs::is_pipe(strm.handle), false);

    free(&strm);

    pipe_t p;
    init(&p);

    assert_equal(fs::is_file(p.read), false);
    assert_equal(fs::is_file(p.write), false);
    assert_equal(fs::is_symlink(p.read), false);
    assert_equal(fs::is_symlink(p.write), false);
    assert_equal(fs::is_pipe(p.read), true);
    assert_equal(fs::is_pipe(p.write), true);

    free(&p);
}

define_test(get_filesystem_type_test)
{
    fs::filesystem_type fstype;
    error err{};

    // directory
    const sys_char *p = nullptr;

    p = SANDBOX_TEST_DIR;
    assert_equal(fs::get_filesystem_type(p, &fstype), true);
    assert_equal(fstype, fs::filesystem_type::Directory);

    p = SANDBOX_TEST_FILE;
    assert_equal(fs::get_filesystem_type(p, &fstype), true);
    assert_equal(fstype, fs::filesystem_type::File);

    p = SANDBOX_TEST_SYMLINK;
    assert_equal(fs::get_filesystem_type(p, &fstype), true);
    assert_equal(fstype, fs::filesystem_type::File);
    assert_equal(fs::get_filesystem_type(p, &fstype, false), true);
    assert_equal(fstype, fs::filesystem_type::Symlink);

    p = SANDBOX_TEST_SYMLINK_NO_TARGET;
    assert_equal(fs::get_filesystem_type(p, &fstype, true, &err), false);

#if Windows
    assert_equal(err.error_code, ERROR_FILE_NOT_FOUND);
#else
    assert_equal(err.error_code, ENOENT);
#endif

    assert_equal(fs::get_filesystem_type(p, &fstype, false, &err), true);
    assert_equal(fstype, fs::filesystem_type::Symlink);

#if Linux
    // pipe behavior on Windows is weird so we skip this test
    p = SANDBOX_TEST_PIPE;
    assert_equal(fs::get_filesystem_type(p, &fstype), true);
    assert_equal(fstype, fs::filesystem_type::Pipe);
#endif

    assert_equal(fs::get_filesystem_type(SANDBOX_DIR "/doesnotexist", &fstype, false, &err), false);

#if Windows
    assert_equal(err.error_code, ERROR_FILE_NOT_FOUND);
#else
    assert_equal(err.error_code, ENOENT);
#endif
}

define_test(get_file_size_test)
{
    s64 size;
    error err{};

    // directory
    const sys_char *p = nullptr;

    p = SANDBOX_TEST_FILE;
    assert_equal(fs::get_file_size(p, &size), true);
    assert_equal(size, 0);

    assert_equal(fs::get_file_size(SANDBOX_DIR "/doesnotexist", &size, false, &err), false);

#if Windows
    assert_equal(err.error_code, ERROR_FILE_NOT_FOUND);
#else
    assert_equal(err.error_code, ENOENT);
#endif
}

#if Linux
define_test(get_permissions_gets_permissions)
{
    fs::permission perms;
    error err{};

    const sys_char *p = nullptr;

    p = SANDBOX_TEST_DIR;
    assert_equal(fs::get_permissions(p, &perms), true);
    assert_equal(perms, fs::permission::All);

    p = SANDBOX_TEST_DIR_NO_PERMISSION;
    assert_equal(fs::get_permissions(p, &perms), true);
    assert_equal(perms, fs::permission::None);

    p = SANDBOX_DIR "/doesnotexist";
    assert_equal(fs::get_permissions(p, &perms, true, &err), false);

#if Windows
    assert_equal(err.error_code, ERROR_FILE_NOT_FOUND);
#else
    assert_equal(err.error_code, ENOENT);
#endif
}

define_test(set_permissions_sets_permissions)
{
    error err{};
    fs::permission perms;

    const char *p = SANDBOX_DIR "/setperm1";

    fs::create_directory(p);

    assert_equal(fs::set_permissions(p, fs::permission::None), true);
    fs::get_permissions(p, &perms);
    assert_equal(perms, fs::permission::None);

    assert_equal(fs::set_permissions(p, fs::permission::User), true);
    fs::get_permissions(p, &perms);
    assert_equal(perms, fs::permission::User);

    p = SANDBOX_DIR "/doesnotexist";
    assert_equal(fs::set_permissions(p, fs::permission::All, true, &err), false);

#if Linux
    assert_equal(err.error_code, ENOENT);
#endif
}
#endif

define_test(exists_returns_true_if_directory_exists)
{
    fs::path p{};
    fs::path_set(&p, SANDBOX_TEST_DIR);

    assert_equal(fs::exists(&p), 1);

    fs::free(&p);
}

define_test(exists_returns_true_if_file_exists)
{
    fs::path p{};
    fs::path_set(&p, SANDBOX_TEST_FILE);

    assert_equal(fs::exists(&p), 1);

    // testing other inputs
    assert_equal(fs::exists(SANDBOX_TEST_FILE), 1);

#if Linux
    assert_equal(fs::exists("/tmp/sandbox/file"), 1);
    assert_equal(fs::exists(u"/tmp/sandbox/file"), 1);
#endif

    fs::free(&p);
}

define_test(exists_checks_if_symlink_exists)
{
    fs::path p{};
    fs::path_set(&p, SANDBOX_TEST_SYMLINK);

    assert_equal(fs::exists(&p), 1);

    fs::path_set(&p, SANDBOX_TEST_SYMLINK_NO_TARGET);
    assert_equal(fs::exists(&p, false), 1);

    fs::free(&p);
}

define_test(exists_checks_if_symlink_target_exists)
{
    fs::path p{};
    fs::path_set(&p, SANDBOX_TEST_SYMLINK_NO_TARGET);
    // target doesnt exist, this checks if symlink exists
    assert_equal(fs::exists(&p, false), 1);

    // this checks if target exists
    assert_equal(fs::exists(&p, true), 0);

    fs::free(&p);
}

define_test(exists_returns_false_when_not_exists)
{
    fs::path p{};
    error err;
    fs::path_set(&p, SANDBOX_TEST_DIR "/abc");

    assert_equal(fs::exists(&p, true, &err), 0);

    fs::free(&p);
}

define_test(exists_yields_error_when_unauthorized)
{
    fs::path p{};
    error err;
#if Windows
    fs::path_set(&p, u"C:\\System Volume Information");
#else
    fs::path_set(&p, "/root/abc");
#endif

    assert_equal(fs::exists(&p, true, &err), -1);

#if Windows
    assert_equal(err.error_code, ERROR_ACCESS_DENIED);
#else
    assert_equal(err.error_code, EACCES);
#endif

    fs::free(&p);
}

define_test(is_absolute_returns_true_if_path_is_absolute)
{
    fs::path p{};

#if Windows
    fs::path_set(&p, uR"=(C:\Windows\notepad.exe)=");
#else
    fs::path_set(&p, "/etc/passwd");
#endif

    assert_equal(fs::is_absolute(&p), true);

    fs::free(&p);
}

define_test(is_absolute_returns_false_if_path_is_not_absolute)
{
    fs::path p{};

#if Windows
    fs::path_set(&p, uR"=(..\notepad.exe)=");
#else
    fs::path_set(&p, "../passwd");
#endif

    assert_equal(fs::is_absolute(&p), false);

    fs::free(&p);
}

define_test(is_relative_returns_true_if_path_is_relative)
{
    fs::path p{};

#if Windows
    fs::path_set(&p, uR"=(..\notepad.exe)=");
#else
    fs::path_set(&p, "../passwd");
#endif

    assert_equal(fs::is_relative(&p), true);

    fs::free(&p);
}

define_test(is_relative_returns_false_if_path_is_not_relative)
{
    fs::path p{};

#if Windows
    fs::path_set(&p, uR"=(C:\Windows\notepad.exe)=");
#else
    fs::path_set(&p, "/etc/passwd");
#endif

    assert_equal(fs::is_relative(&p), false);

    fs::free(&p);
}

define_test(are_equivalent_returns_true_for_same_path)
{
    fs::path p1{};
    fs::path p2{};

#if Windows
    fs::path_set(&p1, uR"=(C:\Windows\notepad.exe)=");
    fs::path_set(&p2, uR"=(C:\Windows\notepad.exe)=");
#else
    fs::path_set(&p1, "/etc/passwd");
    fs::path_set(&p2, "/etc/passwd");
#endif

    assert_equal(fs::are_equivalent(&p1, &p2), true);

    fs::free(&p1);
    fs::free(&p2);
}

define_test(are_equivalent_returns_true_for_equivalent_paths)
{
    fs::path p1{};
    fs::path p2{};

#if Windows
    fs::path_set(&p1, uR"=(C:\Windows\..\Windows\notepad.exe)=");
    fs::path_set(&p2, uR"=(C:\Windows\notepad.exe)=");
#else
    fs::path_set(&p1, "/etc/passwd");
    fs::path_set(&p2, "/etc/../etc/passwd");
#endif

    assert_equal(fs::are_equivalent(&p1, &p2), true);

    fs::free(&p1);
    fs::free(&p2);
}

define_test(are_equivalent_returns_false_for_different_existing_paths)
{
    fs::path p1{};
    fs::path p2{};

#if Windows
    fs::path_set(&p1, uR"=(C:\Windows\notepad.exe)=");
    fs::path_set(&p2, uR"=(C:\Windows\regedit.exe)=");
#else
    fs::path_set(&p1, "/etc/passwd");
    fs::path_set(&p2, "/etc/profile");
#endif

    error err{};

    assert_equal(fs::are_equivalent(&p1, &p2, true, &err), false);
    assert_equal(err.error_code, 0);

    fs::free(&p1);
    fs::free(&p2);
}

define_test(are_equivalent_returns_false_if_only_one_path_doesnt_exist)
{
    fs::path p1{};
    fs::path p2{};

#if Windows
    fs::path_set(&p1, uR"=(C:\Windows\notepad.exe)=");
    fs::path_set(&p2, uR"=(C:\Windows\notepad2.exe)=");
#else
    fs::path_set(&p1, "/etc/passwd");
    fs::path_set(&p2, "/etc/passwd2");
#endif

    error err{};
    assert_equal(fs::are_equivalent(&p1, &p2, true, &err), false);

#if Windows
    assert_equal(err.error_code, ERROR_FILE_NOT_FOUND);
#else
    assert_equal(err.error_code, ENOENT);
#endif

    fs::free(&p1);
    fs::free(&p2);
}

define_test(filename_returns_the_filename)
{
#if Windows
    assert_equal_str(fs::filename(u"/foo/bar.txt"_cs), u"bar.txt");
    assert_equal_str(fs::filename(u"/foo/.bar"_cs), u".bar");
    assert_equal_str(fs::filename(u"/foo/bar/"_cs), u"");
    assert_equal_str(fs::filename(u"/foo/."_cs), u".");
    assert_equal_str(fs::filename(u"/foo/.."_cs), u"..");
    assert_equal_str(fs::filename(u"."_cs), u".");
    assert_equal_str(fs::filename(u".."_cs), u"..");
    assert_equal_str(fs::filename(u"/"_cs), u"");
    assert_equal_str(fs::filename(u"//host"_cs), u"");
    assert_equal_str(fs::filename(u"//host/"_cs), u"");
    assert_equal_str(fs::filename(u"//host/share"_cs), u"");
    assert_equal_str(fs::filename(u"//host/share/"_cs), u"");
    assert_equal_str(fs::filename(u"//host/share/file.txt"_cs), u"file.txt");
    assert_equal_str(fs::filename(u"//host/share/dir/file.txt"_cs), u"file.txt");
    assert_equal_str(fs::filename(u"//."_cs), u"");
    assert_equal_str(fs::filename(u"//./UNC"_cs), u"");
    assert_equal_str(fs::filename(u"//./UNC/server"_cs), u"");
    assert_equal_str(fs::filename(u"//./UNC/server/"_cs), u"");
    assert_equal_str(fs::filename(u"//./UNC/server/share"_cs), u"");
    assert_equal_str(fs::filename(u"//./UNC/server/share/"_cs), u"");
    assert_equal_str(fs::filename(u"//./UNC/server/share/file.txt"_cs), u"file.txt");
    assert_equal_str(fs::filename(u"//./UNC/server/share/dir/file.txt"_cs), u"file.txt");
    assert_equal_str(fs::filename(uR"(C:\foo\bar.txt)"_cs), u"bar.txt");
    assert_equal_str(fs::filename(uR"(C:\foo\.bar)"_cs), u".bar");
    assert_equal_str(fs::filename(uR"(C:\foo\bar\)"_cs), u"");
    assert_equal_str(fs::filename(uR"(C:\foo\.)"_cs), u".");
    assert_equal_str(fs::filename(uR"(C:\foo\..)"_cs), u"..");
    assert_equal_str(fs::filename(uR"(C:\)"_cs), u"");
    assert_equal_str(fs::filename(uR"(C:foo.txt)"_cs), u"foo.txt");
#else
    assert_equal_str(fs::filename("/foo/bar.txt"_cs), "bar.txt");
    assert_equal_str(fs::filename("/foo/.bar"_cs), ".bar");
    assert_equal_str(fs::filename("/foo/bar/"_cs), "");
    assert_equal_str(fs::filename("/foo/."_cs), ".");
    assert_equal_str(fs::filename("/foo/.."_cs), "..");
    assert_equal_str(fs::filename("."_cs), ".");
    assert_equal_str(fs::filename(".."_cs), "..");
    assert_equal_str(fs::filename("/"_cs), "");
    assert_equal_str(fs::filename("//host"_cs), "host");

    // https://theboostcpplibraries.com/boost.filesystem-paths#ex.filesystem_05
    // not that we try to imitate boost or std::filesystem (which would be bad),
    // but interpreting paths of other systems is a bad idea as the default
    // behavior.
    assert_equal_str(fs::filename(R"(C:\foo\bar.txt)"_cs), R"(C:\foo\bar.txt)");
    assert_equal_str(fs::filename(R"(C:\foo\.bar)"_cs), R"(C:\foo\.bar)");
    assert_equal_str(fs::filename(R"(C:\foo\bar\)"_cs), R"(C:\foo\bar\)");
    assert_equal_str(fs::filename(R"(C:\foo\.)"_cs), R"(C:\foo\.)");
    assert_equal_str(fs::filename(R"(C:\foo\..)"_cs), R"(C:\foo\..)");
    assert_equal_str(fs::filename(R"(C:\)"_cs), R"(C:\)");
#endif
}

define_test(extension_returns_path_extension)
{
    assert_equal_str(fs::file_extension(SYS_CHAR("/foo/bar.txt"_cs)), SYS_CHAR(".txt"));
    assert_equal_str(fs::file_extension(SYS_CHAR(R"(C:\foo\bar.txt)"_cs)), SYS_CHAR(".txt"));
    assert_equal_str(fs::file_extension(SYS_CHAR("/foo/bar."_cs)), SYS_CHAR("."));
    assert_equal_str(fs::file_extension(SYS_CHAR("/foo/bar"_cs)), SYS_CHAR(""));
    assert_equal_str(fs::file_extension(SYS_CHAR("/foo/bar.txt/bar.cc"_cs)), SYS_CHAR(".cc"));
    assert_equal_str(fs::file_extension(SYS_CHAR("/foo/bar.txt/bar."_cs)), SYS_CHAR("."));
    assert_equal_str(fs::file_extension(SYS_CHAR("/foo/bar.txt/bar"_cs)), SYS_CHAR(""));
    assert_equal_str(fs::file_extension(SYS_CHAR("/foo/."_cs)), SYS_CHAR(""));
    assert_equal_str(fs::file_extension(SYS_CHAR("/foo/.."_cs)), SYS_CHAR(""));
    // this differs from std::filesystem
    assert_equal_str(fs::file_extension(SYS_CHAR("/foo/.hidden"_cs)), SYS_CHAR(".hidden"));
    assert_equal_str(fs::file_extension(SYS_CHAR("/foo/..bar"_cs)), SYS_CHAR(".bar"));
}

define_test(replace_filename_replaces_filename_of_path)
{
    fs::path p{};

    // filename is just the last part of a path, could be
    // a directory too.
    fs::path_set(&p, "/foo/bar");
    fs::replace_filename(&p, "xyz"_cs);
    assert_equal_str(p, SYS_CHAR("/foo/xyz"));

    // shorter
    fs::path_set(&p, "/foo/bar");
    fs::replace_filename(&p, "g"_cs);
    assert_equal_str(p, SYS_CHAR("/foo/g"));

    // longer
    fs::path_set(&p, "/foo/bar");
    fs::replace_filename(&p, "hello world. this is a long filename"_cs);
    assert_equal_str(p, SYS_CHAR("/foo/hello world. this is a long filename"));

    // setting filename
    fs::path_set(&p, "/foo/");
    fs::replace_filename(&p, "abc"_cs);
    assert_equal_str(p, SYS_CHAR("/foo/abc"));

    fs::path_set(&p, "/");
    fs::replace_filename(&p, "abc"_cs);
    assert_equal_str(p, SYS_CHAR("/abc"));

    fs::path_set(&p, "abc");
    fs::replace_filename(&p, "xyz"_cs);
    assert_equal_str(p, SYS_CHAR("xyz"));

    fs::path_set(&p, ".");
    fs::replace_filename(&p, "xyz"_cs);
    assert_equal_str(p, SYS_CHAR("xyz"));

    // empty
    fs::path_set(&p, "");
    fs::replace_filename(&p, "abc"_cs);
    assert_equal_str(p, SYS_CHAR("abc"));

#if Windows
    fs::path_set(&p, u"C:/");
    fs::replace_filename(&p, u"abc"_cs);
    assert_equal_str(p, SYS_CHAR("C:/abc"));

    fs::path_set(&p, u"C:\\");
    fs::replace_filename(&p, u"abc"_cs);
    assert_equal_str(p, SYS_CHAR("C:\\abc"));

    fs::path_set(&p, u"C:");
    fs::replace_filename(&p, u"abc"_cs);
    assert_equal_str(p, SYS_CHAR("C:abc"));

    fs::path_set(&p, u"C:");
    fs::replace_filename(&p, "abc"_cs);
    assert_equal_str(p, SYS_CHAR("C:abc"));

    fs::path_set(&p, uR"(\\server\share)");
    fs::replace_filename(&p, "abc"_cs);
    assert_equal_str(p, SYS_CHAR(R"(\\server\share\abc)"));

    fs::path_set(&p, uR"(\\server\share\)");
    fs::replace_filename(&p, "abc"_cs);
    assert_equal_str(p, SYS_CHAR(R"(\\server\share\abc)"));
#endif

    fs::free(&p);
}

define_test(parent_path_segment_returns_the_parent_path_segment)
{
    assert_equal_str(fs::parent_path_segment(SYS_CHAR("/foo/bar"_cs)), SYS_CHAR("/foo"));
    assert_equal_str(fs::parent_path_segment(SYS_CHAR("/foo"_cs)), SYS_CHAR("/"));
    assert_equal_str(fs::parent_path_segment(SYS_CHAR("/"_cs)), SYS_CHAR("/"));
    assert_equal_str(fs::parent_path_segment(SYS_CHAR("/bar/"_cs)), SYS_CHAR("/bar"));
    assert_equal_str(fs::parent_path_segment(SYS_CHAR(""_cs)), SYS_CHAR(""));

    assert_equal_str(fs::parent_path_segment(SYS_CHAR("abc/def"_cs)), SYS_CHAR("abc"));

#if Windows
    assert_equal_str(fs::parent_path_segment(u"C:/abc"_cs), u"C:/");
    assert_equal_str(fs::parent_path_segment(u"C:/"_cs),    u"C:/");
    assert_equal_str(fs::parent_path_segment(u"C:abc"_cs),  u"C:");
    assert_equal_str(fs::parent_path_segment(u"C:"_cs),     u"C:");
    assert_equal_str(fs::parent_path_segment(u"//server/share"_cs), u"//server/share");
    assert_equal_str(fs::parent_path_segment(u"//server/share/"_cs), u"//server/share/");
    assert_equal_str(fs::parent_path_segment(u"//server/share/file"_cs), u"//server/share/");
    assert_equal_str(fs::parent_path_segment(uR"(\\.\UNC\server\share)"_cs), uR"(\\.\UNC\server\share)");
    assert_equal_str(fs::parent_path_segment(uR"(\\.\UNC\server\share\abc)"_cs), uR"(\\.\UNC\server\share\)");
#endif
}

define_test(path_segments_gets_the_segments_of_a_path)
{
    fs::path p{};
    array<fs::const_fs_string> segs{};

    fs::path_set(&p, "/foo/bar");
    fs::path_segments(&p, &segs);

    assert_equal(segs.size, 3);
    assert_equal_str(segs[0], SYS_CHAR("/"));
    assert_equal_str(segs[1], SYS_CHAR("foo"));
    assert_equal_str(segs[2], SYS_CHAR("bar"));

    // trailing slash is irrelevant
    fs::path_set(&p, "/foo/bar/");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 3);
    assert_equal_str(segs[0], SYS_CHAR("/"));
    assert_equal_str(segs[1], SYS_CHAR("foo"));
    assert_equal_str(segs[2], SYS_CHAR("bar"));

    fs::path_set(&p, "/foo/file.txt");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 3);
    assert_equal_str(segs[0], SYS_CHAR("/"));
    assert_equal_str(segs[1], SYS_CHAR("foo"));
    assert_equal_str(segs[2], SYS_CHAR("file.txt"));

    fs::path_set(&p, "/");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 1);
    assert_equal_str(segs[0], SYS_CHAR("/"));

    fs::path_set(&p, "a/b/c");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 3);
    assert_equal_str(segs[0], SYS_CHAR("a"));
    assert_equal_str(segs[1], SYS_CHAR("b"));
    assert_equal_str(segs[2], SYS_CHAR("c"));

    fs::path_set(&p, ".");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 1);
    assert_equal_str(segs[0], SYS_CHAR("."));

    fs::path_set(&p, "./../");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 2);
    assert_equal_str(segs[0], SYS_CHAR("."));
    assert_equal_str(segs[1], SYS_CHAR(".."));

    fs::path_set(&p, "");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 0);

#if Windows
    fs::path_set(&p, SYS_CHAR(R"(C:)"));
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 1);
    assert_equal_str(segs[0], SYS_CHAR("C:"));

    fs::path_set(&p, SYS_CHAR(R"(C:\)"));
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 1);
    assert_equal_str(segs[0], SYS_CHAR("C:\\"));

    fs::path_set(&p, SYS_CHAR(R"(C:\file.txt)"));
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 2);
    assert_equal_str(segs[0], SYS_CHAR("C:\\"));
    assert_equal_str(segs[1], SYS_CHAR("file.txt"));

    fs::path_set(&p, SYS_CHAR(R"(C:\dir\file.txt)"));
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 3);
    assert_equal_str(segs[0], SYS_CHAR("C:\\"));
    assert_equal_str(segs[1], SYS_CHAR("dir"));
    assert_equal_str(segs[2], SYS_CHAR("file.txt"));

    fs::path_set(&p, SYS_CHAR(R"(\\server)"));
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 1);
    assert_equal_str(segs[0], SYS_CHAR(R"(\\server)"));

    fs::path_set(&p, SYS_CHAR(R"(\\server\share)"));
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 1);
    assert_equal_str(segs[0], SYS_CHAR(R"(\\server\share)"));

    fs::path_set(&p, SYS_CHAR(R"(\\server\share\file.txt)"));
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 2);
    assert_equal_str(segs[0], SYS_CHAR(R"(\\server\share\)"));
    assert_equal_str(segs[1], SYS_CHAR(R"(file.txt)"));

    fs::path_set(&p, SYS_CHAR(R"(\\server\share\dir\file.txt)"));
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 3);
    assert_equal_str(segs[0], SYS_CHAR(R"(\\server\share\)"));
    assert_equal_str(segs[1], SYS_CHAR(R"(dir)"));
    assert_equal_str(segs[2], SYS_CHAR(R"(file.txt)"));
#endif

    fs::free(&p);
    ::free(&segs);
}

define_test(root_returns_the_path_root)
{
    fs::path p{};

#if Windows
#define assert_path_root(Pth, Root)\
    assert_equal_str(fs::root(to_const_string(Pth)), Root)

    // please refer to this link
    // https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats
    assert_path_root(u"", u"");
    assert_path_root(u".", u"");
    assert_path_root(u"..", u"");
    assert_path_root(u"file", u"");
    assert_path_root(u"file.txt", u"");
    assert_path_root(u"C:", uR"(C:)");
    assert_path_root(u"C:\\", uR"(C:\)");
    assert_path_root(u"C:\\file", uR"(C:\)");
    assert_path_root(u"C:\\dir\\file.txt", uR"(C:\)");
    // single slash / backslash at the start means current drive
    assert_path_root(u"/", uR"(/)");
    assert_path_root(u"/file", uR"(/)");
    assert_path_root(u"/dir/file", uR"(/)");
    assert_path_root(u"a", uR"()");
    assert_path_root(u"a/b", uR"()");
    assert_path_root(u"a/b/c", uR"()");
    assert_path_root(u"\\", uR"(\)");
    assert_path_root(u"\\file", uR"(\)");
    assert_path_root(u"\\dir\file", uR"(\)");
    assert_path_root(u"//server", uR"(//server)");
    assert_path_root(u"//server/share", uR"(//server/share)");
    assert_path_root(u"//server/share/file", uR"(//server/share/)");
    assert_path_root(u"//127.0.0.1", uR"(//127.0.0.1)");
    assert_path_root(u"//127.0.0.1/c$", uR"(//127.0.0.1/c$)");
    assert_path_root(u"//127.0.0.1/c$/file", uR"(//127.0.0.1/c$/)");
    assert_path_root(uR"(\dir\file)", uR"(\)");
    assert_path_root(uR"(\\server)", uR"(\\server)");
    assert_path_root(uR"(\\server\share)", uR"(\\server\share)");
    assert_path_root(uR"(\\server\share\file)", uR"(\\server\share\)");
    assert_path_root(uR"(\\127.0.0.1)", uR"(\\127.0.0.1)");
    assert_path_root(uR"(\\127.0.0.1\c$)", uR"(\\127.0.0.1\c$)");
    assert_path_root(uR"(\\127.0.0.1\c$\file)", uR"(\\127.0.0.1\c$\)");
    assert_path_root(u"//?", uR"(//?)"); // is this even valid?
    assert_path_root(u"//?/", uR"(//?/)"); // is this even valid?
    assert_path_root(u"//?/c:", uR"(//?/c:)");
    assert_path_root(u"//?/D:/file", uR"(//?/D:/)");
    assert_path_root(u"//?/E:/dir/file", uR"(//?/E:/)");
    assert_path_root(uR"(\\?)", uR"(\\?)"); // is this even valid?
    assert_path_root(uR"(\\?\)", uR"(\\?\)"); // is this even valid?
    assert_path_root(uR"(\\?\c:)", uR"(\\?\c:)");
    assert_path_root(uR"(\\?\D:\file)", uR"(\\?\D:\)");
    assert_path_root(uR"(\\?\E:\dir\file)", uR"(\\?\E:\)");
    assert_path_root(uR"(\\.)", uR"(\\.)"); // valid?
    assert_path_root(uR"(\\.\C:)", uR"(\\.\C:)");
    assert_path_root(uR"(\\.\C:\file)", uR"(\\.\C:\)");
    assert_path_root(uR"(\\.\C:\dir\file)", uR"(\\.\C:\)");
    assert_path_root(uR"(\\.\Volume{b75e2c83-0000-0000-0000-602f00000000})", uR"(\\.\Volume{b75e2c83-0000-0000-0000-602f00000000})");
    assert_path_root(uR"(\\.\Volume{b75e2c83-0000-0000-0000-602f00000000}\file)", uR"(\\.\Volume{b75e2c83-0000-0000-0000-602f00000000}\)");
    assert_path_root(uR"(\\.\Volume{b75e2c83-0000-0000-0000-602f00000000}\dir\file)", uR"(\\.\Volume{b75e2c83-0000-0000-0000-602f00000000}\)");
    assert_path_root(uR"(\\?\Volume{b75e2c83-0000-0000-0000-602f00000000})", uR"(\\?\Volume{b75e2c83-0000-0000-0000-602f00000000})");
    assert_path_root(uR"(\\?\Volume{b75e2c83-0000-0000-0000-602f00000000}\file)", uR"(\\?\Volume{b75e2c83-0000-0000-0000-602f00000000}\)");
    assert_path_root(uR"(\\?\Volume{b75e2c83-0000-0000-0000-602f00000000}\dir\file)", uR"(\\?\Volume{b75e2c83-0000-0000-0000-602f00000000}\)");
    assert_path_root(uR"(\\.\UNC)", uR"(\\.\UNC)"); // again, is this even valid?
    assert_path_root(uR"(\\.\UNC\server)", uR"(\\.\UNC\server)");
    assert_path_root(uR"(\\.\UNC\server\share)", uR"(\\.\UNC\server\share)");
    assert_path_root(uR"(\\.\UNC\server\share\file)", uR"(\\.\UNC\server\share\)");
    assert_path_root(uR"(\\.\UNC\server\share\dir\file)", uR"(\\.\UNC\server\share\)");
    assert_path_root(uR"(\\?\UNC)", uR"(\\?\UNC)"); // again, is this even valid?
    assert_path_root(uR"(\\?\UNC\server)", uR"(\\?\UNC\server)");
    assert_path_root(uR"(\\?\UNC\server\share)", uR"(\\?\UNC\server\share)");
    assert_path_root(uR"(\\?\UNC\server\share\file)", uR"(\\?\UNC\server\share\)");
    assert_path_root(uR"(\\?\UNC\server\share\dir\file)", uR"(\\?\UNC\server\share\)");

    assert_path_root(uR"(\\\)", uR"(\)");
#else
    fs::path_set(&p, "/foo/bar");
    assert_equal_str(fs::root(&p), SYS_CHAR("/"));

    fs::path_set(&p, "/foo");
    assert_equal_str(fs::root(&p), SYS_CHAR("/"));

    fs::path_set(&p, "/");
    assert_equal_str(fs::root(&p), SYS_CHAR("/"));

    fs::path_set(&p, "bar");
    assert_equal_str(fs::root(&p), SYS_CHAR(""));

    fs::path_set(&p, ".");
    assert_equal_str(fs::root(&p), SYS_CHAR(""));

    fs::path_set(&p, "");
    assert_equal_str(fs::root(&p), SYS_CHAR(""));

#endif

    fs::free(&p);
}

define_test(normalize_normalizes_path)
{
    fs::path p{};

    fs::path_set(&p, "");
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR(""));

    fs::path_set(&p, "/");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\)"));
#else
    assert_equal_str(p, SYS_CHAR("/"));
#endif

    fs::path_set(&p, "///");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\)"));
#else
    assert_equal_str(p, SYS_CHAR("/"));
#endif

    fs::path_set(&p, "/a");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\a)"));
#else
    assert_equal_str(p, SYS_CHAR("/a"));
#endif

    fs::path_set(&p, "/a/////b");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\a\b)"));
#else
    assert_equal_str(p, SYS_CHAR("/a/b"));
#endif

    fs::path_set(&p, "/.");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\)"));
#else
    assert_equal_str(p, SYS_CHAR("/"));
#endif

    fs::path_set(&p, "/..");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\)"));
#else
    assert_equal_str(p, SYS_CHAR("/"));
#endif

    fs::path_set(&p, "/abc/./def");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc\def)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc/def"));
#endif

    fs::path_set(&p, "/abc/../def");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\def)"));
#else
    assert_equal_str(p, SYS_CHAR("/def"));
#endif

    fs::path_set(&p, "/abc/def/xyz/../../uvw");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc\uvw)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc/uvw"));
#endif

    fs::path_set(&p, "/abc/../def/./");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\def)"));
#else
    assert_equal_str(p, SYS_CHAR("/def"));
#endif

    fs::path_set(&p, "/abc/../../def/./");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\def)"));
#else
    assert_equal_str(p, SYS_CHAR("/def"));
#endif

    fs::path_set(&p, "/abc/../def/../../../../");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\)"));
#else
    assert_equal_str(p, SYS_CHAR("/"));
#endif

    fs::path_set(&p, "/abc/def/./.././");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc"));
#endif

    // make sure other dots don't get changed
    fs::path_set(&p, "/abc/def./");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc\def.)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc/def."));
#endif

    fs::path_set(&p, "/abc/.def/");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc\.def)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc/.def"));
#endif

    fs::path_set(&p, "/abc/def../");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc\def..)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc/def.."));
#endif

    fs::path_set(&p, "/abc/..def/");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc\..def)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc/..def"));
#endif

    fs::path_set(&p, "/abc/def.../");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc\def...)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc/def..."));
#endif

    fs::path_set(&p, "/abc/...def/");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc\...def)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc/...def"));
#endif

    // yes this is a valid filename
    fs::path_set(&p, "/abc/.../");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc\...)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc/..."));
#endif

    fs::path_set(&p, "/abc/..../");
    fs::normalize(&p);
#if Windows
    assert_equal_str(p, SYS_CHAR(R"(\abc\....)"));
#else
    assert_equal_str(p, SYS_CHAR("/abc/...."));
#endif

    // relative paths
    fs::path_set(&p, "..");
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR(".."));

    fs::path_set(&p, "../");
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR(".."));

    fs::path_set(&p, ".");
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR("."));

    fs::path_set(&p, "./");
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR("."));

    fs::path_set(&p, "...");
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR("..."));

    fs::path_set(&p, "....");
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR("...."));

#if Windows
    fs::path_set(&p, SYS_CHAR(R"(\\server)"));
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR(R"(\\server)"));

    fs::path_set(&p, SYS_CHAR(R"(\\server\)"));
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR(R"(\\server\)"));

    fs::path_set(&p, SYS_CHAR(R"(\\server\share\)"));
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR(R"(\\server\share\)"));

    fs::path_set(&p, SYS_CHAR(R"(\\server\share\dir\file)"));
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR(R"(\\server\share\dir\file)"));

    fs::path_set(&p, SYS_CHAR(R"(\\server\share\dir\..\file)"));
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR(R"(\\server\share\file)"));

    fs::path_set(&p, SYS_CHAR(R"(\\server\share\dir\..\..\file)"));
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR(R"(\\server\share\file)"));

    fs::path_set(&p, SYS_CHAR(R"(\\.\UNC\server\share\dir\..\..\file)"));
    fs::normalize(&p);
    assert_equal_str(p, SYS_CHAR(R"(\\.\UNC\server\share\file)"));
#endif

    fs::free(&p);
}

define_test(longest_existing_path_returns_longest_existing_path)
{
    fs::path p{};
    fs::path longest{};

    fs::path_set(&p, SANDBOX_DIR);
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SANDBOX_DIR);

    fs::path_set(&p, SANDBOX_TEST_FILE);
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SANDBOX_TEST_FILE);

#if Windows
    fs::path_set(&p, SYS_CHAR(R"(C:\DoesntExist\Hopefully)"));
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SYS_CHAR(R"(C:\)"));

    fs::path_set(&p, SYS_CHAR(R"(/abc/def)"));
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SYS_CHAR(R"(/)"));

    fs::path_set(&p, SYS_CHAR(R"(C:\Windows)"));
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SYS_CHAR(R"(C:\Windows)"));

    fs::path_set(&p, SYS_CHAR(R"(C:\Windows\system1234)"));
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SYS_CHAR(R"(C:\Windows)"));

    fs::path_set(&p, SYS_CHAR(R"(C:/Windows/notepad.exe/xyz)"));
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SYS_CHAR(R"(C:/Windows/notepad.exe)"));
#else
    fs::path_set(&p, "/foo/bar");
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SYS_CHAR("/"));

    fs::path_set(&p, "/tmp");
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SYS_CHAR("/tmp"));

    // let's hope abcxyz doesn't exist
    fs::path_set(&p, "/tmp/abcxyz");
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SYS_CHAR("/tmp"));
#endif

    fs::free(&p);
    fs::free(&longest);
}

define_test(absolute_path_gets_the_absolute_path)
{
    fs::path p{};
    fs::path absp{};

    fs::path_set(&p, "/foo/bar");
    fs::absolute_path(&p, &absp);
    assert_equal_str(absp, SYS_CHAR("/foo/bar"));

    fs::path_set(&p, "foo/bar");
    fs::absolute_path(&p, &absp);
#if Windows
    assert_equal_str(absp, SANDBOX_DIR u"\\foo/bar");
#else
    assert_equal_str(absp, SANDBOX_DIR "/foo/bar");
#endif

    // does not expand relative paths within the path,
    // use normalize(&p) to remove relative parts (. and ..) within the path,
    // or resolve the path and symlinks with canonical_path(&p, &canon).
    fs::path_set(&p, "/foo/bar/./abc/../def");
    fs::absolute_path(&p, &absp);
    assert_equal_str(absp, SYS_CHAR("/foo/bar/./abc/../def"));

    fs::path_set(&p, "foo/bar/./abc/../def");
    fs::absolute_path(&p, &absp);
#if Windows
    assert_equal_str(absp, SANDBOX_DIR "\\foo/bar/./abc/../def");
#else
    assert_equal_str(absp, SANDBOX_DIR "/foo/bar/./abc/../def");
#endif

    // using the same pointer for input and output was removed because it's unintuitive.
    // an assert will now crash this.
    /*
    fs::path_set(&p, "foo/bar");
    fs::absolute_path(&p, &p);
    assert_equal_str(p, SANDBOX_DIR "/foo/bar");
    */

    fs::free(&absp);
    fs::free(&p);
}

define_test(canonical_path_gets_canonical_path)
{
    fs::path p{};
    fs::path canonp{};

    fs::path_set(&p, SANDBOX_TEST_DIR);
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

    fs::path_set(&p, SANDBOX_TEST_DIR "/.");
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

    fs::path_set(&p, SANDBOX_TEST_DIR "/..");
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

    fs::path_set(&p, SANDBOX_TEST_DIR "/../");
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

    fs::path_set(&p, SANDBOX_TEST_DIR "/./../.");
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

    fs::path_set(&p, SANDBOX_TEST_DIR "/././.");
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

    // fails on paths that don't exist
    fs::path_set(&p, "/tmp/abc/../def");
    assert_equal(fs::canonical_path(&p, &canonp), false);

    fs::free(&canonp);
    fs::free(&p);
}

define_test(weakly_canonical_path_gets_weakly_canonical_path)
{
    fs::path p{};
    fs::path canonp{};

    fs::path_set(&p, SANDBOX_TEST_DIR);
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

#if Windows
    fs::path_set(&p, SANDBOX_TEST_DIR "\\.");
#else
    fs::path_set(&p, SANDBOX_TEST_DIR "/.");
#endif
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

#if Windows
    fs::path_set(&p, SANDBOX_TEST_DIR "\\..");
#else
    fs::path_set(&p, SANDBOX_TEST_DIR "/..");
#endif
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

#if Windows
    fs::path_set(&p, SANDBOX_TEST_DIR "\\..\\");
#else
    fs::path_set(&p, SANDBOX_TEST_DIR "/../");
#endif
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

#if Windows
    fs::path_set(&p, SANDBOX_TEST_DIR "\\.\\..\\.");
#else
    fs::path_set(&p, SANDBOX_TEST_DIR "/./../.");
#endif
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

#if Windows
    fs::path_set(&p, SANDBOX_TEST_DIR "\\.\\.\\.");
#else
    fs::path_set(&p, SANDBOX_TEST_DIR "/././.");
#endif
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

    // does not fail on paths that don't exist (assuming /tmp/abc does not exist)
#if Windows
    fs::path_set(&p, R"(\tmp\abc\..\def)");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SYS_CHAR(R"(\tmp\def)"));
#else
    fs::path_set(&p, "/tmp/abc/../def");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SYS_CHAR("/tmp/def"));
#endif

#if Windows
    fs::path_set(&p, R"(\tmp\.\.\abc\..\def\abc)");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SYS_CHAR(R"(\tmp\def\abc)"));
#else
    fs::path_set(&p, "/tmp/././abc/../def/abc");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SYS_CHAR("/tmp/def/abc"));
#endif

    fs::free(&canonp);
    fs::free(&p);
}

define_test(get_symlink_target_reads_symlink)
{
    error err{};
    fs::path target{};

    assert_equal(fs::get_symlink_target(SANDBOX_TEST_SYMLINK, &target, &err), true);
    assert_equal_str(target, SANDBOX_TEST_FILE);

    assert_equal(fs::get_symlink_target(SANDBOX_TEST_SYMLINK_NO_TARGET, &target, &err), true);
#if Windows
    assert_equal_str(target, SANDBOX_DIR "\\symlink_dest");
#else
    assert_equal_str(target, SANDBOX_DIR "/symlink_dest");
#endif

    assert_equal(fs::get_symlink_target(SANDBOX_TEST_FILE, &target, &err), false);

#if Linux
    assert_equal(err.error_code, EINVAL);
#endif

    assert_equal(fs::get_symlink_target(SANDBOX_DIR "/doesnotexist", &target, &err), false);

#if Windows
    assert_equal(err.error_code, ERROR_FILE_NOT_FOUND);
#else
    assert_equal(err.error_code, ENOENT);
#endif

    fs::free(&target);
}

define_test(get_current_path_gets_current_path)
{
    fs::path p{};

    bool ret = fs::get_current_path(&p);

    assert_equal(ret, true);
    assert_equal(string_length(p.data), p.size);
    assert_equal_str(p.data, SANDBOX_DIR);

    fs::free(&p);
}

define_test(set_current_path_sets_current_path)
{
    fs::path p{};
    fs::path p2{};

    fs::path_set(&p, SANDBOX_TEST_DIR);

    bool ret = fs::set_current_path(&p);

    fs::get_current_path(&p2);

    assert_equal(ret, true);
    assert_equal_str(p2, SANDBOX_TEST_DIR);

    fs::path_set(&p, SANDBOX_DIR);
    fs::set_current_path(&p);

    fs::free(&p);
    fs::free(&p2);
}

define_test(append_appends_to_path)
{
    fs::path p{};

#if Windows
    fs::path_set(&p, R"(C:\Windows)");
    fs::path_append(&p, "notepad.exe");
    assert_equal_str(p, SYS_CHAR(R"(C:\Windows\notepad.exe)"));

    fs::path_set(&p, R"(C:\Windows\)");
    fs::path_append(&p, "notepad.exe");
    assert_equal_str(p, SYS_CHAR(R"(C:\Windows\notepad.exe)"));

    fs::path_set(&p, R"(C:\Windows)");
    fs::path_append(&p, "/notepad.exe");
    assert_equal_str(p, SYS_CHAR(R"(/notepad.exe)"));

    fs::path_set(&p, R"(C:\Windows)");
    fs::path_append(&p, "\\notepad.exe");
    assert_equal_str(p, SYS_CHAR(R"(\notepad.exe)"));

    fs::path_set(&p, R"(C:\Windows)");
    fs::path_append(&p, "notepad.exe/system32");
    assert_equal_str(p, SYS_CHAR(R"(C:\Windows\notepad.exe/system32)"));

    fs::path_set(&p, R"(C:)");
    fs::path_append(&p, "Windows\\System32");
    assert_equal_str(p, SYS_CHAR(R"(C:\Windows\System32)"));

    fs::path_set(&p, "abc");
    fs::path_append(&p, "xyz");
    assert_equal_str(p, SYS_CHAR("abc\\xyz"));

    fs::path_set(&p, "");
    fs::path_append(&p, "xyz");
    assert_equal_str(p, SYS_CHAR("xyz"));
#else
    fs::path_set(&p, "/etc");
    fs::path_append(&p, "passwd");
    assert_equal_str(p, SYS_CHAR("/etc/passwd"));

    // with trailing separator, same result
    fs::path_set(&p, "/etc/");
    fs::path_append(&p, "passwd");
    assert_equal_str(p, SYS_CHAR("/etc/passwd"));

    // replaces when appending absolute path
    fs::path_set(&p, "/etc");
    fs::path_append(&p, "/passwd");
    assert_equal_str(p, SYS_CHAR("/passwd"));

    fs::path_set(&p, "//xyz");
    fs::path_append(&p, "abc");
    assert_equal_str(p, SYS_CHAR("//xyz/abc"));

    fs::path_set(&p, "//xyz/");
    fs::path_append(&p, "abc");
    assert_equal_str(p, SYS_CHAR("//xyz/abc"));

    fs::path_set(&p, "//xyz/dir");
    fs::path_append(&p, "/abc");
    assert_equal_str(p, SYS_CHAR("/abc"));

    fs::path_set(&p, "//xyz/dir");
    fs::path_append(&p, "//xyz/abc");
    assert_equal_str(p, SYS_CHAR("//xyz/abc"));

    // path alone doesnt check whether what it's pointing to is a directory or not
    // so this will just append at the end.
    fs::path_set(&p, "/etc/test.txt");
    fs::path_append(&p, "passwd");
    assert_equal_str(p, SYS_CHAR("/etc/test.txt/passwd"));

    fs::path_set(&p, "/etc/test.txt");
    fs::path_append(&p, "passwd/test2.txt");
    assert_equal_str(p, SYS_CHAR("/etc/test.txt/passwd/test2.txt"));

    // relative
    fs::path_set(&p, "abc");
    fs::path_append(&p, "xyz");
    assert_equal_str(p, SYS_CHAR("abc/xyz"));

    fs::path_set(&p, "");
    fs::path_append(&p, "xyz");
    assert_equal_str(p, SYS_CHAR("xyz"));

#endif

    fs::free(&p);
}

define_test(concat_concats_to_path)
{
    fs::path p{};

    fs::path_set(&p, "");
    fs::path_concat(&p, "");
    assert_equal(p.size, 0);
    assert_equal_str(p, SYS_CHAR(""));

    fs::path_set(&p, "/");
    fs::path_concat(&p, "");
    assert_equal(p.size, 1);
    assert_equal_str(p, SYS_CHAR("/"));

    fs::path_set(&p, "");
    fs::path_concat(&p, "/");
    assert_equal(p.size, 1);
    assert_equal_str(p, SYS_CHAR("/"));

    fs::path_set(&p, "abc");
    fs::path_concat(&p, "def");
    assert_equal(p.size, 6);
    assert_equal_str(p, SYS_CHAR("abcdef"));

    fs::path_set(&p, "/etc");
    fs::path_concat(&p, "passwd");
    assert_equal(p.size, 10);
    assert_equal_str(p, SYS_CHAR("/etcpasswd"));

    // paths of different character types will get converted
    fs::path_set(&p, "/etc");
    fs::path_concat(&p, u"passwd");
    assert_equal(p.size, 10);
    assert_equal_str(p, SYS_CHAR("/etcpasswd"));

    fs::free(&p);
}

define_test(relative_path_gets_the_relative_path_to_another)
{
    fs::path from{};
    fs::path to{};
    fs::path rel{};

#define assert_rel_path(From, To, ExpectedResult)\
    fs::path_set(&from, SYS_CHAR(From));\
    fs::path_set(&to, SYS_CHAR(To));\
    fs::relative_path(&from, &to, &rel);\
    assert_equal_str(to_const_string(&rel), to_const_string(SYS_CHAR(ExpectedResult)))

#if Windows
    assert_rel_path(".\\",      ".\\def", "def");
    assert_rel_path(".\\",      "def",    "def");
    assert_rel_path(".",        "def",    "def");
    assert_rel_path(".",        ".\\def", "def");
    assert_rel_path(".\\dir",   ".\\def", "..\\def");
    assert_rel_path(".\\dir\\", ".\\def", "..\\def");
    assert_rel_path(".\\dir\\", "def",    "..\\def");

    assert_rel_path("C:\\a\\b\\c",  "C:\\a\\d",   "..\\..\\d");
    assert_rel_path("C:\\a\\d",    "C:\\a\\b\\c", "..\\b\\c");
    assert_rel_path("a",       "a\\b\\c",  "b\\c");
    assert_rel_path("a",       "b",  "..\\b");
    // roots differ
    assert_rel_path("a",       "C:\\a\\b\\c", "");
    assert_rel_path("C:\\a\\b\\c",  "a",      "");
    assert_rel_path("C:\\a\\d",    "D:\\a\\b\\c", "");

    assert_rel_path(".\\dir\\a\\b\\c", "def", "..\\..\\..\\..\\def");
#else

    assert_rel_path("./",      "./def", "def");
    assert_rel_path("./",      "def",   "def");
    assert_rel_path(".",       "def",   "def");
    assert_rel_path(".",       "./def", "def");
    assert_rel_path("./dir",   "./def", "../def");
    assert_rel_path("./dir/",  "./def", "../def");
    assert_rel_path("./dir/",  "def",   "../def");

    assert_rel_path("/a/b/c",  "/a/d",   "../../d");
    assert_rel_path("/a/d",    "/a/b/c", "../b/c");
    assert_rel_path("a",       "a/b/c",  "b/c");
    assert_rel_path("a",       "b",  "../b");
    // roots differ
    assert_rel_path("a",       "/a/b/c", "");
    assert_rel_path("/a/b/c",  "a",      "");

    assert_rel_path("./dir/a/b/c", "def", "../../../../def");

#endif

    fs::free(&from);
    fs::free(&to);
    fs::free(&rel);
}

define_test(touch_touches_file)
{
    fs::path p{};

    fs::path_set(&p, SANDBOX_TEST_FILE "_touch1");

    assert_equal(fs::exists(&p), 0);
    assert_equal(fs::touch(&p), true);
    assert_equal(fs::exists(&p), 1);

    fs::filesystem_info old_info{};
    fs::filesystem_info new_info{};

    fs::query_filesystem(&p, &old_info, true, fs::query_flag::FileTimes);

    sleep_ms(200);

    assert_equal(fs::touch(&p), true);
    fs::query_filesystem(&p, &new_info, true, fs::query_flag::FileTimes);

#if Windows
    assert_greater(new_info.detail.file_times.last_write_time, old_info.detail.file_times.last_write_time);
    assert_greater(new_info.detail.file_times.change_time,    old_info.detail.file_times.change_time);
#else
    assert_greater(new_info.stx_mtime, old_info.stx_mtime);
#endif

    fs::free(&p);
}

define_test(copy_file_copies_file)
{
    fs::path from{};
    fs::path to{};

    fs::path_set(&from, SANDBOX_TEST_FILE);
    fs::path_set(&to, SANDBOX_TEST_FILE "_copy1");

    error err{};

    assert_equal(fs::copy_file(&from, &to, fs::copy_file_option::None, &err), true);

    assert_equal(fs::copy_file(&from, &to, fs::copy_file_option::None, &err), false);

#if Windows
    assert_equal(err.error_code, ERROR_ALREADY_EXISTS);
#else
    assert_equal(err.error_code, EEXIST);
#endif

    // overwriting
    assert_equal(fs::copy_file(&from, &to, fs::copy_file_option::OverwriteExisting, &err), true);

    assert_equal(fs::copy_file(&from, &to, fs::copy_file_option::SkipExisting, &err), true);

    // returns true even if not copied
    assert_equal(fs::copy_file(&from, &to, fs::copy_file_option::UpdateExisting, &err), true);

    fs::filesystem_info from_info{};
    fs::filesystem_info to_info{};
    assert_equal(fs::query_filesystem(&from, &from_info, true, fs::query_flag::FileTimes), true);
    assert_equal(fs::query_filesystem(&to, &to_info, true, fs::query_flag::FileTimes), true);

    // check that the date from To is indeed later than From
#if Windows
    assert_greater(to_info.detail.file_times.last_write_time, from_info.detail.file_times.last_write_time);
    assert_greater(to_info.detail.file_times.change_time,     from_info.detail.file_times.change_time);
#else
    assert_greater(to_info.stx_mtime, from_info.stx_mtime);
#endif

    sleep_ms(100);
    fs::touch(&from);

    assert_equal(fs::query_filesystem(&from, &from_info, true, fs::query_flag::FileTimes), true);

    // now From should be newer than To
#if Windows
    assert_greater(from_info.detail.file_times.last_write_time, to_info.detail.file_times.last_write_time);
    assert_greater(from_info.detail.file_times.change_time,     to_info.detail.file_times.change_time);
#else
    assert_greater(from_info.stx_mtime, to_info.stx_mtime);
#endif

    sleep_ms(100);
    assert_equal(fs::copy_file(&from, &to, fs::copy_file_option::UpdateExisting, &err), true);

    assert_equal(fs::query_filesystem(&to, &to_info, true, fs::query_flag::FileTimes), true);

    // and now To is newer again
#if Windows
    assert_greater(to_info.detail.file_times.last_write_time, from_info.detail.file_times.last_write_time);
    assert_greater(to_info.detail.file_times.change_time,     from_info.detail.file_times.change_time);
#else
    assert_greater(to_info.stx_mtime, from_info.stx_mtime);
#endif

    fs::free(&from);
    fs::free(&to);
}

define_test(copy_directory_copies_directory)
{
    error err{};

    const sys_char *dir_from = SANDBOX_DIR "/copy_dir";
    const sys_char *dir_to = SANDBOX_DIR "/copy_dir_to";

    fs::create_directory(dir_from);
    fs::create_directories(SANDBOX_DIR "/copy_dir/dir1/dir2");
    fs::create_directory(SANDBOX_DIR "/copy_dir/dir3");
    fs::touch(SANDBOX_DIR "/copy_dir/file1");
    fs::touch(SANDBOX_DIR "/copy_dir/dir1/file2");
    fs::touch(SANDBOX_DIR "/copy_dir/dir1/dir2/file3");

    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 0);
    assert_equal(fs::copy_directory(dir_from, dir_to, -1, fs::copy_file_option::None, &err), true);
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/file1"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir3"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/file2"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2/file3"), 1);

    fs::remove(dir_to);

    // up to depth 0 only
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 0);
    assert_equal(fs::copy_directory(dir_from, dir_to, 0, fs::copy_file_option::None, &err), true);
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/file1"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir3"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/file2"), 0);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2"), 0);
    assert_not_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2/file3"), 1);

    fs::remove(dir_to);

    // up to depth 1
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 0);
    assert_equal(fs::copy_directory(dir_from, dir_to, 1, fs::copy_file_option::None, &err), true);
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/file1"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir3"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/file2"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2/file3"), 0);

    fs::remove(dir_to);

    // target directory exists
    fs::create_directory(dir_to);
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 1);
    assert_equal(fs::copy_directory(dir_from, dir_to, -1, fs::copy_file_option::None, &err), false);
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/file1"), 0);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir3"), 0);
    assert_not_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/file2"), 1);
    assert_not_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2"), 1);
    assert_not_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2/file3"), 1);

#if Windows
    assert_equal(err.error_code, ERROR_ALREADY_EXISTS);
#else
    assert_equal(err.error_code, EEXIST);
#endif

    // target directory exists but we overwrite
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 1);
    assert_equal(fs::copy_directory(dir_from, dir_to, -1, fs::copy_file_option::OverwriteExisting, &err), true);
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/file1"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir3"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/file2"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2"), 1);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2/file3"), 1);

    fs::remove(dir_to);
}

define_test(copy_copies_files_and_directories)
{
    const sys_char *dir_from = SANDBOX_DIR "/copy1_dir";
    const sys_char *dir_to = SANDBOX_DIR "/copy1_dir_to";
    fs::create_directory(dir_from);

    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 0);
    assert_equal(fs::copy(dir_from, dir_to), true);
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 1);

    const sys_char *file_from = SANDBOX_DIR "/copy1_file";
    const sys_char *file_to = SANDBOX_DIR "/copy1_file_to";
    fs::touch(file_from);

    assert_equal(fs::exists(file_from), 1);
    assert_equal(fs::exists(file_to), 0);
    assert_equal(fs::copy(file_from, file_to), true);
    assert_equal(fs::exists(file_from), 1);
    assert_equal(fs::exists(file_to), 1);
}

define_test(create_directory_creates_directory)
{
    error err{};
    fs::path p{};

    fs::path_set(&p, SANDBOX_TEST_DIR "/_create_dir1");

    assert_equal(fs::exists(&p), 0); 
    assert_equal(fs::create_directory(&p), true); 
    assert_equal(fs::exists(&p), 1); 

    // creating a directory that already exists returns true (but also yields error code)
    assert_equal(fs::create_directory(&p, fs::permission::User, &err), true); 
    assert_equal(fs::exists(&p), 1); 

#if Windows
    assert_equal(err.error_code, ERROR_ALREADY_EXISTS);
#else
    assert_equal(err.error_code, EEXIST);
#endif

    // creating a directory on a path thats not a directory DOES yield an error
    fs::path_set(&p, SANDBOX_TEST_FILE);
    assert_equal(fs::exists(&p), 1); 
    assert_equal(fs::create_directory(&p, fs::permission::User, &err), false);
    assert_equal(fs::exists(&p), 1); 

#if Windows
    assert_equal(err.error_code, ERROR_ALREADY_EXISTS);
#else
    assert_equal(err.error_code, EEXIST);
#endif

    // relative path (in sandbox)
    fs::path_set(&p, "_create_dir2");

    assert_equal(fs::exists(&p), 0); 
    assert_equal(fs::create_directory(&p), true); 
    assert_equal(fs::exists(&p), 1); 

    fs::path_set(&p, "_create_dir2/xyz");

    assert_equal(fs::exists(&p), 0); 
    assert_equal(fs::create_directory(&p), true); 
    assert_equal(fs::exists(&p), 1); 

    // no permission
    fs::path_set(&p, "/root/_create_dir3");
    assert_equal(fs::create_directory(&p, fs::permission::User, &err), false); 

    // does not create parent directories
    fs::path_set(&p, SANDBOX_TEST_DIR "/_create_dir4/abc/def");
    assert_equal(fs::create_directory(&p, fs::permission::User, &err), false); 

#if Windows
    assert_equal(err.error_code, ERROR_PATH_NOT_FOUND);
#else
    assert_equal(err.error_code, ENOENT);
#endif

    fs::free(&p);
}

define_test(create_directories_creates_directories_and_parents)
{
    fs::path p{};

    fs::path_set(&p, SANDBOX_TEST_DIR "/_create_dirs1");

    assert_equal(fs::exists(&p), 0); 
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), 1); 

    // creating a directory that already exists does not yield an error
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), 1); 

    // relative path (in sandbox)
    fs::path_set(&p, "_create_dirs2");

    assert_equal(fs::exists(&p), 0); 
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), 1); 

    fs::path_set(&p, "_create_dirs2/xyz");

    assert_equal(fs::exists(&p), 0); 
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), 1); 
    
    
    // DOES create parent directories
    fs::path_set(&p, SANDBOX_TEST_DIR "/_create_dirs4/abc/def");
    assert_not_equal(fs::exists(&p), 1);
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), 1); 

    fs::path_set(&p, "_create_dirs5/xyz");
    assert_not_equal(fs::exists(&p), 1);
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), 1); 

    error err{};

    // no permission
    fs::path_set(&p, "/root/_create_dirs3");
    assert_equal(fs::create_directories(&p, fs::permission::User, &err), false); 

    fs::path_set(&p, "/root/_create_dirs3/xyz");
    assert_equal(fs::create_directories(&p, fs::permission::User, &err), false); 

    fs::free(&p);
}

define_test(create_hard_link_creates_hard_link)
{
    fs::path target{};
    fs::path link{};
    error err{};

    fs::path_set(&target, SANDBOX_TEST_FILE);
    fs::path_set(&link,   SANDBOX_DIR "/_hardlink1");

    assert_equal(fs::exists(&link), 0); 
    assert_equal(fs::create_hard_link(&target, &link), true); 
    assert_equal(fs::exists(&link), 1); 

    assert_equal(fs::are_equivalent(&target, &link), true);
    assert_equal(fs::are_equivalent(&target, &link, false), true);

    // does not overwrite links or files
    assert_equal(fs::create_hard_link(&target, &link, &err), false); 

#if Windows
    assert_equal(err.error_code, ERROR_ALREADY_EXISTS);
#else
    assert_equal(err.error_code, EEXIST);
#endif
    
    fs::path_set(&target, SANDBOX_TEST_FILE "_does_not_exist1");
    fs::path_set(&link,   SANDBOX_DIR "/_hardlink2");

    // can't create hard links to things that don't exist
    assert_equal(fs::create_hard_link(&target, &link, &err), false); 

#if Windows
    assert_equal(err.error_code, ERROR_FILE_NOT_FOUND);
#else
    assert_equal(err.error_code, ENOENT);
#endif

    // cannot create hard link to a directory
    fs::path_set(&target, SANDBOX_TEST_DIR);
    fs::path_set(&link,   SANDBOX_DIR "/_hardlink3");

    assert_equal(fs::exists(&link), 0); 
    assert_equal(fs::create_hard_link(&target, &link, &err), false); 
    assert_equal(fs::exists(&link), 0); 

#if Windows
    assert_equal(err.error_code, ERROR_ACCESS_DENIED);
#else
    assert_equal(err.error_code, EPERM);
#endif

    fs::free(&target);
    fs::free(&link);
}

define_test(create_symlink_creates_symlink)
{
    fs::path target{};
    fs::path link{};
    error err{};

    fs::path_set(&target, SANDBOX_TEST_FILE);
    fs::path_set(&link,   SANDBOX_DIR "/_symlink1");

    assert_equal(fs::exists(&link), 0); 
    assert_equal(fs::create_symlink(&target, &link), true); 
    assert_equal(fs::exists(&link), 1); 

    assert_equal(fs::are_equivalent(&target, &link), true);
    assert_equal(fs::are_equivalent(&target, &link, false), false);

    // does not overwrite links or files
    assert_equal(fs::create_symlink(&target, &link, &err), false); 

#if Windows
    assert_equal(err.error_code, ERROR_ALREADY_EXISTS);
#else
    assert_equal(err.error_code, EEXIST);
#endif
    
    fs::path_set(&target, SANDBOX_TEST_FILE "_does_not_exist2");
    fs::path_set(&link,   SANDBOX_DIR "/_symlink2");

    assert_equal(fs::create_symlink(&target, &link, &err), true);

    assert_equal(fs::exists(&link), 0); 
    assert_equal(fs::exists(&link, false), 1); 

    // works on directories
    fs::path_set(&target, SANDBOX_TEST_DIR);
    fs::path_set(&link,   SANDBOX_DIR "/_symlink3");

    assert_equal(fs::exists(&link), 0); 
    assert_equal(fs::create_symlink(&target, &link), true); 
    assert_equal(fs::exists(&link), 1); 

    assert_equal(fs::are_equivalent(&target, &link), true);
    assert_equal(fs::are_equivalent(&target, &link, false), false);

    fs::free(&target);
    fs::free(&link);
}

define_test(move_moves_files_and_directories)
{
    fs::path src{};
    fs::path dst{};
    error err{};

    fs::path_set(&src, SANDBOX_DIR "/_move1");
    fs::path_set(&dst, SANDBOX_DIR "/_move2");

    fs::copy_file(SANDBOX_TEST_FILE, &src);

    assert_equal(fs::exists(&src), 1); 
    assert_equal(fs::exists(&dst), 0); 
    assert_equal(fs::move(&src, &dst), true); 
    assert_equal(fs::exists(&src), 0); 
    assert_equal(fs::exists(&dst), 1); 

    fs::copy_file(SANDBOX_TEST_FILE, &src);

    // overwrites existing files
    assert_equal(fs::exists(&src), 1); 
    assert_equal(fs::exists(&dst), 1); 
    assert_equal(fs::move(&src, &dst), true); 
    assert_equal(fs::exists(&src), 0); 
    assert_equal(fs::exists(&dst), 1); 

    // can move directories
    auto src_dir = SANDBOX_DIR "/_movedir1";
    auto dst_dir = SANDBOX_DIR "/_movedir2";

    fs::create_directory(src_dir);

    assert_equal(fs::exists(src_dir), 1); 
    assert_equal(fs::exists(dst_dir), 0); 
    assert_equal(fs::move(src_dir, dst_dir), true); 
    assert_equal(fs::exists(src_dir), 0); 
    assert_equal(fs::exists(dst_dir), 1); 

    assert_equal(fs::move(dst_dir, src_dir), true); 

    // cannot move directory into file
    assert_equal(fs::move(src_dir, dst, &err), false); 

#if Windows
    assert_equal(err.error_code, ERROR_INVALID_PARAMETER);
#else
    assert_equal(err.error_code, ENOTDIR);
#endif

    // cannot move file into directory (as in, replace the directorys name)
    assert_equal(fs::move(dst, src_dir, &err), false); 

#if Windows
    assert_equal(err.error_code, ERROR_INVALID_PARAMETER);
#else
    assert_equal(err.error_code, EISDIR);
#endif

    // can move directory thats not empty
    assert_equal(fs::move(dst, SANDBOX_DIR "/_movedir1/_move1"), true);

    assert_equal(fs::exists(src_dir), 1); 
    assert_equal(fs::exists(dst_dir), 0); 
    assert_equal(fs::move(src_dir, dst_dir), true); 
    assert_equal(fs::exists(src_dir), 0); 
    assert_equal(fs::exists(dst_dir), 1); 
    assert_equal(fs::exists(SANDBOX_DIR "/_movedir2/_move1"), 1); 

    fs::free(&src);
    fs::free(&dst);
}

define_test(remove_file_removes_file)
{
    fs::path p{};
    error err{};

    fs::path_set(&p, SANDBOX_DIR "/_remove_file1");
    fs::copy_file(SANDBOX_TEST_FILE, &p);

    assert_equal(fs::exists(p), 1); 
    assert_equal(fs::remove_file(p), true); 
    assert_equal(fs::exists(p), 0); 

    assert_equal(fs::remove_file(p, &err), false); 

#if Windows
    assert_equal(err.error_code, ERROR_FILE_NOT_FOUND);
#else
    assert_equal(err.error_code, ENOENT);
#endif

    assert_equal(fs::remove_file(SANDBOX_DIR, &err), false); 

#if Windows
    assert_equal(err.error_code, ERROR_ACCESS_DENIED);
#else
    assert_equal(err.error_code, EISDIR);
#endif

    fs::free(&p);
}

define_test(remove_empty_directory_removes_only_empty_directories)
{
    error err{};

    const sys_char *dir1 = SANDBOX_DIR "/remove_empty_dir1";
    const sys_char *dir2 = SANDBOX_DIR "/remove_empty_dir2";
    const sys_char *file1 = SANDBOX_DIR "/remove_empty_dir_file";
    const sys_char *file2 = SANDBOX_DIR "/remove_empty_dir2/not_empty";

    fs::create_directory(dir1);
    fs::create_directory(dir2);
    fs::touch(file1);
    fs::touch(file2);

    assert_equal(fs::exists(dir1), 1); 
    assert_equal(fs::remove_empty_directory(dir1, &err), true); 
    assert_equal(fs::exists(dir1), 0); 

    // not empty
    assert_equal(fs::exists(dir2), 1); 
    assert_equal(fs::remove_empty_directory(dir2, &err), false); 
    assert_equal(fs::exists(dir2), 1); 

#if Windows
    assert_equal(err.error_code, ERROR_DIR_NOT_EMPTY);
#else
    assert_equal(err.error_code, ENOTEMPTY);
#endif

    assert_equal(fs::exists(file1), 1); 
    assert_equal(fs::remove_empty_directory(file1, &err), false); 
    assert_equal(fs::exists(file1), 1); 

#if Windows
    assert_equal(err.error_code, ERROR_DIRECTORY);
#else
    assert_equal(err.error_code, ENOTDIR);
#endif
}

define_test(remove_directory_removes_directories)
{
    error err{};

    const sys_char *dir1 = SANDBOX_DIR  "/remove_dir1";
    const sys_char *dir2 = SANDBOX_DIR  "/remove_dir2";
    const sys_char *dir3 = SANDBOX_DIR  "/remove_dir3";
    const sys_char *file1 = SANDBOX_DIR "/remove_dir_file";

    fs::create_directory(dir1);
    fs::create_directory(dir2);
    fs::create_directory(dir3);
    fs::touch(file1);
    fs::touch(SANDBOX_DIR "/remove_dir2/notempty");

    fs::create_directory(SANDBOX_DIR "/remove_dir3/dir4");
    fs::create_directory(SANDBOX_DIR "/remove_dir3/dir5");
    fs::touch(SANDBOX_DIR "/remove_dir3/file3");
    fs::touch(SANDBOX_DIR "/remove_dir3/dir4/file4");

    assert_equal(fs::exists(dir1), 1); 
    assert_equal(fs::remove_directory(dir1, &err), true); 
    assert_equal(fs::exists(dir1), 0); 

    // not empty
    assert_equal(fs::exists(dir2), 1); 
    assert_equal(fs::remove_directory(dir2, &err), true);
    assert_equal(fs::exists(dir2), 0); 

    assert_equal(err.error_code, 0);

    // not empty with subdirectories and more descendants
    assert_equal(fs::exists(dir3), 1); 
    assert_equal(fs::remove_directory(dir3, &err), true);
    assert_equal(fs::exists(dir3), 0); 

    assert_equal(err.error_code, 0);

    assert_equal(fs::exists(file1), 1); 
    assert_equal(fs::remove_directory(file1, &err), false); 
    assert_equal(fs::exists(file1), 1); 

#if Windows
    assert_equal(err.error_code, ERROR_DIRECTORY);
#else
    assert_equal(err.error_code, ENOTDIR);
#endif
}

define_test(remove_removes_anything)
{
    error err{};

    const sys_char *dir1 = SANDBOX_DIR  "/remove1";
    const sys_char *dir2 = SANDBOX_DIR  "/remove2";
    const sys_char *dir3 = SANDBOX_DIR  "/remove3";
    const sys_char *file1 = SANDBOX_DIR "/remove_file";
    const sys_char *doesnotexist = SANDBOX_DIR "/doesnotexist";

    fs::create_directory(dir1);
    fs::create_directory(dir2);
    fs::create_directory(dir3);
    fs::touch(file1);
    fs::touch(SANDBOX_DIR "/remove/notempty");

    fs::create_directory(SANDBOX_DIR "/remove/dir4");
    fs::create_directory(SANDBOX_DIR "/remove/dir5");
    fs::touch(SANDBOX_DIR "/remove/file3");
    fs::touch(SANDBOX_DIR "/remove/dir4/file4");

    assert_equal(fs::exists(dir1), 1); 
    assert_equal(fs::remove(dir1, &err), true); 
    assert_equal(fs::exists(dir1), 0); 

    // not empty
    assert_equal(fs::exists(dir2), 1); 
    assert_equal(fs::remove(dir2, &err), true);
    assert_equal(fs::exists(dir2), 0); 

    // not empty with subdirectories and more descendants
    assert_equal(fs::exists(dir3), 1); 
    assert_equal(fs::remove(dir3, &err), true);
    assert_equal(fs::exists(dir3), 0); 

    assert_equal(fs::exists(file1), 1); 
    assert_equal(fs::remove(file1, &err), true); 
    assert_equal(fs::exists(file1), 0); 

    // does not exist, still returns true
    assert_equal(fs::exists(doesnotexist), 0); 
    assert_equal(fs::remove(doesnotexist, &err), true); 
}

/*
define_test(iterator_test1)
{
    error err{};

    if (fs::fs_iterator it; true)
    if (defer { fs::free(&it); }; fs::init(&it, SANDBOX_DIR, &err))
    for (fs::fs_iterator_item *item = fs::_iterate(&it, fs::iterate_option::None, &err);
         item != nullptr;
         item = fs::_iterate(&it, fs::iterate_option::None, &err))
        printf("%x - %ws\n", (u32)item->type, item->path.c_str);

    assert_equal(err.error_code, 0);
}
*/

define_test(iterator_test)
{
    error err{};
    array<fs::path> descendants{};

    fs::create_directories(SANDBOX_DIR "/it/dir1");
    fs::create_directories(SANDBOX_DIR "/it/dir2/dir3");
    fs::touch(SANDBOX_DIR "/it/file1");
    fs::touch(SANDBOX_DIR "/it/dir2/file2");

    for_path(item, SYS_CHAR("it"), fs::iterate_option::None, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::path_set(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 3);

    assert_equal_str(descendants[0], SYS_CHAR("dir1"));
    assert_equal_str(descendants[1], SYS_CHAR("dir2"));
    assert_equal_str(descendants[2], SYS_CHAR("file1"));

    assert_equal(err.error_code, 0);

    free<true>(&descendants);
}

define_test(iterator_type_filter_test)
{
    error err{};
    array<fs::path> descendants{};

    fs::create_directories(SANDBOX_DIR "/it2/dir1");
    fs::create_directories(SANDBOX_DIR "/it2/dir2/dir3");
    fs::touch(SANDBOX_DIR "/it2/file1");
    fs::touch(SANDBOX_DIR "/it2/dir2/file2");

    // see "for_path_files"
    for_path_files(item, SYS_CHAR("it2"), fs::iterate_option::None, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::path_set(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 1);
    assert_equal_str(descendants[0], SYS_CHAR("file1"));
    assert_equal(err.error_code, 0);

    free_values(&descendants);
    clear(&descendants);

    // directories
    for_path_type(fs::filesystem_type::Directory, item, SYS_CHAR("it2"), fs::iterate_option::None, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::path_set(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 2);
    assert_equal_str(descendants[0], SYS_CHAR("dir1"));
    assert_equal_str(descendants[1], SYS_CHAR("dir2"));
    assert_equal(err.error_code, 0);

    free<true>(&descendants);
}

define_test(iterator_fullpath_test)
{
    error err{};
    array<fs::path> descendants{};

    fs::create_directories(SANDBOX_DIR "/it3/dir1");
    fs::create_directories(SANDBOX_DIR "/it3/dir2/dir3");
    fs::touch(SANDBOX_DIR "/it3/file1");
    fs::touch(SANDBOX_DIR "/it3/dir2/file2");

    for_path(item, SYS_CHAR("it3"), fs::iterate_option::Fullpaths, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::path_set(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 3);

#if Windows
    assert_equal_str(descendants[0], SANDBOX_DIR "\\it3\\dir1");
    assert_equal_str(descendants[1], SANDBOX_DIR "\\it3\\dir2");
    assert_equal_str(descendants[2], SANDBOX_DIR "\\it3\\file1");
#else
    assert_equal_str(descendants[0], SANDBOX_DIR "/it3/dir1");
    assert_equal_str(descendants[1], SANDBOX_DIR "/it3/dir2");
    assert_equal_str(descendants[2], SANDBOX_DIR "/it3/file1");
#endif

    assert_equal(err.error_code, 0);

    free<true>(&descendants);
}

define_test(recursive_iterator_test)
{
    error err{};
    array<fs::path> descendants{};

    fs::create_directories(SANDBOX_DIR "/rit/dir1/dir2/dir3/dir4");
    fs::touch(SANDBOX_DIR "/rit/dir1/dir2/dir3/dir4/file");

    fs::iterate_option opts = fs::iterate_option::None;//Fullpaths;

#if Windows
    const sys_char *rit = SANDBOX_DIR "\\rit";
#else
    const sys_char *rit = SANDBOX_DIR "/rit";
#endif

    for_recursive_path(item, rit, opts, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::path_set(cp, item->path);
    }

    assert_equal(descendants.size, 5);

    // order here is fixed and the results must look like this.
    // use ChildrenFirst for reverse order (see next test)
#if Windows
    assert_equal_str(descendants[0], SANDBOX_DIR "\\rit\\dir1");
    assert_equal_str(descendants[1], SANDBOX_DIR "\\rit\\dir1\\dir2");
    assert_equal_str(descendants[2], SANDBOX_DIR "\\rit\\dir1\\dir2\\dir3");
    assert_equal_str(descendants[3], SANDBOX_DIR "\\rit\\dir1\\dir2\\dir3\\dir4");
    assert_equal_str(descendants[4], SANDBOX_DIR "\\rit\\dir1\\dir2\\dir3\\dir4\\file");
#else
    assert_equal_str(descendants[0], SANDBOX_DIR "/rit/dir1");
    assert_equal_str(descendants[1], SANDBOX_DIR "/rit/dir1/dir2");
    assert_equal_str(descendants[2], SANDBOX_DIR "/rit/dir1/dir2/dir3");
    assert_equal_str(descendants[3], SANDBOX_DIR "/rit/dir1/dir2/dir3/dir4");
    assert_equal_str(descendants[4], SANDBOX_DIR "/rit/dir1/dir2/dir3/dir4/file");
#endif

    free<true>(&descendants);
}

define_test(recursive_iterator_children_first_test)
{
    error err{};

    fs::create_directories(SANDBOX_DIR "/cf/dir1/dir2/dir3/dir4");
    fs::touch(SANDBOX_DIR "/cf/dir1/dir2/dir3/dir4/file");

    array<fs::path> descendants{};

    fs::iterate_option opts = fs::iterate_option::Fullpaths | fs::iterate_option::ChildrenFirst;

    for_recursive_path(item, "cf", opts, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::path_set(cp, item->path);
    }

    assert_equal(descendants.size, 5);

    // order here is fixed and the results must look like this
#if Windows
    assert_equal_str(descendants[0], SANDBOX_DIR "\\cf\\dir1\\dir2\\dir3\\dir4\\file");
    assert_equal_str(descendants[1], SANDBOX_DIR "\\cf\\dir1\\dir2\\dir3\\dir4");
    assert_equal_str(descendants[2], SANDBOX_DIR "\\cf\\dir1\\dir2\\dir3");
    assert_equal_str(descendants[3], SANDBOX_DIR "\\cf\\dir1\\dir2");
    assert_equal_str(descendants[4], SANDBOX_DIR "\\cf\\dir1");
#else
    assert_equal_str(descendants[0], SANDBOX_DIR "/cf/dir1/dir2/dir3/dir4/file");
    assert_equal_str(descendants[1], SANDBOX_DIR "/cf/dir1/dir2/dir3/dir4");
    assert_equal_str(descendants[2], SANDBOX_DIR "/cf/dir1/dir2/dir3");
    assert_equal_str(descendants[3], SANDBOX_DIR "/cf/dir1/dir2");
    assert_equal_str(descendants[4], SANDBOX_DIR "/cf/dir1");
#endif

    free<true>(&descendants);
}

define_test(recursive_iterator_symlink_test)
{
    error err{};
    array<fs::path> descendants{};

    fs::create_directories(SANDBOX_DIR "/rit_sym/dir1");
    fs::create_directories(SANDBOX_DIR "/rit_sym/dir2/dir3");
    fs::create_symlink(SANDBOX_DIR "/rit_sym/dir2", SANDBOX_DIR "/rit_sym/symlink");
    fs::touch(SANDBOX_DIR "/rit_sym/file1");
    fs::touch(SANDBOX_DIR "/rit_sym/dir2/file2");

    fs::iterate_option opts = fs::iterate_option::FollowSymlinks;

    for_recursive_path(item, "rit_sym", opts, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::path_set(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);
    assert_equal(descendants.size, 8);

#if Windows
    assert_equal_str(descendants[0], SYS_CHAR("rit_sym\\dir1"));
    assert_equal_str(descendants[1], SYS_CHAR("rit_sym\\dir2"));
    assert_equal_str(descendants[2], SYS_CHAR("rit_sym\\file1"));
    assert_equal_str(descendants[3], SYS_CHAR("rit_sym\\symlink"));
    assert_equal_str(descendants[4], SYS_CHAR("rit_sym\\dir2\\dir3"));
    assert_equal_str(descendants[5], SYS_CHAR("rit_sym\\dir2\\file2"));
    assert_equal_str(descendants[6], SYS_CHAR("rit_sym\\symlink\\dir3"));
    assert_equal_str(descendants[7], SYS_CHAR("rit_sym\\symlink\\file2"));
#else
    assert_equal_str(descendants[0], SYS_CHAR("rit_sym/dir1"));
    assert_equal_str(descendants[1], SYS_CHAR("rit_sym/dir2"));
    assert_equal_str(descendants[2], SYS_CHAR("rit_sym/file1"));
    assert_equal_str(descendants[3], SYS_CHAR("rit_sym/symlink"));
    assert_equal_str(descendants[4], SYS_CHAR("rit_sym/dir2/dir3"));
    assert_equal_str(descendants[5], SYS_CHAR("rit_sym/dir2/file2"));
    assert_equal_str(descendants[6], SYS_CHAR("rit_sym/symlink/dir3"));
    assert_equal_str(descendants[7], SYS_CHAR("rit_sym/symlink/file2"));
#endif

    free<true>(&descendants);
}

define_test(recursive_iterator_type_filter_test)
{
    error err{};
    array<fs::path> descendants{};

    fs::create_directories(SANDBOX_DIR "/rit_filter/dir1");
    fs::create_directories(SANDBOX_DIR "/rit_filter/dir2/dir3");
    fs::touch(SANDBOX_DIR "/rit_filter/file1");
    fs::touch(SANDBOX_DIR "/rit_filter/dir2/file2");

    for_recursive_path_type(fs::filesystem_type::File, item, "rit_filter", fs::iterate_option::None, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::path_set(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 2);

#if Windows
    assert_equal_str(descendants[0], SYS_CHAR("rit_filter\\file1"));
    assert_equal_str(descendants[1], SYS_CHAR("rit_filter\\dir2\\file2"));
#else
    assert_equal_str(descendants[0], SYS_CHAR("rit_filter/file1"));
    assert_equal_str(descendants[1], SYS_CHAR("rit_filter/dir2/file2"));
#endif

    free_values(&descendants);
    clear(&descendants);

    // directories
    for_recursive_path_directories(item, "rit_filter", fs::iterate_option::None, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::path_set(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 3);

#if Windows
    assert_equal_str(descendants[0], SYS_CHAR("rit_filter\\dir1"));
    assert_equal_str(descendants[1], SYS_CHAR("rit_filter\\dir2"));
    assert_equal_str(descendants[2], SYS_CHAR("rit_filter\\dir2\\dir3"));
#else
    assert_equal_str(descendants[0], SYS_CHAR("rit_filter/dir1"));
    assert_equal_str(descendants[1], SYS_CHAR("rit_filter/dir2"));
    assert_equal_str(descendants[2], SYS_CHAR("rit_filter/dir2/dir3"));
#endif

    free<true>(&descendants);
}

define_test(get_children_names_gets_directory_children_names)
{
    error err{};
    array<fs::path> children{};

    fs::create_directories(SANDBOX_DIR "/get_children/dir1");
    fs::create_directories(SANDBOX_DIR "/get_children/dir2/dir3");
    fs::touch(SANDBOX_DIR "/get_children/file1");
    fs::touch(SANDBOX_DIR "/get_children/dir2/file2");

    s64 count = fs::get_children_names(SANDBOX_DIR "/get_children", &children, &err);

    assert_equal(err.error_code, 0);
    assert_equal(count, 3);
    assert_equal(children.size, 3);

    sort(children.data, children.size, path_comparer);

    assert_equal_str(children[0], SYS_CHAR("dir1"));
    assert_equal_str(children[1], SYS_CHAR("dir2"));
    assert_equal_str(children[2], SYS_CHAR("file1"));

    free<true>(&children);
}

define_test(get_children_names_returns_minus_one_on_error)
{
    error err{};
    array<fs::path> children{};

#if Windows
    s64 count = fs::get_children_names(uR"(C:\System Volume Information)", &children, &err);
#else
    s64 count = fs::get_children_names(SANDBOX_TEST_DIR_NO_PERMISSION, &children, &err);
#endif

    assert_equal(count, -1);
    assert_equal(children.size, 0);

#if Linux
    assert_equal(err.error_code, EACCES);
#endif

    free<true>(&children);
}

define_test(get_children_fullpaths_gets_directory_children_fullpaths)
{
    error err{};
    array<fs::path> children{};

    fs::create_directories(SANDBOX_DIR "/get_children2/dir1");
    fs::create_directories(SANDBOX_DIR "/get_children2/dir2/dir3");
    fs::touch(SANDBOX_DIR "/get_children2/file1");
    fs::touch(SANDBOX_DIR "/get_children2/dir2/file2");

    s64 count = fs::get_children_fullpaths(SANDBOX_DIR "/get_children2", &children, &err);

    assert_equal(err.error_code, 0);
    assert_equal(count, 3);
    assert_equal(children.size, 3);

    sort(children.data, children.size, path_comparer);

#if Windows
    // note: even though input is SANDBOX_DIR "/get_children", the output
    // uses backslashes
    assert_equal_str(children[0], SANDBOX_DIR "\\get_children2\\dir1");
    assert_equal_str(children[1], SANDBOX_DIR "\\get_children2\\dir2");
    assert_equal_str(children[2], SANDBOX_DIR "\\get_children2\\file1");
#else
    assert_equal_str(children[0], SANDBOX_DIR "/get_children2/dir1");
    assert_equal_str(children[1], SANDBOX_DIR "/get_children2/dir2");
    assert_equal_str(children[2], SANDBOX_DIR "/get_children2/file1");
#endif

    free<true>(&children);
}

define_test(get_all_descendants_paths_gets_all_descendants_paths_relative_to_given_directory)
{
    error err{};
    array<fs::path> all_descendants{};

    fs::create_directories(SANDBOX_DIR "/get_all_descendants/dir1");
    fs::create_directories(SANDBOX_DIR "/get_all_descendants/dir2/dir3");
    fs::touch(SANDBOX_DIR "/get_all_descendants/file1");
    fs::touch(SANDBOX_DIR "/get_all_descendants/dir2/file2");

    // not absolute path, so paths are relative to this parameter
    s64 count = fs::get_all_descendants_paths("get_all_descendants", &all_descendants, &err);

    assert_equal(err.error_code, 0);
    assert_equal(count, 5);
    assert_equal(all_descendants.size, 5);

    sort(all_descendants.data, all_descendants.size, path_comparer);

#if Windows
    assert_equal_str(all_descendants[0], SYS_CHAR("get_all_descendants\\dir1"));
    assert_equal_str(all_descendants[1], SYS_CHAR("get_all_descendants\\dir2"));
    assert_equal_str(all_descendants[2], SYS_CHAR("get_all_descendants\\file1"));
    assert_equal_str(all_descendants[3], SYS_CHAR("get_all_descendants\\dir2\\dir3"));
    assert_equal_str(all_descendants[4], SYS_CHAR("get_all_descendants\\dir2\\file2"));
#else
    assert_equal_str(all_descendants[0], SYS_CHAR("get_all_descendants/dir1"));
    assert_equal_str(all_descendants[1], SYS_CHAR("get_all_descendants/dir2"));
    assert_equal_str(all_descendants[2], SYS_CHAR("get_all_descendants/file1"));
    assert_equal_str(all_descendants[3], SYS_CHAR("get_all_descendants/dir2/dir3"));
    assert_equal_str(all_descendants[4], SYS_CHAR("get_all_descendants/dir2/file2"));
#endif

    free<true>(&all_descendants);
}

define_test(get_all_descendants_fullpaths_gets_all_descendants_paths)
{
    error err{};
    array<fs::path> all_descendants{};

    fs::create_directories(SANDBOX_DIR "/get_all_descendants2/dir1");
    fs::create_directories(SANDBOX_DIR "/get_all_descendants2/dir2/dir3");
    fs::touch(SANDBOX_DIR "/get_all_descendants2/file1");
    fs::touch(SANDBOX_DIR "/get_all_descendants2/dir2/file2");

    s64 count = fs::get_all_descendants_fullpaths("get_all_descendants", &all_descendants, &err);

    assert_equal(err.error_code, 0);
    assert_equal(count, 5);
    assert_equal(all_descendants.size, 5);

    sort(all_descendants.data, all_descendants.size, path_comparer);

#if Windows
    assert_equal_str(all_descendants[0], SANDBOX_DIR "\\get_all_descendants\\dir1");
    assert_equal_str(all_descendants[1], SANDBOX_DIR "\\get_all_descendants\\dir2");
    assert_equal_str(all_descendants[2], SANDBOX_DIR "\\get_all_descendants\\file1");
    assert_equal_str(all_descendants[3], SANDBOX_DIR "\\get_all_descendants\\dir2\\dir3");
    assert_equal_str(all_descendants[4], SANDBOX_DIR "\\get_all_descendants\\dir2\\file2");
#else
    assert_equal_str(all_descendants[0], SANDBOX_DIR "/get_all_descendants/dir1");
    assert_equal_str(all_descendants[1], SANDBOX_DIR "/get_all_descendants/dir2");
    assert_equal_str(all_descendants[2], SANDBOX_DIR "/get_all_descendants/file1");
    assert_equal_str(all_descendants[3], SANDBOX_DIR "/get_all_descendants/dir2/dir3");
    assert_equal_str(all_descendants[4], SANDBOX_DIR "/get_all_descendants/dir2/file2");
#endif

    free<true>(&all_descendants);
}

define_test(get_children_count_gets_children_count)
{
    error err{};

    fs::create_directories(SANDBOX_DIR "/get_children_count/dir1");
    fs::create_directories(SANDBOX_DIR "/get_children_count/dir2/dir3");
    fs::touch(SANDBOX_DIR "/get_children_count/file1");
    fs::touch(SANDBOX_DIR "/get_children_count/dir2/file2");

    assert_equal(fs::get_children_count(SANDBOX_DIR "/get_children_count", &err), 3);
    assert_equal(fs::get_children_count(SANDBOX_DIR "/doesnotexist", &err), -1);

#if Windows
    assert_equal(err.error_code, ERROR_FILE_NOT_FOUND);
#else
    assert_equal(err.error_code, ENOENT);
#endif

#if Windows
    assert_equal(fs::get_children_count(uR"(C:\System Volume Information)", &err), -1);
    assert_equal(err.error_code, ERROR_ACCESS_DENIED);
#else
    assert_equal(fs::get_children_count(SANDBOX_TEST_DIR_NO_PERMISSION, &err), -1);
    assert_equal(err.error_code, EACCES);
#endif
}

define_test(get_descendant_count_gets_children_count)
{
    error err{};

    fs::create_directories(SANDBOX_DIR "/get_descendant_count/dir1");
    fs::create_directories(SANDBOX_DIR "/get_descendant_count/dir2/dir3");
    fs::touch(SANDBOX_DIR "/get_descendant_count/file1");
    fs::touch(SANDBOX_DIR "/get_descendant_count/dir2/file2");

    assert_equal(fs::get_descendant_count(SANDBOX_DIR "/get_descendant_count", &err), 5);
    assert_equal(fs::get_descendant_count(SANDBOX_DIR "/doesnotexist", &err), -1);

#if Linux
    assert_equal(err.error_code, ENOENT);
#endif

#if Windows
    assert_equal(fs::get_descendant_count(uR"(C:\System Volume Information)", &err), -1);
    assert_equal(err.error_code, ERROR_ACCESS_DENIED);
#else
    assert_equal(fs::get_descendant_count(SANDBOX_TEST_DIR_NO_PERMISSION, &err), -1);
    assert_equal(err.error_code, EACCES);
#endif
}

define_test(get_home_path_gets_home_path)
{
    // const char *actual = "/home/user";
    fs::path p{};
    error err{};

    assert_equal(fs::get_home_path(&p, &err), true);
    assert_equal(err.error_code, 0);

    // obviously this wont work on all systems
    // assert_equal_str(p, actual);

    fs::free(&p);
}

define_test(get_executable_path_gets_executable_path)
{
    // const char *actual = "/home/user/dev/git/fs/bin/tests/path_tests";
    fs::path p{};
    error err{};

    assert_equal(fs::get_executable_path(&p, &err), true);

#if Windows
    assert_equal_str(fs::filename(&p), SYS_CHAR("path_tests.exe"));
#else
    assert_equal_str(fs::filename(&p), SYS_CHAR("path_tests"));
#endif

    // obviously this wont work on all systems
    // assert_equal_str(p, actual);

    fs::free(&p);
}

define_test(get_preference_path_gets_preference_path)
{
    fs::path pref_path{};
    error err{};

    // on Windows, the drive letter might be omitted.

    // on Windows, this might be C:\Users\<user>\AppData\Roaming
    // on Linux,   this might be /home/<user>/.local/share
    assert_equal(fs::get_preference_path(&pref_path, &err), true);

    // on Windows, this might be C:\Users\<user>\AppData\Roaming\path_tests
    // on Linux,   this might be /home/<user>/.local/share/path_tests
    assert_equal(fs::get_preference_path(&pref_path, "path_tests", &err), true);

    // on Windows, this might be C:\Users\<user>\AppData\Roaming\org\path_tests
    // on Linux,   this might be /home/<user>/.local/share/org/path_tests
    assert_equal(fs::get_preference_path(&pref_path, "path_tests", "org", &err), true);

    fs::free(&pref_path);
}

define_test(get_temporary_path_gets_temporary_path)
{
    fs::path tmp_path{};
    error err{};

    assert_equal(fs::get_temporary_path(&tmp_path, &err), true);

#if Windows
    // could still be different if variables are defined or tests are run as SYSTEM (why?)
    // assert_equal_str(tmp_path, SYS_CHAR(R"(C:\TEMP)"));
#else
    // could still be different if variables are defined
    assert_equal_str(tmp_path, SYS_CHAR("/tmp"));
#endif

    fs::free(&tmp_path);
}

static fs::path old_current_dir;

#if Windows
HANDLE _pipe_handle = INVALID_HANDLE_VALUE;

void create_test_pipe()
{
    _pipe_handle = CreateNamedPipe((const sys_native_char*)SANDBOX_TEST_PIPE,
                                   PIPE_ACCESS_INBOUND,
                                   PIPE_TYPE_BYTE | PIPE_WAIT,
                                   1,
                                   4096,
                                   4096,
                                   0,
                                   nullptr);

    assert(_pipe_handle != INVALID_HANDLE_VALUE);
}

void destroy_test_pipe()
{
    if (_pipe_handle != INVALID_HANDLE_VALUE)
        CloseHandle(_pipe_handle);

    _pipe_handle = INVALID_HANDLE_VALUE;
}
#endif

void _setup()
{
    fs::get_current_path(&old_current_dir);
#if Windows
    fs::remove(SANDBOX_DIR);

    fs::create_directories(SANDBOX_DIR);
    fs::create_directories(SANDBOX_TEST_DIR);
    fs::touch(SANDBOX_TEST_FILE);

    fs::create_symlink(SANDBOX_TEST_FILE, SANDBOX_TEST_SYMLINK);
    fs::create_symlink(SANDBOX_DIR "/symlink_dest", SANDBOX_TEST_SYMLINK_NO_TARGET);

    fs::create_directories(SANDBOX_TEST_DIR2);
    fs::touch(SANDBOX_TEST_FILE2);

    create_test_pipe();

    fs::create_directories(SANDBOX_TEST_DIR_NO_PERMISSION);
    fs::touch(SANDBOX_TEST_FILE_IN_NOPERM_DIR);
    // stdfs::permissions(SANDBOX_TEST_DIR_NO_PERMISSION, stdfs::perms::none);

    fs::set_current_path(SANDBOX_DIR);
#else
    ignore_return_value umask(0); // if this is not set to 0 mkdir might not set correct permissions
    ignore_return_value mkdir(SANDBOX_DIR, 0777);
    ignore_return_value mkdir(SANDBOX_TEST_DIR, 0777);
    FILE *f = fopen(SANDBOX_TEST_FILE, "w");
    assert(f != nullptr);
    ignore_return_value fclose(f);
    ignore_return_value symlink(SANDBOX_TEST_FILE, SANDBOX_TEST_SYMLINK);
    ignore_return_value symlink(SANDBOX_DIR "/symlink_dest", SANDBOX_TEST_SYMLINK_NO_TARGET);
 
    ignore_return_value mknod(SANDBOX_TEST_PIPE, S_IFIFO | 0644, 0);

    ignore_return_value mkdir(SANDBOX_TEST_DIR2, 0777);
    f = fopen(SANDBOX_TEST_FILE2, "w"); assert(f != nullptr); fclose(f);
    ignore_return_value mkdir(SANDBOX_TEST_DIR_NO_PERMISSION, 0777);
    f = fopen(SANDBOX_TEST_FILE_IN_NOPERM_DIR, "w"); assert(f != nullptr); fclose(f);
    ignore_return_value chmod(SANDBOX_TEST_DIR_NO_PERMISSION, 0000);

    fs::set_current_path(SANDBOX_DIR);
#endif
}

void _cleanup()
{
#if Windows
    destroy_test_pipe();

    // stdfs::permissions(SANDBOX_TEST_DIR_NO_PERMISSION, stdfs::perms::all);
    fs::set_current_path(&old_current_dir);

    error err{};

    if (!fs::remove_directory(SANDBOX_DIR, &err))
        fprintf(stderr, "ERROR: could not remove directory %ws. Error code %d:\n%s\n", (wchar_t*)SANDBOX_DIR, err.error_code, (char*)err.what);

    fs::free(&old_current_dir);
#else
    ignore_return_value chmod(SANDBOX_TEST_DIR_NO_PERMISSION, 0777);
    fs::set_current_path(&old_current_dir);
    error err{};
    if (!fs::remove_directory(SANDBOX_DIR, &err))
        fprintf(stderr, "ERROR: could not remove directory %s. Error code %d:\n%s\n", SANDBOX_DIR, err.error_code, err.what);

    fs::free(&old_current_dir);
#endif
}

define_test_main(_setup, _cleanup);
