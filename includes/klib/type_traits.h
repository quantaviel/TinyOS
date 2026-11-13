#pragma once

namespace klib {

template<typename T, typename U>
inline constexpr bool is_same_v = false;

template<typename T>
inline constexpr bool is_same_v<T, T> = true;

} // namespace klib
