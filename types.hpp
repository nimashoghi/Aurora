#pragma once

#include <iostream>
#include <string>
#include <type_traits>

using namespace std;

#if defined(NDEBUG)
#define assert(condition) \
    (void)(condition);
#else
#define assert(condition) \
    if (!(condition)) \
    { \
        exit(GetLastError()); \
    }
#endif

#define nt_assert(success_condition) \
    assert(SUCCEEDED(success_condition))

#define fn(fn_name) static const auto fn_name = [&]
#define closure fn

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using i8 = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

using f32 = float;
using f64 = double;
