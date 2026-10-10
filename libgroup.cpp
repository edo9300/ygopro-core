/*
 * Copyright (c) 2010-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2017-2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
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
#define LUA_CLASS group
#include "function_array_helper.h"

namespace {
namespace LUA_NAMESPACE {

using namespace scriptlib;

#define assert_readonly_group(L, pgroup) do { \
	if(pgroup->is_readonly) \
		lua_error(L, "attempt to modify a read only group"); \
} while(0)

LUA_STATIC_FUNCTION(CreateGroup, lua_range<std::optional<card*>> cards) {
	auto pgroup = pduel->new_group();
	auto& container = pgroup->container;
	for(const auto& card : cards) {
		if(auto* pcard = card.value_or(nullptr); pcard) {
			container.insert(pcard);
		}
	}
	interpreter::pushobject(L, pgroup);
	return 1;
}
LUA_FUNCTION_ALIAS(FromCards);
LUA_FUNCTION(Clone) {
	auto newgroup = pduel->new_group(self);
	interpreter::pushobject(L, newgroup);
	return 1;
}
LUA_FUNCTION(DeleteGroup) {
	return 0;
}
LUA_FUNCTION(KeepAlive) {
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
LUA_FUNCTION(AddCard, std::variant<card*, group*> card_or_group) {
	assert_readonly_group(L, self);
	self->is_iterator_dirty = true;
	if(const auto* const* ppgroup = std::get_if<group*>(&card_or_group); ppgroup) {
		auto* pgroup = *ppgroup;
		self->container.insert(pgroup->container.begin(), pgroup->container.end());
	} else
		self->container.insert(*std::get_if<card*>(&card_or_group));
	interpreter::pushobject(L, self);
	return 1;
}
LUA_FUNCTION_ALIAS(Merge);
LUA_FUNCTION(RemoveCard, std::variant<card*, group*> card_or_group) {
	assert_readonly_group(L, self);
	self->is_iterator_dirty = true;
	if(const auto* const* ppgroup = std::get_if<group*>(&card_or_group); ppgroup) {
		auto* pgroup = *ppgroup;
		if(self == pgroup)
			lua_error(L, "Attempting to remove a group from itself");
		for(auto& _pcard : pgroup->container)
			self->container.erase(_pcard);
	} else
		self->container.erase(*std::get_if<card*>(&card_or_group));
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
LUA_FUNCTION(TakeatPos, size_t pos) {
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
LUA_FUNCTION(Filter, Function filter, std::variant<card*, group*, Nil> excluded_card_or_group, VariadicArgs extraargs) {
	card_set cset(self->container);
	if(auto* ppgroup = std::get_if<group*>(&excluded_card_or_group); ppgroup) {
		for(auto& pcard : (*ppgroup)->container)
			cset.erase(pcard);
	} else if(auto* ppcard = std::get_if<card*>(&excluded_card_or_group); ppcard) {
		cset.erase(*ppcard);
	}
	auto new_group = pduel->new_group();
	for(auto& pcard : cset) {
		if(pduel->lua->check_matching(pcard, filter, extraargs.size)) {
			new_group->container.insert(pcard);
		}
	}
	interpreter::pushobject(L, new_group);
	return 1;
}
LUA_FUNCTION(Match, Function filter, std::variant<card*, group*, Nil> excluded_card_or_group, VariadicArgs extraargs) {
	assert_readonly_group(L, self);
	self->is_iterator_dirty = true;
	auto call_filter = [&](card* pcard) {
		return pduel->lua->check_matching(pcard, filter, extraargs.size);
	};
	auto& cset = self->container;
	if(auto* ppcard = std::get_if<card*>(&excluded_card_or_group); ppcard) {
		auto* pexception = *ppcard;
		for(auto cit = cset.begin(), cend = cset.end(); cit != cend; ) {
			auto rm = cit++;
			auto* pcard = *rm;
			if(pcard == pexception || !call_filter(pcard))
				cset.erase(rm);
		}
	} else if(auto* ppgroup = std::get_if<group*>(&excluded_card_or_group); ppgroup) {
		auto* pexgroup = *ppgroup;
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
			if(should_remove(pcard) || !call_filter(pcard))
				cset.erase(rm);
		}
	} else {
		for(auto cit = cset.begin(), cend = cset.end(); cit != cend; ) {
			auto rm = cit++;
			auto* pcard = *rm;
			if(!call_filter(pcard))
				cset.erase(rm);
		}
	}
	interpreter::pushobject(L, self);
	return 1;
}
LUA_FUNCTION(FilterCount, Function filter, std::variant<card*, group*, Nil> excluded_card_or_group, VariadicArgs extraargs) {
	card_set cset(self->container);
	if(auto* ppgroup = std::get_if<group*>(&excluded_card_or_group); ppgroup) {
		for(auto& pcard : (*ppgroup)->container)
			cset.erase(pcard);
	} else if(auto* ppcard = std::get_if<card*>(&excluded_card_or_group); ppcard) {
		cset.erase(*ppcard);
	}
	lua_pushinteger(L, std::count_if(cset.begin(), cset.end(),
									 [&](auto* pcard) {
										 return pduel->lua->check_matching(pcard, filter, extraargs.size);
									 }));
	return 1;
}
LUA_FUNCTION(FilterSelect) {
	check_action_permission(L);
	check_param_count(L, 6);
	auto playerid = get_lua<playerid_t>(L, 2);
	auto filter = get_lua<Function>(L, 3);
	auto min = get_lua<uint16_t>(L, 4);
	auto max = get_lua<uint16_t>(L, 5);
	int lastarg = 6;
	bool cancelable = false;
	std::pair<card*, group*> pexception_pair;
	if(auto cancelable_or_pexception = get_lua<std::variant<card*, group*, bool, Nil>>(L, lastarg);
	   std::holds_alternative<bool>(cancelable_or_pexception)) {
		++lastarg;
		cancelable = *std::get_if<bool>(&cancelable_or_pexception);
		pexception_pair = expand_to_card_or_group(get_lua<std::variant<card*, group*, Nil>>(L, lastarg));
	} else {
		pexception_pair = expand_to_card_or_group(cancelable_or_pexception);
	}
	card_set cset(self->container);
	if(auto [pexception, pexgroup] = pexception_pair; pexception) {
		cset.erase(pexception);
	} else if(pexgroup) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	uint32_t extraargs = lua_gettop(L) - lastarg;
	pduel->game_field->core.select_cards.clear();
	for(auto& pcard : cset) {
		if(pduel->lua->check_matching(pcard, filter, extraargs))
			pduel->game_field->core.select_cards.push_back(pcard);
	}
	pduel->game_field->emplace_process<Processors::SelectCard>(playerid, cancelable, min, max);
	return push_return_cards(L, cancelable);
}
LUA_FUNCTION(Select, playerid_t playerid, uint16_t min, uint16_t max, std::variant<card*, group*, bool, Nil> cancelable_or_pexception) {
	check_action_permission(L);
	card_set cset(self->container);
	bool cancelable = false;
	if(auto [pexception, pexgroup] = expand_to_card_or_group(cancelable_or_pexception); pexception) {
		cset.erase(pexception);
	} else if(pexgroup) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	} else if(std::holds_alternative<bool>(cancelable_or_pexception)) {
		cancelable = *std::get_if<bool>(&cancelable_or_pexception);
	}
	pduel->game_field->core.select_cards.assign(cset.begin(), cset.end());
	pduel->game_field->emplace_process<Processors::SelectCard>(playerid, cancelable, min, max);
	return push_return_cards(L, cancelable);
}
LUA_FUNCTION(Select, playerid_t playerid, uint16_t min, uint16_t max, std::variant<bool> cancelable, std::variant<card*, group*> pexception_card_or_group) {
	check_action_permission(L);
	card_set cset(self->container);
	if(auto [pexception, pexgroup] = expand_to_card_or_group(pexception_card_or_group); pexception) {
		cset.erase(pexception);
	} else if(pexgroup) {
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	pduel->game_field->core.select_cards.assign(cset.begin(), cset.end());
	pduel->game_field->emplace_process<Processors::SelectCard>(playerid, *std::get_if<bool>(&cancelable), min, max);
	return push_return_cards(L, *std::get_if<bool>(&cancelable));
}
LUA_FUNCTION(SelectUnselect, std::optional<group*> selected_group, playerid_t playerid, std::optional<bool> finishable,
			 std::optional<bool> cancelable, std::optional<uint16_t> min, std::optional<uint16_t> max) {
	check_action_permission(L);
	pduel->game_field->core.unselect_cards.clear();
	if(selected_group.has_value()) {
		auto* pgroup = *selected_group;
		auto first1 = self->container.begin();
		auto last1 = self->container.end();
		auto first2 = pgroup->container.begin();
		auto last2 = pgroup->container.end();
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
		pduel->game_field->core.unselect_cards.assign(pgroup->container.begin(), pgroup->container.end());
	}
	pduel->game_field->core.select_cards.assign(self->container.begin(), self->container.end());
	auto min_real = min.value_or(1);
	auto max_real = max.value_or(1);
	if(min_real > max_real)
		min_real = max_real;
	pduel->game_field->emplace_process<Processors::SelectUnselectCard>(playerid, cancelable.value_or(false), min_real, max_real, finishable.value_or(false));
	return yieldk({
		if(pduel->game_field->return_cards.canceled)
			lua_pushnil(L);
		else
			interpreter::pushobject(L, pduel->game_field->return_cards.list[0]);
		return 1;
	});
}
LUA_FUNCTION(RandomSelect, playerid_t playerid, uint32_t count) {
	auto newgroup = pduel->new_group();
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
LUA_FUNCTION(IsExists, Function filter, uint16_t count, std::variant<card*, group*, Nil> excluded_card_or_group, VariadicArgs extraargs) {
	card_set cset(self->container);
	if(auto* ppcard = std::get_if<card*>(&excluded_card_or_group); ppcard) {
		cset.erase(*ppcard);
	} else if(auto* ppgroup = std::get_if<group*>(&excluded_card_or_group); ppgroup) {
		for(auto& pcard : (*ppgroup)->container)
			cset.erase(pcard);
	}
	uint32_t fcount = 0;
	for(auto& pcard : cset) {
		if(pduel->lua->check_matching(pcard, filter, extraargs.size)) {
			++fcount;
			if(fcount >= count)
				break;
		}
	}
	lua_pushboolean(L, fcount >= count);
	return 1;
}
LUA_FUNCTION(CheckWithSumEqual, Function filter, uint32_t value_to_reach, int32_t min, int32_t max, VariadicArgs extraargs) {
	if(min < 0)
		min = 0;
	if(max < min)
		max = min;
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = static_cast<int32_t>(cv.size());
	const auto beginit = pduel->game_field->core.must_select_cards.begin();
	const auto endit = pduel->game_field->core.must_select_cards.end();
	for(auto& pcard : self->container) {
		if(std::find(beginit, endit, pcard) == endit)
			cv.push_back(pcard);
	}
	pduel->game_field->core.must_select_cards.clear();
	for(auto& pcard : cv) {
		if(pcard->sum_param = pduel->lua->get_operation_value(pcard, filter, extraargs.size); pcard->sum_param == 0) {
			lua_error(L, "Group contains a card for which the value function returned 0.");
		}
	}
	int32_t should_continue = TRUE;
	lua_pushboolean(L, field::check_with_sum_limit_m(cv, value_to_reach, 0, min, max, mcount, &should_continue));
	lua_pushboolean(L, should_continue);
	return 2;
}
LUA_FUNCTION(SelectWithSumEqual, playerid_t playerid, Function filter, uint32_t value_to_reach, int32_t min, int32_t max, VariadicArgs extraargs) {
	check_action_permission(L);
	if(min < 0)
		min = 0;
	if(max < min)
		max = min;
	pduel->game_field->core.select_cards.assign(self->container.begin(), self->container.end());
	for(auto& pcard : pduel->game_field->core.must_select_cards) {
		auto it = std::remove(pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end(), pcard);
		pduel->game_field->core.select_cards.erase(it, pduel->game_field->core.select_cards.end());
	}
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = static_cast<int32_t>(cv.size());
	cv.insert(cv.end(), pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end());
	for(auto& pcard : cv) {
		if(pcard->sum_param = pduel->lua->get_operation_value(pcard, filter, extraargs.size); pcard->sum_param == 0) {
			lua_error(L, "Group contains a card for which the value function returned 0.");
		}
	}
	if(!field::check_with_sum_limit_m(cv, value_to_reach, 0, min, max, mcount, nullptr)) {
		pduel->game_field->core.must_select_cards.clear();
		auto empty_group = pduel->new_group();
		interpreter::pushobject(L, empty_group);
		return 1;
	}
	pduel->game_field->emplace_process<Processors::SelectSum>(playerid, value_to_reach, min, max);
	return yieldk({
		auto pgroup = pduel->new_group(pduel->game_field->return_cards.list);
		pduel->game_field->core.must_select_cards.clear();
		interpreter::pushobject(L, pgroup);
		return 1;
	});
}
LUA_FUNCTION(CheckWithSumGreater, Function filter, uint32_t value_to_reach, VariadicArgs extraargs) {
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = static_cast<int32_t>(cv.size());
	const auto beginit = pduel->game_field->core.must_select_cards.begin();
	const auto endit = pduel->game_field->core.must_select_cards.end();
	for(auto& pcard : self->container) {
		if(std::find(beginit, endit, pcard) == endit)
			cv.push_back(pcard);
	}
	pduel->game_field->core.must_select_cards.clear();
	for(auto& pcard : cv) {
		if(pcard->sum_param = pduel->lua->get_operation_value(pcard, filter, extraargs.size); pcard->sum_param == 0) {
			lua_error(L, "Group contains a card for which the value function returned 0.");
		}
	}
	int32_t should_continue = TRUE;
	lua_pushboolean(L, field::check_with_sum_greater_limit_m(cv, value_to_reach, 0, 0xffff, mcount, &should_continue));
	lua_pushboolean(L, should_continue);
	return 2;
}
LUA_FUNCTION(SelectWithSumGreater, playerid_t playerid, Function filter, uint32_t value_to_reach, VariadicArgs extraargs) {
	check_action_permission(L);
	pduel->game_field->core.select_cards.assign(self->container.begin(), self->container.end());
	for(auto& pcard : pduel->game_field->core.must_select_cards) {
		auto it = std::remove(pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end(), pcard);
		pduel->game_field->core.select_cards.erase(it, pduel->game_field->core.select_cards.end());
	}
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = static_cast<int32_t>(cv.size());
	cv.insert(cv.end(), pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end());
	for(auto& pcard : cv) {
		if(pcard->sum_param = pduel->lua->get_operation_value(pcard, filter, extraargs.size); pcard->sum_param == 0) {
			lua_error(L, "Group contains a card for which the value function returned 0.");
		}
	}
	if(!field::check_with_sum_greater_limit_m(cv, value_to_reach, 0, 0xffff, mcount, nullptr)) {
		pduel->game_field->core.must_select_cards.clear();
		auto empty_group = pduel->new_group();
		interpreter::pushobject(L, empty_group);
		return 1;
	}
	pduel->game_field->emplace_process<Processors::SelectSum>(playerid, value_to_reach, 0, 0);
	return yieldk({
		auto pgroup = pduel->new_group(pduel->game_field->return_cards.list);
		pduel->game_field->core.must_select_cards.clear();
		interpreter::pushobject(L, pgroup);
		return 1;
	});
}
LUA_FUNCTION(GetMinGroup, Function filter, VariadicArgs extraargs) {
	if(self->container.size() == 0)
		return 0;
	auto newgroup = pduel->new_group();
	auto cit = self->container.begin();
	auto min = pduel->lua->get_operation_value(*cit, filter, extraargs.size);
	newgroup->container.insert(*cit);
	++cit;
	for(; cit != self->container.end(); ++cit) {
		auto res = pduel->lua->get_operation_value(*cit, filter, extraargs.size);
		if(res == min)
			newgroup->container.insert(*cit);
		else if(res < min) {
			newgroup->container.clear();
			newgroup->container.insert(*cit);
			min = res;
		}
	}
	interpreter::pushobject(L, newgroup);
	lua_pushinteger(L, min);
	return 2;
}
LUA_FUNCTION(GetMaxGroup, Function filter, VariadicArgs extraargs) {
	if(self->container.size() == 0)
		return 0;
	auto newgroup = pduel->new_group();
	auto cit = self->container.begin();
	auto max = pduel->lua->get_operation_value(*cit, filter, extraargs.size);
	newgroup->container.insert(*cit);
	++cit;
	for(; cit != self->container.end(); ++cit) {
		auto res = pduel->lua->get_operation_value(*cit, filter, extraargs.size);
		if(res == max)
			newgroup->container.insert(*cit);
		else if(res > max) {
			newgroup->container.clear();
			newgroup->container.insert(*cit);
			max = res;
		}
	}
	interpreter::pushobject(L, newgroup);
	lua_pushinteger(L, max);
	return 2;
}
LUA_FUNCTION(GetSum, Function filter, VariadicArgs extraargs) {
	int64_t sum = 0;
	for(auto& pcard : self->container) {
		sum += pduel->lua->get_operation_value(pcard, filter, extraargs.size);
	}
	lua_pushinteger(L, sum);
	return 1;
}
LUA_FUNCTION(GetBitwiseAnd, Function filter, VariadicArgs extraargs) {
	uint64_t total = 0;
	for(auto& pcard : self->container) {
		total &= static_cast<uint64_t>(pduel->lua->get_operation_value(pcard, filter, extraargs.size));
		if(total == 0) {
			break;
		}
	}
	lua_pushinteger(L, total);
	return 1;
}
LUA_FUNCTION(GetBitwiseOr, Function filter, VariadicArgs extraargs) {
	uint64_t total = 0;
	for(auto& pcard : self->container) {
		total |= static_cast<uint64_t>(pduel->lua->get_operation_value(pcard, filter, extraargs.size));
	}
	lua_pushinteger(L, total);
	return 1;
}
LUA_FUNCTION(GetClass, Function filter, VariadicArgs extraargs) {
	std::set<int64_t> values;
	for(auto& pcard : self->container) {
		values.insert(pduel->lua->get_operation_value(pcard, filter, extraargs.size));
	}
	lua_createtable(L, static_cast<int>(values.size()), 0);
	int i = 1;
	for(auto& val : values) {
		lua_pushinteger(L, i++);
		lua_pushinteger(L, val);
		lua_settable(L, -3);
	}
	return 1;
}
LUA_FUNCTION(GetClassCount, Function filter, VariadicArgs extraargs) {
	std::set<int64_t> er;
	for(auto& pcard : self->container) {
		er.insert(pduel->lua->get_operation_value(pcard, filter, extraargs.size));
	}
	lua_pushinteger(L, er.size());
	return 1;
}
LUA_FUNCTION(Remove, Function filter, std::variant<card*, group*, Nil> excluded_card_or_group, VariadicArgs extraargs) {
	assert_readonly_group(L, self);
	self->is_iterator_dirty = true;
	auto call_filter = [&](card* pcard) {
		return pduel->lua->check_matching(pcard, filter, extraargs.size);
	};
	auto& cset = self->container;
	if(auto* ppcard = std::get_if<card*>(&excluded_card_or_group); ppcard) {
		auto* pexception = *ppcard;
		for(auto cit = cset.begin(), cend = cset.end(); cit != cend; ) {
			auto rm = cit++;
			auto* pcard = *rm;
			if(pcard != pexception && call_filter(pcard))
				cset.erase(rm);
		}
	} else if(auto* ppgroup = std::get_if<group*>(&excluded_card_or_group); ppgroup) {
		auto* pexgroup = *ppgroup;
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
			if(!should_keep(pcard) && call_filter(pcard))
				cset.erase(rm);
		}
	} else {
		for(auto cit = cset.begin(), cend = cset.end(); cit != cend; ) {
			auto rm = cit++;
			auto* pcard = *rm;
			if(call_filter(pcard))
				cset.erase(rm);
		}
	}
	interpreter::pushobject(L, self);
	return 1;
}
inline std::tuple<group*, group*, card*> get_binary_op_group_card_parameters(const std::variant<card*, group*>& lhs, const std::variant<card*, group*>& rhs) {
	auto* rhs_ptr = &rhs;
	auto* ppgroup = std::get_if<group*>(&lhs);
	if(!ppgroup){
		rhs_ptr = &lhs;
		ppgroup = std::get_if<group*>(&rhs);
	}
	if(!ppgroup) {
		return {};
	}
	if(auto* ppgroup2 = std::get_if<group*>(rhs_ptr); ppgroup2) {
		return { *ppgroup, *ppgroup2, nullptr };
	} else {
		return { *ppgroup, nullptr, *std::get_if<card*>(rhs_ptr) };
	}
}
LUA_STATIC_FUNCTION(__band, std::variant<card*, group*> lhs, std::variant<card*, group*> rhs) {
	auto [pgroup1, pgroup2, pcard] = get_binary_op_group_card_parameters(lhs, rhs);
	if(!pgroup1)
		lua_error(L, "At least 1 parameter should be \"Group\".");
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
LUA_STATIC_FUNCTION(__add, std::variant<card*, group*> lhs, std::variant<card*, group*> rhs) {
	auto [pgroup1, pgroup2, pcard] = get_binary_op_group_card_parameters(lhs, rhs);
	if(!pgroup1)
		lua_error(L, "At least 1 parameter should be \"Group\".");
	auto newgroup = pduel->new_group(pgroup1);
	if(pcard) {
		newgroup->container.insert(pcard);
	} else {
		newgroup->container.insert(pgroup2->container.begin(), pgroup2->container.end());
	}
	interpreter::pushobject(L, newgroup);
	return 1;
}
LUA_FUNCTION(__sub, std::variant<card*, group*> card_or_group) {
	auto newgroup = pduel->new_group(self);
	if(const auto* const* ppgroup = std::get_if<group*>(&card_or_group); ppgroup) {
		auto* pgroup = *ppgroup;
		for(auto& pcard : pgroup->container)
			newgroup->container.erase(pcard);
	} else
		newgroup->container.erase(*std::get_if<card*>(&card_or_group));
	interpreter::pushobject(L, newgroup);
	return 1;
}
LUA_FUNCTION(__len) {
	lua_pushinteger(L, self->container.size());
	return 1;
}
LUA_FUNCTION(__eq, group* other) {
	lua_pushboolean(L, self->container.size() == other->container.size());
	return 1;
}
LUA_FUNCTION(Equal, group* other) {
	lua_pushboolean(L, self->container == other->container);
	return 1;
}
LUA_FUNCTION(__lt, group* other) {
	lua_pushboolean(L, self->container.size() < other->container.size());
	return 1;
}
LUA_FUNCTION(__le, group* other) {
	lua_pushboolean(L, self->container.size() <= other->container.size());
	return 1;
}
LUA_FUNCTION(__gc) {
	pduel->delete_group(self);
	return 1;
}
LUA_FUNCTION(IsContains, card* pcard) {
	lua_pushboolean(L, self->has_card(pcard));
	return 1;
}
LUA_FUNCTION(SearchCard, Function filter, VariadicArgs extraargs) {
	for(auto& pcard : self->container) {
		if(pduel->lua->check_matching(pcard, filter, extraargs.size)) {
			interpreter::pushobject(L, pcard);
			return 1;
		}
	}
	return 0;
}
LUA_FUNCTION(Split, Function filter, std::variant<card*, group*, Nil> excluded_card_or_group, VariadicArgs extraargs) {
	card_set cset(self->container);
	card_set notmatching;
	if(auto* ppgroup = std::get_if<group*>(&excluded_card_or_group); ppgroup) {
		for(auto& pcard : (*ppgroup)->container) {
			cset.erase(pcard);
			notmatching.insert(pcard);
		}
	} else if(auto* ppcard = std::get_if<card*>(&excluded_card_or_group); ppcard) {
		auto* pcard = *ppcard;
		cset.erase(pcard);
		notmatching.insert(pcard);
	}
	for(auto it = cset.begin(); it != cset.end();) {
		auto pcard = *it;
		if(pduel->lua->check_matching(pcard, filter, extraargs.size)) {
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
LUA_FUNCTION(Includes, group* pgroup2) {
	int res = TRUE;
	if(self->container.size() < pgroup2->container.size())
		res = FALSE;
	else if(!pgroup2->container.empty())
		res = std::includes(self->container.cbegin(), self->container.cend(), pgroup2->container.cbegin(), pgroup2->container.cend(), card_sort());
	lua_pushboolean(L, res);
	return 1;
}
LUA_FUNCTION(GetBinClassCount, Function filter, VariadicArgs extraargs) {
	uint64_t er = 0;
	for(auto& pcard : self->container)
		er |= static_cast<uint64_t>(pduel->lua->get_operation_value(pcard, filter, extraargs.size));
	lua_pushinteger(L, bit::popcnt(er));
	return 1;
}
LUA_FUNCTION_EXISTING(GetLuaRef, get_lua_ref<group>);
LUA_FUNCTION_EXISTING(FromLuaRef, from_lua_ref<group>);
LUA_FUNCTION_EXISTING(IsDeleted, is_deleted_object);
}
}

void scriptlib::push_group_lib(lua_State* L) {
	static constexpr auto grouplib = GET_LUA_FUNCTIONS_ARRAY();
	static_assert(grouplib.back().name == nullptr);
	lua_createtable(L, 0, static_cast<int>(grouplib.size() - 1));
	ensure_luaL_stack(luaL_setfuncs, L, grouplib.data(), 0);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Group");
}
