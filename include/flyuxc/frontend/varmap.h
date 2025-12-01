#ifndef FLYUXC_VARMAP_H
#define FLYUXC_VARMAP_H

#include <stddef.h>
#include <stdio.h>
#include "normalize.h"  // 引入 SourceLocation 定义

/**
 * 变量种类（目前主要用于扩展，暂时可全部 UNKNOWN）
 */
typedef enum {
    VARKIND_UNKNOWN = 0,
    VARKIND_LOCAL,
    VARKIND_PARAM,
    VARKIND_GLOBAL
} VarKind;

/**
 * 单个变量映射项
 */
typedef struct {
    char*   original;     // 原始名字，例如 "x"、"🚀"
    char*   mapped;       // 映射后的名字，例如 "_00001"
    VarKind kind;         // 变量类别（暂未精细区分，默认 UNKNOWN）
    int     first_line;   // 首次出现的行号（目前填 0，占位）
    int     first_column; // 首次出现的列号（目前填 0，占位）
} VarMapEntry;

/**
 * 变量映射结果
 */
typedef struct {
    char* mapped_source;         /* Variable-mapped source code */
    VarMapEntry* entries;       /* Mapping table entries */
    size_t entry_count;         /* Number of entries */
    size_t* offset_map;         /* mapped_offset → normalized_offset 映射 */
    size_t offset_map_size;     /* offset_map 数组大小 */
    char* error_msg;            /* Error message (if any) */
    int error_code;             /* 0 = success, non-zero = error */
} VarMapResult;

/**
 * 对规范化后的 FLYUX 源码进行变量名映射。
 * 只映射"标识符 token"，并跳过关键字、类型名、布尔/特殊字面量等。
 *
 * @param normalized_source 来自 flyux_normalize(...) 的 normalized 字符串
 * @param source_map 来自 flyux_normalize(...) 的源码位置映射
 * @param source_map_size 源码位置映射数组长度
 * @param original_source 原始源代码（用于错误报告）
 * @return 映射结果，需要调用 varmap_result_free 释放
 */
VarMapResult flyux_varmap_process(const char* normalized_source, 
                                  const SourceLocation* source_map,
                                  size_t source_map_size,
                                  const char* original_source);

/**
 * 释放 VarMapResult 内部动态资源。
 */
void varmap_result_free(VarMapResult* result);

/**
 * 调试辅助：将映射表打印到指定 FILE*。
 * 输出格式示例：
 *   [1] x -> _00001 (UNKNOWN)
 *   [2] 🚀 -> _00002 (UNKNOWN)
 */
void varmap_print_table(const VarMapResult* result, FILE* out);

#endif // FLYUXC_VARMAP_H
