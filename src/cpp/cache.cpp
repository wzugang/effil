#include "cache.h"
#include "shared_mutex"

namespace effil {
namespace cache {

namespace {

typedef int LuaRef;
typedef std::unordered_map<lua_State*, LuaRef> LuaStateStorage;

std::shared_timed_mutex storageLock;
std::unordered_map<GCHandle, LuaStateStorage> cacheStorage;

void removeFromCache(GCHandle handle) {
    DEBUG("cache") << "remove object " << std::hex << handle;

    std::unique_lock<std::shared_timed_mutex> lock(storageLock);
    auto luaStorage = cacheStorage.find(handle);
    if (luaStorage != cacheStorage.end())
    {
        for (const auto& stateStorage: luaStorage->second)
        {
            DEBUG("cache") << "remove object from state" << std::hex << stateStorage.first;
            luaL_unref(stateStorage.first, LUA_REGISTRYINDEX, stateStorage.second);
        }
        cacheStorage.erase(luaStorage);
    }
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

    obj.push();
    const auto id = luaL_ref(lua, LUA_REGISTRYINDEX);
    luaStorage->second[lua] = id;

    DEBUG("cache") << "pushed " << std::hex << handle
                   << " in state " << std::hex << lua
                   << " under adress " << std::hex << id;
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
        {
            const auto id = stateStorage->second;
            DEBUG("cache") << "found " << std::hex << handle
                           << " in " << std::hex << lua
                           << " under id " << std::hex << id;
            return sol::object(lua, sol::ref_index(id));
        }
    }
    DEBUG("cache") << "not found " << std::hex << handle
                   << " in " << std::hex << lua;
    return sol::nullopt;
}

void removeStateCache(lua_State* lua)
{
    DEBUG("cache") << "remove Lua state " << std::hex << lua;

    std::unique_lock<std::shared_timed_mutex> lock(storageLock);
    for (auto& objPair: cacheStorage)
    {
        LuaStateStorage& storage = objPair.second;
        const auto& iter = storage.find(lua);
        if (iter != storage.end())
        {
            luaL_unref(lua, LUA_REGISTRYINDEX, iter->second);
            storage.erase(iter);
        }
    }
}

} // namespace cache

CachedGCData::~CachedGCData() {
    cache::removeFromCache(this);
}

} // namespace effil

