/**
 * @file vm_factory.h
 * @brief this file contains the declaration of the VMFactory class
 * @version 0.1
 * @date 2025-10-23
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#pragma once

#include "vm/vm_base.h"
#include "vm_types.h"
#include <map>
#include <functional>
using VMContainer = std::map<VMType,std::function<std::unique_ptr<VmBase>(void)>>; 

/**
 * @brief This class is responsible for creating VM instance based on the VMType
 * this class uses the factory design pattern
 * 
 */


class VMFactory 
{
    public:
        static std::unique_ptr<VmBase> createVM(VMType type);
        template <typename T>
        static void RegisterVM(VMType type)
        {
            getInstance().m_vmContainer[type] = []() { return std::make_unique<T>(); };
        }
    private:
        static VMFactory& getInstance()
        {
            static VMFactory instance;
            return instance;
        }
        VMContainer m_vmContainer;
};
