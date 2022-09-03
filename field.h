/*
 * field.h
 *
 *  Created on: 2010-5-8
 *      Author: Argon
 */

#ifndef FIELD_H_
#define FIELD_H_

#include "containers_fwd.h"
#include "common.h"
#include "card.h"
#include "progressivebuffer.h"
#include "effectset.h"
#include <vector>
#include <set>
#include <map>
#include <list>
#include <array>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <cmath>

class duel;
class group;
class effect;

struct tevent {
	card* trigger_card;
	group* event_cards;
	effect* reason_effect;
	uint32_t event_code;
	uint32_t event_value;
	uint32_t reason;
	uint8_t event_player;
	uint8_t reason_player;
	uint32_t global_id;
	bool operator< (const tevent& v) const;
};
struct optarget {
	group* op_cards;
	uint8_t op_count;
	uint8_t op_player;
	int32_t op_param;
};
struct chain {
	using opmap = std::unordered_map<uint32_t, optarget>;
	uint16_t chain_id;
	uint8_t chain_count;
	uint8_t triggering_player;
	uint8_t triggering_controler;
	uint16_t triggering_location;
	uint32_t triggering_sequence;
	uint8_t triggering_position;
	card_state triggering_state;
	effect* triggering_effect;
	group* target_cards;
	int32_t replace_op;
	uint8_t target_player;
	int32_t target_param;
	effect* disable_reason;
	uint8_t disable_player;
	tevent evt;
	opmap opinfos;
	opmap possibleopinfos;
	uint32_t flag;
	uint32_t event_id;
	bool was_just_sent{ false };
	using applied_chain_counter_t = std::vector<uint32_t>;
	applied_chain_counter_t* applied_chain_counters;
	static bool chain_operation_sort(const chain& c1, const chain& c2);
	void set_triggering_state(card* pcard);
};

struct player_info {
	int32_t lp;
	int32_t start_lp;
	int32_t start_count;
	int32_t draw_count;
	uint32_t used_location{ 0 };
	uint32_t disabled_location{ 0 };
	uint32_t extra_p_count{ 0 };
	uint32_t exchanges{ 0 };
	uint32_t tag_index{ 0 };
	bool recharge{ false };
	card_vector list_mzone;
	card_vector list_szone;
	card_vector list_main;
	card_vector list_grave;
	card_vector list_hand;
	card_vector list_remove;
	card_vector list_extra;
	std::vector<card_vector> extra_lists_main;
	std::vector<card_vector> extra_lists_hand;
	std::vector<card_vector> extra_lists_extra;
	std::vector<uint32_t> extra_extra_p_count;
	player_info(const OCG_Player& team) :
		lp(team.startingLP), start_lp(team.startingLP),
		start_count(team.startingDrawCount), draw_count(team.drawCountPerTurn) {
		list_mzone.resize(7, nullptr);
		list_szone.resize(8, nullptr);
		list_main.reserve(45);
		list_hand.reserve(10);
		list_grave.reserve(30);
		list_remove.reserve(30);
		list_extra.reserve(15);
	}
};
struct field_effect {
	using oath_effects = std::unordered_map<effect*, effect*>;
	using effect_collection = std::unordered_set<effect*>;
	using gain_effects = std::unordered_map<card*, effect*>;
	struct grant_effect_container {
		struct effect_sort_by_ref {
			bool operator()(effect* e1, effect* e2) const;
		};
		std::unordered_map<effect*, gain_effects> unsorted;
		std::map<effect*, gain_effects*, effect_sort_by_ref> sorted;
		using key_iterator = std::unordered_map<effect*, gain_effects>::iterator;
		template<typename... Args>
		void emplace(Args&&... args) {
			const auto ret = unsorted.emplace(std::forward<Args>(args)...);
			if(ret.second) {
				const auto elem = ret.first;
				sorted.emplace(elem->first, &elem->second);
			}
		}
		template<typename... Args>
		key_iterator find(Args&&... args) {
			return unsorted.find(std::forward<Args>(args)...);
		}
		void erase(const key_iterator& eit) {
			const auto peff = eit->first;
			sorted.erase(peff);
			unsorted.erase(eit);
		}
	};

	effect_container aura_effect;
	effect_container ignition_effect;
	effect_container activate_effect;
	effect_container trigger_o_effect;
	effect_container trigger_f_effect;
	effect_container quick_o_effect;
	effect_container quick_f_effect;
	effect_container continuous_effect;
	effect_indexer indexer;
	oath_effects oath;
	effect_collection pheff;
	effect_collection cheff;
	effect_collection rechargeable;
	effect_collection spsummon_count_eff;

	std::unordered_set<card*> disable_check_set;

	grant_effect_container grant_effect;
};
struct field_info {
	uint32_t event_id{ 1 };
	uint32_t field_id{ 1 };
	uint16_t copy_id{ 1 };
	int16_t turn_id{ 0 };
	int16_t turn_id_by_player[2]{ 0,0 };
	uint32_t card_id{ 1 };
	uint16_t phase{ 0 };
	uint8_t turn_player{ 0 };
	uint8_t priorities[2]{ 0,0 };
	uint8_t can_shuffle{ TRUE };
};
struct lpcost {
	int32_t count{ 0 };
	int32_t amount{ 0 };
	int32_t lpstack[8]{};
};
struct processor_unit {
	uint16_t type;
	int16_t step;
	effect* peffect;
	group* ptarget;
	int64_t arg1;
	int64_t arg2;
	int64_t arg3;
	int64_t arg4;
	void* ptr1;
	void* ptr2;
};
template<typename T>
class return_card_generic {
public:
	bool canceled;
	std::vector<T> list;
	return_card_generic():canceled(false) {};
	void clear() {
		canceled = false;
		list.clear();
	}
};

using return_card = return_card_generic<card*>;
using return_card_code = return_card_generic<std::pair<uint32_t, uint32_t>>;

struct processor {
	using option_vector = std::vector<uint64_t>;
	using processor_list = std::list<processor_unit>;
	using delayed_effect_collection = std::set<std::pair<effect*, tevent>>;
	using effect_count_map = std::unordered_map<uint64_t, uint32_t>;
	struct chain_limit_t {
		chain_limit_t(int32_t f, int32_t p): function(f), player(p) {}
		int32_t function;
		int32_t player;
	};
	using chain_limit_list = std::vector<chain_limit_t>;
	struct action_value_t {
		int32_t check_function;
		uint16_t player_amount[2];
	};
	using action_counter_t = std::unordered_map<uint32_t, action_value_t>;

	processor_list units;
	processor_list subunits;
	processor_unit reserved;
	card_set just_sent_cards;
	card_vector select_cards;
	std::vector<std::pair<uint32_t, uint32_t>> select_cards_codes;
	card_vector unselect_cards;
	card_vector summonable_cards;
	card_vector spsummonable_cards;
	card_vector repositionable_cards;
	card_vector msetable_cards;
	card_vector ssetable_cards;
	card_vector attackable_cards;
	effect_vector select_effects;
	option_vector select_options;
	card_vector must_select_cards;
	event_list point_event;
	event_list instant_event;
	event_list queue_event;
	event_list delayed_activate_event;
	event_list full_event;
	event_list used_event;
	event_list single_event;
	event_list solving_event;
	event_list sub_solving_event;
	chain_array select_chains;
	chain_array current_chain;
	int32_t real_chain_count;
	chain_list tpchain;
	chain_list ntpchain;
	chain_array ignition_priority_chains;
	chain_list continuous_chain;
	chain_list solving_continuous;
	chain_list sub_solving_continuous;
	chain_list delayed_continuous_tp;
	chain_list delayed_continuous_ntp;
	chain_list desrep_chain;
	chain_list new_fchain;
	chain_list new_fchain_s;
	chain_list new_ochain;
	chain_list new_ochain_s;
	chain_list new_fchain_b;
	chain_list new_ochain_b;
	chain_list new_ochain_h;
	chain_list new_chains;
	delayed_effect_collection delayed_quick_tmp;
	delayed_effect_collection delayed_quick;
	instant_f_list quick_f_chain;
	card_set leave_confirmed;
	card_set special_summoning;
	card_set ss_tograve_set;
	card_set equiping_cards;
	card_set control_adjust_set[2];
	card_set unique_destroy_set;
	card_set self_destroy_set;
	card_set self_tograve_set;
	card_set trap_monster_adjust_set[2];
	card_set release_cards;
	card_set release_cards_ex;
	card_set release_cards_ex_oneof;
	card_set battle_destroy_rep;
	card_set fusion_materials;
	card_set operated_set;
	card_set discarded_set;
	card_set destroy_canceled;
	card_set delayed_enable_set;
	card_set set_group_pre_set;
	card_set set_group_set;
	effect_set_v disfield_effects;
	effect_set_v extra_mzone_effects;
	effect_set_v extra_szone_effects;
	std::set<effect*> reseted_effects;
	std::unordered_map<card*, uint32_t> readjust_map;
	std::unordered_set<card*> unique_cards[2];
	effect_count_map effect_count_code;
	effect_count_map effect_count_code_duel;
	std::unordered_map<uint32_t, uint32_t> spsummon_once_map[2];
	std::unordered_map<uint32_t, uint32_t> spsummon_once_map_rst[2];
	std::multimap<int32_t, card*, std::greater<int32_t>> xmaterial_lst;
	int64_t temp_var[4];
	uint32_t global_flag;
	uint32_t pre_field[2];
	std::set<uint32_t> opp_mzone;
	chain_limit_list chain_limit;
	chain_limit_list chain_limit_p;
	uint8_t chain_solving;
	uint8_t conti_solving;
	uint8_t win_player{ 5 };
	uint8_t win_reason;
	uint8_t re_adjust;
	effect* reason_effect;
	uint8_t reason_player{ PLAYER_NONE };
	card* summoning_card;
	uint32_t summoning_proc_group_type;
	uint8_t summon_depth;
	uint8_t summon_cancelable;
	card* attacker;
	card* attack_target;
	bool set_forced_attack;
	bool forced_attack;
	card* forced_attacker;
	card* forced_attack_target;
	group* must_use_mats;
	group* only_use_mats;
	int32_t forced_summon_minc;
	int32_t forced_summon_maxc;
	uint8_t attack_cancelable;
	uint8_t attack_cost_paid;
	uint8_t attack_rollback;
	uint8_t effect_damage_step;
	int32_t battle_damage[2];
	int32_t summon_count[2];
	uint8_t extra_summon[2];
	int32_t spe_effect[2];
	uint64_t duel_options;
	uint32_t copy_reset;
	uint8_t copy_reset_count;
	uint32_t last_control_changed_id;
	uint32_t set_group_used_zones;
	uint8_t set_group_seq[7];
	uint8_t dice_result[5];
	uint8_t coin_result[5];
	uint8_t to_bp;
	uint8_t to_m2;
	uint8_t to_ep;
	uint8_t skip_m2;
	uint8_t chain_attack;
	uint32_t chain_attacker_id;
	card* chain_attack_target;
	uint8_t attack_player;
	uint8_t selfdes_disabled;
	uint8_t overdraw[2];
	int32_t check_level;
	uint8_t shuffle_check_disabled;
	uint8_t shuffle_hand_check[2];
	uint8_t shuffle_deck_check[2];
	uint8_t deck_reversed;
	uint8_t remove_brainwashing;
	uint8_t flip_delayed;
	uint8_t damage_calculated;
	uint8_t hand_adjusted;
	uint8_t summon_state_count[2];
	uint8_t normalsummon_state_count[2];
	uint8_t flipsummon_state_count[2];
	uint8_t spsummon_state_count[2];
	uint8_t spsummon_state_count_rst[2];
	uint8_t spsummon_state_count_tmp[2];
	bool spsummon_rst;
	uint8_t attack_state_count[2];
	uint8_t battle_phase_count[2];
	uint8_t battled_count[2];
	uint8_t phase_action;
	uint32_t hint_timing[2];
	uint8_t current_player;
	uint8_t conti_player{ PLAYER_NONE };
	bool force_turn_end;
	action_counter_t summon_counter;
	action_counter_t normalsummon_counter;
	action_counter_t spsummon_counter;
	action_counter_t flipsummon_counter;
	action_counter_t attack_counter;
	action_counter_t chain_counter;
	processor_list recover_damage_reserve;
	effect_vector dec_count_reserve;
	action_counter_t& get_counter_map(ActivityType counter_type) {
		switch(counter_type) {
			case ACTIVITY_SUMMON:
				return summon_counter;
			case ACTIVITY_NORMALSUMMON:
				return normalsummon_counter;
			case ACTIVITY_SPSUMMON:
				return spsummon_counter;
			case ACTIVITY_FLIPSUMMON:
				return flipsummon_counter;
			case ACTIVITY_ATTACK:
				return attack_counter;
			case ACTIVITY_CHAIN:
				return chain_counter;
			default:
				unreachable();
		}
	}
};
class field {
public:
	duel* pduel;
	std::array<player_info,2> player;
	card* temp_card;
	field_info infos;
	//lpcost cost[2];
	field_effect effects;
	processor core{};
	ProgressiveBuffer returns;
	return_card return_cards;
	return_card_code return_card_codes;
	tevent nil_event;

	static int32_t field_used_count[32];
	explicit field(duel* pduel, const OCG_DuelOptions& options);
	~field() = default;
	void reload_field_info();

	void add_card(uint8_t playerid, card* pcard, uint8_t location, uint8_t sequence, uint8_t pzone = FALSE);
	void remove_card(card* pcard);
	uint8_t move_card(uint8_t playerid, card* pcard, uint8_t location, uint8_t sequence, uint8_t pzone = FALSE);
	void swap_card(card* pcard1, card* pcard2, uint8_t new_sequence1, uint8_t new_sequence2);
	void swap_card(card* pcard1, card* pcard2);
	void set_control(card* pcard, uint8_t playerid, uint16_t reset_phase, uint8_t reset_count);
	card* get_field_card(uint32_t playerid, uint32_t location, uint32_t sequence);
	int32_t is_location_useable(uint32_t playerid, uint32_t location, uint32_t sequence);
	int32_t get_useable_count(card* pcard, uint8_t playerid, uint8_t location, uint8_t uplayer, uint32_t reason, uint32_t zone = 0xff, uint32_t* list = 0);
	int32_t get_useable_count_fromex(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone = 0xff, uint32_t* list = 0);
	int32_t get_spsummonable_count(card* pcard, uint8_t playerid, uint32_t zone = 0xff, uint32_t* list = 0);
	int32_t get_spsummonable_count_fromex(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone = 0xff, uint32_t* list = 0);
	int32_t get_useable_count_other(card* pcard, uint8_t playerid, uint8_t location, uint8_t uplayer, uint32_t reason, uint32_t zone = 0xff, uint32_t* list = 0);
	int32_t get_tofield_count(card* pcard, uint8_t playerid, uint8_t location, uint32_t uplayer, uint32_t reason, uint32_t zone = 0xff, uint32_t* list = 0);
	int32_t get_useable_count_fromex_rule4(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone = 0xff, uint32_t* list = 0);
	int32_t get_spsummonable_count_fromex_rule4(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone = 0xff, uint32_t* list = 0);
	int32_t get_mzone_limit(uint8_t playerid, uint8_t uplayer, uint32_t reason);
	int32_t get_szone_limit(uint8_t playerid, uint8_t uplayer, uint32_t reason);
	int32_t get_forced_zones(card* pcard, uint8_t playerid, uint8_t location, uint32_t uplayer, uint32_t reason);
	uint32_t get_rule_zone_fromex(int32_t playerid, card* pcard);
	uint32_t get_linked_zone(int32_t playerid, bool free = false, bool actually_linked = false);
	void get_linked_cards(uint8_t self, uint8_t location1, uint8_t location2, card_set* cset);
	int32_t check_extra_link(int32_t playerid, card* pcard, int32_t sequence);
	void get_cards_in_zone(card_set* cset, uint32_t zone, int32_t playerid, int32_t location);
	void shuffle(uint8_t playerid, uint8_t location);
	void reset_sequence(uint8_t playerid, uint8_t location);
	void swap_deck_and_grave(uint8_t playerid);
	void reverse_deck(uint8_t playerid);
	int get_player_count(uint8_t playerid);
	void tag_swap(uint8_t playerid);
	bool relay_check(uint8_t playerid);
	void next_player(uint8_t playerid);

	bool is_flag(uint64_t flag) const;
	bool has_separate_pzone(uint8_t p) const;
	uint32_t get_pzone_zones_flag() const;
	int32_t get_pzone_index(uint8_t seq, uint8_t p) const;

	void add_effect(effect* peffect, uint8_t owner_player = 2);
	void remove_effect(effect* peffect);
	void remove_oath_effect(effect* reason_effect);
	void release_oath_relation(effect* reason_effect);
	void reset_phase(uint32_t phase);
	void reset_chain();
	void add_effect_code(uint32_t code, uint8_t flag, uint8_t hopt_index, uint8_t playerid);
	uint32_t get_effect_code(uint32_t code, uint8_t flag, uint8_t hopt_index, uint8_t playerid);
	void dec_effect_code(uint32_t code, uint8_t flag, uint8_t hopt_index, uint8_t playerid);

	void filter_field_effect(uint32_t code, effect_set* eset, uint8_t sort = TRUE);
	void filter_affected_cards(effect* peffect, card_set* cset);
	void filter_inrange_cards(effect* peffect, card_set* cset);
	void filter_player_effect(uint8_t playerid, uint32_t code, effect_set* eset, uint8_t sort = TRUE);
	int32_t filter_matching_card(int32_t findex, uint8_t self, uint32_t location1, uint32_t location2, group* pgroup, card* pexception, group* pexgroup, uint32_t extraargs, card** pret = 0, int32_t fcount = 0, int32_t is_target = FALSE);
	int32_t filter_field_card(uint8_t self, uint32_t location, uint32_t location2, group* pgroup);
	effect* is_player_affected_by_effect(uint8_t playerid, uint32_t code);
	int32_t get_player_effect(uint8_t playerid, uint32_t code);

	int32_t get_release_list(uint8_t playerid, card_set* release_list, card_set* ex_list, card_set* ex_list_oneof, int32_t use_hand, int32_t fun, int32_t exarg, card* exc, group* exg, uint8_t use_oppo);
	int32_t check_release_list(uint8_t playerid, int32_t min, int32_t max, int32_t use_hand, int32_t fun, int32_t exarg, card* exc, group* exg, uint8_t check_field, uint8_t to_player, uint8_t zone, card* to_check, uint8_t use_oppo);
	int32_t get_summon_release_list(card* target, card_set* release_list, card_set* ex_list, card_set* ex_list_oneof, group* mg = nullptr, uint32_t ex = 0, uint32_t releasable = 0xff00ff, uint32_t pos = 0x1);
	int32_t get_summon_count_limit(uint8_t playerid);
	int32_t get_draw_count(uint8_t playerid);
	void get_ritual_material(uint8_t playerid, effect* peffect, card_set* material, bool check_level);
	void get_fusion_material(uint8_t playerid, card_set* material);
	void ritual_release(const card_set& material);
	void get_overlay_group(uint8_t playerid, uint8_t self, uint8_t oppo, card_set* pset, group* pgroup);
	int32_t get_overlay_count(uint8_t playerid, uint8_t self, uint8_t oppo, group* pgroup);
	void update_disable_check_list(effect* peffect);
	void add_to_disable_check_list(card* pcard);
	void adjust_disable_check_list();
	void adjust_self_destroy_set();
	int32_t trap_monster_adjust(uint16_t step);
	void erase_grant_effect(effect* peffect);
	int32_t adjust_grant_effect();
	void add_unique_card(card* pcard);
	void remove_unique_card(card* pcard);
	effect* check_unique_onfield(card* pcard, uint8_t controler, uint8_t location, card* icard = 0);
	int32_t check_spsummon_once(card* pcard, uint8_t playerid);
	void check_card_counter(card* pcard, ActivityType counter_type, int32_t playerid);
	void check_card_counter(group* pgroup, ActivityType counter_type, int32_t playerid);
	chain::applied_chain_counter_t* check_chain_counter(effect* peffect, int32_t playerid, int32_t chainid);
	void restore_chain_counter(uint8_t playerid, const chain::applied_chain_counter_t& counters);
	void set_spsummon_counter(uint8_t playerid, bool add = true, bool chain = false);
	int32_t check_spsummon_counter(uint8_t playerid, uint8_t ct = 1);

	int32_t check_lp_cost(uint8_t playerid, uint32_t cost);
	void save_lp_cost() {}
	void restore_lp_cost() {}
	int32_t pay_lp_cost(uint32_t step, uint8_t playerid, uint32_t cost);

	uint32_t get_field_counter(uint8_t playerid, uint8_t self, uint8_t oppo, uint16_t countertype);
	int32_t effect_replace_check(uint32_t code, const tevent& e);
	int32_t get_attack_target(card* pcard, card_vector* v, uint8_t chain_attack = FALSE, bool select_target = true);
	bool confirm_attack_target();
	void attack_all_target_check();
	int32_t check_tribute(card* pcard, int32_t min, int32_t max, group* mg, uint8_t toplayer, uint32_t zone = 0x1f, uint32_t releasable = 0xff00ff, uint32_t pos = 0x1);
	static int32_t check_with_sum_limit(const card_vector& mats, int32_t acc, int32_t index, int32_t count, int32_t min, int32_t max, int32_t* should_continue);
	static int32_t check_with_sum_limit_m(const card_vector& mats, int32_t acc, int32_t index, int32_t min, int32_t max, int32_t must_count, int32_t* should_continue);
	static int32_t check_with_sum_greater_limit(const card_vector& mats, int32_t acc, int32_t index, int32_t opmin, int32_t* should_continue);
	static int32_t check_with_sum_greater_limit_m(const card_vector& mats, int32_t acc, int32_t index, int32_t opmin, int32_t must_count, int32_t* should_continue);

	int32_t is_player_can_draw(uint8_t playerid);
	int32_t is_player_can_discard_deck(uint8_t playerid, int32_t count);
	int32_t is_player_can_discard_deck_as_cost(uint8_t playerid, int32_t count);
	int32_t is_player_can_discard_hand(uint8_t playerid, card* pcard, effect* peffect, uint32_t reason);
	int32_t is_player_can_action(uint8_t playerid, uint32_t actionlimit);
	int32_t is_player_can_summon(uint32_t sumtype, uint8_t playerid, card* pcard, uint8_t toplayer);
	int32_t is_player_can_mset(uint32_t sumtype, uint8_t playerid, card* pcard, uint8_t toplayer);
	int32_t is_player_can_sset(uint8_t playerid, card* pcard);
	int32_t is_player_can_spsummon(uint8_t playerid);
	int32_t is_player_can_spsummon(effect* peffect, uint32_t sumtype, uint8_t sumpos, uint8_t playerid, uint8_t toplayer, card* pcard);
	int32_t is_player_can_flipsummon(uint8_t playerid, card* pcard);
	int32_t is_player_can_spsummon_monster(uint8_t playerid, uint8_t toplayer, uint8_t sumpos, uint32_t sumtype, card_data* pdata);
	int32_t is_player_can_spsummon_count(uint8_t playerid, uint32_t count);
	int32_t is_player_can_release(uint8_t playerid, card* pcard);
	int32_t is_player_can_place_counter(uint8_t playerid, card* pcard, uint16_t countertype, uint16_t count);
	int32_t is_player_can_remove_counter(uint8_t playerid, card* pcard, uint8_t self, uint8_t oppo, uint16_t countertype, uint16_t count, uint32_t reason);
	int32_t is_player_can_remove_overlay_card(uint8_t playerid, group* pcard, uint8_t self, uint8_t oppo, uint16_t count, uint32_t reason);
	int32_t is_player_can_send_to_grave(uint8_t playerid, card* pcard);
	int32_t is_player_can_send_to_hand(uint8_t playerid, card* pcard);
	int32_t is_player_can_send_to_deck(uint8_t playerid, card* pcard);
	int32_t is_player_can_remove(uint8_t playerid, card* pcard, uint32_t reason);
	int32_t is_chain_negatable(uint8_t chaincount);
	int32_t is_chain_disablable(uint8_t chaincount);
	int32_t is_chain_disabled(uint8_t chaincount);
	int32_t check_chain_target(uint8_t chaincount, card* pcard);
	chain* get_chain(uint8_t chaincount);
	int32_t get_cteffect(effect* peffect, int32_t playerid, int32_t store);
	int32_t get_cteffect_evt(effect* feffect, int32_t playerid, const tevent& e, int32_t store);
	int32_t check_cteffect_hint(effect* peffect, uint8_t playerid);
	int32_t check_nonpublic_trigger(chain& ch);
	int32_t check_trigger_effect(const chain& ch) const;
	int32_t check_spself_from_hand_trigger(const chain& ch) const;
	int32_t is_able_to_enter_bp();

	void add_process(uint16_t type, uint16_t step, effect* peffect, group* target, int64_t arg1, int64_t arg2, int64_t arg3 = 0, int64_t arg4 = 0, void* ptr1 = nullptr, void* ptr2 = nullptr);
	int32_t process();
	int32_t execute_cost(uint16_t step, effect* peffect, uint8_t triggering_player);
	int32_t execute_operation(uint16_t step, effect* peffect, uint8_t triggering_player);
	int32_t execute_target(uint16_t step, effect* peffect, uint8_t triggering_player);
	void raise_event(card* event_card, uint32_t event_code, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t event_player, uint32_t event_value);
	void raise_event(card_set* event_cards, uint32_t event_code, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t event_player, uint32_t event_value);
	void raise_single_event(card* trigger_card, card_set* event_cards, uint32_t event_code, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t event_player, uint32_t event_value);
	int32_t check_event(uint32_t code, tevent* pe = 0);
	int32_t check_event_c(effect* peffect, uint8_t playerid, int32_t neglect_con, int32_t neglect_cost, int32_t copy_info, tevent* pe = 0);
	int32_t check_hint_timing(effect* peffect);
	int32_t process_phase_event(int16_t step, int32_t phase_event);
	int32_t process_point_event(int16_t step, int32_t skip_trigger, int32_t skip_freechain, int32_t skip_new);
	int32_t process_quick_effect(int16_t step, int32_t skip_freechain, uint8_t priority);
	int32_t process_instant_event();
	int32_t process_single_event();
	int32_t process_single_event(effect* peffect, const tevent& e, chain_list& tp, chain_list& ntp);
	int32_t process_idle_command(uint16_t step);
	int32_t process_battle_command(uint16_t step);
	int32_t process_forced_battle(uint16_t step);
	int32_t process_damage_step(uint16_t step, uint32_t new_attack);
	void calculate_battle_damage(effect** pdamchange, card** preason_card, uint8_t* battle_destroyed);
	int32_t process_turn(uint16_t step, uint8_t turn_player);

	int32_t add_chain(uint16_t step);
	int32_t sort_chain(uint16_t step, uint8_t tp);
	void solve_continuous(uint8_t playerid, effect* peffect, const tevent& e);
	int32_t solve_continuous(uint16_t step);
	int32_t solve_chain(uint16_t step, uint32_t chainend_arg1, uint32_t chainend_arg2);
	int32_t break_effect(bool clear_sent = true);
	void adjust_instant();
	void adjust_all();
	void refresh_location_info_instant();
	int32_t refresh_location_info(uint16_t step);
	int32_t adjust_step(uint16_t step);
	int32_t startup(uint16_t step);
	int32_t refresh_relay(uint16_t step);

	//operations
	int32_t negate_chain(uint8_t chaincount);
	int32_t disable_chain(uint8_t chaincount);
	void change_chain_effect(uint8_t chaincount, int32_t replace_op);
	void change_target(uint8_t chaincount, group* targets);
	void change_target_player(uint8_t chaincount, uint8_t playerid);
	void change_target_param(uint8_t chaincount, int32_t param);
	void remove_counter(uint32_t reason, card* pcard, uint32_t rplayer, uint8_t self, uint8_t oppo, uint32_t countertype, uint32_t count);
	void remove_overlay_card(uint32_t reason, group* pgroup, uint32_t rplayer, uint8_t self, uint8_t oppo, uint16_t min, uint16_t max);
	void get_control(card_set targets, effect* reason_effect, uint32_t reason_player, uint32_t playerid, uint32_t reset_phase, uint32_t reset_count, uint32_t zone);
	void get_control(card* target, effect* reason_effect, uint32_t reason_player, uint32_t playerid, uint32_t reset_phase, uint32_t reset_count, uint32_t zone);
	void swap_control(effect* reason_effect, uint32_t reason_player, card_set targets1, card_set targets2, uint32_t reset_phase, uint32_t reset_count);
	void swap_control(effect* reason_effect, uint32_t reason_player, card* pcard1, card* pcard2, uint32_t reset_phase, uint32_t reset_count);
	void equip(uint32_t equip_player, card* equip_card, card* target, uint32_t up, uint32_t is_step);
	void draw(effect* reason_effect, uint32_t reason, uint32_t reason_player, uint32_t playerid, uint32_t count);
	void damage(effect* reason_effect, uint32_t reason, uint8_t reason_player, card* reason_card, uint8_t playerid, uint32_t amount, bool is_step = false);
	void recover(effect* reason_effect, uint32_t reason, uint32_t reason_player, uint32_t playerid, uint32_t amount, bool is_step = false);
	void summon(uint32_t sumplayer, card* target, effect* proc, uint32_t ignore_count, uint32_t min_tribute, uint32_t zone = 0x1f);
	void mset(uint32_t setplayer, card* target, effect* proc, uint32_t ignore_count, uint32_t min_tribute, uint32_t zone = 0x1f);
	void special_summon_rule(uint32_t sumplayer, card* target, uint32_t summon_type);
	void special_summon_rule_group(uint32_t sumplayer, uint32_t summon_type);
	void special_summon(card_set target, uint32_t sumtype, uint32_t sumplayer, uint32_t playerid, uint32_t nocheck, uint32_t nolimit, uint32_t positions, uint32_t zone);
	void special_summon_step(card* target, uint32_t sumtype, uint32_t sumplayer, uint32_t playerid, uint32_t nocheck, uint32_t nolimit, uint32_t positions, uint32_t zone);
	void special_summon_complete(effect* reason_effect, uint8_t reason_player);
	void destroy(card_set targets, effect* reason_effect, uint32_t reason, uint32_t reason_player, uint32_t playerid = 2, uint32_t destination = 0, uint32_t sequence = 0);
	void destroy(card* target, effect* reason_effect, uint32_t reason, uint32_t reason_player, uint32_t playerid = 2, uint32_t destination = 0, uint32_t sequence = 0);
	void release(card_set targets, effect* reason_effect, uint32_t reason, uint32_t reason_player);
	void release(card* target, effect* reason_effect, uint32_t reason, uint32_t reason_player);
	void send_to(card_set targets, effect* reason_effect, uint32_t reason, uint32_t reason_player, uint32_t playerid, uint32_t destination, uint32_t sequence, uint32_t position, uint32_t ignore = false);
	void send_to(card* target, effect* reason_effect, uint32_t reason, uint32_t reason_player, uint32_t playerid, uint32_t destination, uint32_t sequence, uint32_t position, uint32_t ignore = false);
	void move_to_field(card* target, uint32_t move_player, uint32_t playerid, uint32_t destination, uint32_t positions, uint8_t enable = FALSE, uint8_t ret = 0, uint8_t zone = 0xff, uint8_t rule = FALSE, uint8_t reason = 0, uint8_t confirm = TRUE);
	void change_position(card_set targets, effect* reason_effect, uint32_t reason_player, uint32_t au, uint32_t ad, uint32_t du, uint32_t dd, uint32_t flag, uint32_t enable = FALSE);
	void change_position(card* target, effect* reason_effect, uint32_t reason_player, uint32_t npos, uint32_t flag, uint32_t enable = FALSE);
	void operation_replace(int32_t type, int32_t step, group* targets);
	void select_tribute_cards(card* target, uint8_t playerid, uint8_t cancelable, int32_t min, int32_t max, uint8_t toplayer, uint32_t zone);

	int32_t remove_counter(uint16_t step, uint32_t reason, card* pcard, uint8_t rplayer, uint8_t s, uint8_t o, uint16_t countertype, uint16_t count);
	int32_t remove_overlay_card(uint16_t step, uint32_t reason, group* pgroup, uint8_t rplayer, uint8_t s, uint8_t o, uint16_t min, uint16_t max);
	int32_t get_control(uint16_t step, effect* reason_effect, uint8_t chose_player, group* targets, uint8_t playerid, uint16_t reset_phase, uint8_t reset_count, uint32_t zone);
	int32_t swap_control(uint16_t step, effect* reason_effect, uint8_t reason_player, group* targets1, group* targets2, uint16_t reset_phase, uint8_t reset_count);
	int32_t control_adjust(uint16_t step);
	int32_t self_destroy(uint16_t step, card* ucard, int32_t p);
	int32_t equip(uint16_t step, uint8_t equip_player, card* equip_card, card* target, uint32_t up, uint32_t is_step);
	int32_t draw(uint16_t step, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t playerid, uint32_t count);
	int32_t damage(uint16_t step, effect* reason_effect, uint32_t reason, uint8_t reason_player, card* reason_card, uint8_t playerid, uint32_t amount, bool is_step);
	int32_t recover(uint16_t step, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t playerid, uint32_t amount, bool is_step);
	int32_t summon(uint16_t step, uint8_t sumplayer, card* target, effect* proc, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone);
	int32_t flip_summon(uint16_t step, uint8_t sumplayer, card* target);
	int32_t mset(uint16_t step, uint8_t setplayer, card* ptarget, effect* proc, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone);
	int32_t sset(uint16_t step, uint8_t setplayer, uint8_t toplayer, card* ptarget, effect* reason_effect);
	int32_t sset_g(uint16_t step, uint8_t setplayer, uint8_t toplayer, group* ptarget, uint8_t confirm, effect* reason_effect);
	int32_t special_summon_rule(uint16_t step, uint8_t sumplayer, card* target, uint32_t summon_type);
	int32_t special_summon_rule_group(uint16_t step, uint8_t sumplayer, uint32_t summon_type);
	int32_t special_summon_step(uint16_t step, group* targets, card* target, uint32_t zone);
	int32_t special_summon(uint16_t step, effect* reason_effect, uint8_t reason_player, group* targets, uint32_t zone);
	int32_t destroy_replace(uint16_t step, group* targets, card* target, uint8_t battle);
	int32_t destroy(uint16_t step, group* targets, effect* reason_effect, uint32_t reason, uint8_t reason_player);
	int32_t release_replace(uint16_t step, group* targets, card* target);
	int32_t release(uint16_t step, group* targets, effect* reason_effect, uint32_t reason, uint8_t reason_player);
	int32_t send_replace(uint16_t step, group* targets, card* target);
	int32_t send_to(uint16_t step, group* targets, effect* reason_effect, uint32_t reason, uint8_t reason_player);
	int32_t discard_deck(uint16_t step, uint8_t playerid, uint8_t count, uint32_t reason);
	int32_t move_to_field(uint16_t step, card* target, uint8_t enable, uint8_t ret, uint8_t pzone, uint8_t zone, uint8_t rule = FALSE, uint8_t reason = 0, uint8_t confirm = TRUE);
	int32_t change_position(uint16_t step, group* targets, effect* reason_effect, uint8_t reason_player, uint32_t enable);
	int32_t operation_replace(uint16_t step, effect* replace_effect, group* targets, card* target, int32_t is_destroy);
	int32_t activate_effect(uint16_t step, effect* peffect);
	int32_t select_release_cards(int16_t step, uint8_t playerid, uint8_t cancelable, int32_t min, int32_t max, uint8_t check_field = FALSE, card* to_check = nullptr, uint8_t toplayer = PLAYER_NONE, uint8_t zone = 0xff);
	int32_t select_tribute_cards(int16_t step, card* target, uint8_t playerid, uint8_t cancelable, int32_t min, int32_t max, uint8_t toplayer, uint32_t zone);
	int32_t toss_coin(uint16_t step, effect* reason_effect, uint8_t reason_player, uint8_t playerid, uint8_t count);
	int32_t toss_dice(uint16_t step, effect* reason_effect, uint8_t reason_player, uint8_t playerid, uint8_t count1, uint8_t count2);
	int32_t rock_paper_scissors(uint16_t step, uint8_t repeat);

	int32_t select_battle_command(uint16_t step, uint8_t playerid);
	int32_t select_idle_command(uint16_t step, uint8_t playerid);
	int32_t select_effect_yes_no(uint16_t step, uint8_t playerid, uint64_t description, card* pcard);
	int32_t select_yes_no(uint16_t step, uint8_t playerid, uint64_t description);
	int32_t select_option(uint16_t step, uint8_t playerid);
	bool parse_response_cards(bool cancelable);
	int32_t select_card(uint16_t step, uint8_t playerid, uint8_t cancelable, uint8_t min, uint8_t max);
	int32_t select_card_codes(uint16_t step, uint8_t playerid, uint8_t cancelable, uint8_t min, uint8_t max);
	int32_t select_unselect_card(uint16_t step, uint8_t playerid, uint8_t cancelable, uint8_t min, uint8_t max, uint8_t finishable);
	int32_t select_chain(uint16_t step, uint8_t playerid, uint8_t spe_count, uint8_t forced);
	int32_t select_place(uint16_t step, uint8_t playerid, uint32_t flag, uint8_t count);
	int32_t select_position(uint16_t step, uint8_t playerid, uint32_t code, uint8_t positions);
	int32_t select_tribute(uint16_t step, uint8_t playerid, uint8_t cancelable, uint8_t min, uint8_t max);
	int32_t select_counter(uint16_t step, uint8_t playerid, uint16_t countertype, uint16_t count, uint8_t s, uint8_t o);
	int32_t select_with_sum_limit(int16_t step, uint8_t playerid, int32_t acc, int32_t min, int32_t max);
	int32_t sort_card(int16_t step, uint8_t playerid, uint8_t is_chain);
	int32_t announce_race(int16_t step, uint8_t playerid, int32_t count, int32_t available);
	int32_t announce_attribute(int16_t step, uint8_t playerid, int32_t count, int32_t available);
	int32_t announce_card(int16_t step, uint8_t playerid);
	int32_t announce_number(int16_t step, uint8_t playerid);
};

//Location Use Reason
#define LOCATION_REASON_TOFIELD	0x1
#define LOCATION_REASON_CONTROL	0x2
#define LOCATION_REASON_COUNT	0x4
#define LOCATION_REASON_RETURN	0x8
//Chain Info
#define CHAIN_DISABLE_ACTIVATE	0x01
#define CHAIN_DISABLE_EFFECT	0x02
#define CHAIN_HAND_EFFECT		0x04
#define CHAIN_CONTINUOUS_CARD	0x08
#define CHAIN_ACTIVATING		0x10
#define CHAIN_HAND_TRIGGER		0x20
//#define CHAIN_DECK_EFFECT		0x40
#define CHAININFO_CHAIN_COUNT			0x01
#define CHAININFO_TRIGGERING_EFFECT		0x02
#define CHAININFO_TRIGGERING_PLAYER		0x04
#define CHAININFO_TRIGGERING_CONTROLER	0x08
#define CHAININFO_TRIGGERING_LOCATION	0x10
#define CHAININFO_TRIGGERING_SEQUENCE	0x20
#define CHAININFO_TARGET_CARDS			0x40
#define CHAININFO_TARGET_PLAYER			0x80
#define CHAININFO_TARGET_PARAM			0x100
#define CHAININFO_DISABLE_REASON		0x200
#define CHAININFO_DISABLE_PLAYER		0x400
#define CHAININFO_CHAIN_ID				0x800
#define CHAININFO_TYPE					0x1000
#define CHAININFO_EXTTYPE				0x2000
#define CHAININFO_TRIGGERING_POSITION	0x4000
#define CHAININFO_TRIGGERING_CODE		0x8000
#define CHAININFO_TRIGGERING_CODE2		0x10000
//#define CHAININFO_TRIGGERING_TYPE		0x20000
#define CHAININFO_TRIGGERING_LEVEL		0x40000
#define CHAININFO_TRIGGERING_RANK		0x80000
#define CHAININFO_TRIGGERING_ATTRIBUTE	0x100000
#define CHAININFO_TRIGGERING_RACE		0x200000
#define CHAININFO_TRIGGERING_ATTACK		0x400000
#define CHAININFO_TRIGGERING_DEFENSE	0x800000
//Timing
#define TIMING_DRAW_PHASE			0x1
#define TIMING_STANDBY_PHASE		0x2
#define TIMING_MAIN_END				0x4
#define TIMING_BATTLE_START			0x8
#define TIMING_BATTLE_END			0x10
#define TIMING_END_PHASE			0x20
#define TIMING_SUMMON				0x40
#define TIMING_SPSUMMON				0x80
#define TIMING_FLIPSUMMON			0x100
#define TIMING_MSET					0x200
#define TIMING_SSET					0x400
#define TIMING_POS_CHANGE			0x800
#define TIMING_ATTACK				0x1000
#define TIMING_DAMAGE_STEP			0x2000
#define TIMING_DAMAGE_CAL			0x4000
#define TIMING_CHAIN_END			0x8000
#define TIMING_DRAW					0x10000
#define TIMING_DAMAGE				0x20000
#define TIMING_RECOVER				0x40000
#define TIMING_DESTROY				0x80000
#define TIMING_REMOVE				0x100000
#define TIMING_TOHAND				0x200000
#define TIMING_TODECK				0x400000
#define TIMING_TOGRAVE				0x800000
#define TIMING_BATTLE_PHASE			0x1000000
#define TIMING_EQUIP				0x2000000
#define TIMING_BATTLE_STEP_END		0x4000000
#define TIMING_BATTLED				0x8000000

#define GLOBALFLAG_DECK_REVERSE_CHECK	0x1
#define GLOBALFLAG_BRAINWASHING_CHECK	0x2
//#define GLOBALFLAG_SCRAP_CHIMERA		0x4
//#define GLOBALFLAG_DELAYED_QUICKEFFECT	0x8
#define GLOBALFLAG_DETACH_EVENT			0x10
//#define GLOBALFLAG_MUST_BE_SMATERIAL	0x20
#define GLOBALFLAG_SPSUMMON_COUNT		0x40
//#define GLOBALFLAG_XMAT_COUNT_LIMIT		0x80
#define GLOBALFLAG_SELF_TOGRAVE			0x100
#define GLOBALFLAG_SPSUMMON_ONCE		0x200
//#define GLOBALFLAG_TUNE_MAGICIAN		0x400
//
#define PROCESSOR_FLAG_END		0
#define PROCESSOR_FLAG_WAITING	0x1
#define PROCESSOR_FLAG_CONTINUE	0x2

#define PROCESSOR_ADJUST			1
#define PROCESSOR_HINT				2
#define PROCESSOR_TURN				3
#define PROCESSOR_REFRESH_LOC		5
#define PROCESSOR_STARTUP			6
#define PROCESSOR_SELECT_IDLECMD	10
#define PROCESSOR_SELECT_EFFECTYN	11
#define PROCESSOR_SELECT_BATTLECMD	12
#define PROCESSOR_SELECT_YESNO		13
#define PROCESSOR_SELECT_OPTION		14
#define PROCESSOR_SELECT_CARD		15
#define PROCESSOR_SELECT_CHAIN		16
#define PROCESSOR_SELECT_UNSELECT_CARD	17
#define PROCESSOR_SELECT_PLACE		18
#define PROCESSOR_SELECT_POSITION	19
#define PROCESSOR_SELECT_TRIBUTE_P	20
#define PROCESSOR_SORT_CHAIN		21
#define PROCESSOR_SELECT_COUNTER	22
#define PROCESSOR_SELECT_SUM		23
#define PROCESSOR_SELECT_DISFIELD	24
#define PROCESSOR_SORT_CARD			25
#define PROCESSOR_SELECT_RELEASE	26
#define PROCESSOR_SELECT_TRIBUTE	27
#define PROCESSOR_SELECT_CARD_CODES	28
#define PROCESSOR_POINT_EVENT		30
#define PROCESSOR_QUICK_EFFECT		31
#define PROCESSOR_IDLE_COMMAND		32
#define PROCESSOR_PHASE_EVENT		33
#define PROCESSOR_BATTLE_COMMAND	34
#define PROCESSOR_DAMAGE_STEP		35
#define PROCESSOR_FORCED_BATTLE		36
#define PROCESSOR_ADD_CHAIN			40
#define PROCESSOR_SOLVE_CHAIN		42
#define PROCESSOR_SOLVE_CONTINUOUS	43
#define PROCESSOR_EXECUTE_COST		44
#define PROCESSOR_EXECUTE_OPERATION	45
#define PROCESSOR_EXECUTE_TARGET	46
#define PROCESSOR_DESTROY			50
#define PROCESSOR_RELEASE			51
#define PROCESSOR_SENDTO			52
#define PROCESSOR_MOVETOFIELD		53
#define PROCESSOR_CHANGEPOS			54
#define PROCESSOR_OPERATION_REPLACE	55
#define PROCESSOR_DESTROY_REPLACE	56
#define PROCESSOR_RELEASE_REPLACE	57
#define PROCESSOR_SENDTO_REPLACE	58
#define PROCESSOR_SUMMON_RULE		60
#define PROCESSOR_SPSUMMON_RULE		61
#define PROCESSOR_SPSUMMON			62
#define PROCESSOR_FLIP_SUMMON		63
#define PROCESSOR_MSET				64
#define PROCESSOR_SSET				65
#define PROCESSOR_SPSUMMON_STEP		66
#define PROCESSOR_SSET_G			67
#define PROCESSOR_SPSUMMON_RULE_G	68
#define PROCESSOR_DRAW				70
#define PROCESSOR_DAMAGE			71
#define PROCESSOR_RECOVER			72
#define PROCESSOR_EQUIP				73
#define PROCESSOR_GET_CONTROL		74
#define PROCESSOR_SWAP_CONTROL		75
#define PROCESSOR_CONTROL_ADJUST	76
#define PROCESSOR_SELF_DESTROY		77
#define PROCESSOR_TRAP_MONSTER_ADJUST	78
#define PROCESSOR_PAY_LPCOST		80
#define PROCESSOR_REMOVE_COUNTER	81
#define PROCESSOR_ATTACK_DISABLE	82
#define PROCESSOR_ACTIVATE_EFFECT	83

#define PROCESSOR_ANNOUNCE_RACE		110
#define PROCESSOR_ANNOUNCE_ATTRIB	111
#define PROCESSOR_ANNOUNCE_LEVEL	112
#define PROCESSOR_ANNOUNCE_CARD		113
#define PROCESSOR_ANNOUNCE_TYPE		114
#define PROCESSOR_ANNOUNCE_NUMBER	115
#define PROCESSOR_ANNOUNCE_COIN		116
#define PROCESSOR_TOSS_DICE			117
#define PROCESSOR_TOSS_COIN			118
#define PROCESSOR_ROCK_PAPER_SCISSORS	119

#define PROCESSOR_SELECT_FUSION		131
#define PROCESSOR_DISCARD_HAND	150
#define PROCESSOR_DISCARD_DECK	151
#define PROCESSOR_SORT_DECK		152
#define PROCESSOR_REMOVE_OVERLAY		160

#define PROCESSOR_REFRESH_RELAY		170

//#define PROCESSOR_DESTROY_S			100
//#define PROCESSOR_RELEASE_S			101
//#define PROCESSOR_SENDTO_S			102
//#define PROCESSOR_CHANGEPOS_S		103
//#define PROCESSOR_SELECT_YESNO_S	120
//#define PROCESSOR_SELECT_OPTION_S	121
//#define PROCESSOR_SELECT_CARD_S		122
//#define PROCESSOR_SELECT_EFFECTYN_S	123
//#define PROCESSOR_SELECT_UNSELECT_CARD_S	124
//#define PROCESSOR_SELECT_PLACE_S	125
//#define PROCESSOR_SELECT_POSITION_S	126
//#define PROCESSOR_SELECT_TRIBUTE_S	127
//#define PROCESSOR_SORT_CARDS_S		128
//#define PROCESSOR_SELECT_RELEASE_S	129
//#define PROCESSOR_SELECT_TARGET		130
//#define PROCESSOR_SELECT_SYNCHRO	132
//#define PROCESSOR_SELECT_XMATERIAL	139
//#define PROCESSOR_SELECT_SUM_S		133
//#define PROCESSOR_SELECT_DISFIELD_S	134
//#define PROCESSOR_SPSUMMON_S		135
//#define PROCESSOR_SPSUMMON_STEP_S	136
//#define PROCESSOR_SPSUMMON_COMP_S	137
//#define PROCESSOR_RANDOM_SELECT_S	138
//#define PROCESSOR_DRAW_S			140
//#define PROCESSOR_DAMAGE_S			141
//#define PROCESSOR_RECOVER_S			142
//#define PROCESSOR_EQUIP_S			143
//#define PROCESSOR_GET_CONTROL_S		144
//#define PROCESSOR_SWAP_CONTROL_S	145
//#define PROCESSOR_MOVETOFIELD_S		161

#endif /* FIELD_H_ */
