#define _POSIX_C_SOURCE 200809L
#include "minikv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 打印单个键值对的回调函数
void print_item(const char* key, const char* value, void* user_data) {
    (void)user_data;
    printf("%s=%s\n", key, value);
}

// 键值对结构体，用于排序输出
typedef struct {
    char* key;
    char* value;
} kv_pair;

// 比较两个键值对的 key，用于 qsort 排序
int compare_kv(const void* a, const void* b) {
    return strcmp(((kv_pair*)a)->key, ((kv_pair*)b)->key);
}

// 遍历时收集键值对的回调函数
void collect_items(const char* key, const char* value, void* user_data) {
    struct { kv_pair* arr; int idx; } *ctx = user_data;
    ctx->arr[ctx->idx].key = strdup(key);
    ctx->arr[ctx->idx].value = strdup(value);
    ctx->idx++;
}

// 通用命令处理入口
int process_command(int argc, char* argv[]) {
    // 检查参数数量是否足够
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file> <cmd> [args...]\n", argv[0]);
        fprintf(stderr, "Commands:\n");
        fprintf(stderr, "  get <key>\n");
        fprintf(stderr, "  set <key> <value>\n");
        fprintf(stderr, "  del <key>\n");
        fprintf(stderr, "  list\n");
        return 1;
    }

    const char* filepath = argv[1];
    const char* command = argv[2];
    
    // 创建 KV 实例
    mk_t* kv = mk_create();
    // 检查实例创建是否成功
    if (!kv) {
        fprintf(stderr, "Error: Failed to create kv instance\n");
        return 1;
    }

    // 尝试加载文件
    mk_load(kv, filepath);

    int ret = 0;

    // 处理 get 命令
    if (strcmp(command, "get") == 0) {
        // 检查 get 命令参数是否完整
        if (argc < 4) {
            fprintf(stderr, "Usage: %s <file> get <key>\n", argv[0]);
            ret = 1;
        } else {
            const char* val = mk_get(kv, argv[3]);
            // 检查是否找到对应的值
            if (val) {
                printf("%s\n", val);
            } else {
                fprintf(stderr, "Key not found\n");
                ret = 2;
            }
        }
    } 
    // 处理 set 命令
    else if (strcmp(command, "set") == 0) {
        // 检查 set 命令参数是否完整
        if (argc < 5) {
            fprintf(stderr, "Usage: %s <file> set <key> <value>\n", argv[0]);
            ret = 1;
        } else {
            // 检查设置操作是否成功
            if (mk_set(kv, argv[3], argv[4]) != 0) {
                fprintf(stderr, "Error: Failed to set value (invalid key?)\n");
                ret = 1;
            } else {
                // 检查保存文件是否成功
                if (mk_save(kv, filepath) != 0) {
                    fprintf(stderr, "Error: Failed to save file\n");
                    ret = 1;
                }
            }
        }
    } 
    // 处理 del 命令
    else if (strcmp(command, "del") == 0) {
        // 检查 del 命令参数是否完整
        if (argc < 4) {
            fprintf(stderr, "Usage: %s <file> del <key>\n", argv[0]);
            ret = 1;
        } else {
            mk_del(kv, argv[3]);
            // 检查保存文件是否成功
            if (mk_save(kv, filepath) != 0) {
                fprintf(stderr, "Error: Failed to save file\n");
                ret = 1;
            }
        }
    } 
    // 处理 list 命令
    else if (strcmp(command, "list") == 0) {
        size_t count = mk_count(kv);
        // 检查是否有键值对需要显示
        if (count > 0) {
            struct { kv_pair* arr; int idx; } ctx;
            ctx.arr = malloc(sizeof(kv_pair) * count);
            ctx.idx = 0;
            // 检查内存分配是否成功
            if (ctx.arr) {
                mk_foreach(kv, collect_items, &ctx);
                qsort(ctx.arr, count, sizeof(kv_pair), compare_kv);
                // 遍历并输出所有键值对
                for (size_t i = 0; i < count; i++) {
                    printf("%s=%s\n", ctx.arr[i].key, ctx.arr[i].value);
                    free(ctx.arr[i].key);
                    free(ctx.arr[i].value);
                }
                free(ctx.arr);
            }
        }
    } 
    // 处理未知命令
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        ret = 1;
    }

    mk_destroy(kv);
    return ret;
}

// 解析命令行输入，将字符串分割为参数数组
static int parse_line(char *line, char **argv, int max_args) {
    int argc = 0;
    char *p = line;
    // 循环解析每个参数直到字符串结束或达到最大参数数
    while (*p && argc < max_args) {
        // 跳过前导空白
        while (isspace((unsigned char)*p)) p++;
        // 检查是否已到字符串末尾
        if (*p == '\0') break;

        char *start = p;
        // 处理双引号括起来的参数
        if (*p == '"') {
            start++;
            p++;
            // 寻找匹配的右引号
            while (*p && *p != '"') p++;
            // 检查是否找到右引号
            if (*p == '"') {
                *p = '\0';
                p++;
            }
        } else {
            // 处理普通参数
            while (*p && !isspace((unsigned char)*p)) p++;
            // 如果遇到空白字符则替换为字符串结束符
            if (*p) {
                *p = '\0';
                p++;
            }
        }
        argv[argc++] = start;
    }
    argv[argc] = NULL;
    return argc;
}

// 执行具体的 KV 操作逻辑
int perform_kv_action(mk_t* kv, const char* cmd, char** args, int args_count, const char* filepath) {
    // 处理 get 操作
    if (strcmp(cmd, "get") == 0) {
        // 检查参数数量
        if (args_count < 1) {
            fprintf(stderr, "Error: get requires <key>\n");
            return 1;
        }
        const char* val = mk_get(kv, args[0]);
        // 检查是否找到值
        if (val) {
            printf("%s\n", val);
        } else {
            fprintf(stderr, "Key not found\n");
        }
    } 
    // 处理 set 操作
    else if (strcmp(cmd, "set") == 0) {
        // 检查参数数量
        if (args_count < 2) {
            fprintf(stderr, "Error: set requires <key> <value>\n");
            return 1;
        }
        // 检查设置操作是否成功
        if (mk_set(kv, args[0], args[1]) != 0) {
            fprintf(stderr, "Error: Failed to set value\n");
            return 1;
        }
        // 如果指定了文件，则立即保存
        if (filepath) {
            // 检查保存是否成功
            if (mk_save(kv, filepath) != 0) {
                fprintf(stderr, "Error: Failed to save file\n");
                return 1;
            }
        }
    } 
    // 处理 del 操作
    else if (strcmp(cmd, "del") == 0) {
        // 检查参数数量
        if (args_count < 1) {
            fprintf(stderr, "Error: del requires <key>\n");
            return 1;
        }
        mk_del(kv, args[0]);
        // 如果指定了文件，则立即保存
        if (filepath) {
            // 检查保存是否成功
            if (mk_save(kv, filepath) != 0) {
                fprintf(stderr, "Error: Failed to save file\n");
                return 1;
            }
        }
    } 
    // 处理 list 操作
    else if (strcmp(cmd, "list") == 0) {
        size_t count = mk_count(kv);
        // 检查是否有数据需要列出
        if (count > 0) {
            struct { kv_pair* arr; int idx; } ctx;
            ctx.arr = malloc(sizeof(kv_pair) * count);
            ctx.idx = 0;
            // 检查内存分配是否成功
            if (ctx.arr) {
                mk_foreach(kv, collect_items, &ctx);
                qsort(ctx.arr, count, sizeof(kv_pair), compare_kv);
                // 遍历输出所有键值对
                for (size_t i = 0; i < count; i++) {
                    printf("%s=%s\n", ctx.arr[i].key, ctx.arr[i].value);
                    free(ctx.arr[i].key);
                    free(ctx.arr[i].value);
                }
                free(ctx.arr);
            }
        }
    } 
    // 处理 load 操作
    else if (strcmp(cmd, "load") == 0) {
        // 检查是否在 -f 模式下使用 load
        if (filepath) {
            fprintf(stderr, "Error: load command cannot be used with -f\n");
            return 1;
        }
        // 检查参数数量
        if (args_count < 1) {
            fprintf(stderr, "Error: load requires <file>\n");
            return 1;
        }
        // 检查加载是否成功
        if (mk_load(kv, args[0]) != 0) {
            fprintf(stderr, "Error: Failed to load file %s\n", args[0]);
            return 1;
        }
        printf("Loaded %s\n", args[0]);
    } 
    // 处理 save 操作
    else if (strcmp(cmd, "save") == 0) {
        // 检查是否在 -f 模式下使用 save
        if (filepath) {
            fprintf(stderr, "Error: save command cannot be used with -f\n");
            return 1;
        }
        // 检查参数数量
        if (args_count < 1) {
            fprintf(stderr, "Error: save requires <file>\n");
            return 1;
        }
        // 检查保存是否成功
        if (mk_save(kv, args[0]) != 0) {
            fprintf(stderr, "Error: Failed to save to file %s\n", args[0]);
            return 1;
        }
        printf("Saved %s\n", args[0]);
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        return 1;
    }
    return 0;
}

// 解析并执行单行命令
void execute_interactive_command(mk_t* kv, int argc, char* argv[]) {
    // 寻找 -f 参数
    const char* filepath = NULL;
    int f_index = -1;
    
    // 遍历参数寻找 -f 选项
    for (int i = 0; i < argc; i++) {
        // 检查是否为 -f 选项
        if (strcmp(argv[i], "-f") == 0) {
            // 检查 -f 后是否有文件名参数
            if (i + 1 < argc) {
                filepath = argv[i + 1];
                f_index = i;
            } else {
                fprintf(stderr, "Error: -f requires a filename\n");
                return;
            }
            break;
        }
    }

    // 重新构建参数列表，排除 -f <file>
    char* clean_argv[64];
    int clean_argc = 0;
    
    // 遍历所有参数，过滤掉 -f 及其文件名
    for (int i = 0; i < argc; i++) {
        // 跳过 -f 及其参数
        if (f_index != -1 && (i == f_index || i == f_index + 1)) continue;
        clean_argv[clean_argc++] = argv[i];
    }
    
    // 检查是否还有有效命令
    if (clean_argc == 0) return;

    // 提取命令和参数
    char* cmd = clean_argv[0];
    char** cmd_args = &clean_argv[1];
    int cmd_args_count = clean_argc - 1;

    // 决定使用哪个 KV 对象
    mk_t* target_kv = kv;
    mk_t* temp_kv = NULL;

    // 如果指定了文件路径，创建临时环境
    if (filepath) {
        // 如果指定了 -f，创建临时环境
        temp_kv = mk_create();
        // 检查内存分配是否成功
        if (!temp_kv) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return;
        }
        mk_load(temp_kv, filepath);
        target_kv = temp_kv;
    }

    // 执行动作
    perform_kv_action(target_kv, cmd, cmd_args, cmd_args_count, filepath);

    // 清理临时环境
    if (temp_kv) {
        mk_destroy(temp_kv);
    }
}

// 进入交互式模式的主循环
void interactive_mode() {
    char line[1024];
    char* argv[64];
    
    // 初始化内部 KV 存储实例
    mk_t* global_kv = mk_create();
    // 检查实例创建是否成功
    if (!global_kv) {
        fprintf(stderr, "Error: Failed to initialize memory kv\n");
        return;
    }
    printf("MiniKV Interactive Mode. Type 'h' or 'help' for commands, 'q' or 'quit' to exit.\n");
    // 循环读取命令
    while (1) {
        printf("minikv> ");
        // 检查是否成功读取输入
        if (!fgets(line, sizeof(line), stdin)) {
             break; 
        }
        
        // 去除行末换行符
        line[strcspn(line, "\n")] = 0;
        
        // 忽略空行
        if (strlen(line) == 0) continue;
        
        // 构建参数数组
        int argc = parse_line(line, argv, 64);
        // 检查是否解析到有效参数
        if (argc == 0) continue;

        // 处理退出命令
        if (strcmp(argv[0], "q") == 0 || strcmp(argv[0], "quit") == 0) {
            break;
        }
        // 处理帮助命令
        if (strcmp(argv[0], "h") == 0 || strcmp(argv[0], "help") == 0) {
            printf("Usage:\n");
            printf("  get <key> [-f <file>]\n");
            printf("  set <key> <value> [-f <file>]\n");
            printf("  del <key> [-f <file>]\n");
            printf("  list [-f <file>]\n");
            printf("  load <file> (internal only)\n");
            printf("  save <file> (internal only)\n");
            printf("  quit / q : Exit\n");
            continue;
        }

        // 执行解析后的命令
        execute_interactive_command(global_kv, argc, argv);
    }
    
    mk_destroy(global_kv);
}

// 程序主入口函数
int main(int argc, char* argv[]) {
    // 如果没有参数，进入交互模式
    if (argc == 1) {
        interactive_mode();
        return 0;
    }
    
    // 否则正常执行单次命令
    return process_command(argc, argv);
}
