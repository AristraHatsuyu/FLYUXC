#ifndef FLYUXC_CODEGEN_INTERNAL_H
#define FLYUXC_CODEGEN_INTERNAL_H

#include "flyuxc/backend/codegen.h"

/* ============================================================================
 * 错误处理函数 - codegen.c
 * ============================================================================ */

/* 设置编译错误 */
void codegen_set_error(CodeGen *gen, const char *message);

/* 设置带位置信息的编译错误 */
void codegen_set_error_at(CodeGen *gen, int line, int column, int length,
                          const char *var_name, const char *message);

/* 从映射表中查找映射后名字对应的原始名字 */
const char *codegen_lookup_original_name(CodeGen *gen, const char *mapped_name);

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

/* 注册全局变量到全局符号表 */
void register_global(CodeGen *gen, const char *var_name);

/* 检查变量是否已定义 */
int is_symbol_defined(CodeGen *gen, const char *var_name);

/* 检查变量是否是全局变量 */
int is_global_var(CodeGen *gen, const char *var_name);

/* ============================================================================
 * 作用域跟踪函数声明 - codegen_utils.c (P2: 作用域退出清理)
 * ============================================================================ */

/* 创建作用域跟踪器 */
ScopeTracker *scope_tracker_create(void);

/* 销毁作用域跟踪器 */
void scope_tracker_free(ScopeTracker *scope);

/* 添加局部变量到作用域跟踪器 */
void scope_add_local(ScopeTracker *scope, const char *var_name);

/* 生成作用域退出清理代码 - 释放所有局部变量 */
void scope_generate_cleanup(CodeGen *gen, ScopeTracker *scope);

/* 生成作用域退出清理代码，但排除指定变量（用于返回值保护） */
void scope_generate_cleanup_except(CodeGen *gen, ScopeTracker *scope, const char *except_var);

/* ============================================================================
 * 循环作用域函数声明 - codegen_utils.c (Break/Next 清理)
 * ============================================================================ */

/* 进入循环 - 压入新的循环作用域 */
void loop_scope_push(CodeGen *gen, const char *loop_end_label, const char *loop_continue_label, const char *label);

/* 退出循环 - 弹出循环作用域 */
void loop_scope_pop(CodeGen *gen);

/* 添加变量到当前循环作用域 */
void loop_scope_add_var(CodeGen *gen, const char *var_name);

/* 为 break 生成循环作用域清理代码 */
void loop_scope_generate_break_cleanup(CodeGen *gen);

/* 为 next (continue) 生成循环作用域清理代码 */
void loop_scope_generate_next_cleanup(CodeGen *gen);

/* 按标签查找目标循环，返回目标循环的 break 标签 */
const char *loop_scope_find_break_label(CodeGen *gen, const char *target_label);

/* 按标签查找目标循环，返回目标循环的 continue 标签 */
const char *loop_scope_find_continue_label(CodeGen *gen, const char *target_label);

/* 生成跨层 break 清理代码（清理从当前到目标循环之间的所有循环作用域） */
void loop_scope_generate_multilevel_break_cleanup(CodeGen *gen, const char *target_label);

/* 生成跨层 next 清理代码（清理从当前到目标循环之间的所有循环作用域） */
void loop_scope_generate_multilevel_next_cleanup(CodeGen *gen, const char *target_label);

/* ============================================================================
 * 中间值管理函数声明 - codegen_utils.c (表达式求值期间临时值跟踪)
 * ============================================================================ */

/* 创建中间值栈 */
TempValueStack *temp_value_stack_create(void);

/* 释放中间值栈（仅释放栈本身，不释放条目） */
void temp_value_stack_free(TempValueStack *stack);

/* 注册一个中间值到栈中 */
void temp_value_register(CodeGen *gen, const char *temp_name);

/* 生成中间值清理代码，释放除最终结果外的所有中间值 */
void temp_value_release_except(CodeGen *gen, const char *keep_name);

/* 清空中间值栈（不生成清理代码） */
void temp_value_clear(CodeGen *gen);

/* ============================================================================
 * 表达式代码生成声明 - codegen_expr.c
 * ============================================================================ */

/* 生成表达式的LLVM IR代码，返回结果值的临时变量名 */
char *codegen_expr(CodeGen *gen, ASTNode *node);

/* ============================================================================
 * 语句代码生成声明 - codegen_stmt.c
 * ============================================================================ */

/* 生成语句的LLVM IR代码 */
void codegen_stmt(CodeGen *gen, ASTNode *node);

/* 收集需要在entry块alloca的catch参数 */
void collect_catch_params(CodeGen *gen, ASTNode *node, FILE *entry_buf);

#endif /* FLYUXC_CODEGEN_INTERNAL_H */
