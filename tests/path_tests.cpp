
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

define_test(canonical_path_gets_the_canonical_path)
{
#if Linux
    fs::path p("/etc/././passwd");
    fs::path p2;

    fs::canonical_path(&p, &p2);

    assert_equal_str(p.c_str(), "/etc/././passwd");
    assert_equal_str(p2.c_str(), "/etc/passwd");
#else
#error "add test"
#endif
}

define_test(weakly_canonical_path_gets_the_weakly_canonical_path)
{
#if Linux
    fs::path p("/etc/././passwd123");
    fs::path p2;

    fs::weakly_canonical_path(&p, &p2);

    assert_equal_str(p.c_str(), "/etc/././passwd123");
    assert_equal_str(p2.c_str(), "/etc/passwd123");
#else
#error "add test"
#endif
}

define_test(absolute_path_gets_the_absolute_path)
{
#if Linux
    fs::path p("abc.txt");
    fs::path p2;

    fs::absolute_path(&p, &p2);

    fs::path cur;
    fs::get_current_path(&cur);
    fs::append_path(&cur, "abc.txt");

    assert_equal(p2, cur);
#else
#error "add test"
#endif
}

define_test(relative_gets_the_relative_path)
{
#if Linux
    fs::path from("/etc/");
    fs::path to("/etc/passwd");
    fs::path rel;

    fs::relative_path(&from, &to, &rel);

    assert_equal_str(rel.c_str(), "passwd");
#else
#error "add test"
#endif
}

define_test(relative_gets_the_relative_path2)
{
#if Linux
    fs::path from("/a/b/c/");
    fs::path to("/");
    fs::path rel;

    fs::relative_path(&from, &to, &rel);

    assert_equal_str(rel.c_str(), "../../..");
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
