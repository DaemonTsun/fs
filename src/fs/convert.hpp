
#pragma once

#include "shl/string.hpp"
#include "fs/common.hpp"

namespace fs
{
struct path;
// empty because we want to error when _needs_conversion is fed a type it doesn't know of.
template<typename T> struct _needs_conversion { };
template<> struct _needs_conversion<fs::path> { static constexpr bool value = false; };

template<> struct _needs_conversion<c8>      { static constexpr bool value = !is_same(c8, sys_char);  };
template<> struct _needs_conversion<c16>     { static constexpr bool value = !is_same(c16, sys_char); };
template<> struct _needs_conversion<c32>     { static constexpr bool value = !is_same(c32, sys_char); };
template<> struct _needs_conversion<wchar_t> { static constexpr bool value = !is_same(wchar_t, sys_native_char); };

template<> struct _needs_conversion<const_string>    { static constexpr bool value = !is_same(c8, sys_char);  };
template<> struct _needs_conversion<const_u16string> { static constexpr bool value = !is_same(c16, sys_char); };
template<> struct _needs_conversion<const_u32string> { static constexpr bool value = !is_same(c32, sys_char); };
template<> struct _needs_conversion<string>    { static constexpr bool value = !is_same(c8, sys_char);  };
template<> struct _needs_conversion<u16string> { static constexpr bool value = !is_same(c16, sys_char); };
template<> struct _needs_conversion<u32string> { static constexpr bool value = !is_same(c32, sys_char); };
#define needs_conversion(C) fs::_needs_conversion<typename remove_const(typename remove_pointer(C))>::value
}
