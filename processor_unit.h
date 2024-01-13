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
#include "common.h"
#include "containers_fwd.h"

namespace P {
template<bool NA>
struct Process {
	static constexpr auto needs_answer = NA;
	Process(const Process&) = delete; // non construction-copyable
	Process& operator=(const Process&) = delete; // non copyable
	Process(Process&&) = default; // construction-movable
	Process& operator=(Process&&) = default; // movable
	int16_t step;
protected:
	explicit Process(uint16_t step_) : step(step_) {}
};
struct Adjust : public Process<false> {
	Adjust(uint16_t step_) : Process(step_) {}
};
struct Turn : public Process<false> {
	uint8_t turn_player;
	bool has_performed_second_battle_phase;
	Turn(uint16_t step_, uint8_t turn_player_) : Process(step_), turn_player(turn_player_),
		has_performed_second_battle_phase(false) {}
};
struct RefreshLoc : public Process<false> {
	uint8_t dis_count;
	uint32_t previously_disabled_locations;
	effect* current_disable_field_effect;
	RefreshLoc(uint16_t step_) :
		Process(step_), dis_count(0), previously_disabled_locations(0),
		current_disable_field_effect(nullptr) {}
};
struct Startup : public Process<false> {
	Startup(uint16_t step_) : Process(step_) {}
};
struct SelectBattleCmd : public Process<true> {
	uint8_t playerid;
	SelectBattleCmd(uint16_t step_, uint8_t playerid_) : Process(step_), playerid(playerid_) {}
};
struct SelectIdleCmd : public Process<true> {
	uint8_t playerid;
	SelectIdleCmd(uint16_t step_, uint8_t playerid_) : Process(step_), playerid(playerid_) {}
};
struct SelectEffectYesNo : public Process<true> {
	uint8_t playerid;
	card* pcard;
	uint64_t description;
	SelectEffectYesNo(uint16_t step_, uint8_t playerid_, uint64_t description_, card* pcard_) :
		Process(step_), playerid(playerid_), pcard(pcard_), description(description_) {}
};
struct SelectYesNo : public Process<true> {
	uint8_t playerid;
	uint64_t description;
	SelectYesNo(uint16_t step_, uint8_t playerid_, uint64_t description_) :
		Process(step_), playerid(playerid_), description(description_) {}
};
struct SelectOption : public Process<true> {
	uint8_t playerid;
	SelectOption(uint16_t step_, uint8_t playerid_) : Process(step_), playerid(playerid_) {}
};
struct SelectCard : public Process<true> {
	uint8_t playerid;
	bool cancelable;
	uint8_t min;
	uint8_t max;
	SelectCard(uint16_t step_, uint8_t playerid_, bool cancelable_,
			   uint8_t min_, uint8_t max_) :
		Process(step_), playerid(playerid_), cancelable(cancelable_), min(min_), max(max_) {}
};
struct SelectCardCodes : public Process<true> {
	uint8_t playerid;
	bool cancelable;
	uint8_t min;
	uint8_t max;
	SelectCardCodes(uint16_t step_, uint8_t playerid_, bool cancelable_,
					uint8_t min_, uint8_t max_) :
		Process(step_), playerid(playerid_), cancelable(cancelable_), min(min_), max(max_) {}
};
struct SelectUnselectCard : public Process<true> {
	uint8_t playerid;
	bool cancelable;
	uint8_t min;
	uint8_t max;
	bool finishable;
	SelectUnselectCard(uint16_t step_, uint8_t playerid_, bool cancelable_,
					   uint8_t min_, uint8_t max_, bool finishable_) :
		Process(step_), playerid(playerid_), cancelable(cancelable_), min(min_), max(max_),
		finishable(finishable_) {}
};
struct SelectChain : public Process<true> {
	uint8_t playerid;
	uint8_t spe_count;
	bool forced;
	SelectChain(uint16_t step_, uint8_t playerid_, uint8_t spe_count_, bool forced_) :
		Process(step_), playerid(playerid_), spe_count(spe_count_), forced(forced_) {}
};
struct SelectPlace : public Process<true> {
	uint8_t playerid;
	uint8_t count;
	uint32_t flag;
	bool disable_field;
	SelectPlace(uint16_t step_, uint8_t playerid_, uint32_t flag_, uint8_t count_) :
		Process(step_), playerid(playerid_), count(count_), flag(flag_), disable_field(false) {}
};
struct SelectDisField : SelectPlace {
	SelectDisField(uint16_t step_, uint8_t playerid_, uint32_t flag_, uint8_t count_) :
		SelectPlace(step_, playerid_, flag_, count_) {
		disable_field = true;
	}
};
struct SelectPosition : public Process<true> {
	uint8_t playerid;
	uint8_t positions;
	uint32_t code;
	SelectPosition(uint16_t step_, uint8_t playerid_, uint32_t code_, uint8_t positions_) :
		Process(step_), playerid(playerid_), positions(positions_), code(code_) {}
};
struct SelectTributeP : public Process<true> {
	uint8_t playerid;
	bool cancelable;
	uint8_t min;
	uint8_t max;
	SelectTributeP(uint16_t step_, uint8_t playerid_, bool cancelable_, uint8_t min_, uint8_t max_) :
		Process(step_), playerid(playerid_), cancelable(cancelable_), min(min_), max(max_) {}
};
struct SortChain : public Process<false> {
	uint8_t playerid;
	SortChain(uint16_t step_, uint8_t playerid_) : Process(step_), playerid(playerid_) {}
};
struct SelectCounter : public Process<true> {
	uint16_t countertype;
	uint16_t count;
	uint8_t playerid;
	uint8_t self;
	uint8_t oppo;
	SelectCounter(uint16_t step_, uint8_t playerid_, uint16_t countertype_, uint16_t count_,
				  uint8_t self_, uint8_t oppo_) :
		Process(step_), countertype(countertype_), count(count_), playerid(playerid_), self(self_),
		oppo(oppo_) {}
};
struct SelectSum : public Process<true> {
	uint8_t playerid;
	int32_t acc;
	int32_t min;
	int32_t max;
	SelectSum(uint16_t step_, uint8_t playerid_, int32_t acc_, int32_t min_, int32_t max_) :
		Process(step_), playerid(playerid_), acc(acc_), min(min_), max(max_) {}
};
struct SortCard : public Process<true> {
	uint8_t playerid;
	bool is_chain;
	SortCard(uint16_t step_, uint8_t playerid_, bool is_chain_) :
		Process(step_), playerid(playerid_), is_chain(is_chain_) {}
};
struct SelectRelease : public Process<false> {
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
	SelectRelease(uint16_t step_, uint8_t playerid_, bool cancelable_, uint16_t min_,
				  uint16_t max_, bool check_field_, card* to_check_, uint8_t toplayer_,
				  uint8_t zone_) :
		Process(step_), min(min_), max(max_), playerid(playerid_), cancelable(cancelable_),
		check_field(check_field_), toplayer(toplayer_), zone(zone_), to_check(to_check_),
		extra_release_nonsum_effect(nullptr), must_choose_one(nullptr) {}
};
struct SelectTribute : public Process<false> {
	uint16_t min;
	uint16_t max;
	uint8_t playerid;
	bool cancelable;
	uint8_t toplayer;
	uint8_t zone;
	card* target;
	effect* extra_release_effect;
	card_set must_choose_one;
	SelectTribute(uint16_t step_, card* target_, uint8_t playerid_, bool cancelable_, int32_t min_,
				  int32_t max_, uint8_t toplayer_, uint8_t zone_) :
		Process(step_), min(min_), max(max_), playerid(playerid_), cancelable(cancelable_),
		toplayer(toplayer_), zone(zone_), target(target_), extra_release_effect(nullptr) {}
};
struct PointEvent : public Process<false> {
	bool skip_trigger;
	bool skip_freechain;
	bool skip_new;
	PointEvent(uint16_t step_, bool skip_trigger_, bool skip_freechain_, bool skip_new_) :
		Process(step_), skip_trigger(skip_trigger_), skip_freechain(skip_freechain_), skip_new(skip_new_) {}
};
struct QuickEffect : public Process<false> {
	bool skip_freechain;
	bool is_opponent;
	uint8_t priority_player;
	QuickEffect(uint16_t step_, bool skip_freechain_, uint8_t priority_player_) :
		Process(step_), skip_freechain(skip_freechain_), is_opponent(false), priority_player(priority_player_) {}
};
struct IdleCommand : public Process<false> {
	uint8_t phase_to_change_to;
	card* card_to_reposition;
	IdleCommand(uint16_t step_) :
		Process(step_), phase_to_change_to(0), card_to_reposition(nullptr) {}
};
struct PhaseEvent : public Process<false> {
	uint16_t phase;
	bool is_opponent;
	bool priority_passed;
	PhaseEvent(uint16_t step_, uint16_t phase_) :
		Process(step_), phase(phase_), is_opponent(false), priority_passed(false) {}
};
struct BattleCommand : public Process<false> {
	uint16_t phase_to_change_to;
	bool is_replaying_attack;
	bool attack_announce_failed;
	bool repeat_battle_phase;
	bool second_battle_phase_is_optional;
	bool previous_point_event_had_any_trigger_to_resolve;
	uint8_t reason_player;
	effect* damage_change_effect;
	group* cards_destroyed_by_battle;
	card* reason_card;
	BattleCommand(uint16_t step_, group* cards_destroyed_by_battle_ = nullptr) :
		Process(step_), phase_to_change_to(0), is_replaying_attack(false), attack_announce_failed(false),
		repeat_battle_phase(false), second_battle_phase_is_optional(false),
		previous_point_event_had_any_trigger_to_resolve(false), reason_player(PLAYER_NONE), damage_change_effect(nullptr),
		cards_destroyed_by_battle(cards_destroyed_by_battle_), reason_card(nullptr) {}
};
struct DamageStep : public Process<false> {
	uint16_t backup_phase;
	bool new_attack;
	card* attacker;
	card* attack_target;
	group* cards_destroyed_by_battle;
	DamageStep(uint16_t step_, card* attacker_, card* attack_target_, bool new_attack_) :
		Process(step_), backup_phase(0), new_attack(new_attack_), attacker(attacker_), attack_target(attack_target_), cards_destroyed_by_battle(nullptr) {}
};
struct ForcedBattle : public Process<false> {
	uint16_t backup_phase;
	ForcedBattle(uint16_t step_) :
		Process(step_), backup_phase(0) {}
};
struct AddChain : public Process<false> {
	bool is_activated_effect;
	AddChain(uint16_t step_) :
		Process(step_), is_activated_effect(false) {}
};
struct SolveChain : public Process<false> {
	bool skip_trigger;
	bool skip_freechain;
	bool skip_new;
	int32_t backed_up_operation;
	SolveChain(uint16_t step_, bool skip_trigger_, bool skip_freechain_, bool skip_new_) :
		Process(step_), skip_trigger(skip_trigger_), skip_freechain(skip_freechain_),
		skip_new(skip_new_), backed_up_operation(0) {}
};
struct SolveContinuous : public Process<false> {
	uint8_t reason_player;
	effect* reason_effect;
	SolveContinuous(uint16_t step_) :
		Process(step_), reason_player(0), reason_effect(nullptr) {}
};
struct ExecuteCost : public Process<false> {
	uint8_t triggering_player;
	bool shuffle_check_was_disabled;
	effect* triggering_effect;
	ExecuteCost(uint16_t step_, effect* triggering_effect_, uint8_t triggering_player_) :
		Process(step_), triggering_player(triggering_player_), shuffle_check_was_disabled(false),
		triggering_effect(triggering_effect_) {}
};
struct ExecuteOperation : public Process<false> {
	uint8_t triggering_player;
	bool shuffle_check_was_disabled;
	effect* triggering_effect;
	ExecuteOperation(uint16_t step_, effect* triggering_effect_, uint8_t triggering_player_) :
		Process(step_), triggering_player(triggering_player_), shuffle_check_was_disabled(false),
		triggering_effect(triggering_effect_) {}
};
struct ExecuteTarget : public Process<false> {
	uint8_t triggering_player;
	bool shuffle_check_was_disabled;
	effect* triggering_effect;
	ExecuteTarget(uint16_t step_, effect* triggering_effect_, uint8_t triggering_player_) :
		Process(step_), triggering_player(triggering_player_), shuffle_check_was_disabled(false),
		triggering_effect(triggering_effect_) {}
};
struct Destroy : public Process<false> {
	uint8_t reason_player;
	uint32_t reason;
	group* targets;
	effect* reason_effect;
	Destroy(uint16_t step_, group* targets_, effect* reason_effect_, uint32_t reason_,
					 uint8_t reason_player_) :
		Process(step_), reason_player(reason_player_), reason(reason_), targets(targets_),
		reason_effect(reason_effect_) {}
};
struct Release : public Process<false> {
	uint8_t reason_player;
	uint32_t reason;
	group* targets;
	effect* reason_effect;
	Release(uint16_t step_, group* targets_, effect* reason_effect_, uint32_t reason_,
					 uint8_t reason_player_) :
		Process(step_), reason_player(reason_player_), reason(reason_), targets(targets_),
		reason_effect(reason_effect_) {}
};
struct SendTo : public Process<false> {
	struct exargs {
		card_set leave_field, leave_grave, detach;
		bool show_decktop[2];
		card_vector cv;
		card_vector::iterator cvit;
		effect* predirect;
	};
	uint8_t reason_player;
	uint32_t reason;
	group* targets;
	effect* reason_effect;
	std::unique_ptr<exargs> extra_args;
	SendTo(uint16_t step_, group* targets_, effect* reason_effect_, uint32_t reason_,
					uint8_t reason_player_) :
		Process(step_), reason_player(reason_player_), reason(reason_), targets(targets_),
		reason_effect(reason_effect_), extra_args(nullptr) {}
};
struct DestroyReplace : public Process<false> {
	bool battle;
	group* targets;
	card* target;
	DestroyReplace(uint16_t step_, group* targets_, card* target_, bool battle_) :
		Process(step_), battle(battle_), targets(targets_), target(target_) {}
};
struct ReleaseReplace : public Process<false> {
	group* targets;
	card* target;
	ReleaseReplace(uint16_t step_, group* targets_, card* target_) :
		Process(step_), targets(targets_), target(target_) {}
};
struct SendToReplace : public Process<false> {
	group* targets;
	card* target;
	SendToReplace(uint16_t step_, group* targets_, card* target_) :
		Process(step_), targets(targets_), target(target_) {}
};
struct MoveToField : public Process<false> {
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
		Process(step_), enable(enable_), ret(ret_) , pzone(pzone_) , zone(zone_) , rule(rule_) ,
		location_reason(location_reason_) , confirm(confirm_), target(target_) {}
};
struct ChangePos : public Process<false> {
	uint8_t reason_player;
	bool enable;
	bool oppo_selection;
	effect* reason_effect;
	group* targets;
	card_set to_grave_set;
	ChangePos(uint16_t step_, group* targets_, effect* reason_effect_,
					   uint8_t reason_player_, bool enable_) :
		Process(step_), reason_player(reason_player_), enable(enable_), oppo_selection(false),
		reason_effect(reason_effect_), targets(targets_) {}
};
struct OperationReplace : public Process<false> {
	bool is_destroy;
	effect* replace_effect;
	group* targets;
	card* target;
	OperationReplace(uint16_t step_, effect* replace_effect_, group* targets_,
							  card* target_, bool is_destroy_) :
		Process(step_), is_destroy(is_destroy_), replace_effect(replace_effect_),
		targets(targets_), target(target_) {}
};
struct ActivateEffect : public Process<false> {
	effect* peffect;
	ActivateEffect(uint16_t step_, effect* peffect_) :
		Process(step_), peffect(peffect_) {}
};
struct SummonRule : public Process<false> {
	uint8_t sumplayer;
	uint8_t min_tribute;
	uint8_t max_allowed_tributes;
	bool ignore_count;
	uint32_t zone;
	card* target;
	effect* summon_procedure_effect;
	effect* extra_summon_effect;
	card_set tributes;
	effect_set summon_cost_effects;
	SummonRule(uint16_t step_, uint8_t sumplayer_, card* target_, effect* proc_,
						bool ignore_count_, uint8_t min_tribute_, uint32_t zone_) :
		Process(step_), sumplayer(sumplayer_), min_tribute(min_tribute_), max_allowed_tributes(0),
		ignore_count(ignore_count_), zone(zone_), target(target_), summon_procedure_effect(proc_),
		extra_summon_effect(nullptr) {}
};
struct SpSummonRule : public Process<false> {
	uint8_t sumplayer;
	bool is_mid_chain;
	uint32_t summon_type;
	card* target;
	effect* summon_proc_effect;
	group* cards_to_summon_g;
	effect_set spsummon_cost_effects;
	SpSummonRule(uint16_t step_, uint8_t sumplayer_, card* target_, uint32_t summon_type_, bool is_mid_chain_ = false) :
		Process(step_), sumplayer(sumplayer_), is_mid_chain(is_mid_chain_), summon_type(summon_type_), target(target_),
		summon_proc_effect(nullptr), cards_to_summon_g(nullptr) {}
};
struct SpSummon : public Process<false> {
	uint8_t reason_player;
	uint32_t zone;
	effect* reason_effect;
	group* targets;
	SpSummon(uint16_t step_, effect* reason_effect_, uint8_t reason_player_,
					  group* targets_, uint32_t zone_) :
		Process(step_), reason_player(reason_player_), zone(zone_), reason_effect(reason_effect_),
		targets(targets_) {}
};
struct FlipSummon : public Process<false> {
	uint8_t sumplayer;
	card* target;
	effect_set flip_summon_cost_effects;
	FlipSummon(uint16_t step_, uint8_t sumplayer_, card* target_) :
		Process(step_), sumplayer(sumplayer_), target(target_) {}
};
struct MonsterSet : public Process<false> {
	uint8_t setplayer;
	uint8_t min_tribute;
	uint8_t max_allowed_tributes;
	bool ignore_count;
	uint32_t zone;
	card* target;
	effect* summon_procedure_effect;
	effect* extra_summon_effect;
	card_set tributes;
	MonsterSet(uint16_t step_, uint8_t setplayer_, card* target_, effect* proc_,
						bool ignore_count_, uint8_t min_tribute_, uint32_t zone_) :
		Process(step_), setplayer(setplayer_), min_tribute(min_tribute_), max_allowed_tributes(0), ignore_count(ignore_count_),
		zone(zone_), target(target_), summon_procedure_effect(proc_), extra_summon_effect(nullptr) {}
};
struct SpellSet : public Process<false> {
	uint8_t setplayer;
	uint8_t toplayer;
	card* target;
	effect* reason_effect;
	SpellSet(uint16_t step_, uint8_t setplayer_, uint8_t toplayer_, card* target_,
					  effect* reason_effect_) :
		Process(step_), setplayer(setplayer_), toplayer(toplayer_), target(target_),
		reason_effect(reason_effect_) {}
};
struct SpSummonStep : public Process<false> {
	uint32_t zone;
	group* targets;
	card* target;
	effect_set spsummon_cost_effects;
	SpSummonStep(uint16_t step_, group* targets_, card* target_, uint32_t zone_) :
		Process(step_), zone(zone_), targets(targets_), target(target_) {}
};
struct SpellSetGroup : public Process<false> {
	uint8_t setplayer;
	uint8_t toplayer;
	bool confirm;
	group* ptarget;
	effect* reason_effect;
	SpellSetGroup(uint16_t step_, uint8_t setplayer_, uint8_t toplayer_,
						   group* ptarget_, bool confirm_, effect* reason_effect_) :
		Process(step_), setplayer(setplayer_), toplayer(toplayer_), confirm(confirm_),
		ptarget(ptarget_), reason_effect(reason_effect_) {}
};
struct SpSummonRuleGroup : public Process<false> {
	uint8_t sumplayer;
	uint32_t summon_type;
	SpSummonRuleGroup(uint16_t step_, uint8_t sumplayer_, uint32_t summon_type_) :
		Process(step_), sumplayer(sumplayer_), summon_type(summon_type_) {}
};
struct Draw : public Process<false> {
	uint16_t count;
	uint8_t reason_player;
	uint8_t playerid;
	uint32_t reason;
	effect* reason_effect;
	card_set drawn_set;
	Draw(uint16_t step_, effect* reason_effect_, uint32_t reason_, uint8_t reason_player_,
				  uint8_t playerid_, uint16_t count_) :
		Process(step_), count(count_), reason_player(reason_player_), playerid(playerid_),
		reason(reason_), reason_effect(reason_effect_) {}
};
struct Damage : public Process<false> {
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
		Process(step_), reason_player(reason_player_), playerid(playerid_), is_step(is_step_), is_reflected(false),
		amount(amount_), reason(reason_), reason_card(reason_card_), reason_effect(reason_effect_) {}
};
struct Recover : public Process<false> {
	uint8_t reason_player;
	uint8_t playerid;
	bool is_step;
	uint32_t amount;
	uint32_t reason;
	effect* reason_effect;
	Recover(uint16_t step_, effect* reason_effect_, uint32_t reason_, uint8_t reason_player_,
					uint8_t playerid_, uint32_t amount_, bool is_step_) :
		Process(step_), reason_player(reason_player_), playerid(playerid_), is_step(is_step_), amount(amount_),
		reason(reason_), reason_effect(reason_effect_) {}
};
struct Equip : public Process<false> {
	uint8_t equip_player;
	bool is_step;
	bool faceup;
	card* equip_card;
	card* target;
	Equip(uint16_t step_, uint8_t equip_player_, card* equip_card_, card* target_,
				   bool faceup_, bool is_step_) :
		Process(step_), equip_player(equip_player_), is_step(is_step_), faceup(faceup_),
		equip_card(equip_card_), target(target_) {}
};
struct GetControl : public Process<false> {
	uint8_t chose_player;
	uint8_t playerid;
	uint8_t reset_count;
	uint16_t reset_phase;
	uint32_t zone;
	effect* reason_effect;
	group* targets;
	card_set destroy_set;
	GetControl(uint16_t step_, effect* reason_effect_, uint8_t chose_player_, group* targets_,
						uint8_t playerid_, uint16_t reset_phase_, uint8_t reset_count_, uint32_t zone_) :
		Process(step_), chose_player(chose_player_), playerid(playerid_), reset_count(reset_count_),
		reset_phase(reset_phase_), zone(zone_), reason_effect(reason_effect_), targets(targets_) {}
};
struct SwapControl : public Process<false> {
	uint8_t reset_count;
	uint8_t reason_player;
	uint8_t self_selected_sequence;
	uint16_t reset_phase;
	effect* reason_effect;
	group* targets1;
	group* targets2;
	SwapControl(uint16_t step_, effect* reason_effect_, uint8_t reason_player_,
						 group* targets1_, group* targets2_, uint16_t reset_phase_, uint8_t reset_count_) :
		Process(step_), reset_count(reset_count_), reason_player(reason_player_), self_selected_sequence(0),
		reset_phase(reset_phase_), reason_effect(reason_effect_), targets1(targets1_), targets2(targets2_) {}
};
struct ControlAdjust : public Process<false> {
	uint8_t adjusting_player;
	card_set destroy_set;
	card_set adjust_set;
	ControlAdjust(uint16_t step_) :
		Process(step_), adjusting_player(0) {}
};
struct SelfDestroyUnique : public Process<false> {
	uint8_t playerid;
	card* unique_card;
	SelfDestroyUnique(uint16_t step_, card* unique_card_, uint8_t playerid_) :
		Process(step_), playerid(playerid_), unique_card(unique_card_) {}
};
struct SelfDestroy : public Process<false> {
	SelfDestroy(uint16_t step_) :
		Process(step_) {}
};
struct SelfToGrave : public Process<false> {
	SelfToGrave(uint16_t step_) :
		Process(step_) {}
};
struct TrapMonsterAdjust : public Process<false> {
	bool oppo_selection;
	card_set to_grave_set;
	TrapMonsterAdjust(uint16_t step_) :
		Process(step_), oppo_selection(false) {}
};
struct PayLPCost : public Process<false> {
	uint8_t playerid;
	uint32_t cost;
	PayLPCost(uint16_t step_, uint8_t playerid_, uint32_t cost_) :
		Process(step_), playerid(playerid_), cost(cost_) {}
};
struct RemoveCounter : public Process<false> {
	uint8_t rplayer;
	uint8_t self;
	uint8_t oppo;
	uint16_t countertype;
	uint16_t count;
	uint32_t reason;
	card* pcard;
	RemoveCounter(uint16_t step_, uint32_t reason_, card* pcard_, uint8_t rplayer_, uint8_t self_,
						   uint8_t oppo_, uint16_t countertype_, uint16_t count_) :
		Process(step_), rplayer(rplayer_), self(self_), oppo(oppo_), countertype(countertype_), count(count_),
		reason(reason_), pcard(pcard_) {}
};
struct AttackDisable : public Process<false> {
	AttackDisable(uint16_t step_) :
		Process(step_) {}
};
struct AnnounceRace : public Process<true> {
	uint8_t playerid;
	uint8_t count;
	uint64_t available;
	AnnounceRace(uint16_t step_, uint8_t playerid_, uint8_t count_, uint64_t available_) :
		Process(step_), playerid(playerid_), count(count_), available(available_) {}
};
struct AnnounceAttribute : public Process<true> {
	uint8_t playerid;
	uint8_t count;
	uint32_t available;
	AnnounceAttribute(uint16_t step_, uint8_t playerid_, uint8_t count_, uint32_t available_) :
		Process(step_), playerid(playerid_), count(count_), available(available_) {}
};
struct AnnounceCard : public Process<true> {
	uint8_t playerid;
	AnnounceCard(uint16_t step_, uint8_t playerid_) :
		Process(step_), playerid(playerid_) {}
};
struct AnnounceNumber : public Process<true> {
	uint8_t playerid;
	AnnounceNumber(uint16_t step_, uint8_t playerid_) :
		Process(step_), playerid(playerid_) {}
};
struct TossCoin : public Process<false> {
	uint8_t playerid;
	uint8_t reason_player;
	uint8_t count;
	effect* reason_effect;
	TossCoin(uint16_t step_, effect* reason_effect_, uint8_t reason_player_, uint8_t playerid_,
					  uint8_t count_) :
		Process(step_), playerid(playerid_), reason_player(reason_player_), count(count_),
		reason_effect(reason_effect_) {}
};
struct TossDice : public Process<false> {
	uint8_t playerid;
	uint8_t reason_player;
	uint8_t count1;
	uint8_t count2;
	effect* reason_effect;
	TossDice(uint16_t step_, effect* reason_effect_, uint8_t reason_player_, uint8_t playerid_,
					  uint8_t count1_, uint8_t count2_) :
		Process(step_), playerid(playerid_), reason_player(reason_player_), count1(count1_), count2(count2_),
		reason_effect(reason_effect_) {}
};
struct RockPaperScissors : public Process<true> {
	bool repeat;
	uint8_t hand0;
	RockPaperScissors(uint16_t step_, bool repeat_) :
		Process(step_), repeat(repeat_), hand0(0) {}
};
struct SelectFusion : public Process<false> {
	uint8_t playerid;
	uint32_t chkf;
	group* fusion_materials;
	group* forced_materials;
	card* pcard;
	SelectFusion(uint16_t step_, uint8_t playerid_, group* fusion_materials_, uint32_t chkf_,
						  group* forced_materials_, card* pcard_) :
		Process(step_), playerid(playerid_), chkf(chkf_), fusion_materials(fusion_materials_),
		forced_materials(forced_materials_), pcard(pcard_) {}
};
struct DiscardHand : public Process<false> {
	uint8_t playerid;
	uint8_t min;
	uint8_t max;
	uint32_t reason;
	DiscardHand(uint16_t step_, uint8_t playerid_, uint8_t min_, uint8_t max_, uint32_t reason_) :
		Process(step_), playerid(playerid_), min(min_), max(max_), reason(reason_) {}
};
struct DiscardDeck : public Process<false> {
	uint16_t count;
	uint8_t playerid;
	uint32_t reason;
	DiscardDeck(uint16_t step_, uint8_t playerid_, uint16_t count_, uint32_t reason_) :
		Process(step_), count(count_), playerid(playerid_), reason(reason_) {}
};
struct SortDeck : public Process<false> {
	uint16_t count;
	uint8_t sort_player;
	uint8_t target_player;
	bool bottom;
	SortDeck(uint16_t step_, uint8_t sort_player_, uint8_t target_player_, uint16_t count_,
					  bool bottom_) :
		Process(step_), count(count_), sort_player(sort_player_), target_player(target_player_),
		bottom(bottom_) {}
};
struct RemoveOverlay : public Process<false> {
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
		Process(step_), min(min_), max(max_), replaced_amount(0),
		has_used_overlay_remove_replace_effect(false), rplayer(rplayer_), self(self_),
		oppo(oppo_), reason(reason_), pgroup(pgroup_) {}
};
struct XyzOverlay : public Process<false> {
	bool send_materials_to_grave;
	card* target;
	group* materials;
	XyzOverlay(uint16_t step_, card* target_, group* materials_, bool send_materials_to_grave_) :
		Process(step_), send_materials_to_grave(send_materials_to_grave_), target(target_),
		materials(materials_) {}
};
struct RefreshRelay : public Process<false> {
	RefreshRelay(uint16_t step_) :
		Process(step_) {}
};

using processors = std::variant<Adjust, Turn, RefreshLoc, Startup,
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

//Ugly hack only to ease debugging the core under visual studio it
//splits the big variant into multiple variants of 30 types max so
//that they can be properly displayed with the visual studio debugger
#if (defined(_DEBUG) && defined(_MSC_VER)) || defined(PROCESSOR_VARIANT_CAP)
#if !defined(PROCESSOR_VARIANT_CAP) || (PROCESSOR_VARIANT_CAP + 0) == 0
static constexpr auto MAX_VARIANT_TYPES = 30;
#else
static constexpr auto MAX_VARIANT_TYPES = PROCESSOR_VARIANT_CAP;
#endif

template <typename T, std::size_t offset, std::size_t amount, typename = std::make_index_sequence<amount>>
struct variant_slice;

template <typename T, std::size_t offset, std::size_t amount, std::size_t... ints>
struct variant_slice<T, offset, amount, std::index_sequence<ints...>> {
	using type = std::variant<std::variant_alternative_t<offset + ints, T>...>;
};

template <typename T, std::size_t offset, std::size_t amount>
using variant_slice_t = typename variant_slice<T, offset, amount>::type;

template <typename T, std::size_t max_variant_size, typename = std::make_index_sequence<std::variant_size_v<T> / max_variant_size>>
struct split_to_subvariants;

template <typename T, std::size_t max_variant_size, std::size_t... ints>
struct split_to_subvariants<T, max_variant_size, std::index_sequence<ints...>> {
private:
	static constexpr auto total = std::variant_size_v<T>;
	static constexpr auto remaining_types = total % max_variant_size;
public:
	using type = std::conditional_t<
		remaining_types != 0,
		std::variant<
		variant_slice_t<T, ints * max_variant_size, max_variant_size>...,
		variant_slice_t<T, total - remaining_types, remaining_types>
		>,
		std::variant<variant_slice_t<T, ints * max_variant_size, max_variant_size>...>
	>;
};

using processor_unit = split_to_subvariants<processors, MAX_VARIANT_TYPES>::type;

template<typename T>
static constexpr inline auto* get_opt_variant(processor_unit& unit) {
	return std::visit([](auto& v) -> T* {
		if constexpr(std::is_constructible_v<std::remove_reference_t<decltype(v)>, T>) {
			return std::get_if<T>(&v);
		}
		return nullptr;
	}, unit);
}

template<typename T, typename... Args>
constexpr inline void emplace_variant(std::list<processor_unit>& list, Args&&... args) {
	T obj(std::forward<Args>(args)...);
	list.push_back(std::move(obj));
}
#else

using processor_unit = processors;

template<typename T>
static constexpr inline auto* get_opt_variant(processor_unit& unit) {
	return std::get_if<T>(&unit);
}

template<typename T, typename... Args>
constexpr inline void emplace_variant(std::list<processor_unit>& list, Args&&... args) {
	list.emplace_back(std::in_place_type<T>, std::forward<Args>(args)...);
}
#endif

template<typename T>
inline constexpr bool NeedsAnswer = T::needs_answer;

}

namespace Processors = P;

#endif /* PROCESSOR_UNIT_H_ */
