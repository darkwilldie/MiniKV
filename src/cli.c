#define _POSIX_C_SOURCE 200809L // 使用 strdup
#include "minikv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_item(const char* key, const char* value, void* user_data) {
    (void)user_data;
    printf("%s=%s\n", key, value);
}

// 使用更高效的排序方式会更好（如创建排序数组）。
// 但 CLI 的 list 命令是文件 IO 场景，直接加载后排序数组再输出即可。
// 需求中“按 key 排序输出”为加分项。
// 这里在 CLI 中实现一个简单排序流程。

typedef struct {
    char* key;
    char* value;
} kv_pair;

int compare_kv(const void* a, const void* b) {
    return strcmp(((kv_pair*)a)->key, ((kv_pair*)b)->key);
}

void collect_items(const char* key, const char* value, void* user_data) {
    // 回调里通常无法提前知道数量，因此这里先用 mk_count 预分配。
    // user_data 用于传递数组和索引的结构体
    struct { kv_pair* arr; int idx; } *ctx = user_data;
    ctx->arr[ctx->idx].key = strdup(key);
    ctx->arr[ctx->idx].value = strdup(value);
    ctx->idx++;
}

int main(int argc, char* argv[]) {
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
    
    mk_t* kv = mk_create();
    if (!kv) {
        fprintf(stderr, "Error: Failed to create kv instance\n");
        return 1;
    }

    // 先尝试加载已有文件（对 set 来说文件不存在是可接受的；
    // 对 get/del 可能需要区分权限问题或空文件）。
    // mk_load 返回 1 表示文件不存在，这对新建文件的 set 是可接受的。
    mk_load(kv, filepath);

    int ret = 0;

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
            
            if (mk_set(kv, argv[3], argv[4]) != 0) {
                fprintf(stderr, "Error: Failed to set value (invalid key?)\n");
                ret = 1;
            } else {
                if (mk_save(kv, filepath) != 0) {
                    fprintf(stderr, "Error: Failed to save file\n");
                    ret = 1;
                }
            }
        }
    } 
    else if (strcmp(command, "del") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s <file> del <key>\n", argv[0]);
            ret = 1;
        } else {
            mk_del(kv, argv[3]);
            if (mk_save(kv, filepath) != 0) {
                fprintf(stderr, "Error: Failed to save file\n");
                ret = 1;
            }
        }
    } 
    else if (strcmp(command, "list") == 0) {
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
    } 
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        ret = 1;
    }

    mk_destroy(kv);
    return ret;
}
