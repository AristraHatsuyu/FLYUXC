#ifndef FLYUXC_CODEGEN_H
#define FLYUXC_CODEGEN_H

#include "flyuxc/frontend/ast.h"
#include <stdio.h>

/* 代码生成器结构 */
typedef struct CodeGen {
    FILE *output;           /* 最终输出文件 */
    FILE *globals_buf;      /* 全局声明缓冲区 */
    FILE *code_buf;         /* 代码缓冲区 */
    int temp_count;         /* 临时变量计数器 */
    int label_count;        /* 标签计数器 */
    int string_count;       /* 字符串常量计数器 */
} CodeGen;

/* 创建代码生成器 */
CodeGen *codegen_create(FILE *output);

/* 生成LLVM IR */
void codegen_generate(CodeGen *gen, ASTNode *ast);

/* 释放代码生成器 */
void codegen_free(CodeGen *gen);

#endif /* FLYUXC_CODEGEN_H */
