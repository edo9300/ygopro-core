/*
 * Copyright (c) 2010-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2017-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <algorithm> //std::find, std::remove, std::includes, std::set_intersection
#include <iterator> //std::advance, std::inserter
#include <set>
#include <tuple>
#include <utility> //std::move, std::swap
#include "bit.h"
#include "card.h"
#include "duel.h"
#include "field.h"
#include "group.h"
#include "scriptlib.h"

#define LUA_MODULE Group
using LUA_CLASS = group;
#include "function_array_helper.h"

namespace {

using namespace scriptlib;

void assert_readonly_group(lua_State* L, group* pgroup) {
	if(pgroup->is_readonly != 1)
		return;
	lua_error(L, "attempt to modify a read only group");
}

LUA_STATIC_FUNCTION(CreateGroup) {
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
	group* newgroup = pduel->new_group(self);
	interpreter::pushobject(L, newgroup);
	return 1;
}
LUA_FUNCTION(DeleteGroup) {
	if(self->is_readonly != 2)
		return 0;
	self->is_readonly = 0;
	pduel->sgroups.insert(self);
	return 0;
}
LUA_FUNCTION(KeepAlive) {
	if(self->is_readonly != 1) {
		self->is_readonly = 2;
		pduel->sgroups.erase(self);
	}
	interpreter::pushobject(L, self);
	return 1;
}
LUA_FUNCTION(Clear) {
	assert_readonly_group(L, self);
	self->is_iterator_dirty = true;
	self->container.clear();
	interpreter::pushobject(L, self);
	return 1;
}
LUA_FUNCTION(AddCard) {
	check_param_count(L, 2);
	assert_readonly_group(L, self);
	self->is_iterator_dirty = true;
	if(auto [pcard, pgroup] = lua_get_card_or_group(L, 2); pcard)
		self->container.insert(pcard);
	else
		self->container.insert(pgroup->container.begin(), pgroup->container.end());
	interpreter::pushobject(L, self);
	return 1;
}
LUA_FUNCTION_ALIAS(Merge);
LUA_FUNCTION(RemoveCard) {
	check_param_count(L, 2);
	assert_readonly_group(L, self);
	self->is_iterator_dirty = true;
	if(auto [pcard, pgroup] = lua_get_card_or_group(L, 2); pcard)
		self->container.erase(pcard);
	else {
		if(self == pgroup)
			lua_error(L, "Attempting to remove a group from itself");
		for(auto& _pcard : pgroup->container)
			self->container.erase(_pcard);
	}
	interpreter::pushobject(L, self);
	return 1;
}
LUA_FUNCTION_ALIAS(Sub);
LUA_FUNCTION(GetNext) {
	if(self->is_iterator_dirty)
		lua_error(L, "Called Group.GetNext without first calling Group.GetFirst");
	if(self->it == self->container.end() || (++self->it) == self->container.end())
		lua_pushnil(L);
	else
		interpreter::pushobject(L, *self->it);
	return 1;
}
LUA_FUNCTION(GetFirst) {
	self->is_iterator_dirty = false;
	if(self->it = self->container.begin(); self->it != self->container.end())
		interpreter::pushobject(L, *self->it);
	else
		lua_pushnil(L);
	return 1;
}
LUA_FUNCTION(TakeatPos) {
	check_param_count(L, 2);
	auto pos = lua_get<size_t>(L, 2);
	if(pos >= self->container.size())
		lua_pushnil(L);
	else {
		auto cit = self->container.begin();
		std::advance(cit, pos);
		interpreter::pushobject(L, *cit);
	}
	return 1;
}
LUA_FUNCTION(GetCount) {
	lua_pushinteger(L, self->container.size());
	return 1;
}
LUA_FUNCTION(Filter) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	card_set cset(self->container);
	if(auto [pexception, pexgroup] = lua_get_card_or_group<true>(L, 3); pexception) {
		cset.erase(pexception);
	} else if(pexgroup) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
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
	assert_readonly_group(L, self);
	self->is_iterator_dirty = true;
	uint32_t extraargs = lua_gettop(L) - 3;
	auto& cset = self->container;
	if(auto [pexception, pexgroup] = lua_get_card_or_group<true>(L, 3); pexception) {
		for(auto cit = cset.begin(), cend = cset.end(); cit != cend; ) {
			auto rm = cit++;
			auto* pcard = *rm;
			if(pcard == pexception || !pduel->lua->check_matching(pcard, findex, extraargs))
				cset.erase(rm);
		}
	} else if(pexgroup) {
		auto should_remove = [pexbegin = pexgroup->container.cbegin(), pexend = pexgroup->container.cend()](card* pcard) mutable {
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
	interpreter::pushobject(L, self);
	return 1;
}
LUA_FUNCTION(FilterCount) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	card_set cset(self->container);
	if(auto [pexception, pexgroup] = lua_get_card_or_group<true>(L, 3); pexception) {
		cset.erase(pexception);
	} else if(pexgroup) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
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
	card_set cset(self->container);
	bool cancelable = false;
	uint8_t lastarg = 6;
	if(lua_isboolean(L, lastarg)) {
		cancelable = lua_get<bool, false>(L, lastarg);
		++lastarg;
	}
	if(auto [pexception, pexgroup] = lua_get_card_or_group<true>(L, lastarg); pexception) {
		cset.erase(pexception);
	} else if(pexgroup) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
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
	pduel->game_field->emplace_process<Processors::SelectCard>(playerid, cancelable, min, max);
	return push_return_cards(L, cancelable);
}
LUA_FUNCTION(Select) {
	check_action_permission(L);
	check_param_count(L, 5);
	card_set cset(self->container);
	bool cancelable = false;
	uint8_t lastarg = 5;
	if(lua_isboolean(L, lastarg)) {
		cancelable = lua_get<bool, false>(L, lastarg);
		++lastarg;
	}
	if(auto [pexception, pexgroup] = lua_get_card_or_group<true>(L, lastarg); pexception) {
		cset.erase(pexception);
	} else if(pexgroup) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto min = lua_get<uint16_t>(L, 3);
	auto max = lua_get<uint16_t>(L, 4);
	pduel->game_field->core.select_cards.assign(cset.begin(), cset.end());
	pduel->game_field->emplace_process<Processors::SelectCard>(playerid, cancelable, min, max);
	return push_return_cards(L, cancelable);
}
LUA_FUNCTION(SelectUnselect) {
	check_action_permission(L);
	check_param_count(L, 3);
	auto pgroup2 = lua_get<group*>(L, 2);
	auto playerid = lua_get<uint8_t>(L, 3);
	if(playerid != 0 && playerid != 1)
		return 0;
	if(pgroup2) {
		auto first1 = self->container.begin();
		auto last1 = self->container.end();
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
	pduel->game_field->core.select_cards.assign(self->container.begin(), self->container.end());
	if(pgroup2)
		pduel->game_field->core.unselect_cards.assign(pgroup2->container.begin(), pgroup2->container.end());
	else
		pduel->game_field->core.unselect_cards.clear();
	pduel->game_field->emplace_process<Processors::SelectUnselectCard>(playerid, cancelable, min, max, finishable);
	return yieldk({
		if(pduel->game_field->return_cards.canceled)
			lua_pushnil(L);
		else
			interpreter::pushobject(L, pduel->game_field->return_cards.list[0]);
		return 1;
	});
}
LUA_FUNCTION(RandomSelect) {
	check_param_count(L, 3);
	auto playerid = lua_get<uint8_t>(L, 2);
	size_t count = lua_get<uint32_t>(L, 3);
	group* newgroup = pduel->new_group();
	if(count > self->container.size())
		count = self->container.size();
	if(count == 0) {
		interpreter::pushobject(L, newgroup);
		return 1;
	}
	if(count == self->container.size())
		newgroup->container = self->container;
	else {
		while(newgroup->container.size() < count) {
			int32_t i = pduel->get_next_integer(0, (int32_t)self->container.size() - 1);
			auto cit = self->container.begin();
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
	card_set cset(self->container);
	if(auto [pexception, pexgroup] = lua_get_card_or_group<true>(L, 4); pexception) {
		cset.erase(pexception);
	} else if(pexgroup) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
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
	for(auto& pcard : self->container) {
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
	pduel->game_field->core.select_cards.assign(self->container.begin(), self->container.end());
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
	pduel->game_field->emplace_process<Processors::SelectSum>(playerid, acc, min, max);
	return yieldk({
		group* pgroup = pduel->new_group(pduel->game_field->return_cards.list);
		pduel->game_field->core.must_select_cards.clear();
		interpreter::pushobject(L, pgroup);
		return 1;
	});
}
LUA_FUNCTION(CheckWithSumGreater) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	auto acc = lua_get<uint32_t>(L, 3);
	int32_t extraargs = lua_gettop(L) - 3;
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = static_cast<int32_t>(cv.size());
	const auto beginit = pduel->game_field->core.must_select_cards.begin();
	const auto endit = pduel->game_field->core.must_select_cards.end();
	for(auto& pcard : self->container) {
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
	auto playerid = lua_get<uint8_t>(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	auto acc = lua_get<uint32_t>(L, 4);
	int32_t extraargs = lua_gettop(L) - 4;
	pduel->game_field->core.select_cards.assign(self->container.begin(), self->container.end());
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
	pduel->game_field->emplace_process<Processors::SelectSum>(playerid, acc, 0, 0);
	return yieldk({
		group* pgroup = pduel->new_group(pduel->game_field->return_cards.list);
		pduel->game_field->core.must_select_cards.clear();
		interpreter::pushobject(L, pgroup);
		return 1;
	});
}
LUA_FUNCTION(GetMinGroup) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	if(self->container.size() == 0)
		return 0;
	group* newgroup = pduel->new_group();
	int64_t min, op;
	int32_t extraargs = lua_gettop(L) - 2;
	auto cit = self->container.begin();
	min = pduel->lua->get_operation_value(*cit, findex, extraargs);
	newgroup->container.insert(*cit);
	++cit;
	for(; cit != self->container.end(); ++cit) {
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
	if(self->container.size() == 0)
		return 0;
	group* newgroup = pduel->new_group();
	int64_t max, op;
	int32_t extraargs = lua_gettop(L) - 2;
	auto cit = self->container.begin();
	max = pduel->lua->get_operation_value(*cit, findex, extraargs);
	newgroup->container.insert(*cit);
	++cit;
	for(; cit != self->container.end(); ++cit) {
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
	int32_t extraargs = lua_gettop(L) - 2;
	int64_t sum = 0;
	for(auto& pcard : self->container) {
		sum += pduel->lua->get_operation_value(pcard, findex, extraargs);
	}
	lua_pushinteger(L, sum);
	return 1;
}
LUA_FUNCTION(GetBitwiseAnd) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	int32_t extraargs = lua_gettop(L) - 2;
	uint64_t total = 0;
	for(auto& pcard : self->container) {
		total &= static_cast<uint64_t>(pduel->lua->get_operation_value(pcard, findex, extraargs));
	}
	lua_pushinteger(L, total);
	return 1;
}
LUA_FUNCTION(GetBitwiseOr) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	int32_t extraargs = lua_gettop(L) - 2;
	uint64_t total = 0;
	for(auto& pcard : self->container) {
		total |= static_cast<uint64_t>(pduel->lua->get_operation_value(pcard, findex, extraargs));
	}
	lua_pushinteger(L, total);
	return 1;
}
LUA_FUNCTION(GetClass) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	int32_t extraargs = lua_gettop(L) - 2;
	std::set<int64_t> er;
	for(auto& pcard : self->container) {
		er.insert(pduel->lua->get_operation_value(pcard, findex, extraargs));
	}
	lua_createtable(L, er.size(), 0);
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
	int32_t extraargs = lua_gettop(L) - 2;
	std::set<int64_t> er;
	for(auto& pcard : self->container) {
		er.insert(pduel->lua->get_operation_value(pcard, findex, extraargs));
	}
	lua_pushinteger(L, er.size());
	return 1;
}
LUA_FUNCTION(Remove) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	assert_readonly_group(L, self);
	self->is_iterator_dirty = true;
	uint32_t extraargs = lua_gettop(L) - 3;
	auto& cset = self->container;
	if(auto [pexception, pexgroup] = lua_get_card_or_group<true>(L, 3); pexception) {
		for(auto cit = cset.begin(), cend = cset.end(); cit != cend; ) {
			auto rm = cit++;
			auto* pcard = *rm;
			if(pcard != pexception && pduel->lua->check_matching(pcard, findex, extraargs))
				cset.erase(rm);
		}
	} else if(pexgroup) {
		auto should_keep = [pexbegin = pexgroup->container.cbegin(), pexend = pexgroup->container.cend()](card* pcard) mutable {
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
			if(!should_keep(pcard) && pduel->lua->check_matching(pcard, findex, extraargs))
				cset.erase(rm);
		}
	} else {
		for(auto cit = cset.begin(), cend = cset.end(); cit != cend; ) {
			auto rm = cit++;
			auto* pcard = *rm;
			if(pduel->lua->check_matching(pcard, findex, extraargs))
				cset.erase(rm);
		}
	}
	interpreter::pushobject(L, self);
	return 1;
}
std::tuple<group*, group*, card*> get_binary_op_group_card_parameters(lua_State* L) {
	auto obj1 = lua_get<lua_obj*>(L, 1);
	auto obj2 = lua_get<lua_obj*>(L, 2);
	if(!obj1 || !obj2)
		lua_error(L, "At least 1 parameter should be \"Group\".");
	if(obj1->lua_type != LuaParam::GROUP)
		std::swap(obj1, obj2);
	if(obj1->lua_type != LuaParam::GROUP)
		lua_error(L, "At least 1 parameter should be \"Group\".");

	switch(obj2->lua_type) {
	case LuaParam::GROUP:
		return { static_cast<group*>(obj1), static_cast<group*>(obj2), nullptr};
	case LuaParam::CARD:
		return { static_cast<group*>(obj1), nullptr, static_cast<card*>(obj2) };
	default:
		lua_error(L, "A parameter isn't \"Group\" nor \"Card\".");
	}
}
LUA_STATIC_FUNCTION(__band) {
	check_param_count(L, 2);
	card_set cset;
	if(auto [pgroup1, pgroup2, pcard] = get_binary_op_group_card_parameters(L); pcard) {
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
LUA_STATIC_FUNCTION(__add) {
	check_param_count(L, 2);
	auto [pgroup1, pgroup2, pcard] = get_binary_op_group_card_parameters(L);
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
	auto [pcard, pgroup] = lua_get_card_or_group(L, 2);
	group* newgroup = pduel->new_group(self);
	if(pgroup) {
		for(auto& _pcard : pgroup->container)
			newgroup->container.erase(_pcard);
	} else
		newgroup->container.erase(pcard);
	interpreter::pushobject(L, newgroup);
	return 1;
}
LUA_FUNCTION(__len) {
	lua_pushinteger(L, self->container.size());
	return 1;
}
LUA_FUNCTION(__eq) {
	check_param_count(L, 2);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, self->container.size() == sgroup->container.size());
	return 1;
}
LUA_FUNCTION(Equal) {
	check_param_count(L, 2);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, self->container == sgroup->container);
	return 1;
}
LUA_FUNCTION(__lt) {
	check_param_count(L, 2);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, self->container.size() < sgroup->container.size());
	return 1;
}
LUA_FUNCTION(__le) {
	check_param_count(L, 2);
	auto sgroup = lua_get<group*, true>(L, 2);
	lua_pushboolean(L, self->container.size() <= sgroup->container.size());
	return 1;
}
LUA_FUNCTION(IsContains) {
	check_param_count(L, 2);
	auto pcard = lua_get<card*, true>(L, 2);
	lua_pushboolean(L, self->has_card(pcard));
	return 1;
}
LUA_FUNCTION(SearchCard) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	uint32_t extraargs = lua_gettop(L) - 2;
	for(auto& pcard : self->container)
		if(pduel->lua->check_matching(pcard, findex, extraargs)) {
			interpreter::pushobject(L, pcard);
			return 1;
		}
	return 0;
}
LUA_FUNCTION(Split) {
	check_param_count(L, 3);
	const auto findex = lua_get<function, true>(L, 2);
	card_set cset(self->container);
	card_set notmatching;
	if(auto [pexception, pexgroup] = lua_get_card_or_group<true>(L, 3); pexception) {
		cset.erase(pexception);
		notmatching.insert(pexception);
	} else if(pexgroup) {
		for(auto& pcard : pexgroup->container) {
			cset.erase(pcard);
			notmatching.insert(pcard);
		}
	}
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
	auto pgroup2 = lua_get<group*, true>(L, 2);
	int res = TRUE;
	if(self->container.size() < pgroup2->container.size())
		res = FALSE;
	else if(!pgroup2->container.empty())
		res = std::includes(self->container.cbegin(), self->container.cend(), pgroup2->container.cbegin(), pgroup2->container.cend(), card_sort());
	lua_pushboolean(L, res);
	return 1;
}
LUA_FUNCTION(GetBinClassCount) {
	check_param_count(L, 2);
	const auto findex = lua_get<function, true>(L, 2);
	int32_t extraargs = lua_gettop(L) - 2;
	uint64_t er = 0;
	for(auto& pcard : self->container)
		er |= static_cast<uint64_t>(pduel->lua->get_operation_value(pcard, findex, extraargs));
	lua_pushinteger(L, bit::popcnt(er));
	return 1;
}
LUA_FUNCTION_EXISTING(GetLuaRef, get_lua_ref<group>);
LUA_FUNCTION_EXISTING(FromLuaRef, from_lua_ref<group>);
LUA_FUNCTION_EXISTING(IsDeleted, is_deleted_object);
}

void scriptlib::push_group_lib(lua_State* L) {
	static constexpr auto grouplib = GET_LUA_FUNCTIONS_ARRAY();
	static_assert(grouplib.back().name == nullptr);
	lua_createtable(L, 0, static_cast<int>(grouplib.size() - 1));
	luaL_setfuncs(L, grouplib.data(), 0);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Group");
}
