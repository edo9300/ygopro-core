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

namespace {

using namespace scriptlib;

void assert_readonly_group(lua_State* L, group* pgroup) {
	if(pgroup->is_readonly != 1)
		return;
	luaL_error(L, "attempt to modify a read only group");
	unreachable();
}

int32_t group_new(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t group_clone(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pgroup = lua_get<group*, true>(L, 1);
	group* newgroup = pduel->new_group(pgroup);
	interpreter::pushobject(L, newgroup);
	return 1;
}
int32_t group_from_cards(lua_State* L) {
	const auto pduel = lua_get<duel*>(L);
	group* pgroup = pduel->new_group();
	for(int32_t i = 0; i < lua_gettop(L); ++i) {
		auto pcard = lua_get<card*, true>(L, i + 1);
		pgroup->container.insert(pcard);
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t group_delete(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pgroup = lua_get<group*, true>(L, 1);
	if(pgroup->is_readonly != 2)
		return 0;
	pgroup->is_readonly = 0;
	pduel->sgroups.insert(pgroup);
	return 0;
}
int32_t group_keep_alive(lua_State* L) {
	check_param_count(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto pgroup = lua_get<group*, true>(L, 1);
	if(pgroup->is_readonly == 1)
		return 0;
	pgroup->is_readonly = 2;
	pduel->sgroups.erase(pgroup);
	return 0;
}
int32_t group_clear(lua_State* L) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	assert_readonly_group(L, pgroup);
	pgroup->container.clear();
	return 0;
}
int32_t group_add_card(lua_State* L) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	assert_readonly_group(L, pgroup);
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
int32_t group_remove_card(lua_State* L) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	assert_readonly_group(L, pgroup);
	card* pcard = nullptr;
	group* sgroup = nullptr;
	get_card_or_group(L, 2, pcard, sgroup);
	if(pcard)
		pgroup->container.erase(pcard);
	else {
		for(auto& pcard : sgroup->container) {
			pgroup->container.erase(pcard);
		}
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t group_get_next(lua_State* L) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
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
int32_t group_get_first(lua_State* L) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	if(pgroup->container.size())
		interpreter::pushobject(L, *(pgroup->it = pgroup->container.begin()));
	else
		lua_pushnil(L);
	return 1;
}
int32_t group_take_at_pos(lua_State* L) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto pos = lua_get<size_t>(L, 2);
	if(pos > pgroup->container.size())
		lua_pushnil(L);
	else {
		auto cit = pgroup->container.begin();
		std::advance(cit, pos);
		interpreter::pushobject(L, *cit);
	}
	return 1;
}
int32_t group_get_count(lua_State* L) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	lua_pushinteger(L, pgroup->container.size());
	return 1;
}
int32_t group_filter(lua_State* L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
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
		if(pduel->lua->check_matching(pcard, 2, extraargs)) {
			new_group->container.insert(pcard);
		}
	}
	interpreter::pushobject(L, new_group);
	return 1;
}
int32_t group_filter_in_place(lua_State* L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	assert_readonly_group(L, pgroup);
	card* pexception = nullptr;
	group* pexgroup = nullptr;
	card_set::const_iterator pexbegin, pexend;
	if(!(pexception = lua_get<card*>(L, 3))) {
		if((pexgroup = lua_get<group*>(L, 3)) != nullptr) {
			pexbegin = pexgroup->container.cbegin();
			pexend = pexgroup->container.cend();
		}
	}
	auto check_card = [&](card* pcard) {
		if(!pexception && !pexgroup)
			return true;
		if(pexception)
			return pcard != pexception;
		if(pexbegin == pexend)
			return true;
		if(*pexbegin == pcard) {
			pexbegin++;
			return false;
		}
		return true;
	};
	const auto pduel = lua_get<duel*>(L);
	uint32_t extraargs = lua_gettop(L) - 3;
	auto& cset = pgroup->container;
	for(auto cit = cset.begin(); cit != cset.end(); ) {
		auto rm = cit++;
		if(!check_card(*rm) || !pduel->lua->check_matching(*rm, 2, extraargs))
			cset.erase(rm);
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
int32_t group_filter_count(lua_State* L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	card_set cset(pgroup->container);
	if(auto pexception = lua_get<card*>(L, 3))
		cset.erase(pexception);
	else if(auto pexgroup = lua_get<group*>(L, 3)) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	const auto pduel = lua_get<duel*>(L);
	uint32_t extraargs = lua_gettop(L) - 3;
	uint32_t count = 0;
	for (auto& pcard : cset) {
		if(pduel->lua->check_matching(pcard, 2, extraargs))
			count++;
	}
	lua_pushinteger(L, count);
	return 1;
}
int32_t group_filter_select(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 6);
	check_param(L, PARAM_TYPE_FUNCTION, 3);
	auto pgroup = lua_get<group*, true>(L, 1);
	card_set cset(pgroup->container);
	bool cancelable = false;
	uint8_t lastarg = 6;
	if(lua_isboolean(L, lastarg)) {
		cancelable = lua_get<bool, false>(L, lastarg);
		lastarg++;
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
		if(pduel->lua->check_matching(pcard, 3, extraargs))
			pduel->game_field->core.select_cards.push_back(pcard);
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16));
	return lua_yieldk(L, 0, (lua_KContext)cancelable, push_return_cards);
}
int32_t group_select(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 5);
	auto pgroup = lua_get<group*, true>(L, 1);
	card_set cset(pgroup->container);
	bool cancelable = false;
	uint8_t lastarg = 5;
	if(lua_isboolean(L, lastarg)) {
		cancelable = lua_get<bool, false>(L, lastarg);
		lastarg++;
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
int32_t group_select_unselect(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto pgroup1 = lua_get<group*, true>(L, 1);
	auto pgroup2 = lua_get<group*, true>(L, 2);
	const auto pduel = lua_get<duel*>(L);
	auto playerid = lua_get<uint8_t>(L, 3);
	if(playerid != 0 && playerid != 1)
		return 0;
	if(pgroup1->container == pgroup2->container)
		return 0;
	for(auto& pcard : pgroup2->container) {
		for(auto& pcard2 : pgroup1->container) {
			if(pcard2->cardid > pcard->cardid)
				break;
			if(pcard2->cardid == pcard->cardid)
				return 0;
		}
	}
	bool finishable = lua_get<bool, false>(L, 4);
	bool cancelable = lua_get<bool, false>(L, 5);
	uint16_t min = 1;
	if(lua_gettop(L) > 5) {
		min = lua_get<uint16_t>(L, 6);
	}
	uint16_t max = 1;
	if(lua_gettop(L) > 6) {
		max = lua_get<uint16_t>(L, 7);
	}
	if(min > max)
		min = max;
	pduel->game_field->core.select_cards.assign(pgroup1->container.begin(), pgroup1->container.end());
	pduel->game_field->core.unselect_cards.assign(pgroup2->container.begin(), pgroup2->container.end());
	pduel->game_field->add_process(PROCESSOR_SELECT_UNSELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16), finishable);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		if(pduel->game_field->return_cards.canceled)
			lua_pushnil(L);
		else
			interpreter::pushobject(L, pduel->game_field->return_cards.list[0]);
		return 1;
	});
}
int32_t group_random_select(lua_State* L) {
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
int32_t group_is_exists(lua_State* L) {
	check_param_count(L, 4);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
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
		if(pduel->lua->check_matching(pcard, 2, extraargs)) {
			fcount++;
			if(fcount >= count)
				break;
		}
	}
	lua_pushboolean(L, fcount >= count);
	return 1;
}
int32_t group_check_with_sum_equal(lua_State* L) {
	check_param_count(L, 5);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
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
	int32_t mcount = cv.size();
	const auto beginit = pduel->game_field->core.must_select_cards.begin();
	const auto endit = pduel->game_field->core.must_select_cards.end();
	for(auto& pcard : pgroup->container) {
		if(std::find(beginit, endit, pcard) == endit)
			cv.push_back(pcard);
	}
	pduel->game_field->core.must_select_cards.clear();
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, 2, extraargs);
	int32_t should_continue = TRUE;
	lua_pushboolean(L, field::check_with_sum_limit_m(cv, acc, 0, min, max, mcount, &should_continue));
	lua_pushboolean(L, should_continue);
	return 2;
}
int32_t group_select_with_sum_equal(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 6);
	check_param(L, PARAM_TYPE_FUNCTION, 3);
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
	int32_t mcount = cv.size();
	cv.insert(cv.end(), pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end());
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, 3, extraargs);
	if(!field::check_with_sum_limit_m(cv, acc, 0, min, max, mcount, nullptr)) {
		pduel->game_field->core.must_select_cards.clear();
		group* empty_group = pduel->new_group();
		interpreter::pushobject(L, empty_group);
		return 1;
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_SUM, 0, 0, 0, acc, playerid + (min << 16) + (max << 24));
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		group* pgroup = pduel->new_group(pduel->game_field->return_cards.list);
		pduel->game_field->core.must_select_cards.clear();
		interpreter::pushobject(L, pgroup);
		return 1;
	});
}
int32_t group_check_with_sum_greater(lua_State* L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	auto acc = lua_get<uint32_t>(L, 3);
	int32_t extraargs = lua_gettop(L) - 3;
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = cv.size();
	const auto beginit = pduel->game_field->core.must_select_cards.begin();
	const auto endit = pduel->game_field->core.must_select_cards.end();
	for(auto& pcard : pgroup->container) {
		if(std::find(beginit, endit, pcard) == endit)
			cv.push_back(pcard);
	}
	pduel->game_field->core.must_select_cards.clear();
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, 2, extraargs);
	int32_t should_continue = TRUE;
	lua_pushboolean(L, field::check_with_sum_greater_limit_m(cv, acc, 0, 0xffff, mcount, &should_continue));
	lua_pushboolean(L, should_continue);
	return 2;
}
int32_t group_select_with_sum_greater(lua_State* L) {
	check_action_permission(L);
	check_param_count(L, 4);
	check_param(L, PARAM_TYPE_FUNCTION, 3);
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
	int32_t mcount = cv.size();
	cv.insert(cv.end(), pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end());
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, 3, extraargs);
	if(!field::check_with_sum_greater_limit_m(cv, acc, 0, 0xffff, mcount, nullptr)) {
		pduel->game_field->core.must_select_cards.clear();
		group* empty_group = pduel->new_group();
		interpreter::pushobject(L, empty_group);
		return 1;
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_SUM, 0, 0, 0, acc, playerid);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State* L, int32_t/* status*/, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		group* pgroup = pduel->new_group(pduel->game_field->return_cards.list);
		pduel->game_field->core.must_select_cards.clear();
		interpreter::pushobject(L, pgroup);
		return 1;
					  });
}
int32_t group_get_min_group(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	if(pgroup->container.size() == 0)
		return 0;
	group* newgroup = pduel->new_group();
	int32_t min, op;
	int32_t extraargs = lua_gettop(L) - 2;
	auto cit = pgroup->container.begin();
	min = pduel->lua->get_operation_value(*cit, 2, extraargs);
	newgroup->container.insert(*cit);
	++cit;
	for(; cit != pgroup->container.end(); ++cit) {
		op = pduel->lua->get_operation_value(*cit, 2, extraargs);
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
int32_t group_get_max_group(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	if(pgroup->container.size() == 0)
		return 0;
	group* newgroup = pduel->new_group();
	int32_t max, op;
	int32_t extraargs = lua_gettop(L) - 2;
	auto cit = pgroup->container.begin();
	max = pduel->lua->get_operation_value(*cit, 2, extraargs);
	newgroup->container.insert(*cit);
	++cit;
	for(; cit != pgroup->container.end(); ++cit) {
		op = pduel->lua->get_operation_value(*cit, 2, extraargs);
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
int32_t group_get_sum(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t extraargs = lua_gettop(L) - 2;
	int32_t sum = 0;
	for(auto& pcard : pgroup->container) {
		sum += pduel->lua->get_operation_value(pcard, 2, extraargs);
	}
	lua_pushinteger(L, sum);
	return 1;
}
int32_t group_get_class(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t extraargs = lua_gettop(L) - 2;
	std::set<uint32_t> er;
	for(auto& pcard : pgroup->container) {
		er.insert(pduel->lua->get_operation_value(pcard, 2, extraargs));
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
int32_t group_get_class_count(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t extraargs = lua_gettop(L) - 2;
	std::set<uint32_t> er;
	for(auto& pcard : pgroup->container) {
		er.insert(pduel->lua->get_operation_value(pcard, 2, extraargs));
	}
	lua_pushinteger(L, er.size());
	return 1;
}
int32_t group_remove(lua_State* L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	assert_readonly_group(L, pgroup);
	card* pexception = 0;
	if(!lua_isnoneornil(L, 3))
		pexception = lua_get<card*, true>(L, 3);
	const auto pduel = lua_get<duel*>(L);
	uint32_t extraargs = lua_gettop(L) - 3;
	for(auto cit = pgroup->container.begin(); cit != pgroup->container.end();) {
		auto rm = cit++;
		if((*rm) != pexception && pduel->lua->check_matching(*rm, 2, extraargs)) {
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
	if((!obj1 || !obj2) || ((type = obj1->lua_type | obj2->lua_type) & PARAM_TYPE_GROUP) == 0) {
		luaL_error(L, "At least 1 parameter should be \"Group\".");
		unreachable();
	}
	if((type & ~(PARAM_TYPE_GROUP | PARAM_TYPE_CARD)) != 0) {
		luaL_error(L, "A parameter isn't \"Group\" nor \"Card\".");
		unreachable();
	}
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
int32_t group_band(lua_State* L) {
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
int32_t group_add(lua_State* L) {
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
int32_t group_sub_const(lua_State* L) {
	check_param_count(L, 2);
	auto pgroup1 = lua_get<group*, true>(L, 1);
	group* pgroup2 = nullptr;
	card* pcard = nullptr;
	const auto pduel = lua_get<duel*>(L);
	get_card_or_group(L, 2, pcard, pgroup2);
	group* newgroup = pduel->new_group(pgroup1);
	if(pgroup2) {
		for(auto& _pcard : pgroup2->container) {
			newgroup->container.erase(_pcard);
		}
	} else
		newgroup->container.erase(pcard);
	interpreter::pushobject(L, newgroup);
	return 1;
}
int32_t group_len(lua_State* L) {
	check_param_count(L, 1);
	auto pgroup = lua_get<group*, true>(L, 1);
	lua_pushinteger(L, pgroup->container.size());
	return 1;
}
int32_t group_equal_size(lua_State* L) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, pgroup->container.size() == sgroup->container.size());
	return 1;
}
int32_t group_equal(lua_State* L) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, pgroup->container == sgroup->container);
	return 1;
}
int32_t group_less_than(lua_State* L) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, pgroup->container.size() < sgroup->container.size());
	return 1;
}
int32_t group_less_equal_than(lua_State* L) {
	check_param_count(L, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, pgroup->container.size() <= sgroup->container.size());
	return 1;
}
int32_t group_is_contains(lua_State* L) {
	check_param_count(L, 2);
	const auto pgroup = lua_get<group*, true>(L, 1);
	auto pcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, pgroup->has_card(pcard));
	return 1;
}
int32_t group_search_card(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	uint32_t extraargs = lua_gettop(L) - 2;
	for(auto& pcard : pgroup->container)
		if(pduel->lua->check_matching(pcard, 2, extraargs)) {
			interpreter::pushobject(L, pcard);
			return 1;
		}
	return 0;
}
int32_t group_split(lua_State* L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
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
	group* new_group = pduel->new_group();
	uint32_t extraargs = lua_gettop(L) - 3;
	for(auto& pcard : cset) {
		if(pduel->lua->check_matching(pcard, 2, extraargs)) {
			new_group->container.insert(pcard);
		} else {
			notmatching.insert(pcard);
		}
	}
	interpreter::pushobject(L, new_group);
	interpreter::pushobject(L, pduel->new_group(std::move(notmatching)));
	return 2;
}
int32_t group_includes(lua_State* L) {
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
int32_t group_get_bin_class_count(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	auto pgroup = lua_get<group*, true>(L, 1);
	const auto pduel = lua_get<duel*>(L);
	int32_t extraargs = lua_gettop(L) - 2;
	int32_t er = 0;
	for(auto& pcard : pgroup->container) {
		er |= pduel->lua->get_operation_value(pcard, 2, extraargs);
	}
	int32_t ans = 0;
	while(er) {
		er &= er - 1;
		ans++;
	}
	lua_pushinteger(L, ans);
	return 1;
}

static constexpr luaL_Reg grouplib[] = {
	{ "__band", group_band },
	{ "__add", group_add },
	{ "__sub", group_sub_const },
	{ "__len", group_len },
	{ "__eq", group_equal_size },
	{ "__lt", group_less_than },
	{ "__le", group_less_equal_than },
	{ "CreateGroup", group_new },
	{ "KeepAlive", group_keep_alive },
	{ "DeleteGroup", group_delete },
	{ "Clone", group_clone },
	{ "FromCards", group_from_cards },
	{ "Clear", group_clear },
	{ "AddCard", group_add_card },
	{ "RemoveCard", group_remove_card },
	{ "Merge", group_add_card },
	{ "Sub", group_remove_card },
	{ "GetNext", group_get_next },
	{ "GetFirst", group_get_first },
	{ "TakeatPos", group_take_at_pos },
	{ "GetCount", group_get_count },
	{ "Filter", group_filter },
	{ "Match", group_filter_in_place },
	{ "FilterCount", group_filter_count },
	{ "FilterSelect", group_filter_select },
	{ "Select", group_select },
	{ "SelectUnselect", group_select_unselect },
	{ "RandomSelect", group_random_select },
	{ "IsExists", group_is_exists },
	{ "CheckWithSumEqual", group_check_with_sum_equal },
	{ "SelectWithSumEqual", group_select_with_sum_equal },
	{ "CheckWithSumGreater", group_check_with_sum_greater },
	{ "SelectWithSumGreater", group_select_with_sum_greater },
	{ "GetMinGroup", group_get_min_group },
	{ "GetMaxGroup", group_get_max_group },
	{ "GetSum", group_get_sum },
	{ "GetClass", group_get_class },
	{ "GetClassCount", group_get_class_count },
	{ "Remove", group_remove },
	{ "Equal", group_equal },
	{ "IsContains", group_is_contains },
	{ "SearchCard", group_search_card },
	{ "Split", group_split },
	{ "Includes", group_includes },
	{ "GetLuaRef", get_lua_ref<group> },
	{ "FromLuaRef", from_lua_ref<group> },
	{ "IsDeleted", is_deleted_object },
	{ NULL, NULL }
};
}

void scriptlib::push_group_lib(lua_State* L) {
	luaL_newlib(L, grouplib);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Group");
}