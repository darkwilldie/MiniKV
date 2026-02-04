# MiniKV - 轻量级 C 语言键值存储库

MiniKV 是一个使用 C 语言实现的轻量级、基于文件的键值存储库。它提供简洁的 API，用于加载、查询、修改并保存 `key=value` 格式的配置文件，同时包含一个命令行工具（CLI）用于管理这些文件。

本项目用于练习 C 语言模块化、内存管理、Makefile 工程化构建以及 CUnit 单元测试。

## 功能特性

*   **文件持久化**：从文本文件加载并保存（`key=value` 格式）。
*   **格式简单**：支持注释行（`#`、`;`），忽略空行；自动去除 key/value 两侧空白。
*   **API 支持**：提供 C 库（`libminikv.a`），包含 `get`、`set`、`del`、`load`、`save` 等操作。
*   **CLI 工具**：提供 `minikv` 命令行工具，快速操作配置文件。
*   **内存管理**：无内存泄漏（通过 valgrind 检查）。

## 项目结构

```
MiniKV/
  include/
    minikv.h        # 公共 API 头文件
  src/
    minikv.c        # 核心库实现
    cli.c           # CLI 工具实现
  tests/
    test_minikv.c   # CUnit 测试用例
  Makefile          # 构建脚本
```

## 环境依赖

*   GCC 编译器
*   Make
*   CUnit（运行测试用）

Ubuntu/Debian 安装 CUnit：
```bash
sudo apt-get install libcunit1-dev
```

## 构建

构建静态库与 CLI 工具：

```bash
make all
```

生成产物：
*   `libminikv.a`：静态库。
*   `minikv`：CLI 可执行文件。

清理构建产物：
```bash
make clean
```

## 使用说明

### 1. 使用 CLI 工具

`minikv` 工具用于直接在终端操作键值文件。

**用法：**
```bash
./minikv <filename> <command> [args...]
```

**示例：**

*   **设置键值**（文件不存在会创建，key 存在会覆盖）：
    ```bash
    ./minikv config.txt set server_host "127.0.0.1"
    ./minikv config.txt set port 8080
    ```

*   **获取键值**：
    ```bash
    ./minikv config.txt get server_host
    # 输出: 127.0.0.1
    ```

*   **列出所有键值**（按 key 排序）：
    ```bash
    ./minikv config.txt list
    # 输出:
    # port=8080
    # server_host=127.0.0.1
    ```

*   **删除键**：
    ```bash
    ./minikv config.txt del port
    ```

### 2. 使用 C 库（`libminikv.a`）

在 C 程序中包含头文件 `include/minikv.h`，并链接 `libminikv.a`。

**示例代码：**

```c
#include "minikv.h"
#include <stdio.h>

int main() {
    // 创建实例
    mk_t* kv = mk_create();
    
    // 加载已有文件（可选）
    mk_load(kv, "app.conf");

    // 设置键值
    mk_set(kv, "username", "admin");
    mk_set(kv, "theme", "dark");

    // 读取键值
    const char* user = mk_get(kv, "username");
    printf("User: %s\n", user ? user : "(not set)");

    // 保存回文件
    mk_save(kv, "app.conf");

    // 释放资源
    mk_destroy(kv);
    return 0;
}
```

**编译示例：**

```bash
gcc -Iinclude main.c libminikv.a -o myapp
```

## 测试

运行单元测试（需安装 CUnit）：

```bash
make test
```

## 配置文件格式说明

配置文件为简单文本格式：
*   **格式**：`Key=Value`
*   **注释**：以 `#` 或 `;` 开头的行会被忽略。
*   **空白**：key/value 两侧空白会被去除；value 可包含内部空格。

**示例 `config.txt`：**
```ini
# Database Config
host = localhost
port = 5432
name = My Database
; Backup settings
backup_enabled = true
```

## 许可

本项目开源，仅用于学习和教学用途。
