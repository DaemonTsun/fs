
#pragma once

/* iterator.hpp
 *
 * use fs::iterate(path, recursive = false) in a for range loop, for example:
 *
 * fs::path p("/home");
 *
 * for (fs::path *i : fs::iterate(&p))
 *     printf("%s\n", i->c_str());
 *
 * to iterate recursively, set the recursive parameter to true.
 */

#include "fs/path.hpp"

namespace fs
{
struct iterator
{
    struct _iterator;
    _iterator *ptr;

    iterator();
    iterator(const fs::iterator &other);
    iterator(fs::iterator &&other);

    ~iterator();

    fs::iterator &operator=(const fs::iterator &other);
    fs::iterator &operator=(fs::iterator &&other);
};

fs::iterator iterate(const char *path, bool recursive = false);
fs::iterator iterate(const wchar_t *path, bool recursive = false);
fs::iterator iterate(const fs::path *path, bool recursive = false);

void advance(fs::iterator *it);
bool is_at_end(const fs::iterator *it);
const fs::path *current(const fs::iterator *it);

#define iterate_path(Var, Path, ...)\
    fs::iterator Var##_it = fs::iterate(Path __VA_OPT__(,) __VA_ARGS__);\
    for (const fs::path *Var = fs::current(&Var##_it); !fs::is_at_end(&Var##_it); fs::advance(&Var##_it), Var = fs::current(&Var##_it))

}
