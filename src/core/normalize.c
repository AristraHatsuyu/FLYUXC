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
 * 规范化块内部的换行符为分号
 * 用于处理main函数内部没有显式分号的语句
 */
static char* normalize_internal_newlines(const char* stmt) {
    if (!stmt) return NULL;
    
    size_t len = strlen(stmt);
    char* result = malloc(len * 2 + 1);  // 为可能的分号预留空间
    if (!result) return NULL;
    
    int out_idx = 0;
    int in_str = 0;
    int escape = 0;
    int brace_depth = 0;
    int paren_depth = 0;
    int bracket_depth = 0;
    
    for (int i = 0; i < len; i++) {
        char ch = stmt[i];
        
        // 处理转义
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
        
        // 处理字符串
        if (ch == '"' || ch == '\'') {
            in_str = !in_str;
            result[out_idx++] = ch;
            continue;
        }
        
        if (in_str) {
            result[out_idx++] = ch;
            continue;
        }
        
        // 处理左大括号
        if (ch == '{') {
            brace_depth++;
            result[out_idx++] = ch;
            continue;
        }
        
        // 处理右大括号
        if (ch == '}') {
            // 如果这是函数体的最后一个 }（brace_depth == 1），检查是否需要分号
            // 但首先要确认这个语句确实是函数体
            if (brace_depth == 1 && is_closing_function_body(result, out_idx)) {
                int last = out_idx - 1;
                while (last >= 0 && (result[last] == ' ' || result[last] == '\t' || result[last] == '\n')) {
                    last--;
                }
                // 在函数体末尾，任何非分号、非开括号的内容后面都需要分号
                if (last >= 0 && result[last] != ';' && result[last] != '{') {
                    // 需要添加分号
                    for (int j = out_idx; j > last + 1; j--) {
                        result[j] = result[j - 1];
                    }
                    result[last + 1] = ';';
                    out_idx++;
                }
            }
            
            if (brace_depth > 0) brace_depth--;
            result[out_idx++] = ch;
            continue;
        }
        
        // 跟踪圆括号
        if (ch == '(') {
            paren_depth++;
        } else if (ch == ')') {
            paren_depth--;
        }
        
        // 跟踪方括号
        if (ch == '[') {
            bracket_depth++;
        } else if (ch == ']') {
            bracket_depth--;
        }
        
        // 在函数块内，把换行符替换为分号
        if (ch == '\n' && brace_depth > 0 && paren_depth == 0 && bracket_depth == 0) {
            // 检查后续是否是闭括号
            int j = i + 1;
            while (j < len && (stmt[j] == ' ' || stmt[j] == '\t' || stmt[j] == '\n')) {
                j++;
            }
            
            if (j < len && stmt[j] != '}') {
                // 不是块的结尾，检查是否需要添加分号
                if (out_idx > 0) {
                    int last = out_idx - 1;
                    // 去除末尾空白
                    while (last >= 0 && (result[last] == ' ' || result[last] == '\t')) {
                        last--;
                    }
                    if (last >= 0 && result[last] != ';' && result[last] != '{' && result[last] != '(') {
                        result[out_idx++] = ';';
                    }
                }
                continue;  // 跳过换行符
            } else {
                // 块的结尾，跳过换行符
                continue;
            }
        }
        
        result[out_idx++] = ch;
    }
    
    result[out_idx] = '\0';
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