#include "processor/cache/policies/custom_policy.h"
#include "processor/cache/custom_policy_engine.h"

#include <stdexcept>

namespace Kites
{
CustomReplacementPolicy::CustomReplacementPolicy()
    : m_engine_(std::make_unique<CustomPolicyEngine>())
{
}

CustomReplacementPolicy::CustomReplacementPolicy(std::string scriptPath) : CustomReplacementPolicy()
{
    if (!scriptPath.empty())
    {
        custom_policy_script_path_ = scriptPath;
        loadScript(scriptPath);
    }
}

CustomReplacementPolicy::~CustomReplacementPolicy() = default;

void CustomReplacementPolicy::loadScript(const std::string &scriptPath)
{
    if (!m_engine_)
    {
        m_engine_ = std::make_unique<CustomPolicyEngine>();
    }

    m_engine_->loadCustomPolicyScript(scriptPath);
}

size_t CustomReplacementPolicy::chooseVictim(std::span<const CacheLineView> lines,
                                             const CacheRequestView &request,
                                             const CacheContextView &context)
{
    if (!m_engine_ || !m_engine_->hasScript())
    {
        throw std::runtime_error(
            "Custom replacement policy cannot choose a victim before a Lua script is loaded");
    }

    return m_engine_->callChooseVictim(lines, request, context);
}

void CustomReplacementPolicy::onAccess(const CacheLineView &line, const CacheRequestView &request,
                                       const CacheContextView &context)
{
    if (!m_engine_ || !m_engine_->hasScript())
    {
        return;
    }

    m_engine_->callOnAccess(std::span<const CacheLineView>(&line, 1), request, context);
}

void CustomReplacementPolicy::onInsert(const CacheLineView &line, const CacheRequestView &request,
                                       const CacheContextView &context)
{
    if (!m_engine_ || !m_engine_->hasScript())
    {
        return;
    }

    m_engine_->callOnInsert(std::span<const CacheLineView>(&line, 1), request, context);
}

void CustomReplacementPolicy::onEvict(const CacheLineView &line, const CacheRequestView &request,
                                      const CacheContextView &context)
{
    if (!m_engine_ || !m_engine_->hasScript())
    {
        return;
    }

    m_engine_->callOnEvict(std::span<const CacheLineView>(&line, 1), request, context);
}

std::string_view CustomReplacementPolicy::name() const
{
    return "CUSTOM";
}

ReplacementPolicy CustomReplacementPolicy::type() const
{
    return ReplacementPolicy::Custom;
}

std::string CustomReplacementPolicy::getScriptPath() const
{
    return custom_policy_script_path_;
}
}//namespace Kites