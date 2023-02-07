/*
 * libgroup.cpp
 *
 *  Created on: 2010-5-6
 *      Author: Argon
 */

#include <iterator>
#include <algorithm>
#include "scriptlib.h"
#include "group.h"
#include "card.h"
#include "effect.h"
#include "duel.h"

#define LUA_MODULE Group
#include "function_array_helper.h"

namespace {

using namespace scriptlib;

void assert_readonly_group(lua_State* L, group* pgroup) {
	if(pgroup->is_readonly != 1)
		return;
	lua_error(L, "attempt to modify a read only group");
}

LUA_FUNCTION(CreateGroup) {
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	lua_iterate_table_or_stack(L, 1, lua_gettop(L), [L, &container = pgroup->container] {
		if(!lua_isnil(L, -1)) {
			auto pcard = lua_get<card*, true>(L, -1);
			container.insert(pcard);
		}
	});
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION_ALIAS(FromCards);
LUA_FUNCTION(Clone) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pgroup = lua_get<group*, true>(L, 1);
	group* newgroup = pduel->new_group(pgroup);
	interpreter::pushobject(L, newgroup);
	return 1;
}
LUA_FUNCTION(DeleteGroup) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pgroup = lua_get<group*, true>(L, 1);
	if(pgroup->is_readonly != 2)
		return 0;
	pgroup->is_readonly = 0;
	pduel->sgroups.insert(pgroup);
	return 0;
}
LUA_FUNCTION(KeepAlive) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pgroup = lua_get<group*, true>(L, 1);
	if(pgroup->is_readonly != 1) {
		pgroup->is_readonly = 2;
		pduel->sgroups.erase(pgroup);
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(Clear) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	assert_readonly_group(L, pgroup);
	pgroup->is_iterator_dirty = true;
	pgroup->container.clear();
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(AddCard) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	assert_readonly_group(L, pgroup);
	pgroup->is_iterator_dirty = true;
	card* pcard = nullptr;
	group* sgroup = nullptr;
	get_card_or_group(L, 2, pcard, sgroup);
	if(pcard)
		pgroup->container.insert(pcard);
	else
		pgroup->container.insert(sgroup->container.begin(), sgroup->container.end());
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION_ALIAS(Merge);
LUA_FUNCTION(RemoveCard) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	assert_readonly_group(L, pgroup);
	pgroup->is_iterator_dirty = true;
	card* pcard = nullptr;
	group* sgroup = nullptr;
	get_card_or_group(L, 2, pcard, sgroup);
	if(pcard)
		pgroup->container.erase(pcard);
	else {
		if(pgroup == sgroup)
			lua_error(L, "Attempting to remove a group from itself");
		for(auto& _pcard : sgroup->container)
			pgroup->container.erase(_pcard);
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION_ALIAS(Sub);
LUA_FUNCTION(GetNext) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	if(pgroup->is_iterator_dirty)
		lua_error(L, "Called Group.GetNext without first calling Group.GetFirst");
	if(pgroup->it == pgroup->container.end())
		lua_pushnil(L);
	else {
		if((++pgroup->it) == pgroup->container.end())
			lua_pushnil(L);
		else
			interpreter::pushobject(L, *pgroup->it);
	}
	return 1;
}
LUA_FUNCTION(GetFirst) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	pgroup->is_iterator_dirty = false;
	if(pgroup->container.size())
		interpreter::pushobject(L, *(pgroup->it = pgroup->container.begin()));
	else
		lua_pushnil(L);
	return 1;
}
LUA_FUNCTION(TakeatPos) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto pos = lua_get<size_t>(L, 2);
	if(pos >= pgroup->container.size())
		lua_pushnil(L);
	else {
		auto cit = pgroup->container.begin();
		std::advance(cit, pos);
		interpreter::pushobject(L, *cit);
	}
	return 1;
}
LUA_FUNCTION(GetCount) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	lua_pushinteger(L, pgroup->container.size());
	return 1;
}
LUA_FUNCTION(Filter) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	card_set cset(pgroup->container);
	if(auto pexception = lua_get<card*>(L, 3))
		cset.erase(pexception);
	else if(auto pexgroup = lua_get<group*>(L, 3)) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	const auto pduel = lua_get<duel*>(L);
	group* new_group = pduel->new_group();
	uint32_t extraargs = lua_gettop(L) - 3;
	for(auto& pcard : cset) {
		if(pduel->lua->check_matching(pcard, findex, extraargs)) {
			new_group->container.insert(pcard);
		}
	}
	interpreter::pushobject(L, new_group);
	return 1;
}
LUA_FUNCTION(Match) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	assert_readonly_group(L, pgroup);
	pgroup->is_iterator_dirty = true;
	card* pexception = nullptr;
	group* pexgroup = nullptr;
	if((pexception = lua_get<card*>(L, 3)) == nullptr)
		pexgroup = lua_get<group*>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	uint32_t extraargs = lua_gettop(L) - 3;
	auto& cset = pgroup->container;
	if(pexception) {
		for(auto cit = cset.begin(), cend = cset.end(); cit != cend; ) {
			auto rm = cit++;
			auto* pcard = *rm;
			if(pcard == pexception || !pduel->lua->check_matching(pcard, findex, extraargs))
				cset.erase(rm);
		}
	} else if(pexgroup) {
		auto pexbegin = pexgroup->container.cbegin();
		auto pexend = pexgroup->container.cend();
		auto should_remove = [&pexbegin, &pexend](card* pcard) {
			if(pexbegin == pexend)
				return false;
			if(*pexbegin == pcard) {
				++pexbegin;
				return true;
			}
			return false;
		};
		for(auto cit = cset.begin(), cend = cset.end(); cit != cend; ) {
			auto rm = cit++;
			auto* pcard = *rm;
			if(should_remove(pcard) || !pduel->lua->check_matching(pcard, findex, extraargs))
				cset.erase(rm);
		}
	} else {
		for(auto cit = cset.begin(), cend = cset.end(); cit != cend; ) {
			auto rm = cit++;
			auto* pcard = *rm;
			if(!pduel->lua->check_matching(pcard, findex, extraargs))
				cset.erase(rm);
		}
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION(FilterCount) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	card_set cset(pgroup->container);
	card* pexception = nullptr;
	group* pexgroup = nullptr;
	if((pexception = lua_get<card*>(L, 3)) != nullptr)
		cset.erase(pexception);
	else if((pexgroup = lua_get<group*>(L, 3)) != nullptr) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	const auto pduel = lua_get<duel*>(L);
	uint32_t extraargs = lua_gettop(L) - 3;
	uint32_t count = 0;
	for (auto& pcard : cset) {
		if(pduel->lua->check_matching(pcard, findex, extraargs))
			++count;
	}
	lua_pushinteger(L, count);
	return 1;
}
LUA_FUNCTION(FilterSelect) {
	check_action_permission(L);
	check_param_count(L, 6);
	const auto findex = lua_get<function, true>(L, 3);
	auto pgroup = lua_get<group*, true>(L, 1);
	card_set cset(pgroup->container);
	bool cancelable = false;
	uint8_t lastarg = 6;
	if(lua_isboolean(L, lastarg)) {
		cancelable = lua_get<bool, false>(L, lastarg);
		++lastarg;
	}
	if(auto pexception = lua_get<card*>(L, lastarg))
		cset.erase(pexception);
	else if(auto pexgroup = lua_get<group*>(L, lastarg)) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto min = lua_get<uint16_t>(L, 4);
	auto max = lua_get<uint16_t>(L, 5);
	uint32_t extraargs = lua_gettop(L) - lastarg;
	pduel->game_field->core.select_cards.clear();
	for(auto& pcard : cset) {
		if(pduel->lua->check_matching(pcard, findex, extraargs))
			pduel->game_field->core.select_cards.push_back(pcard);
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16));
	return lua_yieldk(L, 0, (lua_KContext)cancelable, push_return_cards);
}
LUA_FUNCTION(Select) {
	check_action_permission(L);
	check_param_count(L, 5);
	auto pgroup = lua_get<group*, true>(L, 1);
	card_set cset(pgroup->container);
	bool cancelable = false;
	uint8_t lastarg = 5;
	if(lua_isboolean(L, lastarg)) {
		cancelable = lua_get<bool, false>(L, lastarg);
		++lastarg;
	}
	if(auto pexception = lua_get<card*>(L, lastarg))
		cset.erase(pexception);
	else if(auto pexgroup = lua_get<group*>(L, lastarg)) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto min = lua_get<uint16_t>(L, 3);
	auto max = lua_get<uint16_t>(L, 4);
	pduel->game_field->core.select_cards.assign(cset.begin(), cset.end());
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16));
	return lua_yieldk(L, 0, (lua_KContext)cancelable, push_return_cards);
}
LUA_FUNCTION(SelectUnselect) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto pgroup1 = lua_get<group*, true>(L, 1);
	auto pgroup2 = lua_get<group*>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 3);
	if(playerid != 0 && playerid != 1)
		return 0;
	if(pgroup2) {
		auto first1 = pgroup1->container.begin();
		auto last1 = pgroup1->container.end();
		auto first2 = pgroup2->container.begin();
		auto last2 = pgroup2->container.end();
		while(first1 != last1 && first2 != last2) {
			if((*first1)->cardid < (*first2)->cardid) {
				++first1;
			} else {
				if(!((*first2)->cardid < (*first1)->cardid)) {
					return 0;
				}
				++first2;
			}
		}
	}
	bool finishable = lua_get<bool, false>(L, 4);
	bool cancelable = lua_get<bool, false>(L, 5);
	uint16_t min = lua_get<uint16_t, 1>(L, 6);
	uint16_t max = lua_get<uint16_t, 1>(L, 7);
	if(min > max)
		min = max;
	pduel->game_field->core.select_cards.assign(pgroup1->container.begin(), pgroup1->container.end());
	if(pgroup2)
		pduel->game_field->core.unselect_cards.assign(pgroup2->container.begin(), pgroup2->container.end());
	else
		pduel->game_field->core.unselect_cards.clear();
	pduel->game_field->add_process(PROCESSOR_SELECT_UNSELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16), finishable);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
		if(pduel->game_field->return_cards.canceled)
			lua_pushnil(L);
		else
			interpreter::pushobject(L, pduel->game_field->return_cards.list[0]);
		return 1;
	});
}
LUA_FUNCTION(RandomSelect) {
	check_param_count(L, 3);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto playerid = lua_get<uint8_t>(L, 2);
	size_t count = lua_get<uint32_t>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	group* newgroup = pduel->new_group();
	if(count > pgroup->container.size())
		count = pgroup->container.size();
	if(count == 0) {
		interpreter::pushobject(L, newgroup);
		return 1;
	}
	if(count == pgroup->container.size())
		newgroup->container = pgroup->container;
	else {
		while(newgroup->container.size() < count) {
			int32_t i = pduel->get_next_integer(0, (int32_t)pgroup->container.size() - 1);
			auto cit = pgroup->container.begin();
			std::advance(cit, i);
			newgroup->container.insert(*cit);
		}
	}
	auto message = pduel->new_message(MSG_RANDOM_SELECTED);
	message->write<uint8_t>(playerid);
	message->write<uint32_t>(count);
	for(auto& pcard : newgroup->container) {
		message->write(pcard->get_info_location());
	}
	interpreter::pushobject(L, newgroup);
	return 1;
}
LUA_FUNCTION(IsExists) {
	check_param_count(L, 4);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	card_set cset(pgroup->container);
	if(auto pexception = lua_get<card*>(L, 4))
		cset.erase(pexception);
	else if(auto pexgroup = lua_get<group*>(L, 4)) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	const auto pduel = lua_get<duel*>(L);
	auto count = lua_get<uint16_t>(L, 3);
	uint32_t extraargs = lua_gettop(L) - 4;
	uint32_t fcount = 0;
	for(auto& pcard : cset) {
		if(pduel->lua->check_matching(pcard, findex, extraargs)) {
			++fcount;
			if(fcount >= count)
				break;
		}
	}
	lua_pushboolean(L, fcount >= count);
	return 1;
}
LUA_FUNCTION(CheckWithSumEqual) {
	check_param_count(L, 5);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto acc = lua_get<uint32_t>(L, 3);
	auto min = lua_get<int32_t>(L, 4);
	auto max = lua_get<int32_t>(L, 5);
	if(min < 0)
		min = 0;
	if(max < min)
		max = min;
	int32_t extraargs = lua_gettop(L) - 5;
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = static_cast<int32_t>(cv.size());
	const auto beginit = pduel->game_field->core.must_select_cards.begin();
	const auto endit = pduel->game_field->core.must_select_cards.end();
	for(auto& pcard : pgroup->container) {
		if(std::find(beginit, endit, pcard) == endit)
			cv.push_back(pcard);
	}
	pduel->game_field->core.must_select_cards.clear();
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, findex, extraargs);
	int32_t should_continue = TRUE;
	lua_pushboolean(L, field::check_with_sum_limit_m(cv, acc, 0, min, max, mcount, &should_continue));
	lua_pushboolean(L, should_continue);
	return 2;
}
LUA_FUNCTION(SelectWithSumEqual) {
	check_action_permission(L);
	check_param_count(L, 6);
	const auto findex = lua_get<function, true>(L, 3);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto acc = lua_get<uint32_t>(L, 4);
	auto min = lua_get<int32_t>(L, 5);
	auto max = lua_get<int32_t>(L, 6);
	if(min < 0)
		min = 0;
	if(max < min)
		max = min;
	int32_t extraargs = lua_gettop(L) - 6;
	pduel->game_field->core.select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	for(auto& pcard : pduel->game_field->core.must_select_cards) {
		auto it = std::remove(pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end(), pcard);
		pduel->game_field->core.select_cards.erase(it, pduel->game_field->core.select_cards.end());
	}
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = static_cast<int32_t>(cv.size());
	cv.insert(cv.end(), pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end());
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, findex, extraargs);
	if(!field::check_with_sum_limit_m(cv, acc, 0, min, max, mcount, nullptr)) {
		pduel->game_field->core.must_select_cards.clear();
		group* empty_group = pduel->new_group();
		interpreter::pushobject(L, empty_group);
		return 1;
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_SUM, 0, 0, 0, acc, playerid + (min << 16) + (max << 24));
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
		group* pgroup = pduel->new_group(pduel->game_field->return_cards.list);
		pduel->game_field->core.must_select_cards.clear();
		interpreter::pushobject(L, pgroup);
		return 1;
	});
}
LUA_FUNCTION(CheckWithSumGreater) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto acc = lua_get<uint32_t>(L, 3);
	int32_t extraargs = lua_gettop(L) - 3;
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = static_cast<int32_t>(cv.size());
	const auto beginit = pduel->game_field->core.must_select_cards.begin();
	const auto endit = pduel->game_field->core.must_select_cards.end();
	for(auto& pcard : pgroup->container) {
		if(std::find(beginit, endit, pcard) == endit)
			cv.push_back(pcard);
	}
	pduel->game_field->core.must_select_cards.clear();
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, findex, extraargs);
	int32_t should_continue = TRUE;
	lua_pushboolean(L, field::check_with_sum_greater_limit_m(cv, acc, 0, 0xffff, mcount, &should_continue));
	lua_pushboolean(L, should_continue);
	return 2;
}
LUA_FUNCTION(SelectWithSumGreater) {
	check_action_permission(L);
	check_param_count(L, 4);
	const auto findex = lua_get<function, true>(L, 3);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto acc = lua_get<uint32_t>(L, 4);
	int32_t extraargs = lua_gettop(L) - 4;
	pduel->game_field->core.select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	for(auto& pcard : pduel->game_field->core.must_select_cards) {
		auto it = std::remove(pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end(), pcard);
		pduel->game_field->core.select_cards.erase(it, pduel->game_field->core.select_cards.end());
	}
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = static_cast<int32_t>(cv.size());
	cv.insert(cv.end(), pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end());
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, findex, extraargs);
	if(!field::check_with_sum_greater_limit_m(cv, acc, 0, 0xffff, mcount, nullptr)) {
		pduel->game_field->core.must_select_cards.clear();
		group* empty_group = pduel->new_group();
		interpreter::pushobject(L, empty_group);
		return 1;
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_SUM, 0, 0, 0, acc, playerid);
	return lua_yieldk(L, 0, 0, [](lua_State* L, int32_t/* status*/, lua_KContext/* ctx*/) {
		const auto pduel = lua_get<duel*>(L);
		group* pgroup = pduel->new_group(pduel->game_field->return_cards.list);
		pduel->game_field->core.must_select_cards.clear();
		interpreter::pushobject(L, pgroup);
		return 1;
	});
}
LUA_FUNCTION(GetMinGroup) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	if(pgroup->container.size() == 0)
		return 0;
	group* newgroup = pduel->new_group();
	int64_t min, op;
	int32_t extraargs = lua_gettop(L) - 2;
	auto cit = pgroup->container.begin();
	min = pduel->lua->get_operation_value(*cit, findex, extraargs);
	newgroup->container.insert(*cit);
	++cit;
	for(; cit != pgroup->container.end(); ++cit) {
		op = pduel->lua->get_operation_value(*cit, findex, extraargs);
		if(op == min)
			newgroup->container.insert(*cit);
		else if(op < min) {
			newgroup->container.clear();
			newgroup->container.insert(*cit);
			min = op;
		}
	}
	interpreter::pushobject(L, newgroup);
	lua_pushinteger(L, min);
	return 2;
}
LUA_FUNCTION(GetMaxGroup) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	if(pgroup->container.size() == 0)
		return 0;
	group* newgroup = pduel->new_group();
	int64_t max, op;
	int32_t extraargs = lua_gettop(L) - 2;
	auto cit = pgroup->container.begin();
	max = pduel->lua->get_operation_value(*cit, findex, extraargs);
	newgroup->container.insert(*cit);
	++cit;
	for(; cit != pgroup->container.end(); ++cit) {
		op = pduel->lua->get_operation_value(*cit, findex, extraargs);
		if(op == max)
			newgroup->container.insert(*cit);
		else if(op > max) {
			newgroup->container.clear();
			newgroup->container.insert(*cit);
			max = op;
		}
	}
	interpreter::pushobject(L, newgroup);
	lua_pushinteger(L, max);
	return 2;
}
LUA_FUNCTION(GetSum) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t extraargs = lua_gettop(L) - 2;
	int64_t sum = 0;
	for(auto& pcard : pgroup->container) {
		sum += pduel->lua->get_operation_value(pcard, findex, extraargs);
	}
	lua_pushinteger(L, sum);
	return 1;
}
LUA_FUNCTION(GetBitwiseAnd) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t extraargs = lua_gettop(L) - 2;
	uint64_t total = 0;
	for(auto& pcard : pgroup->container) {
		total &= static_cast<uint64_t>(pduel->lua->get_operation_value(pcard, findex, extraargs));
	}
	lua_pushinteger(L, total);
	return 1;
}
LUA_FUNCTION(GetBitwiseOr) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t extraargs = lua_gettop(L) - 2;
	uint64_t total = 0;
	for(auto& pcard : pgroup->container) {
		total |= static_cast<uint64_t>(pduel->lua->get_operation_value(pcard, findex, extraargs));
	}
	lua_pushinteger(L, total);
	return 1;
}
LUA_FUNCTION(GetClass) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t extraargs = lua_gettop(L) - 2;
	std::set<int64_t> er;
	for(auto& pcard : pgroup->container) {
		er.insert(pduel->lua->get_operation_value(pcard, findex, extraargs));
	}
	lua_newtable(L);
	int i = 1;
	for(auto& val : er) {
		lua_pushinteger(L, i++);
		lua_pushinteger(L, val);
		lua_settable(L, -3);
	}
	return 1;
}
LUA_FUNCTION(GetClassCount) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t extraargs = lua_gettop(L) - 2;
	std::set<int64_t> er;
	for(auto& pcard : pgroup->container) {
		er.insert(pduel->lua->get_operation_value(pcard, findex, extraargs));
	}
	lua_pushinteger(L, er.size());
	return 1;
}
LUA_FUNCTION(Remove) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	assert_readonly_group(L, pgroup);
	pgroup->is_iterator_dirty = true;
	card* pexception = 0;
	if(!lua_isnoneornil(L, 3))
		pexception = lua_get<card*, true>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	uint32_t extraargs = lua_gettop(L) - 3;
	for(auto cit = pgroup->container.begin(); cit != pgroup->container.end();) {
		auto rm = cit++;
		if((*rm) != pexception && pduel->lua->check_matching(*rm, findex, extraargs)) {
			pgroup->container.erase(rm);
		}
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
void get_groupcard(lua_State* L, group*& pgroup1, group*& pgroup2, card*& pcard) {
	auto obj1 = lua_get<lua_obj*>(L, 1);
	auto obj2 = lua_get<lua_obj*>(L, 2);
	uint32_t type = 0;
	if((!obj1 || !obj2) || ((type = obj1->lua_type | obj2->lua_type) & PARAM_TYPE_GROUP) == 0)
		lua_error(L, "At least 1 parameter should be \"Group\".");
	if((type & ~(PARAM_TYPE_GROUP | PARAM_TYPE_CARD)) != 0)
		lua_error(L, "A parameter isn't \"Group\" nor \"Card\".");
	if(obj1->lua_type == PARAM_TYPE_CARD) {
		pcard = static_cast<card*>(obj1);
		pgroup1 = static_cast<group*>(obj2);
	} else if(obj2->lua_type == PARAM_TYPE_CARD) {
		pgroup1 = static_cast<group*>(obj1);
		pcard = static_cast<card*>(obj2);
	} else {
		pgroup1 = static_cast<group*>(obj1);
		pgroup2 = static_cast<group*>(obj2);
	}
}
LUA_FUNCTION(__band) {
	check_param_count(L, 2);
	group* pgroup1 = nullptr;
	group* pgroup2 = nullptr;
	card* pcard = nullptr;
	get_groupcard(L, pgroup1, pgroup2, pcard);
	const auto pduel = lua_get<duel*>(L);
	card_set cset;
	if(pcard) {
		if(pgroup1->has_card(pcard)) {
			cset.insert(pcard);
		}
	} else {
		std::set_intersection(pgroup1->container.cbegin(), pgroup1->container.cend(), pgroup2->container.cbegin(), pgroup2->container.cend(),
							  std::inserter(cset, cset.begin()), card_sort());
	}
	interpreter::pushobject(L, pduel->new_group(std::move(cset)));
	return 1;
}
LUA_FUNCTION(__add) {
	check_param_count(L, 2);
	group* pgroup1 = nullptr;
	group* pgroup2 = nullptr;
	card* pcard = nullptr;
	get_groupcard(L, pgroup1, pgroup2, pcard);
	const auto pduel = lua_get<duel*>(L);
	group* newgroup = pduel->new_group(pgroup1);
	if(pcard) {
		newgroup->container.insert(pcard);
	} else {
		newgroup->container.insert(pgroup2->container.begin(), pgroup2->container.end());
	}
	interpreter::pushobject(L, newgroup);
	return 1;
}
LUA_FUNCTION(__sub) {
	check_param_count(L, 2);
	auto pgroup1 = lua_get<group*, true>(L, 1);
	group* pgroup2 = nullptr;
	card* pcard = nullptr;
	const auto pduel = lua_get<duel*>(L);
	get_card_or_group(L, 2, pcard, pgroup2);
	group* newgroup = pduel->new_group(pgroup1);
	if(pgroup2) {
		for(auto& _pcard : pgroup2->container)
			newgroup->container.erase(_pcard);
	} else
		newgroup->container.erase(pcard);
	interpreter::pushobject(L, newgroup);
	return 1;
}
LUA_FUNCTION(__len) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	lua_pushinteger(L, pgroup->container.size());
	return 1;
}
LUA_FUNCTION(__eq) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, pgroup->container.size() == sgroup->container.size());
	return 1;
}
LUA_FUNCTION(Equal) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, pgroup->container == sgroup->container);
	return 1;
}
LUA_FUNCTION(__lt) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, pgroup->container.size() < sgroup->container.size());
	return 1;
}
LUA_FUNCTION(__le) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, pgroup->container.size() <= sgroup->container.size());
	return 1;
}
LUA_FUNCTION(IsContains) {
	check_param_count(L, 2);
	const auto pgroup = lua_get<group*, true>(L, 1);
	auto pcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, pgroup->has_card(pcard));
	return 1;
}
LUA_FUNCTION(SearchCard) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	uint32_t extraargs = lua_gettop(L) - 2;
	for(auto& pcard : pgroup->container)
		if(pduel->lua->check_matching(pcard, findex, extraargs)) {
			interpreter::pushobject(L, pcard);
			return 1;
		}
	return 0;
}
LUA_FUNCTION(Split) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	card_set cset(pgroup->container);
	card_set notmatching;
	if(auto pexception = lua_get<card*>(L, 3)) {
		cset.erase(pexception);
		notmatching.insert(pexception);
	} else if(auto pexgroup = lua_get<group*>(L, 3)) {
		for(auto& pcard : pexgroup->container) {
			cset.erase(pcard);
			notmatching.insert(pcard);
		}
	}
	const auto pduel = lua_get<duel*>(L);
	uint32_t extraargs = lua_gettop(L) - 3;
	for(auto it = cset.begin(); it != cset.end();) {
		auto pcard = *it;
		if(pduel->lua->check_matching(pcard, findex, extraargs)) {
			++it;
		} else {
			notmatching.insert(pcard);
			it = cset.erase(it);
		}
	}
	interpreter::pushobject(L, pduel->new_group(std::move(cset)));
	interpreter::pushobject(L, pduel->new_group(std::move(notmatching)));
	return 2;
}
LUA_FUNCTION(Includes) {
	check_param_count(L, 2);
	auto pgroup1 = lua_get<group*, true>(L, 1);
	auto pgroup2 = lua_get<group*, true>(L, 2);
	int res = TRUE;
	if(pgroup1->container.size() < pgroup2->container.size())
		res = FALSE;
	else if(!pgroup2->container.empty())
		res = std::includes(pgroup1->container.cbegin(), pgroup1->container.cend(), pgroup2->container.cbegin(), pgroup2->container.cend(), card_sort());
	lua_pushboolean(L, res);
	return 1;
}
LUA_FUNCTION(GetBinClassCount) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t extraargs = lua_gettop(L) - 2;
	uint64_t er = 0;
	for(auto& pcard : pgroup->container) {
		er |= static_cast<uint64_t>(pduel->lua->get_operation_value(pcard, findex, extraargs));
	}
	int32_t ans = 0;
	while(er) {
		er &= er - 1;
		++ans;
	}
	lua_pushinteger(L, ans);
	return 1;
}
LUA_FUNCTION_EXISTING(GetLuaRef, get_lua_ref<group>);
LUA_FUNCTION_EXISTING(FromLuaRef, from_lua_ref<group>);
LUA_FUNCTION_EXISTING(IsDeleted, is_deleted_object);
}

void scriptlib::push_group_lib(lua_State* L) {
	static constexpr auto grouplib = GET_LUA_FUNCTIONS_ARRAY();
	static_assert(grouplib.back().name == nullptr, "");
	lua_createtable(L, 0, static_cast<int>(grouplib.size() - 1));
	luaL_setfuncs(L, grouplib.data(), 0);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Group");
}
