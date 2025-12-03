#ifndef FLYUXC_VALUE_H
#define FLYUXC_VALUE_H

#include <stdint.h>
#include <stdbool.h>

/* 值类型枚举 */
typedef enum {
    VALUE_NUMBER = 0,   /* 数字 (double) */
    VALUE_STRING = 1,   /* 字符串 (char*) */
    VALUE_ARRAY = 2,    /* 数组 */
    VALUE_OBJECT = 3,   /* 对象 */
    VALUE_BOOL = 4,     /* 布尔 */
    VALUE_NULL = 5,     /* null */
    VALUE_UNDEFINED = 6, /* undefined */
    VALUE_FUNCTION = 7  /* 函数/闭包 */
} ValueType;

/* 函数对象结构体 - 存储函数指针和闭包捕获的变量 */
typedef struct FunctionObject {
    void *func_ptr;           /* 函数指针 */
    struct Value **captured;  /* 捕获的变量数组 */
    int captured_count;       /* 捕获变量数量 */
    int param_count;          /* 函数参数数量 */
} FunctionObject;

/* 运行时值结构 (用于 C runtime 辅助函数) */
typedef struct {
    ValueType type;
    union {
        double number;
        char *string;
        void *pointer;  /* 用于数组、对象等复杂类型 */
    } data;
} Value;

/* 
 * LLVM IR 中的值表示：
 * - 所有值都用 %struct.Value* 表示（8字节指针）
 * - Value 结构体包含：
 *   - i32 type (4字节)
 *   - [12字节 union]: double 或 i8* 指针
 * - 总共 16 字节对齐
 */

#endif /* FLYUXC_VALUE_H */
