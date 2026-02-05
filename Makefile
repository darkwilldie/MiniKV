# gcc flags
CFLAGS_COMMON = -std=c11 -Wall -g
CFLAGS_SRC = $(CFLAGS_COMMON) -Iinclude
CFLAGS_TEST = $(CFLAGS_COMMON)

# 目录
SRCDIR = src
INCDIR = include
TESTDIR = tests
OBJDIR = obj
BINDIR = bin

# 目标文件
LIBVAL = libminikv.a
TARGET = $(BINDIR)/minikv
TEST_TARGET = $(BINDIR)/test_runner

SRC = $(SRCDIR)/minikv.c
CLI_SRC = $(SRCDIR)/cli.c
TEST_SRC = $(TESTDIR)/test_minikv.c

OBJ = $(OBJDIR)/minikv.o
CLI_OBJ = $(OBJDIR)/cli.o
TEST_OBJ = $(OBJDIR)/test_minikv.o

# 声明伪目标
.PHONY: all clean test directories install uninstall

# all目标创建目录，生成.a库和可执行文件
# make默认执行第一个目标
all: directories $(LIBVAL) $(TARGET)

directories:
	mkdir -p $(OBJDIR) $(BINDIR)

# 生成目标文件（.o文件）
# $<表示第一个依赖文件，$@表示目标文件
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	gcc $(CFLAGS_SRC) -c $< -o $@

$(OBJDIR)/%.o: $(TESTDIR)/%.c
	gcc $(CFLAGS_TEST) -c $< -o $@

# 打包minikv.o生成静态库（.a文件）
# $^表示所有依赖文件
$(LIBVAL): $(OBJ)
	ar rcs $@ $^

# 生成可执行文件
# minikv可执行文件依赖cli.o和静态库
$(TARGET): $(CLI_OBJ) $(LIBVAL)
	gcc $(CFLAGS_SRC) -o $@ $^

# test_runner可执行文件依赖test_minikv.o和静态库
$(TEST_TARGET): $(TEST_OBJ) $(LIBVAL)
	gcc $(CFLAGS_TEST) -o $@ $^ -lcunit

# 运行测试
test: directories $(TEST_TARGET)
	./$(TEST_TARGET)

# 安装到系统
install: all
	sudo cp $(TARGET) /usr/local/bin/minikv
	sudo chmod +x /usr/local/bin/minikv

# 从系统卸载
uninstall:
	sudo rm -f /usr/local/bin/minikv

clean:
	rm -rf $(LIBVAL) $(TARGET) $(TEST_TARGET) $(OBJDIR) $(BINDIR)
