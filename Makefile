# CN_Language 项目构建文件
# 版本: 2.0.0
# 作者: CN_Language架构委员会
# 日期: 2026-01-09

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -D_GNU_SOURCE \
         -I./src/application \
         -I./src/core \
         -I./src/infrastructure \
         -I./src/application/cli \
         -I./src/application/repl \
         -I./src/core/lexer \
         -I./src/core/parser \
         -I./src/core/ast \
         -I./src/core/semantic \
         -I./src/core/codegen \
         -I./src/core/runtime \
         -I./src/core/token \
         -I./src/infrastructure/containers/array \
         -I./src/infrastructure/memory \
         -I./src/infrastructure/platform \
         -I./src/infrastructure/utils

LDFLAGS = -lm

# 目标可执行文件
TARGET = bin/CN_Language

# 应用层源文件
APP_SRCS = src/application/CN_main.c \
           src/application/cli/CN_cli.c \
           src/application/cli/CN_command_parser.c \
           src/application/cli/CN_command_executor.c \
           src/application/repl/CN_repl_impl.c

# 核心层源文件 - 编译器
CORE_SRCS = src/core/CN_compiler_impl.c \
            src/core/lexer/CN_lexer_interface.c \
            src/core/lexer/CN_lexer_impl.c \
            src/core/lexer/errors/CN_lexer_errors.c \
            src/core/lexer/keywords/CN_lexer_keywords.c \
            src/core/lexer/operators/CN_lexer_operators.c \
            src/core/lexer/scanner/CN_lexer_scanner.c \
            src/core/lexer/token_scanners/CN_lexer_token_scanners.c \
            src/core/lexer/utils/CN_lexer_utils.c \
            src/core/token/CN_token.c

# 基础设施层源文件
INFRA_SRCS = src/infrastructure/containers/array/CN_dynamic_array.c \
             src/infrastructure/containers/array/CN_dynamic_array_core.c \
             src/infrastructure/containers/array/CN_dynamic_array_interface_impl.c \
             src/infrastructure/containers/array/CN_dynamic_array_operations.c \
             src/infrastructure/containers/array/CN_dynamic_array_utils.c \
             src/infrastructure/utils/CN_utils_string.c

# 所有源文件
ALL_SRCS = $(APP_SRCS) $(CORE_SRCS) $(INFRA_SRCS)

# 对象文件 - 放在build目录的相应位置
APP_OBJS = $(patsubst src/%.c,build/%.o,$(APP_SRCS))
CORE_OBJS = $(patsubst src/%.c,build/%.o,$(CORE_SRCS))
INFRA_OBJS = $(patsubst src/%.c,build/%.o,$(INFRA_SRCS))
ALL_OBJS = $(APP_OBJS) $(CORE_OBJS) $(INFRA_OBJS)

# 默认目标
all: $(TARGET)

# 链接可执行文件
$(TARGET): $(ALL_OBJS)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "构建完成: $(TARGET)"

# 通用编译规则
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 清理构建文件
clean:
	rm -rf build bin
	@echo "清理完成: 已删除build和bin目录"

# 运行程序
run: $(TARGET)
	./$(TARGET) help

# 测试帮助命令
test-help: $(TARGET)
	./$(TARGET) help

# 测试版本命令
test-version: $(TARGET)
	./$(TARGET) version

# 测试编译命令
test-compile: $(TARGET)
	./$(TARGET) compile test.cn

# 测试运行命令
test-run: $(TARGET)
	./$(TARGET) run test.cn

# 测试调试命令
test-debug: $(TARGET)
	./$(TARGET) debug test.cn

# 测试REPL
test-repl: $(TARGET)
	@echo "启动REPL测试..."
	@echo "help\nexit" | ./$(TARGET) || true

# 显示构建信息
info:
	@echo "CN_Language 构建系统"
	@echo "===================="
	@echo "编译器: $(CC)"
	@echo "编译标志: $(CFLAGS)"
	@echo "链接标志: $(LDFLAGS)"
	@echo "目标文件: $(TARGET)"
	@echo "应用层源文件:"
	@for src in $(APP_SRCS); do echo "  $$src"; done
	@echo "核心层源文件:"
	@for src in $(CORE_SRCS); do echo "  $$src"; done
	@echo "基础设施层源文件:"
	@for src in $(INFRA_SRCS); do echo "  $$src"; done

# 帮助信息
help:
	@echo "可用目标:"
	@echo "  all       构建所有目标（默认）"
	@echo "  clean     清理构建文件（删除build和bin目录）"
	@echo "  run       运行程序（显示帮助）"
	@echo "  test-*    运行各种测试"
	@echo "  info      显示构建信息"
	@echo "  help      显示此帮助信息"

.PHONY: all clean run test-help test-version test-compile test-run test-debug info help
