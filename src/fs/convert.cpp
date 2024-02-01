
#include <stdlib.h>

#include "shl/memory.hpp"

#include "fs/convert.hpp"

void fs::free(fs::converted_string<char> *str)
{
    ::free_memory(str->data);
}

void fs::free(fs::converted_string<wchar_t> *str)
{
    ::free_memory(str->data);
}

fs::converted_string<char> fs::convert_string(const wchar_t *wcstring, u64 wchar_count)
{
    fs::converted_string<char> ret;
    u64 sz = (wchar_count + 1) * sizeof(char);
    ret.data = (char*)::allocate_memory(sz);

    ::fill_memory((void*)ret.data, 0, sz);

    ret.size = ::wcstombs(ret.data, wcstring, wchar_count * sizeof(wchar_t));

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

fs::converted_string<wchar_t> fs::convert_string(const char *cstring, u64 char_count)
{
    fs::converted_string<wchar_t> ret;
    u64 sz = (char_count + 1) * sizeof(wchar_t);
    ret.data = (wchar_t*)::allocate_memory(sz);

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
