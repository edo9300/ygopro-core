/*
 * Copyright (c) 2020-2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef LUA_OBJ_H_
#define LUA_OBJ_H_

#include <cstdint>

enum class LuaParam : uint8_t {
	INT = 1,
	STRING,
	CARD,
	GROUP,
	EFFECT,
	FUNCTION,
	BOOLEAN,
	INDEX,
	DELETED,
};

class duel;
template<typename T>
class owned_lua;

class lua_obj {
	template<typename T>
	friend class owned_lua;
public:
	duel* pduel{ nullptr };
	int32_t ref_handle{};
	int32_t weak_ref_handle{};
	LuaParam lua_type;
protected:
	lua_obj(LuaParam _lua_type, duel* _pduel) :pduel(_pduel), lua_type(_lua_type) {}
private:
	int num_ref{};
	void incr_ref();
	void decr_ref();
};

template<typename T>
class owned_lua {
	T* obj{};
public:
	using lua_type = T*;
	owned_lua() = default;
	owned_lua(T* obj_) : obj(obj_) {
		if(obj) {
			obj->incr_ref();
		}
	}
	owned_lua(const owned_lua<T>& other) : obj(other.obj) {
		if(obj) {
			obj->incr_ref();
		}
	}
	owned_lua(owned_lua<T>&& other) : obj(other.obj) {
		other.obj = nullptr;
	}
	~owned_lua() {
		if(obj) {
			obj->decr_ref();
		}
	}
	owned_lua<T>& operator=(const owned_lua<T>& other) {
		if(obj) {
			obj->decr_ref();
		}
		if(obj = other.obj; obj) {
			obj->incr_ref();
		}
		return *this;
	}
	owned_lua<T>& operator=(owned_lua<T>&& other) {
		if(obj) {
			obj->decr_ref();
		}
		obj = other.obj;
		other.obj = nullptr;
		return *this;
	}
	owned_lua<T>& operator=(T* rhs) {
		if(obj) {
			obj->decr_ref();
		}
		if(obj = rhs; obj) {
			obj->incr_ref();
		}
		return *this;
	}
	operator T* () const {
		return obj;
	}
	operator T* () {
		return obj;
	}
	T* operator-> () const {
		return obj;
	}
	T* operator-> () {
		return obj;
	}
	explicit operator bool() {
		return obj != nullptr;
	}
	explicit operator bool() const {
		return obj != nullptr;
	}
};

template<LuaParam _Type>
class lua_obj_helper : public lua_obj {
	static_assert(_Type == LuaParam::CARD || _Type == LuaParam::GROUP ||
				  _Type == LuaParam::EFFECT || _Type == LuaParam::DELETED,
				  "Invalid parameter type");
public:
	lua_obj_helper(duel* pduel_) : lua_obj(_Type, pduel_) {}
};

#endif /* LUA_OBJ_H_ */
