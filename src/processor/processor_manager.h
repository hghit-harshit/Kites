/**
 * @file vm_runner.h
 * @brief This file contains the declaration of the ProcessorManager class
 */

#pragma once

#include "common/assembled_program.h"
#include "config/config.h"
#include "processor/processor_base.h"
#include "processor_types.h"
#include "profiler/profiler.h"
#include "ui/processor_tab/circuit_scene.h"
#include <QObject>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace Kites
{
class RVSSProcessor;           // forward declaration
class RV5StageProcessorNHNF; // forward declaration
class RV5StageProcessorHNF;  // forward declaration
class RV5StageProcessorNHF;  // forward declaration
class RV5StageProcessorHF;   // forward declaration

/**
 * @brief This class is responsible for the management of the VM instance
 *
 */
class ProcessorManager : public QObject
{
    Q_OBJECT
public:
    ProcessorManager(QObject *parent = nullptr, ProcessorType vmType = ProcessorType::RVSS);
    // only for now later we will make it so that it pull the type from config.ini
    // static ProcessorManager& getInstance(ProcessorType vmType = ProcessorType::RVSS)
    // {
    //     static ProcessorManager instance(vmType);
    //     return instance;
    // }singletons are bad apparently
    /**
     * @brief change the currnet VM to the given type
     *
     * @param vmType
     */
    void changeVM(ProcessorType vmType);
    ProcessorType getVMType();
    void reset();
    void loadProgram(const AssembledProgram &program);
    void run();
    void step();
    void debugRun();
    void stop();
    void pause();
    void resume();
    void undo();
    void redo();

    void setStepDelay(unsigned int delay);

    void setBreakpoints(const std::vector<uint64_t> &breakpoints);

    RegisterFile *getRegisterFile() const;
    MemoryController *getMemoryController() const;
    Kites::CircuitScene *getCircuitScene() const;
    // QMap<QString, QVariant> &getVMStateMap() const;
    Profiler* getProfiler() const;

    // these are kinda ununsed for now
    uint64_t getProgramCounter() const;
    float getCPI() const;
    float getIPC() const;
    unsigned int getBranchMispredictions() const;
    unsigned int getStallCycles() const;
    unsigned int getCycles() const;
    unsigned int getInstructionsRetired() const;

private:
    std::unique_ptr<ProcessorBase> m_currentVM{};
    ProcessorType m_currentVMType;
    std::unique_ptr<Profiler> m_profiler{};
    // we need this as when we chage vm we need preserve the step delay
    unsigned int m_stepDelayMs{1000};
public slots:
    void runSlot()
    {
        run();
        emit runFinishedSignal();
    }

signals:
    void runFinishedSignal();
    void processorStageChangedSignal(const QMap<QString, QVariant> &vmState);
    void processorPausedAtBreakpointSignal();
    void runErrorSignal(const QString &errorMessage);
    // std::unique_ptr<> m_instance;
};
}//namespace Kites
