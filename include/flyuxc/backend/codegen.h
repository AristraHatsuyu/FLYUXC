#ifndef FLYUXC_CODEGEN_H
#define FLYUXC_CODEGEN_H

#include "flyuxc/frontend/ast.h"
#include <stdio.h>

/* 数组元数据 */
typedef struct ArrayMetadata {
    char *var_name;         /* 变量名 */
    char *array_ptr;        /* 数组指针 (LLVM 临时变量) */
    size_t elem_count;      /* 元素数量 */
    struct ArrayMetadata *next;
} ArrayMetadata;

/* 对象属性元数据 */
typedef struct ObjectField {
    char *field_name;       /* 属性名 */
    char *field_ptr;        /* 属性的LLVM指针 */
    struct ObjectField *next;
} ObjectField;

/* 对象元数据 */
typedef struct ObjectMetadata {
    char *var_name;         /* 变量名 */
    ObjectField *fields;    /* 属性链表 */
    struct ObjectMetadata *next;
} ObjectMetadata;

/* 符号表条目 - 记录已定义的变量 */
typedef struct SymbolEntry {
    char *name;             /* 变量名 */
    struct SymbolEntry *next;
} SymbolEntry;

/* 代码生成器结构 */
typedef struct CodeGen {
    FILE *output;           /* 最终输出文件 */
    FILE *globals_buf;      /* 全局声明缓冲区 */
    FILE *code_buf;         /* 代码缓冲区 */
    int temp_count;         /* 临时变量计数器 */
    int label_count;        /* 标签计数器 */
    int string_count;       /* 字符串常量计数器 */
    ArrayMetadata *arrays;  /* 数组元数据链表 */
    ObjectMetadata *objects; /* 对象元数据链表 */
    SymbolEntry *symbols;   /* 符号表 - 已定义的变量 */
    const char *current_var_name;  /* 当前正在赋值的变量名（用于数组/对象跟踪） */
} CodeGen;

/* 创建代码生成器 */
CodeGen *codegen_create(FILE *output);

/* 生成LLVM IR */
void codegen_generate(CodeGen *gen, ASTNode *ast);

/* 释放代码生成器 */
void codegen_free(CodeGen *gen);

#endif /* FLYUXC_CODEGEN_H */
