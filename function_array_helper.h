/*
 * Copyright (c) 2022-2026, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef FUNCTION_ARRAY_HELPER_H
#define FUNCTION_ARRAY_HELPER_H

#include "scriptlib.h"

#include <meta>
#include <string_view>

#define LUA_NAMESPACE lua_functions

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MAKE_LUA_MODULE_IMPL(module) c_lua_##module##_
#define MAKE_LUA_MODULE(module) MAKE_LUA_MODULE_IMPL(module)

#define LUA_PREFIX STR(MAKE_LUA_MODULE(LUA_MODULE))

#define MAKE_LUA_NAME_IMPL(module, name) c_lua_##module##_##name
#define MAKE_LUA_NAME(module, name) MAKE_LUA_NAME_IMPL(module, name)
#define LUA_FUNCTION(name, ...) [[maybe_unused]] static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, \
		[[maybe_unused]] duel* const pduel, [[maybe_unused]] LUA_CLASS* const self __VA_OPT__(,) __VA_ARGS__)

#define LUA_STATIC_FUNCTION(name, ...) [[maybe_unused]] static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, [[maybe_unused]] duel* const pduel __VA_OPT__(,) __VA_ARGS__)

#define LUA_FUNCTION_ALIAS(name) [[maybe_unused]] static lua_alias MAKE_LUA_NAME(LUA_MODULE,name)

#define LUA_FUNCTION_EXISTING(name,...) [[maybe_unused]] [[=aliases(__VA_ARGS__)]] static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L)

namespace {
namespace LUA_NAMESPACE {

struct Function {
	int value;
	operator int() {
		return value;
	}
	operator int() const {
		return value;
	}
};

struct Table {
	int value;
	operator int() {
		return value;
	}
	operator int() const {
		return value;
	}
};

using Nil = struct {}*;

namespace Detail {

template <typename T, typename Enable = void>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template<typename T>
[[maybe_unused]] inline constexpr bool is_optional_v = is_optional<T>::value;

template <typename T, typename Enable = void>
struct is_variant : std::false_type {};

template <typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type {};

template<typename T>
[[maybe_unused]] inline constexpr bool is_variant_v = is_variant<T>::value;

template<typename VARIANT_T, typename T>
struct is_variant_member : public std::false_type {};

template<typename T, typename... Args>
struct is_variant_member<std::variant<Args...>, T>
	: public std::disjunction<std::is_same<T, Args>...> {};

template<typename variant, typename T>
[[maybe_unused]] inline constexpr bool is_variant_member_v = is_variant_member<variant, T>::value;

template <typename Arg, typename... Args>
constexpr auto count_trailing_optionals(std::tuple<Arg, Args...>) {
	if constexpr(sizeof...(Args) == 0) {
		return static_cast<int>(is_optional_v<Arg> || (is_variant_v<Arg> && is_variant_member_v<Arg, Nil>));
	} else {
		constexpr auto result = count_trailing_optionals(std::tuple<Args...>{});
		if constexpr(result != sizeof...(Args))
			return result;
		else
			return result + static_cast<int>(is_optional_v<Arg> || (is_variant_v<Arg> && is_variant_member_v<Arg, Nil>));
	}
}

template<typename T, std::enable_if_t<!is_variant_v<T>, int> = 0>
inline constexpr T get_lua(lua_State* L, int idx) {
	using namespace scriptlib;
	// we need to not have type::value_type be evaluated if the type isn't an optional
	auto _ = [](auto a) {
		using type = decltype(a);
		if constexpr(is_optional_v<type>) {
			return typename type::value_type{};
		} else {
			return type{};
		}
	};
	using actual_type = FunctionResult<decltype(_), T>;
	if constexpr(is_optional_v<T>) {
		if(lua_isnoneornil(L, idx))
			return std::nullopt;
	}
	auto return_value = [](auto val) {
		if constexpr(is_optional_v<T>) {
			return std::make_optional<actual_type>(val);
		} else {
			return static_cast<actual_type>(val);
		}
	};
	if constexpr(std::is_same_v<actual_type, lua_obj*>) {
		auto* ret = lua_get<lua_obj*>(L, idx);
		if(!ret) {
			lua_error(L, R"(Parameter %d should be one of "Card", "Group", "Effect" but is "%s".)", idx, get_lua_type_name(L, idx));
		}
		return return_value(ret);
	} else {
		constexpr auto lua_type = [] {
			if constexpr(std::is_same_v<Function, actual_type>)
				return LuaParam::FUNCTION;
			else if constexpr(std::is_same_v<Table, actual_type>)
				return LuaParam::TABLE;
			else
				return get_lua_param_type<actual_type>();
		}();
		if constexpr(lua_type == LuaParam::CARD || lua_type == LuaParam::GROUP || lua_type == LuaParam::EFFECT) {
			lua_obj* ret = nullptr;
			check_param<lua_type>(L, idx, &ret);
			return return_value(reinterpret_cast<actual_type>(ret));
		} else {
			check_param<lua_type>(L, idx);
			if constexpr(lua_type == LuaParam::BOOLEAN) {
				return return_value(static_cast<bool>(lua_toboolean(L, idx)));
			} else if constexpr(lua_type == LuaParam::INT) {
				if(lua_isinteger(L, idx))
					return return_value(static_cast<actual_type>(lua_tointeger(L, idx)));
				return return_value(static_cast<actual_type>(static_cast<lua_Integer>(std::round(lua_tonumber(L, idx)))));
			} else if constexpr(lua_type == LuaParam::FUNCTION) {
				return return_value(Function{ idx });
			} else if constexpr(lua_type == LuaParam::TABLE) {
				return return_value(Table{ idx });
			}
		}
	}
}

template<typename variant_t>
struct check_variant_types_functor;

template<typename... Args>
struct check_variant_types_functor<std::variant<Args...>> {
	constexpr bool operator()(LuaParam lua_type) {
		using namespace scriptlib;
		if constexpr((IsCard<Args> || ...) || (IsLuaObj<Args> || ...)) {
			if(lua_type == LuaParam::CARD)
				return true;
		}
		if constexpr((IsGroup<Args> || ...) || (IsLuaObj<Args> || ...)) {
			if(lua_type == LuaParam::GROUP)
				return true;
		}
		if constexpr((IsEffect<Args> || ...) || (IsLuaObj<Args> || ...)) {
			if(lua_type == LuaParam::EFFECT)
				return true;
		}
		if constexpr((std::is_same_v<Function, Args> || ...)) {
			if(lua_type == LuaParam::FUNCTION)
				return true;
		}
		if constexpr((std::is_same_v<Table, Args> || ...)) {
			if(lua_type == LuaParam::TABLE)
				return true;
		}
		if constexpr((IsBool<Args> || ...)) {
			if(lua_type == LuaParam::BOOLEAN)
				return true;
		}
		if constexpr((IsInteger<Args> || ...)) {
			if(lua_type == LuaParam::INT)
				return true;
		}
		if constexpr((std::is_same_v<Nil, Args> || ...)) {
			if(lua_type == LuaParam::NIL || lua_type == LuaParam::NONE)
				return true;
		}
		return false;		
	}
};

template<typename variant_t>
struct get_variant_type_functor;

template<typename... Args>
struct get_variant_type_functor<std::variant<Args...>> {
	using variant_t = std::variant<Args...>;
	template<typename T>
	static inline constexpr bool is_handled_variant_type = scriptlib::IsCard<T> || scriptlib::IsGroup<T> ||
		scriptlib::IsEffect<T> || scriptlib::IsLuaObj<T> || std::is_same_v<Function, T> ||
		std::is_same_v<Table, T> || scriptlib::IsBool<T> || scriptlib::IsInteger<T> || std::is_same_v<Nil, T>;
	constexpr variant_t operator()(lua_State* L, int idx, LuaParam lua_type) {
		static_assert(((is_handled_variant_type<Args> * 1) +...) == std::variant_size_v<variant_t>, "Unhandled variant type passed");
		using namespace scriptlib;
		if constexpr((IsCard<Args> || ...)) {
			if(lua_type == LuaParam::CARD)
				return reinterpret_cast<card*>(lua_get<lua_obj*>(L, idx));
		}
		if constexpr((IsGroup<Args> || ...)) {
			if(lua_type == LuaParam::GROUP)
				return reinterpret_cast<group*>(lua_get<lua_obj*>(L, idx));
		}
		if constexpr((IsEffect<Args> || ...)) {
			if(lua_type == LuaParam::EFFECT)
				return reinterpret_cast<effect*>(lua_get<lua_obj*>(L, idx));
		}
		if constexpr((IsLuaObj<Args> || ...)) {
			if(lua_type == LuaParam::CARD || lua_type == LuaParam::EFFECT || lua_type == LuaParam::GROUP)
				return lua_get<lua_obj*>(L, idx);
		}
		if constexpr((std::is_same_v<Function, Args> || ...)) {
			if(lua_type == LuaParam::FUNCTION)
				return Function{ idx };
		}
		if constexpr((std::is_same_v<Table, Args> || ...)) {
			if(lua_type == LuaParam::TABLE)
				return Table{ idx };
		}
		if constexpr((IsBool<Args> || ...)) {
			if(lua_type == LuaParam::BOOLEAN)
				return static_cast<bool>(lua_toboolean(L, idx));
		}
		if constexpr((IsInteger<Args> || ...)) {
			static_assert((IsInteger<Args> +...) <= 1, "Variant must have at most 1 integer type");
			if(lua_type == LuaParam::INT) {
				constexpr auto int_index = [] {
					int i = 0;
					int elem = 0;
					// do this comma operator spam to avoid sequence point warnings
					((!IsInteger<Args> || (elem = i), ++i), ...);
					return elem;
				}();
				using integer_type = std::tuple_element_t<int_index, std::tuple<Args...>>;
				auto val = lua_isinteger(L, idx) ? lua_tointeger(L, idx) : static_cast<lua_Integer>(std::round(lua_tonumber(L, idx)));
				return static_cast<integer_type>(val);
			}
		}
		if constexpr((std::is_same_v<Nil, Args> || ...)) {
			if(lua_type == LuaParam::NIL || lua_type == LuaParam::NONE)
				return Nil{};
		}
		unreachable();
	}
};

template<typename variant_t>
struct get_variant_names_functor;

template<typename... Args>
struct get_variant_names_functor<std::variant<Args...>> {
	constexpr std::array<char, 128> operator()() {
		using namespace scriptlib;
		std::array<char, 128> ret{};
		auto it = ret.begin();
		bool is_first = true;
		auto copy_string = [&](const char* string) {
			if(!is_first) {
				*it++ = ',';
				*it++ = ' ';
			}
			is_first = false;
			*it++ = '"';
			while(*string) {
				*it++ = *string++;
			}
			*it++ = '"';
		};
		if constexpr((IsCard<Args> || ...) || (IsLuaObj<Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::CARD>());
		}
		if constexpr((IsGroup<Args> || ...) || (IsLuaObj<Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::GROUP>());
		}
		if constexpr((IsEffect<Args> || ...) || (IsLuaObj<Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::EFFECT>());
		}
		if constexpr((std::is_same_v<Function, Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::FUNCTION>());
		}
		if constexpr((std::is_same_v<Table, Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::TABLE>());
		}
		if constexpr((IsBool<Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::BOOLEAN>());
		}
		if constexpr((IsInteger<Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::INT>());
		}
		if constexpr((std::is_same_v<Nil, Args> || ...)) {
			copy_string(get_lua_param_name<LuaParam::NIL>());
		}
		return ret;
	}
};

template<typename T, std::enable_if_t<is_variant_v<T>, int> = 0>
inline constexpr T get_lua(lua_State* L, int idx) {
	using namespace scriptlib;
	auto type = get_lua_type(L, idx);
	if(!check_variant_types_functor<T>()(type)) {
		constexpr auto types_string = get_variant_names_functor<T>()();
		static_assert(types_string.back() == '\0');
		lua_error(L, R"(Parameter %d should be one of %s but is "%s".)", idx, types_string.data(), get_lua_type_name(L, idx));
	}
	return get_variant_type_functor<T>()(L, idx, type);
}

template<typename Sig>
struct get_function_arguments;

template<typename Ret, typename... Args>
struct get_function_arguments<Ret(*)(Args...)> {
	using type = std::tuple<Args...>;
};
template<typename Ret, typename... Args>
struct get_function_arguments<Ret(*const)(Args...)> {
	using type = std::tuple<Args...>;
};
template<typename Sig>
using get_function_arguments_t = typename get_function_arguments<Sig>::type;

template<typename tuple, size_t tuple_offset, size_t lua_offset, size_t... indices>
constexpr decltype(auto) parse_helper([[maybe_unused]] lua_State* L, std::index_sequence<indices...>) {
	return std::make_tuple(get_lua<std::tuple_element_t<indices + tuple_offset, tuple>>(L, indices + lua_offset + 1)...);
}

template<typename tuple, size_t tuple_offset, size_t lua_offset = 0>
decltype(auto) parse_arguments_tuple(lua_State* L) {
	return parse_helper<tuple, tuple_offset, lua_offset>(L, std::make_index_sequence<std::tuple_size_v<tuple> - tuple_offset>{});
}

} // namespace Detail
} // namespace LUA_NAMESPACE

struct aliases { lua_CFunction field; };

struct lua_alias { };
template<std::meta::info Namespace>
constexpr auto get_lua_functions() {
	constexpr auto lua_symbols = [] {
		constexpr auto ctx = std::meta::access_context::current();
		std::vector<std::meta::info> res;
		template for (constexpr auto M : define_static_array(members_of(Namespace, ctx))) {
			if constexpr(identifier_of(M).starts_with(LUA_PREFIX)) {
				res.push_back(M);
			}
		}
		return define_static_array(res);
	}();
	std::array<luaL_Reg, lua_symbols.size()+1> ret_array{};
	int i = 0;
    template for (constexpr auto M : define_static_array(lua_symbols)) {
		constexpr auto identifier = identifier_of(M).substr(std::string_view{LUA_PREFIX}.size());
		if constexpr(is_function(M)) {
			constexpr auto annotations = define_static_array(annotations_of_with_type(M, ^^aliases));
			static_assert(annotations.size() <= 1);
			if constexpr(annotations.size() == 1) {
				constexpr auto annotation = extract<aliases>(annotations[0]);
				static_assert(annotation.field != nullptr);
				ret_array[i] = luaL_Reg{identifier.data(), annotation.field};
			} else {
				static constexpr auto func_ptr = [:M:];
				static constexpr auto parameters = define_static_array(parameters_of(M));
				static constexpr auto num_of_parameters = parameters.size();
				ret_array[i] = luaL_Reg{identifier.data(), [](lua_State* L) -> int {
					using namespace scriptlib;
					if constexpr(num_of_parameters == 2) {
						return func_ptr(L, lua_get<duel*>(L));
					} else {
						using function_arguments = LUA_NAMESPACE::Detail::get_function_arguments_t<decltype(func_ptr)>;
						static constexpr auto extra_args = 2;
						static constexpr int required_args = (static_cast<int>(num_of_parameters) - LUA_NAMESPACE::Detail::count_trailing_optionals(function_arguments{})) - extra_args;
						if constexpr(required_args > 0)
							check_param_count(L, required_args);
						return std::apply(func_ptr,
							std::tuple_cat(
								std::make_tuple(L, lua_get<duel*>(L)),
								LUA_NAMESPACE::Detail::parse_arguments_tuple<function_arguments, extra_args>(L)
							)
						);
					}
				}};
			}
		} else if constexpr(is_variable(M)) {
			static_assert(type_of(M) == ^^lua_alias);
			ret_array[i] = luaL_Reg{identifier.data(), ret_array[i - 1].func};
		}
		i++;
    }
	ret_array[i] = {nullptr,nullptr};
	return ret_array;
}
}

#undef LUA_PREFIX
#undef STR
#undef STR_HELPER
#undef MAKE_LUA_MODULE_IMPL
#undef MAKE_LUA_MODULE

#define GET_LUA_FUNCTIONS_ARRAY() get_lua_functions<^^LUA_NAMESPACE>()

#endif // FUNCTION_ARRAY_HELPER_H
