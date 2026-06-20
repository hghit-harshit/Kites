#include "vm/cache/custom_policy_engine.h"

#include <stdexcept>
#include <string_view>

namespace Kites
{
extern "C"
{
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

}

// we want hasfuction to be visible only within this file, so we put it in an anonymous namespace
//  same as defining it static, but this is the more modern way to do it in C++
namespace
{
bool hasFunction(lua_State *state, const char *name)
{
    // gets the fuction and then pushes it on the state
    lua_getglobal(state, name);
    // we check if the topmost value is a fuction
    const bool isFunction = lua_isfunction(state, -1);
    // pop the function (or non function)
    lua_pop(state, 1);
    return isFunction;
}
} // namespace

CustomPolicyEngine::CustomPolicyEngine() : m_state_(luaL_newstate())
{
    if (!m_state_)
    {
        throw std::runtime_error("Failed to create Lua state for custom cache policy");
    }

    luaL_openlibs(m_state_);
}

CustomPolicyEngine::~CustomPolicyEngine()
{
    if (m_state_)
    {
        lua_close(m_state_);
        m_state_ = nullptr;
    }
}

void CustomPolicyEngine::loadCustomPolicyScript(const std::string &path)
{
    if (!m_state_)
    {
        throw std::runtime_error("Lua state is not initialized");
    }

    if (luaL_dofile(m_state_, path.c_str()) != LUA_OK)
    {
        std::string message = readLuaError(m_state_);
        lua_pop(m_state_, 1);
        throw std::runtime_error("Failed to load custom cache policy script '" + path +
                                 "': " + message);
    }

    m_script_path_ = path;
}

bool CustomPolicyEngine::hasScript() const
{
    return !m_script_path_.empty();
}

size_t CustomPolicyEngine::callChooseVictim(std::span<const CacheLineView> cacheLines,
                                            const CacheRequestView &request,
                                            const CacheContextView &context)
{
    return callChooseVictimFunction("chooseVictim", cacheLines, request, context, true);
}

void CustomPolicyEngine::callOnAccess(std::span<const CacheLineView> cacheLines,
                                      const CacheRequestView &request,
                                      const CacheContextView &context)
{
    callOptionalVoidFunction("onAccess", cacheLines, request, context);
}

void CustomPolicyEngine::callOnInsert(std::span<const CacheLineView> cacheLines,
                                      const CacheRequestView &request,
                                      const CacheContextView &context)
{
    callOptionalVoidFunction("onInsert", cacheLines, request, context);
}

void CustomPolicyEngine::callOnEvict(std::span<const CacheLineView> cacheLines,
                                     const CacheRequestView &request,
                                     const CacheContextView &context)
{
    callOptionalVoidFunction("onEvict", cacheLines, request, context);
}

void CustomPolicyEngine::pushLineTable(lua_State *state, const CacheLineView &line, size_t index)
{
    lua_newtable(state);

    lua_pushinteger(state, static_cast<lua_Integer>(index));
    lua_setfield(state, -2, "index");

    lua_pushboolean(state, line.valid);
    lua_setfield(state, -2, "valid");

    lua_pushinteger(state, static_cast<lua_Integer>(line.tag));
    lua_setfield(state, -2, "tag");

    lua_pushinteger(state, static_cast<lua_Integer>(line.age));
    lua_setfield(state, -2, "age");

    lua_pushinteger(state, static_cast<lua_Integer>(line.frequency));
    lua_setfield(state, -2, "frequency");

    lua_pushinteger(state, static_cast<lua_Integer>(line.insertTime));
    lua_setfield(state, -2, "insertTime");

    lua_pushinteger(state, static_cast<lua_Integer>(line.lastAccess));
    lua_setfield(state, -2, "lastAccess");

    lua_pushboolean(state, line.dirty);
    lua_setfield(state, -2, "dirty");
}

void CustomPolicyEngine::pushLinesTable(lua_State *state, std::span<const CacheLineView> cacheLines)
{
    lua_newtable(state);

    for (size_t i = 0; i < cacheLines.size(); ++i)
    {
        pushLineTable(state, cacheLines[i], i);
        // using i + 1 since Lua tables are 1 indexed
        lua_seti(state, -2, static_cast<lua_Integer>(i + 1));
    }
}

void CustomPolicyEngine::pushRequestTable(lua_State *state, const CacheRequestView &request)
{
    lua_newtable(state);

    lua_pushinteger(state, static_cast<lua_Integer>(request.address));
    lua_setfield(state, -2, "address");

    lua_pushinteger(state, static_cast<lua_Integer>(request.setIndex));
    lua_setfield(state, -2, "setIndex");

    lua_pushinteger(state, static_cast<lua_Integer>(request.wayIndex));
    lua_setfield(state, -2, "wayIndex");

    lua_pushinteger(state, static_cast<lua_Integer>(request.offset));
    lua_setfield(state, -2, "offset");

    lua_pushinteger(state, static_cast<lua_Integer>(request.accessSize));
    lua_setfield(state, -2, "accessSize");

    lua_pushboolean(state, request.isWrite);
    lua_setfield(state, -2, "isWrite");

    lua_pushinteger(state, static_cast<lua_Integer>(request.tag));
    lua_setfield(state, -2, "tag");
}

void CustomPolicyEngine::pushContextTable(lua_State *state, const CacheContextView &context)
{
    lua_newtable(state);

    lua_pushinteger(state, static_cast<lua_Integer>(context.setCount));
    lua_setfield(state, -2, "setCount");

    lua_pushinteger(state, static_cast<lua_Integer>(context.wayCount));
    lua_setfield(state, -2, "wayCount");

    lua_pushinteger(state, static_cast<lua_Integer>(context.blockSize));
    lua_setfield(state, -2, "blockSize");

    lua_pushinteger(state, static_cast<lua_Integer>(context.tick));
    lua_setfield(state, -2, "tick");
}

std::string CustomPolicyEngine::readLuaError(lua_State *state)
{
    const char *message = lua_tostring(state, -1);
    return message ? std::string(message) : std::string("unknown Lua error");
}

size_t CustomPolicyEngine::callChooseVictimFunction(const char *functionName,
                                                    std::span<const CacheLineView> cacheLines,
                                                    const CacheRequestView &request,
                                                    const CacheContextView &context, bool required)
{
    if (!m_state_)
    {
        throw std::runtime_error("Lua state is not initialized");
    }

    if (!hasFunction(m_state_, functionName))
    {
        if (required)
        {
            throw std::runtime_error(
                std::string("Custom policy script does not define function '") + functionName +
                "'");
        }
        return 0;
    }

    // hasFunction only validates existence; push the callback to call it with pcall.
    lua_getglobal(m_state_, functionName);

    pushLinesTable(m_state_, cacheLines);
    pushRequestTable(m_state_, request);
    pushContextTable(m_state_, context);

    // lua_pcall (protected call) will call the function
    // at the top of the stack
    // the args are no of args,no of return values and error function (0 means no error function)
    if (lua_pcall(m_state_, 3, 1, 0) != LUA_OK)
    {
        std::string message = readLuaError(m_state_);
        lua_pop(m_state_, 1);
        throw std::runtime_error(std::string("Custom policy script function '") + functionName +
                                 "' failed: " + message);
    }

    if (!lua_isinteger(m_state_, -1))
    {
        lua_pop(m_state_, 1);
        throw std::runtime_error(std::string("Custom policy function '") + functionName +
                                 "' must return an integer victim index");
    }

    const lua_Integer returnedIndex = lua_tointeger(m_state_, -1);
    lua_pop(m_state_, 1);

    if (returnedIndex < 0)
    {
        throw std::runtime_error(std::string("Custom policy function '") + functionName +
                                 "' returned a negative victim index");
    }

    if (cacheLines.empty())
    {
        throw std::runtime_error(std::string("Custom policy function '") + functionName +
                                 "' was called with no cache lines");
    }

    if (returnedIndex > static_cast<lua_Integer>(cacheLines.size()))
    {
        throw std::runtime_error(std::string("Custom policy function '") + functionName +
                                 "' returned an out-of-range victim index");
    }

    if (returnedIndex == 0)
    {
        return 0;
    }

    return static_cast<size_t>(returnedIndex - 1);
}

void CustomPolicyEngine::callOptionalVoidFunction(const char *functionName,
                                                  std::span<const CacheLineView> cacheLines,
                                                  const CacheRequestView &request,
                                                  const CacheContextView &context)
{
    if (!m_state_ || !hasFunction(m_state_, functionName))
    {
        return;
    }

    // hasFunction pops the probed value, so push the function again for pcall.
    lua_getglobal(m_state_, functionName);

    pushLinesTable(m_state_, cacheLines);
    pushRequestTable(m_state_, request);
    pushContextTable(m_state_, context);

    if (lua_pcall(m_state_, 3, 0, 0) != LUA_OK)
    {
        std::string message = readLuaError(m_state_);
        lua_pop(m_state_, 1);
        throw std::runtime_error(std::string("Custom policy script function '") + functionName +
                                 "' failed: " + message);
    }
}
}//namespace Kites