/**
 * FLYUX 统一错误报告模块
 * 
 * 提供统一的错误、警告输出接口，支持：
 * - 彩色输出
 * - 源码位置显示
 * - UTF-8 字符宽度处理
 * - 下划线指示器
 */

#ifndef FLYUXC_ERROR_H
#define FLYUXC_ERROR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 错误类型 */
typedef enum {
    ERR_ERROR,      /* 错误（红色） */
    ERR_WARNING,    /* 警告（黄色） */
    ERR_NOTE,       /* 注释（蓝色） */
    ERR_HINT        /* 提示（绿色） */
} ErrorType;

/* 错误阶段 */
typedef enum {
    PHASE_LEXER,    /* 词法分析 */
    PHASE_PARSER,   /* 语法分析 */
    PHASE_CODEGEN,  /* 代码生成 */
    PHASE_RUNTIME   /* 运行时 */
} ErrorPhase;

/**
 * 报告带位置信息的错误
 * 
 * @param type          错误类型
 * @param phase         错误阶段（可选，用于前缀）
 * @param source        原始源代码（用于显示上下文）
 * @param line          行号（从1开始）
 * @param column        列号（从1开始）
 * @param length        错误标记的长度
 * @param message       错误消息
 */
void report_error_at(ErrorType type, ErrorPhase phase,
                     const char *source, 
                     int line, int column, int length,
                     const char *message);

/**
 * 报告带位置信息的错误（带格式化消息）
 * 
 * @param type          错误类型
 * @param phase         错误阶段
 * @param source        原始源代码
 * @param line          行号
 * @param column        列号
 * @param length        错误标记长度
 * @param format        格式化字符串
 * @param ...           格式化参数
 */
void report_error_at_fmt(ErrorType type, ErrorPhase phase,
                         const char *source,
                         int line, int column, int length,
                         const char *format, ...);

/**
 * 报告简单错误（无位置信息）
 * 
 * @param type          错误类型
 * @param phase         错误阶段
 * @param message       错误消息
 */
void report_error(ErrorType type, ErrorPhase phase, const char *message);

/**
 * 报告简单错误（带格式化消息）
 */
void report_error_fmt(ErrorType type, ErrorPhase phase, 
                      const char *format, ...);

/**
 * 构建错误消息字符串（用于需要存储错误的场景）
 * 返回的字符串需要调用者 free()
 * 
 * @param source        原始源代码
 * @param line          行号
 * @param column        列号
 * @param length        错误标记长度
 * @param message       错误消息
 * @return              格式化的错误消息，需要 free()
 */
char *build_error_message(const char *source,
                          int line, int column, int length,
                          const char *message);

#ifdef __cplusplus
}
#endif

#endif /* FLYUXC_ERROR_H */
