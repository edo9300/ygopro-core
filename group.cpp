/*
 * Copyright (c) 2010-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2020-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include "card.h"
#include "group.h"

group::group(duel* pd, lua_obj* pobj) :
	lua_obj_helper(pd)
{
	if(pobj->lua_type == LuaParam::CARD)
		container.insert(static_cast<card*>(pobj));
	else if(pobj->lua_type == LuaParam::GROUP)
		container = static_cast<group*>(pobj)->container;
}
