#define _POSIX_C_SOURCE 200809L
#include "minikv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 去掉字符串两边的空白
char* trim(char* str) {
    if (!str) return NULL;

    // 跳到第一个非空白字符
    while (isspace((unsigned char)*str)) str++;
    // 如果检测到末尾0，返回当前空白字符串
    if (*str == 0) return str;

    // end定位到字符串尾部，末尾0之前
    char* end = str + strlen(str) - 1;
    // 找到尾部第一个空白字符
    while (end > str && isspace((unsigned char)*end)) end--;
    // 将end下一个位置设为新的末尾0
    *(end + 1) = '\0';

    return str;
}

// 验证字符是否在[A-Za-z0-9_.-]范围内
int is_valid_key(const char* key) {
    // 当前字符串为NULL或空字符串直接返回0
    if (!key || *key == '\0') return 0;
    while (*key) {
        // 检测到不属于字母/数字、下划线、点、分隔符的返回0
        if (!isalnum((unsigned char)*key) && *key != '_' && *key != '.' && *key != '-') {
            return 0;
        }
        key++;
    }
    return 1;
}

// 解析单行键值对，返回是否成功解析
// key_out和val_out是输出参数，需要预分配足够空间
int parse_key_value_line(char* line, char** key_out, char** val_out) {
    if (!line || !key_out || !val_out) return 0;

    // 去掉行首尾空白
    line = trim(line);
    
    // 跳过空行或注释行
    if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
        return 0;
    }

    char* eq = strchr(line, '=');
    if (!eq) {
        return 0; // 没有找到等号
    }

    // 把"="改为"\0"分割key和value
    *eq = '\0'; 
    // 各自去掉空白
    char* key = trim(line);
    char* val = trim(eq + 1);
    
    // 验证key和value
    if (!key || !val || !is_valid_key(key)) {
        return 0;
    }

    *key_out = key;
    *val_out = val;
    return 1;
}