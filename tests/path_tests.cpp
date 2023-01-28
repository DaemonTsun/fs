
#include <t1/t1.hpp>

#include "fs/path.hpp"

define_test(get_executable_path_gets_executable_path)
{
    fs::path actual("/home/user/dev/git/fs/bin/tests/path_tests");
    fs::path p;

    get_executable_path(&p);

    // obviously this wont work on all systems
    assert_equal(p, actual);
}

define_default_test_main();
