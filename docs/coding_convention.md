# Coding Conventions
## IMPORTANT BUGS
* DO NOT REFORMAT THE UI FILES IT WILL BREAK THINGS UNEXPECTEDLY
## Naming

| Item              | Convention    | Example                       |
| ----------------- | ------------- | ----------------------------- |
| Folders           | `snake_case`  | `profiler_panel`              |
| Files             | `snake_case`  | `execution_profile_model.cpp` |
| Classes / Structs | `PascalCase`  | `ExecutionProfileModel`       |
| Functions         | `camelCase`   | `updateExecutionCounts()`     |
| Local Variables   | `camelCase`   | `executionCount`              |
| Member Variables  | `m_camelCase` | `m_executionCount`            |
| Namespaces        | `snake_case`  | `namespace instruction_set`   |
| Enums             | `PascalCase`  | `enum class InstructionTypes` |
| Enum Values       | `PascalCase`  | `PipelineState::RType`        |
| Macros            | `ALL_CAPS`    | `KITES_VERSION`               |

* When naming variables that store count of something prefer nameCount 
  eg. prefer m_wayCount instead of m_numWays

## Exceptions
* The Enum Value for AluOP are named using ALL_CAPS as they align better with
their names in the official ISA Documentation.
* Namespace enveloping the whole codebase is Kites, but all other namespaces
  should follow the coding convention.

## Directory Structure

* Organize code by **feature/domain**, not by file type.
* Keep related `.h`, `.cpp`, and `.ui` files together.
* GUI code belongs under `src/ui/`.
* Shared GUI components belong under `src/ui/common/`.

## Formatting

* Use **4 spaces** for indentation.
* Do **not** use tabs.
* Opening braces should be placed on a new line.

Example:

```cpp
if (condition)
{
    doSomething();
}
```
