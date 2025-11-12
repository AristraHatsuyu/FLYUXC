#include "flyuxc/normalize.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * 表达式过滤模块
 * 如果存在main函数，则删除根层的函数调用和其他表达式
 */

/**
 * 检查是否是函数调用表达式
 * 特征：标识符直接跟 (...)，且内容只是这个调用
 */
static int is_function_call(const char* stmt) {
    if (!stmt) return 0;
    
    // 跳过前导空白
    int start = 0;
    while (stmt[start] && isspace(stmt[start])) {
        start++;
    }
    
    // 必须以字母、下划线或unicode字符开始
    if (!isalpha(stmt[start]) && stmt[start] != '_') {
        // 简单的unicode检查（非ASCII字符）
        if ((unsigned char)stmt[start] < 128) {
            return 0;
        }
    }
    
    // 扫描标识符
    int i = start;
    while (stmt[i] && (isalnum(stmt[i]) || stmt[i] == '_')) {
        i++;
    }
    
    // 跳过空白
    while (stmt[i] && isspace(stmt[i])) {
        i++;
    }
    
    // 检查是否直接跟 (
    if (stmt[i] != '(') {
        return 0;
    }
    
    // 检查 () 内部是否匹配
    int paren_count = 1;
    i++;
    while (stmt[i] && paren_count > 0) {
        if (stmt[i] == '(') paren_count++;
        else if (stmt[i] == ')') paren_count--;
        i++;
    }
    
    if (paren_count != 0) {
        return 0;  // 括号不匹配
    }
    
    // 检查是否到了语句末尾（忽略空白）
    while (stmt[i] && isspace(stmt[i])) {
        i++;
    }
    
    return stmt[i] == '\0';
}

/**
 * 标记在main函数内的语句
 * 需要追踪大括号的深度来确定是否在main函数内
 */
static void mark_statements_in_main(Statement* statements, int count) {
    int in_main = 0;
    int brace_depth = 0;
    
    for (int i = 0; i < count; i++) {
        // 检查当前语句是否是main函数定义
        if (statements[i].is_main_func) {
            in_main = 1;
            brace_depth = 0;
        }
        
        if (in_main) {
            statements[i].in_main = 1;
        }
        
        // 计算大括号深度来追踪是否还在main函数内
        const char* content = statements[i].content;
        for (int j = 0; content[j]; j++) {
            if (content[j] == '{') {
                brace_depth++;
            } else if (content[j] == '}') {
                brace_depth--;
                // 如果大括号回到0且这是main函数，说明main函数结束了
                if (brace_depth == 0 && statements[i].is_main_func && in_main) {
                    in_main = 0;
                }
            }
        }
    }
}

/**
 * 过滤表达式
 * 如果存在main函数，删除根层的函数调用
 */
void normalize_filter_expressions(Statement* statements, int* count) {
    if (!statements || !count || *count == 0) {
        return;
    }
    
    // 检查是否存在main函数
    int has_main = 0;
    for (int i = 0; i < *count; i++) {
        if (statements[i].is_main_func) {
            has_main = 1;
            break;
        }
    }
    
    if (!has_main) {
        return;  // 没有main函数，不需要过滤
    }
    
    // 标记在main内的语句
    mark_statements_in_main(statements, *count);
    
    // 删除根层表达式：函数调用和非定义表达式
    int write_idx = 0;
    for (int i = 0; i < *count; i++) {
        int keep = 1;
        
        // 删除以下情况（在main函数外）：
        // 1. 函数调用：func()
        // 2. 其他表达式和赋值
        if (!statements[i].in_main) {
            // 定义语句保留
            if (statements[i].type == STMT_VARIABLE ||
                statements[i].type == STMT_CONSTANT ||
                statements[i].type == STMT_FUNCTION) {
                keep = 1;
            }
            // 根层表达式和赋值删除
            else if (statements[i].type == STMT_EXPRESSION ||
                     statements[i].type == STMT_ASSIGNMENT) {
                keep = 0;
            }
        }
        
        // 移动或删除语句
        if (keep) {
            if (write_idx != i) {
                statements[write_idx] = statements[i];
            }
            write_idx++;
        } else {
            // 释放被删除的语句内存
            free(statements[i].content);
        }
    }
    
    *count = write_idx;
}
