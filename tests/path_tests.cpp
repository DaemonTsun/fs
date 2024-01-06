
#include <t1/t1.hpp>

#if Windows
#else
#include <sys/stat.h>
#endif
#include <unistd.h>

#include "shl/string.hpp"
#include "shl/time.hpp" // for sleep
#include "shl/platform.hpp"
#include "shl/print.hpp"
#include "shl/sort.hpp"
#include "fs/path.hpp"

int path_comparer(const fs::path *a, const fs::path *b)
{
    return compare_strings(to_const_string(a), to_const_string(b));
}

#define assert_equal_str(STR1, STR2)\
    assert_equal(compare_strings(to_const_string(STR1), STR2), 0)

// TODO: windows tests...

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

define_test(set_path_sets_path)
{
    fs::path pth{};
    
    fs::set_path(&pth, "/abc"); assert_equal_str(pth.data, "/abc");
    fs::set_path(&pth, "/abc/def"); assert_equal_str(pth.data, "/abc/def");
    fs::set_path(&pth, L"/abc///:def"); assert_equal_str(pth.data, "/abc///:def");
    fs::set_path(&pth, L"C:/abc///:def"); assert_equal_str(pth.data, "C:/abc///:def");

    fs::free(&pth);
}

define_test(literal_path_sets_path)
{
    fs::path pth = "/abc/def"_path;
    
    assert_equal_str(pth.data, "/abc/def");

    fs::free(&pth);
}

define_test(is_fs_type_tests)
{
    fs::path p{};

    // directory
    fs::set_path(&p, SANDBOX_TEST_DIR);
    assert_equal(fs::is_file(&p),         false);
    assert_equal(fs::is_block_device(&p), false);
    assert_equal(fs::is_symlink(&p),      false);
    assert_equal(fs::is_pipe(&p),         false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    true);

    // file
    fs::set_path(&p, SANDBOX_TEST_FILE);
    assert_equal(fs::is_file(&p),         true);
    assert_equal(fs::is_block_device(&p), false);
    assert_equal(fs::is_symlink(&p),      false);
    assert_equal(fs::is_pipe(&p),         false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    false);

    assert_equal(fs::is_file(SANDBOX_TEST_FILE), true);

    // symlink
    fs::set_path(&p, SANDBOX_TEST_SYMLINK);
    assert_equal(fs::is_file(&p),         true);
    assert_equal(fs::is_file(&p, false),  false);
    assert_equal(fs::is_block_device(&p), false);
    assert_equal(fs::is_symlink(&p),      true);
    assert_equal(fs::is_pipe(&p),         false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    false);

    fs::set_path(&p, SANDBOX_TEST_SYMLINK_NO_TARGET);
    assert_equal(fs::is_file(&p),         false);
    assert_equal(fs::is_file(&p, false),  false);
    assert_equal(fs::is_block_device(&p), false);
    assert_equal(fs::is_symlink(&p),      true);
    assert_equal(fs::is_pipe(&p),         false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    false);

    // pipe
    fs::set_path(&p, SANDBOX_TEST_PIPE);
    assert_equal(fs::is_file(&p),         false);
    assert_equal(fs::is_block_device(&p), false);
    assert_equal(fs::is_symlink(&p),      false);
    assert_equal(fs::is_pipe(&p),         true);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    false);

    // block device
    fs::set_path(&p, "/dev/sda");
    assert_equal(fs::is_file(&p),         false);
    assert_equal(fs::is_block_device(&p), true);
    assert_equal(fs::is_symlink(&p),      false);
    assert_equal(fs::is_pipe(&p),         false);
    assert_equal(fs::is_socket(&p),       false);
    assert_equal(fs::is_directory(&p),    false);

    fs::free(&p);
}

define_test(get_filesystem_type_test)
{
    fs::filesystem_type fstype;
    fs::fs_error err{};

    // directory
    const char *p = nullptr;

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

#if Linux
    assert_equal(err.error_code, ENOENT);
#endif

    assert_equal(fs::get_filesystem_type(p, &fstype, false, &err), true);
    assert_equal(fstype, fs::filesystem_type::Symlink);

    p = SANDBOX_TEST_PIPE;
    assert_equal(fs::get_filesystem_type(p, &fstype), true);
    assert_equal(fstype, fs::filesystem_type::Pipe);

    assert_equal(fs::get_filesystem_type(SANDBOX_DIR "/doesnotexist", &fstype, false, &err), false);

#if Linux
    assert_equal(err.error_code, ENOENT);
#endif
}

define_test(get_permissions_gets_permissions)
{
    fs::permission perms;
    fs::fs_error err{};

    const char *p = nullptr;

    p = SANDBOX_TEST_DIR;
    assert_equal(fs::get_permissions(p, &perms), true);
    assert_equal(perms, fs::permission::All);

    p = SANDBOX_TEST_DIR_NO_PERMISSION;
    assert_equal(fs::get_permissions(p, &perms), true);
    assert_equal(perms, fs::permission::None);

    p = SANDBOX_DIR "/doesnotexist";
    assert_equal(fs::get_permissions(p, &perms, true, &err), false);

#if Linux
    assert_equal(err.error_code, ENOENT);
#endif
}

define_test(set_permissions_sets_permissions)
{
    fs::fs_error err{};
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

define_test(exists_returns_true_if_directory_exists)
{
    fs::path p{};
    fs::set_path(&p, SANDBOX_TEST_DIR);

    assert_equal(fs::exists(&p), 1);

    fs::free(&p);
}

define_test(exists_returns_true_if_file_exists)
{
    fs::path p{};
    fs::set_path(&p, SANDBOX_TEST_FILE);

    assert_equal(fs::exists(&p), 1);

    // testing other inputs
    assert_equal(fs::exists(SANDBOX_TEST_FILE), 1);
    assert_equal(fs::exists("/tmp/sandbox/file"), 1);
    assert_equal(fs::exists(L"/tmp/sandbox/file"), 1);

    fs::free(&p);
}

define_test(exists_checks_if_symlink_exists)
{
    fs::path p{};
    fs::set_path(&p, SANDBOX_TEST_SYMLINK);

    assert_equal(fs::exists(&p), 1);

    fs::set_path(&p, SANDBOX_TEST_SYMLINK_NO_TARGET);
    assert_equal(fs::exists(&p, false), 1);

    fs::free(&p);
}

define_test(exists_returns_false_when_not_exists)
{
    fs::path p{};
    fs::fs_error err;
    fs::set_path(&p, SANDBOX_TEST_DIR "/abc");

    assert_equal(fs::exists(&p, true, &err), 0);

    fs::free(&p);
}

define_test(exists_yields_error_when_unauthorized)
{
    fs::path p{};
    fs::fs_error err;
    fs::set_path(&p, "/root/abc");

    assert_equal(fs::exists(&p, true, &err), -1);

#if Linux
    assert_equal(err.error_code, EACCES);
#endif

    fs::free(&p);
}

define_test(exists_checks_if_symlink_target_exists)
{
    fs::path p{};
    fs::set_path(&p, SANDBOX_TEST_SYMLINK_NO_TARGET);
    // target doesnt exist, this checks if symlink exists
    assert_equal(fs::exists(&p, false), 1);

    // this checks if target exists
    assert_equal(fs::exists(&p, true), 0);

    fs::free(&p);
}

define_test(is_absolute_returns_true_if_path_is_absoltue)
{
#if Windows
    fs::path p = R"=(C:\Windows\notepad.exe)="_path;
#else
    fs::path p = "/etc/passwd"_path;
#endif

    assert_equal(fs::is_absolute(&p), true);

    fs::free(&p);
}

define_test(is_absolute_returns_false_if_path_is_not_absolute)
{
#if Windows
    fs::path p = R"=(..\notepad.exe)="_path;
#else
    fs::path p = "../passwd"_path;
#endif

    assert_equal(fs::is_absolute(&p), false);

    fs::free(&p);
}

define_test(is_relative_returns_true_if_path_is_relative)
{
#if Windows
    fs::path p = R"=(..\notepad.exe)="_path;
#else
    fs::path p = "../passwd"_path;
#endif

    assert_equal(fs::is_relative(&p), true);

    fs::free(&p);
}

define_test(is_relative_returns_false_if_path_is_not_relative)
{
#if Windows
    fs::path p = R"=(C:\Windows\notepad.exe)="_path;
#else
    fs::path p = "/etc/passwd"_path;
#endif

    assert_equal(fs::is_relative(&p), false);

    fs::free(&p);
}

define_test(are_equivalent_returns_true_for_same_path)
{
#if Windows
    // fs::path p = R"=(C:\Windows\notepad.exe)="_path;
#else
    fs::path p1 = "/etc/passwd"_path;
    fs::path p2 = "/etc/passwd"_path;
#endif

    assert_equal(fs::are_equivalent(&p1, &p2), true);

    fs::free(&p1);
    fs::free(&p2);
}

define_test(are_equivalent_returns_true_for_equivalent_paths)
{
#if Windows
    // fs::path p = R"=(C:\Windows\notepad.exe)="_path;
#else
    fs::path p1 = "/etc/passwd"_path;
    fs::path p2 = "/etc/../etc/passwd"_path;
#endif

    assert_equal(fs::are_equivalent(&p1, &p2), true);

    fs::free(&p1);
    fs::free(&p2);
}

define_test(are_equivalent_returns_false_for_different_existing_paths)
{
#if Windows
    // fs::path p = R"=(C:\Windows\notepad.exe)="_path;
#else
    fs::path p1 = "/etc/passwd"_path;
    fs::path p2 = "/etc/profile"_path;
#endif

    assert_equal(fs::are_equivalent(&p1, &p2), false);

    fs::free(&p1);
    fs::free(&p2);
}

define_test(are_equivalent_returns_false_if_only_one_path_doesnt_exist)
{
#if Windows
    // fs::path p = R"=(C:\Windows\notepad.exe)="_path;
#else
    fs::path p1 = "/etc/passwd"_path;
    fs::path p2 = "/etc/passwd2"_path;
#endif

    assert_equal(fs::are_equivalent(&p1, &p2), false);

    fs::free(&p1);
    fs::free(&p2);
}

define_test(filename_returns_the_filename)
{
    fs::path p{};

    fs::set_path(&p, "/foo/bar.txt");
    assert_equal_str(fs::filename(&p), "bar.txt");

    fs::set_path(&p, R"(C:\foo\bar.txt)");
#if Windows
    assert_equal_str(fs::filename(&p), "bar.txt");
#else
    // https://theboostcpplibraries.com/boost.filesystem-paths#ex.filesystem_05
    // not that we try to imitate boost or std::filesystem (which would be bad),
    // but interpreting paths of other systems is a bad idea as the default
    // behavior.
    assert_equal_str(fs::filename(&p), R"(C:\foo\bar.txt)");
#endif

    fs::set_path(&p, "/foo/.bar");
    assert_equal_str(fs::filename(&p), ".bar");

    fs::set_path(&p, R"(C:\foo\.bar)");
#if Windows
    assert_equal_str(fs::filename(&p), ".bar");
#else
    assert_equal_str(fs::filename(&p), R"(C:\foo\.bar)");
#endif

    fs::set_path(&p, "/foo/bar/");
    assert_equal_str(fs::filename(&p), "");

    fs::set_path(&p, R"(C:\foo\bar\)");
#if Windows
    assert_equal_str(fs::filename(&p), "");
#else
    assert_equal_str(fs::filename(&p), R"(C:\foo\bar\)");
#endif

    fs::set_path(&p, "/foo/.");
    assert_equal_str(fs::filename(&p), ".");

    fs::set_path(&p, R"(C:\foo\.)");
#if Windows
    assert_equal_str(fs::filename(&p), ".");
#else
    assert_equal_str(fs::filename(&p), R"(C:\foo\.)");
#endif

    fs::set_path(&p, "/foo/..");
    assert_equal_str(fs::filename(&p), "..");

    fs::set_path(&p, R"(C:\foo\..)");
#if Windows
    assert_equal_str(fs::filename(&p), "..");
#else
    assert_equal_str(fs::filename(&p), R"(C:\foo\..)");
#endif

    fs::set_path(&p, ".");
    assert_equal_str(fs::filename(&p), ".");

    fs::set_path(&p, "..");
    assert_equal_str(fs::filename(&p), "..");

    fs::set_path(&p, "/");
    assert_equal_str(fs::filename(&p), "");

    fs::set_path(&p, R"(C:\)");
#if Windows
    assert_equal_str(fs::filename(&p), "");
#else
    assert_equal_str(fs::filename(&p), R"(C:\)");
#endif

    fs::set_path(&p, "//host");
    assert_equal_str(fs::filename(&p), "host");

    fs::free(&p);
}

define_test(extension_returns_path_extension)
{
    fs::path p{};

    fs::set_path(&p, "/foo/bar.txt");
    assert_equal_str(fs::file_extension(&p), ".txt");

    fs::set_path(&p, R"(C:\foo\bar.txt)");
    assert_equal_str(fs::file_extension(&p), ".txt");

    fs::set_path(&p, "/foo/bar.");
    assert_equal_str(fs::file_extension(&p), ".");

    fs::set_path(&p, "/foo/bar");
    assert_equal_str(fs::file_extension(&p), "");

    fs::set_path(&p, "/foo/bar.txt/bar.cc");
    assert_equal_str(fs::file_extension(&p), ".cc");

    fs::set_path(&p, "/foo/bar.txt/bar.");
    assert_equal_str(fs::file_extension(&p), ".");

    fs::set_path(&p, "/foo/bar.txt/bar");
    assert_equal_str(fs::file_extension(&p), "");

    fs::set_path(&p, "/foo/.");
    assert_equal_str(fs::file_extension(&p), "");

    fs::set_path(&p, "/foo/..");
    assert_equal_str(fs::file_extension(&p), "");

    fs::set_path(&p, "/foo/.hidden");
    // this differs from std::filesystem
    assert_equal_str(fs::file_extension(&p), ".hidden");

    fs::set_path(&p, "/foo/..bar");
    assert_equal_str(fs::file_extension(&p), ".bar");

    fs::free(&p);
}

define_test(replace_filename_replaces_filename_of_path)
{
    fs::path p{};

    // filename is just the last part of a path, could be
    // a directory too.
    fs::set_path(&p, "/foo/bar");
    fs::replace_filename(&p, "xyz"_cs);
    assert_equal_str(p, "/foo/xyz");

    // shorter
    fs::set_path(&p, "/foo/bar");
    fs::replace_filename(&p, "g"_cs);
    assert_equal_str(p, "/foo/g");

    // longer
    fs::set_path(&p, "/foo/bar");
    fs::replace_filename(&p, "hello world. this is a long filename"_cs);
    assert_equal_str(p, "/foo/hello world. this is a long filename");

    // setting filename
    fs::set_path(&p, "/foo/");
    fs::replace_filename(&p, "abc"_cs);
    assert_equal_str(p, "/foo/abc");

    fs::set_path(&p, "/");
    fs::replace_filename(&p, "abc"_cs);
    assert_equal_str(p, "/abc");

    fs::set_path(&p, "abc");
    fs::replace_filename(&p, "xyz"_cs);
    assert_equal_str(p, "xyz");

    fs::set_path(&p, ".");
    fs::replace_filename(&p, "xyz"_cs);
    assert_equal_str(p, "xyz");

    // empty
    fs::set_path(&p, "");
    fs::replace_filename(&p, "abc"_cs);
    assert_equal_str(p, "abc");

    fs::free(&p);
}

define_test(parent_path_segment_returns_the_parent_path_segment)
{
    fs::path p{};

    fs::set_path(&p, "/foo/bar");
    assert_equal_str(fs::parent_path_segment(&p), "/foo");

    fs::set_path(&p, "/foo");
    assert_equal_str(fs::parent_path_segment(&p), "/");

    fs::set_path(&p, "/");
    assert_equal_str(fs::parent_path_segment(&p), "/");

    fs::set_path(&p, "/bar/");
    assert_equal_str(fs::parent_path_segment(&p), "/bar");

    fs::set_path(&p, ".");
    assert_equal_str(fs::parent_path_segment(&p), "");

    fs::free(&p);
}

define_test(path_segments_gets_the_segments_of_a_path)
{
    fs::path p{};
    array<fs::const_fs_string> segs{};

#if Windows
    // TODO: add tests
#else
    fs::set_path(&p, "/foo/bar");
    fs::path_segments(&p, &segs);

    assert_equal(segs.size, 3);
    assert_equal_str(segs[0], "/");
    assert_equal_str(segs[1], "foo");
    assert_equal_str(segs[2], "bar");

    // trailing slash is irrelevant
    fs::set_path(&p, "/foo/bar/");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 3);
    assert_equal_str(segs[0], "/");
    assert_equal_str(segs[1], "foo");
    assert_equal_str(segs[2], "bar");

    fs::set_path(&p, "/foo/file.txt");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 3);
    assert_equal_str(segs[0], "/");
    assert_equal_str(segs[1], "foo");
    assert_equal_str(segs[2], "file.txt");

    fs::set_path(&p, "/");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 1);
    assert_equal_str(segs[0], "/");

    fs::set_path(&p, "a/b/c");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 3);
    assert_equal_str(segs[0], "a");
    assert_equal_str(segs[1], "b");
    assert_equal_str(segs[2], "c");

    fs::set_path(&p, ".");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 1);
    assert_equal_str(segs[0], ".");

    fs::set_path(&p, "./../");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 2);
    assert_equal_str(segs[0], ".");
    assert_equal_str(segs[1], "..");

    fs::set_path(&p, "");
    fs::path_segments(&p, &segs);
    assert_equal(segs.size, 0);
#endif

    fs::free(&p);
    ::free(&segs);
}

define_test(root_returns_the_path_root)
{
    fs::path p{};

#if Windows
    // TODO: implement
#else
    fs::set_path(&p, "/foo/bar");
    assert_equal_str(fs::root(&p), "/");

    fs::set_path(&p, "/foo");
    assert_equal_str(fs::root(&p), "/");

    fs::set_path(&p, "/");
    assert_equal_str(fs::root(&p), "/");

    fs::set_path(&p, "bar");
    assert_equal_str(fs::root(&p), "");

    fs::set_path(&p, ".");
    assert_equal_str(fs::root(&p), "");

    fs::set_path(&p, "");
    assert_equal_str(fs::root(&p), "");

#endif

    fs::free(&p);
}

define_test(normalize_normalizes_path)
{
    fs::path p{};

#if Windows
    // TODO: add tests
#else
    fs::set_path(&p, "");
    fs::normalize(&p);
    assert_equal_str(p, "");

    fs::set_path(&p, "/");
    fs::normalize(&p);
    assert_equal_str(p, "/");

    fs::set_path(&p, "///");
    fs::normalize(&p);
    assert_equal_str(p, "/");

    fs::set_path(&p, "/a");
    fs::normalize(&p);
    assert_equal_str(p, "/a");

    fs::set_path(&p, "/a/////b");
    fs::normalize(&p);
    assert_equal_str(p, "/a/b");

    fs::set_path(&p, "/.");
    fs::normalize(&p);
    assert_equal_str(p, "/");

    fs::set_path(&p, "/..");
    fs::normalize(&p);
    assert_equal_str(p, "/");

    fs::set_path(&p, "/abc/./def");
    fs::normalize(&p);
    assert_equal_str(p, "/abc/def");

    fs::set_path(&p, "/abc/../def");
    fs::normalize(&p);
    assert_equal_str(p, "/def");

    fs::set_path(&p, "/abc/def/xyz/../../uvw");
    fs::normalize(&p);
    assert_equal_str(p, "/abc/uvw");

    fs::set_path(&p, "/abc/../def/./");
    fs::normalize(&p);
    assert_equal_str(p, "/def");

    fs::set_path(&p, "/abc/../def/../../../../");
    fs::normalize(&p);
    assert_equal_str(p, "/");

    fs::set_path(&p, "/abc/def/./.././");
    fs::normalize(&p);
    assert_equal_str(p, "/abc");

    // make sure other dots don't get changed
    fs::set_path(&p, "/abc/def./");
    fs::normalize(&p);
    assert_equal_str(p, "/abc/def.");

    fs::set_path(&p, "/abc/.def/");
    fs::normalize(&p);
    assert_equal_str(p, "/abc/.def");

    fs::set_path(&p, "/abc/def../");
    fs::normalize(&p);
    assert_equal_str(p, "/abc/def..");

    fs::set_path(&p, "/abc/..def/");
    fs::normalize(&p);
    assert_equal_str(p, "/abc/..def");

    fs::set_path(&p, "/abc/def.../");
    fs::normalize(&p);
    assert_equal_str(p, "/abc/def...");

    fs::set_path(&p, "/abc/...def/");
    fs::normalize(&p);
    assert_equal_str(p, "/abc/...def");

    // yes this is a valid filename
    fs::set_path(&p, "/abc/.../");
    fs::normalize(&p);
    assert_equal_str(p, "/abc/...");

    fs::set_path(&p, "/abc/..../");
    fs::normalize(&p);
    assert_equal_str(p, "/abc/....");

    // relative paths
    fs::set_path(&p, "..");
    fs::normalize(&p);
    assert_equal_str(p, "..");

    fs::set_path(&p, "../");
    fs::normalize(&p);
    assert_equal_str(p, "..");

    fs::set_path(&p, ".");
    fs::normalize(&p);
    assert_equal_str(p, ".");

    fs::set_path(&p, "./");
    fs::normalize(&p);
    assert_equal_str(p, ".");

    fs::set_path(&p, "...");
    fs::normalize(&p);
    assert_equal_str(p, "...");

    fs::set_path(&p, "....");
    fs::normalize(&p);
    assert_equal_str(p, "....");
#endif

    fs::free(&p);
}

define_test(longest_existing_path_returns_longest_existing_path)
{
    fs::path p{};
    fs::path longest{};

#if Windows
    // TODO: add tests
#else
    fs::set_path(&p, "/foo/bar");
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, "/");

    fs::set_path(&p, SANDBOX_DIR);
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SANDBOX_DIR);

    fs::set_path(&p, SANDBOX_TEST_FILE);
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, SANDBOX_TEST_FILE);

    fs::set_path(&p, "/tmp");
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, "/tmp");

    // let's hope abcxyz doesn't exist
    fs::set_path(&p, "/tmp/abcxyz");
    fs::longest_existing_path(&p, &longest);
    assert_equal_str(longest, "/tmp");
#endif

    fs::free(&p);
    fs::free(&longest);
}


define_test(absolute_path_gets_the_absolute_path)
{
    fs::path p{};
    fs::path absp{};

#if Windows
    // TODO: add tests
    assert_equal(true, false);
#else
    fs::set_path(&p, "/foo/bar");
    fs::absolute_path(&p, &absp);
    assert_equal_str(absp, "/foo/bar");

    fs::set_path(&p, "foo/bar");
    fs::absolute_path(&p, &absp);
    assert_equal_str(absp, SANDBOX_DIR "/foo/bar");

    // does not expand relative paths within the path,
    // use normalize(&p) to remove relative parts (. and ..) within the path,
    // or resolve the path and symlinks with canonical_path(&p, &canon).
    fs::set_path(&p, "/foo/bar/./abc/../def");
    fs::absolute_path(&p, &absp);
    assert_equal_str(absp, "/foo/bar/./abc/../def");

    fs::set_path(&p, "foo/bar/./abc/../def");
    fs::absolute_path(&p, &absp);
    assert_equal_str(absp, SANDBOX_DIR "/foo/bar/./abc/../def");


    // using the same pointer for input and output was removed because it's unintuitive.
    // an assert will now crash this.
    /*
    fs::set_path(&p, "foo/bar");
    fs::absolute_path(&p, &p);
    assert_equal_str(p, SANDBOX_DIR "/foo/bar");
    */
#endif

    fs::free(&absp);
    fs::free(&p);
}

define_test(canonical_path_gets_canonical_path)
{
    fs::path p{};
    fs::path canonp{};

#if Windows
    // TODO: add tests
    assert_equal(true, false);
#else
    fs::set_path(&p, SANDBOX_TEST_DIR);
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

    fs::set_path(&p, SANDBOX_TEST_DIR "/.");
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

    fs::set_path(&p, SANDBOX_TEST_DIR "/..");
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

    fs::set_path(&p, SANDBOX_TEST_DIR "/../");
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

    fs::set_path(&p, SANDBOX_TEST_DIR "/./../.");
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

    fs::set_path(&p, SANDBOX_TEST_DIR "/././.");
    assert_equal(fs::canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

    // fails on paths that don't exist
    fs::set_path(&p, "/tmp/abc/../def");
    assert_equal(fs::canonical_path(&p, &canonp), false);
#endif

    fs::free(&canonp);
    fs::free(&p);
}

define_test(weakly_canonical_path_gets_weakly_canonical_path)
{
    fs::path p{};
    fs::path canonp{};

#if Windows
    // TODO: add tests
    assert_equal(true, false);
#else
    fs::set_path(&p, SANDBOX_TEST_DIR);
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

    fs::set_path(&p, SANDBOX_TEST_DIR "/.");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

    fs::set_path(&p, SANDBOX_TEST_DIR "/..");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

    fs::set_path(&p, SANDBOX_TEST_DIR "/../");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

    fs::set_path(&p, SANDBOX_TEST_DIR "/./../.");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_DIR);

    fs::set_path(&p, SANDBOX_TEST_DIR "/././.");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, SANDBOX_TEST_DIR);

    // does not fail on paths that don't exist (assuming /tmp/abc does not exist)
    fs::set_path(&p, "/tmp/abc/../def");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, "/tmp/def");

    fs::set_path(&p, "/tmp/././abc/../def/abc");
    assert_equal(fs::weakly_canonical_path(&p, &canonp), true);
    assert_equal_str(canonp, "/tmp/def/abc");
    
#endif

    fs::free(&canonp);
    fs::free(&p);
}

define_test(get_symlink_target_reads_symlink)
{
    fs::fs_error err{};
    fs::path target{};

    assert_equal(fs::get_symlink_target(SANDBOX_TEST_SYMLINK, &target, &err), true);
    assert_equal_str(target, SANDBOX_TEST_FILE);

    assert_equal(fs::get_symlink_target(SANDBOX_TEST_SYMLINK_NO_TARGET, &target, &err), true);
    assert_equal_str(target, SANDBOX_DIR "/symlink_dest");

    assert_equal(fs::get_symlink_target(SANDBOX_TEST_FILE, &target, &err), false);

#if Linux
    assert_equal(err.error_code, EINVAL);
#endif

    assert_equal(fs::get_symlink_target(SANDBOX_DIR "/doesnotexist", &target, &err), false);

#if Linux
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

    fs::set_path(&p, SANDBOX_TEST_DIR);

    bool ret = fs::set_current_path(&p);

    fs::get_current_path(&p2);

    assert_equal(ret, true);
    assert_equal_str(p2, SANDBOX_TEST_DIR);

    fs::set_path(&p, SANDBOX_DIR);
    fs::set_current_path(&p);

    fs::free(&p);
    fs::free(&p2);
}

define_test(append_appends_to_path)
{
    fs::path p{};

#if Windows
    fs::set_path(&p, R"(C:\Windows)");
    fs::append_path(&p, "notepad.exe");
    assert_equal_str(p.data, R"(C:\Windows\notepad.exe)");

    // TODO: more tests
    assert_equal(false, true);
#else
    fs::set_path(&p, "/etc");
    fs::append_path(&p, "passwd");
    assert_equal_str(p.c_str(), "/etc/passwd");

    // with trailing separator, same result
    fs::set_path(&p, "/etc/");
    fs::append_path(&p, "passwd");
    assert_equal_str(p.c_str(), "/etc/passwd");

    // replaces when appending absolute path
    fs::set_path(&p, "/etc");
    fs::append_path(&p, "/passwd");
    assert_equal_str(p.c_str(), "/passwd");

    fs::set_path(&p, "//xyz");
    fs::append_path(&p, "abc");
    assert_equal_str(p.c_str(), "//xyz/abc");

    fs::set_path(&p, "//xyz/");
    fs::append_path(&p, "abc");
    assert_equal_str(p.c_str(), "//xyz/abc");

    fs::set_path(&p, "//xyz/dir");
    fs::append_path(&p, "/abc");
    assert_equal_str(p.c_str(), "/abc");

    fs::set_path(&p, "//xyz/dir");
    fs::append_path(&p, "//xyz/abc");
    assert_equal_str(p.c_str(), "//xyz/abc");

    // path alone doesnt check whether what it's pointing to is a directory or not
    // so this will just append at the end.
    fs::set_path(&p, "/etc/test.txt");
    fs::append_path(&p, "passwd");
    assert_equal_str(p.c_str(), "/etc/test.txt/passwd");

    fs::set_path(&p, "/etc/test.txt");
    fs::append_path(&p, "passwd/test2.txt");
    assert_equal_str(p.c_str(), "/etc/test.txt/passwd/test2.txt");

    // relative
    fs::set_path(&p, "abc");
    fs::append_path(&p, "xyz");
    assert_equal_str(p.c_str(), "abc/xyz");

    fs::set_path(&p, "");
    fs::append_path(&p, "xyz");
    assert_equal_str(p.c_str(), "xyz");

#endif

    fs::free(&p);
}

define_test(concat_concats_to_path)
{
    fs::path p{};

    fs::set_path(&p, "");
    fs::concat_path(&p, "");
    assert_equal(p.size, 0);
    assert_equal_str(p, "");

    fs::set_path(&p, "/");
    fs::concat_path(&p, "");
    assert_equal(p.size, 1);
    assert_equal_str(p, "/");

    fs::set_path(&p, "");
    fs::concat_path(&p, "/");
    assert_equal(p.size, 1);
    assert_equal_str(p, "/");

    fs::set_path(&p, "abc");
    fs::concat_path(&p, "def");
    assert_equal(p.size, 6);
    assert_equal_str(p, "abcdef");

    fs::set_path(&p, "/etc");
    fs::concat_path(&p, "passwd");
    assert_equal(p.size, 10);
    assert_equal_str(p, "/etcpasswd");

    // wide paths will get converted
    fs::set_path(&p, "/etc");
    fs::concat_path(&p, L"passwd");
    assert_equal(p.size, 10);
    assert_equal_str(p, "/etcpasswd");

    fs::free(&p);
}

define_test(relative_path_gets_the_relative_path_to_another)
{
    fs::path from{};
    fs::path to{};
    fs::path rel{};

#if Windows
    // TODO: add test
#else
#define assert_rel_path(From, To, ExpectedResult)\
    fs::set_path(&from, From);\
    fs::set_path(&to, To);\
    fs::relative_path(&from, &to, &rel);\
    assert_equal_str(to_const_string(&rel), to_const_string(ExpectedResult))

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

    fs::set_path(&p, SANDBOX_TEST_FILE "_touch1");

    assert_equal(fs::exists(&p), 0);
    assert_equal(fs::touch(&p), true);
    assert_equal(fs::exists(&p), 1);

    fs::filesystem_info old_info{};
    fs::filesystem_info new_info{};

    fs::get_filesystem_info(&p, &old_info);

    sleep_ms(200);

    assert_equal(fs::touch(&p), true);
    fs::get_filesystem_info(&p, &new_info);

#if Windows
    // TODO: implement
#else
    assert_greater(new_info.stx_mtime, old_info.stx_mtime);
#endif

    fs::free(&p);
}

define_test(copy_file_copies_file)
{
    fs::path from{};
    fs::path to{};

    fs::set_path(&from, SANDBOX_TEST_FILE);
    fs::set_path(&to, SANDBOX_TEST_FILE "_copy1");

    fs::fs_error err{};

    assert_equal(fs::copy_file(&from, &to, fs::copy_file_option::None, &err), true);

    assert_equal(fs::copy_file(&from, &to, fs::copy_file_option::None, &err), false);

#if Windows
    // TODO: implement
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
    assert_equal(fs::get_filesystem_info(&from, &from_info), true);
    assert_equal(fs::get_filesystem_info(&to, &to_info), true);

    // check that the date from To is indeed later than From
#if Windows
    // TODO: implement
#else
    assert_greater(to_info.stx_mtime, from_info.stx_mtime);
#endif

    sleep_ms(100);
    fs::touch(&from);

    assert_equal(fs::get_filesystem_info(&from, &from_info), true);

    // now From should be newer than To
#if Windows
    // TODO: implement
#else
    assert_greater(from_info.stx_mtime, to_info.stx_mtime);
#endif

    sleep_ms(100);
    assert_equal(fs::copy_file(&from, &to, fs::copy_file_option::UpdateExisting, &err), true);

    assert_equal(fs::get_filesystem_info(&to, &to_info), true);

    // and now To is newer again
#if Windows
    // TODO: implement
#else
    assert_greater(to_info.stx_mtime, from_info.stx_mtime);
#endif

    fs::free(&from);
    fs::free(&to);
}

define_test(copy_directory_copies_directory)
{
    fs::fs_error err{};

    const char *dir_from = SANDBOX_DIR "/copy_dir";
    const char *dir_to = SANDBOX_DIR "/copy_dir_to";

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
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2/file3"), 0);

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
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/file2"), 0);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2"), 0);
    assert_equal(fs::exists(SANDBOX_DIR "/copy_dir_to/dir1/dir2/file3"), 0);

#if Linux
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
    const char *dir_from = SANDBOX_DIR "/copy1_dir";
    const char *dir_to = SANDBOX_DIR "/copy1_dir_to";
    fs::create_directory(dir_from);

    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 0);
    assert_equal(fs::copy(dir_from, dir_to), true);
    assert_equal(fs::exists(dir_from), 1);
    assert_equal(fs::exists(dir_to), 1);

    const char *file_from = SANDBOX_DIR "/copy1_file";
    const char *file_to = SANDBOX_DIR "/copy1_file_to";
    fs::touch(file_from);

    assert_equal(fs::exists(file_from), 1);
    assert_equal(fs::exists(file_to), 0);
    assert_equal(fs::copy(file_from, file_to), true);
    assert_equal(fs::exists(file_from), 1);
    assert_equal(fs::exists(file_to), 1);
}

define_test(create_directory_creates_directory)
{
    fs::fs_error err{};
    fs::path p{};

    fs::set_path(&p, SANDBOX_TEST_DIR "/_create_dir1");

    assert_equal(fs::exists(&p), false); 
    assert_equal(fs::create_directory(&p), true); 
    assert_equal(fs::exists(&p), true); 

    // creating a directory that already exists returns true (but also yields error code)
    assert_equal(fs::create_directory(&p, fs::permission::User, &err), true); 
    assert_equal(fs::exists(&p), true); 

#if Linux
    assert_equal(err.error_code, EEXIST);
#endif

    // creating a directory on a path thats not a directory DOES yield an error
    fs::set_path(&p, SANDBOX_TEST_FILE);
    assert_equal(fs::exists(&p), true); 
    assert_equal(fs::create_directory(&p, fs::permission::User, &err), false);
    assert_equal(fs::exists(&p), true); 

#if Linux
    assert_equal(err.error_code, EEXIST);
#endif

    // relative path (in sandbox)
    fs::set_path(&p, "_create_dir2");

    assert_equal(fs::exists(&p), false); 
    assert_equal(fs::create_directory(&p), true); 
    assert_equal(fs::exists(&p), true); 

    fs::set_path(&p, "_create_dir2/xyz");

    assert_equal(fs::exists(&p), false); 
    assert_equal(fs::create_directory(&p), true); 
    assert_equal(fs::exists(&p), true); 

    // no permission
    fs::set_path(&p, "/root/_create_dir3");
    assert_equal(fs::create_directory(&p, fs::permission::User, &err), false); 

    // does not create parent directories
    fs::set_path(&p, SANDBOX_TEST_DIR "/_create_dir4/abc/def");
    assert_equal(fs::create_directory(&p, fs::permission::User, &err), false); 

#if Windows
    // TODO: check error
#else
    assert_equal(err.error_code, ENOENT);
#endif

    // TODO: create directory on top of file

    fs::free(&p);
}

define_test(create_directories_creates_directories_and_parents)
{
    fs::path p{};

    fs::set_path(&p, SANDBOX_TEST_DIR "/_create_dirs1");

    assert_equal(fs::exists(&p), false); 
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), true); 

    // creating a directory that already exists does not yield an error
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), true); 

    // relative path (in sandbox)
    fs::set_path(&p, "_create_dirs2");

    assert_equal(fs::exists(&p), false); 
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), true); 

    fs::set_path(&p, "_create_dirs2/xyz");

    assert_equal(fs::exists(&p), false); 
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), true); 
    
    
    // DOES create parent directories
    fs::set_path(&p, SANDBOX_TEST_DIR "/_create_dirs4/abc/def");
    assert_equal(fs::exists(&p), false); 
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), true); 

    fs::set_path(&p, "_create_dirs5/xyz");
    assert_equal(fs::exists(&p), false); 
    assert_equal(fs::create_directories(&p), true); 
    assert_equal(fs::exists(&p), true); 

    fs::fs_error err{};

    // no permission
    fs::set_path(&p, "/root/_create_dirs3");
    assert_equal(fs::create_directories(&p, fs::permission::User, &err), false); 

    fs::set_path(&p, "/root/_create_dirs3/xyz");
    assert_equal(fs::create_directories(&p, fs::permission::User, &err), false); 

    fs::free(&p);
}

define_test(create_hard_link_creates_hard_link)
{
    fs::path target{};
    fs::path link{};
    fs::fs_error err{};

    fs::set_path(&target, SANDBOX_TEST_FILE);
    fs::set_path(&link,   SANDBOX_DIR "/_hardlink1");

    assert_equal(fs::exists(&link), false); 
    assert_equal(fs::create_hard_link(&target, &link), true); 
    assert_equal(fs::exists(&link), true); 

    assert_equal(fs::are_equivalent(&target, &link), true);
    assert_equal(fs::are_equivalent(&target, &link, false), true);

    // does not overwrite links or files
    assert_equal(fs::create_hard_link(&target, &link, &err), false); 

#if Windows
    // TODO: check error
#else
    assert_equal(err.error_code, EEXIST);
#endif
    
    fs::set_path(&target, SANDBOX_TEST_FILE "_does_not_exist1");
    fs::set_path(&link,   SANDBOX_DIR "/_hardlink2");

    // can't create hard links to things that don't exist
    assert_equal(fs::create_hard_link(&target, &link, &err), false); 

#if Windows
    // TODO: check error
#else
    assert_equal(err.error_code, ENOENT);
#endif

    // cannot create hard link to a directory
    fs::set_path(&target, SANDBOX_TEST_DIR);
    fs::set_path(&link,   SANDBOX_DIR "/_hardlink3");

    assert_equal(fs::exists(&link), false); 
    assert_equal(fs::create_hard_link(&target, &link, &err), false); 
    assert_equal(fs::exists(&link), false); 

#if Windows
    // TODO: check error
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
    fs::fs_error err{};

    fs::set_path(&target, SANDBOX_TEST_FILE);
    fs::set_path(&link,   SANDBOX_DIR "/_symlink1");

    assert_equal(fs::exists(&link), false); 
    assert_equal(fs::create_symlink(&target, &link), true); 
    assert_equal(fs::exists(&link), true); 

    assert_equal(fs::are_equivalent(&target, &link), true);
    assert_equal(fs::are_equivalent(&target, &link, false), false);

    // does not overwrite links or files
    assert_equal(fs::create_symlink(&target, &link, &err), false); 

#if Windows
    // TODO: check error
#else
    assert_equal(err.error_code, EEXIST);
#endif
    
    fs::set_path(&target, SANDBOX_TEST_FILE "_does_not_exist2");
    fs::set_path(&link,   SANDBOX_DIR "/_symlink2");

    assert_equal(fs::create_symlink(&target, &link, &err), true);

    assert_equal(fs::exists(&link), false); 
    assert_equal(fs::exists(&link, false), true); 

    // works on directories
    fs::set_path(&target, SANDBOX_TEST_DIR);
    fs::set_path(&link,   SANDBOX_DIR "/_symlink3");

    assert_equal(fs::exists(&link), false); 
    assert_equal(fs::create_symlink(&target, &link), true); 
    assert_equal(fs::exists(&link), true); 

    assert_equal(fs::are_equivalent(&target, &link), true);
    assert_equal(fs::are_equivalent(&target, &link, false), false);

    fs::free(&target);
    fs::free(&link);
}

define_test(move_moves_files_and_directories)
{
    fs::path src{};
    fs::path dst{};
    fs::fs_error err{};

    fs::set_path(&src, SANDBOX_DIR "/_move1");
    fs::set_path(&dst, SANDBOX_DIR "/_move2");

    fs::copy_file(SANDBOX_TEST_FILE, &src);

    assert_equal(fs::exists(&src), true); 
    assert_equal(fs::exists(&dst), false); 
    assert_equal(fs::move(&src, &dst), true); 
    assert_equal(fs::exists(&src), false); 
    assert_equal(fs::exists(&dst), true); 

    fs::copy_file(SANDBOX_TEST_FILE, &src);

    // overwrites existing files
    assert_equal(fs::exists(&src), true); 
    assert_equal(fs::exists(&dst), true); 
    assert_equal(fs::move(&src, &dst), true); 
    assert_equal(fs::exists(&src), false); 
    assert_equal(fs::exists(&dst), true); 

    // can move directories
    auto src_dir = SANDBOX_DIR "/_movedir1";
    auto dst_dir = SANDBOX_DIR "/_movedir2";

    fs::create_directory(src_dir);

    assert_equal(fs::exists(src_dir), true); 
    assert_equal(fs::exists(dst_dir), false); 
    assert_equal(fs::move(src_dir, dst_dir), true); 
    assert_equal(fs::exists(src_dir), false); 
    assert_equal(fs::exists(dst_dir), true); 

    assert_equal(fs::move(dst_dir, src_dir), true); 

    // cannot move directory into file
    assert_equal(fs::move(src_dir, dst, &err), false); 

#if Windows
#else
    assert_equal(err.error_code, ENOTDIR);
#endif

    // cannot move file into directory
    assert_equal(fs::move(dst, src_dir, &err), false); 

#if Windows
#else
    assert_equal(err.error_code, EISDIR);
#endif

    // can move directory thats not empty
    assert_equal(fs::move(dst, SANDBOX_DIR "/_movedir1/_move1"), true);

    assert_equal(fs::exists(src_dir), true); 
    assert_equal(fs::exists(dst_dir), false); 
    assert_equal(fs::move(src_dir, dst_dir), true); 
    assert_equal(fs::exists(src_dir), false); 
    assert_equal(fs::exists(dst_dir), true); 
    assert_equal(fs::exists(SANDBOX_DIR "/_movedir2/_move1"), true); 

    fs::free(&src);
    fs::free(&dst);
}

define_test(remove_file_removes_file)
{
    fs::path p{};
    fs::fs_error err{};

    fs::set_path(&p, SANDBOX_DIR "/_remove_file1");
    fs::copy_file(SANDBOX_TEST_FILE, &p);

    assert_equal(fs::exists(p), true); 
    assert_equal(fs::remove_file(p), true); 
    assert_equal(fs::exists(p), false); 

    assert_equal(fs::remove_file(p, &err), false); 

#if Windows
#else
    assert_equal(err.error_code, ENOENT);
#endif

    assert_equal(fs::remove_file(SANDBOX_DIR, &err), false); 

#if Windows
#else
    assert_equal(err.error_code, EISDIR);
#endif

    fs::free(&p);
}

define_test(remove_empty_directory_removes_only_empty_directories)
{
    fs::fs_error err{};

    const char *dir1 = SANDBOX_DIR "/remove_empty_dir1";
    const char *dir2 = SANDBOX_DIR "/remove_empty_dir2";
    const char *file1 = SANDBOX_DIR "/remove_empty_dir_file";
    const char *file2 = SANDBOX_DIR "/remove_empty_dir2/not_empty";

    fs::create_directory(dir1);
    fs::create_directory(dir2);
    fs::touch(file1);
    fs::touch(file2);

    assert_equal(fs::exists(dir1), true); 
    assert_equal(fs::remove_empty_directory(dir1, &err), true); 
    assert_equal(fs::exists(dir1), false); 

    // not empty
    assert_equal(fs::exists(dir2), true); 
    assert_equal(fs::remove_empty_directory(dir2, &err), false); 
    assert_equal(fs::exists(dir2), true); 

#if Linux
    assert_equal(err.error_code, ENOTEMPTY);
#endif

    assert_equal(fs::exists(file1), true); 
    assert_equal(fs::remove_empty_directory(file1, &err), false); 
    assert_equal(fs::exists(file1), true); 

#if Linux
    assert_equal(err.error_code, ENOTDIR);
#endif
}

define_test(remove_directory_removes_directories)
{
    fs::fs_error err{};

    const char *dir1 = SANDBOX_DIR  "/remove_dir1";
    const char *dir2 = SANDBOX_DIR  "/remove_dir2";
    const char *dir3 = SANDBOX_DIR  "/remove_dir3";
    const char *file1 = SANDBOX_DIR "/remove_dir_file";

    fs::create_directory(dir1);
    fs::create_directory(dir2);
    fs::create_directory(dir3);
    fs::touch(file1);
    fs::touch(SANDBOX_DIR "/remove_dir2/notempty");

    fs::create_directory(SANDBOX_DIR "/remove_dir3/dir4");
    fs::create_directory(SANDBOX_DIR "/remove_dir3/dir5");
    fs::touch(SANDBOX_DIR "/remove_dir3/file3");
    fs::touch(SANDBOX_DIR "/remove_dir3/dir4/file4");

    assert_equal(fs::exists(dir1), true); 
    assert_equal(fs::remove_directory(dir1, &err), true); 
    assert_equal(fs::exists(dir1), false); 

    // not empty
    assert_equal(fs::exists(dir2), true); 
    assert_equal(fs::remove_directory(dir2, &err), true);
    assert_equal(fs::exists(dir2), false); 

#if Linux
    assert_equal(err.error_code, 0);
#endif

    // not empty with subdirectories and more descendants
    assert_equal(fs::exists(dir3), true); 
    assert_equal(fs::remove_directory(dir3, &err), true);
    assert_equal(fs::exists(dir3), false); 

#if Linux
    assert_equal(err.error_code, 0);
#endif

    assert_equal(fs::exists(file1), true); 
    assert_equal(fs::remove_directory(file1, &err), false); 
    assert_equal(fs::exists(file1), true); 

#if Linux
    assert_equal(err.error_code, ENOTDIR);
#endif
}

define_test(remove_removes_anything)
{
    fs::fs_error err{};

    const char *dir1 = SANDBOX_DIR  "/remove1";
    const char *dir2 = SANDBOX_DIR  "/remove2";
    const char *dir3 = SANDBOX_DIR  "/remove3";
    const char *file1 = SANDBOX_DIR "/remove_file";
    const char *doesnotexist = SANDBOX_DIR "/doesnotexist";

    fs::create_directory(dir1);
    fs::create_directory(dir2);
    fs::create_directory(dir3);
    fs::touch(file1);
    fs::touch(SANDBOX_DIR "/remove/notempty");

    fs::create_directory(SANDBOX_DIR "/remove/dir4");
    fs::create_directory(SANDBOX_DIR "/remove/dir5");
    fs::touch(SANDBOX_DIR "/remove/file3");
    fs::touch(SANDBOX_DIR "/remove/dir4/file4");

    assert_equal(fs::exists(dir1), true); 
    assert_equal(fs::remove(dir1, &err), true); 
    assert_equal(fs::exists(dir1), false); 

    // not empty
    assert_equal(fs::exists(dir2), true); 
    assert_equal(fs::remove(dir2, &err), true);
    assert_equal(fs::exists(dir2), false); 

    // not empty with subdirectories and more descendants
    assert_equal(fs::exists(dir3), true); 
    assert_equal(fs::remove(dir3, &err), true);
    assert_equal(fs::exists(dir3), false); 

    assert_equal(fs::exists(file1), true); 
    assert_equal(fs::remove(file1, &err), true); 
    assert_equal(fs::exists(file1), false); 

    // does not exist, still returns true
    assert_equal(fs::exists(doesnotexist), false); 
    assert_equal(fs::remove(doesnotexist, &err), true); 
}


/*
define_test(iterator_test1)
{
    fs::fs_error err{};

    if (fs::fs_iterator it; true)
    if (defer { fs::free(&it); }; fs::init(&it, SANDBOX_DIR, &err))
    for (fs::fs_iterator_item *item = fs::_iterate(&it, fs::iterate_option::None, &err);
         item != nullptr;
         item = fs::_iterate(&it, fs::iterate_option::None, &err))
        printf("%x - %s\n", (u32)item->type, item->path.c_str);

    assert_equal(err.error_code, 0);
}
*/

define_test(iterator_test)
{
    fs::fs_error err{};
    array<fs::path> descendants{};

    fs::create_directories(SANDBOX_DIR "/it/dir1");
    fs::create_directories(SANDBOX_DIR "/it/dir2/dir3");
    fs::touch(SANDBOX_DIR "/it/file1");
    fs::touch(SANDBOX_DIR "/it/dir2/file2");

    for_path(item, "it", fs::iterate_option::None, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::set_path(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 3);

    assert_equal_str(descendants[0], "dir1");
    assert_equal_str(descendants[1], "dir2");
    assert_equal_str(descendants[2], "file1");

    assert_equal(err.error_code, 0);

    free<true>(&descendants);
}

define_test(iterator_type_filter_test)
{
    fs::fs_error err{};
    array<fs::path> descendants{};

    fs::create_directories(SANDBOX_DIR "/it2/dir1");
    fs::create_directories(SANDBOX_DIR "/it2/dir2/dir3");
    fs::touch(SANDBOX_DIR "/it2/file1");
    fs::touch(SANDBOX_DIR "/it2/dir2/file2");

    for_path_files(item, "it2", fs::iterate_option::None, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::set_path(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 1);
    assert_equal_str(descendants[0], "file1");
    assert_equal(err.error_code, 0);

    free_values(&descendants);
    clear(&descendants);

    // directories
    for_path_type(fs::filesystem_type::Directory, item, "it2", fs::iterate_option::None, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::set_path(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 2);
    assert_equal_str(descendants[0], "dir1");
    assert_equal_str(descendants[1], "dir2");
    assert_equal(err.error_code, 0);

    free<true>(&descendants);
}

define_test(recursive_iterator_test)
{
    fs::fs_error err{};
    array<fs::path> descendants{};

    fs::create_directories(SANDBOX_DIR "/rit/dir1/dir2/dir3/dir4");
    fs::touch(SANDBOX_DIR "/rit/dir1/dir2/dir3/dir4/file");

    fs::iterate_option opts = fs::iterate_option::None;//Fullpaths;

    for_recursive_path(item, SANDBOX_DIR "/rit", opts, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::set_path(cp, item->path);
    }

    assert_equal(descendants.size, 5);

    // order here is fixed and the results must look like this.
    // use ChildrenFirst for reverse order (see next test)
    assert_equal_str(descendants[0], SANDBOX_DIR "/rit/dir1");
    assert_equal_str(descendants[1], SANDBOX_DIR "/rit/dir1/dir2");
    assert_equal_str(descendants[2], SANDBOX_DIR "/rit/dir1/dir2/dir3");
    assert_equal_str(descendants[3], SANDBOX_DIR "/rit/dir1/dir2/dir3/dir4");
    assert_equal_str(descendants[4], SANDBOX_DIR "/rit/dir1/dir2/dir3/dir4/file");

    free<true>(&descendants);
}

define_test(recursive_iterator_children_first_test)
{
    fs::fs_error err{};

    fs::create_directories(SANDBOX_DIR "/cf/dir1/dir2/dir3/dir4");
    fs::touch(SANDBOX_DIR "/cf/dir1/dir2/dir3/dir4/file");

    array<fs::path> descendants{};

    fs::iterate_option opts = fs::iterate_option::Fullpaths | fs::iterate_option::ChildrenFirst;

    for_recursive_path(item, "cf", opts, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::set_path(cp, item->path);
    }

    assert_equal(descendants.size, 5);

    // order here is fixed and the results must look like this
    assert_equal_str(descendants[0], SANDBOX_DIR "/cf/dir1/dir2/dir3/dir4/file");
    assert_equal_str(descendants[1], SANDBOX_DIR "/cf/dir1/dir2/dir3/dir4");
    assert_equal_str(descendants[2], SANDBOX_DIR "/cf/dir1/dir2/dir3");
    assert_equal_str(descendants[3], SANDBOX_DIR "/cf/dir1/dir2");
    assert_equal_str(descendants[4], SANDBOX_DIR "/cf/dir1");

    free<true>(&descendants);
}

define_test(recursive_iterator_symlink_test)
{
    fs::fs_error err{};
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
        fs::set_path(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);
    assert_equal(descendants.size, 8);

    assert_equal_str(descendants[0], "rit_sym/dir1");
    assert_equal_str(descendants[1], "rit_sym/dir2");
    assert_equal_str(descendants[2], "rit_sym/file1");
    assert_equal_str(descendants[3], "rit_sym/symlink");
    assert_equal_str(descendants[4], "rit_sym/dir2/dir3");
    assert_equal_str(descendants[5], "rit_sym/dir2/file2");
    assert_equal_str(descendants[6], "rit_sym/symlink/dir3");
    assert_equal_str(descendants[7], "rit_sym/symlink/file2");

    free<true>(&descendants);
}

define_test(recursive_iterator_type_filter_test)
{
    fs::fs_error err{};
    array<fs::path> descendants{};

    fs::create_directories(SANDBOX_DIR "/rit_filter/dir1");
    fs::create_directories(SANDBOX_DIR "/rit_filter/dir2/dir3");
    fs::touch(SANDBOX_DIR "/rit_filter/file1");
    fs::touch(SANDBOX_DIR "/rit_filter/dir2/file2");

    for_recursive_path_type(fs::filesystem_type::File, item, "rit_filter", fs::iterate_option::None, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::set_path(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 2);

    assert_equal_str(descendants[0], "rit_filter/file1");
    assert_equal_str(descendants[1], "rit_filter/dir2/file2");

    free_values(&descendants);
    clear(&descendants);

    // directories
    for_recursive_path_directories(item, "rit_filter", fs::iterate_option::None, &err)
    {
        fs::path *cp = ::add_at_end(&descendants);
        fs::init(cp);
        fs::set_path(cp, item->path);
    }

    sort(descendants.data, descendants.size, path_comparer);

    assert_equal(descendants.size, 3);

    assert_equal_str(descendants[0], "rit_filter/dir1");
    assert_equal_str(descendants[1], "rit_filter/dir2");
    assert_equal_str(descendants[2], "rit_filter/dir2/dir3");

    free<true>(&descendants);
}

define_test(get_children_names_gets_directory_children_names)
{
    fs::fs_error err{};
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

    assert_equal_str(children[0], "dir1");
    assert_equal_str(children[1], "dir2");
    assert_equal_str(children[2], "file1");

    free<true>(&children);
}

define_test(get_children_names_returns_minus_one_on_error)
{
    fs::fs_error err{};
    array<fs::path> children{};

    s64 count = fs::get_children_names(SANDBOX_TEST_DIR_NO_PERMISSION, &children, &err);

    assert_equal(count, -1);
    assert_equal(children.size, 0);

#if Linux
    assert_equal(err.error_code, EACCES);
#endif

    free<true>(&children);
}

define_test(get_children_fullpaths_gets_directory_children_fullpaths)
{
    fs::fs_error err{};
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

    assert_equal_str(children[0], SANDBOX_DIR "/get_children2/dir1");
    assert_equal_str(children[1], SANDBOX_DIR "/get_children2/dir2");
    assert_equal_str(children[2], SANDBOX_DIR "/get_children2/file1");

    free<true>(&children);
}

define_test(get_all_descendants_paths_gets_all_descendants_paths_relative_to_given_directory)
{
    fs::fs_error err{};
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

    assert_equal_str(all_descendants[0], "get_all_descendants/dir1");
    assert_equal_str(all_descendants[1], "get_all_descendants/dir2");
    assert_equal_str(all_descendants[2], "get_all_descendants/file1");
    assert_equal_str(all_descendants[3], "get_all_descendants/dir2/dir3");
    assert_equal_str(all_descendants[4], "get_all_descendants/dir2/file2");

    free<true>(&all_descendants);
}

define_test(get_all_descendants_fullpaths_gets_all_descendants_paths)
{
    fs::fs_error err{};
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

    assert_equal_str(all_descendants[0], SANDBOX_DIR "/get_all_descendants/dir1");
    assert_equal_str(all_descendants[1], SANDBOX_DIR "/get_all_descendants/dir2");
    assert_equal_str(all_descendants[2], SANDBOX_DIR "/get_all_descendants/file1");
    assert_equal_str(all_descendants[3], SANDBOX_DIR "/get_all_descendants/dir2/dir3");
    assert_equal_str(all_descendants[4], SANDBOX_DIR "/get_all_descendants/dir2/file2");

    free<true>(&all_descendants);
}

define_test(get_children_count_gets_children_count)
{
    fs::fs_error err{};

    fs::create_directories(SANDBOX_DIR "/get_children_count/dir1");
    fs::create_directories(SANDBOX_DIR "/get_children_count/dir2/dir3");
    fs::touch(SANDBOX_DIR "/get_children_count/file1");
    fs::touch(SANDBOX_DIR "/get_children_count/dir2/file2");

    assert_equal(fs::get_children_count(SANDBOX_DIR "/get_children_count", &err), 3);
    assert_equal(fs::get_children_count(SANDBOX_DIR "/doesnotexist", &err), -1);

#if Linux
    assert_equal(err.error_code, ENOENT);
#endif

    assert_equal(fs::get_children_count(SANDBOX_TEST_DIR_NO_PERMISSION, &err), -1);

#if Linux
    assert_equal(err.error_code, EACCES);
#endif
}

define_test(get_descendant_count_gets_children_count)
{
    fs::fs_error err{};

    fs::create_directories(SANDBOX_DIR "/get_descendant_count/dir1");
    fs::create_directories(SANDBOX_DIR "/get_descendant_count/dir2/dir3");
    fs::touch(SANDBOX_DIR "/get_descendant_count/file1");
    fs::touch(SANDBOX_DIR "/get_descendant_count/dir2/file2");

    assert_equal(fs::get_descendant_count(SANDBOX_DIR "/get_descendant_count", &err), 5);
    assert_equal(fs::get_descendant_count(SANDBOX_DIR "/doesnotexist", &err), -1);

#if Linux
    assert_equal(err.error_code, ENOENT);
#endif

    assert_equal(fs::get_descendant_count(SANDBOX_TEST_DIR_NO_PERMISSION, &err), -1);

#if Linux
    assert_equal(err.error_code, EACCES);
#endif
}

#if 0
define_test(iterator_test4)
{
    fs::fs_error err{};

    for_path(item, SANDBOX_DIR, fs::iterate_option::None, &err)
        printf("%x - %s\n", (u32)item->type, item->path.c_str);

    assert_equal(err.error_code, 0);
}
#endif

define_test(get_executable_path_gets_executable_path)
{
    // const char *actual = "/home/user/dev/git/fs/bin/tests/path_tests";
    fs::path p{};
    fs::fs_error err{};

    assert_equal(fs::get_executable_path(&p, &err), true);
    assert_equal_str(fs::filename(&p), "path_tests");

    // obviously this wont work on all systems
    // assert_equal(p, actual);

    fs::free(&p);
}

static fs::path old_current_dir;

void _setup()
{
    fs::get_current_path(&old_current_dir);
    umask(0); // if this is not set to 0 mkdir might not set correct permissions
    mkdir(SANDBOX_DIR, 0777);
    mkdir(SANDBOX_TEST_DIR, 0777);
    FILE *f = fopen(SANDBOX_TEST_FILE, "w");
    assert(f != nullptr);
    fclose(f);
    symlink(SANDBOX_TEST_FILE, SANDBOX_TEST_SYMLINK);
    symlink(SANDBOX_DIR "/symlink_dest", SANDBOX_TEST_SYMLINK_NO_TARGET);
 
    mkfifo(SANDBOX_TEST_PIPE, 0644);

    mkdir(SANDBOX_TEST_DIR2, 0777);
    f = fopen(SANDBOX_TEST_FILE2, "w"); assert(f != nullptr); fclose(f);
    mkdir(SANDBOX_TEST_DIR_NO_PERMISSION, 0777);
    f = fopen(SANDBOX_TEST_FILE_IN_NOPERM_DIR, "w"); assert(f != nullptr); fclose(f);
    chmod(SANDBOX_TEST_DIR_NO_PERMISSION, 0000);

    fs::path _tmp{};
    fs::set_path(&_tmp, SANDBOX_DIR);
    fs::set_current_path(&_tmp);
    fs::free(&_tmp);
}

// #include <filesystem> // lol

void _cleanup()
{
    chmod(SANDBOX_TEST_DIR_NO_PERMISSION, 0777);
    fs::set_current_path(&old_current_dir);
    // std::filesystem::remove_all(SANDBOX_DIR);
    fs::fs_error err{};
    if (!fs::remove_directory(SANDBOX_DIR, &err))
        fprintf(stderr, "ERROR: could not remove directory %s. Error code %d:\n%s\n", SANDBOX_DIR, err.error_code, err.what);

    fs::free(&old_current_dir);
}

define_test_main(_setup(), _cleanup());
