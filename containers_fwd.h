/*
 * Copyright (c) 2021-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef CONTAINERS_FWD_H
#define CONTAINERS_FWD_H

#include <cstdint>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include <set>

class group;

class card;
using card_vector = std::vector<card*>;
struct card_sort {
	bool operator()(const card* c1, const card* c2) const;
};
using card_set = std::set<card*, card_sort>;

class effect;
using effect_vector = std::vector<effect*>;
using effect_container = std::multimap<uint32_t, effect*>;
using effect_indexer = std::unordered_map<effect*, effect_container::iterator>;
using effect_set = std::vector<effect*>;
using effect_set_v = effect_set;

bool effect_sort_id(const effect* e1, const effect* e2);

struct chain;
using chain_array = std::vector<chain>;
using chain_list = std::list<chain>;

using instant_f_list = std::map<effect*, chain>;

struct tevent;
using event_list = std::list<tevent>;

#endif /* CONTAINERS_FWD_H */
