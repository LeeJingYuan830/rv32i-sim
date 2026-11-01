# RV32I-Sim: A Lightweight RISC-V CPU Emulator

A from-scratch, modular 32-bit RISC-V (RV32I) instruction set emulator written in C. This project simulates the core Fetch-Decode-Execute cycle of a modern processor, bridging the gap between hardware architecture and low-level software engineering.

## 🚀 Features

- **Full RV32I Base Integer Instruction Set**: Supports ALU operations, branching, jumps, and memory access.
- **Hardware-Accurate Memory Mapping**: Simulates 64KB Little-Endian RAM with precise memory alignment and byte-level manipulation (`lw`, `sw`, `lbu`, etc.).
- **Decoupled Architecture**: Separates the CPU logic from the software. Runs compiled binary (`.bin`) firmware directly via a built-in bootloader.
- **Lightweight & Modular**: Zero external dependencies. Built entirely with standard C libraries.

## 🧠 Implemented Instructions

| Category | Instructions |
| :--- | :--- |
| **ALU (I-Type/R-Type)** | `add`, `addi`, `sub`, `sll`, `slli`, `slt`, `slti`, `sltu`, `sltiu`, `xor`, `xori`, `srl`, `srli`, `sra`, `srai`, `or`, `ori`, `and`, `andi` |
| **Control Flow (B-Type/J-Type)** | `beq`, `bne`, `blt`, `bge`, `bltu`, `bgeu`, `jal` |
| **Memory Access (Load/Store)** | `lw`, `lh`, `lbu`, `sw`, `sh`, `sb` |

## 🛠️ Build and Run

Compiled via GCC on Linux/WSL.

```bash
# 1. Clone the repository
git clone [https://github.com/your-username/rv32i-sim.git](https://github.com/your-username/rv32i-sim.git)
cd rv32i-sim

# 2. Generate the test binary firmware
gcc src/gen_test.c -o gen_test
./gen_test

# 3. Build the emulator and run automated tests
make
make test

# 4. Run manually
./rv32i-sim test.bin

🔍 Internal Architecture
The emulator structure strictly follows the Von Neumann architecture:

1. Fetch: Reads 32-bit instructions from simulated Little-Endian memory.
2. Decode: Extracts opcode, funct3, funct7, and handles sign-extended immediate reconstruction (especially the complex B-Type and S-Type offsets).
3. Execute: Evaluates arithmetic logic and mutates architectural states (PC and x0-x31 registers).

Built for educational purposes and architectural exploration by Jing Yuan Lee.