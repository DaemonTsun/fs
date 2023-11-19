
#include <t1/t1.hpp>

#include "shl/string.hpp"
#include "shl/platform.hpp"
#include "fs/path.hpp"

#define assert_equal_str(STR1, STR2)\
    assert_equal(compare_strings(STR1, STR2), 0)

// TODO: windows tests...

#define SANDBOX_DIR             "/tmp/sandbox"
#define SANDBOX_TEST_DIR        SANDBOX_DIR "/dir"
#define SANDBOX_TEST_FILE       SANDBOX_DIR "/file"
#define SANDBOX_TEST_SYMLINK    SANDBOX_DIR "/symlink"
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

define_test(test2)
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
    assert_equal(fs::is_file(&p),         false);
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

/*
define_test(exists_returns_true_if_directory_exists)
{
#if Windows
    fs::path p = LR"=(C:\)=";
#else
    fs::path p = "/"_path;
#endif

    assert_equal(fs::exists(&p), true);
}
*/

#if 0
define_test(filename_returns_the_filename)
{
#if Windows
    fs::path p(R"=(C:\this\is\a\test)=");
#else
    fs::path p("/this/is/a/test");
#endif

    assert_equal_str(fs::filename(&p), "test");
}

define_test(parent_path_returns_the_parent_path)
{
#if Windows
    fs::path p(R"=(C:\this\is\a\test)=");
#else
    fs::path p("/this/is/a/test");
#endif

    fs::path parent;

    fs::parent_path(&p, &parent);

#if Windows
    assert_equal_str(parent.c_str(), R"=(C:\this\is\a)=");
#else
    assert_equal_str(parent.c_str(), "/this/is/a");
#endif
}

define_test(parent_path_returns_the_parent_path2)
{
#if Windows
    fs::path p(R"=(C:\)=");
#else
    fs::path p("/");
#endif

    fs::path parent;

    fs::parent_path(&p, &parent);

#if Windows
    assert_equal_str(parent.c_str(), R"=(C:\)=");
#else
    assert_equal_str(parent.c_str(), "/");
#endif
}

define_test(exists_returns_true_if_path_exists)
{
#if Windows
    fs::path p(R"=(C:\)=");
#else
    fs::path p("/");
#endif

    assert_equal(fs::exists(&p), true);
}

define_test(exists_returns_false_if_path_doesnt_exists)
{
#if Windows
    fs::path p(R"=(C:\abc)=");
#else
    fs::path p("/abc");
#endif

    assert_equal(fs::exists(&p), false);
}

define_test(is_file_returns_true_if_path_is_file)
{
#if Windows
    fs::path p(R"=(C:\Windows\notepad.exe)=");
#else
    fs::path p("/etc/passwd");
#endif

    assert_equal(fs::is_file(&p), true);
}

define_test(is_file_returns_false_if_path_is_not_file)
{
#if Windows
    fs::path p(R"=(C:\Windows\)=");
#else
    fs::path p("/etc/");
#endif

    assert_equal(fs::is_file(&p), false);
}

define_test(is_directory_returns_true_if_path_is_directory)
{
#if Windows
    fs::path p(R"=(C:\Windows\)=");
#else
    fs::path p("/etc/");
#endif

    assert_equal(fs::is_directory(&p), true);
}

define_test(is_directory_returns_false_if_path_is_not_directory)
{
#if Windows
    fs::path p(R"=(C:\Windows\notepad.exe)=");
#else
    fs::path p("/etc/passwd");
#endif

    assert_equal(fs::is_directory(&p), false);
}

define_test(is_absolute_returns_true_if_path_is_absoltue)
{
#if Windows
    fs::path p(R"=(C:\Windows\notepad.exe)=");
#else
    fs::path p("/etc/passwd");
#endif

    assert_equal(fs::is_absolute(&p), true);
}

define_test(is_absolute_returns_false_if_path_is_not_absolute)
{
#if Windows
    fs::path p(R"=(..\notepad.exe)=");
#else
    fs::path p("../passwd");
#endif

    assert_equal(fs::is_absolute(&p), false);
}

define_test(is_relative_returns_true_if_path_is_relative)
{
#if Windows
    fs::path p(R"=(..\notepad.exe)=");
#else
    fs::path p("../passwd");
#endif

    assert_equal(fs::is_relative(&p), true);
}

define_test(is_relative_returns_false_if_path_is_not_relative)
{
#if Windows
    fs::path p(R"=(C:\Windows\notepad.exe)=");
#else
    fs::path p("/etc/passwd");
#endif

    assert_equal(fs::is_relative(&p), false);
}

define_test(append_appends_to_path)
{
#if Windows
    fs::path p(R"=(C:\Windows\)=");
    fs::append_path(&p, "notepad.exe");
    assert_equal_str(p.c_str(), R"=(C:\Windows\notepad.exe)=");
#else
    fs::path p("/etc/");
    fs::append_path(&p, "passwd");
    assert_equal_str(p.c_str(), "/etc/passwd");
#endif
}

define_test(append_appends_to_path2)
{
#if Windows
    fs::path p(R"=(C:\Windows\)=");
    fs::path p2;
    fs::append_path(&p, "notepad.exe", &p2);
    assert_equal_str(p.c_str(), R"=(C:\Windows\)=");
    assert_equal_str(p2.c_str(), R"=(C:\Windows\notepad.exe)=");
#else
    fs::path p("/etc");
    fs::path p2;
    fs::append_path(&p, "passwd", &p2);
    assert_equal_str(p.c_str(), "/etc");
    assert_equal_str(p2.c_str(), "/etc/passwd");
#endif
}

define_test(append_appends_to_path3)
{
#if Windows
    fs::path p(R"=(C:\Windows\)=");
    p = p / "notepad.exe"_cs;
    assert_equal_str(p.c_str(), R"=(C:\Windows\notepad.exe)=");
#else
    fs::path p("/etc/");
    p = p / "passwd"_cs;
    assert_equal_str(p.c_str(), "/etc/passwd");
#endif
}

define_test(append_appends_to_path4)
{
#if Windows
    fs::path p(R"=(C:\Windows\)=");
    p /= "notepad.exe";
    assert_equal_str(p.c_str(), R"=(C:\Windows\notepad.exe)=");
#else
    fs::path p("/etc/");
    p /= "passwd";
    assert_equal_str(p.c_str(), "/etc/passwd");
#endif
}

define_test(concat_concats_to_path)
{
#if Windows
    fs::path p(R"=(C:\Windows)=");
    fs::concat_path(&p, "notepad.exe");
    assert_equal_str(p.c_str(), R"=(C:\Windowsnotepad.exe)=");
#else
    fs::path p("/etc");
    fs::concat_path(&p, "passwd");
    assert_equal_str(p.c_str(), "/etcpasswd");
#endif
}

define_test(concat_concats_to_path2)
{
#if Windows
    fs::path p(R"=(C:\Windows)=");
    fs::path p2;
    fs::concat_path(&p, "notepad.exe", &p2);
    assert_equal_str(p.c_str(), R"=(C:\Windows)=");
    assert_equal_str(p2.c_str(), R"=(C:\Windowsnotepad.exe)=");
#else
    fs::path p("/etc");
    fs::path p2;
    fs::concat_path(&p, "passwd", &p2);
    assert_equal_str(p.c_str(), "/etc");
    assert_equal_str(p2.c_str(), "/etcpasswd");
#endif
}

define_test(concat_concats_to_path3)
{
#if Windows
    fs::path p(R"=(C:\Windows)=");
    p = p + "notepad.exe"_cs;
    assert_equal_str(p.c_str(), R"=(C:\Windowsnotepad.exe)=");
#else
    fs::path p("/etc");
    p = p + "passwd"_cs;
    assert_equal_str(p.c_str(), "/etcpasswd");
#endif
}

define_test(concat_concats_to_path4)
{
#if Windows
    fs::path p(R"=(C:\Windows)=");
    p += "notepad.exe"_cs;
    assert_equal_str(p.c_str(), R"=(C:\Windowsnotepad.exe)=");
#else
    fs::path p("/etc");
    p += "passwd"_cs;
    assert_equal_str(p.c_str(), "/etcpasswd");
#endif
}

define_test(canonical_path_gets_the_canonical_path)
{
#if Windows
    fs::path p(R"=(C:\Windows\.\.\notepad.exe)=");
    fs::path p2;

    fs::canonical_path(&p, &p2);

    assert_equal_str(p.c_str(), R"=(C:\Windows\.\.\notepad.exe)=");
    assert_equal_str(p2.c_str(), R"=(C:\Windows\notepad.exe)=");
#else
    fs::path p("/etc/././passwd");
    fs::path p2;

    fs::canonical_path(&p, &p2);

    assert_equal_str(p.c_str(), "/etc/././passwd");
    assert_equal_str(p2.c_str(), "/etc/passwd");
#endif
}

define_test(weakly_canonical_path_gets_the_weakly_canonical_path)
{
#if Windows
    fs::path p(R"=(C:\Windows\.\.\notepad123.exe)=");
    fs::path p2;

    fs::weakly_canonical_path(&p, &p2);

    assert_equal_str(p.c_str(), R"=(C:\Windows\.\.\notepad123.exe)=");
    assert_equal_str(p2.c_str(), R"=(C:\Windows\notepad123.exe)=");
#else
    fs::path p("/etc/././passwd123");
    fs::path p2;

    fs::weakly_canonical_path(&p, &p2);

    assert_equal_str(p.c_str(), "/etc/././passwd123");
    assert_equal_str(p2.c_str(), "/etc/passwd123");
#endif
}

define_test(absolute_path_gets_the_absolute_path)
{
    fs::path p("abc.txt");
    fs::path p2;

    fs::absolute_path(&p, &p2);

    fs::path cur;
    fs::get_current_path(&cur);
    fs::append_path(&cur, "abc.txt");

    assert_equal(p2, cur);
}

define_test(relative_gets_the_relative_path)
{
#if Windows
    fs::path from(R"=(C:\Windows\)=");
    fs::path to(R"=(C:\Windows\notepad.exe)=");
#else
    fs::path from("/etc/");
    fs::path to("/etc/passwd");
#endif

    fs::path rel;

    fs::relative_path(&from, &to, &rel);

#if Windows
    assert_equal_str(rel.c_str(), "notepad.exe");
#else
    assert_equal_str(rel.c_str(), "passwd");
#endif
}

define_test(relative_gets_the_relative_path2)
{
#if Windows
    fs::path from(R"=(C:\a\b\c)=");
    fs::path to(R"=(C:\)=");
#else
    fs::path from("/a/b/c/");
    fs::path to("/");
#endif

    fs::path rel;

    fs::relative_path(&from, &to, &rel);

#if Windows
    assert_equal_str(rel.c_str(), "..\\..\\..");
#else
    assert_equal_str(rel.c_str(), "../../..");
#endif
}

define_test(get_executable_path_gets_executable_path)
{
    // fs::path actual("/home/user/dev/git/fs/bin/tests/path_tests");
    fs::path p;

    get_executable_path(&p);

    // obviously this wont work on all systems
    // assert_equal(p, actual);
}
#endif

define_default_test_main();
