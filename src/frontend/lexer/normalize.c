// normalize.c
#include "flyuxc/frontend/normalize.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/**
 * 外部模块声明
 */
extern char* normalize_remove_comments(const char* input);
extern Statement* normalize_split_statements(const char* input, int* stmt_count);
extern void normalize_filter_expressions(Statement* statements, int* count);
extern char* normalize_statement_content(const char* stmt);

/**
 * 检查字符是否可以作为标识符的开始
 * 允许：字母、下划线、中文、emoji等非ASCII字符
 */
static int is_valid_identifier_start(const char* str, int* bytes_consumed) {
    unsigned char c = (unsigned char)str[0];
    
    // ASCII字母或下划线
    if (isalpha(c) || c == '_') {
        *bytes_consumed = 1;
        return 1;
    }
    
    // 数字不能作为开始
    if (isdigit(c)) {
        *bytes_consumed = 1;
        return 0;
    }
    
    // UTF-8多字节字符（中文、emoji等）
    if (c >= 0x80) {
        // 简单检查：所有非ASCII字符都允许
        // 计算UTF-8字符的字节数
        if ((c & 0xE0) == 0xC0) {
            *bytes_consumed = 2;
        } else if ((c & 0xF0) == 0xE0) {
            *bytes_consumed = 3;
        } else if ((c & 0xF8) == 0xF0) {
            *bytes_consumed = 4;
        } else {
            *bytes_consumed = 1;
        }
        return 1;
    }
    
    // 其他字符不允许
    *bytes_consumed = 1;
    return 0;
}

/**
 * 验证变量声明：检查 := 左侧是否为有效标识符
 * 如果发现无效标识符（如数字、字符串字面量等），返回错误信息
 */
static char* validate_variable_declarations(const char* source) {
    if (!source) return NULL;
    
    const char* p = source;
    int line = 1;
    int column = 1;
    
    while (*p) {
        // 跳过空白
        while (*p && isspace(*p)) {
            if (*p == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            p++;
        }
        
        if (!*p) break;
        
        // 检查是否遇到 :=
        if (p[0] == ':' && p[1] == '=') {
            // 回溯找到 := 左侧的token
            const char* left_end = p - 1;
            
            // 跳过 := 左侧的空白
            while (left_end >= source && isspace(*left_end)) {
                left_end--;
            }
            
            if (left_end < source) {
                // := 前面什么都没有
                static char error_buf[256];
                snprintf(error_buf, sizeof(error_buf), 
                         "Line %d: Missing identifier before ':='", line);
                return strdup(error_buf);
            }
            
            // 找到左侧token的开始
            const char* left_start = left_end;
            
            // 检查最后一个字符，判断token类型
            if (*left_end == ')' || *left_end == ']' || *left_end == '}') {
                // 可能是 arr[i] := x 或 obj.prop := x，这是合法的
                // 暂时跳过，让parser处理更复杂的情况
                p += 2;
                column += 2;
                continue;
            }
            
            if (*left_end == '"' || *left_end == '\'') {
                // 字符串字面量
                static char error_buf[256];
                snprintf(error_buf, sizeof(error_buf), 
                         "Line %d: String literal cannot be used as variable name", line);
                return strdup(error_buf);
            }
            
            // 向前扫描找到token开始
            while (left_start > source) {
                unsigned char c = (unsigned char)*(left_start - 1);
                if (isalnum(c) || c == '_' || c >= 0x80 || c == '.') {
                    left_start--;
                } else {
                    break;
                }
            }
            
            // 检查token开始字符
            int bytes_consumed = 0;
            if (!is_valid_identifier_start(left_start, &bytes_consumed)) {
                // 无效的标识符开始
                static char error_buf[256];
                
                // 判断是什么类型的无效token
                if (isdigit((unsigned char)*left_start)) {
                    snprintf(error_buf, sizeof(error_buf), 
                             "Line %d: Number literal cannot be used as variable name", line);
                } else if (*left_start == '"' || *left_start == '\'') {
                    snprintf(error_buf, sizeof(error_buf), 
                             "Line %d: String literal cannot be used as variable name", line);
                } else if (strncmp(left_start, "true", 4) == 0 || strncmp(left_start, "false", 5) == 0) {
                    snprintf(error_buf, sizeof(error_buf), 
                             "Line %d: Boolean literal cannot be used as variable name", line);
                } else {
                    snprintf(error_buf, sizeof(error_buf), 
                             "Line %d: Invalid character '%c' at start of identifier", line, *left_start);
                }
                return strdup(error_buf);
            }
            
            // 检查是否是布尔字面量
            int token_len = left_end - left_start + 1;
            if ((token_len == 4 && strncmp(left_start, "true", 4) == 0) ||
                (token_len == 5 && strncmp(left_start, "false", 5) == 0)) {
                static char error_buf[256];
                snprintf(error_buf, sizeof(error_buf), 
                         "Line %d: Boolean literal cannot be used as variable name", line);
                return strdup(error_buf);
            }
            
            p += 2;
            column += 2;
            continue;
        }
        
        // 跳过其他字符
        if (*p == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        p++;
    }
    
    return NULL;  // 验证通过
}

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
                // 经验规则：出现在 ) / } / ] 之后的 { 是代码块
                // 出现在 = / := / , / : / [ / ( 之后的 { 是对象字面量
                if (prev == ')' || prev == '}' || prev == ']') {
                    is_block = 1;
                } else if (prev == '=' || prev == ',' || prev == ':' || prev == '[' || prev == '(') {
                    is_block = 0;
                } else {
                    // 其他情况（如操作符、标识符后），默认为代码块
                    is_block = 1;
                }
            } else {
                // 行首或文件开头的 { ，视为代码块（顶层代码块）
                is_block = 1;
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

        /* 块内换行 → 分号（仅在不处于 () / [] 中；且换行后第一个非空不是 '}'；且当前是代码块而非对象字面量） */
        if (ch == '\n' && brace_depth > 0 && paren_depth == 0 && bracket_depth == 0) {
            // 检查当前是否在代码块中（而不是对象字面量中）
            unsigned char is_block = brace_is_block[brace_depth];
            
            if (getenv("DEBUG_NORM")) {
                fprintf(stderr, "[DEBUG] Newline at i=%d, brace_depth=%d, is_block=%d\n", i, brace_depth, is_block);
            }
            
            // 只在代码块中添加分号，对象字面量中保留换行
            if (is_block) {
                int j = i + 1;
                while (j < (int)len && (stmt[j] == ' ' || stmt[j] == '\t' || stmt[j] == '\n')) {
                    j++;
                }
                if (j < (int)len && stmt[j] != '}') {
                    // 检查当前输出末尾（去除空格后）的最后字符
                    int last = out_idx - 1;
                    while (last >= 0 && (result[last] == ' ' || result[last] == '\t')) {
                        last--;
                    }
                    
                    if (getenv("DEBUG_NORM")) {
                        fprintf(stderr, "[DEBUG] last=%d, last_char='%c', next_char='%c'\n", 
                                last, last >= 0 ? result[last] : '?', stmt[j]);
                    }
                    
                    // 如果已经有分号、或者是开括号，则不添加
                    if (last >= 0 && (result[last] == ';' || result[last] == '{' || result[last] == '(')) {
                        // 跳过换行
                        if (getenv("DEBUG_NORM")) {
                            fprintf(stderr, "[DEBUG] Skipping semicolon (already have delimiter)\n");
                        }
                        continue;
                    }
                    
                    // 否则添加分号
                    if (getenv("DEBUG_NORM")) {
                        fprintf(stderr, "[DEBUG] Adding semicolon\n");
                    }
                    result[out_idx++] = ';';
                    // 跳过该换行
                    continue;
                } else {
                    // 块的结尾（后续是 '}'），跳过该换行，交由 '}' 分支统一补 ';'
                    if (getenv("DEBUG_NORM")) {
                        fprintf(stderr, "[DEBUG] Skipping newline before '}'\n");
                    }
                    continue;
                }
            } else {
                // 对象字面量中的换行，转换为空格以便解析
                result[out_idx++] = ' ';
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
    NormalizeResult result = {NULL, NULL, 0, NULL, 0};
    
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
    
    // DEBUG
    if (getenv("DEBUG_NORM")) {
        fprintf(stderr, "=== AFTER REMOVE COMMENTS ===\n%s\n=== END ===\n", no_comments);
    }
    
    // Step 1.3: 验证变量声明（检查 := 左侧是否为有效标识符）
    char* validation_error = validate_variable_declarations(no_comments);
    if (validation_error) {
        result.error_msg = validation_error;
        result.error_code = -1;
        free(no_comments);
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
    
    // DEBUG
    if (getenv("DEBUG_NORM")) {
        fprintf(stderr, "=== AFTER NORMALIZE NEWLINES ===\n%s\n=== END ===\n", normalized_newlines);
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
    
    // 构建源码位置映射：记录每个规范化字符对应的原始位置
    size_t norm_len = strlen(normalized);
    SourceLocation* source_map = malloc(norm_len * sizeof(SourceLocation));
    if (!source_map) {
        free(normalized);
        result.error_msg = "Memory allocation failed for source_map";
        result.error_code = -1;
        return result;
    }
    
    // 构建字符级映射
    int orig_line = 1, orig_col = 1;
    size_t src_idx = 0, norm_idx = 0;
    size_t src_len = strlen(source_code);
    int src_in_string = 0;  // 跟踪原始源码是否在字符串内
    char src_string_quote = '\0';  // 字符串的引号类型
    
    // 主循环：扫描normalized的每个字符，找到其在原始源码中的位置
    while (norm_idx < norm_len) {
        // 内循环：跳过原始源码中的注释、换行、空白，找到下一个有效字符
        while (src_idx < src_len) {
            char src_ch = source_code[src_idx];
            
            // 处理字符串状态
            if (src_ch == '"' || src_ch == '\'') {
                if (!src_in_string) {
                    src_in_string = 1;
                    src_string_quote = src_ch;
                } else if (src_ch == src_string_quote) {
                    // 检查是否是转义的引号
                    int is_escaped = 0;
                    if (src_idx > 0 && source_code[src_idx - 1] == '\\') {
                        // 简单检测：前一个是反斜杠（可能需要更复杂的转义检测）
                        is_escaped = 1;
                    }
                    if (!is_escaped) {
                        src_in_string = 0;
                        src_string_quote = '\0';
                    }
                }
            }
            
            // 只在字符串外跳过注释
            if (!src_in_string) {
                // 跳过块注释 /* ... */
                if (src_ch == '/' && src_idx + 1 < src_len && source_code[src_idx + 1] == '*') {
                    src_idx += 2;
                    orig_col += 2;
                    while (src_idx + 1 < src_len) {
                        if (source_code[src_idx] == '\n') {
                            orig_line++;
                            orig_col = 1;
                            src_idx++;
                        } else {
                            // 按UTF-8字符计数列号
                            unsigned char byte = (unsigned char)source_code[src_idx];
                            int bytes_to_skip = 1;
                            if (byte >= 0xF0) bytes_to_skip = 4;
                            else if (byte >= 0xE0) bytes_to_skip = 3;
                            else if (byte >= 0xC0) bytes_to_skip = 2;
                            
                            src_idx += bytes_to_skip;
                            orig_col++;  // 一个UTF-8字符只加1列
                        }
                        
                        if (src_idx + 1 < src_len && source_code[src_idx] == '*' && source_code[src_idx + 1] == '/') {
                            src_idx += 2;
                            orig_col += 2;
                            break;
                        }
                    }
                    continue;
                }
                
                // 跳过行注释 //...
                if (src_ch == '/' && src_idx + 1 < src_len && source_code[src_idx + 1] == '/') {
                    src_idx += 2;
                    orig_col += 2;  // 跳过 //
                    while (src_idx < src_len && source_code[src_idx] != '\n') {
                        // 按UTF-8字符计数列号
                        unsigned char byte = (unsigned char)source_code[src_idx];
                        int bytes_to_skip = 1;
                        if (byte >= 0xF0) bytes_to_skip = 4;
                        else if (byte >= 0xE0) bytes_to_skip = 3;
                        else if (byte >= 0xC0) bytes_to_skip = 2;
                        
                        src_idx += bytes_to_skip;
                        orig_col++;  // 一个UTF-8字符只加1列
                    }
                    if (src_idx < src_len && source_code[src_idx] == '\n') {
                        src_idx++;
                        orig_line++;
                        orig_col = 1;
                    }
                    continue;
                }
                
                // 跳过换行
                if (src_ch == '\n') {
                    orig_line++;
                    orig_col = 1;
                    src_idx++;
                    continue;
                }
                
                // 跳过空白
                if (src_ch == ' ' || src_ch == '\t' || src_ch == '\r') {
                    orig_col++;
                    src_idx++;
                    continue;
                }
            } else {
                // 在字符串内，不跳过注释和空白，只跳过字符串外的换行（不应该有）
                // 字符串内的所有字符都需要匹配
            }
            
            // 找到有效字符，跳出内循环
            break;
        }
        
        // 现在src_idx指向下一个有效字符（或已到文件末尾）
        // 尝试匹配normalized[norm_idx]
        
        if (src_idx >= src_len) {
            // 原始源码已结束，normalized还有字符 → 必定是synthetic
            if (normalized[norm_idx] == ';') {
                source_map[norm_idx].orig_line = 0;
                source_map[norm_idx].orig_column = 0;
                source_map[norm_idx].orig_length = 0;
                source_map[norm_idx].is_synthetic = 1;
                norm_idx++;
                continue;
            }
            // 其他情况（不应该发生）
            norm_idx++;
            continue;
        }
        
        char src_ch = source_code[src_idx];
        
        // 尝试匹配
        if (src_ch == normalized[norm_idx]) {
            // 匹配成功！
            source_map[norm_idx].orig_line = orig_line;
            source_map[norm_idx].orig_column = orig_col;
            source_map[norm_idx].is_synthetic = 0;
            
            // 判断UTF-8字符长度
            int char_bytes = 1;
            if ((unsigned char)src_ch >= 0x80) {
                if ((unsigned char)src_ch >= 0xF0) char_bytes = 4;
                else if ((unsigned char)src_ch >= 0xE0) char_bytes = 3;
                else if ((unsigned char)src_ch >= 0xC0) char_bytes = 2;
                else char_bytes = 0;  // UTF-8后续字节
            }
            
            if (char_bytes > 0) {
                // 字符首字节
                source_map[norm_idx].orig_length = char_bytes;
                
                // 处理多字节字符的后续字节
                for (int b = 1; b < char_bytes && (norm_idx + b) < norm_len && (src_idx + b) < src_len; b++) {
                    if (source_code[src_idx + b] == normalized[norm_idx + b]) {
                        source_map[norm_idx + b].orig_line = orig_line;
                        source_map[norm_idx + b].orig_column = orig_col;
                        source_map[norm_idx + b].orig_length = 0;
                        source_map[norm_idx + b].is_synthetic = 0;
                    }
                }
                
                // 前进
                norm_idx += char_bytes;
                src_idx += char_bytes;
                orig_col++;
            } else {
                // UTF-8后续字节，已处理
                src_idx++;
            }
        } else {
            // 不匹配：可能是synthetic分号
            if (normalized[norm_idx] == ';') {
                source_map[norm_idx].orig_line = 0;
                source_map[norm_idx].orig_column = 0;
                source_map[norm_idx].orig_length = 0;
                source_map[norm_idx].is_synthetic = 1;
                norm_idx++;
                // 不前进src_idx，继续用当前原始位置
            } else {
                // 其他不匹配：跳过原始字符（不应该发生）
                src_idx++;
                orig_col++;
            }
        }
    }
    
    // 标记合成字符（normalize添加的分号等）
    while (norm_idx < norm_len) {
        source_map[norm_idx].orig_line = 0;  // 0表示合成
        source_map[norm_idx].orig_column = 0;
        source_map[norm_idx].orig_length = 0;
        source_map[norm_idx].is_synthetic = 1;
        norm_idx++;
    }
    
    result.normalized = normalized;
    result.source_map = source_map;
    result.source_map_size = norm_len;
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
    if (result->source_map) {
        free(result->source_map);
        result->source_map = NULL;
    }
    if (result->error_msg) {
        free(result->error_msg);
        result->error_msg = NULL;
    }
}