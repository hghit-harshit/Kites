# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Kites is a RISC-V simulator and assembly code editor (Qt6/C++20). It supports I, M, D, F extensions in single-cycle mode and I, M extensions in 5-stage pipelined mode (D/F pipelined support planned).

## Build

Requires Qt >= 6.9.

```bash
cmake -B build -DCMAKE_PREFIX_PATH="path/to/Qt/6.x.x/<compiler>" .
cmake --build build
```

Useful CMake options (pass as `-D<OPTION>=ON/OFF`):
- `ENABLE_TESTS` — build the `tests` GoogleTest binary (fetched via FetchContent, not a system dependency)
- `ENABLE_COVERAGE` — adds gcovr coverage instrumentation, requires GCC/Clang and `gcovr` on PATH; adds a `coverage` target
- `LOG_PANEL` — enables a Qt log panel window in `main.cpp` for debugging
- `DISABLE_GUI` (tests only, default ON) — excludes GUI-dependent code paths in the test binary
- `VM_DEBUG_PRINTS` (tests only, default OFF) — enables VM debug prints during tests

## Tests

```bash
cmake -B build -DCMAKE_PREFIX_PATH="..." -DENABLE_TESTS=ON .
cmake --build build --target tests
cd build && ./tests --gtest_color=yes
```

Run a single test or filter:
```bash
./tests --gtest_filter=RV5StageVM_H_NF_TEST.l_b_forwarding
```

There is also a `test_run` custom target (`cmake --build build --target test_run`) that runs `./tests` and never fails the build even if tests fail (useful in CI pipelines that gate separately).

## Coding conventions (from `docs/coding_convention.md`)

- **Do not reformat `.ui` files** — this breaks things unexpectedly.
- Naming: folders/files `snake_case`, classes/structs `PascalCase`, functions/local vars `camelCase`, member vars `m_camelCase`, namespaces `snake_case`, enums and enum values `PascalCase`, macros `ALL_CAPS`.
  - Exception: enum values that map directly to hardware/ISA structures (e.g. `AluOP`, `PipelineStage`, `VMType`) may use `ALL_CAPS`.
  - Count variables: prefer `m_wayCount` over `m_numWays`.
- Root namespace for the whole codebase is `Kites`; nested namespaces still follow snake_case.
- Organize code by feature/domain, not file type; keep `.h`/`.cpp`/`.ui` for a feature together. GUI code lives under `src/ui/`; shared GUI widgets under `src/ui/common/`.
- 4-space indent, no tabs, opening braces on a new line (Allman style — see `.clang-format`, based on LLVM with `BreakBeforeBraces: Allman`, 100-col limit).

## Architecture

### Assembler (`src/assembler/`)
Pipeline: `lexer` → `tokens` → `parser` → `code_generator` (emits an `AssembledProgram`, see `src/common/assembled_program.h`), with `errors.{h,cpp}` for diagnostics and `assembler.{h,cpp}` orchestrating the whole flow. Instruction-format-specific parsing (per RISC-V extension) lives in `src/assembler/parse_formats/`: `csr_formats`, `f_d_formats`, `i_m_formats`, `pseudo_formats`.

### Processor / VM core (`src/processor/`)
- `processor_base.h` — `ProcessorBase : public QObject`, the abstract VM interface (`Run`/`DebugRun`/`Step`/`Undo`/`Redo`/`Reset`, all pure virtual).
- `processor_types.h` — `enum class ProcessorType { RVSS, RV5Stage_NH_NF, RV5Stage_H_NF, RV5Stage_NH_F, RV5Stage_H_F }`.
- `processor_factory.h` — factory (`ProcessorFactory::createVM(type)`) that instantiates the right VM implementation.
- `processor_manager.{h,cpp}` — `ProcessorManager : public QObject` owns/drives the active VM and is the bridge to the UI via Qt signals (`runFinishedSignal`, `runErrorSignal`, `updateDisassemblySignal`, `updateEditorHighlightSignal`, `processorPausedAtBreakpointSignal`, ...).
- `rvss/` — single-cycle implementation: `RVSSProcessor : public ProcessorBase` + `rvss_control_unit`.
- `rv5s/` — 5-stage pipeline implementation. Shared base `RV5StageVM_Base : public ProcessorBase` (pipeline hooks `pipeline_fetch/execute/execute_float/execute_double`, `handle_syscall`) plus `rv5s_hdu` (hazard detection unit) and `rv5s_control_unit`. Four concrete VMs combine Hazard-detection (H/NH) x Forwarding (F/NF) — `rv5s_processor_h_f`, `h_nf`, `nh_f`, `nh_nf` — each mapping 1:1 to a `ProcessorType` above.
- Shared infra used by both modes: `alu`, `registers`, `main_memory`, `memory_controller`/`memory_device`/`memory_block`/`mmio_devices`, `pipeline_registers.h`, `processor_state.h`.
- `cache/` — `cache.{h,cpp}` + `cacheconfig.h` implement the cache model; `cache/policies/` has pluggable eviction strategies behind `cache_replacement_policy.h` (`lru`, `fifo`, `custom_policy`).

### Lua custom cache replacement policies
Documented in `docs/CUSTOM_REPLACEMENT_POLICY.md`. Implemented by `src/processor/cache/policies/custom_policy.{h,cpp}` (`CustomReplacementPolicy`, implements the standard `chooseVictim`/`onAccess`/`onInsert`/`onEvict` interface) which delegates to `src/processor/cache/custom_policy_engine.{h,cpp}`'s `CustomPolicyEngine`. That engine owns a `lua_State*` (bundled Lua 5.5 sources are vendored in `external/lua-5.5.0/` and compiled directly into the app/tests targets), loads the user's Lua script, and marshals cache state into Lua tables (`pushLineTable`/`pushLinesTable`/`pushRequestTable`/`pushContextTable`) before invoking the matching Lua callback per cache lifecycle event.

### Command handler (`src/command_handler/`)
Not the undo/redo GoF pattern — a text-command dispatcher into the simulator. `command_handler.h` defines `CommandType` (LOAD, RUN, STOP, STEP, UNDO, REDO, DUMP_MEMORY, ADD_BREAKPOINT, ...), a `Command{type, args}` struct, `ParseCommand(string)`, and `ExecuteCommand(command, RVSSProcessor&)`.

### UI (`src/ui/`)
`ui/mainwindow/` — `MainWindow : public QMainWindow` owns a `QStackedWidget* m_stackedTabs` of `KitesTab*` (Editor, Memory, Processor, Cache, Compiler, Profiler — indexed by `TabIndex`, see `src/utils/to_index.h`) and a `ProcessorManager*`. The backend↔UI relationship is a bidirectional Qt signal/slot bridge: UI actions (run/step/undo/pause) connect to `ProcessorManager` slots (`runSlot`, `step`, `undo`, ...), and `ProcessorManager` signals drive UI updates back (editor highlighting, disassembly view, breakpoint pause state, etc). Feature-specific tabs live in `ui/editor_tab`, `ui/processor_tab`, `ui/cache_tab`, `ui/memory_tab`, `ui/compiler_tab`, `ui/profiler_tab`, plus `ui/dialogs`, `ui/theme`, `ui/register_table`, `ui/log_panel`, `ui/common` (shared widgets).

### Other modules
- `src/elf_util` — ELF file parsing/loading for program binaries.
- `src/custom_pseudo_manager` — manages user-defined pseudo-instructions.
- `src/config` — app/VM configuration (`VmTypes`, `VmConfig`) and settings persistence.
- `src/profiler` — `Profiler : public QObject`, collects execution stats for the profiler tab.
- `src/common` — shared data types (`assembled_program.h`, `instructions.{h,cpp}`, `instruction_types.h`, `globals.{h,cpp}`, `rounding_modes.h`, `debug_colors.h`).
- `src/utils` — misc helpers (dumping registers/disassembly/errors, config file setup, `to_index.h` enum→array-index helper used throughout the UI).

### Resources
Qt resources are split across multiple `.qrc` files registered in `CMakeLists.txt`: `resources/resources.qrc`, `resources/circuit_designs/circuit_designs.qrc`, `resources/icons/icons.qrc`, `resources/fonts/fonts.qrc`, `resources/themes/themes.qrc`, `resources/instructions/instructions.qrc`.

## Tests (`test/`)
Single GoogleTest binary; `test/test_main.cpp` is the entry point. Test files are organized by concern:
- `rv5s_test.cpp`, `rv5s_h_f_test.cpp`, `rv5s_h_nf_test.cpp`, `rv5s_nh_f_test.cpp`, `rv5s_nh_nf_fp_test.cpp`, `rv5s_nh_nf_fp_memory_test.cpp`, `rv5s_f_extension_test.cpp`, `rv5s_fp_test.cpp` — cover the four pipelined VM variants and their F/D extension behavior.
- `rv5s_undo_redo_test.cpp` — snapshots VM state (PC, cycles, GPRs, watched memory) to verify Undo/Redo correctness.
- `cache_test.cpp`, `cache_policies_test.cpp`, `cache_matrix_mul_l1_l2_test.cpp`, `custom_policy_test.cpp` — cache subsystem, including the Lua custom policy engine.
- `pseudo_inst_test.cpp` — assembler pseudo-instruction expansion.
