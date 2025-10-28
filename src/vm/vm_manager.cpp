/**
 * @file vm_manager.cpp
 * @brief This file contains the implementation of the VMManager class
 * @version 0.1
 * @date 2025-10-23
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "vm/vm_manager.h"
#include "vm/vm_factory.h"

VMManager::VMManager(QObject* parent,VMType vmType)
: QObject(parent)
{
    //first we register all the VMs

    VMFactory::RegisterVM<RVSSVM>(VMType::RVSS);
    m_currentVMType = vmType;
    m_currentVM = VMFactory::createVM(vmType);
}

void VMManager::loadProgram(const AssembledProgram &program)
{
    m_currentVM->LoadProgram(program);
}

void VMManager::run()
{
    m_currentVM->Run();
}

RegisterFile* VMManager::getRegisterFile()
{
    return &m_currentVM->registers_;
}

void VMManager::reset()
{
    m_currentVM->Reset();
}