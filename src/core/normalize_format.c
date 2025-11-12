#include "flyuxc/normalize.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * 格式规范化模块
 * 处理空白、括号嵌套等
 */

/**
 * 检查位置是否在字符串内
 */
static int is_in_string_format(const char* text, int pos) {
    int in_str = 0;
    int escape = 0;
    
    for (int i = 0; i < pos && text[i] != '\0'; i++) {
        if (escape) {
            escape = 0;
            continue;
        }
        
        if (text[i] == '\\') {
            escape = 1;
            continue;
        }
        
        if (text[i] == '"' || text[i] == '\'') {
            in_str = !in_str;
        }
    }
    
    return in_str;
}

/**
 * 规范化空白和操作符周围的空格
 * 删除所有不必要的空白
 */
static char* normalize_whitespace(const char* stmt) {
    if (!stmt) return NULL;
    
    size_t len = strlen(stmt);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    int out_idx = 0;
    
    for (int i = 0; i < len; i++) {
        char ch = stmt[i];
        
        // 如果是空白字符
        if (isspace(ch)) {
            // 跳过连续空白
            while (i + 1 < len && isspace(stmt[i + 1])) {
                i++;
            }
            // 不在开头、结尾且不在多个空白之后才保留一个空白
            if (out_idx > 0 && i + 1 < len && !isspace(stmt[i + 1])) {
                // 检查是否需要空白
                char prev = result[out_idx - 1];
                char next = stmt[i + 1];
                
                // 某些情况下需要保留空白，但我们要尽量删除
                // 除非是标识符之间需要分隔
                int prev_is_id_char = isalnum(prev) || prev == '_' || prev == ')' || prev == ']' || prev == '}';
                int next_is_id_char = isalnum(next) || next == '_' || next == '(';
                
                if (prev_is_id_char && next_is_id_char) {
                    // 需要空白来分隔标识符，但我们尽量避免
                    // 除了特定关键字情况
                    // 这里简化处理：删除所有空白
                }
            }
            continue;
        }
        
        result[out_idx++] = ch;
    }
    
    result[out_idx] = '\0';
    return result;
}

/**
 * 移除多余的括号嵌套
 * 例如: (((x))) -> (x)
 */
static char* remove_redundant_parens(const char* expr) {
    if (!expr || strlen(expr) == 0) return NULL;
    
    size_t len = strlen(expr);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    // 重复删除多余括号，直到没有变化
    int changed = 1;
    char* current = malloc(len + 1);
    strcpy(current, expr);
    
    while (changed) {
        changed = 0;
        strcpy(result, current);
        
        int out_idx = 0;
        int i = 0;
        
        while (i < strlen(current)) {
            // 如果是括号，检查是否可以简化
            if (current[i] == '(' && !is_in_string_format(current, i)) {
                // 找到匹配的 )
                int paren = 1;
                int j = i + 1;
                
                while (j < strlen(current) && paren > 0) {
                    if (current[j] == '(' && !is_in_string_format(current, j)) {
                        paren++;
                    } else if (current[j] == ')' && !is_in_string_format(current, j)) {
                        paren--;
                    }
                    j++;
                }
                
                if (paren == 0) {
                    j--;  // 指向 )
                    
                    // 检查内部是否整个都被包围了
                    int inner_start = i + 1;
                    int inner_end = j - 1;
                    
                    // 检查内部括号配对
                    int inner_paren = 0;
                    int can_remove = 1;
                    
                    for (int k = inner_start; k <= inner_end; k++) {
                        if (current[k] == '(' && !is_in_string_format(current, k)) {
                            inner_paren++;
                        } else if (current[k] == ')' && !is_in_string_format(current, k)) {
                            inner_paren--;
                            if (inner_paren < 0) {
                                can_remove = 0;
                                break;
                            }
                        }
                    }
                    
                    // 如果内部完全被单层括号包围且不是函数调用
                    if (can_remove && inner_paren == 0) {
                        // 检查是否是 (single_expr)
                        // 这种情况下可以移除外层括号
                        
                        // 但是我们需要保留函数参数列表的括号
                        // 简化：保留所有括号，只删除完全冗余的
                    }
                }
                
                result[out_idx++] = current[i];
                i++;
            } else {
                result[out_idx++] = current[i];
                i++;
            }
        }
        
        result[out_idx] = '\0';
        
        if (strlen(result) < strlen(current)) {
            changed = 1;
            strcpy(current, result);
        }
    }
    
    free(current);
    return result;
}

/**
 * 简化括号：(((expr))) -> (expr)
 * 处理任何被多层括号包围的表达式
 */
static char* simplify_parens(const char* expr) {
    if (!expr || strlen(expr) == 0) return NULL;
    
    size_t len = strlen(expr);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    strcpy(result, expr);
    
    // 循环扫描，每次寻找 ((...)) 的模式并简化为 (...)
    int changed = 1;
    while (changed) {
        changed = 0;
        len = strlen(result);
        char* output = malloc(len + 1);
        if (!output) break;
        
        int out_idx = 0;
        int i = 0;
        
        while (i < len) {
            // 寻找括号对
            if (result[i] == '(' && !is_in_string_format(result, i)) {
                int open_pos = i;
                
                // 找到匹配的 )
                int paren_count = 1;
                int j = i + 1;
                int close_pos = -1;
                
                while (j < len && paren_count > 0) {
                    if (result[j] == '(' && !is_in_string_format(result, j)) {
                        paren_count++;
                    } else if (result[j] == ')' && !is_in_string_format(result, j)) {
                        paren_count--;
                        if (paren_count == 0) {
                            close_pos = j;
                        }
                    }
                    j++;
                }
                
                if (close_pos > 0) {
                    // 检查内部是否是 (...)
                    int inner_start = open_pos + 1;
                    int inner_end = close_pos - 1;
                    
                    // 跳过内部前导/末尾空白（实际上空白已被规范化，但为安全起见还是检查）
                    while (inner_start <= inner_end && result[inner_start] == ' ') {
                        inner_start++;
                    }
                    while (inner_end >= inner_start && result[inner_end] == ' ') {
                        inner_end--;
                    }
                    
                    // 检查内部是否以 ( 开头
                    if (inner_start <= inner_end && 
                        result[inner_start] == '(' && 
                        result[inner_end] == ')' &&
                        !is_in_string_format(result, inner_start)) {
                        
                        // 检查这个内部 ( 是否匹配最后的 )
                        int inner_paren = 1;
                        int k = inner_start + 1;
                        int inner_close = -1;
                        
                        while (k <= inner_end && inner_paren > 0) {
                            if (result[k] == '(' && !is_in_string_format(result, k)) {
                                inner_paren++;
                            } else if (result[k] == ')' && !is_in_string_format(result, k)) {
                                inner_paren--;
                                if (inner_paren == 0) {
                                    inner_close = k;
                                }
                            }
                            k++;
                        }
                        
                        // 如果内层 ) 恰好在末尾，说明整个内容被内层括号包围
                        if (inner_close == inner_end) {
                            // 检查是否有顶级逗号
                            int has_toplevel_comma = 0;
                            int check_paren = 0;
                            
                            for (int x = open_pos + 1; x < close_pos; x++) {
                                if (result[x] == '(' && !is_in_string_format(result, x)) {
                                    check_paren++;
                                } else if (result[x] == ')' && !is_in_string_format(result, x)) {
                                    check_paren--;
                                } else if (result[x] == ',' && check_paren == 0 && !is_in_string_format(result, x)) {
                                    has_toplevel_comma = 1;
                                    break;
                                }
                            }
                            
                            // 如果没有顶级逗号，删除外层括号
                            if (!has_toplevel_comma) {
                                // 复制外层括号内的内容（即删除外层）
                                for (int x = open_pos + 1; x < close_pos; x++) {
                                    output[out_idx++] = result[x];
                                }
                                i = close_pos + 1;
                                changed = 1;
                                continue;
                            }
                        }
                    }
                    
                    // 没有简化，复制括号对
                    for (int x = open_pos; x <= close_pos; x++) {
                        output[out_idx++] = result[x];
                    }
                    i = close_pos + 1;
                } else {
                    // 括号不匹配，直接复制
                    output[out_idx++] = result[i];
                    i++;
                }
            } else {
                output[out_idx++] = result[i];
                i++;
            }
        }
        
        output[out_idx] = '\0';
        
        if (strcmp(output, result) == 0) {
            free(output);
            break;  // 没有变化
        }
        
        free(result);
        result = output;
    }
    
    return result;
}

/**
 * 规范化单个语句内容
 */
char* normalize_statement_content(const char* stmt) {
    if (!stmt) return NULL;
    
    // 第一步：规范化空白
    char* temp = normalize_whitespace(stmt);
    if (!temp) return NULL;
    
    // 第二步：简化多余括号（迭代多次确保彻底）
    char* result = temp;
    char* prev = NULL;
    
    for (int iter = 0; iter < 10; iter++) {  // 最多迭代10次
        char* next = simplify_parens(result);
        if (next == NULL) {
            break;
        }
        
        if (strcmp(next, result) == 0) {
            // 没有变化，停止迭代
            if (prev) free(prev);
            free(next);
            break;
        }
        
        // 保存前一个结果以便释放
        if (result != temp) {
            if (prev) free(prev);
            prev = result;
        }
        result = next;
    }
    
    // 清理临时指针
    if (prev && prev != temp) {
        free(prev);
    }
    if (result != temp && temp) {
        free(temp);
    }
    
    return result;
}
