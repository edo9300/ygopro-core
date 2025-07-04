/*
 * Copyright (c) 2020-2025, Edoardo Lolletti (edo9300) <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#ifndef LUA_OBJ_H_
#define LUA_OBJ_H_

#include <cstdint>
#include <utility> // std::exchange

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
	owned_lua<T>& assign(T* other) {
		if(obj) {
			obj->decr_ref();
		}
		obj = other;
		if(obj) {
			obj->incr_ref();
		}
		return *this;
	}
public:
	using lua_type = T*;
	owned_lua() = default;
	owned_lua(T* obj_) : obj(obj_) {
		if(obj) {
			obj->incr_ref();
		}
	}
	owned_lua(const owned_lua<T>& other) : owned_lua(other.obj) {}
	owned_lua(owned_lua<T>&& other) : obj(other.obj) {
		other.obj = nullptr;
	}
	~owned_lua() {
		if(obj) {
			obj->decr_ref();
		}
	}
	owned_lua<T>& operator=(owned_lua<T>&& rhs) {
		if(obj) {
			obj->decr_ref();
		}
		obj = std::exchange(rhs.obj, nullptr);
		return *this;
	}
	owned_lua<T>& operator=(const owned_lua<T>& rhs) {
		return assign(rhs.obj);
	}
	owned_lua<T>& operator=(T* rhs) {
		return assign(rhs);
	}
	operator T* () const {
		return obj;
	}
	T* operator-> () const {
		return obj;
	}
	bool operator ==(const owned_lua<T>& other) const {
		return obj == other.obj;
	}
	bool operator ==(T* other) const {
		return obj == other;
	}
	bool operator <(const owned_lua<T>& other) const {
		return obj < other.obj;
	}
	explicit operator bool() const {
		return obj != nullptr;
	}
};

template<LuaParam type>
class lua_obj_helper : public lua_obj {
	static_assert(type == LuaParam::CARD || type == LuaParam::GROUP ||
				  type == LuaParam::EFFECT || type == LuaParam::DELETED,
				  "Invalid parameter type");
public:
	lua_obj_helper(duel* pduel_) : lua_obj(type, pduel_) {}
};

#endif /* LUA_OBJ_H_ */
