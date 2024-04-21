
#pragma once

#include "shl/string.hpp"
#include "shl/number_types.hpp"
#include "shl/type_functions.hpp"

#include "fs/common.hpp"

namespace fs
{
struct path;

template<typename T>
struct converted_string
{
    T *data;
    s64 size;
    s64 buffer_size;
};

void free(fs::converted_string<char> *str);
void free(fs::converted_string<wchar_t> *str);

typedef fs::converted_string<fs::path_char_t> platform_converted_string;

// we want to error when _needs_conversion is fed a type it doesn't know of.
template<typename T> struct _needs_conversion { };
template<> struct _needs_conversion<fs::path> { static constexpr bool value = false; };

#if Windows
template<> struct _needs_conversion<char>           { static constexpr bool value = true;  };
template<> struct _needs_conversion<const_string>   { static constexpr bool value = true;  };
template<> struct _needs_conversion<string>         { static constexpr bool value = true;  };
template<> struct _needs_conversion<wchar_t>        { static constexpr bool value = false; };
template<> struct _needs_conversion<const_wstring>  { static constexpr bool value = false;  };
template<> struct _needs_conversion<wstring>        { static constexpr bool value = false;  };
#else
template<> struct _needs_conversion<char>           { static constexpr bool value = false;  };
template<> struct _needs_conversion<const_string>   { static constexpr bool value = false;  };
template<> struct _needs_conversion<string>         { static constexpr bool value = false;  };
template<> struct _needs_conversion<wchar_t>        { static constexpr bool value = true; };
template<> struct _needs_conversion<const_wstring>  { static constexpr bool value = true;  };
template<> struct _needs_conversion<wstring>        { static constexpr bool value = true;  };
#endif
#define needs_conversion(C) fs::_needs_conversion<typename remove_const(typename remove_pointer(C))>::value

fs::converted_string<char>    convert_string(const wchar_t *cstring, s64 wchar_count);
fs::converted_string<char>    convert_string(const wchar_t *cstring);
fs::converted_string<char>    convert_string(const_wstring  cstring);
fs::converted_string<wchar_t> convert_string(const char  *cstring, s64 char_count);
fs::converted_string<wchar_t> convert_string(const char  *cstring);
fs::converted_string<wchar_t> convert_string(const_string cstring);
}

inline const_string_base<fs::path_char_t> to_const_string(fs::platform_converted_string str)
{
    return const_string_base<fs::path_char_t>{.c_str = str.data, .size = str.size};
}
