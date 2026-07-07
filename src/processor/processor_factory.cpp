/**
 * @file vm_factory.cpp
 * @brief This file contains the implementation of the ProcessorFactory class
 * @version 0.1
 * @date 2025-10-23
 *
 * @copyright Copyright (c) 2025a
 *
 */

#include "processor/processor_factory.h"

namespace Kites
{
std::unique_ptr<ProcessorBase> ProcessorFactory::createVM(ProcessorType type)
{
    auto &instance = getInstance();
    auto it = instance.m_vmContainer.find(type);
    if (it != instance.m_vmContainer.end())
    {
        return it->second();
    }
    throw std::runtime_error("ProcessorFactory: Unknown ProcessorType");
}
}//namespace Kites