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
#include "vm/rvss/rvss_vm.h"
#include "vm/rv5s_vms/rv5svm_h_f.h"
#include "vm/rv5s_vms/rv5svm_h_nf.h"
#include "vm/rv5s_vms/rv5svm_nh_f.h"
#include "vm/rv5s_vms/rv5svm_nh_nf.h"

VMManager::VMManager(QObject* parent,VMType vmType)
: QObject(parent)
{
    //first we register all the VMs

    VMFactory::RegisterVM<RVSSVM>(VMType::RVSS);
    VMFactory::RegisterVM<RV5StageVM_NH_NF>(VMType::RV5Stage_NH_NF);
    VMFactory::RegisterVM<RV5StageVM_H_NF>(VMType::RV5Stage_H_NF);
    VMFactory::RegisterVM<RV5StageVM_NH_F>(VMType::RV5Stage_NH_F);
    VMFactory::RegisterVM<RV5StageVM_H_F>(VMType::RV5Stage_H_F);
    m_currentVMType = vmType;
    m_currentVM = VMFactory::createVM(vmType);
}

void VMManager::changeVM(VMType vmType)
{
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

void VMManager::step()
{
    m_currentVM->Step();
}   

RegisterFile* VMManager::getRegisterFile()
{
    return &m_currentVM->registers_;
}

MemoryController* VMManager::getMemoryController()
{
    return &m_currentVM->memory_controller_;
}

void VMManager::reset()
{
    m_currentVM->Reset();
}

VMType VMManager::getVMType()
{
    return m_currentVMType;
}
