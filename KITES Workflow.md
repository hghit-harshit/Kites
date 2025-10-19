## **Code Understanding Workflow (Dependency-Based)**

### **Stage 1: The Building Blocks (Data & Core Services)**

Focus on what the VM stores and the fundamental operations it performs. These components are independent of the execution cycle.

1. **`registers.cpp` and `registers.h`:**   
   * **Goal:** Understand the state storage for the CPU.  
   * **Focus:** How `GPR`, `FPR`, and `CSR` values are stored, read, and written. Note the constraints (e.g., register `x0` always returns zero).  
2. **`alu.cpp` and `alu.h`:**  
   * **Goal:** Understand the computational engine.  
   * **Focus:** How `AluOp` is mapped to integer and floating-point functions (`execute` vs. `fpexecute`/`dfpexecute`). Pay attention to how overflow, rounding, and status flags are handled.  
3. **`main_memory.h` and `main_memory.cpp`:**   
   **The memory\_contoller.h is just a wrapper on main memory so that cache can be implemented in the middle.**  
   * **Goal:** Understand the memory model.  
   * **Focus:** The sparse memory structure (`blocks_` map). Learn how addresses are translated into a **Block Index** and **Offset**. Understand the difference between `Read(address)` (single byte) and `ReadWord(address)` (multi-byte generic read/write logic for Little-Endian).

	**`Also checked the mmio.h, looks useless as of now, like, ig kept for further use maybe`**.

4. **`vm_base.cpp` and `vm_base.h`:**  
   * **Goal:** Understand common VM utilities and state management.  
   * **Focus:** How `program_counter_` is initialized and updated. Crucially, review the `ImmGenerator` function to see how immediate values for all RISC-V instruction formats (I, S, B, U, J) are parsed and sign-extended.

### **Stage 2: Instruction Control Flow**

Focus on how instructions are interpreted and how execution is managed.

1. **`rvss_control_unit.cpp` and `control_unit_base.cpp`:**  
   * **Goal:** Understand the decoder logic.  
   * **Focus:** Trace the `SetControlSignals` function. Understand how it maps the instruction's opcode/funct fields to the required control signals (`RegWrite`, `MemRead`, `AluSrc`, etc.). This is the blueprint for the entire datapath.  
2. **`command_handler.cpp` and `command_handler.h`:**  
   * **Goal:** Understand user input and control.  
   * **Focus:** How commands like `step`, `run_debug`, and `modify_register` are parsed from text input.

### **Stage 3: The Execution Engine (Putting it Together)**

Focus on the main driver of the simulation.

1. **`rvss_vm.cpp` and `vm_runner.h`:**  
   * **Goal:** Understand the single-stage execution loop.  
   * **Focus:**  
     * **The 5-Step Cycle:** Trace the `Fetch`, `Decode`, `Execute`, `WriteMemory`, and `WriteBack` functions sequentially. Note how data is passed between these implicit "stages" via temporary member variables (like `execution_result_`).  
     * **State History:** Analyze the `Undo` and `Redo` functions to see how register and memory changes are recorded in the `StepDelta` structure. This is essential for debugging tools.  
     * **Syscalls:** Review `HandleSyscall` to see how the VM interacts with the outside world (I/O, exit).

