#ifndef FLYUXC_NORMALIZE_H
#define FLYUXC_NORMALIZE_H

#include <stddef.h>

/**
 * 代码规范化模块
 * 负责将原始FLYUX源代码规范化为单行格式，便于后续词法分析
 */

/**
 * 源码位置映射：记录规范化后每个字符对应的原始源码位置
 */
typedef struct {
    int orig_line;       /* 原始行号（1-based），0表示合成字符 */
    int orig_column;     /* 原始列号（1-based） */
    int orig_length;     /* 原始字符长度（字节数） */
    int is_synthetic;    /* 1=合成字符（如添加的分号），0=来自原始代码 */
} SourceLocation;

/**
 * 语句类型
 */
typedef enum {
    STMT_VARIABLE,      // 变量定义
    STMT_CONSTANT,      // 常量定义
    STMT_FUNCTION,      // 函数定义
    STMT_ASSIGNMENT,    // 赋值语句
    STMT_EXPRESSION,    // 表达式（如函数调用）
    STMT_UNKNOWN        // 未知类型
} StatementType;

/**
 * 语句结构体
 */
typedef struct {
    char* content;          // 语句内容（规范化前）
    StatementType type;     // 语句类型
    int line;               // 原始行号
    int column;             // 原始列号
    int in_main;            // 是否在main函数内
    int is_main_func;       // 是否是main函数定义
} Statement;

/**
 * 规范化结果结构体
 */
typedef struct {
    char* normalized;       // 规范化后的代码
    SourceLocation* source_map;  // 源码位置映射数组
    size_t source_map_size;      // 映射数组长度
    char* error_msg;        // 错误信息（如有）
    int error_code;         // 错误代码
} NormalizeResult;

/**
 * 主规范化入口函数
 * @param source_code 原始FLYUX源代码
 * @return 规范化结果，需要通过normalize_result_free释放
 */
NormalizeResult flyux_normalize(const char* source_code);

/**
 * 释放规范化结果内存
 * @param result 规范化结果指针
 */
void normalize_result_free(NormalizeResult* result);

// 内部使用的辅助函数（可选导出）
char* normalize_remove_comments(const char* input);
Statement* normalize_split_statements(const char* input, int* stmt_count);
void normalize_filter_expressions(Statement* statements, int* count);
char* normalize_statement_content(const char* stmt);

#endif // FLYUXC_NORMALIZE_H