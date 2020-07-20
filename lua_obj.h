#ifndef LUA_OBJ_H_
#define LUA_OBJ_H_

#include <cstdint>

class duel;

class lua_obj {
public:
	uint8_t lua_type{ 0 };
	int32_t ref_handle{ 0 };
	duel* pduel{ nullptr };
};

#endif /* LUA_OBJ_H_ */
