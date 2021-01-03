#ifndef LUA_OBJ_H_
#define LUA_OBJ_H_

#include <cstdint>

#define	PARAM_TYPE_INT		0x01
#define	PARAM_TYPE_STRING	0x02
#define	PARAM_TYPE_CARD		0x04
#define	PARAM_TYPE_GROUP	0x08
#define PARAM_TYPE_EFFECT	0x10
#define	PARAM_TYPE_FUNCTION	0x20
#define PARAM_TYPE_BOOLEAN	0x40
#define PARAM_TYPE_INDEX	0x80
#define PARAM_TYPE_DELETED	0x100

class duel;

class lua_obj {
public:
	uint16_t lua_type{ 0 };
	int32_t ref_handle{ 0 };
	duel* pduel{ nullptr };
protected:
	lua_obj(uint16_t _lua_type, duel* _pduel) :lua_type(_lua_type), pduel(_pduel) {};
};

template<uint16_t _Type>
class lua_obj_helper : public lua_obj {
public:
	lua_obj_helper(duel* pduel) : lua_obj(_Type, pduel) {};
};

#endif /* LUA_OBJ_H_ */
