#define _POSIX_C_SOURCE 200809L
#include "minikv.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 单个键值对节点（用于哈希桶内链表）
typedef struct mk_node {
    char* key;
    char* value;
    struct mk_node* next;
} mk_node_t;

// 简易哈希表
struct mk_t {
    // 桶指针数组，指向链表头节点
    mk_node_t** buckets;
    // 桶的数量
    size_t bucket_count;
    // 当前存储的键值对数量
    size_t count;
};





// 获取键值对数量
size_t mk_count(const mk_t* kv) {
    // 如果没有kv实例返回0，否则返回count
    return kv ? kv->count : 0;
}

// djb2 哈希函数
static unsigned long hash_key(const char* str) {
    unsigned long hash = 5381;
    int c;
    // hash = hash * 33 + c（核心递推公式）
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + (unsigned long)c;
    }
    return hash;
}

// 计算桶索引
// C不支持默认参数
static size_t bucket_index(const mk_t* kv, const char* key) {
    return (size_t)(hash_key(key) % kv->bucket_count);
}

// 创建kv哈希表
mk_t* mk_create(void) {
    // 创建哈希表实例
    mk_t* kv = (mk_t*)malloc(sizeof(mk_t));
    if (!kv) return NULL;
    // 初始化桶数量为256
    kv->bucket_count = 256;
    // 初始化键值对数量为0
    kv->count = 0;
    // 分配桶指针数组，calloc会初始化为NULL以便分辨空桶
    kv->buckets = (mk_node_t**)calloc(kv->bucket_count, sizeof(mk_node_t*));
    // 分配失败则释放kv实例并返回NULL
    if (!kv->buckets) {
        free(kv);
        return NULL;
    }
    return kv;
}

// 根据kv创建新节点
mk_node_t* create_node(const char* key, const char* value) {
    mk_node_t* node = (mk_node_t*)malloc(sizeof(mk_node_t));
    if (!node) return NULL;
    node->key = strdup(key);
    node->value = strdup(value);
    node->next = NULL;
    if (!node->key || !node->value) {
        free(node->key);
        free(node->value);
        free(node);
        return NULL;
    }
    return node;
}

// 扩容哈希表
// 参数为哈希表指针kv和新的桶数量
static int mk_resize(mk_t* kv, size_t new_bucket_count) {
    if (!kv) return -1;
    // 同样calloc分配新桶指针数组
    mk_node_t** new_buckets = (mk_node_t**)calloc(new_bucket_count, sizeof(mk_node_t*));
    if (!new_buckets) return -2;
    // 重新分配所有节点到新桶
    for (size_t i = 0; i < kv->bucket_count; i++) {
        // 遍历旧桶中的链表，将节点重新分配到新桶
        mk_node_t* node = kv->buckets[i];
        while (node) {
            mk_node_t* next = node->next;
            // 桶数量变了，哈希值取模也要变
            size_t idx = (size_t)(hash_key(node->key) % new_bucket_count);
            // 典型的头插法，注意new_buckets[idx]是原来的头节点地址
            // 新节点先指向原来的头节点，桶指向新节点
            node->next = new_buckets[idx];
            new_buckets[idx] = node;
            node = next;
        }
    }
    // 释放旧桶指针数组
    free(kv->buckets);
    // 更新哈希表结构体的桶指针和桶数量
    kv->buckets = new_buckets;
    kv->bucket_count = new_bucket_count;
    return 0;
}

// 释放桶内链表
static void free_bucket_list(mk_node_t* node) {
    while (node) {
        mk_node_t* next = node->next;
        free(node->key);
        free(node->value);
        free(node);
        node = next;
    }
}

// 销毁kv哈希表
void mk_destroy(mk_t* kv) {
    if (!kv) return;
    // 遍历桶指针数组，释放每个桶的链表
    for (size_t i = 0; i < kv->bucket_count; i++) {
        free_bucket_list(kv->buckets[i]);
    }
    // 释放桶指针数组
    free(kv->buckets);
    // 释放哈希表实例
    free(kv);
}

// 设置键值对
int mk_set(mk_t* kv, const char* key, const char* value) {
    // 参数缺失输出-1，无效key输出-2
    if (!kv || !key || !value) return -1;
    if (!is_valid_key(key)) return -2;
    // 计算桶索引
    size_t idx = bucket_index(kv, key);
    // 遍历链表检查是否已存在该key，存在则更新value
    mk_node_t* current = kv->buckets[idx];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            // 防止value被销毁，用strdup复制一份
            char* new_val = strdup(value);
            if (!new_val) return -1;
            // 释放旧value，更新为新value
            free(current->value);
            current->value = new_val;
            // 找到并更新成功返回0，否则在之后创建新节点
            return 0;
        }
        current = current->next;
    }
    // 到这里说明key不存在，创建新节点并插入链表头
    mk_node_t* new_node = create_node(key, value);
    if (!new_node) return -1;
    // 头插法插入对应的桶
    new_node->next = kv->buckets[idx];
    kv->buckets[idx] = new_node;
    // 更新键值对数量
    kv->count++;

    // 简单的负载因子检查，超过0.75则扩容
    if (kv->count > (kv->bucket_count * 3) / 4) {
        mk_resize(kv, kv->bucket_count * 2);
    }
    return 0;
}

// 根据key获取value
const char* mk_get(const mk_t* kv, const char* key) {
    if (!kv || !key) return NULL;
    // 计算出桶索引
    size_t idx = bucket_index(kv, key);
    // 遍历链表查找key
    mk_node_t* current = kv->buckets[idx];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    // 找不到返回NULL
    return NULL;
}

// 删除键值对
int mk_del(mk_t* kv, const char* key) {
    if (!kv || !key) return -1;
    size_t idx = bucket_index(kv, key);
    mk_node_t* current = kv->buckets[idx];
    mk_node_t* prev = NULL;

    while (current) {
        if (strcmp(current->key, key) == 0) {
            // 有前驱节点，更新前驱节点的next指针
            if (prev) {
                prev->next = current->next;
            } 
            // 没有前驱节点，直接更改头指针
            else {
                kv->buckets[idx] = current->next;
            }
            // 释放当前节点
            free(current->key);
            free(current->value);
            free(current);
            // 更新键值对数量
            kv->count--;
            return 0;
        }
        // prev设为当前节点，继续遍历
        prev = current;
        current = current->next;
    }
    return 0;
}

// 从文件加载键值对
// 参数是kv实例和文件路径
int mk_load(mk_t* kv, const char* filepath) {
    if (!kv || !filepath) return -1;
    // fopen打开文件读取
    FILE* fp = fopen(filepath, "r");
    if (!fp) return 1; // 文件不存在或不可读
    // 逐行读取文件到buffer
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp)) {
        char* key = NULL;
        char* val = NULL;
        
        // 使用新的解析函数
        if (parse_key_value_line(buffer, &key, &val)) {
            // 设置键值对
            mk_set(kv, key, val);
        }
    }
    // 关闭文件
    fclose(fp);
    return 0;
}

// 保存键值对到文件
int mk_save(mk_t* kv, const char* filepath) {
    if (!kv || !filepath) return -1;
    // 以写模式打开文件
    FILE* fp = fopen(filepath, "w");
    if (!fp) return 1;
    // 遍历所有桶和链表，写入key=value格式
    for (size_t i = 0; i < kv->bucket_count; i++) {
        mk_node_t* current = kv->buckets[i];
        while (current) {
            // 格式化写入 key=value
            fprintf(fp, "%s=%s\n", current->key, current->value);
            current = current->next;
        }
    }
    // 关闭文件
    fclose(fp);
    return 0;
}

// 遍历所有键值对，调用回调函数
void mk_foreach(const mk_t* kv, void (*callback)(const char* key, const char* value, void* user_data), void* user_data) {
    // 参数检查：kv为空或回调为空则直接返回
    if (!kv || !callback) return;
    // 遍历所有桶
    for (size_t i = 0; i < kv->bucket_count; i++) {
        // 取出当前桶的链表头
        mk_node_t* current = kv->buckets[i];
        // 遍历当前桶内链表
        while (current) {
            // 对当前键值对执行回调
            callback(current->key, current->value, user_data);
            // 移动到下一个节点
            current = current->next;
        }
    }
}
