#pragma once

#include "sol.hpp"
#include "lua-helpers.h"
#include "gc-data.h"

namespace effil {

namespace cache {

void put(lua_State* lua, GCHandle handle, sol::object);
sol::optional<sol::object> get(lua_State* lua, GCHandle handle);

} // namespace cache

class CachedGCData: public GCData {
public:
    virtual ~CachedGCData();
};

} // namespace effil
