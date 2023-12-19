
#include <t1/t1.hpp>

#if Windows
#else
#include <sys/stat.h>
#endif
#include <unistd.h>

#include "shl/string.hpp"
#include "shl/time.hpp" // for sleep
#include "shl/platform.hpp"
#include "fs/path.hpp"

#define assert_equal_str(STR1, STR2)\
    assert_equal(compare_strings(to_const_string(STR1), STR2), 0)

// TODO: windows tests...

#define SANDBOX_DIR             "/tmp/sandbox"
#define SANDBOX_TEST_DIR        SANDBOX_DIR "/dir"
#define SANDBOX_TEST_FILE       SANDBOX_DIR "/file"
#define SANDBOX_TEST_SYMLINK    SANDBOX_DIR "/symlink"
#define SANDBOX_TEST_SYMLINK_NO_TARGET   SANDBOX_DIR "/symlink2"
#define SANDBOX_TEST_PIPE       SANDBOX_DIR "/pipe"

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

define_test(exists_returns_true_if_directory_exists)
{
    fs::path p{};
    fs::set_path(&p, SANDBOX_TEST_DIR);

    assert_equal(fs::exists(&p), true);

    fs::free(&p);
}

define_test(exists_returns_true_if_file_exists)
{
    fs::path p{};
    fs::set_path(&p, SANDBOX_TEST_FILE);

    assert_equal(fs::exists(&p), true);

    fs::free(&p);
}

define_test(exists_checks_if_symlink_exists)
{
    fs::path p{};
    fs::set_path(&p, SANDBOX_TEST_SYMLINK);

    assert_equal(fs::exists(&p), true);

    fs::set_path(&p, SANDBOX_TEST_SYMLINK_NO_TARGET);
    assert_equal(fs::exists(&p, false), true);

    fs::free(&p);
}

define_test(exists_yields_error_when_not_exists)
{
    fs::path p{};
    fs::fs_error err;
    fs::set_path(&p, SANDBOX_TEST_DIR "/abc");

    assert_equal(fs::exists(&p, true, &err), false);

#if Linux
    assert_equal(err.error_code, ENOENT);
#endif

    fs::free(&p);
}

define_test(exists_checks_if_symlink_target_exists)
{
    fs::path p{};
    fs::set_path(&p, SANDBOX_TEST_SYMLINK_NO_TARGET);
    // target doesnt exist, this checks if symlink exists
    assert_equal(fs::exists(&p, false), true);

    // this checks if target exists
    assert_equal(fs::exists(&p, true), false);

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
#endif

    fs::free(&canonp);
    fs::free(&p);
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

    assert_equal(fs::exists(&p), false);
    assert_equal(fs::touch(&p), true);
    assert_equal(fs::exists(&p), true);

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

    assert_equal(fs::copy_file(&from, &to, fs::copy_file_options::None, &err), true);

    assert_equal(fs::copy_file(&from, &to, fs::copy_file_options::None, &err), false);

#if Windows
    // TODO: implement
#else
    assert_equal(err.error_code, EEXIST);
#endif

    // overwriting
    assert_equal(fs::copy_file(&from, &to, fs::copy_file_options::OverwriteExisting, &err), true);

    assert_equal(fs::copy_file(&from, &to, fs::copy_file_options::SkipExisting, &err), true);

    // returns true even if not copied
    assert_equal(fs::copy_file(&from, &to, fs::copy_file_options::UpdateExisting, &err), true);

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

    sleep_ms(200);
    fs::touch(&from);

    assert_equal(fs::get_filesystem_info(&from, &from_info), true);

    // now From should be newer than To
#if Windows
    // TODO: implement
#else
    assert_greater(from_info.stx_mtime, to_info.stx_mtime);
#endif

    sleep_ms(200);
    assert_equal(fs::copy_file(&from, &to, fs::copy_file_options::UpdateExisting, &err), true);

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

#if 0
define_test(get_executable_path_gets_executable_path)
{
    // fs::path actual("/home/user/dev/git/fs/bin/tests/path_tests");
    fs::path p;

    get_executable_path(&p);

    // obviously this wont work on all systems
    // assert_equal(p, actual);
}
#endif

static fs::path old_current_dir;

void _setup()
{
    fs::get_current_path(&old_current_dir);
    mkdir(SANDBOX_DIR, 0777);
    mkdir(SANDBOX_TEST_DIR, 0777);
    FILE *f = fopen(SANDBOX_TEST_FILE, "w");
    assert(f != nullptr);
    fclose(f);
    symlink(SANDBOX_TEST_FILE, SANDBOX_TEST_SYMLINK);
    symlink(SANDBOX_DIR "/symlink_dest", SANDBOX_TEST_SYMLINK_NO_TARGET);
 
    mkfifo(SANDBOX_TEST_PIPE, 0644);

    fs::path _tmp{};
    fs::set_path(&_tmp, SANDBOX_DIR);
    fs::set_current_path(&_tmp);
    fs::free(&_tmp);
}

#include <filesystem> // lol

void _cleanup()
{
    fs::set_current_path(&old_current_dir);
    std::filesystem::remove_all(SANDBOX_DIR);

    fs::free(&old_current_dir);
}

define_test_main(_setup(), _cleanup());
