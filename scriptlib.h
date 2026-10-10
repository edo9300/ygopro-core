/*
 * Copyright (c) 2016-2026, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef SCRIPTLIB_H_
#define SCRIPTLIB_H_

#include <any>
#include <array>
#include <cmath> //std::round
#include <cstdint>
#include <cstring> //std::memcpy
#include <functional> //std::ref
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <optional>
#include <tuple>
#include <type_traits> //std::is_same_v, std::is_trivially_destructible_v, std::enable_if_t, std::invoke_result_t, std::result_of_t, std::conditional_t, std::is_unsigned_v
#include <utility> //std::move, std::pair
#include <variant>
#include "common.h"
#include "lua_obj.h"
#include "type_traits_utilities.h"

class card;
class effect;
class group;
class duel;

static_assert(LUA_VERSION_NUM >= 503 && LUA_VERSION_NUM <= 505, "Lua 5.3, 5.4 or 5.5 is required, the core won't work with other lua versions");
static_assert(LUA_MAXINTEGER >= INT64_MAX, "Lua has to support 64 bit integers");
static_assert(LUA_EXTRASPACE >= sizeof(duel*), "LUA_EXTRASPACE needs to be big enough to hold a pointer to the duel object");

namespace scriptlib {
	using LuaRet = std::variant<int, const char*, std::function<int(lua_State*)>>;

	void push_card_lib(lua_State* L);
	void push_effect_lib(lua_State* L);
	void push_group_lib(lua_State* L);
	void push_duel_lib(lua_State* L);
	void push_debug_lib(lua_State* L);
	bool is_in_noaction_state(lua_State* L);
	int push_return_cards(lua_State* L, int32_t status, lua_KContext ctx);
	inline LuaRet push_return_cards([[maybe_unused]] lua_State* L, bool cancelable) {
		return [cancelable](lua_State* L)->int32_t { return lua_yieldk(L, 0, static_cast<lua_KContext>(cancelable), push_return_cards); };
	}
	LuaRet is_deleted_object(lua_State* L);
	std::any* set_any_temp_storage(lua_State* L, std::any&& storage);
	void clear_any_temp_storage(lua_State* L);

#define lua_error_unsafe(...) do { luaL_error(__VA_ARGS__); unreachable(); } while(0)

	template<typename... Args>
	inline LuaRet parse_lua_error(const char* format, Args... args) {
		if constexpr(sizeof...(Args) == 0) {
			return format;
		} else {
			return [=](lua_State* L) -> int32_t { lua_error_unsafe(L, format, args...); };
		}
	}
#define lua_error(dummy,...) return parse_lua_error(__VA_ARGS__)
#define check_action_permission(L) do { if(is_in_noaction_state(L)) lua_error(L, "Action is not allowed here."); } while(0)
#define check_param_count(L, count) do { if(lua_gettop(L) < count) lua_error(L, "%d Parameters are needed.", count); } while(0)

	using playerid_t = RangedInteger<uint8_t, 0, 1>;

	using playerid_none_t = RangedInteger<uint8_t, 0, PLAYER_NONE>;

	using playerid_all_t = RangedInteger<uint8_t, 0, PLAYER_ALL>;

	using playerid_either_t = RangedInteger<uint8_t, 0, PLAYER_EITHER>;

	using counter_t = RangedInteger<uint16_t, 1, std::numeric_limits<uint16_t>::max()>;

	struct Function {
		int value;
		Function() = default;
		Function(int v) : value(v) {}
		operator int() {
			return value;
		}
		operator int() const {
			return value;
		}
	};

	struct Table {
		int value;
		Table() = default;
		Table(int v) : value(v) {}
		operator int() {
			return value;
		}
		operator int() const {
			return value;
		}
	};

	using Nil = struct {}*;

	using Invalid = lua_obj_helper<LuaParam::DELETED>;

	struct VariadicArgs {
		int start;
		int size;
	};

	struct Unknown {
		int idx;
	};

	using Any = std::variant<card*, group*, effect*, Invalid*, Function, Table, bool, lua_Integer, Nil, Unknown>;

	template<typename T>
	inline constexpr bool IsBool = std::is_same_v<T, bool>;

	template<typename T>
	inline constexpr bool IsInteger = !IsBool<T> && (std::is_integral_v<T> || std::is_enum_v<T> || is_ranged_integer_v<T>);

	template<typename T>
	inline constexpr bool IsCard = std::is_same_v<T, card*>;

	template<typename T>
	inline constexpr bool IsGroup = std::is_same_v<T, group*>;

	template<typename T>
	inline constexpr bool IsEffect = std::is_same_v<T, effect*>;

	template<typename T>
	inline constexpr bool IsLuaObj = std::is_same_v<T, lua_obj*>;

	template<typename T>
	inline constexpr bool IsFunction = std::is_same_v<T, Function>;

	template<typename T>
	inline constexpr bool IsTable = std::is_same_v<T, Table>;

	template<typename T>
	inline constexpr bool IsNil = std::is_same_v<T, Nil>;

	template<typename T>
	inline constexpr bool IsInvalid = std::is_same_v<T, Invalid*>;

	template<typename T>
	inline constexpr bool IsUnknown = std::is_same_v<T, Unknown>;

	template<typename T>
	inline constexpr bool IsVariadic = std::is_same_v<T, VariadicArgs>;

	template<typename T>
	inline constexpr bool IsOwnedObject = false;

	template<typename T>
	inline constexpr bool IsOwnedObject<owned_lua<T>> = true;

	LuaParam get_lua_type(lua_State* L, int32_t index);

	template<typename T>
	inline constexpr auto get_lua_param_type() {
		if constexpr(IsOwnedObject<T>)
			return get_lua_param_type<typename T::lua_type>();
		else if constexpr(IsCard<T>)
			return LuaParam::CARD;
		else if constexpr(IsGroup<T>)
			return LuaParam::GROUP;
		else if constexpr(IsEffect<T>)
			return LuaParam::EFFECT;
		else if constexpr(IsFunction<T>)
			return LuaParam::FUNCTION;
		else if constexpr(IsBool<T>)
			return LuaParam::BOOLEAN;
		else if constexpr(IsInteger<T>)
			return LuaParam::INT;
		else if constexpr(IsTable<T>)
			return LuaParam::TABLE;
		else if constexpr(is_string_view_v<T>)
			return LuaParam::STRING;
	}

	inline constexpr const char* get_lua_type_name(LuaParam type) {
		switch(type) {
			case LuaParam::FUNCTION:
				return "Function";
			case LuaParam::STRING:
				return "String";
			case LuaParam::INT:
				return "Int";
			case LuaParam::BOOLEAN:
				return "boolean";
			case LuaParam::TABLE:
				return "table";
			case LuaParam::NIL:
			case LuaParam::NONE:
				return "nil";
			case LuaParam::CARD:
				return "Card";
			case LuaParam::GROUP:
				return "Group";
			case LuaParam::EFFECT:
				return "Effect";
			case LuaParam::DELETED:
				return "Deleted";
			default:
				return "unknown";
		}
	}

	//always return a string, whereas lua might return nullptr
	inline const char* lua_get_string_or_empty(lua_State* L, int idx) {
		size_t retlen = 0;
		auto str = lua_tolstring(L, idx, &retlen);
		return (!str || retlen == 0) ? "" : str;
	}

	inline auto lua_pushbool(lua_State* L, bool value) {
		return lua_pushboolean(L, value);
	}

	template<typename T>
	inline void lua_table_iterate(lua_State* L, int idx, T&& func) {
		lua_pushnil(L);
		while(lua_next(L, idx) != 0) {
			func();
			lua_pop(L, 1);
		}
	}

#if (!defined(_MSVC_LANG) && __cplusplus < 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG < 201703L)
	template<typename T, typename... Arg>
	using FunctionResult = std::result_of_t<T(Arg...)>;
#elif defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 7000
	template <class T, class... Args>
	using FunctionResult = typename std::__invoke_of<T, Args...>::type;
#else
	template<typename T, typename... Arg>
	using FunctionResult = std::invoke_result_t<T, Arg...>;
#endif

	template<typename T, typename T2>
	using EnableOnReturn = std::enable_if_t<std::is_same<FunctionResult<T>, T2>::value, int>;

	template<typename T, EnableOnReturn<T, void> = 0>
	inline void lua_iterate_table_or_stack(lua_State* L, int idx, int max, T&& func) {
		if(lua_istable(L, idx))
			return lua_table_iterate(L, idx, func);
		for(; idx <= max; ++idx) {
			lua_pushvalue(L, idx);
			func();
			lua_pop(L, 1);
		}
	}

	template<LuaParam param_type>
	static inline void check_param(lua_State* L, int32_t index) {
		auto type = get_lua_type(L, index);
		bool valid;
		if constexpr(param_type == LuaParam::BOOLEAN) {
			//we're really fine with anything as long as it's something
			valid = type != LuaParam::NIL;
		} else if constexpr(param_type == LuaParam::NIL) {
			valid = (type == param_type) || (type == LuaParam::NONE);
		} else {
			valid = type == param_type;
		}
		if(!valid) {
			lua_error_unsafe(L, R"(Parameter %d should be "%s" but is "%s".)", index, get_lua_type_name(param_type), get_lua_type_name(type));
		}
	}

	template<typename ...Args>
	inline std::pair<card*, group*> expand_to_card_or_group(const std::variant<Args...>& v) {
		if(auto* ppcard = std::get_if<card*>(&v); ppcard) {
			return { *ppcard, nullptr };
		} else if(auto* ppgroup = std::get_if<group*>(&v); ppgroup) {
			return { nullptr, *ppgroup };
		}
		return { nullptr, nullptr };
	}

	template<typename RangedInt>
	static inline RangedInt check_ranged_int(lua_State* L, int idx, lua_Integer value) {
		using base_int_type = typename RangedInt::value_type;
		static constexpr auto min = RangedInt::min_value;
		static constexpr auto max = RangedInt::max_value;
		if constexpr(std::is_unsigned_v<base_int_type>) {
			auto unsigned_val = static_cast<lua_Unsigned>(value);
			if(unsigned_val > max || unsigned_val < min) {
				lua_error_unsafe(L, R"(Integer parameter %d should be in the range [%I-%I] but its value is "%I".)", idx,
						  static_cast<lua_Unsigned>(min), static_cast<lua_Unsigned>(max), unsigned_val);
			}
		} else {
			if(value > max || value < min) {
				lua_error_unsafe(L, R"(Integer parameter %d should be in the range [%I-%I] but its value is "%I".)", idx,
						  static_cast<lua_Integer>(min), static_cast<lua_Integer>(max), value);
			}
		}
		return { static_cast<base_int_type>(value) };
	}

	template<typename T, bool last = false, std::enable_if_t<!is_variant_v<T> && !is_lua_range_v<T>, int> = 0>
	inline constexpr T get_lua(lua_State* L, int idx) {
		static_assert(std::is_trivially_destructible_v<T>);
		static_assert(!IsVariadic<T> || last);
		// we need to not have type::value_type be evaluated if the type isn't an optional
		auto _ = [](auto a) {
			using type = decltype(a);
			if constexpr(is_optional_v<type>) {
				static_assert(!IsVariadic<typename type::value_type>);
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
		if constexpr(IsVariadic<T>) {
			auto absidx = lua_absindex(L, idx);
			return return_value(VariadicArgs{ absidx, std::max<int>(0, (lua_gettop(L) - absidx) + 1) });
		} else if constexpr(IsLuaObj<actual_type>) {
			if(auto obj = lua_touserdata(L, idx)) {
				auto* ret = *static_cast<lua_obj**>(obj);
				if(ret->lua_type == LuaParam::DELETED)
					lua_error_unsafe(L, "Attempting to access deleted object.");
				return return_value(ret);
			}
			lua_error_unsafe(L, R"(Parameter %d should be one of "Card", "Group", "Effect" but is "%s".)", idx, get_lua_type_name(get_lua_type(L, idx)));
		} else {
			constexpr auto lua_type = get_lua_param_type<actual_type>();
			check_param<lua_type>(L, idx);
			if constexpr(lua_type == LuaParam::CARD || lua_type == LuaParam::GROUP || lua_type == LuaParam::EFFECT) {
				return return_value(*static_cast<actual_type*>(lua_touserdata(L, idx)));
			} else {
				if constexpr(lua_type == LuaParam::BOOLEAN) {
					return return_value(static_cast<bool>(lua_toboolean(L, idx)));
				} else if constexpr(lua_type == LuaParam::INT) {
					auto val = lua_isinteger(L, idx) ? lua_tointeger(L, idx) : static_cast<lua_Integer>(std::round(lua_tonumber(L, idx)));
					if constexpr(is_ranged_integer_v<actual_type>) {
						return return_value(check_ranged_int<actual_type>(L, idx, val));
					} else {
						return return_value(static_cast<actual_type>(val));
					}
				} else if constexpr(lua_type == LuaParam::FUNCTION) {
					return return_value(Function{ idx });
				} else if constexpr(lua_type == LuaParam::TABLE) {
					return return_value(Table{ idx });
				} else if constexpr(lua_type == LuaParam::STRING) {
					size_t len{};
					const auto* str = lua_tolstring(L, idx, &len);
					return return_value(std::string_view{ str, len });
				}
			}
		}
	}

	template<typename variant_t>
	struct check_variant_types_functor;

	template<typename... Args>
	struct check_variant_types_functor<std::variant<Args...>> {
		constexpr bool operator()(LuaParam lua_type) {
			if constexpr(((IsCard<Args> || IsLuaObj<Args>) || ...)) {
				if(lua_type == LuaParam::CARD)
					return true;
			}
			if constexpr(((IsGroup<Args> || IsLuaObj<Args>) || ...)) {
				if(lua_type == LuaParam::GROUP)
					return true;
			}
			if constexpr(((IsEffect<Args> || IsLuaObj<Args>) || ...)) {
				if(lua_type == LuaParam::EFFECT)
					return true;
			}
			if constexpr((IsInvalid<Args> || ...)) {
				if(lua_type == LuaParam::DELETED)
					return true;
			}
			if constexpr((IsFunction<Args> || ...)) {
				if(lua_type == LuaParam::FUNCTION)
					return true;
			}
			if constexpr((IsTable<Args> || ...)) {
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
			if constexpr((is_string_view_v<Args> || ...)) {
				if(lua_type == LuaParam::STRING)
					return true;
			}
			if constexpr((IsNil<Args> || ...)) {
				if(lua_type == LuaParam::NIL || lua_type == LuaParam::NONE)
					return true;
			}
			if constexpr((IsUnknown<Args> || ...)) {
				if(lua_type == LuaParam::UNKNOWN)
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
		static inline constexpr bool is_handled_variant_type = IsCard<T> || IsGroup<T> ||
			IsEffect<T> || IsInvalid<T> || IsLuaObj<T> || IsFunction<T> ||
			IsTable<T> || IsBool<T> || IsInteger<T> || IsNil<T> || IsUnknown<T>;
		constexpr variant_t operator()(lua_State* L, int idx, LuaParam lua_type) {
			static_assert(((is_handled_variant_type<Args> * 1) + ...) == std::variant_size_v<variant_t>, "Unhandled variant type passed");
			static_assert(std::is_trivially_destructible_v<variant_t>);
			if constexpr((IsCard<Args> || ...)) {
				if(lua_type == LuaParam::CARD)
					return *static_cast<card**>(lua_touserdata(L, idx));
			}
			if constexpr((IsGroup<Args> || ...)) {
				if(lua_type == LuaParam::GROUP)
					return *static_cast<group**>(lua_touserdata(L, idx));
			}
			if constexpr((IsEffect<Args> || ...)) {
				if(lua_type == LuaParam::EFFECT)
					return *static_cast<effect**>(lua_touserdata(L, idx));
			}
			if constexpr((IsInvalid<Args> || ...)) {
				if(lua_type == LuaParam::DELETED)
					return *static_cast<Invalid**>(lua_touserdata(L, idx));
			}
			if constexpr((IsLuaObj<Args> || ...)) {
				if(lua_type == LuaParam::CARD || lua_type == LuaParam::EFFECT || lua_type == LuaParam::GROUP)
					return *static_cast<lua_obj**>(lua_touserdata(L, idx));
			}
			if constexpr((IsFunction<Args> || ...)) {
				if(lua_type == LuaParam::FUNCTION)
					return Function{ idx };
			}
			if constexpr((IsTable<Args> || ...)) {
				if(lua_type == LuaParam::TABLE)
					return Table{ idx };
			}
			if constexpr((IsBool<Args> || ...)) {
				if(lua_type == LuaParam::BOOLEAN)
					return static_cast<bool>(lua_toboolean(L, idx));
			}
			if constexpr((IsInteger<Args> || ...)) {
				static_assert((IsInteger<Args> + ...) <= 1, "Variant must have at most 1 integer type");
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
					if constexpr(is_ranged_integer_v<integer_type>) {
						return check_ranged_int<integer_type>(L, idx, val);
					} else {
						return static_cast<integer_type>(val);
					}
				}
			}
			if constexpr((is_string_view_v<Args> || ...)) {
				if(lua_type == LuaParam::STRING) {
					size_t len{};
					const auto* str = lua_tolstring(L, idx, &len);
					return std::string_view{ str, len };
				}
			}
			if constexpr((IsNil<Args> || ...)) {
				if(lua_type == LuaParam::NIL || lua_type == LuaParam::NONE)
					return Nil{};
			}
			if constexpr((IsUnknown<Args> || ...)) {
				if(lua_type == LuaParam::UNKNOWN)
					return Unknown{ idx };
			}
			unreachable();
		}
	};

	template<typename variant_t>
	struct get_variant_names_functor;

	template<typename... Args>
	struct get_variant_names_functor<std::variant<Args...>> {
		constexpr std::array<char, 128> operator()() {
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
				copy_string(get_lua_type_name(LuaParam::CARD));
			}
			if constexpr((IsGroup<Args> || ...) || (IsLuaObj<Args> || ...)) {
				copy_string(get_lua_type_name(LuaParam::GROUP));
			}
			if constexpr((IsEffect<Args> || ...) || (IsLuaObj<Args> || ...)) {
				copy_string(get_lua_type_name(LuaParam::EFFECT));
			}
			if constexpr((IsFunction<Args> || ...)) {
				copy_string(get_lua_type_name(LuaParam::FUNCTION));
			}
			if constexpr((IsTable<Args> || ...)) {
				copy_string(get_lua_type_name(LuaParam::TABLE));
			}
			if constexpr((IsBool<Args> || ...)) {
				copy_string(get_lua_type_name(LuaParam::BOOLEAN));
			}
			if constexpr((IsInteger<Args> || ...)) {
				copy_string(get_lua_type_name(LuaParam::INT));
			}
			if constexpr((is_string_view_v<Args> || ...)) {
				copy_string(get_lua_type_name(LuaParam::STRING));
			}
			if constexpr((IsNil<Args> || ...)) {
				copy_string(get_lua_type_name(LuaParam::NIL));
			}
			return ret;
		}
	};

	template<typename T, bool last = false, std::enable_if_t<is_variant_v<T>, int> = 0>
	inline constexpr T get_lua(lua_State* L, int idx) {
		auto type = get_lua_type(L, idx);
		if(!check_variant_types_functor<T>()(type)) {
			constexpr auto types_string = get_variant_names_functor<T>()();
			static_assert(types_string.back() == '\0');
			lua_error_unsafe(L, R"(Parameter %d should be one of %s but is "%s".)", idx, types_string.data(), get_lua_type_name(type));
		}
		return get_variant_type_functor<T>()(L, idx, type);
	}

	template<typename T, bool last, std::enable_if_t<is_lua_range_v<T>, int> = 0>
	inline constexpr decltype(auto) get_lua(lua_State* L, int idx) {
		using vec_type = typename T::base;
		vec_type& result = *std::any_cast<vec_type>(set_any_temp_storage(L, vec_type{}));
		result.from_table = lua_istable(L, idx);
		if constexpr(!is_nonempty_lua_range_v<T> && !last) {
			// range as middle parameter which can also be nil
			if(lua_isnoneornil(L, idx))
				return std::ref(result);
		}
		lua_iterate_table_or_stack(L, idx, last ? lua_gettop(L) : idx, [&] {
#if defined(_MSC_VER) && (_MSC_VER + 0) < 1920
			// Visual Studio 2017 crashes when using push_back on a vector with a RangedInteger as its element, work around that
			// fatal error C1001: An internal error has occurred in the compiler.1>(compiler file ‘msc1.cpp’, line 1518)
			auto val = get_lua<typename T::value_type>(L, -1);
			result.resize(result.size() + 1);
			result.back() = std::move(val);
#else
			result.push_back(get_lua<typename T::value_type>(L, -1));
#endif
		});
		if constexpr(is_nonempty_lua_range_v<T>) {
			if(result.empty())
				lua_error_unsafe(L, "Parameter %d: no values were provided.", idx);
		}
		return std::ref(result);
	}

	template<typename T>
	static LuaRet get_lua_ref(lua_State* L) {
		lua_pushinteger(L, get_lua<T*>(L, 1)->ref_handle);
		return 1;
	}
	template<typename T>
	static LuaRet from_lua_ref(lua_State* L) {
		static_assert(IsCard<T*> || IsGroup<T*> || IsEffect<T*>);
		auto ref = get_lua<int32_t>(L, 1);
		lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		check_param<get_lua_param_type<T*>()>(L, -1);
		return 1;
	}
}

#define yieldk(...) +[](lua_State* L)->int32_t { \
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t status, lua_KContext ctx) -> int { \
		(void)status; \
		(void)ctx; \
		auto pduel = duel::from(L); \
		(void)pduel; \
		do __VA_ARGS__ while(0); \
		unreachable(); \
	}); \
}

#define yield() +[](lua_State* L)->int32_t { return lua_yield(L, 0); }

// double macro to make MSVC happy
#define ensure_luaL_stack_int(L,...) [&](){ luaL_checkstack(L, 5, nullptr); return __VA_ARGS__; }()
#define ensure_luaL_stack(func,L,...) ensure_luaL_stack_int(L,func(L, __VA_ARGS__))

#endif /* SCRIPTLIB_H_ */
