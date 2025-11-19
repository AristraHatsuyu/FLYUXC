#ifndef FLYUXC_CODEGEN_INTERNAL_H
#define FLYUXC_CODEGEN_INTERNAL_H

#include "flyuxc/backend/codegen.h"

/* ============================================================================
 * 内部工具函数声明 - codegen_utils.c
 * ============================================================================ */

/* 生成新的临时变量 */
char *new_temp(CodeGen *gen);

/* 生成新的标签 */
char *new_label(CodeGen *gen);

/* 生成新的字符串标签 */
char *new_string_label(CodeGen *gen);

/* 转义字符串用于LLVM IR输出 - 支持包含\0的字符串 */
char *escape_for_ir(const char *str, size_t in_len, size_t* out_len);

/* 注册数组元数据 */
void register_array(CodeGen *gen, const char *var_name, const char *array_ptr, size_t elem_count);

/* 查找数组元数据 */
ArrayMetadata *find_array(CodeGen *gen, const char *var_name);

/* 注册对象元数据 */
void register_object(CodeGen *gen, const char *var_name, ObjectField *fields);

/* 查找对象元数据 */
ObjectMetadata *find_object(CodeGen *gen, const char *var_name);

/* 在对象中查找字段 */
ObjectField *find_field(ObjectMetadata *obj_meta, const char *field_name);

/* 注册变量到符号表 */
void register_symbol(CodeGen *gen, const char *var_name);

/* 检查变量是否已定义 */
int is_symbol_defined(CodeGen *gen, const char *var_name);

/* ============================================================================
 * 表达式代码生成声明 - codegen_expr.c
 * ============================================================================ */

/* 生成表达式的LLVM IR代码，返回结果值的临时变量名 */
char *codegen_expr(CodeGen *gen, ASTNode *node);

/* ============================================================================
 * 内置函数处理声明 - codegen_builtin.c
 * ============================================================================ */

/* 处理内置函数调用，返回结果值的临时变量名（或NULL表示void）*/
char *codegen_builtin_call(CodeGen *gen, const char *func_name, ASTNode **args, size_t arg_count);

/* ============================================================================
 * 语句代码生成声明 - codegen_stmt.c
 * ============================================================================ */

/* 生成语句的LLVM IR代码 */
void codegen_stmt(CodeGen *gen, ASTNode *node);

/* 收集需要在entry块alloca的catch参数 */
void collect_catch_params(CodeGen *gen, ASTNode *node, FILE *entry_buf);

#endif /* FLYUXC_CODEGEN_INTERNAL_H */
