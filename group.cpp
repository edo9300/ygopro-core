/*
 * group.cpp
 *
 *  Created on: 2010-8-3
 *      Author: Argon
 */

#include "interpreter.h"
#include "group.h"
#include "card.h"
#include "duel.h"

group::group(duel* pd) {
	lua_type = PARAM_TYPE_GROUP;
	pduel = pd;
}
group::group(duel* pd, card* pcard) {
	container.insert(pcard);
	lua_type = PARAM_TYPE_GROUP;
	pduel = pd;
}
group::group(duel* pd, const card_set& cset): container(cset) {
	lua_type = PARAM_TYPE_GROUP;
	pduel = pd;
}
