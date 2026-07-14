/**
 * @file processor_manager.h
 * @brief This file contains the declaration of the ProcessorManager class
 */

#pragma once

#include "common/assembled_program.h"
#include "config/config.h"
#include "processor/processor_base.h"
#include "processor_types.h"
#include "profiler/profiler.h"
#include "ui/processor_tab/circuit_scene.h"
#include "processor_state.h"
#include <QObject>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace Kites
{
class RVSSProcessor;        
class RV5StageProcessorNHNF;
class RV5StageProcessorHNF; 
class RV5StageProcessorNHF; 
class RV5StageProcessorHF; 

/**
 * @brief This class is responsible for the management of the processor instance
 */
class ProcessorManager : public QObject
{
    Q_OBJECT
public:
    ProcessorManager(QObject *parent = nullptr, ProcessorType vmType = ProcessorType::RVSS);
    // only for now later we will make it so that it pull the type from config.ini
    void changeProcessor(ProcessorType vmType);
    ProcessorType getProcessorType();
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
    const Profiler* getProfiler() const;

    // gui calls these functions to get the current state of the vm
    // maybe instead we should bundle the stats in a struct and the return just 
    // that
    uint64_t getProgramCounter() const;
    float getCPI() const;
    float getIPC() const;
    unsigned int getBranchMispredictions() const;
    unsigned int getStallCycles() const;
    unsigned int getCycles() const;
    unsigned int getInstructionsRetired() const;

    /**
     * @brief Get the lines to highlight along with the text to put in the lighligted line
     */
    std::vector<std::pair<int, std::string_view>> getHighlightInfo() const;


private:
    std::unique_ptr<ProcessorBase> m_currentProcessor{};
    ProcessorType m_currentProcessorType;
    Profiler m_profiler{};
    // we need this as when we chage vm we need preserve the step delay
    unsigned int m_stepDelayMs{1000};
public slots:
    void runSlot();

signals:
    void runFinishedSignal();
    void processorStateChangedSignal();
    void processorPausedAtBreakpointSignal();
    void runErrorSignal(const QString &errorMessage);
    // std::unique_ptr<> m_instance;
};
}//namespace Kites
