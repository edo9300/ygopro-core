/*
 * Copyright (c) 2023-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef PROCESSOR_UNIT_H_
#define PROCESSOR_UNIT_H_

#include <cstdint>
#include <memory> //std::unique_ptr
#include <type_traits> //std::false_type, std::true_type
#include <variant>
#include "containers_fwd.h"

namespace P {
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
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	SelectBattleCmd(int16_t step_, uint8_t playerid_) : step(step_), playerid(playerid_) {}
};
struct SelectIdleCmd {
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	SelectIdleCmd(int16_t step_, uint8_t playerid_) : step(step_), playerid(playerid_) {}
};
struct SelectEffectYesNo {
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	card* pcard;
	uint64_t description;
	SelectEffectYesNo(int16_t step_, uint8_t playerid_, uint64_t description_, card* pcard_) :
		step(step_), playerid(playerid_), pcard(pcard_), description(description_) {}
};
struct SelectYesNo {
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	uint64_t description;
	SelectYesNo(int16_t step_, uint8_t playerid_, uint64_t description_) :
		step(step_), playerid(playerid_), description(description_) {}
};
struct SelectOption {
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	SelectOption(int16_t step_, uint8_t playerid_) : step(step_), playerid(playerid_) {}
};
struct SelectCard {
	static constexpr auto needs_answer = true;
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
	static constexpr auto needs_answer = true;
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
	static constexpr auto needs_answer = true;
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
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	uint8_t spe_count;
	bool forced;
	SelectChain(int16_t step_, uint8_t playerid_, uint8_t spe_count_, bool forced_) :
		step(step_), playerid(playerid_), spe_count(spe_count_), forced(forced_) {}
};
struct SelectPlace {
	static constexpr auto needs_answer = true;
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
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	uint8_t positions;
	uint32_t code;
	SelectPosition(int16_t step_, uint8_t playerid_, uint32_t code_, uint8_t positions_) :
		step(step_), playerid(playerid_), positions(positions_), code(code_) {}
};
struct SelectTributeP {
	static constexpr auto needs_answer = true;
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
	static constexpr auto needs_answer = true;
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
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	int32_t acc;
	int32_t min;
	int32_t max;
	SelectSum(int16_t step_, uint8_t playerid_, int32_t acc_, int32_t min_, int32_t max_) :
		step(step_), playerid(playerid_), acc(acc_), min(min_), max(max_) {}
};
struct SortCard {
	static constexpr auto needs_answer = true;
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
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	uint8_t count;
	uint64_t available;
	AnnounceRace(uint16_t step_, uint8_t playerid_, uint8_t count_, uint64_t available_) :
		step(step_), playerid(playerid_), count(count_), available(available_) {}
};
struct AnnounceAttribute {
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	uint8_t count;
	uint32_t available;
	AnnounceAttribute(uint16_t step_, uint8_t playerid_, uint8_t count_, uint32_t available_) :
		step(step_), playerid(playerid_), count(count_), available(available_) {}
};
struct AnnounceCard {
	static constexpr auto needs_answer = true;
	int16_t step;
	uint8_t playerid;
	AnnounceCard(uint16_t step_, uint8_t playerid_) :
		step(step_), playerid(playerid_) {}
};
struct AnnounceNumber {
	static constexpr auto needs_answer = true;
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
	static constexpr auto needs_answer = true;
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

using processor_unit = std::variant<Adjust, Turn, RefreshLoc, Startup,
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

template <typename T, typename = void>
struct NeedsAnswer : std::false_type {};

template <typename T>
struct NeedsAnswer <T, std::void_t<decltype(T::needs_answer)>> : std::true_type {};

}

namespace Processors = P;

#endif /* PROCESSOR_UNIT_H_ */
