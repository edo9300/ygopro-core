/*
 * group.cpp
 *
 *  Created on: 2010-8-3
 *      Author: Argon
 */

#include "group.h"
#include "card.h"

group::group(duel* pd, lua_obj* pobj) :
	lua_obj_helper(pd)
{
	if(pobj->lua_type == LuaParam::CARD)
		container.insert(static_cast<card*>(pobj));
	else if(pobj->lua_type == LuaParam::GROUP)
		container = static_cast<group*>(pobj)->container;
}
