/**
 * @file vm_manager.cpp
 * @brief This file contains the implementation of the VMManager class
 * @version 0.1
 * @date 2025-10-23
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "processor/processor_factory.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rv5s/rv5s_processor_nh_nf.h"
#include "processor/rvss/rvss_processor.h"
#include "processor/processor_manager.h"

namespace Kites
{
ProcessorManager::ProcessorManager(QObject *parent, ProcessorType vmType) : QObject(parent)
{
    // first we register all the VMs
    m_profiler = std::make_unique<Profiler>();

    ProcessorFactory::RegisterVM<RVSSProcessor>(ProcessorType::RVSS);
    ProcessorFactory::RegisterVM<RV5StageProcessorNHNF>(ProcessorType::RV5Stage_NH_NF);
    ProcessorFactory::RegisterVM<RV5StageProcessorHNF>(ProcessorType::RV5Stage_H_NF);
    ProcessorFactory::RegisterVM<RV5StageProcessorNHF>(ProcessorType::RV5Stage_NH_F);
    ProcessorFactory::RegisterVM<RV5StageProcessorHF>(ProcessorType::RV5Stage_H_F);
    m_currentVMType = vmType;
    m_currentVM = ProcessorFactory::createVM(vmType);
    connect(m_currentVM.get(), &ProcessorBase::processorStateChangedSignal, this,
            &ProcessorManager::processorStageChangedSignal, Qt::DirectConnection);
    connect(m_currentVM.get(), &ProcessorBase::processorPausedAtBreakpointSignal, this,
            &ProcessorManager::processorPausedAtBreakpointSignal, Qt::DirectConnection);
    // here we connect the vm state changed signal to the vm manager signal
    // and this will be further connected to the mainwindow slot to update the ui

    connect(m_currentVM.get(), &ProcessorBase::processorStateChangedSignal, m_profiler.get(),
            &Profiler::onVMStateChanged, Qt::DirectConnection);
}

void ProcessorManager::changeVM(ProcessorType vmType)
{
    m_currentVMType = vmType;
    m_currentVM = ProcessorFactory::createVM(vmType);
    m_currentVM->step_delay_ = m_stepDelayMs;
    connect(m_currentVM.get(), &ProcessorBase::processorStateChangedSignal, this,
            &ProcessorManager::processorStageChangedSignal, Qt::DirectConnection);
    connect(m_currentVM.get(), &ProcessorBase::processorPausedAtBreakpointSignal, this,
            &ProcessorManager::processorPausedAtBreakpointSignal, Qt::DirectConnection);
    // create new connections for the new VM
}

void ProcessorManager::loadProgram(const AssembledProgram &program)
{
    m_currentVM->LoadProgram(program);
}

void ProcessorManager::run()
{
    try
    {
        m_currentVM->Run();
    }
    catch (const std::exception &e)
    {
        emit runErrorSignal(QString::fromStdString(e.what()));
    }
}

void ProcessorManager::step()
{
    try
    {
        m_currentVM->Step();
        m_currentVM->cpi_ = m_currentVM->instructions_retired_
                                ? static_cast<float>(m_currentVM->cycle_s_) /
                                      static_cast<float>(m_currentVM->instructions_retired_)
                                : 0.0f;
        m_currentVM->ipc_ = m_currentVM->cycle_s_
                                ? static_cast<float>(m_currentVM->instructions_retired_) /
                                      static_cast<float>(m_currentVM->cycle_s_)
                                : 0.0f;
        m_currentVM->SetVMStateMap();
        m_currentVM->SetActiveWireNames();
        emit m_currentVM->updateCircuitStateSignal(m_currentVM->active_wires_);
        emit m_currentVM->processorStateChangedSignal(m_currentVM->vmState);
    }
    catch (const std::exception &e)
    {
        emit runErrorSignal(QString::fromStdString(e.what()));
    }
}

void ProcessorManager::debugRun()
{
    // Debug mode starts in paused/manual mode; user advances with Step.
    m_currentVM->ClearStop();
    m_currentVM->RequestPause();
}

void ProcessorManager::stop()
{
    m_currentVM->RequestStop();
}
void ProcessorManager::pause()
{
    m_currentVM->RequestPause();
}
void ProcessorManager::resume()
{
    // QMutexLocker locker(&m_currentVM->pause_mutex_);
    m_currentVM->RequestResume();
    // m_currentVM->pause_wait_condition_.wakeAll();
}
void ProcessorManager::undo()
{
    qDebug() << "Undoing last step";
    // m_currentVM->RequestUndo();
    m_currentVM->Undo();
}
void ProcessorManager::redo()
{
    qDebug() << "Redoing last undone step";
    // m_currentVM->RequestRedo();
    m_currentVM->Redo();
}
void ProcessorManager::setStepDelay(unsigned int delay)
{
    m_stepDelayMs = delay;
    m_currentVM->step_delay_ = delay;
}

void ProcessorManager::setBreakpoints(const std::vector<uint64_t> &breakpoints)
{
    m_currentVM->SetBreakpoints(breakpoints);
}

RegisterFile *ProcessorManager::getRegisterFile() const
{
    return &m_currentVM->registers_;
}

MemoryController *ProcessorManager::getMemoryController() const
{
    return &m_currentVM->memory_controller_;
}

Kites::CircuitScene *ProcessorManager::getCircuitScene() const
{
    return m_currentVM->circuit_scene_.get();
}
void ProcessorManager::reset()
{
    qDebug() << "Resetting VM";
    m_currentVM->Reset();
}

ProcessorType ProcessorManager::getVMType()
{
    return m_currentVMType;
}

// QMap<QString, QVariant>& ProcessorManager::getVMStateMap() const
// {
//     return m_currentVM->vmState;
// }

Profiler* ProcessorManager::getProfiler() const
{
    return m_profiler.get();
}

uint64_t ProcessorManager::getProgramCounter() const
{
    return m_currentVM->program_counter_;
}

float ProcessorManager::getCPI() const
{
    return m_currentVM->cpi_;
}

float ProcessorManager::getIPC() const
{
    return m_currentVM->ipc_;
}

unsigned int ProcessorManager::getBranchMispredictions() const
{
    return m_currentVM->branch_mispredictions_;
}

unsigned int ProcessorManager::getStallCycles() const
{
    return m_currentVM->stall_cycles_;
}

unsigned int ProcessorManager::getCycles() const
{
    return m_currentVM->cycle_s_;
}

unsigned int ProcessorManager::getInstructionsRetired() const
{
    return m_currentVM->instructions_retired_;
}
}//namespace Kites