/*
 * group.cpp
 *
 *  Created on: 2010-8-3
 *      Author: Argon
 */

#include "interpreter.h"
#include "group.h"

group::group(duel* pd) :
	lua_obj_helper(pd)
{
}

group::group(duel* pd, card* pcard) :
	lua_obj_helper(pd), container({ pcard })
{
}

group::group(duel* pd, const card_set& cset) :
	lua_obj_helper(pd), container(cset)
{
}

group::group(duel* pd, group* pgroup) :
	lua_obj_helper(pd), container(pgroup->container)
{
}

group::group(duel* pd, const std::vector<card*>& vcard) :
	lua_obj_helper(pd), container(vcard.begin(), vcard.end())
{
}
