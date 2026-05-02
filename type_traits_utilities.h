/*
 * Copyright (c) 2026, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef TEMPLATE_UTILITIES_H
#define TEMPLATE_UTILITIES_H

#include <optional>
#include <string_view>
#include <type_traits> //std::disjunction, std::false_type, std::is_same, std::true_type
#include <variant>

template<typename T>
struct is_optional : std::false_type {};

template<typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template<typename T>
[[maybe_unused]] inline constexpr bool is_optional_v = is_optional<T>::value;

template<typename T>
struct is_variant : std::false_type {};

template<typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type {};

template<typename T>
[[maybe_unused]] inline constexpr bool is_variant_v = is_variant<T>::value;

template<typename T>
struct is_string_view : std::false_type {};

template<>
struct is_string_view<std::string_view> : std::true_type {};

template<typename T>
[[maybe_unused]] inline constexpr bool is_string_view_v = is_string_view<T>::value;

template<typename VARIANT_T, typename T>
struct is_variant_member : public std::false_type {};

template<typename T, typename... Args>
struct is_variant_member<std::variant<Args...>, T>
	: public std::disjunction<std::is_same<T, Args>...> {};

template<typename variant, typename T>
[[maybe_unused]] inline constexpr bool is_variant_member_v = is_variant_member<variant, T>::value;

#endif /* TEMPLATE_UTILITIES_H */
