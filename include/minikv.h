#ifndef MINIKV_H
#define MINIKV_H

#include <stddef.h>

/**
 * MiniKV 实例句柄。
 * 使用不透明结构体以隐藏实现细节。
 */
typedef struct mk_t mk_t;

/**
 * 创建一个新的空 MiniKV 实例。
 * @return 成功返回实例指针，失败返回 NULL。
 */
mk_t* mk_create(void);

/**
 * 销毁 MiniKV 实例并释放相关内存。
 * @param kv 需要销毁的实例。
 */
void mk_destroy(mk_t* kv);

/**
 * 从文件加载键值对到实例中。
 * 已存在的 key 可能会被覆盖。
 * @param kv 实例。
 * @param filepath 文件路径。
 * @return 成功返回 0，失败返回非 0 错误码。
 */
int mk_load(mk_t* kv, const char* filepath);

/**
 * 将实例中的所有键值对保存到文件。
 * 文件内容会被覆盖。
 * @param kv 实例。
 * @param filepath 文件路径。
 * @return 成功返回 0，失败返回非 0 错误码。
 */
int mk_save(mk_t* kv, const char* filepath);

/**
 * 获取与 key 对应的 value。
 * @param kv 实例。
 * @param key 要查询的 key。
 * @return 找到则返回 value 字符串，未找到返回 NULL。
 *         返回指针归实例所有，调用方不应释放或修改。
 */
const char* mk_get(const mk_t* kv, const char* key);

/**
 * 设置键值对；若 key 已存在则覆盖旧值。
 * @param kv 实例。
 * @param key 键。
 * @param value 值。
 * @return 成功返回 0，失败返回非 0。
 */
int mk_set(mk_t* kv, const char* key, const char* value);

/**
 * 删除键值对。
 * @param kv 实例。
 * @param key 要删除的 key。
 * @return 删除成功或未找到都返回 0，出错返回非 0。
 *         （要求“即使不存在也要有可控返回值”，通常理解为 0 表示成功）
 */
int mk_del(mk_t* kv, const char* key);

/**
 * 获取存储的键值对数量。
 * @param kv 实例。
 * @return 键值对数量。
 */
size_t mk_count(const mk_t* kv);

/**
 * 遍历所有键值对（用于 list 命令的辅助接口）。
 * @param kv 实例。
 * @param callback 对每个条目调用的回调（key、value、user_data）。
 * @param user_data 传递给回调的用户数据。
 */
void mk_foreach(const mk_t* kv, void (*callback)(const char* key, const char* value, void* user_data), void* user_data);

#endif // 头文件保护结束
