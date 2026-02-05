#define _POSIX_C_SOURCE 200809L
#include <CUnit/Basic.h>
#include "../include/minikv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static mk_t* kv = NULL;

// 初始化测试套件，创建键值存储实例
static int init_suite(void) {
    kv = mk_create();
    return (kv == NULL) ? -1 : 0; // 如果创建失败返回-1，成功返回0
}

// 清理测试套件，销毁键值存储实例
static int clean_suite(void) {
    mk_destroy(kv);
    kv = NULL;
    return 0;
}

// 创建临时文件并写入内容，用于测试文件操作
static char* write_temp_file(const char* content) {
    char template[] = "/tmp/minikv_test_XXXXXX";
    int fd = mkstemp(template); // 创建唯一的临时文件
    if (fd == -1) return NULL; // 如果创建失败返回NULL

    FILE* fp = fdopen(fd, "w"); // 将文件描述符转换为文件指针
    if (!fp) { // 如果转换失败，清理资源
        close(fd);
        unlink(template);
        return NULL;
    }

    fputs(content, fp); // 写入内容到文件
    fclose(fp);
    return strdup(template); // 返回文件路径的副本
}

// 测试基本的设置和获取功能
static void test_basic_set_get(void) {
    CU_ASSERT_EQUAL(mk_set(kv, "test_key", "test_value"), 0); // 设置键值对
    const char* val = mk_get(kv, "test_key"); // 获取值
    CU_ASSERT_PTR_NOT_NULL(val); // 验证返回值不为空
    if (val) CU_ASSERT_STRING_EQUAL(val, "test_value"); // 如果值存在，验证内容正确
}

// 测试覆盖已存在键的值
static void test_set_overwrite(void) {
    CU_ASSERT_EQUAL(mk_set(kv, "key1", "val1"), 0); // 设置初始值
    CU_ASSERT_EQUAL(mk_set(kv, "key1", "val2"), 0); // 覆盖为新值
    CU_ASSERT_STRING_EQUAL(mk_get(kv, "key1"), "val2"); // 验证值已被覆盖
}

// 测试删除存在的键
static void test_del_existing(void) {
    CU_ASSERT_EQUAL(mk_set(kv, "del_me", "v"), 0); // 先设置一个键值对
    CU_ASSERT_EQUAL(mk_del(kv, "del_me"), 0); // 删除该键
    CU_ASSERT_PTR_NULL(mk_get(kv, "del_me")); // 验证键已被删除
}

// 测试删除不存在的键
static void test_del_non_existing(void) {
    CU_ASSERT_EQUAL(mk_del(kv, "not_exist"), 0); // 删除不存在的键应该成功
}

// 测试设置和删除操作后的计数功能
static void test_count_after_set_del(void) {
    mk_t* mk = mk_create(); // 创建新的键值存储实例
    CU_ASSERT_EQUAL(mk_count(mk), 0); // 初始计数应为0
    mk_set(mk, "a", "1"); // 添加第一个键值对
    mk_set(mk, "b", "2"); // 添加第二个键值对
    CU_ASSERT_EQUAL(mk_count(mk), 2); // 验证计数为2
    mk_del(mk, "a"); // 删除一个键
    CU_ASSERT_EQUAL(mk_count(mk), 1); // 验证计数减少为1
    mk_destroy(mk); // 清理资源
}

// 测试加载文件时忽略空行和注释
static void test_load_ignore_empty_comments(void) {
    char* path = write_temp_file("\n# comment\n; comment\nvalid=1\n"); // 创建包含注释的测试文件
    CU_ASSERT_PTR_NOT_NULL(path); // 验证文件创建成功
    if (!path) return; // 如果文件创建失败，直接返回

    mk_t* mk = mk_create(); // 创建键值存储实例
    mk_load(mk, path); // 从文件加载数据
    CU_ASSERT_EQUAL(mk_count(mk), 1); // 验证只加载了有效的键值对
    CU_ASSERT_STRING_EQUAL(mk_get(mk, "valid"), "1"); // 验证值正确
    mk_destroy(mk); // 清理资源
    unlink(path); // 删除临时文件
    free(path); // 释放路径字符串内存
}

// 测试加载时自动去除首尾空格
static void test_load_trim(void) {
    char* path = write_temp_file("  key  =  val  \n"); // 创建包含空格的测试文件
    CU_ASSERT_PTR_NOT_NULL(path); // 验证文件创建成功
    if (!path) return; // 如果文件创建失败，直接返回

    mk_t* mk = mk_create(); // 创建键值存储实例
    mk_load(mk, path); // 从文件加载数据
    CU_ASSERT_STRING_EQUAL(mk_get(mk, "key"), "val"); // 验证空格已被去除
    mk_destroy(mk); // 清理资源
    unlink(path); // 删除临时文件
    free(path); // 释放路径字符串内存
}

// 测试值中包含空格的情况
static void test_load_value_with_spaces(void) {
    char* path = write_temp_file("name=Tom Lee\n"); // 创建值包含空格的测试文件
    CU_ASSERT_PTR_NOT_NULL(path); // 验证文件创建成功
    if (!path) return; // 如果文件创建失败，直接返回

    mk_t* mk = mk_create(); // 创建键值存储实例
    mk_load(mk, path); // 从文件加载数据
    CU_ASSERT_STRING_EQUAL(mk_get(mk, "name"), "Tom Lee"); // 验证空格被保留
    mk_destroy(mk); // 清理资源
    unlink(path); // 删除临时文件
    free(path); // 释放路径字符串内存
}

// 测试值中包含多个等号的情况
static void test_load_multiple_equals(void) {
    char* path = write_temp_file("path=/a=b/c\n"); // 创建值包含等号的测试文件
    CU_ASSERT_PTR_NOT_NULL(path); // 验证文件创建成功
    if (!path) return; // 如果文件创建失败，直接返回

    mk_t* mk = mk_create(); // 创建键值存储实例
    mk_load(mk, path); // 从文件加载数据
    CU_ASSERT_STRING_EQUAL(mk_get(mk, "path"), "/a=b/c"); // 验证等号被正确处理
    mk_destroy(mk); // 清理资源
    unlink(path); // 删除临时文件
    free(path); // 释放路径字符串内存
}

// 测试保存和加载的一致性
static void test_save_load_consistency(void) {
    char* path = write_temp_file(""); // 创建空的临时文件
    CU_ASSERT_PTR_NOT_NULL(path); // 验证文件创建成功
    if (!path) return; // 如果文件创建失败，直接返回

    mk_t* mk1 = mk_create(); // 创建第一个键值存储实例
    mk_set(mk1, "k1", "v1"); // 设置键值对
    mk_set(mk1, "k2", "v2"); // 设置键值对
    CU_ASSERT_EQUAL(mk_save(mk1, path), 0); // 保存到文件

    mk_t* mk2 = mk_create(); // 创建第二个键值存储实例
    CU_ASSERT_EQUAL(mk_load(mk2, path), 0); // 从文件加载数据
    CU_ASSERT_EQUAL(mk_count(mk1), mk_count(mk2)); // 验证计数一致
    CU_ASSERT_STRING_EQUAL(mk_get(mk2, "k1"), "v1"); // 验证数据一致
    CU_ASSERT_STRING_EQUAL(mk_get(mk2, "k2"), "v2"); // 验证数据一致

    mk_destroy(mk1); // 清理第一个实例
    mk_destroy(mk2); // 清理第二个实例
    unlink(path); // 删除临时文件
    free(path); // 释放路径字符串内存
}

// 测试无效键名的处理
static void test_invalid_key(void) {
    CU_ASSERT_NOT_EQUAL(mk_set(kv, "bad key", "val"), 0); // 包含空格的键名应该失败
    CU_ASSERT_NOT_EQUAL(mk_set(kv, "bad*key", "val"), 0); // 包含特殊字符的键名应该失败
    CU_ASSERT_NOT_EQUAL(mk_set(kv, "", "val"), 0); // 空键名应该失败
}

// 测试覆盖操作不会增加计数
static void test_overwrite_does_not_increase_count(void) {
    mk_t* mk = mk_create(); // 创建键值存储实例
    mk_set(mk, "dup", "1"); // 设置初始值
    mk_set(mk, "dup", "2"); // 覆盖为新值
    CU_ASSERT_EQUAL(mk_count(mk), 1); // 验证计数仍为1
    CU_ASSERT_STRING_EQUAL(mk_get(mk, "dup"), "2"); // 验证值已更新
    mk_destroy(mk); // 清理资源
}

// 主函数，初始化测试框架并运行所有测试
int main(void) {
    if (CUE_SUCCESS != CU_initialize_registry()) // 初始化CUnit注册表
        return CU_get_error();

    CU_pSuite pSuite = CU_add_suite("MiniKV_Suite", init_suite, clean_suite); // 添加测试套件
    if (NULL == pSuite) { // 如果添加套件失败
        CU_cleanup_registry();
        return CU_get_error();
    }

    // 添加所有测试用例到套件中
    if ((NULL == CU_add_test(pSuite, "test_basic_set_get", test_basic_set_get)) ||
        (NULL == CU_add_test(pSuite, "test_set_overwrite", test_set_overwrite)) ||
        (NULL == CU_add_test(pSuite, "test_del_existing", test_del_existing)) ||
        (NULL == CU_add_test(pSuite, "test_del_non_existing", test_del_non_existing)) ||
        (NULL == CU_add_test(pSuite, "test_count_after_set_del", test_count_after_set_del)) ||
        (NULL == CU_add_test(pSuite, "test_load_ignore_empty_comments", test_load_ignore_empty_comments)) ||
        (NULL == CU_add_test(pSuite, "test_load_trim", test_load_trim)) ||
        (NULL == CU_add_test(pSuite, "test_load_value_with_spaces", test_load_value_with_spaces)) ||
        (NULL == CU_add_test(pSuite, "test_load_multiple_equals", test_load_multiple_equals)) ||
        (NULL == CU_add_test(pSuite, "test_save_load_consistency", test_save_load_consistency)) ||
        (NULL == CU_add_test(pSuite, "test_invalid_key", test_invalid_key)) ||
        (NULL == CU_add_test(pSuite, "test_overwrite_does_not_increase_count", test_overwrite_does_not_increase_count)))
    {
        CU_cleanup_registry(); // 如果添加测试用例失败，清理注册表
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE); // 设置详细输出模式
    CU_basic_run_tests(); // 运行所有测试
    CU_cleanup_registry(); // 清理注册表
    return CU_get_error(); // 返回错误状态
}
