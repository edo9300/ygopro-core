/*
 * Copyright (c) 2010-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2017-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef FIELD_H_
#define FIELD_H_

#include <array>
#include <functional>
#include <list>
#include <map>
#include <memory> //std::shared_ptr
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility> //std::forward
#include <vector>
#include "bit.h"
#include "card.h"
#include "common.h"
#include "containers_fwd.h"
#include "mpark/variant.hpp"
#include "progressivebuffer.h"

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
	uint32_t triggering_status{};
	uint32_t triggering_summon_type{};
	uint8_t triggering_summon_location{};
	bool triggering_summon_proc_complete{}; //properly summoned or not
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
	bool can_shuffle{ true };
};
struct lpcost {
	int32_t count{ 0 };
	int32_t amount{ 0 };
	int32_t lpstack[8]{};
};

#define PROCESSOR_ADJUST			1
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
#define PROCESSOR_XYZ_OVERLAY		161

#define PROCESSOR_REFRESH_RELAY		170

namespace Processors {
struct InvalidState {};
struct Adjust {
	int16_t step;
	Adjust(int16_t step_) : step(step_) {}
};
struct Turn {
	int16_t step;
	uint8_t turn_player;
	bool has_performed_second_battle_phase;
	Turn(int16_t step_, uint8_t turn_player_) : step(step_), turn_player(turn_player_),
		has_performed_second_battle_phase(false) {}
};
struct RefreshLoc {
	int16_t step;
	uint8_t dis_count;
	uint32_t previously_disabled_locations;
	effect* current_disable_field_effect;
	RefreshLoc(int16_t step_) :
		step(step_), dis_count(0), previously_disabled_locations(0),
		current_disable_field_effect(nullptr) {}
};
struct Startup {
	int16_t step;
	Startup(int16_t step_) : step(step_) {}
};
struct SelectBattleCmd {
	int16_t step;
	uint8_t playerid;
	SelectBattleCmd(int16_t step_, uint8_t playerid_) : step(step_), playerid(playerid_) {}
};
struct SelectIdleCmd {
	int16_t step;
	uint8_t playerid;
	SelectIdleCmd(int16_t step_, uint8_t playerid_) : step(step_), playerid(playerid_) {}
};
struct SelectEffectYesNo {
	int16_t step;
	uint8_t playerid;
	card* pcard;
	uint64_t description;
	SelectEffectYesNo(int16_t step_, uint8_t playerid_, uint64_t description_, card* pcard_) :
		step(step_), playerid(playerid_), pcard(pcard_), description(description_) {}
};
struct SelectYesNo {
	int16_t step;
	uint8_t playerid;
	uint64_t description;
	SelectYesNo(int16_t step_, uint8_t playerid_, uint64_t description_) :
		step(step_), playerid(playerid_), description(description_) {}
};
struct SelectOption {
	int16_t step;
	uint8_t playerid;
	SelectOption(int16_t step_, uint8_t playerid_) : step(step_), playerid(playerid_) {}
};
struct SelectCard {
	int16_t step;
	uint8_t playerid;
	bool cancelable;
	uint8_t min;
	uint8_t max;
	SelectCard(int16_t step_, uint8_t playerid_, bool cancelable_,
			   uint8_t min_, uint8_t max_) :
		step(step_), playerid(playerid_), cancelable(cancelable_), min(min_), max(max_) {}
};
struct SelectCardCodes {
	int16_t step;
	uint8_t playerid;
	bool cancelable;
	uint8_t min;
	uint8_t max;
	SelectCardCodes(int16_t step_, uint8_t playerid_, bool cancelable_,
					uint8_t min_, uint8_t max_) :
		step(step_), playerid(playerid_), cancelable(cancelable_), min(min_), max(max_) {}
};
struct SelectUnselectCard {
	int16_t step;
	uint8_t playerid;
	bool cancelable;
	uint8_t min;
	uint8_t max;
	bool finishable;
	SelectUnselectCard(int16_t step_, uint8_t playerid_, bool cancelable_,
					   uint8_t min_, uint8_t max_, bool finishable_) :
		step(step_), playerid(playerid_), cancelable(cancelable_), min(min_), max(max_),
		finishable(finishable_) {}
};
struct SelectChain {
	int16_t step;
	uint8_t playerid;
	uint8_t spe_count;
	bool forced;
	SelectChain(int16_t step_, uint8_t playerid_, uint8_t spe_count_, bool forced_) :
		step(step_), playerid(playerid_), spe_count(spe_count_), forced(forced_) {}
};
struct SelectPlace {
	int16_t step;
	uint8_t playerid;
	uint8_t count;
	uint32_t flag;
	bool disable_field;
	SelectPlace(int16_t step_, uint8_t playerid_, uint32_t flag_, uint8_t count_) :
		step(step_), playerid(playerid_), count(count_), flag(flag_), disable_field(false) {}
};
struct SelectDisField : SelectPlace {
	SelectDisField(int16_t step_, uint8_t playerid_, uint32_t flag_, uint8_t count_) :
		SelectPlace(step_, playerid_, flag_, count_) {
		disable_field = true;
	};
};
struct SelectPosition {
	int16_t step;
	uint8_t playerid;
	uint8_t positions;
	uint32_t code;
	SelectPosition(int16_t step_, uint8_t playerid_, uint32_t code_, uint8_t positions_) :
		step(step_), playerid(playerid_), positions(positions_), code(code_) {}
};
struct SelectTributeP {
	int16_t step;
	uint8_t playerid;
	bool cancelable;
	uint8_t min;
	uint8_t max;
	SelectTributeP(int16_t step_, uint8_t playerid_, bool cancelable_, uint8_t min_, uint8_t max_) :
		step(step_), playerid(playerid_), cancelable(cancelable_), min(min_), max(max_) {}
};
struct SortChain {
	int16_t step;
	uint8_t playerid;
	SortChain(int16_t step_, uint8_t playerid_) : step(step_), playerid(playerid_) {}
};
struct SelectCounter {
	int16_t step;
	uint16_t countertype;
	uint16_t count;
	uint8_t playerid;
	uint8_t self;
	uint8_t oppo;
	SelectCounter(uint16_t step_, uint8_t playerid_, uint16_t countertype_, uint16_t count_,
				  uint8_t self_, uint8_t oppo_) :
		step(step_), countertype(countertype_), count(count_), playerid(playerid_), self(self_),
		oppo(oppo_) {}
};
struct SelectSum {
	int16_t step;
	uint8_t playerid;
	int32_t acc;
	int32_t min;
	int32_t max;
	SelectSum(int16_t step_, uint8_t playerid_, int32_t acc_, int32_t min_, int32_t max_) :
		step(step_), playerid(playerid_), acc(acc_), min(min_), max(max_) {}
};
struct SortCard {
	int16_t step;
	uint8_t playerid;
	bool is_chain;
	SortCard(int16_t step_, uint8_t playerid_, bool is_chain_) :
		step(step_), playerid(playerid_), is_chain(is_chain_) {}
};
struct SelectRelease {
	int16_t step;
	uint16_t min;
	uint16_t max;
	uint8_t playerid;
	bool cancelable;
	bool check_field;
	uint8_t toplayer;
	uint8_t zone;
	card* to_check;
	effect* extra_release_nonsum_effect;
	std::unique_ptr<card_set> must_choose_one;
	SelectRelease(int16_t step_, uint8_t playerid_, bool cancelable_, uint16_t min_,
				  uint16_t max_, bool check_field_, card* to_check_, uint8_t toplayer_,
				  uint8_t zone_) :
		step(step_), min(min_), max(max_), playerid(playerid_), cancelable(cancelable_),
		check_field(check_field_), toplayer(toplayer_), zone(zone_), to_check(to_check_),
		extra_release_nonsum_effect(nullptr), must_choose_one(nullptr) {}
};
struct SelectTribute {
	int16_t step;
	uint16_t min;
	uint16_t max;
	uint8_t playerid;
	bool cancelable;
	uint8_t toplayer;
	uint8_t zone;
	card* target;
	effect* extra_release_effect;
	std::unique_ptr<card_set> must_choose_one;
	SelectTribute(int16_t step_, card* target_, uint8_t playerid_, bool cancelable_, int32_t min_,
				  int32_t max_, uint8_t toplayer_, uint8_t zone_) :
		step(step_), min(min_), max(max_), playerid(playerid_), cancelable(cancelable_),
		toplayer(toplayer_), zone(zone_), target(target_), extra_release_effect(nullptr),
		must_choose_one(nullptr) {}
};
struct PointEvent {
	int16_t step;
	bool skip_trigger;
	bool skip_freechain;
	bool skip_new;
	PointEvent(int16_t step_, bool skip_trigger_, bool skip_freechain_, bool skip_new_) :
		step(step_), skip_trigger(skip_trigger_), skip_freechain(skip_freechain_), skip_new(skip_new_) {}
};
struct QuickEffect {
	int16_t step;
	bool skip_freechain;
	bool is_opponent;
	uint8_t priority_player;
	QuickEffect(int16_t step_, bool skip_freechain_, uint8_t priority_player_) :
		step(step_), skip_freechain(skip_freechain_), is_opponent(false), priority_player(priority_player_) {}
};
struct IdleCommand {
	int16_t step;
	uint8_t phase_to_change_to;
	card* card_to_reposition;
	IdleCommand(int16_t step_) :
		step(step_), phase_to_change_to(0), card_to_reposition(nullptr) {}
};
struct PhaseEvent {
	int16_t step;
	uint16_t phase;
	bool is_opponent;
	bool priority_passed;
	PhaseEvent(int16_t step_, uint16_t phase_) :
		step(step_), phase(phase_), is_opponent(false), priority_passed(false) {}
};
struct BattleCommand {
	int16_t step;
	uint16_t phase_to_change_to;
	bool is_replaying_attack;
	bool attack_announce_failed;
	bool repeat_battle_phase;
	bool second_battle_phase_is_optional;
	bool previous_point_event_had_any_trigger_to_resolve;
	effect* damage_change_effect;
	group* cards_destroyed_by_battle;
	BattleCommand(int16_t step_, group* cards_destroyed_by_battle_ = nullptr) :
		step(step_), phase_to_change_to(0), is_replaying_attack(false), attack_announce_failed(false),
		repeat_battle_phase(false), second_battle_phase_is_optional(false),
		previous_point_event_had_any_trigger_to_resolve(false), damage_change_effect(nullptr),
		cards_destroyed_by_battle(cards_destroyed_by_battle_) {}
};
struct DamageStep {
	int16_t step;
	uint16_t backup_phase;
	bool new_attack;
	card* attacker;
	card* attack_target;
	group* cards_destroyed_by_battle;
	DamageStep(int16_t step_, card* attacker_, card* attack_target_, bool new_attack_) :
		step(step_), backup_phase(0), new_attack(new_attack_), attacker(attacker_), attack_target(attack_target_), cards_destroyed_by_battle(nullptr) {}
};
struct ForcedBattle {
	int16_t step;
	uint16_t backup_phase;
	ForcedBattle(int16_t step_) :
		step(step_), backup_phase(0) {}
};
struct AddChain {
	int16_t step;
	bool is_activated_effect;
	AddChain(int16_t step_) :
		step(step_), is_activated_effect(false) {}
};
struct SolveChain {
	int16_t step;
	bool skip_trigger;
	bool skip_freechain;
	bool skip_new;
	int32_t backed_up_operation;
	SolveChain(int16_t step_, bool skip_trigger_, bool skip_freechain_, bool skip_new_) :
		step(step_), skip_trigger(skip_trigger_), skip_freechain(skip_freechain_),
		skip_new(skip_new_), backed_up_operation(0) {}
};
struct SolveContinuous {
	int16_t step;
	uint8_t reason_player;
	effect* reason_effect;
	SolveContinuous(int16_t step_) :
		step(step_), reason_player(0), reason_effect(nullptr) {}
};
struct ExecuteCost {
	int16_t step;
	uint8_t triggering_player;
	bool shuffle_check_was_disabled;
	effect* triggering_effect;
	ExecuteCost(int16_t step_, effect* triggering_effect_, uint8_t triggering_player_) :
		step(step_), triggering_player(triggering_player_), shuffle_check_was_disabled(false),
		triggering_effect(triggering_effect_) {}
};
struct ExecuteOperation {
	int16_t step;
	uint8_t triggering_player;
	bool shuffle_check_was_disabled;
	effect* triggering_effect;
	ExecuteOperation(int16_t step_, effect* triggering_effect_, uint8_t triggering_player_) :
		step(step_), triggering_player(triggering_player_), shuffle_check_was_disabled(false),
		triggering_effect(triggering_effect_) {}
};
struct ExecuteTarget {
	int16_t step;
	uint8_t triggering_player;
	bool shuffle_check_was_disabled;
	effect* triggering_effect;
	ExecuteTarget(int16_t step_, effect* triggering_effect_, uint8_t triggering_player_) :
		step(step_), triggering_player(triggering_player_), shuffle_check_was_disabled(false),
		triggering_effect(triggering_effect_) {}
};
struct Destroy {
	int16_t step;
	uint8_t reason_player;
	uint32_t reason;
	group* targets;
	effect* reason_effect;
	Destroy(uint16_t step_, group* targets_, effect* reason_effect_, uint32_t reason_,
					 uint8_t reason_player_) :
		step(step_), reason_player(reason_player_), reason(reason_), targets(targets_),
		reason_effect(reason_effect_) {}
};
struct Release {
	int16_t step;
	uint8_t reason_player;
	uint32_t reason;
	group* targets;
	effect* reason_effect;
	Release(uint16_t step_, group* targets_, effect* reason_effect_, uint32_t reason_,
					 uint8_t reason_player_) :
		step(step_), reason_player(reason_player_), reason(reason_), targets(targets_),
		reason_effect(reason_effect_) {}
};
struct SendTo {
	struct exargs {
		card_set leave_field, leave_grave, detach;
		bool show_decktop[2];
		card_vector cv;
		card_vector::iterator cvit;
		effect* predirect;
	};
	int16_t step;
	uint8_t reason_player;
	uint32_t reason;
	group* targets;
	effect* reason_effect;
	std::unique_ptr<exargs> extra_args;
	SendTo(uint16_t step_, group* targets_, effect* reason_effect_, uint32_t reason_,
					uint8_t reason_player_) :
		step(step_), reason_player(reason_player_), reason(reason_), targets(targets_),
		reason_effect(reason_effect_), extra_args(nullptr) {}
};
struct DestroyReplace {
	int16_t step;
	bool battle;
	group* targets;
	card* target;
	DestroyReplace(uint16_t step_, group* targets_, card* target_, bool battle_) :
		step(step_), battle(battle_), targets(targets_), target(target_) {}
};
struct ReleaseReplace {
	int16_t step;
	group* targets;
	card* target;
	ReleaseReplace(uint16_t step_, group* targets_, card* target_) :
		step(step_), targets(targets_), target(target_) {}
};
struct SendToReplace {
	int16_t step;
	group* targets;
	card* target;
	SendToReplace(uint16_t step_, group* targets_, card* target_) :
		step(step_), targets(targets_), target(target_) {}
};
struct MoveToField {
	int16_t step;
	bool enable;
	uint8_t ret;
	bool pzone;
	uint8_t zone;
	bool rule;
	uint8_t location_reason;
	bool confirm;
	card* target;
	MoveToField(uint16_t step_, card* target_, bool enable_, uint8_t ret_, bool pzone_,
						 uint8_t zone_, bool rule_, uint8_t location_reason_, bool confirm_) :
		step(step_), enable(enable_), ret(ret_) , pzone(pzone_) , zone(zone_) , rule(rule_) ,
		location_reason(location_reason_) , confirm(confirm_), target(target_) {}
};
struct ChangePos {
	int16_t step;
	uint8_t reason_player;
	bool enable;
	bool oppo_selection;
	effect* reason_effect;
	group* targets;
	std::unique_ptr<card_set> to_grave_set;
	ChangePos(uint16_t step_, group* targets_, effect* reason_effect_,
					   uint8_t reason_player_, bool enable_) :
		step(step_), reason_player(reason_player_), enable(enable_), oppo_selection(false),
		reason_effect(reason_effect_), targets(targets_), to_grave_set(nullptr) {}
};
struct OperationReplace {
	int16_t step;
	bool is_destroy;
	effect* replace_effect;
	group* targets;
	card* target;
	OperationReplace(uint16_t step_, effect* replace_effect_, group* targets_,
							  card* target_, bool is_destroy_) :
		step(step_), is_destroy(is_destroy_), replace_effect(replace_effect_),
		targets(targets_), target(target_) {}
};
struct ActivateEffect {
	int16_t step;
	effect* peffect;
	ActivateEffect(uint16_t step_, effect* peffect_) :
		step(step_), peffect(peffect_) {}
};
struct SummonRule {
	int16_t step;
	uint8_t sumplayer;
	uint8_t min_tribute;
	uint8_t max_allowed_tributes;
	bool ignore_count;
	uint32_t zone;
	card* target;
	effect* summon_procedure_effect;
	effect* extra_summon_effect;
	std::unique_ptr<card_set> tributes;
	std::unique_ptr<effect_set> spsummon_cost_effects;
	SummonRule(uint16_t step_, uint8_t sumplayer_, card* target_, effect* proc_,
						bool ignore_count_, uint8_t min_tribute_, uint32_t zone_) :
		step(step_), sumplayer(sumplayer_), min_tribute(min_tribute_), max_allowed_tributes(0),
		ignore_count(ignore_count_), zone(zone_), target(target_), summon_procedure_effect(proc_),
		extra_summon_effect(nullptr), tributes(nullptr), spsummon_cost_effects(nullptr) {}
};
struct SpSummonRule {
	int16_t step;
	uint8_t sumplayer;
	bool is_mid_chain;
	uint32_t summon_type;
	card* target;
	effect* summon_proc_effect;
	group* cards_to_summon_g;
	std::unique_ptr<effect_set> special_spsummon_cost_effects;
	SpSummonRule(uint16_t step_, uint8_t sumplayer_, card* target_, uint32_t summon_type_, bool is_mid_chain_ = false) :
		step(step_), sumplayer(sumplayer_), is_mid_chain(is_mid_chain_), summon_type(summon_type_), target(target_),
		summon_proc_effect(nullptr), cards_to_summon_g(nullptr), special_spsummon_cost_effects(nullptr) {}
};
struct SpSummon {
	int16_t step;
	uint8_t reason_player;
	uint32_t zone;
	effect* reason_effect;
	group* targets;
	SpSummon(uint16_t step_, effect* reason_effect_, uint8_t reason_player_,
					  group* targets_, uint32_t zone_) :
		step(step_), reason_player(reason_player_), zone(zone_), reason_effect(reason_effect_),
		targets(targets_) {}
};
struct FlipSummon {
	int16_t step;
	uint8_t sumplayer;
	card* target;
	std::unique_ptr<effect_set> flip_spsummon_cost_effects;
	FlipSummon(uint16_t step_, uint8_t sumplayer_, card* target_) :
		step(step_), sumplayer(sumplayer_), target(target_), flip_spsummon_cost_effects(nullptr) {}
};
struct MonsterSet {
	int16_t step;
	uint8_t setplayer;
	uint8_t min_tribute;
	uint8_t max_allowed_tributes;
	bool ignore_count;
	uint32_t zone;
	card* target;
	effect* summon_procedure_effect;
	effect* extra_summon_effect;
	std::unique_ptr<card_set> tributes;
	MonsterSet(uint16_t step_, uint8_t setplayer_, card* target_, effect* proc_,
						bool ignore_count_, uint8_t min_tribute_, uint32_t zone_) :
		step(step_), setplayer(setplayer_), min_tribute(min_tribute_), max_allowed_tributes(0), ignore_count(ignore_count_),
		zone(zone_), target(target_), summon_procedure_effect(proc_), extra_summon_effect(nullptr), tributes(nullptr) {}
};
struct SpellSet {
	int16_t step;
	uint8_t setplayer;
	uint8_t toplayer;
	card* target;
	effect* reason_effect;
	SpellSet(uint16_t step_, uint8_t setplayer_, uint8_t toplayer_, card* target_,
					  effect* reason_effect_) :
		step(step_), setplayer(setplayer_), toplayer(toplayer_), target(target_),
		reason_effect(reason_effect_) {}
};
struct SpSummonStep {
	int16_t step;
	uint32_t zone;
	group* targets;
	card* target;
	std::unique_ptr<effect_set> spsummon_cost_effects;
	SpSummonStep(uint16_t step_, group* targets_, card* target_, uint32_t zone_) :
		step(step_), zone(zone_), targets(targets_), target(target_), spsummon_cost_effects(nullptr) {}
};
struct SpellSetGroup {
	int16_t step;
	uint8_t setplayer;
	uint8_t toplayer;
	bool confirm;
	group* ptarget;
	effect* reason_effect;
	SpellSetGroup(uint16_t step_, uint8_t setplayer_, uint8_t toplayer_,
						   group* ptarget_, bool confirm_, effect* reason_effect_) :
		step(step_), setplayer(setplayer_), toplayer(toplayer_), confirm(confirm_),
		ptarget(ptarget_), reason_effect(reason_effect_) {}
};
struct SpSummonRuleGroup {
	int16_t step;
	uint8_t sumplayer;
	uint32_t summon_type;
	SpSummonRuleGroup(uint16_t step_, uint8_t sumplayer_, uint32_t summon_type_) :
		step(step_), sumplayer(sumplayer_), summon_type(summon_type_) {}
};
struct Draw {
	int16_t step;
	uint16_t count;
	uint8_t reason_player;
	uint8_t playerid;
	uint32_t reason;
	effect* reason_effect;
	std::unique_ptr<card_set> drawn_set;
	Draw(uint16_t step_, effect* reason_effect_, uint32_t reason_, uint8_t reason_player_,
				  uint8_t playerid_, uint16_t count_) :
		step(step_), count(count_), reason_player(reason_player_), playerid(playerid_),
		reason(reason_), reason_effect(reason_effect_), drawn_set(nullptr) {}
};
struct Damage {
	int16_t step;
	uint8_t reason_player;
	uint8_t playerid;
	bool is_step;
	bool is_reflected;
	uint32_t amount;
	uint32_t reason;
	card* reason_card;
	effect* reason_effect;
	Damage(uint16_t step_, effect* reason_effect_, uint32_t reason_, uint8_t reason_player_,
					card* reason_card_, uint8_t playerid_, uint32_t amount_, bool is_step_) :
		step(step_), reason_player(reason_player_), playerid(playerid_), is_step(is_step_), is_reflected(false),
		amount(amount_), reason(reason_), reason_card(reason_card_), reason_effect(reason_effect_) {}
};
struct Recover {
	int16_t step;
	uint8_t reason_player;
	uint8_t playerid;
	bool is_step;
	uint32_t amount;
	uint32_t reason;
	effect* reason_effect;
	Recover(uint16_t step_, effect* reason_effect_, uint32_t reason_, uint8_t reason_player_,
					uint8_t playerid_, uint32_t amount_, bool is_step_) :
		step(step_), reason_player(reason_player_), playerid(playerid_), is_step(is_step_), amount(amount_),
		reason(reason_), reason_effect(reason_effect_) {}
};
struct Equip {
	int16_t step;
	uint8_t equip_player;
	bool is_step;
	bool faceup;
	card* equip_card;
	card* target;
	Equip(uint16_t step_, uint8_t equip_player_, card* equip_card_, card* target_,
				   bool faceup_, bool is_step_) :
		step(step_), equip_player(equip_player_), is_step(is_step_), faceup(faceup_),
		equip_card(equip_card_), target(target_) {}
};
struct GetControl {
	int16_t step;
	uint8_t chose_player;
	uint8_t playerid;
	uint8_t reset_count;
	uint16_t reset_phase;
	uint32_t zone;
	effect* reason_effect;
	group* targets;
	std::unique_ptr<card_set> destroy_set;
	GetControl(uint16_t step_, effect* reason_effect_, uint8_t chose_player_, group* targets_,
						uint8_t playerid_, uint16_t reset_phase_, uint8_t reset_count_, uint32_t zone_) :
		step(step_), chose_player(chose_player_), playerid(playerid_), reset_count(reset_count_),
		reset_phase(reset_phase_), zone(zone_), reason_effect(reason_effect_), targets(targets_),
		destroy_set(nullptr) {}
};
struct SwapControl {
	int16_t step;
	uint8_t reset_count;
	uint8_t reason_player;
	uint8_t self_selected_sequence;
	uint16_t reset_phase;
	effect* reason_effect;
	group* targets1;
	group* targets2;
	SwapControl(uint16_t step_, effect* reason_effect_, uint8_t reason_player_,
						 group* targets1_, group* targets2_, uint16_t reset_phase_, uint8_t reset_count_) :
		step(step_), reset_count(reset_count_), reason_player(reason_player_), self_selected_sequence(0),
		reset_phase(reset_phase_), reason_effect(reason_effect_), targets1(targets1_), targets2(targets2_) {}
};
struct ControlAdjust {
	int16_t step;
	uint8_t adjusting_player;
	std::unique_ptr<card_set> destroy_set;
	std::unique_ptr<card_set> adjust_set;
	ControlAdjust(uint16_t step_) :
		step(step_), adjusting_player(0), destroy_set(nullptr), adjust_set(nullptr) {}
};
struct SelfDestroyUnique {
	int16_t step;
	uint8_t playerid;
	card* unique_card;
	SelfDestroyUnique(uint16_t step_, card* unique_card_, uint8_t playerid_) :
		step(step_), playerid(playerid_), unique_card(unique_card_) {}
};
struct SelfDestroy {
	int16_t step;
	SelfDestroy(uint16_t step_) :
		step(step_) {}
};
struct SelfToGrave {
	int16_t step;
	SelfToGrave(uint16_t step_) :
		step(step_) {}
};
struct TrapMonsterAdjust {
	int16_t step;
	bool oppo_selection;
	std::unique_ptr<card_set> to_grave_set;
	TrapMonsterAdjust(uint16_t step_) :
		step(step_), oppo_selection(false), to_grave_set(nullptr) {}
};
struct PayLPCost {
	int16_t step;
	uint8_t playerid;
	uint32_t cost;
	PayLPCost(uint16_t step_, uint8_t playerid_, uint32_t cost_) :
		step(step_), playerid(playerid_), cost(cost_) {}
};
struct RemoveCounter {
	int16_t step;
	uint8_t rplayer;
	uint8_t self;
	uint8_t oppo;
	uint16_t countertype;
	uint16_t count;
	uint32_t reason;
	card* pcard;
	RemoveCounter(uint16_t step_, uint32_t reason_, card* pcard_, uint8_t rplayer_, uint8_t self_,
						   uint8_t oppo_, uint16_t countertype_, uint16_t count_) :
		step(step_), rplayer(rplayer_), self(self_), oppo(oppo_), countertype(countertype_), count(count_),
		reason(reason_), pcard(pcard_) {}
};
struct AttackDisable {
	int16_t step;
	AttackDisable(uint16_t step_) :
		step(step_) {}
};
struct AnnounceRace {
	int16_t step;
	uint8_t playerid;
	uint8_t count;
	uint64_t available;
	AnnounceRace(uint16_t step_, uint8_t playerid_, uint8_t count_, uint64_t available_) :
		step(step_), playerid(playerid_), count(count_), available(available_) {}
};
struct AnnounceAttribute {
	int16_t step;
	uint8_t playerid;
	uint8_t count;
	uint32_t available;
	AnnounceAttribute(uint16_t step_, uint8_t playerid_, uint8_t count_, uint32_t available_) :
		step(step_), playerid(playerid_), count(count_), available(available_) {}
};
struct AnnounceCard {
	int16_t step;
	uint8_t playerid;
	AnnounceCard(uint16_t step_, uint8_t playerid_) :
		step(step_), playerid(playerid_) {}
};
struct AnnounceNumber {
	int16_t step;
	uint8_t playerid;
	AnnounceNumber(uint16_t step_, uint8_t playerid_) :
		step(step_), playerid(playerid_) {}
};
struct TossCoin {
	int16_t step;
	uint8_t playerid;
	uint8_t reason_player;
	uint8_t count;
	effect* reason_effect;
	TossCoin(uint16_t step_, effect* reason_effect_, uint8_t reason_player_, uint8_t playerid_,
					  uint8_t count_) :
		step(step_), playerid(playerid_), reason_player(reason_player_), count(count_),
		reason_effect(reason_effect_) {}
};
struct TossDice {
	int16_t step;
	uint8_t playerid;
	uint8_t reason_player;
	uint8_t count1;
	uint8_t count2;
	effect* reason_effect;
	TossDice(uint16_t step_, effect* reason_effect_, uint8_t reason_player_, uint8_t playerid_,
					  uint8_t count1_, uint8_t count2_) :
		step(step_), playerid(playerid_), reason_player(reason_player_), count1(count1_), count2(count2_),
		reason_effect(reason_effect_) {}
};
struct RockPaperScissors {
	int16_t step;
	bool repeat;
	uint8_t hand0;
	RockPaperScissors(uint16_t step_, bool repeat_) :
		step(step_), repeat(repeat_), hand0(0) {}
};
struct SelectFusion {
	int16_t step;
	uint8_t playerid;
	uint32_t chkf;
	group* fusion_materials;
	group* forced_materials;
	card* pcard;
	SelectFusion(uint16_t step_, uint8_t playerid_, group* fusion_materials_, uint32_t chkf_,
						  group* forced_materials_, card* pcard_) :
		step(step_), playerid(playerid_), chkf(chkf_), fusion_materials(fusion_materials_),
		forced_materials(forced_materials_), pcard(pcard_) {}
};
struct DiscardHand {
	int16_t step;
	uint8_t playerid;
	uint8_t min;
	uint8_t max;
	uint32_t reason;
	DiscardHand(uint16_t step_, uint8_t playerid_, uint8_t min_, uint8_t max_, uint32_t reason_) :
		step(step_), playerid(playerid_), min(min_), max(max_), reason(reason_) {}
};
struct DiscardDeck {
	int16_t step;
	uint16_t count;
	uint8_t playerid;
	uint32_t reason;
	DiscardDeck(uint16_t step_, uint8_t playerid_, uint16_t count_, uint32_t reason_) :
		step(step_), count(count_), playerid(playerid_), reason(reason_) {}
};
struct SortDeck {
	int16_t step;
	uint16_t count;
	uint8_t sort_player;
	uint8_t target_player;
	bool bottom;
	SortDeck(uint16_t step_, uint8_t sort_player_, uint8_t target_player_, uint16_t count_,
					  bool bottom_) :
		step(step_), count(count_), sort_player(sort_player_), target_player(target_player_),
		bottom(bottom_) {}
};
struct RemoveOverlay {
	int16_t step;
	uint16_t min;
	uint16_t max;
	uint16_t replaced_amount;
	bool has_used_overlay_remove_replace_effect;
	uint8_t rplayer;
	uint8_t self;
	uint8_t oppo;
	uint32_t reason;
	group* pgroup;
	RemoveOverlay(uint16_t step_, uint32_t reason_, group* pgroup_, uint8_t rplayer_,
						   uint8_t self_, uint8_t oppo_, uint16_t min_, uint16_t max_) :
		step(step_), min(min_), max(max_), replaced_amount(0),
		has_used_overlay_remove_replace_effect(false), rplayer(rplayer_), self(self_),
		oppo(oppo_), reason(reason_), pgroup(pgroup_) {}
};
struct XyzOverlay {
	int16_t step;
	bool send_materials_to_grave;
	card* target;
	group* materials;
	XyzOverlay(uint16_t step_, card* target_, group* materials_, bool send_materials_to_grave_) :
		step(step_), send_materials_to_grave(send_materials_to_grave_), target(target_),
		materials(materials_) {}
};
struct RefreshRelay {
	int16_t step;
	RefreshRelay(uint16_t step_) :
		step(step_) {}
};

using processor_unit = mpark::variant<InvalidState, Adjust, Turn, RefreshLoc, Startup,
	SelectBattleCmd, SelectIdleCmd, SelectEffectYesNo, SelectYesNo,
	SelectOption, SelectCard, SelectCardCodes, SelectUnselectCard,
	SelectChain, SelectPlace, SelectDisField, SelectPosition,
	SelectTributeP, SortChain, SelectCounter, SelectSum, SortCard,
	SelectRelease, SelectTribute, QuickEffect, IdleCommand, PhaseEvent, PointEvent,
	BattleCommand, DamageStep, ForcedBattle, AddChain, SolveChain, SolveContinuous,
	ExecuteCost, ExecuteOperation, ExecuteTarget, Destroy, Release,
	SendTo, DestroyReplace, ReleaseReplace, SendToReplace, MoveToField,
	ChangePos, OperationReplace, ActivateEffect, SummonRule, SpSummonRule,
	SpSummon, FlipSummon, MonsterSet, SpellSet, SpSummonStep, SpellSetGroup,
	SpSummonRuleGroup, Draw, Damage, Recover, Equip, GetControl, SwapControl,
	ControlAdjust, SelfDestroyUnique, SelfDestroy, SelfToGrave, TrapMonsterAdjust,
	PayLPCost, RemoveCounter, AttackDisable, AnnounceRace, AnnounceAttribute,
	AnnounceCard, AnnounceNumber, TossCoin, TossDice, RockPaperScissors,
	SelectFusion, DiscardHand, DiscardDeck, SortDeck, RemoveOverlay, XyzOverlay,
	RefreshRelay>;
}

using Processors::processor_unit;

template<typename T>
class return_card_generic {
public:
	bool canceled;
	std::vector<T> list;
	return_card_generic():canceled(false) {}
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
	chain_list select_chains;
	chain_array current_chain;
	int32_t real_chain_count;
	chain_list tpchain;
	chain_list ntpchain;
	chain_list ignition_priority_chains;
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
	effect_count_map effect_count_code_chain;
	std::unordered_map<uint32_t, uint32_t> spsummon_once_map[2];
	std::unordered_map<uint32_t, uint32_t> spsummon_once_map_rst[2];
	std::multimap<int32_t, card*, std::greater<int32_t>> xmaterial_lst;
	int64_t temp_var[4];
	uint32_t global_flag;
	uint32_t pre_field[2];
	std::set<uint32_t> opp_mzone;
	chain_limit_list chain_limit;
	chain_limit_list chain_limit_p;
	bool chain_solving;
	bool conti_solving;
	uint8_t win_player{ 5 };
	uint8_t win_reason;
	bool re_adjust;
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
	bool attack_cancelable;
	uint8_t attack_cost_paid;
	bool attack_rollback;
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
	std::vector<uint8_t> dice_results;
	std::vector<bool> coin_results;
	bool to_bp;
	bool to_m2;
	bool to_ep;
	bool skip_m2;
	bool chain_attack;
	uint32_t chain_attacker_id;
	card* chain_attack_target;
	uint8_t attack_player;
	bool selfdes_disabled;
	bool overdraw[2];
	int32_t check_level;
	bool shuffle_check_disabled;
	bool shuffle_hand_check[2];
	bool shuffle_deck_check[2];
	bool deck_reversed;
	bool remove_brainwashing;
	bool flip_delayed;
	bool damage_calculated;
	bool hand_adjusted;
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
	bool phase_action;
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

	static constexpr auto field_used_count = ([]() constexpr {
		std::array<int32_t, 32> ret{};
		for(size_t i = 0; i < ret.size(); ++i) {
			ret[i] = bit::popcnt(i);
		}
		return ret;
	})();
	explicit field(duel* pduel, const OCG_DuelOptions& options);
	~field() = default;
	void reload_field_info();

	void add_card(uint8_t playerid, card* pcard, uint8_t location, uint8_t sequence, bool pzone = false);
	void remove_card(card* pcard);
	bool move_card(uint8_t playerid, card* pcard, uint8_t location, uint8_t sequence, bool pzone = false);
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
	void get_cards_in_zone(card_set* cset, uint32_t zone, int32_t playerid, int32_t location) const;
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
	uint8_t get_pzone_index(uint8_t seq, uint8_t p) const;

	void add_effect(effect* peffect, uint8_t owner_player = 2);
	void remove_effect(effect* peffect);
	void remove_oath_effect(effect* reason_effect);
	void release_oath_relation(effect* reason_effect);
	void reset_phase(uint32_t phase);
	void reset_chain();
	processor::effect_count_map& get_count_map(uint8_t flag);
	void add_effect_code(uint32_t code, uint8_t flag, uint8_t hopt_index, uint8_t playerid);
	uint32_t get_effect_code(uint32_t code, uint8_t flag, uint8_t hopt_index, uint8_t playerid);
	void dec_effect_code(uint32_t code, uint8_t flag, uint8_t hopt_index, uint8_t playerid);

	void filter_field_effect(uint32_t code, effect_set* eset, bool sort = true);
	void filter_affected_cards(effect* peffect, card_set* cset);
	void filter_inrange_cards(effect* peffect, card_set* cset);
	void filter_player_effect(uint8_t playerid, uint32_t code, effect_set* eset, bool sort = true);
	int32_t filter_matching_card(int32_t findex, uint8_t self, uint32_t location1, uint32_t location2, group* pgroup, card* pexception, group* pexgroup, uint32_t extraargs, card** pret = nullptr, int32_t fcount = 0, bool is_target = false);
	int32_t filter_field_card(uint8_t self, uint32_t location, uint32_t location2, group* pgroup);
	effect* is_player_affected_by_effect(uint8_t playerid, uint32_t code);
	void get_player_effect(uint8_t playerid, uint32_t code, effect_set* eset);

	int32_t get_release_list(uint8_t playerid, card_set* release_list, card_set* ex_list, card_set* ex_list_oneof, int32_t use_hand, int32_t fun, int32_t exarg, card* exc, group* exg, bool use_oppo, uint32_t reason);
	int32_t check_release_list(uint8_t playerid, int32_t min, int32_t max, int32_t use_hand, int32_t fun, int32_t exarg, card* exc, group* exg, bool check_field, uint8_t to_player, uint8_t zone, card* to_check, bool use_oppo, uint32_t reason);
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
	int32_t trap_monster_adjust(Processors::TrapMonsterAdjust& arg);
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
	int32_t pay_lp_cost(Processors::PayLPCost& arg);

	uint32_t get_field_counter(uint8_t playerid, uint8_t self, uint8_t oppo, uint16_t countertype);
	int32_t effect_replace_check(uint32_t code, const tevent& e);
	int32_t get_attack_target(card* pcard, card_vector* v, bool chain_attack = false, bool select_target = true);
	bool confirm_attack_target();
	void attack_all_target_check();
	int32_t check_tribute(card* pcard, int32_t min, int32_t max, group* mg, uint8_t toplayer, uint32_t zone = 0x1f, uint32_t releasable = 0xff00ff, uint32_t pos = 0x1);
	static int32_t check_with_sum_limit(const card_vector& mats, int32_t acc, int32_t index, int32_t count, int32_t min, int32_t max, int32_t* should_continue);
	static int32_t check_with_sum_limit_m(const card_vector& mats, int32_t acc, int32_t index, int32_t min, int32_t max, int32_t must_count, int32_t* should_continue);
	static int32_t check_with_sum_greater_limit(const card_vector& mats, int32_t acc, int32_t index, int32_t opmin, int32_t* should_continue);
	static int32_t check_with_sum_greater_limit_m(const card_vector& mats, int32_t acc, int32_t index, int32_t opmin, int32_t must_count, int32_t* should_continue);

	int32_t is_player_can_draw(uint8_t playerid);
	int32_t is_player_can_discard_deck(uint8_t playerid, uint32_t count);
	int32_t is_player_can_discard_deck_as_cost(uint8_t playerid, uint32_t count);
	int32_t is_player_can_discard_hand(uint8_t playerid, card* pcard, effect* peffect, uint32_t reason);
	int32_t is_player_can_action(uint8_t playerid, uint32_t actionlimit);
	int32_t is_player_can_summon(uint32_t sumtype, uint8_t playerid, card* pcard, uint8_t toplayer);
	int32_t is_player_can_mset(uint32_t sumtype, uint8_t playerid, card* pcard, uint8_t toplayer);
	int32_t is_player_can_sset(uint8_t playerid, card* pcard);
	int32_t is_player_can_spsummon(uint8_t playerid);
	int32_t is_player_can_spsummon(effect* peffect, uint32_t sumtype, uint8_t sumpos, uint8_t playerid, uint8_t toplayer, card* pcard, effect* proc_effect = nullptr);
	int32_t is_player_can_flipsummon(uint8_t playerid, card* pcard);
	int32_t is_player_can_spsummon_monster(uint8_t playerid, uint8_t toplayer, uint8_t sumpos, uint32_t sumtype, card_data* pdata);
	int32_t is_player_can_spsummon_count(uint8_t playerid, uint32_t count);
	int32_t is_player_can_release(uint8_t playerid, card* pcard, uint32_t reason);
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

	template<typename T, typename... Args>
	constexpr inline void emplace_process(Args&&... args) {
		emplace_process_step<T>(0, std::forward<Args>(args)...);
	}
	template<typename T, typename... Args>
	constexpr inline void emplace_process_step(uint16_t step, Args&&... args) {
		core.subunits.emplace_back(mpark::in_place_type<T>, step, std::forward<Args>(args)...);
	}
	int32_t process();
	int32_t execute_cost(Processors::ExecuteCost& arg);
	int32_t execute_operation(Processors::ExecuteOperation& arg);
	int32_t execute_target(Processors::ExecuteTarget& arg);
	void raise_event(card* event_card, uint32_t event_code, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t event_player, uint32_t event_value);
	void raise_event(card_set* event_cards, uint32_t event_code, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t event_player, uint32_t event_value);
	void raise_single_event(card* trigger_card, card_set* event_cards, uint32_t event_code, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t event_player, uint32_t event_value);
	int32_t check_event(uint32_t code, tevent* pe = 0);
	int32_t check_event_c(effect* peffect, uint8_t playerid, int32_t neglect_con, int32_t neglect_cost, int32_t copy_info, tevent* pe = 0);
	int32_t check_hint_timing(effect* peffect);
	int32_t process_phase_event(Processors::PhaseEvent& arg);
	int32_t process_point_event(Processors::PointEvent& arg);
	int32_t process_quick_effect(Processors::QuickEffect& arg);
	int32_t process_instant_event();
	int32_t process_single_event();
	int32_t process_single_event(effect* peffect, const tevent& e, chain_list& tp, chain_list& ntp);
	int32_t process_idle_command(Processors::IdleCommand& arg);
	int32_t process_battle_command(Processors::BattleCommand& arg);
	int32_t process_forced_battle(Processors::ForcedBattle& arg);
	int32_t process_damage_step(Processors::DamageStep& arg);
	void calculate_battle_damage(effect** pdamchange, card** preason_card, std::array<bool, 2>* battle_destroyed);
	int32_t process_turn(Processors::Turn& arg);

	int32_t add_chain(Processors::AddChain& arg);
	int32_t sort_chain(Processors::SortChain& arg);
	void solve_continuous(uint8_t playerid, effect* peffect, const tevent& e);
	int32_t solve_continuous(Processors::SolveContinuous& arg);
	int32_t solve_chain(Processors::SolveChain& arg);
	int32_t break_effect(bool clear_sent = true);
	void adjust_instant();
	void adjust_all();
	void refresh_location_info_instant();
	int32_t refresh_location_info(Processors::RefreshLoc& arg);
	int32_t adjust_step(Processors::Adjust& arg);
	int32_t startup(Processors::Startup& arg);
	int32_t refresh_relay(Processors::RefreshRelay& arg);

	//operations
	int32_t negate_chain(uint8_t chaincount);
	int32_t disable_chain(uint8_t chaincount);
	void change_chain_effect(uint8_t chaincount, int32_t replace_op);
	void change_target(uint8_t chaincount, group* targets);
	void change_target_player(uint8_t chaincount, uint8_t playerid);
	void change_target_param(uint8_t chaincount, int32_t param);
	void remove_counter(uint32_t reason, card* pcard, uint32_t rplayer, uint8_t self, uint8_t oppo, uint32_t countertype, uint32_t count);
	void remove_overlay_card(uint32_t reason, group* pgroup, uint32_t rplayer, uint8_t self, uint8_t oppo, uint16_t min, uint16_t max);
	void xyz_overlay(card* target, card_set materials, bool send_materials_to_grave);
	void xyz_overlay(card* target, card* material, bool send_materials_to_grave);
	void get_control(card_set targets, effect* reason_effect, uint32_t reason_player, uint32_t playerid, uint32_t reset_phase, uint32_t reset_count, uint32_t zone);
	void get_control(card* target, effect* reason_effect, uint32_t reason_player, uint32_t playerid, uint32_t reset_phase, uint32_t reset_count, uint32_t zone);
	void swap_control(effect* reason_effect, uint32_t reason_player, card_set targets1, card_set targets2, uint32_t reset_phase, uint32_t reset_count);
	void swap_control(effect* reason_effect, uint32_t reason_player, card* pcard1, card* pcard2, uint32_t reset_phase, uint32_t reset_count);
	void equip(uint32_t equip_player, card* equip_card, card* target, bool faceup, bool is_step);
	void draw(effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t playerid, uint16_t count);
	void damage(effect* reason_effect, uint32_t reason, uint8_t reason_player, card* reason_card, uint8_t playerid, uint32_t amount, bool is_step = false);
	void recover(effect* reason_effect, uint32_t reason, uint32_t reason_player, uint32_t playerid, uint32_t amount, bool is_step = false);
	void summon(uint8_t sumplayer, card* target, effect* summon_procedure_effect, bool ignore_count, uint8_t min_tribute, uint32_t zone = 0x1f);
	void mset(uint8_t setplayer, card* target, effect* proc, bool ignore_count, uint8_t min_tribute, uint32_t zone = 0x1f);
	void special_summon_rule(uint8_t sumplayer, card* target, uint32_t summon_type);
	void special_summon_rule_group(uint8_t sumplayer, uint32_t summon_type);
	void special_summon(card_set target, uint32_t sumtype, uint8_t sumplayer, uint8_t playerid, bool nocheck, bool nolimit, uint8_t positions, uint32_t zone);
	void special_summon_step(card* target, uint32_t sumtype, uint8_t sumplayer, uint8_t playerid, bool nocheck, bool nolimit, uint8_t positions, uint32_t zone);
	void special_summon_complete(effect* reason_effect, uint8_t reason_player);
	void destroy(card_set targets, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t playerid = 2, uint16_t destination = 0, uint32_t sequence = 0);
	void destroy(card* target, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t playerid = 2, uint16_t destination = 0, uint32_t sequence = 0);
	void release(card_set targets, effect* reason_effect, uint32_t reason, uint8_t reason_player);
	void release(card* target, effect* reason_effect, uint32_t reason, uint8_t reason_player);
	void send_to(card_set targets, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t playerid, uint16_t destination, uint32_t sequence, uint8_t position, bool ignore = false);
	void send_to(card* target, effect* reason_effect, uint32_t reason, uint8_t reason_player, uint8_t playerid, uint16_t destination, uint32_t sequence, uint8_t position, bool ignore = false);
	void move_to_field(card* target, uint8_t move_player, uint8_t playerid, uint16_t destination, uint8_t positions, bool enable = false, uint8_t ret = 0, uint8_t zone = 0xff, bool rule = false, uint8_t reason = 0, bool confirm = false);
	void change_position(card_set targets, effect* reason_effect, uint8_t reason_player, uint8_t au, uint8_t ad, uint8_t du, uint8_t dd, uint32_t flag, bool enable = false);
	void change_position(card* target, effect* reason_effect, uint8_t reason_player, uint8_t npos, uint32_t flag, bool enable = false);
	void operation_replace(uint32_t type, uint16_t step, group* targets);
	void select_tribute_cards(card* target, uint8_t playerid, bool cancelable, uint16_t min, uint16_t max, uint8_t toplayer, uint32_t zone);

	int32_t remove_counter(Processors::RemoveCounter& arg);
	int32_t remove_overlay_card(Processors::RemoveOverlay& arg);
	int32_t xyz_overlay(Processors::XyzOverlay& arg);
	int32_t get_control(Processors::GetControl& arg);
	int32_t swap_control(Processors::SwapControl& arg);
	int32_t control_adjust(Processors::ControlAdjust& arg);
	int32_t self_destroy_unique(Processors::SelfDestroyUnique& arg);
	int32_t self_destroy(Processors::SelfDestroy& arg);
	int32_t self_to_grave(Processors::SelfToGrave& arg);
	int32_t equip(Processors::Equip& arg);
	int32_t draw(Processors::Draw& arg);
	int32_t damage(Processors::Damage& arg);
	int32_t recover(Processors::Recover& arg);
	int32_t summon(Processors::SummonRule& arg);
	int32_t flip_summon(Processors::FlipSummon& arg);
	int32_t mset(Processors::MonsterSet& arg);
	int32_t sset(Processors::SpellSet& arg);
	int32_t sset_g(Processors::SpellSetGroup& arg);
	int32_t special_summon_rule(Processors::SpSummonRule& arg);
	int32_t special_summon_rule_group(Processors::SpSummonRuleGroup& arg);
	int32_t special_summon_step(Processors::SpSummonStep& arg);
	int32_t special_summon(Processors::SpSummon& arg);
	int32_t destroy_replace(Processors::DestroyReplace& arg);
	int32_t destroy(Processors::Destroy& arg);
	int32_t release_replace(Processors::ReleaseReplace& arg);
	int32_t release(Processors::Release& arg);
	int32_t send_replace(Processors::SendToReplace& arg);
	int32_t send_to(Processors::SendTo& arg);
	int32_t discard_deck(Processors::DiscardDeck& arg);
	int32_t move_to_field(Processors::MoveToField& arg);
	int32_t change_position(Processors::ChangePos& arg);
	int32_t operation_replace(Processors::OperationReplace& arg);
	int32_t activate_effect(Processors::ActivateEffect& arg);
	int32_t select_release_cards(Processors::SelectRelease& arg);
	int32_t select_tribute_cards(Processors::SelectTribute& arg);
	int32_t toss_coin(Processors::TossCoin& arg);
	int32_t toss_dice(Processors::TossDice& arg);
	int32_t rock_paper_scissors(Processors::RockPaperScissors& arg);

	int32_t select_battle_command(Processors::SelectBattleCmd& arg);
	int32_t select_idle_command(Processors::SelectIdleCmd& arg);
	int32_t select_effect_yes_no(Processors::SelectEffectYesNo& arg);
	int32_t select_yes_no(Processors::SelectYesNo& arg);
	int32_t select_option(Processors::SelectOption& arg);
	bool parse_response_cards(bool cancelable);
	int32_t select_card(Processors::SelectCard& arg);
	int32_t select_card_codes(Processors::SelectCardCodes& arg);
	int32_t select_unselect_card(Processors::SelectUnselectCard& arg);
	int32_t select_chain(Processors::SelectChain& arg);
	int32_t select_place(Processors::SelectPlace& arg);
	int32_t select_position(Processors::SelectPosition& arg);
	int32_t select_tribute(Processors::SelectTributeP& arg);
	int32_t select_counter(Processors::SelectCounter& arg);
	int32_t select_with_sum_limit(Processors::SelectSum& arg);
	int32_t sort_card(Processors::SortCard& arg);
	int32_t announce_race(Processors::AnnounceRace& arg);
	int32_t announce_attribute(Processors::AnnounceAttribute& arg);
	int32_t announce_card(Processors::AnnounceCard& arg);
	int32_t announce_number(Processors::AnnounceNumber& arg);

	int32_t operator()(Processors::InvalidState& arg);
	int32_t operator()(Processors::Adjust& arg);
	int32_t operator()(Processors::Turn& arg);
	int32_t operator()(Processors::RefreshLoc& arg);
	int32_t operator()(Processors::Startup& arg);
	int32_t operator()(Processors::SelectBattleCmd& arg);
	int32_t operator()(Processors::SelectIdleCmd& arg);
	int32_t operator()(Processors::SelectEffectYesNo& arg);
	int32_t operator()(Processors::SelectYesNo& arg);
	int32_t operator()(Processors::SelectOption& arg);
	int32_t operator()(Processors::SelectCard& arg);
	int32_t operator()(Processors::SelectCardCodes& arg);
	int32_t operator()(Processors::SelectUnselectCard& arg);
	int32_t operator()(Processors::SelectChain& arg);
	int32_t operator()(Processors::SelectPlace& arg);
	int32_t operator()(Processors::SelectDisField& arg);
	int32_t operator()(Processors::SelectPosition& arg);
	int32_t operator()(Processors::SelectTributeP& arg);
	int32_t operator()(Processors::SortChain& arg);
	int32_t operator()(Processors::SelectCounter& arg);
	int32_t operator()(Processors::SelectSum& arg);
	int32_t operator()(Processors::SortCard& arg);
	int32_t operator()(Processors::SelectRelease& arg);
	int32_t operator()(Processors::SelectTribute& arg);
	int32_t operator()(Processors::PointEvent& arg);
	int32_t operator()(Processors::SolveContinuous& arg);
	int32_t operator()(Processors::ExecuteCost& arg);
	int32_t operator()(Processors::ExecuteOperation& arg);
	int32_t operator()(Processors::ExecuteTarget& arg);
	int32_t operator()(Processors::QuickEffect& arg);
	int32_t operator()(Processors::IdleCommand& arg);
	int32_t operator()(Processors::PhaseEvent& arg);
	int32_t operator()(Processors::BattleCommand& arg);
	int32_t operator()(Processors::DamageStep& arg);
	int32_t operator()(Processors::ForcedBattle& arg);
	int32_t operator()(Processors::AddChain& arg);
	int32_t operator()(Processors::SolveChain& arg);
	int32_t operator()(Processors::Destroy& arg);
	int32_t operator()(Processors::Release& arg);
	int32_t operator()(Processors::SendTo& arg);
	int32_t operator()(Processors::DestroyReplace& arg);
	int32_t operator()(Processors::ReleaseReplace& arg);
	int32_t operator()(Processors::SendToReplace& arg);
	int32_t operator()(Processors::MoveToField& arg);
	int32_t operator()(Processors::ChangePos& arg);
	int32_t operator()(Processors::OperationReplace& arg);
	int32_t operator()(Processors::ActivateEffect& arg);
	int32_t operator()(Processors::SummonRule& arg);
	int32_t operator()(Processors::SpSummonRule& arg);
	int32_t operator()(Processors::SpSummon& arg);
	int32_t operator()(Processors::FlipSummon& arg);
	int32_t operator()(Processors::MonsterSet& arg);
	int32_t operator()(Processors::SpellSet& arg);
	int32_t operator()(Processors::SpSummonStep& arg);
	int32_t operator()(Processors::SpellSetGroup& arg);
	int32_t operator()(Processors::SpSummonRuleGroup& arg);
	int32_t operator()(Processors::Draw& arg);
	int32_t operator()(Processors::Damage& arg);
	int32_t operator()(Processors::Recover& arg);
	int32_t operator()(Processors::Equip& arg);
	int32_t operator()(Processors::GetControl& arg);
	int32_t operator()(Processors::SwapControl& arg);
	int32_t operator()(Processors::ControlAdjust& arg);
	int32_t operator()(Processors::SelfDestroyUnique& arg);
	int32_t operator()(Processors::SelfDestroy& arg);
	int32_t operator()(Processors::SelfToGrave& arg);
	int32_t operator()(Processors::TrapMonsterAdjust& arg);
	int32_t operator()(Processors::PayLPCost& arg);
	int32_t operator()(Processors::RemoveCounter& arg);
	int32_t operator()(Processors::AttackDisable& arg);
	int32_t operator()(Processors::AnnounceRace& arg);
	int32_t operator()(Processors::AnnounceAttribute& arg);
	int32_t operator()(Processors::AnnounceCard& arg);
	int32_t operator()(Processors::AnnounceNumber& arg);
	int32_t operator()(Processors::TossCoin& arg);
	int32_t operator()(Processors::TossDice& arg);
	int32_t operator()(Processors::RockPaperScissors& arg);
	int32_t operator()(Processors::SelectFusion& arg);
	int32_t operator()(Processors::DiscardHand& arg);
	int32_t operator()(Processors::DiscardDeck& arg);
	int32_t operator()(Processors::SortDeck& arg);
	int32_t operator()(Processors::RemoveOverlay& arg);
	int32_t operator()(Processors::XyzOverlay& arg);
	int32_t operator()(Processors::RefreshRelay& arg);
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

enum class CHAININFO {
	TRIGGERING_EFFECT = 1,
	TRIGGERING_PLAYER,
	TRIGGERING_CONTROLER,
	TRIGGERING_LOCATION,
	TRIGGERING_LOCATION_SYMBOLIC,
	TRIGGERING_SEQUENCE,
	TRIGGERING_SEQUENCE_SYMBOLIC,
	TARGET_CARDS,
	TARGET_PLAYER,
	TARGET_PARAM,
	DISABLE_REASON,
	DISABLE_PLAYER,
	CHAIN_ID,
	TYPE,
	EXTTYPE,
	TRIGGERING_POSITION,
	TRIGGERING_CODE,
	TRIGGERING_CODE2,
	TRIGGERING_TYPE,
	TRIGGERING_LEVEL,
	TRIGGERING_RANK,
	TRIGGERING_ATTRIBUTE,
	TRIGGERING_RACE,
	TRIGGERING_ATTACK,
	TRIGGERING_DEFENSE,
	TRIGGERING_STATUS,
	TRIGGERING_SUMMON_LOCATION,
	TRIGGERING_SUMMON_TYPE,
	TRIGGERING_SUMMON_PROC_COMPLETE,
	TRIGGERING_SETCODES,
};

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

#endif /* FIELD_H_ */
