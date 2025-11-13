#ifndef FLYUXC_VARMAP_H
#define FLYUXC_VARMAP_H

#include <stddef.h>
#include <stdio.h>

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
    char*        mapped_source;   // 已将变量名替换后的完整源码
    VarMapEntry* entries;         // 映射表
    size_t       count;           // 映射表长度

    char*        error_msg;       // 错误信息（如有）
    int          error_code;      // 0 表示成功，非 0 表示失败
} VarMapResult;

/**
 * 对规范化后的 FLYUX 源码进行变量名映射。
 * 只映射“标识符 token”，并跳过关键字、类型名、布尔/特殊字面量等。
 *
 * @param normalized_source 来自 flyux_normalize(...) 的 normalized 字符串
 * @return 映射结果，需要调用 varmap_result_free 释放
 */
VarMapResult flyux_varmap_process(const char* normalized_source);

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
