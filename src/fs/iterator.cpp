
#include <filesystem>
#include <variant>

#include "fs/impl/path.hpp"
#include "fs/iterator.hpp"

typedef std::filesystem::directory_iterator regular_it;
typedef std::filesystem::recursive_directory_iterator recursive_it;
typedef std::variant<regular_it, recursive_it> iterator_t;

struct fs::iterator::_iterator
{
    fs::path cursor;
    iterator_t it;
};

fs::iterator::iterator()
{
    this->ptr = nullptr;
}

void copy(const fs::iterator *from, fs::iterator *to)
{
    if (from->ptr == nullptr)
    {
        to->ptr = nullptr;
        return;
    }

    to->ptr = new fs::iterator::_iterator;
    to->ptr->it = from->ptr->it;
}

fs::iterator::iterator(const fs::iterator &other)
{
    ::copy(&other, this);
}

fs::iterator::iterator(fs::iterator &&other)
{
    this->ptr = other.ptr;
    other.ptr = nullptr;
}

fs::iterator::~iterator()
{
    if (this->ptr != nullptr)
        delete this->ptr;

    this->ptr = nullptr;
}

fs::iterator &fs::iterator::operator=(const fs::iterator &other)
{
    ::copy(&other, this);
    return *this;
}

fs::iterator &fs::iterator::operator=(fs::iterator &&other)
{
    this->ptr = other.ptr;
    other.ptr = nullptr;

    return *this;
}

fs::iterator fs::iterate(const char *path, bool recursive)
{
    fs::path pth(path);
    return fs::iterate(&pth, recursive);
}

fs::iterator fs::iterate(const wchar_t *path, bool recursive)
{
    fs::path pth(path);
    return fs::iterate(&pth, recursive);
}

fs::iterator iterate(const_string  path, bool recursive)
{
    fs::path pth(path);
    return fs::iterate(&pth, recursive);
}

fs::iterator iterate(const_wstring path, bool recursive)
{
    fs::path pth(path);
    return fs::iterate(&pth, recursive);
}

fs::iterator iterate(const string  *path, bool recursive)
{
    fs::path pth(path);
    return fs::iterate(&pth, recursive);
}

fs::iterator iterate(const wstring *path, bool recursive)
{
    fs::path pth(path);
    return fs::iterate(&pth, recursive);
}

fs::iterator fs::iterate(const fs::path *path, bool recursive)
{
    fs::iterator ret;
    ret.ptr = new fs::iterator::_iterator;
    
    if (recursive)
        ret.ptr->it = recursive_it(path->ptr->data);
    else
        ret.ptr->it = regular_it(path->ptr->data);

    return ret;
}

void fs::advance(fs::iterator *iter)
{
    if (std::holds_alternative<regular_it>(iter->ptr->it))
    {
        auto &it = std::get<regular_it>(iter->ptr->it);
        it++;
    }
    else
    {
        auto &it = std::get<recursive_it>(iter->ptr->it);
        it++;
    }
}

bool fs::is_at_end(const fs::iterator *iter)
{
    if (std::holds_alternative<regular_it>(iter->ptr->it))
    {
        auto &it = std::get<regular_it>(iter->ptr->it);
        
        if (it == std::filesystem::end(it))
            return true;
    }
    else
    {
        auto &it = std::get<recursive_it>(iter->ptr->it);
        
        if (it == std::filesystem::end(it))
            return true;
    }

    return false;
}

const fs::path *fs::current(const fs::iterator *iter)
{
    if (fs::is_at_end(iter))
        return nullptr;

    if (std::holds_alternative<regular_it>(iter->ptr->it))
    {
        auto &it = std::get<regular_it>(iter->ptr->it);
        
        const std::filesystem::directory_entry &dir_entry = *it;
        iter->ptr->cursor.ptr->data = std::move(dir_entry.path());
    }
    else
    {
        auto &it = std::get<recursive_it>(iter->ptr->it);
        
        const std::filesystem::directory_entry &dir_entry = *it;
        iter->ptr->cursor.ptr->data = std::move(dir_entry.path());
    }

    return &iter->ptr->cursor;
}

