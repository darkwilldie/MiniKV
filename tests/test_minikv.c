#define _POSIX_C_SOURCE 200809L
#include <CUnit/Basic.h>
#include "../include/minikv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static mk_t* kv = NULL;

static int init_suite(void) {
    kv = mk_create();
    return (kv == NULL) ? -1 : 0;
}

static int clean_suite(void) {
    mk_destroy(kv);
    kv = NULL;
    return 0;
}

static char* write_temp_file(const char* content) {
    char template[] = "/tmp/minikv_test_XXXXXX";
    int fd = mkstemp(template);
    if (fd == -1) return NULL;

    FILE* fp = fdopen(fd, "w");
    if (!fp) {
        close(fd);
        unlink(template);
        return NULL;
    }

    fputs(content, fp);
    fclose(fp);
    return strdup(template);
}

static void test_basic_set_get(void) {
    CU_ASSERT_EQUAL(mk_set(kv, "test_key", "test_value"), 0);
    const char* val = mk_get(kv, "test_key");
    CU_ASSERT_PTR_NOT_NULL(val);
    if (val) CU_ASSERT_STRING_EQUAL(val, "test_value");
}

static void test_set_overwrite(void) {
    CU_ASSERT_EQUAL(mk_set(kv, "key1", "val1"), 0);
    CU_ASSERT_EQUAL(mk_set(kv, "key1", "val2"), 0);
    CU_ASSERT_STRING_EQUAL(mk_get(kv, "key1"), "val2");
}

static void test_del_existing(void) {
    CU_ASSERT_EQUAL(mk_set(kv, "del_me", "v"), 0);
    CU_ASSERT_EQUAL(mk_del(kv, "del_me"), 0);
    CU_ASSERT_PTR_NULL(mk_get(kv, "del_me"));
}

static void test_del_non_existing(void) {
    CU_ASSERT_EQUAL(mk_del(kv, "not_exist"), 0);
}

static void test_count_after_set_del(void) {
    mk_t* mk = mk_create();
    CU_ASSERT_EQUAL(mk_count(mk), 0);
    mk_set(mk, "a", "1");
    mk_set(mk, "b", "2");
    CU_ASSERT_EQUAL(mk_count(mk), 2);
    mk_del(mk, "a");
    CU_ASSERT_EQUAL(mk_count(mk), 1);
    mk_destroy(mk);
}

static void test_load_ignore_empty_comments(void) {
    char* path = write_temp_file("\n# comment\n; comment\nvalid=1\n");
    CU_ASSERT_PTR_NOT_NULL(path);
    if (!path) return;

    mk_t* mk = mk_create();
    mk_load(mk, path);
    CU_ASSERT_EQUAL(mk_count(mk), 1);
    CU_ASSERT_STRING_EQUAL(mk_get(mk, "valid"), "1");
    mk_destroy(mk);
    unlink(path);
    free(path);
}

static void test_load_trim(void) {
    char* path = write_temp_file("  key  =  val  \n");
    CU_ASSERT_PTR_NOT_NULL(path);
    if (!path) return;

    mk_t* mk = mk_create();
    mk_load(mk, path);
    CU_ASSERT_STRING_EQUAL(mk_get(mk, "key"), "val");
    mk_destroy(mk);
    unlink(path);
    free(path);
}

static void test_load_value_with_spaces(void) {
    char* path = write_temp_file("name=Tom Lee\n");
    CU_ASSERT_PTR_NOT_NULL(path);
    if (!path) return;

    mk_t* mk = mk_create();
    mk_load(mk, path);
    CU_ASSERT_STRING_EQUAL(mk_get(mk, "name"), "Tom Lee");
    mk_destroy(mk);
    unlink(path);
    free(path);
}

static void test_load_multiple_equals(void) {
    char* path = write_temp_file("path=/a=b/c\n");
    CU_ASSERT_PTR_NOT_NULL(path);
    if (!path) return;

    mk_t* mk = mk_create();
    mk_load(mk, path);
    CU_ASSERT_STRING_EQUAL(mk_get(mk, "path"), "/a=b/c");
    mk_destroy(mk);
    unlink(path);
    free(path);
}

static void test_save_load_consistency(void) {
    char* path = write_temp_file("");
    CU_ASSERT_PTR_NOT_NULL(path);
    if (!path) return;

    mk_t* mk1 = mk_create();
    mk_set(mk1, "k1", "v1");
    mk_set(mk1, "k2", "v2");
    CU_ASSERT_EQUAL(mk_save(mk1, path), 0);

    mk_t* mk2 = mk_create();
    CU_ASSERT_EQUAL(mk_load(mk2, path), 0);
    CU_ASSERT_EQUAL(mk_count(mk1), mk_count(mk2));
    CU_ASSERT_STRING_EQUAL(mk_get(mk2, "k1"), "v1");
    CU_ASSERT_STRING_EQUAL(mk_get(mk2, "k2"), "v2");

    mk_destroy(mk1);
    mk_destroy(mk2);
    unlink(path);
    free(path);
}

static void test_invalid_key(void) {
    CU_ASSERT_NOT_EQUAL(mk_set(kv, "bad key", "val"), 0);
    CU_ASSERT_NOT_EQUAL(mk_set(kv, "bad*key", "val"), 0);
    CU_ASSERT_NOT_EQUAL(mk_set(kv, "", "val"), 0);
}

static void test_overwrite_does_not_increase_count(void) {
    mk_t* mk = mk_create();
    mk_set(mk, "dup", "1");
    mk_set(mk, "dup", "2");
    CU_ASSERT_EQUAL(mk_count(mk), 1);
    CU_ASSERT_STRING_EQUAL(mk_get(mk, "dup"), "2");
    mk_destroy(mk);
}

int main(void) {
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    CU_pSuite pSuite = CU_add_suite("MiniKV_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

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
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
