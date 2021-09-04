#ifndef CONTAINERS_FWD_H
#define CONTAINERS_FWD_H

#include <vector>
#include <cstdint>
#include <set>
#include <list>
#include <map>
#include <unordered_map>

struct card_sort {
	bool operator()(void* const& c1, void* const& c2) const;
};

class group;

class card;
using card_list = std::list<card*>;
using card_vector = std::vector<card*>;
using card_set = std::set<card*, card_sort>;

class effect;
using effect_vector = std::vector<effect*>;
using effect_container = std::multimap<uint32_t, effect*>;
using effect_indexer = std::unordered_map<effect*, effect_container::iterator>;

struct chain;
using chain_array = std::vector<chain>;
using chain_list = std::list<chain>;

using instant_f_list = std::map<effect*, chain>;

struct tevent;
using event_list = std::list<tevent>;

#endif /* CONTAINERS_FWD_H */
