/*
 * effect.cpp
 *
 *  Created on: 2010-5-7
 *      Author: Argon
 */

#include "effect.h"
#include "card.h"
#include "duel.h"
#include "group.h"
#include "interpreter.h"

bool effect_sort_id(const effect* e1, const effect* e2) {
	return e1->id < e2->id;
}
bool field_effect::grant_effect_container::effect_sort_by_ref::operator()(effect* e1, effect* e2) const {
	return e1->ref_handle < e2->ref_handle;
}
int32_t effect::is_disable_related() {
	if (code == EFFECT_IMMUNE_EFFECT || code == EFFECT_DISABLE || code == EFFECT_CANNOT_DISABLE || code == EFFECT_FORBIDDEN)
		return TRUE;
	return FALSE;
}
int32_t effect::is_self_destroy_related() {
	if(code == EFFECT_UNIQUE_CHECK || code == EFFECT_SELF_DESTROY || code == EFFECT_SELF_TOGRAVE)
		return TRUE;
	return FALSE;
}
int32_t effect::is_can_be_forbidden() {
	uint32_t ctr = code & 0xf0000;
	if (is_flag(EFFECT_FLAG_CANNOT_DISABLE) && !is_flag(EFFECT_FLAG_CANNOT_NEGATE))
		return FALSE;
	if (code == EFFECT_CHANGE_CODE || ctr == EFFECT_COUNTER_PERMIT || ctr == EFFECT_COUNTER_LIMIT)
		return FALSE;
	return TRUE;
}
// check if a single/field/equip effect is available
// check properties: range, EFFECT_FLAG_OWNER_RELATE, STATUS_BATTLE_DESTROYED, STATUS_EFFECT_ENABLED, disabled/forbidden
// check fucntions: condition
int32_t effect::is_available() {
	if (type & EFFECT_TYPE_ACTIONS)
		return FALSE;
	if ((type & (EFFECT_TYPE_SINGLE | EFFECT_TYPE_XMATERIAL)) && !(type & EFFECT_TYPE_FIELD)) {
		card* phandler = get_handler();
		card* powner = get_owner();
		if (phandler->current.controler == PLAYER_NONE && code != EFFECT_ADD_SETCODE && code != EFFECT_ADD_CODE && code != EFFECT_REVIVE_LIMIT)
			return FALSE;
		if(!is_in_range_of_symbolic_mzone(phandler))
			return FALSE;
		if(is_flag(EFFECT_FLAG_SINGLE_RANGE) && !in_range(phandler))
			return FALSE;
		if(is_flag(EFFECT_FLAG_SINGLE_RANGE) && !phandler->get_status(STATUS_EFFECT_ENABLED) && !is_flag(EFFECT_FLAG_IMMEDIATELY_APPLY))
			return FALSE;
		if(is_flag(EFFECT_FLAG_SINGLE_RANGE) && (phandler->current.location & LOCATION_ONFIELD) && !phandler->is_position(POS_FACEUP))
			return FALSE;
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && is_can_be_forbidden() && powner->is_status(STATUS_FORBIDDEN))
			return FALSE;
		if(powner == phandler && is_can_be_forbidden() && phandler->get_status(STATUS_FORBIDDEN))
			return FALSE;
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && powner->is_status(STATUS_DISABLED))
			return FALSE;
		if(powner == phandler && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && phandler->get_status(STATUS_DISABLED))
			return FALSE;
	}
	if (type & EFFECT_TYPE_EQUIP) {
		if(handler->current.controler == PLAYER_NONE)
			return FALSE;
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && is_can_be_forbidden() && owner->is_status(STATUS_FORBIDDEN))
			return FALSE;
		if(owner == handler && is_can_be_forbidden() && handler->get_status(STATUS_FORBIDDEN))
			return FALSE;
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && owner->is_status(STATUS_DISABLED))
			return FALSE;
		if(owner == handler && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && handler->get_status(STATUS_DISABLED))
			return FALSE;
		if(!is_flag(EFFECT_FLAG_SET_AVAILABLE)) {
			if(!(handler->get_status(STATUS_EFFECT_ENABLED)))
				return FALSE;
			if(!handler->is_position(POS_FACEUP))
				return FALSE;
		}
	}
	if (type & (EFFECT_TYPE_FIELD | EFFECT_TYPE_TARGET)) {
		card* phandler = get_handler();
		card* powner = get_owner();
		if (!is_flag(EFFECT_FLAG_FIELD_ONLY)) {
			if(phandler->current.controler == PLAYER_NONE)
				return FALSE;
			if(!is_in_range_of_symbolic_mzone(phandler) || !in_range(phandler))
				return FALSE;
			if(!phandler->get_status(STATUS_EFFECT_ENABLED) && !is_flag(EFFECT_FLAG_IMMEDIATELY_APPLY))
				return FALSE;
			if((phandler->current.location & LOCATION_ONFIELD) && !phandler->is_position(POS_FACEUP))
				return FALSE;
			if(is_flag(EFFECT_FLAG_OWNER_RELATE) && is_can_be_forbidden() && powner->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(powner == phandler && is_can_be_forbidden() && phandler->get_status(STATUS_FORBIDDEN))
				return FALSE;
			if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && powner->is_status(STATUS_DISABLED))
				return FALSE;
			if(powner == phandler && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && phandler->get_status(STATUS_DISABLED))
				return FALSE;
			if(phandler->is_status(STATUS_BATTLE_DESTROYED))
				return FALSE;
		}
	}
	if (!condition)
		return TRUE;
	pduel->lua->add_param<PARAM_TYPE_EFFECT>(this);
	int32_t res = pduel->lua->check_condition(condition, 1);
	if(res) {
		if(!(status & EFFECT_STATUS_AVAILABLE))
			id = pduel->game_field->infos.field_id++;
		status |= EFFECT_STATUS_AVAILABLE;
	} else
		status &= ~EFFECT_STATUS_AVAILABLE;
	return res;
}
// reset_count: count of effect reset
// count_limit: left count of activation
// count_limit_max: max count of activation
int32_t effect::check_count_limit(uint8_t playerid) const {
	if(is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
		if(count_limit == 0)
			return FALSE;
		if(count_code || count_flag) {
			uint32_t count = count_limit_max;
			if(count_flag & EFFECT_COUNT_CODE_SINGLE) {
				if(pduel->game_field->get_effect_code(get_handler()->fieldid, count_flag, count_hopt_index, PLAYER_NONE) >= count)
					return FALSE;
			} else {
				if(pduel->game_field->get_effect_code(count_code, count_flag, count_hopt_index, playerid) >= count)
					return FALSE;
			}
		}
	}
	return TRUE;
}
// check if an EFFECT_TYPE_ACTIONS effect can be activated
// for triggering effects, it checks EFFECT_FLAG_DAMAGE_STEP, EFFECT_FLAG_SET_AVAILABLE
int32_t effect::is_activateable(uint8_t playerid, const tevent& e, int32_t neglect_cond, int32_t neglect_cost, int32_t neglect_target, int32_t neglect_loc, int32_t neglect_faceup) {
	if(!(type & EFFECT_TYPE_ACTIONS))
		return FALSE;
	if(!check_count_limit(playerid))
		return FALSE;
	if (!is_flag(EFFECT_FLAG_FIELD_ONLY)) {
		if (type & EFFECT_TYPE_ACTIVATE) {
			if((handler->current.controler != playerid) && !(flag[0] & EFFECT_FLAG_BOTH_SIDE))
				return FALSE;
			if(pduel->game_field->check_unique_onfield(handler, playerid, LOCATION_SZONE))
				return FALSE;
			if(!(handler->data.type & TYPE_COUNTER)) {
				if((code < 1132 || code > 1149) && pduel->game_field->infos.phase == PHASE_DAMAGE && !is_flag(EFFECT_FLAG_DAMAGE_STEP)
					&& !pduel->game_field->get_cteffect(this, playerid, FALSE))
					return FALSE;
				if((code < 1134 || code > 1136) && pduel->game_field->infos.phase == PHASE_DAMAGE_CAL && !is_flag(EFFECT_FLAG_DAMAGE_CAL)
					&& !pduel->game_field->get_cteffect(this, playerid, FALSE))
					return FALSE;
			}
			uint32_t zone = 0xff;
			if(!(handler->data.type & (TYPE_FIELD | TYPE_PENDULUM)) && is_flag(EFFECT_FLAG_LIMIT_ZONE)) {
				pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
				pduel->lua->add_param<PARAM_TYPE_GROUP>(e.event_cards );
				pduel->lua->add_param<PARAM_TYPE_INT>(e.event_player);
				pduel->lua->add_param<PARAM_TYPE_INT>(e.event_value);
				pduel->lua->add_param<PARAM_TYPE_EFFECT>(e.reason_effect );
				pduel->lua->add_param<PARAM_TYPE_INT>(e.reason);
				pduel->lua->add_param<PARAM_TYPE_INT>(e.reason_player);
				zone = get_value(7);
				if(!zone)
					return FALSE;
			}
			// additional check for each location
			if(handler->current.location == LOCATION_SZONE) {
				if(handler->is_position(POS_FACEUP))
					return FALSE;
				if(handler->equiping_target)
					return FALSE;
				if(!(handler->data.type & (TYPE_FIELD | TYPE_PENDULUM)) && is_flag(EFFECT_FLAG_LIMIT_ZONE) && !(zone & (1u << handler->current.sequence)))
					return FALSE;
			} else {
				if(!(((handler->data.type & TYPE_FIELD) && (!is_flag(EFFECT_FLAG_LIMIT_ZONE) && value<=0)) || (!is_flag(EFFECT_FLAG_LIMIT_ZONE) && (value & LOCATION_FZONE)) || (!is_flag(EFFECT_FLAG_LIMIT_ZONE) && (value & LOCATION_HAND)))) {
					if (!is_flag(EFFECT_FLAG_LIMIT_ZONE) && (value & LOCATION_MZONE)) {
						if (pduel->game_field->get_useable_count(handler, playerid, LOCATION_MZONE, playerid, LOCATION_REASON_TOFIELD) <= 0)
							return FALSE;
					} else if ((handler->data.type & TYPE_PENDULUM) || (!is_flag(EFFECT_FLAG_LIMIT_ZONE) && (value & LOCATION_PZONE))) {
						if(!pduel->game_field->is_location_useable(playerid, LOCATION_PZONE, 0)
							&& !pduel->game_field->is_location_useable(playerid, LOCATION_PZONE, 1))
						return FALSE;
					} else {
						if (pduel->game_field->get_useable_count(handler, playerid, LOCATION_SZONE, playerid, LOCATION_REASON_TOFIELD, zone) <= 0)
							return FALSE;
					}
				}
			}
			// check activate in hand/in set turn
			int32_t ecode = 0;
			if(handler->current.location == LOCATION_HAND && !neglect_loc) {
				if(handler->data.type & TYPE_TRAP)
					ecode = EFFECT_TRAP_ACT_IN_HAND;
				else if((handler->data.type & TYPE_SPELL) && pduel->game_field->infos.turn_player != playerid) {
					if((handler->data.type & TYPE_QUICKPLAY) || handler->is_affected_by_effect(EFFECT_BECOME_QUICK))
						ecode = EFFECT_QP_ACT_IN_NTPHAND;
					else
						return FALSE;
				}
			} else if(handler->current.location == LOCATION_SZONE) {
				if((handler->data.type & TYPE_TRAP) && handler->get_status(STATUS_SET_TURN))
					ecode = EFFECT_TRAP_ACT_IN_SET_TURN;
				if((handler->data.type & TYPE_SPELL) && (handler->data.type & TYPE_QUICKPLAY || handler->is_affected_by_effect(EFFECT_BECOME_QUICK)) && handler->get_status(STATUS_SET_TURN))
					ecode = EFFECT_QP_ACT_IN_SET_TURN;
			}
			if(ecode) {
				bool available = false;
				effect_set eset;
				handler->filter_effect(ecode, &eset);
				for(const auto& peff : eset) {
					if(peff->check_count_limit(playerid)) {
						available = true;
						break;
					}
				}
				if(!available)
					return FALSE;
			}
			if(handler->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(handler->is_affected_by_effect(EFFECT_CANNOT_TRIGGER))
				return FALSE;
		} else if(!(type & EFFECT_TYPE_CONTINUOUS)) {
			card* phandler = get_handler();
			if(!pduel->game_field->is_flag(DUEL_TRIGGER_WHEN_PRIVATE_KNOWLEDGE) && !(phandler->get_type() & TYPE_MONSTER) && (get_active_type() & TYPE_MONSTER))
				return FALSE;
			if(!is_flag(EFFECT_FLAG2_CONTINUOUS_EQUIP) && (phandler->get_type() & TYPE_CONTINUOUS) && (phandler->get_type() & TYPE_EQUIP))
				return FALSE;
			if(!neglect_faceup && (phandler->current.location & (LOCATION_ONFIELD | LOCATION_REMOVED))) {
				if(!phandler->is_position(POS_FACEUP) && !is_flag(EFFECT_FLAG_SET_AVAILABLE))
					return FALSE;
				if(phandler->is_position(POS_FACEUP) && !phandler->is_status(STATUS_EFFECT_ENABLED))
					return FALSE;
			}
			if(!(type & (EFFECT_TYPE_FLIP | EFFECT_TYPE_TRIGGER_F)) && !((type & EFFECT_TYPE_TRIGGER_O) && (type & EFFECT_TYPE_SINGLE))) {
				if((code < 1132 || code > 1149) && pduel->game_field->infos.phase == PHASE_DAMAGE && !is_flag(EFFECT_FLAG_DAMAGE_STEP))
					return FALSE;
				if((code < 1134 || code > 1136) && pduel->game_field->infos.phase == PHASE_DAMAGE_CAL && !is_flag(EFFECT_FLAG_DAMAGE_CAL))
					return FALSE;
			}
			if(phandler->current.location == LOCATION_OVERLAY)
				return FALSE;
			if((type & EFFECT_TYPE_FIELD) && (phandler->current.controler != playerid) && !is_flag(EFFECT_FLAG_BOTH_SIDE | EFFECT_FLAG_EVENT_PLAYER))
				return FALSE;
			if(!pduel->game_field->is_flag(DUEL_TRIGGER_WHEN_PRIVATE_KNOWLEDGE) && !pduel->game_field->is_flag(DUEL_RETURN_TO_DECK_TRIGGERS) &&
			   (phandler->current.location == LOCATION_DECK
				|| (phandler->current.location == LOCATION_EXTRA && (phandler->current.position & POS_FACEDOWN)))) {
				if((type & EFFECT_TYPE_SINGLE) && code != EVENT_TO_DECK)
					return FALSE;
				if((type & EFFECT_TYPE_FIELD) && !(range & (LOCATION_DECK | LOCATION_EXTRA)))
					return FALSE;
			}
			if(phandler->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(phandler->is_affected_by_effect(EFFECT_CANNOT_TRIGGER))
				return FALSE;
		} else {
			card* phandler = get_handler();
			if((type & EFFECT_TYPE_FIELD) && phandler->is_status(STATUS_BATTLE_DESTROYED))
				return FALSE;
			if(((type & EFFECT_TYPE_FIELD) || ((type & EFFECT_TYPE_SINGLE) && is_flag(EFFECT_FLAG_SINGLE_RANGE))) && (phandler->current.location & LOCATION_ONFIELD)
			        && (!phandler->is_position(POS_FACEUP) || !phandler->is_status(STATUS_EFFECT_ENABLED)))
				return FALSE;
			if((type & EFFECT_TYPE_SINGLE) && is_flag(EFFECT_FLAG_SINGLE_RANGE) && !in_range(phandler))
				return FALSE;
			if(is_flag(EFFECT_FLAG_OWNER_RELATE) && is_can_be_forbidden() && owner->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(phandler == owner && is_can_be_forbidden() && phandler->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && owner->is_status(STATUS_DISABLED))
				return FALSE;
			if(phandler == owner && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && phandler->is_status(STATUS_DISABLED))
				return FALSE;
		}
	} else {
		if((get_owner_player() != playerid) && !is_flag(EFFECT_FLAG_BOTH_SIDE))
			return FALSE;
	}
	pduel->game_field->save_lp_cost();
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = this;
	pduel->game_field->core.reason_player = playerid;
	int32_t result = TRUE;
	if(!(type & EFFECT_TYPE_CONTINUOUS))
		result = is_action_check(playerid);
	if(result)
		result = is_activate_ready(playerid, e, neglect_cond, neglect_cost, neglect_target);
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	pduel->game_field->restore_lp_cost();
	return result;
}
// check functions: value of EFFECT_CANNOT_ACTIVATE, target, cost of EFFECT_ACTIVATE_COST
int32_t effect::is_action_check(uint8_t playerid) {
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, EFFECT_CANNOT_ACTIVATE, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		if(peff->check_value_condition(2))
			return FALSE;
	}
	eset.clear();
	pduel->game_field->filter_player_effect(playerid, EFFECT_ACTIVATE_COST, &eset);
	for(const auto& peff : eset) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		if(!pduel->lua->check_condition(peff->target, 3))
			continue;
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peff);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		if(!pduel->lua->check_condition(peff->cost, 3))
			return FALSE;
	}
	return TRUE;
}
// check functions: condition, cost(chk=0), target(chk=0)
int32_t effect::is_activate_ready(effect* reason_effect, uint8_t playerid, const tevent& e, int32_t neglect_cond, int32_t neglect_cost, int32_t neglect_target) {
	if(!is_in_range_of_symbolic_mzone(reason_effect->get_handler())) {
		return FALSE;
	}
	if(!neglect_cond && condition) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(reason_effect);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_GROUP>(e.event_cards);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.event_player);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.event_value);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(e.reason_effect);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.reason);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.reason_player);
		if(!pduel->lua->check_condition(condition, 8)) {
			return FALSE;
		}
	}
	if(!neglect_cost && cost && !(type & EFFECT_TYPE_CONTINUOUS)) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(reason_effect);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_GROUP>(e.event_cards);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.event_player);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.event_value);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(e.reason_effect);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.reason);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.reason_player);
		pduel->lua->add_param<PARAM_TYPE_INT>(0);
		if(!pduel->lua->check_condition(cost, 9)) {
			return FALSE;
		}
	}
	if(!neglect_target && target) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(reason_effect);
		pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
		pduel->lua->add_param<PARAM_TYPE_GROUP>(e.event_cards);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.event_player);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.event_value);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(e.reason_effect);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.reason);
		pduel->lua->add_param<PARAM_TYPE_INT>(e.reason_player);
		pduel->lua->add_param<PARAM_TYPE_INT>(0);
		if(!pduel->lua->check_condition(target, 9)) {
			return FALSE;
		}
	}
	return TRUE;
}
int32_t effect::is_activate_ready(uint8_t playerid, const tevent& e, int32_t neglect_cond, int32_t neglect_cost, int32_t neglect_target) {
	return is_activate_ready(this, playerid, e, neglect_cond, neglect_cost, neglect_target);
}
// check functions: condition
int32_t effect::is_condition_check(uint8_t playerid, const tevent& e) {
	card* phandler = get_handler();
	if(!(type & EFFECT_TYPE_ACTIVATE) && (phandler->current.location & (LOCATION_ONFIELD | LOCATION_REMOVED)) && !phandler->is_position(POS_FACEUP))
		return FALSE;
	if(!is_in_range_of_symbolic_mzone(phandler))
		return FALSE;
	if(!condition)
		return TRUE;
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = this;
	pduel->game_field->core.reason_player = playerid;
	pduel->game_field->save_lp_cost();
	pduel->lua->add_param<PARAM_TYPE_EFFECT>(this);
	pduel->lua->add_param<PARAM_TYPE_INT>(playerid);
	pduel->lua->add_param<PARAM_TYPE_GROUP>(e.event_cards );
	pduel->lua->add_param<PARAM_TYPE_INT>(e.event_player);
	pduel->lua->add_param<PARAM_TYPE_INT>(e.event_value);
	pduel->lua->add_param<PARAM_TYPE_EFFECT>(e.reason_effect );
	pduel->lua->add_param<PARAM_TYPE_INT>(e.reason);
	pduel->lua->add_param<PARAM_TYPE_INT>(e.reason_player);
	if(!pduel->lua->check_condition(condition, 8)) {
		pduel->game_field->restore_lp_cost();
		pduel->game_field->core.reason_effect = oreason;
		pduel->game_field->core.reason_player = op;
		return FALSE;
	}
	pduel->game_field->restore_lp_cost();
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	return TRUE;
}
int32_t effect::is_activate_check(uint8_t playerid, const tevent& e, int32_t neglect_cond, int32_t neglect_cost, int32_t neglect_target) {
	pduel->game_field->save_lp_cost();
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = this;
	pduel->game_field->core.reason_player = playerid;
	int32_t result = is_activate_ready(playerid, e, neglect_cond, neglect_cost, neglect_target);
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	pduel->game_field->restore_lp_cost();
	return result;
}
int32_t effect::is_target(card* pcard) {
	if(type & EFFECT_TYPE_ACTIONS)
		return FALSE;
	if(type & (EFFECT_TYPE_SINGLE | EFFECT_TYPE_EQUIP | EFFECT_TYPE_XMATERIAL) && !(type & EFFECT_TYPE_FIELD))
		return TRUE;
	if((type & EFFECT_TYPE_TARGET) && !(type & EFFECT_TYPE_FIELD)) {
		return is_fit_target_function(pcard);
	}
	if(pcard && !is_flag(EFFECT_FLAG_SET_AVAILABLE) && (pcard->current.location & LOCATION_ONFIELD)
			&& !pcard->is_position(POS_FACEUP))
		return FALSE;
	if(!is_flag(EFFECT_FLAG_IGNORE_RANGE)) {
		if(pcard->get_status(STATUS_SUMMONING | STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED | STATUS_SPSUMMON_STEP))
			return FALSE;
		if(is_flag(EFFECT_FLAG_SPSUM_PARAM))
			return FALSE;
		if(is_flag(EFFECT_FLAG_ABSOLUTE_TARGET)) {
			if(pcard->current.controler == 0) {
				if(!pcard->current.is_location(s_range))
					return FALSE;
			} else {
				if(!pcard->current.is_location(o_range))
					return FALSE;
			}
		} else {
			if(pcard->current.controler == get_handler_player()) {
				if(!pcard->current.is_location(s_range))
					return FALSE;
			} else {
				if(!pcard->current.is_location(o_range))
					return FALSE;
			}
		}
	}
	return is_fit_target_function(pcard);
}
int32_t effect::is_fit_target_function(card* pcard) {
	if(target) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		if (!pduel->lua->check_condition(target, 2)) {
			return FALSE;
		}
	}
	return TRUE;
}
int32_t effect::is_target_player(uint8_t playerid) {
	if(!is_flag(EFFECT_FLAG_PLAYER_TARGET))
		return FALSE;
	uint8_t self = get_handler_player();
	if(is_flag(EFFECT_FLAG_ABSOLUTE_TARGET)) {
		if(s_range && playerid == 0)
			return TRUE;
		if(o_range && playerid == 1)
			return TRUE;
	} else {
		if(s_range && self == playerid)
			return TRUE;
		if(o_range && self != playerid)
			return TRUE;
	}
	return FALSE;
}
int32_t effect::is_player_effect_target(card* pcard) {
	if(target) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this);
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
		if(!pduel->lua->check_condition(target, 2)) {
			return FALSE;
		}
	}
	return TRUE;
}
int32_t effect::is_immuned(card* pcard) {
	for (const auto& peffect : pcard->immune_effect) {
		if(peffect->is_available() && peffect->value) {
			pduel->lua->add_param<PARAM_TYPE_EFFECT>(this);
			pduel->lua->add_param<PARAM_TYPE_CARD>(pcard);
			if(peffect->check_value_condition(2))
				return TRUE;
		}
	}
	return FALSE;
}
int32_t effect::is_chainable(uint8_t tp) {
	if(!(type & EFFECT_TYPE_ACTIONS))
		return FALSE;
	int32_t sp = get_speed();
	// Curse of Field is the exception
	if((type & EFFECT_TYPE_ACTIVATE) && (sp <= 1) && !is_flag(EFFECT_FLAG2_COF))
		return FALSE;
	if(pduel->game_field->core.current_chain.size()) {
		if(!is_flag(EFFECT_FLAG_FIELD_ONLY) && (type & EFFECT_TYPE_TRIGGER_O)
				&& (get_handler()->current.location == LOCATION_HAND)) {
			if(pduel->game_field->core.current_chain.rbegin()->triggering_effect->get_speed() > 2)
				return FALSE;
		} else if(sp < pduel->game_field->core.current_chain.rbegin()->triggering_effect->get_speed())
			return FALSE;
	}
	for(const auto& ch_lim : pduel->game_field->core.chain_limit) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this);
		pduel->lua->add_param<PARAM_TYPE_INT>(ch_lim.player);
		pduel->lua->add_param<PARAM_TYPE_INT>(tp);
		if(!pduel->lua->check_condition(ch_lim.function, 3))
			return FALSE;
	}
	for(const auto& ch_lim_p : pduel->game_field->core.chain_limit_p) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this);
		pduel->lua->add_param<PARAM_TYPE_INT>(ch_lim_p.player);
		pduel->lua->add_param<PARAM_TYPE_INT>(tp);
		if(!pduel->lua->check_condition(ch_lim_p.function, 3))
			return FALSE;
	}
	return TRUE;
}
//return: this can be reset by reset_level or not
//RESET_DISABLE is valid only when owner == handler
int32_t effect::reset(uint32_t reset_level, uint32_t reset_type) {
	switch (reset_type) {
	case RESET_EVENT: {
		if(!(reset_flag & RESET_EVENT))
			return FALSE;
		if(owner != handler)
			reset_level &= ~RESET_DISABLE;
		if(reset_level & 0xffff0000 & reset_flag)
			return TRUE;
		return FALSE;
		break;
	}
	case RESET_CARD: {
		return owner && (owner->data.code == reset_level);
		break;
	}
	case RESET_PHASE: {
		if(!(reset_flag & RESET_PHASE))
			return FALSE;
		uint8_t pid = get_handler_player();
		uint8_t tp = pduel->game_field->infos.turn_player;
		if((((reset_flag & RESET_SELF_TURN) && pid == tp) || ((reset_flag & RESET_OPPO_TURN) && pid != tp)) 
				&& (reset_level & 0x3ff & reset_flag))
			--reset_count;
		if(reset_count == 0)
			return TRUE;
		return FALSE;
		break;
	}
	case RESET_CODE: {
		return (code == reset_level) && (type & EFFECT_TYPE_SINGLE) && !(type & EFFECT_TYPE_ACTIONS);
		break;
	}
	case RESET_COPY: {
		return copy_id == reset_level;
		break;
	}
	}
	return FALSE;
}
void effect::dec_count(uint32_t playerid) {
	if(!is_flag(EFFECT_FLAG_COUNT_LIMIT))
		return;
	if(count_limit == 0)
		return;
	if(count_code == 0 || is_flag(EFFECT_FLAG_NO_TURN_RESET))
		count_limit -= 1;
	if(count_code || count_flag) {
		if(count_flag & EFFECT_COUNT_CODE_SINGLE)
			pduel->game_field->add_effect_code(get_handler()->fieldid, count_flag, count_hopt_index, PLAYER_NONE);
		else
			pduel->game_field->add_effect_code(count_code, count_flag, count_hopt_index, playerid);
	}
}
void effect::recharge() {
	if(is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
		count_limit = count_limit_max;
	}
}
uint8_t effect::get_client_mode() const {
	if(is_flag(EFFECT_FLAG_FIELD_ONLY))
		return EFFECT_CLIENT_MODE_RESOLVE;
	else if(!(type & EFFECT_TYPE_ACTIONS))
		return EFFECT_CLIENT_MODE_RESET;
	else
		return EFFECT_CLIENT_MODE_NORMAL;
}
lua_Integer effect::get_value(uint32_t extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this, true);
		lua_Integer res = pduel->lua->get_function_value(value, 1 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return static_cast<lua_Integer>(value);
	}
}
lua_Integer effect::get_value(card* pcard, uint32_t extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard, true);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this, true);
		lua_Integer res = pduel->lua->get_function_value(value, 2 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return static_cast<lua_Integer>(value);
	}
}
lua_Integer effect::get_value(effect* peffect, uint32_t extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peffect, true);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this, true);
		lua_Integer res = pduel->lua->get_function_value(value, 2 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return static_cast<lua_Integer>(value);
	}
}
void effect::get_value(uint32_t extraargs, std::vector<lua_Integer>& result) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this, true);
		pduel->lua->get_function_value(value, 1 + extraargs, result);
	} else {
		pduel->lua->params.clear();
		result.push_back(static_cast<lua_Integer>(value));
	}
}
void effect::get_value(card* pcard, uint32_t extraargs, std::vector<lua_Integer>& result) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param<PARAM_TYPE_CARD>(pcard, true);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this, true);
		pduel->lua->get_function_value(value, 2 + extraargs, result);
	} else {
		pduel->lua->params.clear();
		result.push_back(static_cast<lua_Integer>(value));
	}
}
void effect::get_value(effect* peffect, uint32_t extraargs, std::vector<lua_Integer>& result) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(peffect, true);
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this, true);
		pduel->lua->get_function_value(value, 2 + extraargs, result);
	} else {
		pduel->lua->params.clear();
		result.push_back(static_cast<lua_Integer>(value));
	}
}
int32_t effect::check_value_condition(uint32_t extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param<PARAM_TYPE_EFFECT>(this, true);
		int32_t res = pduel->lua->check_condition(value, 1 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return (int32_t)value;
	}
}
void* effect::get_label_object() {
	return pduel->lua->get_ref_object(label_object);
}
int32_t effect::get_speed() {
	if(!(type & EFFECT_TYPE_ACTIONS))
		return 0;
	if(type & (EFFECT_TYPE_TRIGGER_O | EFFECT_TYPE_TRIGGER_F | EFFECT_TYPE_IGNITION))
		return 1;
	else if(type & (EFFECT_TYPE_QUICK_O | EFFECT_TYPE_QUICK_F))
		return 2;
	else if(type & EFFECT_TYPE_ACTIVATE) {
		if(handler->data.type & TYPE_MONSTER)
			return 0;
		else if(handler->data.type & TYPE_SPELL) {
			if((handler->data.type & TYPE_QUICKPLAY) || handler->is_affected_by_effect(EFFECT_BECOME_QUICK))
				return 2;
			return 1;
		} else {
			if (handler->data.type & TYPE_COUNTER)
				return 3;
			return 2;
		}
	}
	return 0;
}
effect* effect::clone(int32_t majestic) {
	effect* ceffect = pduel->new_effect();
	int32_t ref = ceffect->ref_handle;
	*ceffect = *this;
	ceffect->ref_handle = ref;
	ceffect->handler = 0;
	if(condition)
		ceffect->condition = pduel->lua->clone_lua_ref(condition);
	if(cost)
		ceffect->cost = pduel->lua->clone_lua_ref(cost);
	if(target)
		ceffect->target = pduel->lua->clone_lua_ref(target);
	if(operation)
		ceffect->operation = pduel->lua->clone_lua_ref(operation);
	if(label_object)
		ceffect->label_object = pduel->lua->clone_lua_ref(label_object);
	if(value && is_flag(EFFECT_FLAG_FUNC_VALUE))
		ceffect->value = pduel->lua->clone_lua_ref(value);
	if(majestic && is_flag(EFFECT_FLAG2_MAJESTIC_MUST_COPY)) {
		if(value && !is_flag(EFFECT_FLAG_FUNC_VALUE))
			ceffect->value = value;
		ceffect->label = label;
		if(label_object)
			ceffect->label_object = label_object;
		if(s_range)
			ceffect->s_range = s_range;
		if(o_range)
			ceffect->o_range = o_range;
		if(flag[0])
			ceffect->flag[0] = flag[0];
		if(hint_timing[0])
			ceffect->hint_timing[0] = hint_timing[0];
		if(hint_timing[1])
			ceffect->hint_timing[1] = hint_timing[1];
		if(code)
			ceffect->code = code;
		if(type)
			ceffect->type = type;
	}
	return ceffect;
}
card* effect::get_owner() const {
	if(active_handler)
		return active_handler;
	if(type & EFFECT_TYPE_XMATERIAL)
		return handler->overlay_target;
	return owner;
}
uint8_t effect::get_owner_player() {
	if(effect_owner != PLAYER_NONE)
		return effect_owner;
	return get_owner()->current.controler;
}
card* effect::get_handler() const {
	if(active_handler)
		return active_handler;
	if(type & EFFECT_TYPE_XMATERIAL)
		return handler->overlay_target;
	return handler;
}
uint8_t effect::get_handler_player() {
	if(is_flag(EFFECT_FLAG_FIELD_ONLY))
		return effect_owner;
	return get_handler()->current.controler;
}
int32_t effect::in_range(card* pcard) {
	if(type & EFFECT_TYPE_XMATERIAL)
		return handler->overlay_target ? TRUE : FALSE;
	auto real_range = range;
	if((range & (LOCATION_MMZONE | LOCATION_EMZONE))) {
		real_range &= ~(LOCATION_MMZONE | LOCATION_EMZONE);
		real_range |= LOCATION_MZONE;
	}
	return pcard->current.is_location(real_range);
}
int32_t effect::is_in_range_of_symbolic_mzone(card* pcard) {
	if(!(range & (LOCATION_MMZONE | LOCATION_EMZONE)))
		return TRUE;
	return !pcard->current.is_location(LOCATION_MZONE) || pcard->current.is_location(range);
}
int32_t effect::in_range(const chain& ch) {
	if(type & EFFECT_TYPE_XMATERIAL)
		return handler->overlay_target ? TRUE : FALSE;
	return range & ch.triggering_location;
}
void effect::set_activate_location() {
	card* phandler = get_handler();
	active_location = phandler->current.location;
	active_sequence = phandler->current.sequence;
}
void effect::set_active_type() {
	card* phandler = get_handler();
	active_type = phandler->get_type();
	if(active_type & TYPE_TRAPMONSTER)
		active_type &= ~TYPE_TRAP;
}
uint32_t effect::get_active_type() {
	if(type & 0x7f0) {
		if(active_type)
			return active_type;
		else if((type & EFFECT_TYPE_ACTIVATE) && (get_handler()->data.type & TYPE_PENDULUM))
			return TYPE_PENDULUM + TYPE_SPELL;
		else
			return get_handler()->get_type();
	} else
		return owner->get_type();
}
