
#include <stdlib.h>

#include "shl/memory.hpp"

#include "fs/convert.hpp"

#if 0
void fs::free(fs::converted_string<char> *str)
{
    ::dealloc(str->data, str->buffer_size);
}

void fs::free(fs::converted_string<wchar_t> *str)
{
    ::dealloc(str->data, str->buffer_size);
}

fs::converted_string<char> fs::convert_string(const wchar_t *cstring, s64 wchar_count)
{
    fs::converted_string<char> ret;
    s64 sz = (wchar_count + 1) * sizeof(char);
    ret.data = (char*)::alloc(sz);
    ret.buffer_size = sz;

    ::fill_memory((void*)ret.data, 0, sz);

    ret.size = ::wcstombs(ret.data, cstring, wchar_count * sizeof(wchar_t));

    return ret;
}

fs::converted_string<char> fs::convert_string(const wchar_t *cstring)
{
    return fs::convert_string(cstring, ::string_length(cstring));
}

fs::converted_string<char> fs::convert_string(const_wstring cstring)
{
    return fs::convert_string(cstring.c_str, cstring.size);
}

fs::converted_string<wchar_t> fs::convert_string(const char *cstring, s64 char_count)
{
    fs::converted_string<wchar_t> ret;
    s64 sz = (char_count + 1) * sizeof(wchar_t);
    ret.data = (wchar_t*)::alloc(sz);
    ret.buffer_size = sz;

    ::fill_memory((void*)ret.data, 0, sz);

    ret.size = ::mbstowcs(ret.data, cstring, char_count * sizeof(char));

    return ret;
}

fs::converted_string<wchar_t> fs::convert_string(const char *cstring)
{
    return fs::convert_string(cstring, ::string_length(cstring));
}

fs::converted_string<wchar_t> fs::convert_string(const_string cstring)
{
    return fs::convert_string(cstring.c_str, cstring.size);
}
#endif
