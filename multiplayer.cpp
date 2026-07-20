/*
 * Copyright (c) 2026 The EDOPro multiplayer modes contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include "multiplayer.h"

void MultiplayerState::configure(MultiplayerMode new_mode) {
	reset();
	duel_mode = new_mode;
	if(new_mode == MultiplayerMode::BATTLE_ROYALE) {
		players_mask = 0x0f;
		teams = { 0, 1, 2, 3 };
		// Network seats are A1, A2, B1, B2. The requested round order is
		// A1 -> B1 -> A2 -> B2.
		turn_order = { 0, 2, 1, 3 };
		turn_player = 0;
	} else if(new_mode == MultiplayerMode::THREE_V_ONE) {
		players_mask = 0x0f;
		teams = { 0, 1, 1, 1 };
		turn_order = { 0, 1, 2, 3 };
		turn_player = 0;
	}
}

void MultiplayerState::reset() {
	duel_mode = MultiplayerMode::NONE;
	players_mask = 0;
	teams = { NO_TEAM, NO_TEAM, NO_TEAM, NO_TEAM };
	turn_order = { 0, 1, 2, 3 };
	reasons.fill(PlayerEliminationReason::LP);
	winning_player = NO_PLAYER;
	winning_team = NO_TEAM;
	turn_player = NO_PLAYER;
}

MultiplayerMode MultiplayerState::mode() const {
	return duel_mode;
}

bool MultiplayerState::enabled() const {
	return duel_mode != MultiplayerMode::NONE;
}

uint8_t MultiplayerState::active_mask() const {
	return players_mask;
}

uint8_t MultiplayerState::active_count() const {
	return count_bits(players_mask);
}

bool MultiplayerState::is_active(uint8_t player) const {
	return player < MAX_PLAYERS && (players_mask & (1u << player));
}

uint8_t MultiplayerState::team_of(uint8_t player) const {
	return player < MAX_PLAYERS ? teams[player] : NO_TEAM;
}

uint8_t MultiplayerState::field_side_of(uint8_t player) const {
	if(player >= MAX_PLAYERS || !enabled())
		return NO_PLAYER;
	if(duel_mode == MultiplayerMode::BATTLE_ROYALE)
		return player < 2 ? 0 : 1;
	return player == 0 ? 0 : 1;
}

uint8_t MultiplayerState::duelist_index_of(uint8_t player) const {
	if(player >= MAX_PLAYERS || !enabled())
		return NO_PLAYER;
	if(duel_mode == MultiplayerMode::BATTLE_ROYALE)
		return player & 1u;
	return player == 0 ? 0 : static_cast<uint8_t>(player - 1);
}

uint8_t MultiplayerState::logical_player(uint8_t field_side, uint8_t duelist_index) const {
	if(!enabled() || field_side > 1)
		return NO_PLAYER;
	if(duel_mode == MultiplayerMode::BATTLE_ROYALE) {
		if(duelist_index > 1)
			return NO_PLAYER;
		return static_cast<uint8_t>((field_side ? 2 : 0) + duelist_index);
	}
	if(field_side == 0)
		return duelist_index == 0 ? 0 : NO_PLAYER;
	return duelist_index < 3 ? static_cast<uint8_t>(duelist_index + 1) : NO_PLAYER;
}

uint8_t MultiplayerState::current_player() const {
	return turn_player;
}

uint8_t MultiplayerState::advance_turn() {
	if(!enabled() || has_winner())
		return NO_PLAYER;
	turn_player = next_active_player(turn_player);
	return turn_player;
}

uint8_t MultiplayerState::next_active_player(uint8_t player) const {
	if(!enabled() || players_mask == 0)
		return NO_PLAYER;

	std::size_t current_index = turn_order.size();
	for(std::size_t index = 0; index < turn_order.size(); ++index) {
		if(turn_order[index] == player) {
			current_index = index;
			break;
		}
	}
	if(current_index == turn_order.size())
		return NO_PLAYER;

	for(std::size_t offset = 1; offset <= turn_order.size(); ++offset) {
		const uint8_t candidate = turn_order[(current_index + offset) % turn_order.size()];
		if(is_active(candidate))
			return candidate;
	}
	return NO_PLAYER;
}

bool MultiplayerState::eliminate(uint8_t player, PlayerEliminationReason reason) {
	if(player >= MAX_PLAYERS)
		return false;
	auto player_reasons = reasons;
	player_reasons[player] = reason;
	return eliminate_many(static_cast<uint8_t>(1u << player), player_reasons) != 0;
}

uint8_t MultiplayerState::eliminate_many(uint8_t player_mask,
		const std::array<PlayerEliminationReason, MAX_PLAYERS>& player_reasons) {
	if(!enabled() || is_finished())
		return 0;
	const uint8_t eliminated = player_mask & players_mask & 0x0f;
	if(!eliminated)
		return 0;
	players_mask &= static_cast<uint8_t>(~eliminated);
	for(uint8_t player = 0; player < MAX_PLAYERS; ++player) {
		if(eliminated & (1u << player))
			reasons[player] = player_reasons[player];
	}
	update_winner();
	return eliminated;
}

PlayerEliminationReason MultiplayerState::elimination_reason(uint8_t player) const {
	return player < MAX_PLAYERS ? reasons[player] : PlayerEliminationReason::EFFECT;
}

bool MultiplayerState::has_winner() const {
	return winning_player != NO_PLAYER || winning_team != NO_TEAM;
}

bool MultiplayerState::is_draw() const {
	return enabled() && players_mask == 0;
}

bool MultiplayerState::is_finished() const {
	return has_winner() || is_draw();
}

uint8_t MultiplayerState::winner_player() const {
	return winning_player;
}

uint8_t MultiplayerState::winner_team() const {
	return winning_team;
}

uint8_t MultiplayerState::count_bits(uint8_t value) {
	uint8_t count = 0;
	while(value) {
		count += value & 1u;
		value >>= 1u;
	}
	return count;
}

uint8_t MultiplayerState::active_teams_mask() const {
	uint8_t mask = 0;
	for(uint8_t player = 0; player < MAX_PLAYERS; ++player) {
		if(is_active(player) && teams[player] != NO_TEAM)
			mask |= static_cast<uint8_t>(1u << teams[player]);
	}
	return mask;
}

void MultiplayerState::update_winner() {
	winning_player = NO_PLAYER;
	winning_team = NO_TEAM;
	if(duel_mode == MultiplayerMode::BATTLE_ROYALE) {
		if(active_count() != 1)
			return;
		for(uint8_t player = 0; player < MAX_PLAYERS; ++player) {
			if(is_active(player)) {
				winning_player = player;
				winning_team = teams[player];
				return;
			}
		}
		return;
	}
	if(duel_mode == MultiplayerMode::THREE_V_ONE) {
		const uint8_t team_mask = active_teams_mask();
		if(team_mask == 0 || count_bits(team_mask) != 1)
			return;
		for(uint8_t team = 0; team < MAX_PLAYERS; ++team) {
			if(team_mask & (1u << team)) {
				winning_team = team;
				if(team == 0 && is_active(0))
					winning_player = 0;
				return;
			}
		}
	}
}
