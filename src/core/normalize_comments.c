#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * 注释移除模块
 * 处理块注释 和行注释 
 * 需要处理字符串内的注释符号不删除的情况
 */

/**
 * 检查当前位置是否在字符串内
 * @param text 文本
 * @param pos 当前位置
 * @return 1表示在字符串内，0表示不在
 */
static int is_in_string(const char* text, int pos) {
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
 * 移除块注释和行注释
 * @param input 输入代码
 * @return 移除注释后的代码（需要free）
 */
char* normalize_remove_comments(const char* input) {
    if (!input) {
        return calloc(1, sizeof(char));
    }
    
    size_t len = strlen(input);
    char* output = malloc(len + 1);
    if (!output) return NULL;
    
    int out_idx = 0;
    int i = 0;
    
    while (i < len) {
        // 检查字符串
        if ((input[i] == '"' || input[i] == '\'') && !is_in_string(input, i)) {
            // 处理字符串：复制整个字符串内容
            char quote = input[i];
            output[out_idx++] = input[i++];
            
            int escape = 0;
            while (i < len) {
                if (escape) {
                    output[out_idx++] = input[i++];
                    escape = 0;
                    continue;
                }
                
                if (input[i] == '\\') {
                    output[out_idx++] = input[i++];
                    escape = 1;
                    continue;
                }
                
                output[out_idx++] = input[i];
                if (input[i] == quote) {
                    i++;
                    break;
                }
                i++;
            }
            continue;
        }
        
        // 检查块注释 /* ... */
        if (i + 1 < len && input[i] == '/' && input[i + 1] == '*' && !is_in_string(input, i)) {
            i += 2;  // 跳过 /*
            while (i + 1 < len) {
                if (input[i] == '*' && input[i + 1] == '/') {
                    i += 2;  // 跳过 */
                    break;
                }
                i++;
            }
            continue;
        }
        
        // 检查行注释 // ...
        if (i + 1 < len && input[i] == '/' && input[i + 1] == '/' && !is_in_string(input, i)) {
            i += 2;  // 跳过 //
            while (i < len && input[i] != '\n') {
                i++;
            }
            // 保留换行符（后续处理时需要识别语句边界）
            if (i < len && input[i] == '\n') {
                output[out_idx++] = '\n';
                i++;
            }
            continue;
        }
        
        // 常规字符
        output[out_idx++] = input[i++];
    }
    
    output[out_idx] = '\0';
    return output;
}
