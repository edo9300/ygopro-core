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
	if(pobj->lua_type == PARAM_TYPE_CARD)
		container = { static_cast<card*>(pobj) };
	else if(pobj->lua_type == PARAM_TYPE_GROUP)
		container = static_cast<group*>(pobj)->container;
}
