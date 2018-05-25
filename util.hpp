#pragma once

#include <array>
#include <type_traits>

using namespace std;

namespace util
{
    template<typename First, typename ...Rest>
    struct are_all_same: conjunction<is_same<First, Rest>...>
    {
    };

    template<typename ...Ts>
    constexpr auto are_all_same_v = are_all_same<Ts...>::value;

    template<typename ...Args>
    inline auto make_array(Args &&...args)
    {
        static_assert(are_all_same_v<Args...>, "Array elements must all be of same type.");
        return array<common_type_t<decltype(forward<Args>(args))...>, sizeof...(Args)>{forward<Args>(args)...};
    }

    template<typename T, typename ...Args>
    inline auto make_array_of(Args &&...args)
    {
        static_assert(conjunction_v<is_convertible<Args, T>...>, "Input types are not convertible to target type.");
        return array<T, sizeof...(Args)>{static_cast<decltype(forward<T>(declval<T>()))>(forward<Args>(args))...};
    }
}
