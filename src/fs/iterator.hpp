
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

    const fs::path *operator*() const;
    fs::iterator &operator++();
};

bool operator==(const fs::iterator &lhs, const fs::iterator &rhs);
bool operator/=(const fs::iterator &lhs, const fs::iterator &rhs);

fs::iterator iterate(const char *path, bool recursive = false);
fs::iterator iterate(const wchar_t *path, bool recursive = false);
fs::iterator iterate(const fs::path *path, bool recursive = false);

fs::iterator begin(const fs::iterator &it);
fs::iterator end(const fs::iterator &it);
}
