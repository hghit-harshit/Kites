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
    void changeProcessor(ProcessorType vmType);
    ProcessorType getProcessorType();
    void reset();
    void loadProgram(const AssembledProgram &program);
    void loadProgram(const std::string &sourceText);
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
    CircuitScene *getCircuitScene() const;
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
private:
    void updateEditorHighlight(const ProcessorState &processorState);
    // Maps the current program counter back to a 1-based editor source line,
    // or -1 if it isn't in the current program's mapping.
    int resolveCurrentSourceLine() const;

    AssembledProgram m_currentProgram{};
    std::unique_ptr<ProcessorBase> m_currentProcessor{};
    ProcessorType m_currentProcessorType;
    Profiler m_profiler{};
    // we need this as when we chage vm we need preserve the step delay
    unsigned int m_stepDelayMs{1000};
public slots:
    void runSlot();
    void processorClockedSlot(const ProcessorState &processorState);

signals:
    void runFinishedSignal();
    void processorStateChangedSignal();
    void processorPausedAtBreakpointSignal();
    // sourceLine is 1-based, or -1 if the failing PC couldn't be mapped back to a source line.
    void runErrorSignal(const QString &errorMessage, int sourceLine);
    void updateDisassemblySignal(const QString &disassemblyText);
    void updateEditorHighlightSignal(const std::vector<std::pair<int,std::string>> &editorLines, 
        const std::vector<std::pair<int,std::string>> &disassemblyLines);
    // std::unique_ptr<> m_instance;
};
}//namespace Kites
