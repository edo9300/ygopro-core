/*
 * Copyright (c) 2023-2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <variant> //std::visit
#include "field.h"

OCG_DuelStatus field::process() {
	core.units.splice(core.units.begin(), core.subunits);
	if(core.units.empty())
		return OCG_DUEL_STATUS_END;

	auto invoke = [this](auto& arg) -> OCG_DuelStatus {
		if(process(arg)) {
			core.units.pop_front();
			return OCG_DUEL_STATUS_CONTINUE;
		} else {
			++arg.step;
			return Processors::NeedsAnswer<decltype(arg)> ? OCG_DUEL_STATUS_AWAITING : OCG_DUEL_STATUS_CONTINUE;
		}
	};

	return std::visit([&](auto& arg) -> OCG_DuelStatus {
		if constexpr(Processors::IsProcess<decltype(arg)>) {
			return invoke(arg);
		} else {
			// handle the case where the processes are stored
			// in variants of variants in debug mode
			return std::visit(invoke, arg);
		}
	}, core.units.front());
}
