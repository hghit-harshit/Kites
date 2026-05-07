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
    connect(m_currentVM.get(),&VmBase::vmStateChangedSignal,this,&VMManager::vmStageChangedSignal, Qt::DirectConnection);
    connect(m_currentVM.get(),&VmBase::vmPausedAtBreakpointSignal,this,&VMManager::vmPausedAtBreakpointSignal, Qt::DirectConnection);
    // here we connect the vm state changed signal to the vm manager signal
    // and this will be further connected to the mainwindow slot to update the ui
}

void VMManager::changeVM(VMType vmType)
{
    m_currentVMType = vmType;
    m_currentVM = VMFactory::createVM(vmType);
    m_currentVM->step_delay_ = m_stepDelayMs;
    connect(m_currentVM.get(),&VmBase::vmStateChangedSignal,this,&VMManager::vmStageChangedSignal, Qt::DirectConnection);
    connect(m_currentVM.get(),&VmBase::vmPausedAtBreakpointSignal,this,&VMManager::vmPausedAtBreakpointSignal, Qt::DirectConnection);
    // create new connections for the new VM
}

void VMManager::loadProgram(const AssembledProgram &program)
{
    m_currentVM->LoadProgram(program);
}

void VMManager::run()
{
    try
    {
        m_currentVM->Run();
    }
    catch(const std::exception& e)
    {
        emit runErrorSignal(QString::fromStdString(e.what()));
    }
}

void VMManager::step()
{
    try
    {
        m_currentVM->Step();
        m_currentVM->cpi_ = m_currentVM->instructions_retired_ ? static_cast<float>(m_currentVM->cycle_s_) / static_cast<float>(m_currentVM->instructions_retired_) : 0.0f;
        m_currentVM->ipc_ = m_currentVM->cycle_s_ ? static_cast<float>(m_currentVM->instructions_retired_) / static_cast<float>(m_currentVM->cycle_s_) : 0.0f;
        m_currentVM->SetVMStateMap();
        m_currentVM->SetActiveWireNames();
        emit m_currentVM->updateCircuitStateSignal(m_currentVM->active_wires_);
        emit m_currentVM->vmStateChangedSignal(m_currentVM->vm_state_);
    }
    catch(const std::exception& e)
    {
        emit runErrorSignal(QString::fromStdString(e.what()));
    }
}   

void VMManager::debugRun()
{
    // Debug mode starts in paused/manual mode; user advances with Step.
    m_currentVM->ClearStop();
    m_currentVM->RequestPause();
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
void VMManager::undo()
{
    qDebug() << "Undoing last step";
    //m_currentVM->RequestUndo();
    m_currentVM->Undo();
}
void VMManager::redo()
{
    qDebug() << "Redoing last undone step"; 
    //m_currentVM->RequestRedo();
    m_currentVM->Redo();
}
void VMManager::setStepDelay(unsigned int delay)
{
    m_stepDelayMs = delay;
    m_currentVM->step_delay_ = delay;
}

void VMManager::setBreakpoints(const std::vector<uint64_t>& breakpoints)
{
    m_currentVM->SetBreakpoints(breakpoints);
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

QMap<QString,QVariant>& VMManager::getVMStateMap()
{
    return m_currentVM->vm_state_;
} 

uint64_t VMManager::getProgramCounter() const
{
    return m_currentVM->program_counter_;
}

float VMManager::getCPI() const
{
    return m_currentVM->cpi_;
}

float VMManager::getIPC() const
{
    return m_currentVM->ipc_;
}

unsigned int VMManager::getBranchMispredictions() const
{
    return m_currentVM->branch_mispredictions_;
}

unsigned int VMManager::getStallCycles() const
{
    return m_currentVM->stall_cycles_;
}

unsigned int VMManager::getCycles() const
{
    return m_currentVM->cycle_s_;
}

unsigned int VMManager::getInstructionsRetired() const
{
    return m_currentVM->instructions_retired_;
}

