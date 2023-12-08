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
	LuaParam lua_type;
	int32_t ref_handle;
	duel* pduel{ nullptr };
protected:
	lua_obj(LuaParam _lua_type, duel* _pduel) :lua_type(_lua_type), pduel(_pduel) {};
};

template<LuaParam _Type>
class lua_obj_helper : public lua_obj {
	static_assert(_Type == LuaParam::CARD || _Type == LuaParam::GROUP ||
				  _Type == LuaParam::EFFECT || _Type == LuaParam::DELETED,
				  "Invalid parameter type");
public:
	lua_obj_helper(duel* pduel) : lua_obj(_Type, pduel) {};
};

#endif /* LUA_OBJ_H_ */
