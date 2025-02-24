/*
 * Copyright (c) 2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef LUA_RET_H
#define LUA_RET_H
#include <cstdint>
#include <functional>
#include <variant>

using LuaRet = std::variant<int32_t, std::function<int32_t(lua_State*)>>;

#endif // LUA_RET_H
