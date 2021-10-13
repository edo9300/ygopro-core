#ifndef LUA_OBJ_H_
#define LUA_OBJ_H_

#include <cstdint>

enum LuaParamType {
	PARAM_TYPE_INT = 0x01,
	PARAM_TYPE_STRING = 0x02,
	PARAM_TYPE_CARD = 0x04,
	PARAM_TYPE_GROUP = 0x08,
	PARAM_TYPE_EFFECT = 0x10,
	PARAM_TYPE_FUNCTION = 0x20,
	PARAM_TYPE_BOOLEAN = 0x40,
	PARAM_TYPE_INDEX = 0x80,
	PARAM_TYPE_DELETED = 0x100,
};

class duel;

class lua_obj {
public:
	LuaParamType lua_type;
	int32_t ref_handle;
	duel* pduel{ nullptr };
protected:
	lua_obj(LuaParamType _lua_type, duel* _pduel) :lua_type(_lua_type), pduel(_pduel) {};
};

template<LuaParamType _Type>
class lua_obj_helper : public lua_obj {
	static_assert(_Type == PARAM_TYPE_CARD || _Type == PARAM_TYPE_GROUP ||
				  _Type == PARAM_TYPE_EFFECT || _Type == PARAM_TYPE_DELETED,
				  "Invalid parameter type");
public:
	lua_obj_helper(duel* pduel) : lua_obj(_Type, pduel) {};
};

#endif /* LUA_OBJ_H_ */
