/**
 * FLYUX 统一错误报告模块实现
 */

#include "flyuxc/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* 颜色定义 */
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[36m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"

/* 获取错误类型对应的颜色和标签 */
static void get_error_style(ErrorType type, const char **color, const char **label) {
    switch (type) {
        case ERR_ERROR:
            *color = COLOR_RED;
            *label = "Error";
            break;
        case ERR_WARNING:
            *color = COLOR_YELLOW;
            *label = "Warning";
            break;
        case ERR_NOTE:
            *color = COLOR_BLUE;
            *label = "Note";
            break;
        case ERR_HINT:
            *color = COLOR_GREEN;
            *label = "Hint";
            break;
        default:
            *color = COLOR_RED;
            *label = "Error";
    }
}

/* 获取阶段名称 */
static const char *get_phase_name(ErrorPhase phase) {
    switch (phase) {
        case PHASE_LEXER:   return "Lexer";
        case PHASE_PARSER:  return "Parser";
        case PHASE_CODEGEN: return "Codegen";
        case PHASE_RUNTIME: return "Runtime";
        default:            return "";
    }
}

/* 获取指定行的源代码 */
static char *get_source_line(const char *source, int line_num, 
                              const char **line_start_out, const char **line_end_out) {
    if (!source || line_num < 1) return NULL;
    
    const char *p = source;
    int current_line = 1;
    
    /* 找到目标行的开始 */
    while (*p && current_line < line_num) {
        if (*p == '\n') {
            current_line++;
        }
        p++;
    }
    
    if (!*p && current_line < line_num) return NULL;
    
    const char *line_start = p;
    
    /* 找到行末 */
    while (*p && *p != '\n' && *p != '\r') {
        p++;
    }
    const char *line_end = p;
    
    if (line_start_out) *line_start_out = line_start;
    if (line_end_out) *line_end_out = line_end;
    
    size_t line_len = line_end - line_start;
    char *line = malloc(line_len + 1);
    if (!line) return NULL;
    
    memcpy(line, line_start, line_len);
    line[line_len] = '\0';
    return line;
}

/* 计算 UTF-8 字符的显示宽度 */
static int utf8_display_width(unsigned char first_byte) {
    if (first_byte >= 0xF0) return 2;  /* Emoji 等 4 字节字符 */
    if (first_byte >= 0xE0) return 2;  /* 中日韩等 3 字节字符 */
    return 1;
}

/* 计算 UTF-8 字符的字节数 */
static int utf8_char_bytes(unsigned char first_byte) {
    if (first_byte >= 0xF0) return 4;
    if (first_byte >= 0xE0) return 3;
    if (first_byte >= 0xC0) return 2;
    return 1;
}

/* 报告带位置信息的错误 */
void report_error_at(ErrorType type, ErrorPhase phase,
                     const char *source, 
                     int line, int column, int length,
                     const char *message) {
    const char *color, *label;
    get_error_style(type, &color, &label);
    
    /* 输出错误头 */
    fprintf(stderr, "%s%s%s at line %d, column %d", 
            color, label, COLOR_RESET, line, column);
    if (length > 1) {
        fprintf(stderr, "-%d", column + length - 1);
    }
    fprintf(stderr, ": %s\n", message);
    
    /* 如果有源代码，显示上下文 */
    if (source && line > 0) {
        const char *line_start = NULL, *line_end = NULL;
        char *src_line = get_source_line(source, line, &line_start, &line_end);
        
        if (src_line && line_start && line_end) {
            /* 显示行号和源代码 */
            fprintf(stderr, "%s%5d |%s ", COLOR_BLUE, line, COLOR_RESET);
            
            /* 按 UTF-8 字符遍历，高亮错误区域 */
            const char *ptr = line_start;
            int char_pos = 1;
            int error_start = column;
            int error_end = column + (length > 0 ? length : 1) - 1;
            
            while (ptr < line_end) {
                unsigned char c = (unsigned char)*ptr;
                int char_bytes = utf8_char_bytes(c);
                
                /* 开始红色高亮 */
                if (char_pos == error_start) {
                    fprintf(stderr, "%s", color);
                }
                
                /* 输出完整的 UTF-8 字符 */
                for (int i = 0; i < char_bytes && ptr + i < line_end; i++) {
                    fputc(ptr[i], stderr);
                }
                ptr += char_bytes;
                
                /* 结束高亮 */
                if (char_pos == error_end) {
                    fprintf(stderr, "%s", COLOR_RESET);
                }
                
                char_pos++;
            }
            fprintf(stderr, "%s\n", COLOR_RESET);
            
            /* 显示下划线指示器 */
            fprintf(stderr, "%s      |%s ", COLOR_BLUE, COLOR_RESET);
            
            /* 计算视觉列位置 */
            ptr = line_start;
            int visual_col = 1;
            char_pos = 1;
            
            while (ptr < line_end && char_pos < column) {
                unsigned char c = (unsigned char)*ptr;
                int char_bytes = utf8_char_bytes(c);
                int width = utf8_display_width(c);
                
                visual_col += width;
                ptr += char_bytes;
                char_pos++;
            }
            
            /* 输出空格到错误位置 */
            for (int i = 1; i < visual_col; i++) {
                fprintf(stderr, " ");
            }
            
            /* 计算下划线长度 */
            int underline_len = 0;
            int remaining = length > 0 ? length : 1;
            while (ptr < line_end && remaining > 0) {
                unsigned char c = (unsigned char)*ptr;
                int char_bytes = utf8_char_bytes(c);
                int width = utf8_display_width(c);
                
                underline_len += width;
                ptr += char_bytes;
                remaining--;
            }
            if (underline_len == 0) underline_len = 1;
            
            /* 输出下划线 */
            fprintf(stderr, "%s", color);
            for (int i = 0; i < underline_len && i < 50; i++) {
                fprintf(stderr, "^");
            }
            fprintf(stderr, "%s\n", COLOR_RESET);
        }
        
        if (src_line) free(src_line);
    }
}

/* 报告带格式化消息的错误 */
void report_error_at_fmt(ErrorType type, ErrorPhase phase,
                         const char *source,
                         int line, int column, int length,
                         const char *format, ...) {
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    report_error_at(type, phase, source, line, column, length, message);
}

/* 报告简单错误 */
void report_error(ErrorType type, ErrorPhase phase, const char *message) {
    const char *color, *label;
    get_error_style(type, &color, &label);
    const char *phase_name = get_phase_name(phase);
    
    if (phase_name[0]) {
        fprintf(stderr, "%s%s error:%s %s\n", color, phase_name, COLOR_RESET, message);
    } else {
        fprintf(stderr, "%s%s:%s %s\n", color, label, COLOR_RESET, message);
    }
}

/* 报告带格式化消息的简单错误 */
void report_error_fmt(ErrorType type, ErrorPhase phase, 
                      const char *format, ...) {
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    report_error(type, phase, message);
}

/* 构建错误消息字符串 */
char *build_error_message(const char *source,
                          int line, int column, int length,
                          const char *message) {
    const char *line_start = NULL, *line_end = NULL;
    char *src_line = NULL;
    
    if (source && line > 0) {
        src_line = get_source_line(source, line, &line_start, &line_end);
    }
    
    /* 创建位置指示器 */
    char indicator[256] = {0};
    int spaces = column > 0 ? column - 1 : 0;
    if (spaces > 200) spaces = 200;
    for (int i = 0; i < spaces; i++) {
        indicator[i] = ' ';
    }
    int underline_len = length > 0 ? length : 1;
    if (underline_len > 50) underline_len = 50;
    for (int i = 0; i < underline_len; i++) {
        indicator[spaces + i] = '^';
    }
    indicator[spaces + underline_len] = '\0';
    
    /* 构建完整错误消息 */
    size_t buf_size = 512;
    if (src_line) buf_size += strlen(src_line);
    
    char *error_buf = malloc(buf_size);
    if (!error_buf) {
        if (src_line) free(src_line);
        return NULL;
    }
    
    if (src_line) {
        snprintf(error_buf, buf_size,
                 "Error at line %d, column %d: %s\n"
                 "    %d | %s\n"
                 "      | %s",
                 line, column, message,
                 line, src_line,
                 indicator);
    } else {
        snprintf(error_buf, buf_size,
                 "Error at line %d, column %d: %s",
                 line, column, message);
    }
    
    if (src_line) free(src_line);
    return error_buf;
}
