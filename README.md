# med -- Memory and File Editor - compatible with Linux, MacOS, and AmigaOS

A portable memory editor and debugger for AmigaOS (68k), Linux, and macOS.
Provides interactive and command-line access to memory inspection, modification,
testing, disassembly, and CPU operations.

Version 1.2. Designed by Chris Hooper, 2020.

## Features

- Display, change, copy, and compare memory at byte, word, long, quad, oct,
  and hex widths
- Pattern fill and memory testing with multiple test patterns
- 68k disassembler (AmigaOS) and x86-64/x86-32 disassemblers (Linux/macOS)
- CPU identification, register access, and fault injection (AmigaOS)
- Physical memory access bypassing MMU/cache (AmigaOS)
- PCI configuration space access (Linux via sysfs, AmigaOS via Zorro PCI bridges)
- File-backed memory spaces
- Expression evaluator with arithmetic, bitwise, logical, and comparison
  operators
- Interactive REPL with command history and line editing (GNU Readline)
- Single-command mode for scripting
- Command chaining with `;`, `&&`, and `||`

## Building

### Native (Linux/macOS)

```
make
```

Produces `objs/med` (or `objs.mac/med` on macOS).

### AmigaOS cross-compile (GCC)

```
make -f Makefile.amiga
```

Produces `objs-amiga-gcc/med`. Requires `m68k-amigaos-gcc` in PATH. Targets
68060 CPU with clib2 runtime.

### AmigaOS native (DICE)

```
dmake
```

Uses the DICE C compiler natively on AmigaOS. Produces `objs-amiga/med`.

## Usage

**Interactive mode** -- run `med` with no arguments to enter a REPL:

```
$ med
med> d 0 40
med> quit
```

**Command mode** -- pass a command as arguments to execute it and exit:

```
$ med d 0 40
$ med version
```

Multiple commands can be chained with `;`, `&&` (stop on failure), or `||`
(stop on success).

## Command Reference

| Command   | Syntax                                        | Description                          |
|-----------|-----------------------------------------------|--------------------------------------|
| `d`       | `d[bwlqohRS] <addr> [<len>]`                 | Display/dump memory                  |
| `c`       | `c[bwlqohS] <addr> <value...>`                | Change/write memory                  |
| `copy`    | `copy[bwlqoh] <saddr> <daddr> <len>`          | Copy memory                          |
| `comp`    | `comp[bwlqoh] <addr1> <addr2> <len>`          | Compare memory                       |
| `patt`    | `patt[bwlqoh] <addr> <len> <pattern>`         | Pattern fill memory                  |
| `test`    | `test[bwlqoh] <addr> <len> <mode> [<passes>]` | Test memory                          |
| `disas`   | `disas [<addr>] [<count>] [mit\|mot]`         | Disassemble instructions             |
| `cpu`     | `cpu type\|reg\|fault\|spin`                  | CPU operations (AmigaOS)             |
| `time`    | `time cmd <cmd>` or `time now`                | Time commands; show clock (Linux/macOS) |
| `loop`    | `loop <count> <cmd>`                          | Execute a command multiple times     |
| `delay`   | `delay <time> [s\|m\|h\|ms\|us]`             | Pause execution                      |
| `echo`    | `echo <text>`                                 | Display text                         |
| `ignore`  | `ignore <cmd>`                                | Execute command, ignore return code  |
| `history` | `history`                                     | Show command history                 |
| `help`    | `help [<cmd>]` (alias: `?`)                   | Display help                         |
| `version` | `version`                                     | Show version                         |
| `quit`    | `quit` or `q`                                 | Exit the program                     |

## Width Modifiers

Most memory commands accept a width suffix immediately after the command name:

| Suffix | Width    | Bytes |
|--------|----------|-------|
| `b`    | byte     | 1     |
| `w`    | word     | 2     |
| `l`    | long     | 4     |
| `q`    | quad     | 8     |
| `o`    | oct      | 16    |
| `h`    | hex      | 32    |

The default width is byte if no suffix is given.

## Display Flags

The `d` command accepts additional flag suffixes:

| Flag | Effect                                        |
|------|-----------------------------------------------|
| `A`  | Suppress ASCII column in output               |
| `N`  | No output (perform the read but discard data) |
| `R`  | Raw output (no address or ASCII columns)      |
| `S`  | Swap bytes (endian) for display               |
| `SS` | Swap ASCII display (endian)                   |

Flags can be combined with width modifiers, e.g. `dlA` for long-width display
without ASCII.

## Address Space Prefixes

Address arguments can be prefixed with a space identifier to target different
backends:

| Prefix | Backend                                     | Platform       |
|--------|---------------------------------------------|----------------|
| `phys` | Physical memory (bypasses MMU/cache)        | AmigaOS        |
| `pci`  | PCI configuration space                     | Linux, AmigaOS |
| `file` | File-backed access                          | All            |

Example: `d phys 0x07000000 64` dumps 64 bytes of physical memory.

## Pattern Types

The `patt` command accepts these pattern names:

| Pattern  | Description                              |
|----------|------------------------------------------|
| `one`    | All bits set (0xFF...)                   |
| `zero`   | All bits clear (0x00...)                 |
| `blip`   | Alternating blip pattern                 |
| `rand`   | Random values                            |
| `strobe` | Strobing pattern                         |
| `walk0`  | Walking zeros                            |
| `walk1`  | Walking ones                             |
| `addr`   | Address-as-data                          |

A hex value can also be used directly as a pattern.

## Test Modes

The `test` command accepts these mode names:

| Mode     | Description                              |
|----------|------------------------------------------|
| `one`    | All bits set (0xFF...)                   |
| `zero`   | All bits clear (0x00...)                 |
| `rand`   | Random values                            |
| `walk0`  | Walking zeros                            |
| `walk1`  | Walking ones                             |
| `read`   | Read-only test (no writes)               |

A hex value can also be used directly as a test pattern.

## Expression Evaluator

All numeric arguments are parsed as hexadecimal and support inline expressions.
The following operators are available, listed from highest to lowest precedence:

| Operators            | Description                      |
|----------------------|----------------------------------|
| `!` `~`              | Logical NOT, bitwise invert      |
| `*` `/` `%`          | Multiply, divide, modulo         |
| `+` `-`              | Add, subtract                    |
| `>>` `<<`            | Right shift, left shift          |
| `<=` `<` `>=` `>`    | Comparison                       |
| `==` `!=`            | Equality                         |
| `&`                  | Bitwise AND                      |
| `^`                  | Bitwise XOR                      |
| `\|`                 | Bitwise OR                       |
| `&&`                 | Logical AND                      |
| `\|\|`               | Logical OR                       |

Parentheses group sub-expressions. Example: `d 1000+20*4 10` displays 16
bytes starting at address 0x1080.

## CPU Command (AmigaOS)

| Subcommand                 | Description                                   |
|----------------------------|-----------------------------------------------|
| `cpu type`                 | Show CPU type and capabilities                |
| `cpu reg <reg> [<val>]`    | Get or set a CPU register (cacr, dtt*, itt*, pcr, tc, vbr) |
| `cpu fault <type>`         | Deliberately cause a CPU fault                |
| `cpu spin <dev> [w]`       | Spin reading/writing a device (ciaa, ciab, chipmem, or address) |

## Disassembler

On AmigaOS, `disas` provides 68k disassembly with selectable syntax:

- `mot` -- Motorola syntax (default)
- `mit` -- MIT/BSD syntax

On Linux and macOS, `disas` provides x86 disassembly. The width modifier
selects the mode: `disasq` for x86-64, `disasb`/`disasw`/`disasl` for
x86-32.

## Examples

```
med version                          # Show version
med d 0 40                           # Dump 64 bytes at address 0
med dl 4 4                           # Read 4 bytes as a longword at address 4
med dw 0xdff000 20                   # Dump 32 bytes as words (custom chip regs)
med c 1000 DE AD BE EF              # Write 4 bytes at 0x1000
med cl 1000 DEADBEEF                 # Write one longword at 0x1000
med disas 0xf80002 10               # Disassemble 10 instructions at 0xf80002
med cpu type                         # Show CPU type (AmigaOS)
med copyl 1000 2000 100             # Copy 0x100 bytes as longwords
med pattb 1000 1000 AA              # Fill 4096 bytes with 0xAA
med testl 1000 1000 walk1 a         # Test 4096 bytes, walking ones, 10 passes
med "echo 100+200"                   # Expression evaluation: prints 300
med "d 1000 10 ; d 2000 10"         # Chain two dump commands
```

Note: all numeric values are hexadecimal, except `disas` count which is
decimal.

## License

This project contains files under different licenses. See individual source
file headers for license terms.
