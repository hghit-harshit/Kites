#pragma once

#include <memory>
#include <string>

#include "cache_replacement_policy.h"

namespace Kites
{
class CustomPolicyEngine;

class CustomReplacementPolicy : public CacheReplacementPolicy
{
  public:
    CustomReplacementPolicy();
    explicit CustomReplacementPolicy(std::string scriptPath);
    ~CustomReplacementPolicy() override;

    CustomReplacementPolicy(const CustomReplacementPolicy &) = delete;
    CustomReplacementPolicy &operator=(const CustomReplacementPolicy &) = delete;

    size_t chooseVictim(std::span<const CacheLineView> lines, const CacheRequestView &request,
                        const CacheContextView &context) override;

    void onAccess(const CacheLineView &line, const CacheRequestView &request,
                  const CacheContextView &context) override;

    void onInsert(const CacheLineView &line, const CacheRequestView &request,
                  const CacheContextView &context) override;

    void onEvict(const CacheLineView &line, const CacheRequestView &request,
                 const CacheContextView &context) override;

    std::string_view name() const override;

    ReplacementPolicy type() const override;

    void loadScript(const std::string &scriptPath);

    std::string getScriptPath() const;

  private:
    std::unique_ptr<CustomPolicyEngine> m_engine_;
    std::string custom_policy_script_path_;
};
}//namespace Kites