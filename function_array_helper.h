/*
 * Copyright (c) 2022-2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef FUNCTION_ARRAY_HELPER_H
#define FUNCTION_ARRAY_HELPER_H

#include "scriptlib.h"

#include <meta>
#include <string_view>

#define LUA_NAMESPACE lua_functions

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MAKE_LUA_MODULE_IMPL(module) c_lua_##module##_
#define MAKE_LUA_MODULE(module) MAKE_LUA_MODULE_IMPL(module)

#define LUA_PREFIX STR(MAKE_LUA_MODULE(LUA_MODULE))

#define MAKE_LUA_NAME_IMPL(module, name) c_lua_##module##_##name
#define MAKE_LUA_NAME(module, name) MAKE_LUA_NAME_IMPL(module, name)
#define LUA_FUNCTION(name) [[maybe_unused]] static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, \
		[[maybe_unused]] LUA_CLASS* const self, [[maybe_unused]] duel* const pduel)

#define LUA_STATIC_FUNCTION(name) [[maybe_unused]] static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L, [[maybe_unused]] duel* const pduel)

#define LUA_FUNCTION_ALIAS(name) [[maybe_unused]] static lua_alias MAKE_LUA_NAME(LUA_MODULE,name)

#define LUA_FUNCTION_EXISTING(name,...) [[maybe_unused]] [[=aliases(__VA_ARGS__)]] static int32_t MAKE_LUA_NAME(LUA_MODULE,name) \
	([[maybe_unused]] lua_State* const L)

namespace {
struct aliases { lua_CFunction field; };

struct lua_alias { };
template<std::meta::info Namespace>
constexpr auto get_lua_functions() {
	constexpr auto lua_symbols = [] {
		constexpr auto ctx = std::meta::access_context::current();
		std::vector<std::meta::info> res;
		template for (constexpr auto M : define_static_array(members_of(Namespace, ctx))) {
			if constexpr(identifier_of(M).starts_with(LUA_PREFIX)) {
				res.push_back(M);
			}
		}
		return define_static_array(res);
	}();
	std::array<luaL_Reg, lua_symbols.size()+1> ret_array{};
	int i = 0;
    template for (constexpr auto M : lua_symbols) {
		constexpr auto identifier = identifier_of(M).substr(std::string_view{LUA_PREFIX}.size());
		if constexpr(is_function(M)) {
			constexpr auto annotations = define_static_array(annotations_of(M, ^^aliases));
			static_assert(annotations.size() <= 1);
			if constexpr(annotations.size() == 1) {
				constexpr auto annotation = extract<aliases>(annotations[0]);
				static_assert(annotation.field != nullptr);
				ret_array[i] = luaL_Reg{identifier.data(), annotation.field};
			} else {
				static constexpr auto func_ptr = [:M:];
				static constexpr auto parameters = define_static_array(parameters_of(M));
				static constexpr auto num_of_parameters = parameters.size();
				ret_array[i] = luaL_Reg{identifier.data(), [](lua_State* L) -> int {
					using namespace scriptlib;
#if defined(LUA_CLASS)
					if constexpr(num_of_parameters == 3) {
						return func_ptr(L, lua_get<LUA_CLASS*, true>(L, 1), lua_get<duel*>(L));
					} else 
#endif
					if constexpr(num_of_parameters == 2) {
						return func_ptr(L, lua_get<duel*>(L));
					}
				}};
			}
		} else if constexpr(is_variable(M)) {
			static_assert(type_of(M) == ^^lua_alias);
			ret_array[i] = luaL_Reg{identifier.data(), ret_array[i - 1].func};
		}
		i++;
    }
	ret_array[i] = {nullptr,nullptr};
	return ret_array;
}
}

#undef LUA_PREFIX
#undef STR
#undef STR_HELPER
#undef MAKE_LUA_MODULE_IMPL
#undef MAKE_LUA_MODULE

#define GET_LUA_FUNCTIONS_ARRAY() get_lua_functions<^^LUA_NAMESPACE>()

#endif // FUNCTION_ARRAY_HELPER_H
