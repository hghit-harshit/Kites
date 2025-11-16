/**
 * @file vm_base.h
 * @brief File containing the base class for the virtual machine
 * @author Vishank Singh, https://VishankSingh
 */
#ifndef VM_BASE_H
#define VM_BASE_H

#include "memory_controller.h"
#include "alu.h"
#include "vm/registers.h"
#include "vm_asm_mw.h"
#include "ui/circuit_scene.h"
#include <vector>
#include <stack>
#include <string>
#include <filesystem>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <QWidget>
#include <QMap>
#include <QList>
#include <QString>

enum SyscallCode
{
	SYSCALL_PRINT_INT = 1,
	SYSCALL_PRINT_FLOAT = 2,
	SYSCALL_PRINT_DOUBLE = 3,
	SYSCALL_PRINT_STRING = 4,
	SYSCALL_EXIT = 10,
	SYSCALL_READ = 63,
	SYSCALL_WRITE = 64,
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

struct StepDelta
{
	uint64_t old_pc;
	uint64_t new_pc;
	std::vector<RegisterChange> register_changes;
	std::vector<MemoryChange> memory_changes;
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
	std::atomic<bool> stop_requested_ = false;
	std::mutex input_mutex_;
	std::condition_variable input_cv_;
	std::queue<std::string> input_queue_;

	std::vector<uint64_t> breakpoints_;

	uint32_t current_instruction_{};
	uint64_t program_counter_{};

	unsigned int step_delay_{1000}; //well change it later to get it from config
	unsigned int cycle_s_{};
	unsigned int instructions_retired_{};
	float cpi_{};
	float ipc_{};
	unsigned int stall_cycles_{};
	unsigned int branch_mispredictions_{};

	std::stack<StepDelta> undo_stack_;
	std::stack<StepDelta> redo_stack_;

	StepDelta current_delta_;
	std::string output_status_;

	MemoryController memory_controller_;
	RegisterFile registers_;

	alu::Alu alu_;

	QList<QString> active_wires_{};
	size_t always_active_wires_count_{};
	QMap<QString,QVariant> vm_state_{};
	// the list of wire that will be active in this cycle of vm
	// well send this to the gui to highlight those wires

	//todo make every file under kites namespace
	//make just make this a raw pointer later
	// qt is anyways handling the memory management of widgets
	std::unique_ptr<Kites::CircuitScene> circuit_scene_; // Circuit scene for visualization
	
	void LoadProgram(const AssembledProgram &program);
	uint64_t program_size_ = 0;

	uint64_t GetProgramCounter() const;
	void UpdateProgramCounter(int64_t value);

	int32_t ImmGenerator(uint32_t instruction);

	void AddBreakpoint(uint64_t val, bool is_line = true);
	void RemoveBreakpoint(uint64_t val, bool is_line = true);
	bool CheckBreakpoint(uint64_t address);

	virtual void SetActiveWireNames()  = 0;
	virtual void SetVMStateMap()  = 0;
	// void fetchInstruction();
	// void decodeInstruction();
	// void executeInstruction();
	// void memoryAccess();
	// void writeback();

	// void HandleSyscall();
	void PrintString(uint64_t address);

	virtual void Run() = 0;
	virtual void DebugRun() = 0;
	virtual void Step() = 0;
	virtual void Undo() = 0;
	virtual void Redo() = 0;
	virtual void Reset() = 0;
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
	void vmStateChangedSignal(const QMap<QString,QVariant>& vmState);
	//this will send the list of wire that have to 
	void updateCircuitStateSignal(const QList<QString>& wireList);
	
};

#endif // VM_BASE_H
