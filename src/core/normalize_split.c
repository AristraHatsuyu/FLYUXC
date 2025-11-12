/**
 * 语句分割模块
 * 根据分隔符（;、换行符）分割语句
 * 需要处理括号、大括号、字符串内的分隔符
 */

#include "flyuxc/normalize.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * 检查当前位置是否在字符串内
 */
static int is_in_string_split(const char* text, int pos) {
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
 * 获取括号深度（用于判断是否在括号内）
 * 包括 () [] {}
 */
static int get_bracket_depth(const char* text, int pos) {
    int depth = 0;
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
            continue;
        }
        
        if (!in_str) {
            if (text[i] == '(' || text[i] == '[' || text[i] == '{') {
                depth++;
            } else if (text[i] == ')' || text[i] == ']' || text[i] == '}') {
                depth--;
            }
        }
    }
    
    return depth;
}

/**
 * 检查是否是函数定义开始
 */
static int is_function_definition(const char* stmt) {
    // 查找 := 或 :(type)=
    const char* assign = strstr(stmt, ":=");
    const char* type_assign = strchr(stmt, ':');
    
    if (!assign && !type_assign) {
        return 0;
    }
    
    // 找到赋值操作符后的位置
    const char* after_assign = assign ? (assign + 2) : (type_assign + 1);
    
    // 跳过空白查找 (
    while (*after_assign && isspace(*after_assign)) {
        after_assign++;
    }
    
    return *after_assign == '(';
}

/**
 * 检查是否是main函数定义
 */
static int is_main_function(const char* stmt) {
    // 查找 main
    const char* main_pos = strstr(stmt, "main");
    if (!main_pos) return 0;
    
    // 确保main是标识符的开始
    if (main_pos > stmt && (isalnum(*(main_pos - 1)) || *(main_pos - 1) == '_')) {
        return 0;
    }
    
    // 检查main后面跟的是否是 := 或 :...=
    const char* after_main = main_pos + 4;
    while (*after_main && isspace(*after_main)) {
        after_main++;
    }
    
    return (*after_main == ':');
}

/**
 * 分割语句
 * @param input 输入代码（已移除注释）
 * @param stmt_count 输出：语句数量
 * @return 语句数组（需要释放）
 */
Statement* normalize_split_statements(const char* input, int* stmt_count) {
    if (!input || !stmt_count) {
        return NULL;
    }
    
    size_t len = strlen(input);
    
    // 第一遍：计数分隔符
    int count = 0;
    
    for (int i = 0; i < len; i++) {
        // 检查分隔符（;或换行）且不在括号/字符串内
        if (!is_in_string_split(input, i) && get_bracket_depth(input, i) == 0) {
            if (input[i] == ';' || input[i] == '\n') {
                count++;
            }
        }
    }
    
    if (count == 0) {
        *stmt_count = 0;
        return NULL;
    }
    
    // 分配语句数组
    Statement* statements = malloc(count * sizeof(Statement));
    if (!statements) {
        return NULL;
    }
    
    // 第二遍：提取语句
    int stmt_idx = 0;
    int start = 0;
    int stmt_line = 1;
    
    for (int i = 0; i <= len; i++) {
        if (i < len) {
            if (input[i] == '\n') {
                stmt_line++;
            }
        }
        
        int is_separator = (i == len) || 
                          (!is_in_string_split(input, i) && 
                           get_bracket_depth(input, i) == 0 && 
                           (input[i] == ';' || input[i] == '\n'));
        
        if (is_separator && i > start) {
            // 提取语句内容
            int len_stmt = i - start;
            char* content = malloc(len_stmt + 1);
            if (!content) {
                // 错误处理：释放已分配的内存
                for (int j = 0; j < stmt_idx; j++) {
                    free(statements[j].content);
                }
                free(statements);
                return NULL;
            }
            
            strncpy(content, input + start, len_stmt);
            content[len_stmt] = '\0';
            
            // 跳过空语句（只有空白的语句）
            int has_content = 0;
            for (int j = 0; j < len_stmt; j++) {
                if (!isspace(content[j])) {
                    has_content = 1;
                    break;
                }
            }
            
            if (has_content) {
                // 初始化语句结构
                statements[stmt_idx].content = content;
                statements[stmt_idx].line = stmt_line;
                statements[stmt_idx].column = 0;
                statements[stmt_idx].in_main = 0;
                statements[stmt_idx].is_main_func = is_main_function(content);
                
                // 确定语句类型
                if (strstr(content, ":=") || strchr(content, ':')) {
                    if (is_function_definition(content)) {
                        statements[stmt_idx].type = STMT_FUNCTION;
                    } else if (strchr(content, ':')) {
                        // 检查是否是 :(type)= 格式（常量或显式类型）
                        const char* colon = strchr(content, ':');
                        const char* eq = strchr(content, '=');
                        if (eq && colon < eq) {
                            // :(type)= 格式
                            statements[stmt_idx].type = STMT_CONSTANT;
                        } else {
                            statements[stmt_idx].type = STMT_VARIABLE;
                        }
                    } else {
                        statements[stmt_idx].type = STMT_VARIABLE;
                    }
                } else if (strchr(content, '=')) {
                    statements[stmt_idx].type = STMT_ASSIGNMENT;
                } else {
                    statements[stmt_idx].type = STMT_EXPRESSION;
                }
                
                stmt_idx++;
            } else {
                free(content);
            }
            
            // 移动到下一个语句开始
            start = i + 1;
        }
    }
    
    *stmt_count = stmt_idx;
    return statements;
}
