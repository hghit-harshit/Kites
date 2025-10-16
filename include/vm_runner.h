/**
 * @file vm_runner.h
 * @brief This file contains the declaration of the VMRunner class
 * @author Vishank Singh, https://github.com/VishankSingh
 */

#ifndef VM_RUNNER_H
#define VM_RUNNER_H

#include "vm/vm_base.h"
#include "vm/rvss/rvss_vm.h"
#include "config.h"
#include "vm_asm_mw.h"

#include <memory>
#include <stdexcept>
#include <sstream>

// inline std::unique_ptr<VmBase> createVM(vm_config::VmTypes vmType) {
//   if (vmType==vm_config::VmTypes::SINGLE_STAGE) {
//     return std::make_unique<RVSSVM>();
//   }

//   return nullptr;
// }

// class VMRunner {
//   std::unique_ptr<VmBase> vm_;
//  public:
//   VMRunner() {
//     vm_config::VmTypes vmType = vm_config::GetVmType();
//     vm_ = createVM(vmType);
//   }

//   ~VMRunner() = default;

//   void LoadProgram(const AssembledProgram &program) {
//     vm_->LoadProgram(program);
//   }

//   void Custom() {
//     if (auto vmInstance = dynamic_cast<RVSSVM *>(vm_.get())) {
//       vmInstance->PrintType();
//     } else {
//       throw std::runtime_error("VM not initialized.");
//     }
//   }

//   void Step() {
//     vm_->Step();
//   }

// };

#endif // VM_RUNNER_H
