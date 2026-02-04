#define _POSIX_C_SOURCE 200809L // 启用 strdup 等 POSIX 特性
#include "minikv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // 引入 isspace

// 打印单个键值对的回调函数（目前未使用，但保留以备用）
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
    // 转换用户数据指针，需与调用处的结构定义一致
    struct { kv_pair* arr; int idx; } *ctx = user_data;
    // 复制 key 和 value 到数组中
    ctx->arr[ctx->idx].key = strdup(key);
    ctx->arr[ctx->idx].value = strdup(value);
    ctx->idx++;
}

// 通用命令处理入口，提取自原主函数，用于处理单次命令执行
int process_command(int argc, char* argv[]) {
    // 检查基本参数数量，至少需要程序名、文件名、命令
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
    if (!kv) {
        fprintf(stderr, "Error: Failed to create kv instance\n");
        return 1;
    }

    // 尝试加载文件，如果文件不存在可能返回错误，但对 set 来说是允许的
    mk_load(kv, filepath);

    int ret = 0;

    // 处理 get 命令：获取并打印值
    if (strcmp(command, "get") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s <file> get <key>\n", argv[0]);
            ret = 1;
        } else {
            const char* val = mk_get(kv, argv[3]);
            if (val) {
                printf("%s\n", val);
            } else {
                fprintf(stderr, "Key not found\n");
                ret = 2; // 非 0 表示逻辑失败
            }
        }
    } 
    // 处理 set 命令：设置键值并保存
    else if (strcmp(command, "set") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Usage: %s <file> set <key> <value>\n", argv[0]);
            ret = 1;
        } else {
            // value 允许包含空格，未加引号时会被 shell 拆成多个参数。
            // 例如 `set key "a b c"` 时 argv[4] 是 "a b c"；
            // 若输入 `set key a b c`，则 argv[4]="a"、argv[5]="b"...
            // 需求要求支持空格，但标准用法应通过引号传递。
            // 因此这里按规范直接取 argv[4]。
            
            // 设置键值对
            if (mk_set(kv, argv[3], argv[4]) != 0) {
                fprintf(stderr, "Error: Failed to set value (invalid key?)\n");
                ret = 1;
            } else {
                // 保存回文件
                if (mk_save(kv, filepath) != 0) {
                    fprintf(stderr, "Error: Failed to save file\n");
                    ret = 1;
                }
            }
        }
    } 
    // 处理 del 命令：删除键并保存
    else if (strcmp(command, "del") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s <file> del <key>\n", argv[0]);
            ret = 1;
        } else {
            // 删除键
            mk_del(kv, argv[3]);
            if (mk_save(kv, filepath) != 0) {
                fprintf(stderr, "Error: Failed to save file\n");
                ret = 1;
            }
        }
    } 
    // 处理 list 命令：列出所有键值对
    else if (strcmp(command, "list") == 0) {
        size_t count = mk_count(kv);
        if (count > 0) {
            struct { kv_pair* arr; int idx; } ctx;
            ctx.arr = malloc(sizeof(kv_pair) * count);
            ctx.idx = 0;
            
            if (ctx.arr) {
                // 收集所有数据到数组
                mk_foreach(kv, collect_items, &ctx);
                // 对结果进行排序
                qsort(ctx.arr, count, sizeof(kv_pair), compare_kv);
                
                // 输出并清理内存
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

    // 销毁实例并释放资源
    mk_destroy(kv);
    return ret;
}

// 解析命令行输入，将字符串分割为参数数组
// 支持简单的空格分隔和引用字符串，返回参数数量
static int parse_line(char *line, char **argv, int max_args) {
    int argc = 0;
    char *p = line;
    // 循环提取参数直到结束或达到最大限制
    while (*p && argc < max_args) {
        // 跳过前导空白
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0') break;

        char *start = p;
        // 处理双引号括起来的参数
        if (*p == '"') {
            start++; // 跳过左引号
            p++;
            // 寻找右引号
            while (*p && *p != '"') p++;
            if (*p == '"') {
                *p = '\0'; // 将右引号替换为结束符
                p++;
            }
        } else {
            // 处理普通参数，直到遇到空白
            while (*p && !isspace((unsigned char)*p)) p++;
            if (*p) {
                *p = '\0'; // 将空白替换为结束符
                p++;
            }
        }
        argv[argc++] = start;
    }
    argv[argc] = NULL;
    return argc;
}

// 执行具体的 KV 操作逻辑
// kv: 操作的实例对象
// cmd: 操作命令 (get/set/del/list/load/save)
// args: 命令参数数组 (args[0]开始为参数)
// args_count: 参数数量
// filepath: 如果设置了 -f，这里是非 NULL 的文件路径（用于自动保存），否则为 NULL
// 返回值: 成功返回 0，失败返回非 0
int perform_kv_action(mk_t* kv, const char* cmd, char** args, int args_count, const char* filepath) {
    if (strcmp(cmd, "get") == 0) {
        if (args_count < 1) {
            fprintf(stderr, "Error: get requires <key>\n");
            return 1;
        }
        const char* val = mk_get(kv, args[0]);
        if (val) {
            printf("%s\n", val);
        } else {
            fprintf(stderr, "Key not found\n");
            // 用户没有要求 get 失败返回非 0 退出，但逻辑上可认为操作成功完成只是结果为空
        }
    } else if (strcmp(cmd, "set") == 0) {
        if (args_count < 2) {
            fprintf(stderr, "Error: set requires <key> <value>\n");
            return 1;
        }
        if (mk_set(kv, args[0], args[1]) != 0) {
            fprintf(stderr, "Error: Failed to set value\n");
            return 1;
        }
        // 如果指定了文件，则立即保存
        if (filepath) {
            if (mk_save(kv, filepath) != 0) {
                fprintf(stderr, "Error: Failed to save file\n");
                return 1;
            }
        }
    } else if (strcmp(cmd, "del") == 0) {
        if (args_count < 1) {
            fprintf(stderr, "Error: del requires <key>\n");
            return 1;
        }
        mk_del(kv, args[0]);
        // 如果指定了文件，则立即保存
        if (filepath) {
            if (mk_save(kv, filepath) != 0) {
                fprintf(stderr, "Error: Failed to save file\n");
                return 1;
            }
        }
    } else if (strcmp(cmd, "list") == 0) {
        size_t count = mk_count(kv);
        if (count > 0) {
            struct { kv_pair* arr; int idx; } ctx;
            ctx.arr = malloc(sizeof(kv_pair) * count);
            ctx.idx = 0;
            if (ctx.arr) {
                mk_foreach(kv, collect_items, &ctx);
                qsort(ctx.arr, count, sizeof(kv_pair), compare_kv);
                for (size_t i = 0; i < count; i++) {
                    printf("%s=%s\n", ctx.arr[i].key, ctx.arr[i].value);
                    free(ctx.arr[i].key);
                    free(ctx.arr[i].value);
                }
                free(ctx.arr);
            }
        }
    } else if (strcmp(cmd, "load") == 0) {
        // load 操作仅用于内部环境（未指定 -f 时）
        if (filepath) {
            fprintf(stderr, "Error: load command cannot be used with -f\n");
            return 1;
        }
        if (args_count < 1) {
            fprintf(stderr, "Error: load requires <file>\n");
            return 1;
        }
        if (mk_load(kv, args[0]) != 0) {
            fprintf(stderr, "Error: Failed to load file %s\n", args[0]);
            return 1;
        }
        printf("Loaded %s\n", args[0]);
    } else if (strcmp(cmd, "save") == 0) {
        // save 操作仅用于内部环境（未指定 -f 时）
        if (filepath) {
            fprintf(stderr, "Error: save command cannot be used with -f\n");
            return 1;
        }
        if (args_count < 1) {
            fprintf(stderr, "Error: save requires <file>\n");
            return 1;
        }
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
// kv: 交互模式的全局 KV 环境
// argc, argv: 解析后的参数数组
void execute_interactive_command(mk_t* kv, int argc, char* argv[]) {
    // 寻找 -f 参数
    const char* filepath = NULL;
    int f_index = -1;
    
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
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
    
    for (int i = 0; i < argc; i++) {
        // 如果是 -f 及其参数，则跳过
        if (f_index != -1 && (i == f_index || i == f_index + 1)) continue;
        clean_argv[clean_argc++] = argv[i];
    }
    
    if (clean_argc == 0) return;

    // 提取命令和参数
    char* cmd = clean_argv[0];
    char** cmd_args = &clean_argv[1];
    int cmd_args_count = clean_argc - 1;

    // 决定使用哪个 KV 对象
    mk_t* target_kv = kv;
    mk_t* temp_kv = NULL;

    if (filepath) {
        // 如果指定了 -f，创建一个新的临时环境
        temp_kv = mk_create();
        if (!temp_kv) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return;
        }
        // 尝试加载文件
        mk_load(temp_kv, filepath);
        target_kv = temp_kv;
    }

    // 执行动作
    perform_kv_action(target_kv, cmd, cmd_args, cmd_args_count, filepath);

    // 如果使用了临时环境，进行清理
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
    if (!global_kv) {
        fprintf(stderr, "Error: Failed to initialize memory kv\n");
        return;
    }

    // 循环读取命令
    while (1) {
        printf("minikv> ");
        // 从标准输入读取一行
        if (!fgets(line, sizeof(line), stdin)) {
             // 遇到 EOF (Ctrl+D) 正常退出
             break; 
        }
        
        // 去除行末换行符
        line[strcspn(line, "\n")] = 0;
        
        // 忽略空行
        if (strlen(line) == 0) continue;
        
        // 构建参数数组
        int argc = parse_line(line, argv, 64);
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
    
    // 销毁环境
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
