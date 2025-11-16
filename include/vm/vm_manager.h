/**
 * @file vm_runner.h
 * @brief This file contains the declaration of the VMManager class
 */

#pragma once

#include "vm/vm_base.h"
#include "config.h"
#include "vm_asm_mw.h"
#include "vm_types.h"
#include "ui/circuit_scene.h"
#include <memory>
#include <stdexcept>
#include <sstream>
#include <QObject>



class RVSSVM; // forward declaration
class RV5StageVM_NH_NF; // forward declaration
class RV5StageVM_H_NF; // forward declaration
class RV5StageVM_NH_F; // forward declaration
class RV5StageVM_H_F; // forward declaration

/**
 * @brief This class is responsible for the management of the VM instance
 * 
 */
class VMManager : public QObject
{
    Q_OBJECT
    public:
        VMManager(QObject* parent = nullptr,VMType vmType = VMType::RVSS);
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

        void setStepDelay(unsigned int delay);
        
        RegisterFile* getRegisterFile();
        MemoryController* getMemoryController();
        Kites::CircuitScene* getCircuitScene();

    private:
    std::unique_ptr<VmBase> m_currentVM;
    VMType m_currentVMType;

    public slots:
        void runSlot()
        {
            run();
            emit runFinishedSignal();
        }
    signals:
        void runFinishedSignal();
        void vmStageChangedSignal(const QMap<QString,QVariant>& vmState);
    //std::unique_ptr<> m_instance;

};

    