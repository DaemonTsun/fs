
#include <t1/t1.hpp>

#include "shl/string.hpp"
#include "shl/platform.hpp"
#include "fs/path.hpp"

#define assert_equal_str(STR1, STR2)\
    assert_equal(compare_strings(STR1, STR2), 0)

define_test(filename_returns_the_filename)
{
#if Linux
    fs::path p("/this/is/a/test");

    assert_equal_str(fs::filename(&p), "test");
#else
#error "add test"
#endif
}

define_test(parent_path_returns_the_parent_path)
{
#if Linux
    fs::path p("/this/is/a/test");
    fs::path parent;

    fs::parent_path(&p, &parent);

    assert_equal_str(parent.c_str(), "/this/is/a");
#else
#error "add test"
#endif
}

define_test(parent_path_returns_the_parent_path2)
{
#if Linux
    fs::path p("/");
    fs::path parent;

    fs::parent_path(&p, &parent);

    assert_equal_str(parent.c_str(), "/");
#else
#error "add test"
#endif
}

define_test(exists_returns_true_if_path_exists)
{
#if Linux
    fs::path p("/");

    assert_equal(fs::exists(&p), true);
#else
#error "add test"
#endif
}

define_test(exists_returns_false_if_path_doesnt_exists)
{
#if Linux
    fs::path p("/abc");

    assert_equal(fs::exists(&p), false);
#else
#error "add test"
#endif
}

define_test(is_file_returns_true_if_path_is_file)
{
#if Linux
    fs::path p("/etc/passwd");

    assert_equal(fs::is_file(&p), true);
#else
#error "add test"
#endif
}

define_test(is_file_returns_false_if_path_is_not_file)
{
#if Linux
    fs::path p("/etc/");

    assert_equal(fs::is_file(&p), false);
#else
#error "add test"
#endif
}

define_test(is_directory_returns_true_if_path_is_directory)
{
#if Linux
    fs::path p("/etc/");

    assert_equal(fs::is_directory(&p), true);
#else
#error "add test"
#endif
}

define_test(is_directory_returns_false_if_path_is_not_directory)
{
#if Linux
    fs::path p("/etc/passwd");

    assert_equal(fs::is_directory(&p), false);
#else
#error "add test"
#endif
}

define_test(is_absolute_returns_true_if_path_is_absoltue)
{
#if Linux
    fs::path p("/etc/passwd");

    assert_equal(fs::is_absolute(&p), true);
#else
#error "add test"
#endif
}

define_test(is_absolute_returns_false_if_path_is_not_absolute)
{
#if Linux
    fs::path p("../passwd");

    assert_equal(fs::is_absolute(&p), false);
#else
#error "add test"
#endif
}

define_test(is_relative_returns_true_if_path_is_relative)
{
#if Linux
    fs::path p("../passwd");

    assert_equal(fs::is_relative(&p), true);
#else
#error "add test"
#endif
}

define_test(is_relative_returns_false_if_path_is_not_relative)
{
#if Linux
    fs::path p("/etc/passwd");

    assert_equal(fs::is_relative(&p), false);
#else
#error "add test"
#endif
}

define_test(append_appends_to_path)
{
#if Linux
    fs::path p("/etc");

    fs::append_path(&p, "passwd");

    assert_equal_str(p.c_str(), "/etc/passwd");
#else
#error "add test"
#endif
}

define_test(append_appends_to_path2)
{
#if Linux
    fs::path p("/etc");
    fs::path p2;

    fs::append_path(&p, "passwd", &p2);

    assert_equal_str(p.c_str(), "/etc");
    assert_equal_str(p2.c_str(), "/etc/passwd");
#else
#error "add test"
#endif
}

define_test(concat_concats_to_path)
{
#if Linux
    fs::path p("/etc");

    fs::concat_path(&p, "passwd");

    assert_equal_str(p.c_str(), "/etcpasswd");
#else
#error "add test"
#endif
}

define_test(concat_concats_to_path2)
{
#if Linux
    fs::path p("/etc");
    fs::path p2;

    fs::concat_path(&p, "passwd", &p2);

    assert_equal_str(p.c_str(), "/etc");
    assert_equal_str(p2.c_str(), "/etcpasswd");
#else
#error "add test"
#endif
}

define_test(get_executable_path_gets_executable_path)
{
    fs::path actual("/home/user/dev/git/fs/bin/tests/path_tests");
    fs::path p;

    get_executable_path(&p);

    // obviously this wont work on all systems
    assert_equal(p, actual);
}

define_default_test_main();
