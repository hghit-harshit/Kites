/**
 * @file vm_base.h
 * @brief File containing the base class for the virtual machine
 * @author Vishank Singh, https://VishankSingh
 */
#ifndef VM_BASE_H
#define VM_BASE_H

#include "alu.h"
#include "common/assembled_program.h"
#include "memory_controller.h"
#include "processor/processor_state.h"
#include "processor/registers.h"
#include "ui/processor_tab/circuit_scene.h"
#include <QList>
#include <QMap>
#include <QMutex>
#include <QString>
#include <QWaitCondition>
#include <QWidget>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <queue>
#include <stack>
#include <string>
#include <vector>

namespace Kites
{

enum SyscallCode
{
    SYSCALL_PRINT_INT    = 1,
    SYSCALL_PRINT_FLOAT  = 2,
    SYSCALL_PRINT_DOUBLE = 3,
    SYSCALL_PRINT_STRING = 4,
    SYSCALL_EXIT         = 10,
    SYSCALL_READ         = 63,
    SYSCALL_WRITE        = 64,
};

struct RegisterChange
{
    unsigned int reg_index;
    unsigned int reg_type; // 0 for GPR, 1 for CSR, 2 for FPR
    uint64_t old_value;
    uint64_t new_value;
};

struct MemoryChange
{
    uint64_t address;
    std::vector<uint8_t> old_bytes_vec;
    std::vector<uint8_t> new_bytes_vec;
};

/**
 * @brief Base class for the virtual machine.
 */
class VmBase : public QObject
{
    Q_OBJECT
  public:
    VmBase();
    ~VmBase() = default;

    AssembledProgram program_;
    std::atomic<bool> stop_requested_ {false};
    std::atomic<bool> pause_requested_ {false};

    QMutex pause_mutex_;
    QWaitCondition pause_wait_condition_;

    std::mutex input_mutex_;
    std::condition_variable input_cv_;
    std::queue<std::string> input_queue_;

    std::vector<uint64_t> breakpoints_;
    std::optional<uint64_t> last_breakpoint_pc_{}; // Track last PC where we paused at breakpoint
    // This is to prevent multiple pause signals when we are paused at a breakpoint and the user
    // tries to resume but the PC hasn't moved yet

    uint32_t current_instruction_{};
    uint64_t program_counter_{};

    unsigned int step_delay_{1000}; // well change it later to get it from config
    unsigned int cycle_s_{};
    unsigned int instructions_retired_{};
    double cpi_{};
    double ipc_{};
    unsigned int stall_cycles_{};
    unsigned int branch_mispredictions_{};

    std::string output_status_;

    MemoryController memory_controller_;
    RegisterFile registers_;

    alu::Alu alu_;

    QList<QString> active_wires_{};
    size_t always_active_wires_count_{};
    VMState vmState{};
    // the list of wire that will be active in this cycle of vm
    // well send this to the gui to highlight those wires


    std::unique_ptr<CircuitScene> circuit_scene_; // Circuit scene for visualization

    void LoadProgram(const AssembledProgram &program);
    uint64_t program_size_ = 0;

    uint64_t GetProgramCounter() const;
    void UpdateProgramCounter(int64_t value);

    int32_t ImmGenerator(uint32_t instruction);

    /**@brief Sets the breakpoints vector
     */
    void SetBreakpoints(const std::vector<uint64_t> &breakpoints);
    void AddBreakpoint(uint64_t val, bool is_line = true);
    void RemoveBreakpoint(uint64_t val, bool is_line = true);
    bool CheckBreakpoint(uint64_t address);

    void RequestStop();
    void RequestPause();
    void RequestResume();

    bool IsStopRequested() const;
    void ClearStop();
    virtual void SetActiveWireNames() = 0;
    virtual void SetVMStateMap() = 0;
    // void fetchInstruction();
    // void decodeInstruction();
    // void executeInstruction();
    // void memoryAccess();
    // void writeback();

    // void HandleSyscall();
    void PrintString(uint64_t address);

    virtual void Run()      = 0;
    virtual void DebugRun() = 0;
    virtual void Step()     = 0;
    virtual void Undo()     = 0;
    virtual void Redo()     = 0;
    virtual void Reset()    = 0;
    void DumpState(const std::filesystem::path &filename);

    void ModifyRegister(const std::string &reg_name, uint64_t value);
    void PushInput(const std::string &input)
    {
        std::lock_guard<std::mutex> lock(input_mutex_);
        input_queue_.push(input);
        input_cv_.notify_one();
    }

  signals:
    // vm state will have all the info like pc,cycles, control signals
    void vmStateChangedSignal(const VMState &vmState);
    // this will send the list of wire that have to
    void updateCircuitStateSignal(const QList<QString> &wireList);
    // we emit this signal when vm is paused at breakpoint
    //  so that gui can chane paued button to resume
    void vmPausedAtBreakpointSignal();
};

}  // namespace Kites

#endif // VM_BASE_H
