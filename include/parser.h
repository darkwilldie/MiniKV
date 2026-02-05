#ifndef PARSER_H
#define PARSER_H

/**
 * 去掉字符串两边的空白字符
 * @param str 要处理的字符串（会被修改）
 * @return 处理后的字符串指针
 */
char* trim(char* str);

/**
 * 验证键名是否有效
 * @param key 要验证的键名
 * @return 有效返回1，无效返回0
 */
int is_valid_key(const char* key);

/**
 * 解析单行键值对
 * @param line 要解析的行（会被修改）
 * @param key_out 输出参数，指向解析出的键
 * @param val_out 输出参数，指向解析出的值
 * @return 解析成功返回1，失败返回0
 */
int parse_key_value_line(char* line, char** key_out, char** val_out);

#endif // PARSER_H