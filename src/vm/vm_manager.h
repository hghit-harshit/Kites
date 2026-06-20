/**
 * @file vm_runner.h
 * @brief This file contains the declaration of the VMManager class
 */

#pragma once

#include "config/config.h"
#include "ui/processor/circuit_scene.h"
#include "vm/vm_base.h"
#include "profiler/profiler.h"
#include "common/assembled_program.h"
#include "vm_types.h"
#include <QObject>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace Kites
{
class RVSSVM;           // forward declaration
class RV5StageVM_NH_NF; // forward declaration
class RV5StageVM_H_NF;  // forward declaration
class RV5StageVM_NH_F;  // forward declaration
class RV5StageVM_H_F;   // forward declaration

/**
 * @brief This class is responsible for the management of the VM instance
 *
 */
class VMManager : public QObject
{
    Q_OBJECT
  public:
    VMManager(QObject *parent = nullptr, VMType vmType = VMType::RVSS);
    // only for now later we will make it so that it pull the type from config.ini
    // static VMManager& getInstance(VMType vmType = VMType::RVSS)
    // {
    //     static VMManager instance(vmType);
    //     return instance;
    // }singletons are bad apparently
    /**
     * @brief change the currnet VM to the given type
     *
     * @param vmType
     */
    void changeVM(VMType vmType);
    VMType getVMType();
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
    QMap<QString, QVariant> &getVMStateMap() const;
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
    std::unique_ptr<VmBase> m_currentVM{};
    VMType m_currentVMType;
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
    void vmStageChangedSignal(const QMap<QString, QVariant> &vmState);
    void vmPausedAtBreakpointSignal();
    void runErrorSignal(const QString &errorMessage);
    // std::unique_ptr<> m_instance;
};
}//namespace Kites
