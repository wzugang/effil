#include "cache.h"
#include "shared_mutex"

namespace effil {
namespace cache {

namespace {

typedef std::unordered_map<lua_State*, sol::object> LuaStateStorage;

std::shared_timed_mutex storageLock;
std::unordered_map<GCHandle, LuaStateStorage> cacheStorage;

void removeFromCache(GCHandle handle) {
    DEBUG("cache") << "remove object " << std::hex << handle;

    std::unique_lock<std::shared_timed_mutex> lock(storageLock);
    auto luaStorage = cacheStorage.find(handle);
    if (luaStorage != cacheStorage.end())
        cacheStorage.erase(luaStorage);
}

} // namespace

void put(lua_State* lua, GCHandle handle, sol::object obj) {
    DEBUG("cache") << "put " << std::hex << obj.registry_index()
                   << " in state " << std::hex << lua
                   << " for handle " << std::hex << handle;

    std::unique_lock<std::shared_timed_mutex> lock(storageLock);
    auto luaStorage = cacheStorage.find(handle);
    if (luaStorage == cacheStorage.end())
    {
        cacheStorage[handle] = LuaStateStorage();
        luaStorage = cacheStorage.find(handle);
    }

    luaStorage->second[lua] = obj;
}

sol::optional<sol::object> get(lua_State* lua, GCHandle handle) {
    DEBUG("cache") << "get " << std::hex << handle
                   << " from " << std::hex << lua;

    std::shared_lock<std::shared_timed_mutex> lock(storageLock);
    auto luaStorage = cacheStorage.find(handle);
    if (luaStorage != cacheStorage.end())
    {
        auto stateStorage = luaStorage->second.find(lua);
        if (stateStorage != luaStorage->second.end())
            return stateStorage->second;
    }
    return sol::nullopt;
}

} // namespace cache

CachedGCData::~CachedGCData() {
    cache::removeFromCache(this);
}

} // namespace effil

