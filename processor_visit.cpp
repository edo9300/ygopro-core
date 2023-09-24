/*
 * Copyright (c) 2023-2024, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <variant> //std::visit
#include "field.h"

OCG_DuelStatus field::process() {
	core.units.splice(core.units.begin(), core.subunits);
	if(core.units.empty())
		return OCG_DUEL_STATUS_END;
	return std::visit(*this, core.units.front());
}
