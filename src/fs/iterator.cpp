
#include <filesystem>

#include "fs/impl/path.hpp"
#include "fs/iterator.hpp"

struct fs::iterator::_iterator
{
    bool recursive;
    fs::path cursor;

    struct /*union*/
    {
        std::filesystem::directory_iterator regular;
        std::filesystem::recursive_directory_iterator recursive;
    } it;
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
    to->ptr->recursive = from->ptr->recursive;

    if (from->ptr->recursive)
        to->ptr->it.recursive = from->ptr->it.recursive;
    else
        to->ptr->it.regular = from->ptr->it.regular;

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

const fs::path *fs::iterator::operator*() const
{
    if (this->ptr->recursive)
    {
        if (this->ptr->it.recursive == std::filesystem::end(this->ptr->it.recursive))
            return nullptr;

        const std::filesystem::directory_entry &dir_entry = *(this->ptr->it.recursive);

        this->ptr->cursor.ptr->data = dir_entry.path();
    }
    else
    {
        if (this->ptr->it.regular == std::filesystem::end(this->ptr->it.regular))
            return nullptr;
        
        const std::filesystem::directory_entry &dir_entry = *(this->ptr->it.regular);

        this->ptr->cursor.ptr->data = dir_entry.path();
    }

    return &this->ptr->cursor;
}

fs::iterator &fs::iterator::operator++()
{
    if (this->ptr->recursive)
        this->ptr->it.recursive++;
    else
        this->ptr->it.regular++;

    return *this;
}

bool fs::operator==(const fs::iterator &lhs, const fs::iterator &rhs)
{
    bool ret = lhs.ptr->recursive == rhs.ptr->recursive;

    if (!ret)
        return ret;

    if (lhs.ptr->recursive)
        return lhs.ptr->it.recursive == rhs.ptr->it.recursive;
    else
        return lhs.ptr->it.regular == rhs.ptr->it.regular;
}

bool fs::operator/=(const fs::iterator &lhs, const fs::iterator &rhs)
{
    return !(lhs == rhs);
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

fs::iterator fs::iterate(const fs::path *path, bool recursive)
{
    fs::iterator ret;
    ret.ptr = new fs::iterator::_iterator;
    ret.ptr->recursive = recursive;
    
    if (recursive)
        ret.ptr->it.recursive = std::filesystem::recursive_directory_iterator(path->ptr->data);
    else
        ret.ptr->it.regular = std::filesystem::directory_iterator(path->ptr->data);

    return ret;
}

fs::iterator fs::begin(const fs::iterator &it)
{
    fs::iterator ret = it;

    if (ret.ptr->recursive)
        ret.ptr->it.recursive = std::filesystem::begin(it.ptr->it.recursive);
    else
        ret.ptr->it.regular = std::filesystem::begin(it.ptr->it.regular);

    return ret;
}

fs::iterator fs::end(const fs::iterator &it)
{
    fs::iterator ret = it;

    if (ret.ptr->recursive)
        ret.ptr->it.recursive = std::filesystem::end(it.ptr->it.recursive);
    else
        ret.ptr->it.regular = std::filesystem::end(it.ptr->it.regular);

    return ret;
}
