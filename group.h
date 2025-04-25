/*
 * Copyright (c) 2010-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2019-2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef GROUP_H_
#define GROUP_H_

#include <set>
#include <vector>
#include "common.h"
#include "containers_fwd.h"
#include "lua_obj.h"

class card;
class duel;

class group : public lua_obj_helper<LuaParam::GROUP> {
public:
	card_set container;
	card_set::iterator it;
	uint8_t is_readonly{ 0 };
	bool is_iterator_dirty{ true };
	
	bool has_card(card* c) const {
		return container.find(c) != container.end();
	}
	
	explicit group(duel* pd) : lua_obj_helper(pd) {}
	group(duel* pd, card* pcard) : lua_obj_helper(pd), container({ pcard }) {}
	group(duel* pd, card_set cset) : lua_obj_helper(pd), container(std::move(cset)) {}
	group(duel* pd, group* pgroup) : group(pd, pgroup->container) {}
	template<typename Iter>
	group(duel* pd, const Iter begin, const Iter end) : lua_obj_helper(pd), container(begin, end) {}
	group(duel* pd, const std::vector<card*>& vcard) : group(pd, vcard.begin(), vcard.end()) {}
	group(duel* pd, lua_obj* pobj) : lua_obj_helper(pd) {
		if(pobj->lua_type == LuaParam::CARD)
			container.insert(reinterpret_cast<card*>(pobj));
		else if(pobj->lua_type == LuaParam::GROUP)
			container = reinterpret_cast<group*>(pobj)->container;
	}
	~group() = default;
};

#endif /* GROUP_H_ */
