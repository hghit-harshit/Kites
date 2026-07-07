/**
 * @file vm_factory.h
 * @brief this file contains the declaration of the ProcessorFactory class
 * @version 0.1
 * @date 2025-10-23
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include "processor/processor_base.h"
#include "processor_types.h"
#include <functional>
#include <map>

namespace Kites
{
using ProcessorContainer = std::map<ProcessorType, std::function<std::unique_ptr<ProcessorBase>(void)>>;

/**
 * @brief This class is responsible for creating VM instance based on the ProcessorType
 * this class uses the factory design pattern
 *
 */

class ProcessorFactory
{
  public:
    static std::unique_ptr<ProcessorBase> createVM(ProcessorType type);
    template <typename T> static void RegisterVM(ProcessorType type)
    {
        getInstance().m_vmContainer[type] = []() { return std::make_unique<T>(); };
    }

  private:
    ProcessorFactory() = default;
    static ProcessorFactory &getInstance()
    {
        static ProcessorFactory instance;
        return instance;
    }
    ProcessorContainer m_vmContainer;
};
}//namespace Kites