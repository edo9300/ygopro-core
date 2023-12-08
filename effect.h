/*
 * effect.h
 *
 *  Created on: 2010-3-13
 *      Author: Argon
 */

#ifndef EFFECT_H_
#define EFFECT_H_

#include <cstdlib>
#include <vector>
#include <map>
#include "common.h"
#include "lua_obj.h"
#include "field.h"
#include "effect_constants.h"

class card;
class duel;
class group;
struct tevent;
enum effect_flag : uint32_t;
enum effect_flag2 : uint32_t;

class effect : public lua_obj_helper<LuaParam::EFFECT> {
public:
	uint8_t count_limit{};
	uint8_t count_limit_max{};
	uint8_t count_flag{};
	uint8_t count_hopt_index{};
	uint8_t effect_owner{ PLAYER_NONE };
	uint16_t type{};
	uint16_t copy_id{};
	uint16_t range{};
	uint16_t s_range{};
	uint16_t o_range{};
	uint16_t reset_count{};
	uint16_t active_location{};
	uint16_t active_sequence{};
	uint16_t status{};
	uint32_t code{};
	uint32_t flag[2]{};
	uint32_t id{};
	uint32_t reset_flag{};
	uint32_t count_code{};
	uint32_t category{};
	uint32_t hint_timing[2]{};
	uint32_t card_type{};
	uint32_t active_type{};
	int32_t label_object{};
	int32_t condition{};
	int32_t cost{};
	int32_t target{};
	int32_t value{};
	int32_t operation{};
	card* owner{ nullptr };
	card* handler{ nullptr };
	card* active_handler{};
	uint64_t description{};
	std::vector<lua_Integer> label;

	explicit effect(duel* pd) : lua_obj_helper(pd) {};
	~effect() = default;

	int32_t is_disable_related();
	int32_t is_self_destroy_related();
	int32_t is_can_be_forbidden();
	int32_t is_available();
	int32_t check_count_limit(uint8_t playerid) const;
	int32_t has_count_limit() const {
		return is_flag(EFFECT_FLAG_COUNT_LIMIT);
	}
	int32_t is_activateable(uint8_t playerid, const tevent& e, int32_t neglect_cond = FALSE, int32_t neglect_cost = FALSE, int32_t neglect_target = FALSE, int32_t neglect_loc = FALSE, int32_t neglect_faceup = FALSE);
	int32_t is_action_check(uint8_t playerid);
	int32_t is_activate_ready(effect* reason_effect, uint8_t playerid, const tevent& e, int32_t neglect_cond = FALSE, int32_t neglect_cost = FALSE, int32_t neglect_target = FALSE);
	int32_t is_activate_ready(uint8_t playerid, const tevent& e, int32_t neglect_cond = FALSE, int32_t neglect_cost = FALSE, int32_t neglect_target = FALSE);
	int32_t is_condition_check(uint8_t playerid, const tevent& e);
	int32_t is_activate_check(uint8_t playerid, const tevent& e, int32_t neglect_cond = FALSE, int32_t neglect_cost = FALSE, int32_t neglect_target = FALSE);
	int32_t is_target(card* pcard);
	int32_t is_fit_target_function(card* pcard);
	int32_t is_target_player(uint8_t playerid);
	int32_t is_player_effect_target(card* pcard);
	int32_t is_immuned(card* pcard);
	int32_t is_chainable(uint8_t tp);
	int32_t reset(uint32_t reset_level, uint32_t reset_type);
	void dec_count(uint32_t playerid = 2);
	void recharge();
	uint8_t get_client_mode() const;
	bool has_function_value() const {
		return is_flag(EFFECT_FLAG_FUNC_VALUE);
	}
	lua_Integer get_value(uint32_t extraargs = 0);
	lua_Integer get_value(card* pcard, uint32_t extraargs = 0);
	lua_Integer get_value(effect* peffect, uint32_t extraargs = 0);
	void get_value(uint32_t extraargs, std::vector<lua_Integer>& result);
	void get_value(card* pcard, uint32_t extraargs, std::vector<lua_Integer>& result);
	void get_value(effect* peffect, uint32_t extraargs, std::vector<lua_Integer>& result);
	int32_t check_value_condition(uint32_t extraargs = 0);
	void* get_label_object();
	int32_t get_speed();
	effect* clone(int32_t majestic = FALSE);
	card* get_owner() const;
	uint8_t get_owner_player();
	card* get_handler() const;
	uint8_t get_handler_player();
	int32_t in_range(card* pcard);
	int32_t is_in_range_of_symbolic_mzone(card* pcard);
	int32_t in_range(const chain& ch);
	void set_activate_location();
	void set_active_type();
	uint32_t get_active_type();
	bool is_flag(effect_flag flag_to_check) const {
		return !!(flag[0] & flag_to_check);
	}
	bool is_flag(effect_flag2 flag_to_check) const {
		return !!(flag[1] & flag_to_check);
	}
};

#endif /* EFFECT_H_ */
