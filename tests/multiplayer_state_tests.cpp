#include "multiplayer.h"

#include <cstdlib>
#include <iostream>

namespace {
void expect(bool condition, const char* message) {
	if(condition)
		return;
	std::cerr << "FAILED: " << message << '\n';
	std::exit(1);
}

void test_battle_royale_turn_order_and_skip() {
	MultiplayerState state;
	state.configure(MultiplayerMode::BATTLE_ROYALE);
	expect(state.active_mask() == 0x0f, "Battle Royale must start with four active players");
	expect(state.next_active_player(0) == 2, "A1 must pass to B1");
	expect(state.next_active_player(2) == 1, "B1 must pass to A2");
	expect(state.next_active_player(1) == 3, "A2 must pass to B2");
	expect(state.next_active_player(3) == 0, "B2 must pass to A1");
	expect(state.current_player() == 0, "Battle Royale must start with A1");
	expect(state.advance_turn() == 2, "the logical turn must advance from A1 to B1");
	expect(state.advance_turn() == 1, "the logical turn must advance from B1 to A2");
	expect(state.field_side_of(0) == 0 && state.field_side_of(1) == 0,
		"A1 and A2 must use field side 0");
	expect(state.field_side_of(2) == 1 && state.field_side_of(3) == 1,
		"B1 and B2 must use field side 1");
	expect(state.logical_player(1, 0) == 2 && state.logical_player(1, 1) == 3,
		"Battle Royale side-1 duelist mapping must be stable");

	expect(state.eliminate(2, PlayerEliminationReason::LP), "player 2 should be eliminated");
	expect(state.active_mask() == 0x0b, "eliminating player 2 must produce active mask 0x0B");
	expect(state.next_active_player(0) == 1, "the eliminated B1 seat must be skipped");
	expect(state.next_active_player(1) == 3, "the remaining order must continue to B2");
	expect(state.next_active_player(3) == 0, "the remaining order must wrap to A1");
	expect(!state.has_winner(), "three active players must not end Battle Royale");
}

void test_battle_royale_multi_elimination() {
	MultiplayerState state;
	state.configure(MultiplayerMode::BATTLE_ROYALE);
	expect(state.eliminate(1, PlayerEliminationReason::LP), "player 1 elimination must succeed");
	expect(state.eliminate(2, PlayerEliminationReason::DECK), "player 2 elimination must succeed");
	expect(!state.has_winner(), "two active players must not end Battle Royale");
	expect(state.eliminate(3, PlayerEliminationReason::SURRENDER), "player 3 elimination must succeed");
	expect(state.has_winner(), "one remaining player must end Battle Royale");
	expect(state.winner_player() == 0, "player 0 must be the final winner");
	expect(state.active_mask() == 0x01, "only player 0 must remain active");
	expect(!state.eliminate(0, PlayerEliminationReason::EFFECT), "the winner cannot be eliminated after completion");
}

void test_three_vs_one_team_winner() {
	MultiplayerState state;
	state.configure(MultiplayerMode::THREE_V_ONE);
	expect(state.team_of(0) == 0, "player 0 must be the solo team");
	expect(state.team_of(1) == 1 && state.team_of(2) == 1 && state.team_of(3) == 1,
		"players 1, 2 and 3 must share the opposing team");
	expect(state.field_side_of(0) == 0 && state.field_side_of(1) == 1,
		"the solo player and opposing team must use different field sides");
	expect(state.duelist_index_of(3) == 2 && state.logical_player(1, 2) == 3,
		"3 vs 1 duelist mapping must preserve player 3");
	expect(state.eliminate(1, PlayerEliminationReason::LP), "first team member elimination must succeed");
	expect(!state.has_winner(), "one eliminated team member must not end 3 vs 1");
	expect(state.eliminate(2, PlayerEliminationReason::LP), "second team member elimination must succeed");
	expect(!state.has_winner(), "two eliminated team members must not end 3 vs 1");
	expect(state.eliminate(3, PlayerEliminationReason::LP), "last team member elimination must succeed");
	expect(state.has_winner(), "eliminating the full team must end 3 vs 1");
	expect(state.winner_team() == 0 && state.winner_player() == 0, "the solo player must win as team 0");

	MultiplayerState opposing_win;
	opposing_win.configure(MultiplayerMode::THREE_V_ONE);
	expect(opposing_win.eliminate(0, PlayerEliminationReason::LP), "solo player elimination must succeed");
	expect(opposing_win.has_winner(), "eliminating the solo player must end 3 vs 1");
	expect(opposing_win.winner_team() == 1, "the three-player team must win");
	expect(opposing_win.winner_player() == MultiplayerState::NO_PLAYER,
		"a team victory must not invent an individual winner");
}

void test_disabled_state_is_inert() {
	MultiplayerState state;
	expect(!state.enabled(), "the default state must be disabled");
	expect(state.active_mask() == 0, "the disabled state must not expose active players");
	expect(state.next_active_player(0) == MultiplayerState::NO_PLAYER,
		"the disabled state must not provide a turn player");
	expect(!state.eliminate(0, PlayerEliminationReason::LP),
		"the disabled state must reject eliminations");
}

void test_simultaneous_elimination_draw() {
	MultiplayerState state;
	state.configure(MultiplayerMode::BATTLE_ROYALE);
	auto reasons = std::array<PlayerEliminationReason, MultiplayerState::MAX_PLAYERS>{
		PlayerEliminationReason::LP,
		PlayerEliminationReason::LP,
		PlayerEliminationReason::DECK,
		PlayerEliminationReason::LP
	};
	expect(state.eliminate_many(0x0f, reasons) == 0x0f,
		"a simultaneous elimination must remove every active player");
	expect(state.is_draw(), "zero active players must be represented as a draw");
	expect(state.is_finished(), "a draw must finish the multiplayer duel");
	expect(!state.has_winner(), "a simultaneous draw must not invent a winner");
}
}

int main() {
	test_battle_royale_turn_order_and_skip();
	test_battle_royale_multi_elimination();
	test_three_vs_one_team_winner();
	test_disabled_state_is_inert();
	test_simultaneous_elimination_draw();
	std::cout << "All multiplayer state tests passed.\n";
	return 0;
}
