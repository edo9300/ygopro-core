/*
 * card.h
 *
 *  Created on: 2010-4-8
 *      Author: Argon
 */

#ifndef CARD_H_
#define CARD_H_

#include "containers_fwd.h"
#include "common.h"
#include "lua_obj.h"
#include "effectset.h"
#include "ocgapi.h"
#include "duel.h"
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <tuple>

class card;
class duel;
class effect;
class group;
struct chain;

struct loc_info {
	uint8_t controler;
	uint8_t location;
	uint32_t sequence;
	uint32_t position;
};

struct card_state {
	uint32_t code{};
	uint32_t code2{};
	std::set<uint16_t> setcodes;
	uint32_t type{};
	uint32_t level{};
	uint32_t rank{};
	uint32_t link{};
	uint32_t link_marker{};
	uint32_t lscale{};
	uint32_t rscale{};
	uint32_t attribute{};
	uint64_t race{};
	int32_t attack{};
	int32_t defense{};
	int32_t base_attack{};
	int32_t base_defense{};
	uint8_t controler{};
	uint8_t location{};
	uint32_t sequence{};
	uint32_t position{};
	uint32_t reason{};
	bool pzone{};
	card* reason_card{};
	uint8_t reason_player{};
	effect* reason_effect{};
	template<typename T>
	static bool is_location(const T& loc_info, uint16_t loc) {
		if(loc_info.location & static_cast<uint8_t>(loc))
			return true;
		if((loc & LOCATION_EMZONE) && loc_info.location == LOCATION_MZONE && loc_info.sequence >= 5)
			return true;
		if((loc & LOCATION_MMZONE) && loc_info.location == LOCATION_MZONE && loc_info.sequence < 5)
			return true;
		if((loc & LOCATION_STZONE) && loc_info.location == LOCATION_SZONE && loc_info.sequence < 5)
			return true;
		if((loc & LOCATION_FZONE) && loc_info.location == LOCATION_SZONE && loc_info.sequence == 5)
			return true;
		if((loc & LOCATION_PZONE) && loc_info.location == LOCATION_SZONE && loc_info.pzone)
			return true;
		return false;
	}
	bool is_location(uint16_t loc) const {
		return is_location(*this, loc);
	}
	void set0xff();
};

class card : public lua_obj_helper<PARAM_TYPE_CARD> {
public:
	struct effect_relation_hash {
		inline std::size_t operator()(const std::pair<effect*, uint16_t>& v) const {
			return std::hash<uint16_t>()(v.second);
		}
	};
	using effect_container = std::multimap<uint32_t, effect*>;
	using effect_relation = std::unordered_set<std::pair<effect*, uint16_t>, effect_relation_hash>;
	using relation_map = std::unordered_map<card*, uint32_t>;
	using counter_map = std::map<uint16_t, std::array<uint16_t, 2>>;
	using effect_count = std::map<uint32_t, int32_t>;
	class attacker_map : public std::unordered_map<uint32_t, std::pair<card*, uint32_t>> {
	public:
		void addcard(card* pcard);
		uint32_t findcard(card* pcard);
	};
	struct sendto_param_t {
		void set(uint8_t p, uint8_t pos, uint8_t loc, uint8_t seq = 0) {
			playerid = p;
			position = pos;
			location = loc;
			sequence = seq;
		}
		void clear() {
			playerid = 0;
			position = 0;
			location = 0;
			sequence = 0;
		}
		uint8_t playerid{};
		uint8_t position{};
		uint8_t location{};
		uint8_t sequence{};
	};
	card_data data{};
	card_state previous{};
	card_state temp{};
	card_state current{};
	uint8_t owner{ PLAYER_NONE };
	struct summon_info {
		uint32_t type{};
		uint8_t player{};
		uint8_t location{};
		uint8_t sequence{};
		bool pzone{};
	};
	summon_info summon;
	uint32_t status{};
	uint32_t cover{};
	sendto_param_t sendto_param{};
	uint32_t release_param{};
	uint32_t sum_param{};
	uint32_t position_param{};
	uint32_t spsummon_param{};
	uint32_t to_field_param{};
	uint8_t attack_announce_count{};
	uint8_t direct_attackable{};
	uint8_t announce_count{};
	uint8_t attacked_count{};
	uint8_t attack_all_target{};
	uint8_t attack_controler{};
	uint32_t cardid{};
	uint32_t fieldid{};
	uint32_t fieldid_r{};
	uint16_t turnid{};
	uint16_t turn_counter{};
	uint8_t unique_pos[2]{};
	uint32_t unique_fieldid{};
	uint32_t unique_code{};
	uint32_t unique_location{};
	int32_t unique_function{};
	effect* unique_effect{};
	uint32_t spsummon_code{};
	uint16_t spsummon_counter[2]{};
	uint16_t spsummon_counter_rst[2]{};
	std::map<uint32_t, uint64_t> assume;
	card* equiping_target{};
	card* pre_equip_target{};
	card* overlay_target{};
	relation_map relations;
	counter_map counters;
	effect_count indestructable_effects;
	attacker_map announced_cards;
	attacker_map attacked_cards;
	attacker_map battled_cards;
	card_set equiping_cards;
	card_set material_cards;
	card_set effect_target_owner;
	card_set effect_target_cards;
	card_vector xyz_materials;
	effect_container single_effect;
	effect_container field_effect;
	effect_container equip_effect;
	effect_container target_effect;
	effect_container xmaterial_effect;
	effect_indexer indexer;
	effect_relation relate_effect;
	effect_set_v immune_effect;

	explicit card(duel* pd);
	~card() = default;
	static bool card_operation_sort(card* c1, card* c2);
	static bool match_setcode(uint16_t set_code, uint16_t to_match) {
		return (set_code & 0xfffu) == (to_match & 0xfffu) && (set_code & to_match) == set_code;
	}
	bool is_extra_deck_monster() const { return !!(data.type & (TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ | TYPE_LINK)) && !!(data.type & TYPE_MONSTER); }

	void get_infos(int32_t query_flag);
	int32_t is_related_to_chains();
	loc_info get_info_location();
	uint32_t second_code(uint32_t code);
	uint32_t get_code();
	uint32_t get_another_code();
	void get_summon_code(std::set<uint32_t>& codes, card* scard = 0, uint64_t sumtype = 0, uint8_t playerid = 2);
	int32_t is_set_card(uint16_t set_code);
	int32_t is_origin_set_card(uint16_t set_code);
	int32_t is_pre_set_card(uint16_t set_code);
	int32_t is_summon_set_card(uint16_t set_code, card* scard = 0, uint64_t sumtype = 0, uint8_t playerid = 2);
	void get_set_card(std::set<uint16_t>& setcodes);
	const std::set<uint16_t>& get_origin_set_card() const { return data.setcodes; }
	void get_pre_set_card(std::set<uint16_t>& setcodes);
	void get_summon_set_card(std::set<uint16_t>& setcodes, card* scard = 0, uint64_t sumtype = 0, uint8_t playerid = 2);
	uint32_t get_type(card* scard = 0, uint64_t sumtype = 0, uint8_t playerid = 2);
	int32_t get_base_attack();
	int32_t get_attack();
	int32_t get_base_defense();
	int32_t get_defense();
	uint32_t get_level();
	uint32_t get_rank();
	uint32_t get_link();
	uint32_t get_synchro_level(card* pcard);
	uint32_t get_ritual_level(card* pcard);
	uint32_t check_xyz_level(card* pcard, uint32_t lv);
	uint32_t get_attribute(card* scard = 0, uint64_t sumtype = 0, uint8_t playerid = 2);
	uint64_t get_race(card* scard = 0, uint64_t sumtype = 0, uint8_t playerid = 2);
	uint32_t get_lscale();
	uint32_t get_rscale();
	uint32_t get_link_marker();
	int32_t is_link_marker(uint32_t dir);
	int32_t is_link_marker(uint32_t dir, uint32_t marker) const {
		return (int32_t)(marker & dir);
	}
	uint32_t get_linked_zone(bool free = false);
	void get_linked_cards(card_set* cset, uint32_t zones = 0);
	uint32_t get_mutual_linked_zone();
	void get_mutual_linked_cards(card_set* cset);
	int32_t is_link_state();
	int32_t is_mutual_linked(card* pcard, uint32_t zones1 = 0, uint32_t zones2 = 0);
	int32_t is_extra_link_state();
	int32_t is_position(int32_t pos) const {
		return (int32_t)(current.position & pos);
	}
	void set_status(uint32_t status_to_toggle, int32_t enabled) {
		if(enabled)
			status |= status_to_toggle;
		else
			status &= ~status_to_toggle;
	}
	// match at least 1 status
	int32_t get_status(uint32_t status_to_check) const {
		return (int32_t)(status & status_to_check);
	}
	// match all statuses
	int32_t is_status(uint32_t status_to_check) const {
		return (status & status_to_check) == status_to_check;
	}
	uint32_t get_column_zone(int32_t loc1, int32_t left, int32_t right);
	void get_column_cards(card_set* cset, int32_t left, int32_t right);
	int32_t is_all_column();

	void equip(card* target, uint32_t send_msg = TRUE);
	void unequip();
	int32_t get_union_count();
	int32_t get_old_union_count();
	void xyz_add(card* mat);
	void xyz_remove(card* mat);
	void apply_field_effect();
	void cancel_field_effect();
	void enable_field_effect(bool enabled);
	int32_t add_effect(effect* peffect);
	void remove_effect(effect* peffect);
	void remove_effect(effect* peffect, effect_container::iterator it);
	int32_t copy_effect(uint32_t code, uint32_t reset, uint32_t count);
	int32_t replace_effect(uint32_t code, uint32_t reset, uint32_t count, bool recreating = false);
	void reset(uint32_t id, uint32_t reset_type);
	void reset_effect_count();
	void refresh_disable_status();
	std::tuple<uint8_t, effect*> refresh_control_status();

	void count_turn(uint16_t ct);
	void create_relation(card* target, uint32_t reset);
	int32_t is_has_relation(card* target) const;
	void release_relation(card* target);
	void create_relation(const chain& ch);
	int32_t is_has_relation(const chain& ch) const;
	void release_relation(const chain& ch);
	void clear_relate_effect();
	void create_relation(effect* peffect);
	int32_t is_has_relation(effect* peffect) const;
	void release_relation(effect* peffect);
	int32_t leave_field_redirect(uint32_t reason);
	int32_t destination_redirect(uint8_t destination, uint32_t reason);
	int32_t add_counter(uint8_t playerid, uint16_t countertype, uint16_t count, uint8_t singly);
	int32_t remove_counter(uint16_t countertype, uint16_t count);
	int32_t is_can_add_counter(uint8_t playerid, uint16_t countertype, uint16_t count, uint8_t singly, uint32_t loc);
	int32_t get_counter(uint16_t countertype);
	void set_material(card_set* materials);
	void add_card_target(card* pcard);
	void cancel_card_target(card* pcard);
	void clear_card_target();

	void filter_effect(int32_t code, effect_set* eset, uint8_t sort = TRUE);
	void filter_single_effect(int32_t code, effect_set* eset, uint8_t sort = TRUE);
	void filter_single_continuous_effect(int32_t code, effect_set* eset, uint8_t sort = TRUE);
	void filter_immune_effect();
	void filter_disable_related_cards();
	int32_t filter_summon_procedure(uint8_t playerid, effect_set* eset, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone);
	int32_t check_summon_procedure(effect* peffect, uint8_t playerid, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone);
	int32_t filter_set_procedure(uint8_t playerid, effect_set* eset, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone);
	int32_t check_set_procedure(effect* peffect, uint8_t playerid, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone);
	void filter_spsummon_procedure(uint8_t playerid, effect_set* eset, uint32_t summon_type);
	void filter_spsummon_procedure_g(uint8_t playerid, effect_set* eset);
	effect* is_affected_by_effect(int32_t code);
	effect* is_affected_by_effect(int32_t code, card* target);
	void get_card_effect(uint32_t code, effect_set* eset);
	int32_t fusion_check(group* fusion_m, group* cg, uint32_t chkf);
	void fusion_filter_valid(group* fusion_m, group* cg, uint32_t chkf, effect_set* eset);
	int32_t check_fusion_substitute(card* fcard);
	int32_t is_not_tuner(card* scard, uint8_t playerid);

	int32_t check_unique_code(card* pcard);
	void get_unique_target(card_set* cset, int32_t controler, card* icard = 0);
	int32_t check_cost_condition(int32_t ecode, int32_t playerid);
	int32_t check_cost_condition(int32_t ecode, int32_t playerid, int32_t sumtype);
	int32_t is_summonable_card();
	int32_t is_fusion_summonable_card(uint32_t summon_type);
	int32_t is_spsummonable(effect* peffect);
	int32_t is_summonable(effect* peffect, uint8_t min_tribute, uint32_t zone = 0x1f, uint32_t releasable = 0xff00ff, effect* exeffect = nullptr);
	int32_t is_can_be_summoned(uint8_t playerid, uint8_t ingore_count, effect* peffect, uint8_t min_tribute, uint32_t zone = 0x1f);
	int32_t get_summon_tribute_count();
	int32_t get_set_tribute_count();
	int32_t is_can_be_flip_summoned(uint8_t playerid);
	int32_t is_special_summonable(uint8_t playerid, uint32_t summon_type);
	int32_t is_can_be_special_summoned(effect* reason_effect, uint32_t sumtype, uint8_t sumpos, uint8_t sumplayer, uint8_t toplayer, uint8_t nocheck, uint8_t nolimit, uint32_t zone);
	int32_t is_setable_mzone(uint8_t playerid, uint8_t ignore_count, effect* peffect, uint8_t min_tribute, uint32_t zone = 0x1f);
	int32_t is_setable_szone(uint8_t playerid, uint8_t ignore_fd = 0);
	int32_t is_affect_by_effect(effect* peffect);
	int32_t is_can_be_disabled_by_effect(effect* reason_effect, bool is_monster_effect);
	int32_t is_destructable();
	int32_t is_destructable_by_battle(card* pcard);
	effect* check_indestructable_by_effect(effect* peffect, uint8_t playerid);
	int32_t is_destructable_by_effect(effect* peffect, uint8_t playerid);
	int32_t is_removeable(uint8_t playerid, int32_t pos = 0x5/*POS_FACEUP*/, uint32_t reason = 0x40/*REASON_EFFECT*/);
	int32_t is_removeable_as_cost(uint8_t playerid, int32_t pos = 0x5/*POS_FACEUP*/);
	int32_t is_releasable_by_summon(uint8_t playerid, card* pcard);
	int32_t is_releasable_by_nonsummon(uint8_t playerid);
	int32_t is_releasable_by_effect(uint8_t playerid, effect* peffect);
	int32_t is_capable_send_to_grave(uint8_t playerid);
	int32_t is_capable_send_to_hand(uint8_t playerid);
	int32_t is_capable_send_to_deck(uint8_t playerid);
	int32_t is_capable_send_to_extra(uint8_t playerid);
	int32_t is_capable_cost_to_grave(uint8_t playerid);
	int32_t is_capable_cost_to_hand(uint8_t playerid);
	int32_t is_capable_cost_to_deck(uint8_t playerid);
	int32_t is_capable_cost_to_extra(uint8_t playerid);
	int32_t is_capable_attack();
	int32_t is_capable_attack_announce(uint8_t playerid);
	int32_t is_capable_change_position(uint8_t playerid);
	int32_t is_capable_change_position_by_effect(uint8_t playerid);
	int32_t is_capable_turn_set(uint8_t playerid);
	int32_t is_capable_change_control();
	int32_t is_control_can_be_changed(int32_t ignore_mzone, uint32_t zone);
	int32_t is_capable_be_battle_target(card* pcard);
	int32_t is_capable_be_effect_target(effect* peffect, uint8_t playerid);
	int32_t is_capable_overlay(uint8_t playerid);
	int32_t is_can_be_fusion_material(card* fcard, uint64_t summon_type, uint8_t playerid);
	int32_t is_can_be_synchro_material(card* scard, uint8_t playerid, card* tuner = 0);
	int32_t is_can_be_ritual_material(card* scard, uint8_t playerid);
	int32_t is_can_be_xyz_material(card* scard, uint8_t playerid, uint32_t reason);
	int32_t is_can_be_link_material(card* scard, uint8_t playerid);
	int32_t is_can_be_material(card* scard, uint64_t sumtype, uint8_t playerid);
	bool recreate(uint32_t code);
};

//Summon Type
#define SUMMON_TYPE_NORMAL   0x10000000
#define SUMMON_TYPE_ADVANCE  0x11000000
#define SUMMON_TYPE_GEMINI   0x12000000
#define SUMMON_TYPE_FLIP     0x20000000
#define SUMMON_TYPE_SPECIAL  0x40000000
#define SUMMON_TYPE_FUSION   0x43000000
#define SUMMON_TYPE_RITUAL   0x45000000
#define SUMMON_TYPE_SYNCHRO  0x46000000
#define SUMMON_TYPE_XYZ      0x49000000
#define SUMMON_TYPE_PENDULUM 0x4a000000
#define SUMMON_TYPE_LINK     0x4c000000
//Counter
#define COUNTER_WITHOUT_PERMIT 0x1000
#define COUNTER_NEED_ENABLE    0x2000

#define ASSUME_CODE       1
#define ASSUME_TYPE       2
#define ASSUME_LEVEL      3
#define ASSUME_RANK       4
#define ASSUME_ATTRIBUTE  5
#define ASSUME_RACE       6
#define ASSUME_ATTACK     7
#define ASSUME_DEFENSE    8
#define ASSUME_LINK       9
#define ASSUME_LINKMARKER 10

//double-name cards
#define CARD_MARINE_DOLPHIN 78734254
#define CARD_TWINKLE_MOSS   13857930

#endif /* CARD_H_ */

