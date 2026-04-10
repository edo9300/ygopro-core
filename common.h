/*
 * Copyright (c) 2009-2015, Argon Sun (Fluorohydride)
 * Copyright (c) 2017-2026, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef COMMON_H_
#define COMMON_H_

#if defined( _MSC_VER) && !defined(__clang_analyzer__)
#pragma warning(disable: 4244)
#define unreachable() __assume(0)
#define NoInline __declspec(noinline)
#define ForceInline __forceinline
#define Assume(cond) __assume(cond)
#else
#if !defined(__forceinline)
#define ForceInline __attribute__((always_inline)) inline
#else
#define ForceInline __forceinline
#endif
#define unreachable() __builtin_unreachable()
#define NoInline __attribute__ ((noinline))
#define Assume(cond) do { if(!(cond)){ unreachable(); } } while(0)
#endif

#if defined(__clang_analyzer__)
#undef NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include "ocgapi_constants.h"

#define TRUE 1
#define FALSE 0

//Symbolic locations
#define LOCATION_FZONE   0x100
#define LOCATION_PZONE   0x200
#define LOCATION_STZONE  0x400
#define LOCATION_MMZONE  0x800
#define LOCATION_EMZONE  0x1000
//Locations redirect
#define LOCATION_DECKBOT	0x10001		//Return to deck bottom
#define LOCATION_DECKSHF	0x20001		//Return to deck and shuffle

//
#define COIN_HEADS  1
#define COIN_TAILS  0

//Flip effect flags
#define NO_FLIP_EFFECT     0x10000

//Players
#define PLAYER_SELFDES  5

enum ActivityType : uint8_t {
	ACTIVITY_SUMMON = 1,
	ACTIVITY_NORMALSUMMON = 2,
	ACTIVITY_SPSUMMON = 3,
	ACTIVITY_FLIPSUMMON = 4,
	ACTIVITY_ATTACK = 5,
	ACTIVITY_BATTLE_PHASE = 6,
	ACTIVITY_CHAIN = 7,
};

template<typename IntType, auto min_value_, auto max_value_>
struct RangedInteger {
	using value_type = IntType;
	static constexpr IntType min_value = min_value_;
	static constexpr IntType max_value = max_value_;
	IntType value;
	RangedInteger() = default;
	RangedInteger(IntType v) : value(v) {}
	operator IntType() {
		return value;
	}
	operator IntType() const {
		return value;
	}
};

template <typename T>
struct is_ranged_integer : std::false_type {};

template <typename T, auto min, auto max>
struct is_ranged_integer<RangedInteger<T, min, max>> : std::true_type {};

template<typename T>
[[maybe_unused]] inline constexpr bool is_ranged_integer_v = is_ranged_integer<T>::value;


#endif /* COMMON_H_ */
