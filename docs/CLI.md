# Using the Kites CLI

`kites-cli` is a headless build of the Kites simulator: it assembles a RISC-V
`.s` file, runs it on the chosen VM, and writes the results (registers,
disassembly, and optionally a memory dump) to disk. It needs no display and no
`QApplication` — it links the same `kites_core` the GUI uses, built with
`DISABLE_GUI` defined.

> **Status:** the CLI assembles and runs a program end-to-end and dumps
> register/disassembly/memory state. It does not yet support interactive
> debugging (breakpoints, step, undo/redo) from the command line — those are
> driven through `command_handler`, whose `ExecuteCommand` is currently a stub.

## Building

The CLI target is built automatically alongside the GUI app; no extra CMake
option is required.

```bash
cmake -B build -DCMAKE_PREFIX_PATH="path/to/Qt/6.x.x/<compiler>" .
cmake --build build --target kites_cli
```

The resulting binary is `kites-cli` (`build/kites-cli`, or
`build/<config>/kites-cli.exe` on multi-config generators like Visual Studio).

## Usage

```
kites-cli <assembly-file> [--vm <name>] [--mem-dump <hex_addr> <row_count>]...
```

| Argument       | Description                                                                          |
|----------------|---------------------------------------------------------------------------------------|
| `<assembly-file>` | Path to the `.s` file to assemble and run. Required.                              |
| `--vm <name>`  | Which VM to run on (see table below). Defaults to `rvss`.                            |
| `--mem-dump <hex_addr> <row_count>` | Dump `<row_count>` 8-byte rows of memory starting at `<hex_addr>` after the run. Repeatable — pass it multiple times to dump several regions in one invocation. |

### `--vm` values

| Name        | `ProcessorType`         | Description                                   |
|-------------|-------------------------|------------------------------------------------|
| `rvss`      | `RVSS`                  | Single-cycle (default). Supports I, M, D, F.  |
| `rv5-nh-nf` | `RV5Stage_NH_NF`        | 5-stage pipeline, no hazard detection, no forwarding. |
| `rv5-h-nf`  | `RV5Stage_H_NF`         | 5-stage pipeline, hazard detection, no forwarding. |
| `rv5-nh-f`  | `RV5Stage_NH_F`         | 5-stage pipeline, no hazard detection, forwarding. |
| `rv5-h-f`   | `RV5Stage_H_F`          | 5-stage pipeline, hazard detection, forwarding. |

The pipelined VMs currently support the I and M extensions only (D/F
pipelined support is planned).

## Example

```bash
cat > /tmp/hello.s <<'EOF'
li   a0, 42       # value to print
li   a7, 1        # SYSCALL_PRINT_INT
ecall

li   a0, 0        # exit code
li   a7, 10       # SYSCALL_EXIT
ecall
EOF

./build/kites-cli /tmp/hello.s --vm rv5-h-f
```

Sample output:

```
[Syscall output: 42]
Exited with exit code: 0
Assembled '/tmp/hello.s': 4 instruction word(s), 0 data item(s).
Run completed.
Registers   -> /tmp/vm_state/registers_dump.json
Disassembly -> /tmp/vm_state/disassembly.txt
Errors      -> /tmp/vm_state/errors_dump.json
```

Dumping a region of memory in the same run:

```bash
./build/kites-cli /tmp/hello.s --mem-dump 0 4 --mem-dump 100 2
```

adds a `Memory -> .../vm_state/memory_dump.json` line and writes both
requested regions (4 rows starting at address `0x0`, then 2 rows starting at
`0x100`) to that file.

## Program output during the run

Programs that execute `ecall` for I/O print directly to the CLI's stdout
while running, wrapped in a marker so it's easy to spot in mixed output:

```
[Syscall output: <value or string>]
```

`PRINT_INT` (`a7=1`), `PRINT_FLOAT` (`a7=2`), `PRINT_DOUBLE` (`a7=3`), and
`PRINT_STRING` (`a7=4`) all print this way; `EXIT` (`a7=10`) prints
`Exited with exit code: <code>`.

**Known limitation:** a program that hits a blocking `READ` syscall (`a7=63`)
will hang forever — the CLI doesn't yet feed terminal stdin into the VM's
input queue. Avoid programs that read from stdin until this is implemented.

## Output files

All state is written under a `vm_state/` directory rather than to stdout, so
it can be consumed by scripts or other tooling. The directory location
depends on platform:

- **Windows / Linux:** `<current working directory>/vm_state/`
- **macOS:** `~/Library/Application Support/Kites/vm_state/`

| File                    | Written                        | Contents                                                     |
|-------------------------|---------------------------------|----------------------------------------------------------------|
| `disassembly.txt`       | Always, on successful assembly  | Human-readable disassembly with address labels and raw hex words. |
| `errors_dump.json`      | Always                          | `{"errorCode": 0, "errors": []}` on success, or a list of `{line, message}` objects on assembly failure. |
| `registers_dump.json`   | Always, after the run           | CSRs, all 32 GPRs (`x0`-`x31`), and all 32 FPRs (`f0`-`f31`), each as a hex string. |
| `memory_dump.json`      | Only if `--mem-dump` was passed | One entry per 8-byte word in the requested address ranges, keyed by hex address, e.g. `"0x0000000000000000": "0x002a000000000000"`. |

### `registers_dump.json` shape

```json
{
    "control and status registers": {
        "...": "0x..."
    },
    "gp_registers": {
        "x0" : "0x0000000000000000",
        "x1" : "0x0000000000000000",
        ...
    },
    "fp_registers": {
        "f0" : "0x0000000000000000",
        ...
    }
}
```

## Exit codes

| Code | Meaning                                                             |
|------|----------------------------------------------------------------------|
| `0`  | Assembled and ran successfully.                                      |
| `1`  | Bad invocation (missing file argument, unknown `--vm` value, unknown flag) or assembly failed — see `errors_dump.json` and stderr. |
| `2`  | Assembly succeeded but the run raised a runtime error — see stderr for the message. |
| `3`  | The run succeeded but writing the `--mem-dump` output failed (e.g. malformed hex address). |

## Notes

- `--vm` and `--mem-dump` may appear in any order after the file argument,
  and `--mem-dump` may be repeated.
- Instruction step delay (used to pace the GUI's per-instruction animation)
  is forced to `0`, so the CLI runs a program at full speed regardless of the
  GUI's configured delay.
- See [../CLAUDE.md](../CLAUDE.md) for the broader architecture, and
  [TESTING.md](TESTING.md) for how the test suite (which covers the VM core
  the CLI drives) is built and run.
