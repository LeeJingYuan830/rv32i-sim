# Host Compiler toolchain configuration
CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude

# RISC-V Cross-Compiler toolchain configuration
RISCV_CC = riscv64-unknown-elf-gcc
RISCV_CFLAGS = -march=rv32i -mabi=ilp32 -static -mcmodel=medany \
               -fvisibility=hidden -nostdlib -nostartfiles -T linker.ld

# Emulator source files and targets
SRC = src/main.c src/cpu.c src/elf_loader.c
OBJ = $(SRC:.c=.o)
TARGET = rv32i-sim

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build test generation utility for raw binary images
gen_test: src/gen_test.c
	$(CC) $(CFLAGS) $< -o $@

# Generate binary test image
test.bin: gen_test
	./gen_test

# Compile bare-metal assembly into standard ELF32 executable
tests/hello.elf: tests/hello.s linker.ld
	@mkdir -p tests
	$(RISCV_CC) $(RISCV_CFLAGS) $< -o $@

run: $(TARGET) test.bin
	./$(TARGET) test.bin

# Test 1: Run automated raw binary integration tests
test: $(TARGET) test.bin
	@echo "========================================"
	@echo "Running Binary Integration Tests..."
	@echo "========================================"
	./$(TARGET) test.bin

# Test 2: Run standard ELF32 execution test
test-elf: $(TARGET) tests/hello.elf
	@echo "========================================"
	@echo "Running Standard ELF32 Execution Test..."
	@echo "========================================"
	./$(TARGET) tests/hello.elf

clean:
	rm -f src/*.o $(TARGET) test.bin gen_test tests/*.elf

.PHONY: all run test test-elf clean

# ==============================================================================
# RISC-V Compliance Test Suite Configuration (riscv-tests)
# ==============================================================================
PROJ_DIR        = $(CURDIR)
RISCV_TEST_DIR  = $(PROJ_DIR)/tests/riscv-tests
RISCV_ENV_DIR   = $(RISCV_TEST_DIR)/env
RISCV_ISA_DIR   = $(RISCV_TEST_DIR)/isa

# Flags required by official riscv-tests (RV32I + Zicsr + Zifencei)
COMPLIANCE_CFLAGS = -march=rv32i_zicsr_zifencei -mabi=ilp32 -static -mcmodel=medany \
                    -fvisibility=hidden -nostdlib -nostartfiles \
                    -D__riscv_xlen=32 \
                    -I$(RISCV_ENV_DIR) \
                    -I$(RISCV_ENV_DIR)/p \
                    -I$(RISCV_ISA_DIR)/macros/scalar \
                    -T$(RISCV_ENV_DIR)/p/link.ld \
                    -Wl,--no-warn-rwx-segments
					
# Find all RV32UI (User Integer) assembly source files (.S)
COMPLIANCE_SRCS = $(wildcard $(RISCV_ISA_DIR)/rv32ui/*.S)

# Map .S sources to build targets (e.g., tests/riscv-tests/isa/rv32ui-p-add)
COMPLIANCE_ELFS = $(patsubst $(RISCV_ISA_DIR)/rv32ui/%.S, $(RISCV_ISA_DIR)/rv32ui-p-%, $(COMPLIANCE_SRCS))

# Rule to compile a single compliance test assembly into ELF
$(RISCV_ISA_DIR)/rv32ui-p-%: $(RISCV_ISA_DIR)/rv32ui/%.S
	$(RISCV_CC) $(COMPLIANCE_CFLAGS) $< -o $@

# Target: Compile all RV32UI compliance test ELFs
build-compliance: $(COMPLIANCE_ELFS)
	@echo "=================================================="
	@echo "Successfully compiled all $(words $(COMPLIANCE_ELFS)) RV32UI compliance tests."
	@echo "=================================================="

# Target: Run all compiled compliance tests automatically
test-compliance: $(TARGET) build-compliance
	@echo "=================================================="
	@echo "Running Official RISC-V Compliance Tests (RV32UI)..."
	@echo "=================================================="
	@pass=0; fail=0; total=0; \
	for test_elf in $(COMPLIANCE_ELFS); do \
		total=$$((total + 1)); \
		./$(TARGET) $$test_elf > /dev/null 2>&1; \
		if [ $$? -eq 0 ]; then \
			echo "[\033[32mPASS\033[0m] $$(basename $$test_elf)"; \
			pass=$$((pass + 1)); \
		else \
			echo "[\033[31mFAIL\033[0m] $$(basename $$test_elf)"; \
			fail=$$((fail + 1)); \
		fi; \
	done; \
	echo "=================================================="; \
	echo "Test Summary: Total $$total | Passed: \033[32m$$pass\033[0m | Failed: \033[31m$$fail\033[0m"; \
	echo "=================================================="; \
	if [ $$fail -ne 0 ]; then exit 1; fi

.PHONY: build-compliance test-compliance