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
    connect(m_currentVM.get(),&VmBase::vmStateChangedSignal,this,&VMManager::vmStageChangedSignal);
    connect(m_currentVM.get(),&VmBase::vmPausedAtBreakpointSignal,this,&VMManager::vmPausedAtBreakpointSignal);
    // here we connect the vm state changed signal to the vm manager signal
    // and this will be further connected to the mainwindow slot to update the ui
}

void VMManager::changeVM(VMType vmType)
{
    m_currentVMType = vmType;
    m_currentVM = VMFactory::createVM(vmType);
    connect(m_currentVM.get(),&VmBase::vmStateChangedSignal,this,&VMManager::vmStageChangedSignal);
    connect(m_currentVM.get(),&VmBase::vmPausedAtBreakpointSignal,this,&VMManager::vmPausedAtBreakpointSignal);
    // create new connections for the new VM
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

void VMManager::debugRun()
{

}

void VMManager::stop()
{
    m_currentVM->RequestStop();
}
void VMManager::pause()
{
    m_currentVM->RequestPause();
}
void VMManager::resume()
{
    //QMutexLocker locker(&m_currentVM->pause_mutex_);
    m_currentVM->RequestResume();
    //m_currentVM->pause_wait_condition_.wakeAll();
    
}
void VMManager::setStepDelay(unsigned int delay)
{
    m_currentVM->step_delay_ = delay;
}

void VMManager::setBreakpoints(const std::vector<uint64_t>& breakpoints)
{
    m_currentVM->breakpoints_ = breakpoints;
}

RegisterFile* VMManager::getRegisterFile()
{
    return &m_currentVM->registers_;
}

MemoryController* VMManager::getMemoryController()
{
    return &m_currentVM->memory_controller_;
}

Kites::CircuitScene* VMManager::getCircuitScene()
{
    return m_currentVM->circuit_scene_.get();
}
void VMManager::reset()
{   
    qDebug() << "Resetting VM";
    m_currentVM->Reset();
}

VMType VMManager::getVMType()
{
    return m_currentVMType;
}