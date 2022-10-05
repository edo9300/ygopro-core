/*
 * group.h
 *
 *  Created on: 2010-5-6
 *      Author: Argon
 */

#ifndef GROUP_H_
#define GROUP_H_

#include "containers_fwd.h"
#include "common.h"
#include "lua_obj.h"
#include <set>
#include <vector>

class card;
class duel;

class group : public lua_obj_helper<PARAM_TYPE_GROUP> {
public:
	card_set container;
	card_set::iterator it;
	uint32_t is_readonly{ 0 };
	bool is_iterator_dirty{ true };
	
	inline bool has_card(card* c) const {
		return container.find(c) != container.end();
	}
	
	explicit group(duel* pd) : lua_obj_helper(pd) {}
	group(duel* pd, card* pcard) : lua_obj_helper(pd), container({ pcard }) {}
	group(duel* pd, const card_set& cset) : lua_obj_helper(pd), container(cset) {}
	group(duel* pd, card_set&& cset) : lua_obj_helper(pd), container(std::move(cset)) {}
	group(duel* pd, group* pgroup) : lua_obj_helper(pd), container(pgroup->container) {}
	template<typename Iter>
	group(duel* pd, const Iter begin, const Iter end) : lua_obj_helper(pd), container(begin, end) {}
	group(duel* pd, const std::vector<card*>& vcard) : group(pd, vcard.begin(), vcard.end()) {}
	group(duel* pd, lua_obj* pobj);
	~group() = default;
};

#endif /* GROUP_H_ */
