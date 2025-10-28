/**
 * @file vm_factory.cpp
 * @brief This file contains the implementation of the VMFactory class
 * @version 0.1
 * @date 2025-10-23
 * 
 * @copyright Copyright (c) 2025a
 * 
 */

#include "vm/vm_factory.h"

std::unique_ptr<VmBase> VMFactory::createVM(VMType type)
{
    auto& instance = getInstance();
    auto it = instance.m_vmContainer.find(type);
    if (it != instance.m_vmContainer.end()) {
        return it->second();
    }
    throw std::runtime_error("VMFactory: Unknown VMType");
}