#pragma once

#include <cstddef>
#include <span>
#include <string>

#include "vm/cache/policies/cache_replacement_policy.h"

//This is defined in the lua souruce files,
// this is basically the runtime environment for running the custom policy scripts
//providing interface to call lua functiona and pass cache state to the lua script
struct lua_State;

class CustomPolicyEngine
{
public:
    CustomPolicyEngine();
    ~CustomPolicyEngine();

    CustomPolicyEngine(const CustomPolicyEngine&) = delete;
    CustomPolicyEngine& operator=(const CustomPolicyEngine&) = delete;

    void loadCustomPolicyScript(const std::string& path);

    size_t callChooseVictim(std::span<const CacheLineView> cacheLines,
                            const CacheRequestView& request,
                            const CacheContextView& context);

    void callOnAccess(std::span<const CacheLineView> cacheLines,
                      const CacheRequestView& request,
                      const CacheContextView& context);

    void callOnInsert(std::span<const CacheLineView> cacheLines,
                      const CacheRequestView& request,
                      const CacheContextView& context);

    void callOnEvict(std::span<const CacheLineView> cacheLines,
                     const CacheRequestView& request,
                     const CacheContextView& context);

    bool hasScript() const;

private:
    lua_State* m_state_ = nullptr;
    std::string m_script_path_;

    //These push functions convert the cache state into Lua tables and push them onto the Lua stack for the custom policy functions to consume
    static void pushLineTable(lua_State* state, const CacheLineView& line, size_t index);
    static void pushLinesTable(lua_State* state, std::span<const CacheLineView> cacheLines);
    static void pushRequestTable(lua_State* state, const CacheRequestView& request);
    static void pushContextTable(lua_State* state, const CacheContextView& context);

    
    static std::string readLuaError(lua_State* state);


    size_t callChooseVictimFunction(const char* functionName,
                                    std::span<const CacheLineView> cacheLines,
                                    const CacheRequestView& request,
                                    const CacheContextView& context,
                                    bool required);
    void callOptionalVoidFunction(const char* functionName,
                                  std::span<const CacheLineView> cacheLines,
                                  const CacheRequestView& request,
                                  const CacheContextView& context);
};