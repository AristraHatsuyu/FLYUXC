// normalize.c
#include "flyuxc/normalize.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * 外部模块声明
 */
extern char* normalize_remove_comments(const char* input);
extern Statement* normalize_split_statements(const char* input, int* stmt_count);
extern void normalize_filter_expressions(Statement* statements, int* count);
extern char* normalize_statement_content(const char* stmt);

/**
 * 释放语句数组
 */
static void free_statements(Statement* statements, int count) {
    if (!statements) return;
    for (int i = 0; i < count; i++) {
        if (statements[i].content) {
            free(statements[i].content);
        }
    }
    free(statements);
}

/**
 * 检查这个 } 是否对应一个函数体
 * 方法：找到匹配的 {，看它前面是否是 )
 */
static int is_closing_function_body(const char* text, int closing_brace_idx) {
    // 从给定索引向后扫描，找到匹配的 {
    int brace_count = 1;
    
    // text[closing_brace_idx] 后面应该是 }，但在调用时可能还没写入
    // 我们需要从 closing_brace_idx - 1 开始向后扫描
    for (int i = closing_brace_idx - 1; i >= 0; i--) {
        if (text[i] == '}') {
            brace_count++;
        } else if (text[i] == '{') {
            brace_count--;
            if (brace_count == 0) {
                // 找到了匹配的 {，它在索引 i 处
                // 检查它前面是什么（跳过空白）
                int j = i - 1;
                while (j >= 0 && (text[j] == ' ' || text[j] == '\t' || text[j] == '\n')) {
                    j--;
                }
                
                // 如果前面是 )，说明这是函数体
                if (j >= 0 && text[j] == ')') {
                    return 1;
                }
                return 0;
            }
        }
    }
    return 0;
}

/**
 * 规范化块内部的换行符为分号（重构版）
 * 目标：
 *  1) 在代码块(如 if/else/L>/函数体)闭合时，若块内最后一条语句未以 ';' 结束，则在 '}' 之前补 ';'
 *  2) 保持原有：在块内（不在 () / []）的换行，若后续非 '}'，则将换行替换为 ';'
 *  3) 严格区分“代码块 { ... }”与“对象字面量 { ... }”，对象字面量结尾不自动补 ';'
 */
static char* normalize_internal_newlines(const char* stmt) {
    if (!stmt) return NULL;

    size_t len = strlen(stmt);
    // 结果缓冲：保守放大一倍空间
    char* result = (char*)malloc(len * 2 + 4);
    if (!result) return NULL;

    // brace 类型栈：1 表示代码块，0 表示对象字面量
    unsigned char* brace_is_block = (unsigned char*)calloc(len + 2, 1);
    if (!brace_is_block) {
        free(result);
        return NULL;
    }

    int out_idx = 0;
    int in_str = 0;
    int escape = 0;
    int brace_depth = 0;
    int paren_depth = 0;
    int bracket_depth = 0;

    for (int i = 0; i < (int)len; i++) {
        char ch = stmt[i];

        /* 转义处理 */
        if (escape) {
            result[out_idx++] = ch;
            escape = 0;
            continue;
        }
        if (ch == '\\') {
            result[out_idx++] = ch;
            escape = 1;
            continue;
        }

        /* 字符串处理（与原实现一致：简单切换，不区分单双引号配对） */
        if (ch == '"' || ch == '\'') {
            in_str = !in_str;
            result[out_idx++] = ch;
            continue;
        }
        if (in_str) {
            result[out_idx++] = ch;
            continue;
        }

        /* 左大括号：判定这是代码块还是对象字面量，并入栈 */
        if (ch == '{') {
            // 回看输出缓冲中最后一个非空白字符
            int last = out_idx - 1;
            while (last >= 0 && (result[last] == ' ' || result[last] == '\t' || result[last] == '\n')) {
                last--;
            }
            unsigned char is_block = 0;
            if (last >= 0) {
                char prev = result[last];
                // 经验规则：出现在 ) / } / ] 之后的 { 是代码块；其他多为对象字面量场景（:=, =, (, [, , 等）
                if (prev == ')' || prev == '}' || prev == ']') {
                    is_block = 1;
                } else {
                    is_block = 0;
                }
            } else {
                // 行首独立的 { ，保守视为对象字面量（几乎不会出现独立代码块从 { 开始的情况）
                is_block = 0;
            }

            // 入栈（下一个深度）
            brace_is_block[brace_depth + 1] = is_block;
            brace_depth++;
            result[out_idx++] = ch;
            continue;
        }

        /* 右大括号：若当前层是代码块，必要时在 '}' 前补 ';' */
        if (ch == '}') {
            if (brace_depth > 0) {
                unsigned char is_block = brace_is_block[brace_depth];
                if (is_block) {
                    // 找到 '}' 之前最后一个非空白字符
                    int last = out_idx - 1;
                    while (last >= 0 && (result[last] == ' ' || result[last] == '\t' || result[last] == '\n')) {
                        last--;
                    }
                    // 若块非空，且最后不是 ';' 且也不是 '{'（排除空块 {}），则在最后实字符之后插入 ';'
                    if (last >= 0 && result[last] != ';' && result[last] != '{') {
                        // 将 [last+1, out_idx) 右移一位，准备插入 ';'
                        for (int j = out_idx; j > last + 1; j--) {
                            result[j] = result[j - 1];
                        }
                        result[last + 1] = ';';
                        out_idx++;
                    }
                }
                // 退出当前层
                brace_depth--;
            }
            result[out_idx++] = ch;
            continue;
        }

        /* 圆/方括号深度追踪（用于判定换行→分号） */
        if (ch == '(') {
            paren_depth++;
        } else if (ch == ')') {
            paren_depth--;
        } else if (ch == '[') {
            bracket_depth++;
        } else if (ch == ']') {
            bracket_depth--;
        }

        /* 块内换行 → 分号（仅在不处于 () / [] 中；且换行后第一个非空不是 '}'） */
        if (ch == '\n' && brace_depth > 0 && paren_depth == 0 && bracket_depth == 0) {
            int j = i + 1;
            while (j < (int)len && (stmt[j] == ' ' || stmt[j] == '\t' || stmt[j] == '\n')) {
                j++;
            }
            if (j < (int)len && stmt[j] != '}') {
                // 检查当前输出末尾是否已经有 ';' / '{' / '('
                int last = out_idx - 1;
                while (last >= 0 && (result[last] == ' ' || result[last] == '\t')) {
                    last--;
                }
                if (last >= 0 && result[last] != ';' && result[last] != '{' && result[last] != '(') {
                    result[out_idx++] = ';';
                }
                // 跳过该换行
                continue;
            } else {
                // 块的结尾（后续是 '}'），跳过该换行，交由 '}' 分支统一补 ';'
                continue;
            }
        }

        // 其他字符，原样写入
        result[out_idx++] = ch;
    }

    result[out_idx] = '\0';
    free(brace_is_block);
    return result;
}

/**
 * 主规范化函数
 */
NormalizeResult flyux_normalize(const char* source_code) {
    NormalizeResult result = {NULL, NULL, 0};
    
    if (!source_code) {
        result.error_msg = "Source code is null";
        result.error_code = -1;
        return result;
    }
    
    // Step 1: 移除注释
    char* no_comments = normalize_remove_comments(source_code);
    if (!no_comments) {
        result.error_msg = "Failed to remove comments";
        result.error_code = -1;
        return result;
    }
    
    // Step 1.5: 规范化块内的换行符为分号
    char* normalized_newlines = normalize_internal_newlines(no_comments);
    free(no_comments);
    if (!normalized_newlines) {
        result.error_msg = "Failed to normalize newlines";
        result.error_code = -1;
        return result;
    }
    
    // Step 2: 分割语句
    int stmt_count = 0;
    Statement* statements = normalize_split_statements(normalized_newlines, &stmt_count);
    free(normalized_newlines);
    
    if (!statements || stmt_count == 0) {
        result.error_msg = "Failed to split statements";
        result.error_code = -1;
        return result;
    }
    
    // Step 3: 过滤表达式（如果有main函数）
    normalize_filter_expressions(statements, &stmt_count);
    
    // Step 4: 规范化每个语句并构建最终代码
    size_t total_len = 0;
    char** normalized_stmts = malloc(stmt_count * sizeof(char*));
    if (!normalized_stmts) {
        free_statements(statements, stmt_count);
        result.error_msg = "Memory allocation failed";
        result.error_code = -1;
        return result;
    }
    
    for (int i = 0; i < stmt_count; i++) {
        normalized_stmts[i] = normalize_statement_content(statements[i].content);
        if (!normalized_stmts[i]) {
            normalized_stmts[i] = malloc(1);
            normalized_stmts[i][0] = '\0';
        }
        total_len += strlen(normalized_stmts[i]) + 1;  // +1 for semicolon
    }
    
    // Step 5: 组合所有语句，添加分号
    char* normalized = malloc(total_len + 1);
    if (!normalized) {
        for (int i = 0; i < stmt_count; i++) {
            free(normalized_stmts[i]);
        }
        free(normalized_stmts);
        free_statements(statements, stmt_count);
        result.error_msg = "Memory allocation failed";
        result.error_code = -1;
        return result;
    }
    
    int out_idx = 0;
    for (int i = 0; i < stmt_count; i++) {
        int len = strlen(normalized_stmts[i]);
        if (len > 0) {
            strcpy(normalized + out_idx, normalized_stmts[i]);
            out_idx += len;
            normalized[out_idx++] = ';';
        }
        free(normalized_stmts[i]);
    }
    normalized[out_idx] = '\0';
    
    free(normalized_stmts);
    free_statements(statements, stmt_count);
    
    result.normalized = normalized;
    result.error_code = 0;
    return result;
}

/**
 * 释放规范化结果
 */
void normalize_result_free(NormalizeResult* result) {
    if (!result) return;
    if (result->normalized) {
        free(result->normalized);
        result->normalized = NULL;
    }
    if (result->error_msg) {
        free(result->error_msg);
        result->error_msg = NULL;
    }
}