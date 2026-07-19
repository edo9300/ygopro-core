/*
 * Copyright (c) 2026 The EDOPro multiplayer modes contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <array>
#include <cstddef>
#include <cstdint>

enum class MultiplayerMode : uint8_t {
	NONE = 0,
	BATTLE_ROYALE,
	THREE_V_ONE
};

enum class PlayerEliminationReason : uint8_t {
	LP = 1,
	DECK = 2,
	SURRENDER = 3,
	EFFECT = 4
};

class MultiplayerState {
public:
	static constexpr uint8_t MAX_PLAYERS = 4;
	static constexpr uint8_t NO_PLAYER = 0xff;
	static constexpr uint8_t NO_TEAM = 0xff;

	void configure(MultiplayerMode new_mode);
	void reset();

	MultiplayerMode mode() const;
	bool enabled() const;
	uint8_t active_mask() const;
	uint8_t active_count() const;
	bool is_active(uint8_t player) const;
	uint8_t team_of(uint8_t player) const;
	uint8_t next_active_player(uint8_t player) const;

	bool eliminate(uint8_t player, PlayerEliminationReason reason);
	PlayerEliminationReason elimination_reason(uint8_t player) const;

	bool has_winner() const;
	uint8_t winner_player() const;
	uint8_t winner_team() const;

private:
	static uint8_t count_bits(uint8_t value);
	uint8_t active_teams_mask() const;
	void update_winner();

	MultiplayerMode duel_mode{ MultiplayerMode::NONE };
	uint8_t players_mask{ 0 };
	std::array<uint8_t, MAX_PLAYERS> teams{ NO_TEAM, NO_TEAM, NO_TEAM, NO_TEAM };
	std::array<uint8_t, MAX_PLAYERS> turn_order{ 0, 1, 2, 3 };
	std::array<PlayerEliminationReason, MAX_PLAYERS> reasons{
		PlayerEliminationReason::LP,
		PlayerEliminationReason::LP,
		PlayerEliminationReason::LP,
		PlayerEliminationReason::LP
	};
	uint8_t winning_player{ NO_PLAYER };
	uint8_t winning_team{ NO_TEAM };
};

#endif // MULTIPLAYER_H
