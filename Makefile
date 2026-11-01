CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude

SRC = src/main.c src/cpu.c
OBJ = $(SRC:.c=.o)
TARGET = rv32i-sim

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) test.bin

# [新增] 自动化测试脚本
test: $(TARGET)
	@echo "Running Automated Integration Tests..."
	@./$(TARGET) test.bin > /dev/null && echo "✅ ALL TESTS PASSED: System is stable." || (echo "❌ TEST FAILED!" && exit 1)

clean:
	rm -f src/*.o $(TARGET) test.bin gen_test

.PHONY: all run test clean