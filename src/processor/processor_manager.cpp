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
    // m_profiler = std::make_unique<Profiler>();

    ProcessorFactory::RegisterVM<RVSSProcessor>(ProcessorType::RVSS);
    ProcessorFactory::RegisterVM<RV5StageProcessorNHNF>(ProcessorType::RV5Stage_NH_NF);
    ProcessorFactory::RegisterVM<RV5StageProcessorHNF>(ProcessorType::RV5Stage_H_NF);
    ProcessorFactory::RegisterVM<RV5StageProcessorNHF>(ProcessorType::RV5Stage_NH_F);
    ProcessorFactory::RegisterVM<RV5StageProcessorHF>(ProcessorType::RV5Stage_H_F);
    m_currentProcessorType = vmType;
    m_currentProcessor = ProcessorFactory::createVM(vmType);
    connect(m_currentProcessor.get(), &ProcessorBase::processorStateChangedSignal, this,
            &ProcessorManager::processorStateChangedSignal, Qt::DirectConnection);
    connect(m_currentProcessor.get(), &ProcessorBase::processorPausedAtBreakpointSignal, this,
            &ProcessorManager::processorPausedAtBreakpointSignal, Qt::DirectConnection);
    // here we connect the vm state changed signal to the vm manager signal
    // and this will be further connected to the mainwindow slot to update the ui

    connect(m_currentProcessor.get(), &ProcessorBase::processorStateChangedSignal, &m_profiler,
            &Profiler::processorStateChangedSlot, Qt::DirectConnection);
}

void ProcessorManager::changeProcessor(ProcessorType vmType)
{
    m_currentProcessorType = vmType;
    m_currentProcessor = ProcessorFactory::createVM(vmType);
    m_currentProcessor->step_delay_ = m_stepDelayMs;
    connect(m_currentProcessor.get(), &ProcessorBase::processorStateChangedSignal, this,
            &ProcessorManager::processorStateChangedSignal, Qt::DirectConnection);
    connect(m_currentProcessor.get(), &ProcessorBase::processorPausedAtBreakpointSignal, this,
            &ProcessorManager::processorPausedAtBreakpointSignal, Qt::DirectConnection);
    // create new connections for the new VM
}

void ProcessorManager::loadProgram(const AssembledProgram &program)
{
    m_currentProcessor->LoadProgram(program);
}

void ProcessorManager::runSlot()
{
    run();
    emit runFinishedSignal();
}

void ProcessorManager::run()
{
    try
    {
        m_currentProcessor->Run();
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
        m_currentProcessor->Step();
        m_currentProcessor->cpi_ = m_currentProcessor->instructions_retired_
                                ? static_cast<float>(m_currentProcessor->cycle_s_) /
                                      static_cast<float>(m_currentProcessor->instructions_retired_)
                                : 0.0f;
        m_currentProcessor->ipc_ = m_currentProcessor->cycle_s_
                                ? static_cast<float>(m_currentProcessor->instructions_retired_) /
                                      static_cast<float>(m_currentProcessor->cycle_s_)
                                : 0.0f;
        m_currentProcessor->setPorcessorState();
        m_currentProcessor->SetActiveWireNames();
        emit m_currentProcessor->updateCircuitStateSignal(m_currentProcessor->active_wires_);
        emit m_currentProcessor->processorStateChangedSignal();
    }
    catch (const std::exception &e)
    {
        emit runErrorSignal(QString::fromStdString(e.what()));
    }
}

void ProcessorManager::debugRun()
{
    // Debug mode starts in paused/manual mode; user advances with Step.
    m_currentProcessor->ClearStop();
    m_currentProcessor->RequestPause();
}

void ProcessorManager::stop()
{
    m_currentProcessor->RequestStop();
}
void ProcessorManager::pause()
{
    m_currentProcessor->RequestPause();
}
void ProcessorManager::resume()
{
    m_currentProcessor->RequestResume();
}
void ProcessorManager::undo()
{
    m_currentProcessor->Undo();
}
void ProcessorManager::redo()
{
    m_currentProcessor->Redo();
}
void ProcessorManager::setStepDelay(unsigned int delay)
{
    m_stepDelayMs = delay;
    m_currentProcessor->step_delay_ = delay;
}

void ProcessorManager::setBreakpoints(const std::vector<uint64_t> &breakpoints)
{
    m_currentProcessor->SetBreakpoints(breakpoints);
}

RegisterFile *ProcessorManager::getRegisterFile() const
{
    return &m_currentProcessor->registers_;
}

MemoryController *ProcessorManager::getMemoryController() const
{
    return &m_currentProcessor->memory_controller_;
}

Kites::CircuitScene *ProcessorManager::getCircuitScene() const
{
    return m_currentProcessor->circuit_scene_.get();
}
void ProcessorManager::reset()
{
    m_currentProcessor->Reset();
}

ProcessorType ProcessorManager::getProcessorType()
{
    return m_currentProcessorType;
}

const Profiler* ProcessorManager::getProfiler() const
{
    return &m_profiler;
}

uint64_t ProcessorManager::getProgramCounter() const
{
    return m_currentProcessor->program_counter_;
}

float ProcessorManager::getCPI() const
{
    return m_currentProcessor->cpi_;
}

float ProcessorManager::getIPC() const
{
    return m_currentProcessor->ipc_;
}

unsigned int ProcessorManager::getBranchMispredictions() const
{
    return m_currentProcessor->branch_mispredictions_;
}

unsigned int ProcessorManager::getStallCycles() const
{
    return m_currentProcessor->stall_cycles_;
}

unsigned int ProcessorManager::getCycles() const
{
    return m_currentProcessor->cycle_s_;
}

unsigned int ProcessorManager::getInstructionsRetired() const
{
    return m_currentProcessor->instructions_retired_;
}
}//namespace Kites