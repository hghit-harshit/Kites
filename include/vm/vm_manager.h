/**
 * @file vm_runner.h
 * @brief This file contains the declaration of the VMManager class
 */

#pragma once

#include "vm/vm_base.h"
#include "vm/rvss/rvss_vm.h"
#include "config.h"
#include "vm_asm_mw.h"
#include "vm_types.h"
#include <memory>
#include <stdexcept>
#include <sstream>
#include <QObject>



class RVSSVM; // forward declaration

/**
 * @brief This class is responsible for the management of the VM instance
 * 
 */
class VMManager : public QObject
{
    Q_OBJECT
    public:
        VMManager(QObject* parent,VMType vmType = VMType::RVSS);
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
        void debugRun();

        
        RegisterFile* getRegisterFile();
        MemoryController* getMemoryController();
        
    private:
    
    std::unique_ptr<VmBase> m_currentVM;
    VMType m_currentVMType;
    //std::unique_ptr<> m_instance;

};

    