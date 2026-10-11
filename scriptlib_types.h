/*
 * Copyright (c) 2026, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef SCRIPTLIB_TYPES_H_
#define SCRIPTLIB_TYPES_H_

#include <array>
#include <cmath> //std::round
#include <cstdint>
#include <limits> //std::numeric_limits
#include <type_traits> //std::is_same_v
#include <variant>
#include "common.h"
#include "lua_obj.h"
#include "type_traits_utilities.h"

class card;
class effect;
class group;
class duel;

struct lua_State;

namespace scriptlib {
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

	using Any = std::variant<card*, group*, effect*, Invalid*, Function, Table, bool, int64_t, Nil, Unknown>;

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
}

#endif /* SCRIPTLIB_TYPES_H_ */
