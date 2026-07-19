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
	} else if(new_mode == MultiplayerMode::THREE_V_ONE) {
		players_mask = 0x0f;
		teams = { 0, 1, 1, 1 };
		turn_order = { 0, 1, 2, 3 };
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
	if(!is_active(player) || has_winner())
		return false;
	players_mask &= static_cast<uint8_t>(~(1u << player));
	reasons[player] = reason;
	update_winner();
	return true;
}

PlayerEliminationReason MultiplayerState::elimination_reason(uint8_t player) const {
	return player < MAX_PLAYERS ? reasons[player] : PlayerEliminationReason::EFFECT;
}

bool MultiplayerState::has_winner() const {
	return winning_player != NO_PLAYER || winning_team != NO_TEAM;
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
