/*
 * Copyright (c) 2020-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef LUA_OBJ_H_
#define LUA_OBJ_H_

#include <cstdint>

enum class LuaParam : uint8_t {
	INT = 1,
	STRING,
	CARD,
	GROUP,
	EFFECT,
	FUNCTION,
	BOOLEAN,
	INDEX,
	DELETED,
};

class duel;

class lua_obj {
public:
	duel* pduel{ nullptr };
	int32_t ref_handle{};
	LuaParam lua_type;
protected:
	lua_obj(LuaParam _lua_type, duel* _pduel) :pduel(_pduel), lua_type(_lua_type) {}
};

template<LuaParam _Type>
class lua_obj_helper : public lua_obj {
	static_assert(_Type == LuaParam::CARD || _Type == LuaParam::GROUP ||
				  _Type == LuaParam::EFFECT || _Type == LuaParam::DELETED,
				  "Invalid parameter type");
public:
	lua_obj_helper(duel* pduel_) : lua_obj(_Type, pduel_) {}
};

#endif /* LUA_OBJ_H_ */
