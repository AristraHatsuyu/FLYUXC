#include "codegen_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 内联数字运算辅助函数
 * 生成直接的 LLVM 指令而不是调用运行时函数
 * ============================================================================ */

/* 生成内联的数字减法: unbox -> fsub -> box */
static char* gen_inline_num_subtract(CodeGen *gen, const char *left, const char *right) {
    char *left_num = new_temp(gen);
    char *right_num = new_temp(gen);
    char *sub_result = new_temp(gen);
    char *result = new_temp(gen);
    
    // unbox 两个操作数
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", left_num, left);
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", right_num, right);
    // 直接用 LLVM fsub 指令
    fprintf(gen->code_buf, "  %s = fsub double %s, %s\n", sub_result, left_num, right_num);
    // box 结果
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %s)\n", result, sub_result);
    
    free(left_num);
    free(right_num);
    free(sub_result);
    
    return result;
}

/* 生成内联的数字加法: unbox -> fadd -> box */
static char* gen_inline_num_add(CodeGen *gen, const char *left, const char *right) {
    char *left_num = new_temp(gen);
    char *right_num = new_temp(gen);
    char *add_result = new_temp(gen);
    char *result = new_temp(gen);
    
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", left_num, left);
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", right_num, right);
    fprintf(gen->code_buf, "  %s = fadd double %s, %s\n", add_result, left_num, right_num);
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %s)\n", result, add_result);
    
    free(left_num);
    free(right_num);
    free(add_result);
    
    return result;
}

/* 生成内联的数字乘法: unbox -> fmul -> box */
static char* gen_inline_num_multiply(CodeGen *gen, const char *left, const char *right) {
    char *left_num = new_temp(gen);
    char *right_num = new_temp(gen);
    char *mul_result = new_temp(gen);
    char *result = new_temp(gen);
    
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", left_num, left);
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", right_num, right);
    fprintf(gen->code_buf, "  %s = fmul double %s, %s\n", mul_result, left_num, right_num);
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %s)\n", result, mul_result);
    
    free(left_num);
    free(right_num);
    free(mul_result);
    
    return result;
}

/* 生成内联的数字除法: unbox -> fdiv -> box */
static char* gen_inline_num_divide(CodeGen *gen, const char *left, const char *right) {
    char *left_num = new_temp(gen);
    char *right_num = new_temp(gen);
    char *div_result = new_temp(gen);
    char *result = new_temp(gen);
    
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", left_num, left);
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", right_num, right);
    fprintf(gen->code_buf, "  %s = fdiv double %s, %s\n", div_result, left_num, right_num);
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %s)\n", result, div_result);
    
    free(left_num);
    free(right_num);
    free(div_result);
    
    return result;
}

/* 生成内联的数字比较 (小于): unbox -> fcmp olt -> box_bool */
static char* gen_inline_num_less_than(CodeGen *gen, const char *left, const char *right) {
    char *left_num = new_temp(gen);
    char *right_num = new_temp(gen);
    char *cmp_result = new_temp(gen);
    char *cmp_i32 = new_temp(gen);
    char *result = new_temp(gen);
    
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", left_num, left);
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", right_num, right);
    fprintf(gen->code_buf, "  %s = fcmp olt double %s, %s\n", cmp_result, left_num, right_num);
    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", cmp_i32, cmp_result);
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, cmp_i32);
    
    free(left_num);
    free(right_num);
    free(cmp_result);
    free(cmp_i32);
    
    return result;
}

/* 生成内联的数字比较 (大于): unbox -> fcmp ogt -> box_bool */
static char* gen_inline_num_greater_than(CodeGen *gen, const char *left, const char *right) {
    char *left_num = new_temp(gen);
    char *right_num = new_temp(gen);
    char *cmp_result = new_temp(gen);
    char *cmp_i32 = new_temp(gen);
    char *result = new_temp(gen);
    
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", left_num, left);
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", right_num, right);
    fprintf(gen->code_buf, "  %s = fcmp ogt double %s, %s\n", cmp_result, left_num, right_num);
    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", cmp_i32, cmp_result);
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, cmp_i32);
    
    free(left_num);
    free(right_num);
    free(cmp_result);
    free(cmp_i32);
    
    return result;
}

/* 生成内联的数字比较 (小于等于): unbox -> fcmp ole -> box_bool */
static char* gen_inline_num_less_equal(CodeGen *gen, const char *left, const char *right) {
    char *left_num = new_temp(gen);
    char *right_num = new_temp(gen);
    char *cmp_result = new_temp(gen);
    char *cmp_i32 = new_temp(gen);
    char *result = new_temp(gen);
    
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", left_num, left);
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", right_num, right);
    fprintf(gen->code_buf, "  %s = fcmp ole double %s, %s\n", cmp_result, left_num, right_num);
    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", cmp_i32, cmp_result);
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, cmp_i32);
    
    free(left_num);
    free(right_num);
    free(cmp_result);
    free(cmp_i32);
    
    return result;
}

/* 生成内联的数字比较 (大于等于): unbox -> fcmp oge -> box_bool */
static char* gen_inline_num_greater_equal(CodeGen *gen, const char *left, const char *right) {
    char *left_num = new_temp(gen);
    char *right_num = new_temp(gen);
    char *cmp_result = new_temp(gen);
    char *cmp_i32 = new_temp(gen);
    char *result = new_temp(gen);
    
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", left_num, left);
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", right_num, right);
    fprintf(gen->code_buf, "  %s = fcmp oge double %s, %s\n", cmp_result, left_num, right_num);
    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", cmp_i32, cmp_result);
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, cmp_i32);
    
    free(left_num);
    free(right_num);
    free(cmp_result);
    free(cmp_i32);
    
    return result;
}

/* 生成带类型检查快速路径的加法
 * 如果两个操作数都是数字 (type == 1)，则使用 LLVM fadd
 * 否则回退到 value_add 支持字符串拼接
 */
static char* gen_inline_add_with_type_check(CodeGen *gen, const char *left, const char *right) {
    // 检查左操作数类型
    char *left_type = new_temp(gen);
    char *right_type = new_temp(gen);
    char *is_left_num = new_temp(gen);
    char *is_right_num = new_temp(gen);
    char *both_num = new_temp(gen);
    
    // 读取类型字段 (Value 结构体第一个字段是 type，i32)
    fprintf(gen->code_buf, "  %s = getelementptr inbounds %%struct.Value, %%struct.Value* %s, i32 0, i32 0\n", left_type, left);
    char *left_type_val = new_temp(gen);
    fprintf(gen->code_buf, "  %s = load i32, i32* %s\n", left_type_val, left_type);
    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_left_num, left_type_val);  // VALUE_NUMBER = 0
    
    fprintf(gen->code_buf, "  %s = getelementptr inbounds %%struct.Value, %%struct.Value* %s, i32 0, i32 0\n", right_type, right);
    char *right_type_val = new_temp(gen);
    fprintf(gen->code_buf, "  %s = load i32, i32* %s\n", right_type_val, right_type);
    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_right_num, right_type_val);  // VALUE_NUMBER = 0
    
    fprintf(gen->code_buf, "  %s = and i1 %s, %s\n", both_num, is_left_num, is_right_num);
    
    // 创建分支
    char *fast_label = new_label(gen);
    char *slow_label = new_label(gen);
    char *merge_label = new_label(gen);
    
    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", both_num, fast_label, slow_label);
    
    // 快速路径：两个都是数字
    fprintf(gen->code_buf, "%s:\n", fast_label);
    char *fast_left_num = new_temp(gen);
    char *fast_right_num = new_temp(gen);
    char *fast_add = new_temp(gen);
    char *fast_result = new_temp(gen);
    
    // 直接从 data.number 读取（第5个字段，偏移 = 4*i32 = 16 bytes，但我们用 unbox_number 更安全）
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", fast_left_num, left);
    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", fast_right_num, right);
    fprintf(gen->code_buf, "  %s = fadd double %s, %s\n", fast_add, fast_left_num, fast_right_num);
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %s)\n", fast_result, fast_add);
    fprintf(gen->code_buf, "  br label %%%s\n", merge_label);
    
    // 慢速路径：调用 value_add
    fprintf(gen->code_buf, "%s:\n", slow_label);
    char *slow_result = new_temp(gen);
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_add(%%struct.Value* %s, %%struct.Value* %s)\n",
            slow_result, left, right);
    fprintf(gen->code_buf, "  br label %%%s\n", merge_label);
    
    // 合并
    fprintf(gen->code_buf, "%s:\n", merge_label);
    char *result = new_temp(gen);
    fprintf(gen->code_buf, "  %s = phi %%struct.Value* [ %s, %%%s ], [ %s, %%%s ]\n",
            result, fast_result, fast_label, slow_result, slow_label);
    
    // 清理
    free(left_type);
    free(right_type);
    free(left_type_val);
    free(right_type_val);
    free(is_left_num);
    free(is_right_num);
    free(both_num);
    free(fast_label);
    free(slow_label);
    free(merge_label);
    free(fast_left_num);
    free(fast_right_num);
    free(fast_add);
    free(fast_result);
    free(slow_result);
    
    return result;
}

/* ============================================================================
 * 表达式代码生成
 * ============================================================================ */

char *codegen_expr(CodeGen *gen, ASTNode *node) {
    if (!node) return NULL;
    
    switch (node->kind) {
        case AST_NUM_LITERAL: {
            ASTNumLiteral *num = (ASTNumLiteral *)node->data;
            char *temp = new_temp(gen);
            
            // 使用 box_number 将数字装箱为 Value*
            // 使用 %.17e 科学计数法保持完整精度（17位有效数字）
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %.17e)\n", 
                    temp, num->value);
            temp_value_register(gen, temp);  // 注册为中间值
            
            return temp;
        }
        
        case AST_BOOL_LITERAL: {
            ASTBoolLiteral *bl = (ASTBoolLiteral *)node->data;
            char *temp = new_temp(gen);
            
            // 使用 box_bool 将布尔值装箱为 Value*
            // LLVM i1/i32: false=0, true=1
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %d)\n", 
                    temp, bl->value ? 1 : 0);
            temp_value_register(gen, temp);  // 注册为中间值
            
            return temp;
        }
        
        case AST_STRING_LITERAL: {
            ASTStringLiteral *str = (ASTStringLiteral *)node->data;
            char *str_label = new_string_label(gen);
            char *temp = new_temp(gen);
            char *str_ptr = new_temp(gen);
            
            size_t len = str->length;  /* 使用AST中的实际长度，支持\0字符串 */
            size_t escaped_len = 0;
            char *escaped = escape_for_ir(str->value, len, &escaped_len);
            
            // 声明全局字符串常量
            // IR中\0A等转义符会被解析为单字节，所以数组大小应该是原始长度+1
            fprintf(gen->strings_buf, "%s = private unnamed_addr constant [%zu x i8] c\"%s\\00\"\n",
                    str_label, len + 1, escaped);
            
            // 获取指针
            fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x i8], [%zu x i8]* %s, i32 0, i32 0\n",
                    str_ptr, len + 1, len + 1, str_label);
            
            // 使用 box_string_with_length 装箱，传递显式长度支持\0字符串
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_string_with_length(i8* %s, i64 %zu)\n",
                    temp, str_ptr, len);
            temp_value_register(gen, temp);  // 注册为中间值
            
            free(escaped);
            free(str_label);
            free(str_ptr);
            return temp;
        }
        
        case AST_NULL_LITERAL: {
            char *temp = new_temp(gen);
            
            // 调用 box_null() 创建 null 值
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", temp);
            temp_value_register(gen, temp);  // 注册为中间值
            
            return temp;
        }
        
        case AST_UNDEF_LITERAL: {
            char *temp = new_temp(gen);
            
            // undef 使用 box_undef()
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()\n", temp);
            temp_value_register(gen, temp);  // 注册为中间值
            
            return temp;
        }
        
        case AST_IDENTIFIER: {
            ASTIdentifier *id = (ASTIdentifier *)node->data;
            char *temp = new_temp(gen);
            
            // 首先检查是否是函数名
            if (is_function_name(gen, id->name)) {
                // 这是一个函数引用，创建函数值
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_function(i8* bitcast (%%struct.Value* (", temp);
                // 获取函数参数数量以正确生成类型签名
                // 简单处理：假设所有函数都返回 Value* 并接受 Value* 参数
                // 这里我们需要知道函数的参数数量，但暂时用可变参数处理
                fprintf(gen->code_buf, "...)* @%s to i8*), %%struct.Value** null, i32 0, i32 0, i32 0)\n", id->name);  // needs_self=0 for function refs
                // 注册为中间值
                temp_value_register(gen, temp);
                return temp;
            }
            
            // 检查变量是否已定义
            if (!is_symbol_defined(gen, id->name)) {
                // 未定义的变量 - 报错
                // 尝试查找原始名字
                const char *original_name = codegen_lookup_original_name(gen, id->name);
                const char *display_name = original_name ? original_name : id->name;
                
                // 使用显示名称的长度作为错误区域长度
                int name_length = (int)strlen(display_name);
                
                // 使用节点的位置信息
                codegen_set_error_at(gen, node->loc.orig_line, node->loc.orig_column,
                                     name_length,
                                     display_name, "Undefined variable");
                
                // 返回一个 undef 值以继续生成（但会因为 has_error 而失败）
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()  ; ERROR: undefined variable '%s'\n", 
                        temp, id->name);
                temp_value_register(gen, temp);
                return temp;
            }
            
            // 检查是否是全局变量
            // 注意：如果当前在闭包中且该变量被捕获，则使用局部变量而非全局变量
            int use_global = is_global_var(gen, id->name);
            if (use_global && gen->current_captured && 
                captured_vars_contains(gen->current_captured, id->name)) {
                // 在闭包中，被捕获的全局变量应该使用局部变量访问
                use_global = 0;
            }
            
            // 获取变量的 IR 名称（考虑遮蔽）
            const char *ir_name = get_symbol_ir_name(gen, id->name);
            
            if (use_global) {
                // 全局变量访问 - 使用 @var_name
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** @%s\n", 
                        temp, ir_name);
            } else {
                // 局部变量访问 - 使用 %var_name
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n", 
                        temp, ir_name);
            }
            
            // P2 Fix: 对于从变量加载的值，需要 retain，使调用者拥有一个引用
            // 这样表达式结果遵循统一的"调用者拥有"规则
            fprintf(gen->code_buf, "  call %%struct.Value* @value_retain(%%struct.Value* %s)\n", temp);
            
            // 注册为中间值，确保在表达式结束后被释放
            temp_value_register(gen, temp);
            
            return temp;
        }
        
        case AST_SELF_EXPR: {
            // self 关键字 - 加载名为 "self" 的局部变量
            char *temp = new_temp(gen);
            
            // 检查 self 变量是否已定义
            if (!is_symbol_defined(gen, "self")) {
                // 'self' 只能在对象方法内部使用
                codegen_set_error_at(gen, node->loc.orig_line, node->loc.orig_column,
                                     4,  // "self" 的长度
                                     "self", "'self' can only be used inside object methods");
                
                // 返回 undef 以继续生成
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()  ; ERROR: self outside method\n", temp);
                temp_value_register(gen, temp);
                return temp;
            }
            
            // 加载 self 变量（总是局部变量）
            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%self\n", temp);
            return temp;
        }
        
        case AST_BINARY_EXPR: {
            ASTBinaryExpr *expr = (ASTBinaryExpr *)node->data;
            char *left = codegen_expr(gen, expr->left);
            char *right = codegen_expr(gen, expr->right);
            char *result = NULL;
            
            switch (expr->op) {
                case TK_PLUS:
                    // 带类型检查的快速路径：数字直接用 fadd，字符串回退到 value_add
                    result = gen_inline_add_with_type_check(gen, left, right);
                    temp_value_register(gen, result);  // 注册结果为中间值
                    break;
                case TK_MINUS:
                    // 内联数字减法
                    result = gen_inline_num_subtract(gen, left, right);
                    temp_value_register(gen, result);
                    break;
                case TK_STAR:
                    // 内联数字乘法
                    result = gen_inline_num_multiply(gen, left, right);
                    temp_value_register(gen, result);
                    break;
                case TK_SLASH:
                    // 内联数字除法
                    result = gen_inline_num_divide(gen, left, right);
                    temp_value_register(gen, result);
                    break;
                case TK_POWER:
                    result = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_power(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    temp_value_register(gen, result);
                    break;
                case TK_PERCENT:
                    result = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_modulo(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    temp_value_register(gen, result);
                    break;
                case TK_LT:
                    // 内联小于比较
                    result = gen_inline_num_less_than(gen, left, right);
                    temp_value_register(gen, result);
                    break;
                case TK_GT:
                    // 内联大于比较
                    result = gen_inline_num_greater_than(gen, left, right);
                    temp_value_register(gen, result);
                    break;
                case TK_LE:
                    // 内联小于等于比较
                    result = gen_inline_num_less_equal(gen, left, right);
                    temp_value_register(gen, result);
                    break;
                case TK_GE:
                    // 内联大于等于比较
                    result = gen_inline_num_greater_equal(gen, left, right);
                    temp_value_register(gen, result);
                    break;
                case TK_EQ_EQ:
                    result = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_equals(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    temp_value_register(gen, result);
                    break;
                case TK_BANG_EQ: {
                    // != 使用 !value_equals
                    result = new_temp(gen);
                    char *eq = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_equals(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            eq, left, right);
                    char *truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", truthy, eq);
                    // 释放中间 Value* eq
                    fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", eq);
                    char *inverted = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", inverted, truthy);
                    char *as_i32 = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", as_i32, inverted);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, as_i32);
                    temp_value_register(gen, result);
                    free(eq);
                    free(truthy);
                    free(inverted);
                    free(as_i32);
                    break;
                }
                case TK_AND_AND: {
                    // && : 逻辑与
                    result = new_temp(gen);  // 分配结果变量
                    char *left_truthy = new_temp(gen);
                    char *right_truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                            left_truthy, left);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                            right_truthy, right);
                    char *left_bool = new_temp(gen);
                    char *right_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", left_bool, left_truthy);
                    fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", right_bool, right_truthy);
                    char *and_result = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = and i1 %s, %s\n", and_result, left_bool, right_bool);
                    char *as_i32 = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", as_i32, and_result);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, as_i32);
                    temp_value_register(gen, result);
                    free(left_truthy);
                    free(right_truthy);
                    free(left_bool);
                    free(right_bool);
                    free(and_result);
                    free(as_i32);
                    break;
                }
                case TK_OR_OR: {
                    // || : 逻辑或
                    result = new_temp(gen);  // 分配结果变量
                    char *left_truthy = new_temp(gen);
                    char *right_truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                            left_truthy, left);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                            right_truthy, right);
                    char *left_bool = new_temp(gen);
                    char *right_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", left_bool, left_truthy);
                    fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", right_bool, right_truthy);
                    char *or_result = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = or i1 %s, %s\n", or_result, left_bool, right_bool);
                    char *as_i32 = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", as_i32, or_result);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, as_i32);
                    temp_value_register(gen, result);
                    free(left_truthy);
                    free(right_truthy);
                    free(left_bool);
                    free(right_bool);
                    free(or_result);
                    free(as_i32);
                    break;
                }
                case TK_NULLISH_COALESCE: {
                    // ?? : 空值合并 - 如果左边是 null 或 undef，返回右边
                    char *is_null_val = new_temp(gen);
                    char *is_undef_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_null(%%struct.Value* %s)\n", 
                            is_null_val, left);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_undef(%%struct.Value* %s)\n", 
                            is_undef_val, left);
                    char *is_null_truthy = new_temp(gen);
                    char *is_undef_truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                            is_null_truthy, is_null_val);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                            is_undef_truthy, is_undef_val);
                    // 释放临时布尔值
                    fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", is_null_val);
                    fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", is_undef_val);
                    char *is_null_bool = new_temp(gen);
                    char *is_undef_bool = new_temp(gen);
                    char *is_nullish = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_null_bool, is_null_truthy);
                    fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_undef_bool, is_undef_truthy);
                    fprintf(gen->code_buf, "  %s = or i1 %s, %s\n", is_nullish, is_null_bool, is_undef_bool);
                    // 使用 select 指令：如果 nullish 则返回 right，否则返回 left
                    fprintf(gen->code_buf, "  %s = select i1 %s, %%struct.Value* %s, %%struct.Value* %s\n", 
                            result, is_nullish, right, left);
                    // 需要 retain 选中的那个值，release 另一个
                    fprintf(gen->code_buf, "  call void @value_retain(%%struct.Value* %s)\n", result);
                    temp_value_register(gen, result);
                    free(is_null_val);
                    free(is_undef_val);
                    free(is_null_truthy);
                    free(is_undef_truthy);
                    free(is_null_bool);
                    free(is_undef_bool);
                    free(is_nullish);
                    break;
                }
                default:
                    break;
            }
            
            free(left);
            free(right);
            return result;
        }
        
        case AST_TERNARY_EXPR: {
            // 三元运算符: condition ? true_value : false_value
            // 使用 alloca + store/load 模式避免 phi 节点的复杂性
            ASTTernaryExpr *ternary = (ASTTernaryExpr *)node->data;
            
            // 分配结果存储
            char *result_ptr = new_temp(gen);
            fprintf(gen->code_buf, "  %s = alloca %%struct.Value*, align 8\n", result_ptr);
            
            // 保存临时值栈（这样条件表达式和分支表达式的中间值都会在隔离环境中）
            TempValueStack *saved_stack = gen->temp_values;
            gen->temp_values = NULL;
            
            // 生成条件表达式（在隔离的栈环境中）
            char *cond_val = codegen_expr(gen, ternary->condition);
            
            // 检查条件是否为真
            char *truthy = new_temp(gen);
            fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", truthy, cond_val);
            
            // 条件值在分支前释放（在主块中，可以被dominate）
            fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", cond_val);
            
            // 清理条件表达式产生的临时值注册（已手动释放）
            temp_value_clear(gen);
            
            char *cond_bool = new_temp(gen);
            fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", cond_bool, truthy);
            
            // 生成标签
            char *then_label = new_label(gen);
            char *else_label = new_label(gen);
            char *end_label = new_label(gen);
            
            // 条件跳转
            fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", cond_bool, then_label, else_label);
            
            // then 分支
            fprintf(gen->code_buf, "%s:\n", then_label);
            char *true_val = codegen_expr(gen, ternary->true_value);
            fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s, align 8\n", true_val, result_ptr);
            // 清除分支内产生的临时值注册（不生成release，因为值已存储）
            temp_value_clear(gen);
            fprintf(gen->code_buf, "  br label %%%s\n", end_label);
            
            // else 分支
            fprintf(gen->code_buf, "%s:\n", else_label);
            char *false_val = codegen_expr(gen, ternary->false_value);
            fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s, align 8\n", false_val, result_ptr);
            // 清除分支内产生的临时值注册
            temp_value_clear(gen);
            fprintf(gen->code_buf, "  br label %%%s\n", end_label);
            
            // 恢复临时值栈
            gen->temp_values = saved_stack;
            
            // 合并点 - 加载结果
            fprintf(gen->code_buf, "%s:\n", end_label);
            char *result = new_temp(gen);
            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s, align 8\n", result, result_ptr);
            
            // 注册最终结果
            temp_value_register(gen, result);
            
            free(cond_val);
            free(truthy);
            free(cond_bool);
            free(then_label);
            free(else_label);
            free(end_label);
            free(result_ptr);
            free(true_val);
            free(false_val);
            
            return result;
        }
        
        case AST_CALL_EXPR: {
            ASTCallExpr *call = (ASTCallExpr *)node->data;
            
            // 如果 callee 不是标识符（如 obj.method, arr[i] 等），需要间接调用
            if (!call->callee || call->callee->kind != AST_IDENTIFIER) {
                // 计算 callee 表达式得到函数值
                char *func_val = codegen_expr(gen, call->callee);
                if (!func_val) {
                    fprintf(stderr, "Error: failed to evaluate callee expression\n");
                    return NULL;
                }
                
                // 计算参数
                char **arg_regs = NULL;
                if (call->arg_count > 0) {
                    arg_regs = (char **)malloc(call->arg_count * sizeof(char *));
                    for (size_t i = 0; i < call->arg_count; i++) {
                        arg_regs[i] = codegen_expr(gen, call->args[i]);
                    }
                }
                
                // 创建参数数组
                char *args_array = new_temp(gen);
                char *args_ptr = new_temp(gen);
                if (call->arg_count > 0) {
                    fprintf(gen->code_buf, "  %s = alloca [%zu x %%struct.Value*]\n", args_array, call->arg_count);
                    for (size_t i = 0; i < call->arg_count; i++) {
                        char *slot = new_temp(gen);
                        fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 %zu\n",
                                slot, call->arg_count, call->arg_count, args_array, i);
                        fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", arg_regs[i], slot);
                        free(slot);
                    }
                    fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 0\n",
                            args_ptr, call->arg_count, call->arg_count, args_array);
                } else {
                    fprintf(gen->code_buf, "  %s = inttoptr i64 0 to [0 x %%struct.Value*]*\n", args_array);
                    fprintf(gen->code_buf, "  %s = inttoptr i64 0 to %%struct.Value**\n", args_ptr);
                }
                
                // 检查是否是函数类型
                char *is_func_check = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_function(%%struct.Value* %s)\n", is_func_check, func_val);
                
                // 分支：是函数则调用，否则报错
                char *call_label = new_label(gen);
                char *error_label = new_label(gen);
                char *merge_label = new_label(gen);
                
                char *is_func_bool = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_func_bool, is_func_check);
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_func_bool, call_label, error_label);
                
                // 函数调用分支
                fprintf(gen->code_buf, "%s:\n", call_label);
                char *call_result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_call_function(%%struct.Value* %s, %%struct.Value** %s, i64 %zu)\n",
                        call_result, func_val, args_ptr, call->arg_count);
                fprintf(gen->code_buf, "  br label %%%s\n", merge_label);
                
                // 错误分支
                fprintf(gen->code_buf, "%s:\n", error_label);
                fprintf(gen->code_buf, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([30 x i8], [30 x i8]* @.str.not_callable, i64 0, i64 0))\n");
                char *error_result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()\n", error_result);
                fprintf(gen->code_buf, "  br label %%%s\n", merge_label);
                
                // 合并分支
                fprintf(gen->code_buf, "%s:\n", merge_label);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = phi %%struct.Value* [ %s, %%%s ], [ %s, %%%s ]\n",
                        result, call_result, call_label, error_result, error_label);
                
                // 清理
                free(func_val);
                free(args_array);
                free(args_ptr);
                free(is_func_check);
                free(is_func_bool);
                free(call_label);
                free(error_label);
                free(merge_label);
                free(call_result);
                free(error_result);
                if (arg_regs) {
                    for (size_t i = 0; i < call->arg_count; i++) {
                        free(arg_regs[i]);
                    }
                    free(arg_regs);
                }
                
                return result;
            }
            
            ASTIdentifier *callee = (ASTIdentifier *)call->callee->data;
            
            // 特殊处理 print 函数（不换行，逗号分隔直接拼接）
            if (strcmp(callee->name, "print") == 0) {
                // 使用 value_print 打印所有参数，参数间无换行
                for (size_t i = 0; i < call->arg_count; i++) {
                    char *arg = codegen_expr(gen, call->args[i]);
                    fprintf(gen->code_buf, "  call void @value_print(%%struct.Value* %s)\n", arg);
                    free(arg);
                }
                // 返回 true 表示成功输出
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 1)\n", result);
                return result;
            }
            
            // 特殊处理 println 函数（所有参数输出后只在最后换行一次）
            if (strcmp(callee->name, "println") == 0) {
                if (call->arg_count == 0) {
                    // println() 无参数时只输出换行
                    // 创建换行字符串常量
                    char *newline_label = new_string_label(gen);
                    fprintf(gen->strings_buf, "%s = private unnamed_addr constant [2 x i8] c\"\\0A\\00\"\n", newline_label);
                    fprintf(gen->code_buf, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* %s, i32 0, i32 0))\n", newline_label);
                    free(newline_label);
                } else {
                    // 使用 value_print 输出所有参数（不换行）
                    for (size_t i = 0; i < call->arg_count; i++) {
                        char *arg = codegen_expr(gen, call->args[i]);
                        fprintf(gen->code_buf, "  call void @value_print(%%struct.Value* %s)\n", arg);
                        free(arg);
                    }
                    // 最后输出一个换行
                    char *newline_label = new_string_label(gen);
                    fprintf(gen->strings_buf, "%s = private unnamed_addr constant [2 x i8] c\"\\0A\\00\"\n", newline_label);
                    fprintf(gen->code_buf, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* %s, i32 0, i32 0))\n", newline_label);
                    free(newline_label);
                }
                // 返回 true 表示成功输出
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 1)\n", result);
                return result;
            }
            
            // 特殊处理 printf 函数（格式化输出）
            if (strcmp(callee->name, "printf") == 0 && call->arg_count >= 1) {
                // 第一个参数是格式字符串
                char *fmt_arg = codegen_expr(gen, call->args[0]);
                
                // 创建参数数组
                if (call->arg_count > 1) {
                    // 分配数组存储参数
                    char *args_array = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = alloca [%zu x %%struct.Value*]\n", 
                            args_array, call->arg_count - 1);
                    
                    // 填充参数
                    for (size_t i = 1; i < call->arg_count; i++) {
                        char *arg = codegen_expr(gen, call->args[i]);
                        char *elem_ptr = new_temp(gen);
                        fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 %zu\n",
                                elem_ptr, call->arg_count - 1, call->arg_count - 1, args_array, i - 1);
                        fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", arg, elem_ptr);
                        free(arg);
                        free(elem_ptr);
                    }
                    
                    // 转换为 Value** 并调用 value_printf
                    char *args_ptr = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 0\n",
                            args_ptr, call->arg_count - 1, call->arg_count - 1, args_array);
                    fprintf(gen->code_buf, "  call void @value_printf(%%struct.Value* %s, %%struct.Value** %s, i64 %zu)\n",
                            fmt_arg, args_ptr, call->arg_count - 1);
                    free(args_ptr);
                    free(args_array);
                } else {
                    // 只有格式字符串，无参数
                    char *null_ptr = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = inttoptr i64 0 to %%struct.Value**\n", null_ptr);
                    fprintf(gen->code_buf, "  call void @value_printf(%%struct.Value* %s, %%struct.Value** %s, i64 0)\n",
                            fmt_arg, null_ptr);
                    free(null_ptr);
                }
                
                free(fmt_arg);
                // 返回 true 表示成功输出
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 1)\n", result);
                return result;
            }
            
            // 特殊处理 typeOf 函数
            if (strcmp(callee->name, "typeOf") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                char *type_str = new_temp(gen);
                
                // 调用 value_typeof 获取类型字符串
                fprintf(gen->code_buf, "  %s = call i8* @value_typeof(%%struct.Value* %s)\n", type_str, arg);
                // 装箱为 Value*
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_string(i8* %s)\n", result, type_str);
                
                free(arg);
                free(type_str);
                return result;
            }
            
            // ========================================
            // 类型检查函数 (Type Checking Functions)
            // ========================================
            
            // isNum(value) - 检查是否为数字
            if (strcmp(callee->name, "isNum") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_num(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            // isStr(value) - 检查是否为字符串
            if (strcmp(callee->name, "isStr") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_str(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            // isBl(value) - 检查是否为布尔值
            if (strcmp(callee->name, "isBl") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_bl(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            // isArr(value) - 检查是否为数组
            if (strcmp(callee->name, "isArr") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_arr(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            // isObj(value) - 检查是否为对象
            if (strcmp(callee->name, "isObj") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_obj(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            // isNull(value) - 检查是否为 null
            if (strcmp(callee->name, "isNull") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_null(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            // isUndef(value) - 检查是否为 undefined
            if (strcmp(callee->name, "isUndef") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_undef(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            // ========================================
            // 工具函数 (Utility Functions)
            // ========================================
            
            // range(start, end, step?) - 生成数字范围数组
            if (strcmp(callee->name, "range") == 0 && (call->arg_count >= 2 && call->arg_count <= 3)) {
                char *start = codegen_expr(gen, call->args[0]);
                char *end = codegen_expr(gen, call->args[1]);
                char *step;
                if (call->arg_count == 3) {
                    step = codegen_expr(gen, call->args[2]);
                } else {
                    step = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()\n", step);
                }
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_range(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n", result, start, end, step);
                free(start);
                free(end);
                free(step);
                return result;
            }
            
            // assert(condition, message?) - 断言
            if (strcmp(callee->name, "assert") == 0 && (call->arg_count >= 1 && call->arg_count <= 2)) {
                char *condition = codegen_expr(gen, call->args[0]);
                char *message;
                if (call->arg_count == 2) {
                    message = codegen_expr(gen, call->args[1]);
                } else {
                    message = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", message);
                }
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_assert(%%struct.Value* %s, %%struct.Value* %s)\n", result, condition, message);
                free(condition);
                free(message);
                return result;
            }
            
            // 特殊处理 input 函数
            if (strcmp(callee->name, "input") == 0) {
                char *result = new_temp(gen);
                
                if (call->arg_count == 0) {
                    // 无提示符
                    char *null_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", null_val);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_input(%%struct.Value* %s)\n", result, null_val);
                    free(null_val);
                } else {
                    // 有提示符
                    char *prompt = codegen_expr(gen, call->args[0]);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_input(%%struct.Value* %s)\n", result, prompt);
                    free(prompt);
                }
                
                return result;
            }
            
            // 特殊处理类型转换函数
            if (strcmp(callee->name, "toNum") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_num(%%struct.Value* %s)\n", result, arg);
                free(arg);
                
                // 处理错误
                if (call->throw_on_error == 0) {
                    // 无 ! 后缀：检查错误，如果有错误则返回null并清除错误状态
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    
                    char *ok_label = new_label(gen);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, ok_label);
                    
                    // OK分支：保持原结果
                    fprintf(gen->code_buf, "%s:\n", ok_label);
                    fprintf(gen->code_buf, "  br label %%%s\n", continue_label);
                    
                    // 错误分支：清除错误并返回null
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                    char *null_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", null_val);
                    fprintf(gen->code_buf, "  br label %%%s\n", continue_label);
                    
                    // 继续执行 - 使用PHI选择值
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    char *result_phi = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = phi %%struct.Value* [ %s, %%%s ], [ %s, %%%s ]\n", 
                            result_phi, result, ok_label, null_val, error_label);
                    
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(ok_label);
                    free(error_label);
                    free(continue_label);
                    
                    return result_phi;
                } else if (gen->in_try_catch) {
                    // 有 ! 后缀且在 Try-Catch 中：检查错误并跳转到catch标签
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", 
                            is_error, gen->try_catch_label, continue_label);
                    
                    // 继续执行
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(continue_label);
                } else {
                    // 有 ! 后缀且不在 Try-Catch 中：检查错误并终止
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    
                    // 错误处理：调用 value_fatal_error() 并终止
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    
                    // 继续执行
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            if (strcmp(callee->name, "toStr") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_str(%%struct.Value* %s)\n", result, arg);
                temp_value_register(gen, result);  // 注册为中间值
                free(arg);
                return result;
            }
            
            if (strcmp(callee->name, "toBl") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_bl(%%struct.Value* %s)\n", result, arg);
                temp_value_register(gen, result);  // 注册为中间值
                free(arg);
                return result;
            }
            
            if (strcmp(callee->name, "toInt") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_int(%%struct.Value* %s)\n", result, arg);
                temp_value_register(gen, result);  // 注册为中间值
                free(arg);
                
                // 如果没有 ! 后缀，清除错误状态
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                }
                
                return result;
            }
            
            if (strcmp(callee->name, "toFloat") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_float(%%struct.Value* %s)\n", result, arg);
                temp_value_register(gen, result);  // 注册为中间值
                free(arg);
                return result;
            }
            
            // 字符串处理函数
            if (strcmp(callee->name, "len") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_len(%%struct.Value* %s)\n", result, arg);
                temp_value_register(gen, result);  // 注册为中间值
                free(arg);
                
                // 错误处理
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (gen->in_try_catch) {
                    // 有 ! 后缀且在 Try-Catch 中：检查错误并跳转到catch标签
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", 
                            is_error, gen->try_catch_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(continue_label);
                } else {
                    // 有 ! 后缀且不在 Try-Catch 中：检查错误并终止
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            if (strcmp(callee->name, "charAt") == 0 && call->arg_count == 2) {
                char *str = codegen_expr(gen, call->args[0]);
                char *idx = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_char_at(%%struct.Value* %s, %%struct.Value* %s)\n", result, str, idx);
                free(str);
                free(idx);
                
                // 处理错误
                if (call->throw_on_error == 0) {
                    // 无 ! 后缀：检查错误，如果有错误则清除错误状态（返回值已是null）
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (gen->in_try_catch) {
                    // 有 ! 后缀且在 Try-Catch 中：检查错误并跳转到catch标签
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, gen->try_catch_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(continue_label);
                } else if (!gen->in_try_catch) {
                    // 有 ! 后缀但不在 Try-Catch 中：检查错误并终止程序
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            if (strcmp(callee->name, "substr") == 0 && (call->arg_count == 2 || call->arg_count == 3)) {
                char *str = codegen_expr(gen, call->args[0]);
                char *start = codegen_expr(gen, call->args[1]);
                char *len = call->arg_count == 3 ? codegen_expr(gen, call->args[2]) : NULL;
                char *result = new_temp(gen);
                
                if (len) {
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_substr(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n", result, str, start, len);
                    free(len);
                } else {
                    char *null_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", null_val);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_substr(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n", result, str, start, null_val);
                    free(null_val);
                }
                free(str);
                free(start);
                return result;
            }
            
            if (strcmp(callee->name, "indexOf") == 0 && call->arg_count == 2) {
                char *str = codegen_expr(gen, call->args[0]);
                char *substr = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_index_of(%%struct.Value* %s, %%struct.Value* %s)\n", result, str, substr);
                free(str);
                free(substr);
                return result;
            }
            
            if (strcmp(callee->name, "replace") == 0 && call->arg_count == 3) {
                char *str = codegen_expr(gen, call->args[0]);
                char *old = codegen_expr(gen, call->args[1]);
                char *new = codegen_expr(gen, call->args[2]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_replace(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n", result, str, old, new);
                free(str);
                free(old);
                free(new);
                return result;
            }
            
            if (strcmp(callee->name, "split") == 0 && (call->arg_count == 1 || call->arg_count == 2)) {
                char *str = codegen_expr(gen, call->args[0]);
                char *delim = call->arg_count == 2 ? codegen_expr(gen, call->args[1]) : NULL;
                char *result = new_temp(gen);
                
                if (delim) {
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_split(%%struct.Value* %s, %%struct.Value* %s)\n", result, str, delim);
                    free(delim);
                } else {
                    char *null_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", null_val);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_split(%%struct.Value* %s, %%struct.Value* %s)\n", result, str, null_val);
                    free(null_val);
                }
                free(str);
                return result;
            }
            
            if (strcmp(callee->name, "join") == 0 && (call->arg_count == 1 || call->arg_count == 2)) {
                char *arr = codegen_expr(gen, call->args[0]);
                char *sep = call->arg_count == 2 ? codegen_expr(gen, call->args[1]) : NULL;
                char *result = new_temp(gen);
                
                if (sep) {
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_join(%%struct.Value* %s, %%struct.Value* %s)\n", result, arr, sep);
                    free(sep);
                } else {
                    char *null_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", null_val);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_join(%%struct.Value* %s, %%struct.Value* %s)\n", result, arr, null_val);
                    free(null_val);
                }
                free(arr);
                return result;
            }
            
            if (strcmp(callee->name, "trim") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_trim(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            if (strcmp(callee->name, "upper") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_upper(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            if (strcmp(callee->name, "lower") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_lower(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            // 数组操作函数
            if (strcmp(callee->name, "push") == 0 && call->arg_count == 2) {
                char *arr = codegen_expr(gen, call->args[0]);
                char *val = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_push(%%struct.Value* %s, %%struct.Value* %s)\n", result, arr, val);
                free(arr);
                free(val);
                return result;
            }
            
            if (strcmp(callee->name, "pop") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_pop(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            if (strcmp(callee->name, "shift") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_shift(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            if (strcmp(callee->name, "unshift") == 0 && call->arg_count == 2) {
                char *arr = codegen_expr(gen, call->args[0]);
                char *val = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_unshift(%%struct.Value* %s, %%struct.Value* %s)\n", result, arr, val);
                free(arr);
                free(val);
                return result;
            }
            
            if (strcmp(callee->name, "slice") == 0 && (call->arg_count >= 1 && call->arg_count <= 3)) {
                char *arr = codegen_expr(gen, call->args[0]);
                char *start = call->arg_count >= 2 ? codegen_expr(gen, call->args[1]) : NULL;
                char *end = call->arg_count == 3 ? codegen_expr(gen, call->args[2]) : NULL;
                char *result = new_temp(gen);
                
                if (!start) {
                    start = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", start);
                }
                if (!end) {
                    end = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", end);
                }
                
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_slice(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n", result, arr, start, end);
                free(arr);
                free(start);
                free(end);
                return result;
            }
            
            if (strcmp(callee->name, "concat") == 0 && call->arg_count == 2) {
                char *arr1 = codegen_expr(gen, call->args[0]);
                char *arr2 = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_concat(%%struct.Value* %s, %%struct.Value* %s)\n", result, arr1, arr2);
                free(arr1);
                free(arr2);
                return result;
            }
            
            // 动态对象操作函数
            if (strcmp(callee->name, "setField") == 0 && call->arg_count == 3) {
                char *obj = codegen_expr(gen, call->args[0]);
                char *key = codegen_expr(gen, call->args[1]);
                char *val = codegen_expr(gen, call->args[2]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_set_field(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n", result, obj, key, val);
                free(obj);
                free(key);
                free(val);
                return result;
            }
            
            if (strcmp(callee->name, "deleteField") == 0 && call->arg_count == 2) {
                char *obj = codegen_expr(gen, call->args[0]);
                char *key = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_delete_field(%%struct.Value* %s, %%struct.Value* %s)\n", result, obj, key);
                free(obj);
                free(key);
                return result;
            }
            
            if (strcmp(callee->name, "hasField") == 0 && call->arg_count == 2) {
                char *obj = codegen_expr(gen, call->args[0]);
                char *key = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_has_field(%%struct.Value* %s, %%struct.Value* %s)\n", result, obj, key);
                free(obj);
                free(key);
                return result;
            }
            
            if (strcmp(callee->name, "keys") == 0 && call->arg_count == 1) {
                char *obj = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_keys(%%struct.Value* %s)\n", result, obj);
                free(obj);
                return result;
            }
            
            // clone - 浅拷贝
            if (strcmp(callee->name, "clone") == 0 && call->arg_count == 1) {
                char *obj = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_shallow_clone(%%struct.Value* %s)\n", result, obj);
                free(obj);
                return result;
            }
            
            // deepClone - 深拷贝
            if (strcmp(callee->name, "deepClone") == 0 && call->arg_count == 1) {
                char *obj = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_deep_clone(%%struct.Value* %s)\n", result, obj);
                free(obj);
                return result;
            }
            
            // 文件I/O函数 - readFile
            if (strcmp(callee->name, "readFile") == 0 && call->arg_count == 1) {
                char *path = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_read_file(%%struct.Value* %s)\n", result, path);
                free(path);
                
                // 错误处理
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    // 有 ! 但不在 Try-Catch 中：检查错误并调用 value_fatal_error
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // writeFile
            if (strcmp(callee->name, "writeFile") == 0 && call->arg_count == 2) {
                char *path = codegen_expr(gen, call->args[0]);
                char *content = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_write_file(%%struct.Value* %s, %%struct.Value* %s)\n", result, path, content);
                free(path);
                free(content);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // appendFile
            if (strcmp(callee->name, "appendFile") == 0 && call->arg_count == 2) {
                char *path = codegen_expr(gen, call->args[0]);
                char *content = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_append_file(%%struct.Value* %s, %%struct.Value* %s)\n", result, path, content);
                free(path);
                free(content);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // readBytes
            if (strcmp(callee->name, "readBytes") == 0 && call->arg_count == 1) {
                char *path = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_read_bytes(%%struct.Value* %s)\n", result, path);
                free(path);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // writeBytes
            if (strcmp(callee->name, "writeBytes") == 0 && call->arg_count == 2) {
                char *path = codegen_expr(gen, call->args[0]);
                char *data = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_write_bytes(%%struct.Value* %s, %%struct.Value* %s)\n", result, path, data);
                free(path);
                free(data);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // fileExists
            if (strcmp(callee->name, "fileExists") == 0 && call->arg_count == 1) {
                char *path = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_file_exists(%%struct.Value* %s)\n", result, path);
                free(path);
                return result;
            }
            
            // deleteFile
            if (strcmp(callee->name, "deleteFile") == 0 && call->arg_count == 1) {
                char *path = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_delete_file(%%struct.Value* %s)\n", result, path);
                free(path);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // getFileSize
            if (strcmp(callee->name, "getFileSize") == 0 && call->arg_count == 1) {
                char *path = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_get_file_size(%%struct.Value* %s)\n", result, path);
                free(path);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // readLines
            if (strcmp(callee->name, "readLines") == 0 && call->arg_count == 1) {
                char *path = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_read_lines(%%struct.Value* %s)\n", result, path);
                free(path);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // renameFile
            if (strcmp(callee->name, "renameFile") == 0 && call->arg_count == 2) {
                char *old_path = codegen_expr(gen, call->args[0]);
                char *new_path = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_rename_file(%%struct.Value* %s, %%struct.Value* %s)\n", result, old_path, new_path);
                free(old_path);
                free(new_path);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // copyFile
            if (strcmp(callee->name, "copyFile") == 0 && call->arg_count == 2) {
                char *src = codegen_expr(gen, call->args[0]);
                char *dest = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_copy_file(%%struct.Value* %s, %%struct.Value* %s)\n", result, src, dest);
                free(src);
                free(dest);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // createDir
            if (strcmp(callee->name, "createDir") == 0 && call->arg_count == 1) {
                char *path = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_create_dir(%%struct.Value* %s)\n", result, path);
                free(path);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // removeDir
            if (strcmp(callee->name, "removeDir") == 0 && call->arg_count == 1) {
                char *path = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_remove_dir(%%struct.Value* %s)\n", result, path);
                free(path);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // listDir
            if (strcmp(callee->name, "listDir") == 0 && call->arg_count == 1) {
                char *path = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_list_dir(%%struct.Value* %s)\n", result, path);
                free(path);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                
                return result;
            }
            
            // dirExists
            if (strcmp(callee->name, "dirExists") == 0 && call->arg_count == 1) {
                char *path = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_dir_exists(%%struct.Value* %s)\n", result, path);
                free(path);
                
                // 如果没有 ! 后缀，清除错误状态
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                }
                
                return result;
            }
            
            // parseJSON
            if (strcmp(callee->name, "parseJSON") == 0 && call->arg_count == 1) {
                char *json_str = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_parse_json(%%struct.Value* %s)\n", result, json_str);
                free(json_str);
                
                // 处理错误
                if (call->throw_on_error == 0) {
                    // 无 ! 后缀：清除错误状态
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    // 有 ! 后缀且不在 Try-Catch 中：检查错误并终止
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    
                    // 错误处理：调用 value_fatal_error() 并终止 (parseJSON)
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    
                    // 继续执行
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                // 如果在 Try-Catch 中，什么都不做（让 Try-Catch 处理）
                
                return result;
            }
            
            // toJSON
            
            // toJSON
            if (strcmp(callee->name, "toJSON") == 0 && call->arg_count == 1) {
                char *obj = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_json(%%struct.Value* %s)\n", result, obj);
                free(obj);
                
                // toJSON 通常不会出错，但为了一致性也处理
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                }
                
                return result;
            }
            
            // ========================================
            // 数学函数 (Math Functions)
            // ========================================
            
            // abs(num) - 绝对值
            if (strcmp(callee->name, "abs") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_abs(%%struct.Value* %s)\n", result, arg);
                free(arg);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // floor(num) - 向下取整
            if (strcmp(callee->name, "floor") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_floor(%%struct.Value* %s)\n", result, arg);
                free(arg);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (gen->in_try_catch) {
                    // 有 ! 后缀且在 Try-Catch 中：检查错误并跳转到catch标签
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", 
                            is_error, gen->try_catch_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(continue_label);
                } else {
                    // 有 ! 后缀且不在 Try-Catch 中：检查错误并终止
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // ceil(num) - 向上取整
            if (strcmp(callee->name, "ceil") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_ceil(%%struct.Value* %s)\n", result, arg);
                free(arg);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // round(num) - 四舍五入
            if (strcmp(callee->name, "round") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_round(%%struct.Value* %s)\n", result, arg);
                free(arg);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // round(num, digits) - 四舍五入到指定小数位数
            if (strcmp(callee->name, "round") == 0 && call->arg_count == 2) {
                char *arg1 = codegen_expr(gen, call->args[0]);
                char *arg2 = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_round2(%%struct.Value* %s, %%struct.Value* %s)\n", result, arg1, arg2);
                free(arg1);
                free(arg2);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // sqrt(num) - 平方根
            if (strcmp(callee->name, "sqrt") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_sqrt(%%struct.Value* %s)\n", result, arg);
                free(arg);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // pow(base, exp) - 幂运算
            if (strcmp(callee->name, "pow") == 0 && call->arg_count == 2) {
                char *base = codegen_expr(gen, call->args[0]);
                char *exp = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_pow(%%struct.Value* %s, %%struct.Value* %s)\n", result, base, exp);
                free(base);
                free(exp);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // min(a, b) - 最小值
            if (strcmp(callee->name, "min") == 0 && call->arg_count == 2) {
                char *a = codegen_expr(gen, call->args[0]);
                char *b = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_min(%%struct.Value* %s, %%struct.Value* %s)\n", result, a, b);
                free(a);
                free(b);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // max(a, b) - 最大值
            if (strcmp(callee->name, "max") == 0 && call->arg_count == 2) {
                char *a = codegen_expr(gen, call->args[0]);
                char *b = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_max(%%struct.Value* %s, %%struct.Value* %s)\n", result, a, b);
                free(a);
                free(b);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // random() - 随机数 [0,1)
            if (strcmp(callee->name, "random") == 0 && call->arg_count == 0) {
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_random()\n", result);
                return result;
            }
            
            // ========================================
            // 字符串增强函数 (String Enhancement)
            // ========================================
            
            // startsWith(str, prefix) - 判断开头
            if (strcmp(callee->name, "startsWith") == 0 && call->arg_count == 2) {
                char *str = codegen_expr(gen, call->args[0]);
                char *prefix = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_starts_with(%%struct.Value* %s, %%struct.Value* %s)\n", result, str, prefix);
                free(str);
                free(prefix);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // endsWith(str, suffix) - 判断结尾
            if (strcmp(callee->name, "endsWith") == 0 && call->arg_count == 2) {
                char *str = codegen_expr(gen, call->args[0]);
                char *suffix = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_ends_with(%%struct.Value* %s, %%struct.Value* %s)\n", result, str, suffix);
                free(str);
                free(suffix);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // contains(str, substr) - 包含判断
            if (strcmp(callee->name, "contains") == 0 && call->arg_count == 2) {
                char *str = codegen_expr(gen, call->args[0]);
                char *substr = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_contains(%%struct.Value* %s, %%struct.Value* %s)\n", result, str, substr);
                free(str);
                free(substr);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // ========================================
            // 时间函数 (Time Functions)
            // ========================================
            
            // now() - 获取毫秒级时间戳
            if (strcmp(callee->name, "now") == 0 && call->arg_count == 0) {
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_now()\n", result);
                return result;
            }
            
            // time() - 获取Unix时间戳(秒)
            if (strcmp(callee->name, "time") == 0 && call->arg_count == 0) {
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_time()\n", result);
                return result;
            }
            
            // sleep(seconds) - 休眠
            if (strcmp(callee->name, "sleep") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_sleep(%%struct.Value* %s)\n", result, arg);
                free(arg);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // date() - 获取日期时间字符串
            if (strcmp(callee->name, "date") == 0 && call->arg_count == 0) {
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_date()\n", result);
                return result;
            }
            
            // ========================================
            // 系统操作函数 (System Functions)
            // ========================================
            
            // exit(code) - 退出程序
            if (strcmp(callee->name, "exit") == 0 && call->arg_count <= 1) {
                char *result = new_temp(gen);
                if (call->arg_count == 0) {
                    char *zero = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double 0.0)\n", zero);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_exit(%%struct.Value* %s)\n", result, zero);
                    free(zero);
                } else {
                    char *arg = codegen_expr(gen, call->args[0]);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_exit(%%struct.Value* %s)\n", result, arg);
                    free(arg);
                }
                return result;
            }
            
            // getEnv(name) - 获取环境变量
            if (strcmp(callee->name, "getEnv") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_get_env(%%struct.Value* %s)\n", result, arg);
                free(arg);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // setEnv(name, value) - 设置环境变量
            if (strcmp(callee->name, "setEnv") == 0 && call->arg_count == 2) {
                char *name = codegen_expr(gen, call->args[0]);
                char *value = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_set_env(%%struct.Value* %s, %%struct.Value* %s)\n", result, name, value);
                free(name);
                free(value);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // ========================================
            // 实用工具函数 (Utility Functions)
            // ========================================
            
            // isNaN(value) - 判断是否为NaN
            if (strcmp(callee->name, "isNaN") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_nan(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            // isFinite(value) - 判断是否为有限数
            if (strcmp(callee->name, "isFinite") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_finite(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            // clamp(value, min, max) - 限制数值范围
            if (strcmp(callee->name, "clamp") == 0 && call->arg_count == 3) {
                char *val = codegen_expr(gen, call->args[0]);
                char *min_val = codegen_expr(gen, call->args[1]);
                char *max_val = codegen_expr(gen, call->args[2]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_clamp(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n", result, val, min_val, max_val);
                free(val);
                free(min_val);
                free(max_val);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // ========================================
            // 数组扩展函数 (Array Extensions)
            // ========================================
            
            // reverse(arr) - 反转数组
            if (strcmp(callee->name, "reverse") == 0 && call->arg_count == 1) {
                char *arr = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_reverse(%%struct.Value* %s)\n", result, arr);
                free(arr);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                } else if (!gen->in_try_catch) {
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                    char *ok_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                    char *is_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(is_ok);
                    free(ok_bool);
                    free(is_error);
                    free(error_label);
                    free(continue_label);
                }
                return result;
            }
            
            // indexOf(arr, value) - 数组中查找元素索引
            if (strcmp(callee->name, "indexOf") == 0 && call->arg_count == 2) {
                // 检查第一个参数是否是数组类型（通过参数区分数组的indexOf和字符串的indexOf）
                // 这里我们用 value_index_of_array，它内部会检查类型
                // 如果是字符串，会fallback到原来的行为
                char *arr = codegen_expr(gen, call->args[0]);
                char *val = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_index_of_array(%%struct.Value* %s, %%struct.Value* %s)\n", result, arr, val);
                free(arr);
                free(val);
                return result;
            }
            
            // includes(arr, value) - 检查数组是否包含元素
            if (strcmp(callee->name, "includes") == 0 && call->arg_count == 2) {
                char *arr = codegen_expr(gen, call->args[0]);
                char *val = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_includes(%%struct.Value* %s, %%struct.Value* %s)\n", result, arr, val);
                free(arr);
                free(val);
                return result;
            }
            
            // ========================================
            // 对象扩展函数 (Object Extensions)
            // ========================================
            
            // values(obj) - 获取对象所有值
            if (strcmp(callee->name, "values") == 0 && call->arg_count == 1) {
                char *obj = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_values(%%struct.Value* %s)\n", result, obj);
                free(obj);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                }
                return result;
            }
            
            // entries(obj) - 获取对象所有键值对
            if (strcmp(callee->name, "entries") == 0 && call->arg_count == 1) {
                char *obj = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_entries(%%struct.Value* %s)\n", result, obj);
                free(obj);
                
                if (call->throw_on_error == 0) {
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                }
                return result;
            }
            
            // ========================================
            // 高阶函数 (Higher-Order Functions)
            // ========================================
            
            // map(arr, callback) - 将回调应用到数组每个元素
            if (strcmp(callee->name, "map") == 0 && call->arg_count == 2) {
                // 第二个参数必须是函数（匿名函数或标识符）
                ASTNode *callback_node = call->args[1];
                char *func_name = NULL;
                CapturedVars *captured = NULL;
                
                if (callback_node->kind == AST_FUNC_DECL) {
                    // 匿名函数：分析闭包捕获变量
                    ASTFuncDecl *func = (ASTFuncDecl *)callback_node->data;
                    func_name = strdup(func->name);
                    
                    // 分析并捕获外部变量
                    captured = analyze_captured_vars(gen, func->body, 
                                                     func->params, func->param_count);
                    
                    // 设置当前捕获变量，以便 codegen_stmt 可以使用
                    CapturedVars *saved_captured = gen->current_captured;
                    gen->current_captured = captured;
                    
                    // 生成匿名函数代码
                    FILE *saved_code_buf = gen->code_buf;
                    FILE *saved_entry_alloca = gen->entry_alloca_buf;
                    TempValueStack *saved_temp_values = gen->temp_values;  // 保存临时值栈
                    gen->temp_values = NULL;  // 匿名函数使用新的临时值栈
                    FILE *anon_func_buf = tmpfile();
                    gen->code_buf = anon_func_buf;
                    gen->entry_alloca_buf = NULL; // 清除，使匿名函数体内的 alloca 写入函数内
                    codegen_stmt(gen, callback_node);
                    gen->code_buf = saved_code_buf;
                    gen->entry_alloca_buf = saved_entry_alloca;
                    gen->temp_values = saved_temp_values;  // 恢复临时值栈
                    
                    // 恢复捕获变量状态
                    gen->current_captured = saved_captured;
                    
                    // 将匿名函数代码追加到 globals_buf
                    rewind(anon_func_buf);
                    char buf[1024];
                    while (fgets(buf, sizeof(buf), anon_func_buf)) {
                        fputs(buf, gen->globals_buf);
                    }
                    fclose(anon_func_buf);
                } else if (callback_node->kind == AST_IDENTIFIER) {
                    ASTIdentifier *id = (ASTIdentifier *)callback_node->data;
                    func_name = strdup(id->name);
                } else {
                    // 不支持的回调类型
                    char *result = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()  ; map: unsupported callback\n", result);
                    return result;
                }
                
                // 生成 map 循环
                char *arr = codegen_expr(gen, call->args[0]);
                char *arr_len = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i64 @value_array_length(%%struct.Value* %s)\n", arr_len, arr);
                
                // 分配结果数组
                char *result_arr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_create_array(i64 %s)\n", result_arr, arr_len);
                
                // 循环变量
                char *i_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = alloca i64\n", i_ptr);
                fprintf(gen->code_buf, "  store i64 0, i64* %s\n", i_ptr);
                
                // 预先加载捕获的变量值（在循环外）
                char **captured_vals = NULL;
                if (captured && captured->count > 0) {
                    captured_vals = (char **)malloc(captured->count * sizeof(char *));
                    for (size_t i = 0; i < captured->count; i++) {
                        captured_vals[i] = new_temp(gen);
                        // 检查是否是全局变量
                        if (is_global_var(gen, captured->names[i])) {
                            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** @%s\n",
                                    captured_vals[i], captured->names[i]);
                        } else {
                            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                                    captured_vals[i], captured->names[i]);
                        }
                    }
                }
                
                // 循环标签
                char *loop_start = new_label(gen);
                char *loop_body = new_label(gen);
                char *loop_end = new_label(gen);
                
                fprintf(gen->code_buf, "  br label %%%s\n", loop_start);
                fprintf(gen->code_buf, "%s:\n", loop_start);
                
                // 检查条件
                char *i_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load i64, i64* %s\n", i_val, i_ptr);
                char *cond = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp slt i64 %s, %s\n", cond, i_val, arr_len);
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", cond, loop_body, loop_end);
                
                fprintf(gen->code_buf, "%s:\n", loop_body);
                
                // 获取当前元素
                char *idx_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double 0.0)\n", idx_val);
                char *idx_double = new_temp(gen);
                fprintf(gen->code_buf, "  %s = sitofp i64 %s to double\n", idx_double, i_val);
                char *idx_boxed = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %s)\n", idx_boxed, idx_double);
                char *elem = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_array_get(%%struct.Value* %s, %%struct.Value* %s)\n", elem, arr, idx_boxed);
                
                // 调用回调函数（带捕获变量）
                char *mapped = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @%s(%%struct.Value* %s", mapped, func_name, elem);
                if (captured && captured->count > 0) {
                    for (size_t i = 0; i < captured->count; i++) {
                        fprintf(gen->code_buf, ", %%struct.Value* %s", captured_vals[i]);
                    }
                }
                fprintf(gen->code_buf, ")\n");
                
                // 设置结果数组元素
                fprintf(gen->code_buf, "  call %%struct.Value* @value_set_index(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n", result_arr, idx_boxed, mapped);
                
                // 增加计数器
                char *next_i = new_temp(gen);
                fprintf(gen->code_buf, "  %s = add i64 %s, 1\n", next_i, i_val);
                fprintf(gen->code_buf, "  store i64 %s, i64* %s\n", next_i, i_ptr);
                fprintf(gen->code_buf, "  br label %%%s\n", loop_start);
                
                fprintf(gen->code_buf, "%s:\n", loop_end);
                
                // 清理
                if (captured) {
                    if (captured_vals) {
                        for (size_t i = 0; i < captured->count; i++) {
                            free(captured_vals[i]);
                        }
                        free(captured_vals);
                    }
                    captured_vars_free(captured);
                }
                
                free(func_name);
                free(arr);
                free(arr_len);
                free(i_ptr);
                free(loop_start);
                free(loop_body);
                free(loop_end);
                free(i_val);
                free(cond);
                free(idx_val);
                free(idx_double);
                free(idx_boxed);
                free(elem);
                free(mapped);
                free(next_i);
                
                return result_arr;
            }
            
            // filter(arr, callback) - 过滤数组元素
            if (strcmp(callee->name, "filter") == 0 && call->arg_count == 2) {
                ASTNode *callback_node = call->args[1];
                char *func_name = NULL;
                CapturedVars *captured = NULL;
                
                if (callback_node->kind == AST_FUNC_DECL) {
                    ASTFuncDecl *func = (ASTFuncDecl *)callback_node->data;
                    func_name = strdup(func->name);
                    
                    // 分析并捕获外部变量
                    captured = analyze_captured_vars(gen, func->body, 
                                                     func->params, func->param_count);
                    
                    CapturedVars *saved_captured = gen->current_captured;
                    gen->current_captured = captured;
                    
                    FILE *saved_code_buf = gen->code_buf;
                    FILE *saved_entry_alloca = gen->entry_alloca_buf;
                    TempValueStack *saved_temp_values = gen->temp_values;  // 保存临时值栈
                    gen->temp_values = NULL;  // 匿名函数使用新的临时值栈
                    FILE *anon_func_buf = tmpfile();
                    gen->code_buf = anon_func_buf;
                    gen->entry_alloca_buf = NULL;
                    codegen_stmt(gen, callback_node);
                    gen->code_buf = saved_code_buf;
                    gen->entry_alloca_buf = saved_entry_alloca;
                    gen->temp_values = saved_temp_values;  // 恢复临时值栈
                    
                    gen->current_captured = saved_captured;
                    
                    rewind(anon_func_buf);
                    char buf[1024];
                    while (fgets(buf, sizeof(buf), anon_func_buf)) {
                        fputs(buf, gen->globals_buf);
                    }
                    fclose(anon_func_buf);
                } else if (callback_node->kind == AST_IDENTIFIER) {
                    ASTIdentifier *id = (ASTIdentifier *)callback_node->data;
                    func_name = strdup(id->name);
                } else {
                    char *result = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", result);
                    return result;
                }
                
                char *arr = codegen_expr(gen, call->args[0]);
                char *arr_len = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i64 @value_array_length(%%struct.Value* %s)\n", arr_len, arr);
                
                // 创建空结果数组，用 alloca 存储指针（因为 value_push 返回新数组）
                char *result_arr_ptr = new_temp(gen);
                char *init_arr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = alloca %%struct.Value*\n", result_arr_ptr);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_create_array(i64 0)\n", init_arr);
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", init_arr, result_arr_ptr);
                
                // 预先加载捕获的变量值（在循环外）
                char **captured_vals = NULL;
                if (captured && captured->count > 0) {
                    captured_vals = (char **)malloc(captured->count * sizeof(char *));
                    for (size_t i = 0; i < captured->count; i++) {
                        captured_vals[i] = new_temp(gen);
                        // 检查是否是全局变量
                        if (is_global_var(gen, captured->names[i])) {
                            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** @%s\n",
                                    captured_vals[i], captured->names[i]);
                        } else {
                            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                                    captured_vals[i], captured->names[i]);
                        }
                    }
                }
                
                // 循环
                char *i_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = alloca i64\n", i_ptr);
                fprintf(gen->code_buf, "  store i64 0, i64* %s\n", i_ptr);
                
                char *loop_start = new_label(gen);
                char *loop_body = new_label(gen);
                char *loop_push = new_label(gen);
                char *loop_next = new_label(gen);
                char *loop_end = new_label(gen);
                
                fprintf(gen->code_buf, "  br label %%%s\n", loop_start);
                fprintf(gen->code_buf, "%s:\n", loop_start);
                
                char *i_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load i64, i64* %s\n", i_val, i_ptr);
                char *cond = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp slt i64 %s, %s\n", cond, i_val, arr_len);
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", cond, loop_body, loop_end);
                
                fprintf(gen->code_buf, "%s:\n", loop_body);
                
                char *idx_double = new_temp(gen);
                fprintf(gen->code_buf, "  %s = sitofp i64 %s to double\n", idx_double, i_val);
                char *idx_boxed = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %s)\n", idx_boxed, idx_double);
                char *elem = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_array_get(%%struct.Value* %s, %%struct.Value* %s)\n", elem, arr, idx_boxed);
                
                // 调用回调（带捕获变量）
                char *pred_result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @%s(%%struct.Value* %s", pred_result, func_name, elem);
                if (captured && captured->count > 0) {
                    for (size_t i = 0; i < captured->count; i++) {
                        fprintf(gen->code_buf, ", %%struct.Value* %s", captured_vals[i]);
                    }
                }
                fprintf(gen->code_buf, ")\n");
                
                // 检查是否为真
                char *is_truthy = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", is_truthy, pred_result);
                char *should_push = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", should_push, is_truthy);
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", should_push, loop_push, loop_next);
                
                fprintf(gen->code_buf, "%s:\n", loop_push);
                // 加载当前结果数组，原地 push（push 已改为原地修改语义）
                char *cur_result = new_temp(gen);
                char *push_ret = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s\n", cur_result, result_arr_ptr);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_push(%%struct.Value* %s, %%struct.Value* %s)\n", push_ret, cur_result, elem);
                // push 返回新长度，release 返回值
                fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", push_ret);
                fprintf(gen->code_buf, "  br label %%%s\n", loop_next);
                
                fprintf(gen->code_buf, "%s:\n", loop_next);
                char *next_i = new_temp(gen);
                fprintf(gen->code_buf, "  %s = add i64 %s, 1\n", next_i, i_val);
                fprintf(gen->code_buf, "  store i64 %s, i64* %s\n", next_i, i_ptr);
                fprintf(gen->code_buf, "  br label %%%s\n", loop_start);
                
                fprintf(gen->code_buf, "%s:\n", loop_end);
                
                // 加载最终结果
                char *result_arr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s\n", result_arr, result_arr_ptr);
                
                // 清理
                if (captured) {
                    if (captured_vals) {
                        for (size_t i = 0; i < captured->count; i++) {
                            free(captured_vals[i]);
                        }
                        free(captured_vals);
                    }
                    captured_vars_free(captured);
                }
                
                free(func_name);
                free(arr);
                free(arr_len);
                free(i_ptr);
                free(loop_start);
                free(loop_body);
                free(loop_push);
                free(loop_next);
                free(loop_end);
                free(i_val);
                free(cond);
                free(idx_double);
                free(idx_boxed);
                free(elem);
                free(pred_result);
                free(is_truthy);
                free(should_push);
                free(next_i);
                
                return result_arr;
            }
            
            // reduce(arr, callback, initial) - 归约数组
            if (strcmp(callee->name, "reduce") == 0 && (call->arg_count == 2 || call->arg_count == 3)) {
                ASTNode *callback_node = call->args[1];
                char *func_name = NULL;
                CapturedVars *captured = NULL;
                
                if (callback_node->kind == AST_FUNC_DECL) {
                    ASTFuncDecl *func = (ASTFuncDecl *)callback_node->data;
                    func_name = strdup(func->name);
                    
                    // 分析并捕获外部变量
                    captured = analyze_captured_vars(gen, func->body, 
                                                     func->params, func->param_count);
                    
                    CapturedVars *saved_captured = gen->current_captured;
                    gen->current_captured = captured;
                    
                    FILE *saved_code_buf = gen->code_buf;
                    FILE *saved_entry_alloca = gen->entry_alloca_buf;
                    TempValueStack *saved_temp_values = gen->temp_values;  // 保存临时值栈
                    gen->temp_values = NULL;  // 匿名函数使用新的临时值栈
                    FILE *anon_func_buf = tmpfile();
                    gen->code_buf = anon_func_buf;
                    gen->entry_alloca_buf = NULL;
                    codegen_stmt(gen, callback_node);
                    gen->code_buf = saved_code_buf;
                    gen->entry_alloca_buf = saved_entry_alloca;
                    gen->temp_values = saved_temp_values;  // 恢复临时值栈
                    
                    gen->current_captured = saved_captured;
                    
                    rewind(anon_func_buf);
                    char buf[1024];
                    while (fgets(buf, sizeof(buf), anon_func_buf)) {
                        fputs(buf, gen->globals_buf);
                    }
                    fclose(anon_func_buf);
                } else if (callback_node->kind == AST_IDENTIFIER) {
                    ASTIdentifier *id = (ASTIdentifier *)callback_node->data;
                    func_name = strdup(id->name);
                } else {
                    char *result = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", result);
                    return result;
                }
                
                char *arr = codegen_expr(gen, call->args[0]);
                char *arr_len = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i64 @value_array_length(%%struct.Value* %s)\n", arr_len, arr);
                
                // 累加器
                char *acc_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = alloca %%struct.Value*\n", acc_ptr);
                
                // 预先加载捕获的变量值（在循环外）
                char **captured_vals = NULL;
                if (captured && captured->count > 0) {
                    captured_vals = (char **)malloc(captured->count * sizeof(char *));
                    for (size_t i = 0; i < captured->count; i++) {
                        captured_vals[i] = new_temp(gen);
                        // 检查是否是全局变量
                        if (is_global_var(gen, captured->names[i])) {
                            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** @%s\n",
                                    captured_vals[i], captured->names[i]);
                        } else {
                            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                                    captured_vals[i], captured->names[i]);
                        }
                    }
                }
                
                // 初始值和起始索引
                char *start_idx = NULL;
                if (call->arg_count == 3) {
                    char *initial = codegen_expr(gen, call->args[2]);
                    // Retain initial value before storing - it may be freed by temp cleanup
                    // but we still need it if the array is empty
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_retain(%%struct.Value* %s)\n", initial);
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", initial, acc_ptr);
                    start_idx = strdup("0");
                    free(initial);
                } else {
                    // 使用第一个元素作为初始值
                    char *first_idx = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double 0.0)\n", first_idx);
                    char *first_elem = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_array_get(%%struct.Value* %s, %%struct.Value* %s)\n", first_elem, arr, first_idx);
                    // Retain first element as initial accumulator value
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_retain(%%struct.Value* %s)\n", first_elem);
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", first_elem, acc_ptr);
                    start_idx = strdup("1");
                    free(first_idx);
                    free(first_elem);
                }
                
                // 循环
                char *i_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = alloca i64\n", i_ptr);
                fprintf(gen->code_buf, "  store i64 %s, i64* %s\n", start_idx, i_ptr);
                free(start_idx);
                
                char *loop_start = new_label(gen);
                char *loop_body = new_label(gen);
                char *loop_end = new_label(gen);
                
                fprintf(gen->code_buf, "  br label %%%s\n", loop_start);
                fprintf(gen->code_buf, "%s:\n", loop_start);
                
                char *i_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load i64, i64* %s\n", i_val, i_ptr);
                char *cond = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp slt i64 %s, %s\n", cond, i_val, arr_len);
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", cond, loop_body, loop_end);
                
                fprintf(gen->code_buf, "%s:\n", loop_body);
                
                char *idx_double = new_temp(gen);
                fprintf(gen->code_buf, "  %s = sitofp i64 %s to double\n", idx_double, i_val);
                char *idx_boxed = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %s)\n", idx_boxed, idx_double);
                char *elem = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_array_get(%%struct.Value* %s, %%struct.Value* %s)\n", elem, arr, idx_boxed);
                
                // 加载当前累加器
                char *acc_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s\n", acc_val, acc_ptr);
                
                // 调用回调 (acc, elem)（带捕获变量）
                char *new_acc = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @%s(%%struct.Value* %s, %%struct.Value* %s", new_acc, func_name, acc_val, elem);
                if (captured && captured->count > 0) {
                    for (size_t i = 0; i < captured->count; i++) {
                        fprintf(gen->code_buf, ", %%struct.Value* %s", captured_vals[i]);
                    }
                }
                fprintf(gen->code_buf, ")\n");
                
                // Release old accumulator before storing new one
                fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", acc_val);
                
                // 存储新累加器
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", new_acc, acc_ptr);
                
                // 增加计数器
                char *next_i = new_temp(gen);
                fprintf(gen->code_buf, "  %s = add i64 %s, 1\n", next_i, i_val);
                fprintf(gen->code_buf, "  store i64 %s, i64* %s\n", next_i, i_ptr);
                fprintf(gen->code_buf, "  br label %%%s\n", loop_start);
                
                fprintf(gen->code_buf, "%s:\n", loop_end);
                
                // 返回最终累加器
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s\n", result, acc_ptr);
                
                // 清理
                if (captured) {
                    if (captured_vals) {
                        for (size_t i = 0; i < captured->count; i++) {
                            free(captured_vals[i]);
                        }
                        free(captured_vals);
                    }
                    captured_vars_free(captured);
                }
                
                free(func_name);
                free(arr);
                free(arr_len);
                free(acc_ptr);
                free(i_ptr);
                free(loop_start);
                free(loop_body);
                free(loop_end);
                free(i_val);
                free(cond);
                free(idx_double);
                free(idx_boxed);
                free(elem);
                free(acc_val);
                free(new_acc);
                free(next_i);
                
                return result;
            }
            
            // find(arr, callback) - 查找第一个满足条件的元素
            if (strcmp(callee->name, "find") == 0 && call->arg_count == 2) {
                ASTNode *callback_node = call->args[1];
                char *func_name = NULL;
                
                if (callback_node->kind == AST_FUNC_DECL) {
                    ASTFuncDecl *func = (ASTFuncDecl *)callback_node->data;
                    func_name = strdup(func->name);
                    FILE *saved_code_buf = gen->code_buf;
                    FILE *saved_entry_alloca = gen->entry_alloca_buf;
                    TempValueStack *saved_temp_values = gen->temp_values;  // 保存临时值栈
                    gen->temp_values = NULL;  // 匿名函数使用新的临时值栈
                    FILE *anon_func_buf = tmpfile();
                    gen->code_buf = anon_func_buf;
                    gen->entry_alloca_buf = NULL;
                    codegen_stmt(gen, callback_node);
                    gen->code_buf = saved_code_buf;
                    gen->entry_alloca_buf = saved_entry_alloca;
                    gen->temp_values = saved_temp_values;  // 恢复临时值栈
                    rewind(anon_func_buf);
                    char buf[1024];
                    while (fgets(buf, sizeof(buf), anon_func_buf)) {
                        fputs(buf, gen->globals_buf);
                    }
                    fclose(anon_func_buf);
                } else if (callback_node->kind == AST_IDENTIFIER) {
                    ASTIdentifier *id = (ASTIdentifier *)callback_node->data;
                    func_name = strdup(id->name);
                } else {
                    char *result = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()\n", result);
                    return result;
                }
                
                char *arr = codegen_expr(gen, call->args[0]);
                char *arr_len = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i64 @value_array_length(%%struct.Value* %s)\n", arr_len, arr);
                
                // 结果指针
                char *result_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = alloca %%struct.Value*\n", result_ptr);
                char *undef_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()\n", undef_val);
                // Retain the undef value as it will be the default return value
                fprintf(gen->code_buf, "  call %%struct.Value* @value_retain(%%struct.Value* %s)\n", undef_val);
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", undef_val, result_ptr);
                
                // 循环
                char *i_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = alloca i64\n", i_ptr);
                fprintf(gen->code_buf, "  store i64 0, i64* %s\n", i_ptr);
                
                char *loop_start = new_label(gen);
                char *loop_body = new_label(gen);
                char *loop_found = new_label(gen);
                char *loop_next = new_label(gen);
                char *loop_end = new_label(gen);
                
                fprintf(gen->code_buf, "  br label %%%s\n", loop_start);
                fprintf(gen->code_buf, "%s:\n", loop_start);
                
                char *i_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load i64, i64* %s\n", i_val, i_ptr);
                char *cond = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp slt i64 %s, %s\n", cond, i_val, arr_len);
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", cond, loop_body, loop_end);
                
                fprintf(gen->code_buf, "%s:\n", loop_body);
                
                char *idx_double = new_temp(gen);
                fprintf(gen->code_buf, "  %s = sitofp i64 %s to double\n", idx_double, i_val);
                char *idx_boxed = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %s)\n", idx_boxed, idx_double);
                char *elem = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_array_get(%%struct.Value* %s, %%struct.Value* %s)\n", elem, arr, idx_boxed);
                
                // 调用回调
                char *pred_result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @%s(%%struct.Value* %s)\n", pred_result, func_name, elem);
                
                // 检查是否为真
                char *is_truthy = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", is_truthy, pred_result);
                char *is_found = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_found, is_truthy);
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_found, loop_found, loop_next);
                
                fprintf(gen->code_buf, "%s:\n", loop_found);
                // Retain the found element and release the previous result (undef)
                fprintf(gen->code_buf, "  call %%struct.Value* @value_retain(%%struct.Value* %s)\n", elem);
                char *old_result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s\n", old_result, result_ptr);
                fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", old_result);
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", elem, result_ptr);
                fprintf(gen->code_buf, "  br label %%%s\n", loop_end);
                
                fprintf(gen->code_buf, "%s:\n", loop_next);
                char *next_i = new_temp(gen);
                fprintf(gen->code_buf, "  %s = add i64 %s, 1\n", next_i, i_val);
                fprintf(gen->code_buf, "  store i64 %s, i64* %s\n", next_i, i_ptr);
                fprintf(gen->code_buf, "  br label %%%s\n", loop_start);
                
                fprintf(gen->code_buf, "%s:\n", loop_end);
                
                // 返回结果
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s\n", result, result_ptr);
                
                free(func_name);
                free(arr);
                free(arr_len);
                free(result_ptr);
                free(undef_val);
                free(i_ptr);
                free(loop_start);
                free(loop_body);
                free(loop_found);
                free(loop_next);
                free(loop_end);
                free(i_val);
                free(cond);
                free(idx_double);
                free(idx_boxed);
                free(elem);
                free(pred_result);
                free(is_truthy);
                free(is_found);
                free(next_i);
                
                return result;
            }
            
            // sort(arr, callback?) - 排序数组
            // 注意：sort 需要运行时支持，这里只处理无回调的情况
            if (strcmp(callee->name, "sort") == 0 && call->arg_count >= 1) {
                char *arr = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                
                if (call->arg_count == 1) {
                    // 无回调，使用默认排序
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_sort(%%struct.Value* %s, %%struct.Value* (%%struct.Value*, %%struct.Value*)* null)\n", result, arr);
                } else {
                    // 有回调，需要传递函数指针
                    ASTNode *callback_node = call->args[1];
                    char *func_name = NULL;
                    
                    if (callback_node->kind == AST_FUNC_DECL) {
                        ASTFuncDecl *func = (ASTFuncDecl *)callback_node->data;
                        func_name = strdup(func->name);
                        FILE *saved_code_buf = gen->code_buf;
                        FILE *saved_entry_alloca = gen->entry_alloca_buf;
                        TempValueStack *saved_temp_values = gen->temp_values;  // 保存临时值栈
                        gen->temp_values = NULL;  // 匿名函数使用新的临时值栈
                        FILE *anon_func_buf = tmpfile();
                        gen->code_buf = anon_func_buf;
                        gen->entry_alloca_buf = NULL;
                        codegen_stmt(gen, callback_node);
                        gen->code_buf = saved_code_buf;
                        gen->entry_alloca_buf = saved_entry_alloca;
                        gen->temp_values = saved_temp_values;  // 恢复临时值栈
                        rewind(anon_func_buf);
                        char buf[1024];
                        while (fgets(buf, sizeof(buf), anon_func_buf)) {
                            fputs(buf, gen->globals_buf);
                        }
                        fclose(anon_func_buf);
                    } else if (callback_node->kind == AST_IDENTIFIER) {
                        ASTIdentifier *id = (ASTIdentifier *)callback_node->data;
                        func_name = strdup(id->name);
                    }
                    
                    if (func_name) {
                        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_sort(%%struct.Value* %s, %%struct.Value* (%%struct.Value*, %%struct.Value*)* @%s)\n", result, arr, func_name);
                        free(func_name);
                    } else {
                        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_sort(%%struct.Value* %s, %%struct.Value* (%%struct.Value*, %%struct.Value*)* null)\n", result, arr);
                    }
                }
                
                free(arr);
                return result;
            }
            
            // 普通函数调用 - 检查是否是函数名还是变量名
            // 注意：函数名经过 varmap 处理后是 _XXXXX 格式
            if (getenv("DEBUG_CODEGEN")) {
                fprintf(stderr, "[DEBUG CODEGEN] Checking function call: %s, is_function=%d, is_symbol=%d\n", 
                        callee->name, is_function_name(gen, callee->name), is_symbol_defined(gen, callee->name));
            }
            
            // 区分函数名和变量名
            int is_func = is_function_name(gen, callee->name);
            int is_sym = is_symbol_defined(gen, callee->name);
            
            if (is_func) {
                // 直接函数调用 - callee 是一个已定义的函数名
                char **arg_regs = NULL;
                if (call->arg_count > 0) {
                    arg_regs = (char **)malloc(call->arg_count * sizeof(char *));
                    for (size_t i = 0; i < call->arg_count; i++) {
                        arg_regs[i] = codegen_expr(gen, call->args[i]);
                    }
                }
                
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @%s(", result, callee->name);
                
                for (size_t i = 0; i < call->arg_count; i++) {
                    if (i > 0) fprintf(gen->code_buf, ", ");
                    fprintf(gen->code_buf, "%%struct.Value* %s", arg_regs[i]);
                }
                
                fprintf(gen->code_buf, ")\n");
                
                if (arg_regs) {
                    for (size_t i = 0; i < call->arg_count; i++) {
                        free(arg_regs[i]);
                    }
                    free(arg_regs);
                }
                
                return result;
            } else if (is_sym) {
                // 变量调用 - callee 是一个变量，可能存储函数值
                // 需要在运行时检查类型并间接调用
                
                // 计算参数（必须在分支之前，以确保临时值在正确的基本块中）
                char **arg_regs = NULL;
                if (call->arg_count > 0) {
                    arg_regs = (char **)malloc(call->arg_count * sizeof(char *));
                    for (size_t i = 0; i < call->arg_count; i++) {
                        arg_regs[i] = codegen_expr(gen, call->args[i]);
                    }
                }
                
                // 创建参数数组（在分支之前）
                char *args_array = new_temp(gen);
                char *args_ptr = new_temp(gen);
                if (call->arg_count > 0) {
                    fprintf(gen->code_buf, "  %s = alloca [%zu x %%struct.Value*]\n", args_array, call->arg_count);
                    for (size_t i = 0; i < call->arg_count; i++) {
                        char *slot = new_temp(gen);
                        fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 %zu\n",
                                slot, call->arg_count, call->arg_count, args_array, i);
                        fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", arg_regs[i], slot);
                        free(slot);
                    }
                    fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 0\n",
                            args_ptr, call->arg_count, call->arg_count, args_array);
                } else {
                    fprintf(gen->code_buf, "  %s = inttoptr i64 0 to [0 x %%struct.Value*]*\n", args_array);
                    fprintf(gen->code_buf, "  %s = inttoptr i64 0 to %%struct.Value**\n", args_ptr);
                }
                
                // 加载变量值
                char *func_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s  ; load potential function value\n", 
                        func_val, callee->name);
                
                // 检查是否是函数类型
                char *is_func_check = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_function(%%struct.Value* %s)\n", is_func_check, func_val);
                
                // 分支：是函数则调用，否则报错
                char *call_label = new_label(gen);
                char *error_label = new_label(gen);
                char *merge_label = new_label(gen);
                
                char *is_func_bool = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_func_bool, is_func_check);
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_func_bool, call_label, error_label);
                
                // 函数调用分支
                fprintf(gen->code_buf, "%s:\n", call_label);
                
                // 调用运行时辅助函数
                char *call_result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @call_function_value(%%struct.Value* %s, %%struct.Value** %s, i32 %zu)\n",
                        call_result, func_val, args_ptr, call->arg_count);
                
                fprintf(gen->code_buf, "  br label %%%s\n", merge_label);
                
                // 错误分支
                fprintf(gen->code_buf, "%s:\n", error_label);
                char *error_result = new_temp(gen);
                fprintf(gen->code_buf, "  ; ERROR: trying to call non-function value\n");
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()\n", error_result);
                fprintf(gen->code_buf, "  br label %%%s\n", merge_label);
                
                // 合并分支
                fprintf(gen->code_buf, "%s:\n", merge_label);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = phi %%struct.Value* [ %s, %%%s ], [ %s, %%%s ]\n",
                        result, call_result, call_label, error_result, error_label);
                
                // 清理
                if (arg_regs) {
                    for (size_t i = 0; i < call->arg_count; i++) {
                        free(arg_regs[i]);
                    }
                    free(arg_regs);
                }
                free(func_val);
                free(is_func_check);
                free(call_label);
                free(error_label);
                free(merge_label);
                free(is_func_bool);
                free(args_array);
                free(args_ptr);
                free(call_result);
                free(error_result);
                
                return result;
            } else {
                // 未定义的函数 - 报错
                const char *original_name = codegen_lookup_original_name(gen, callee->name);
                const char *display_name = original_name ? original_name : callee->name;
                
                // 使用显示名称的长度作为错误区域长度
                int name_length = (int)strlen(display_name);
                
                codegen_set_error_at(gen, node->loc.orig_line, node->loc.orig_column,
                                     name_length,
                                     display_name, "Undefined function");
                
                // 返回一个 undef 值以继续生成
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()  ; ERROR: undefined function '%s'\n", 
                        result, callee->name);
                return result;
            }
        }
        
        case AST_ARRAY_LITERAL: {
            // 实现：分配 Value* 数组并包装成 Value
            ASTArrayLiteral *arr_lit = (ASTArrayLiteral *)node->data;
            size_t count = arr_lit->elem_count;
            
            if (count == 0) {
                // 空数组：返回一个size=0的数组
                char *temp = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_array(i8* null, i64 0)  ; empty array\n", temp);
                return temp;
            }
            
            // 检查是否有展开元素
            bool has_spread = false;
            if (arr_lit->is_spread) {
                for (size_t i = 0; i < count; i++) {
                    if (arr_lit->is_spread[i]) {
                        has_spread = true;
                        break;
                    }
                }
            }
            
            if (has_spread) {
                // 有展开元素：使用运行时函数动态构建
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_array(i8* null, i64 0)  ; start with empty array\n", result);
                
                for (size_t i = 0; i < count; i++) {
                    char *elem_val = codegen_expr(gen, arr_lit->elements[i]);
                    if (elem_val) {
                        char *new_result = new_temp(gen);
                        if (arr_lit->is_spread[i]) {
                            // 展开：调用 spread_into_array
                            fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_spread_into_array(%%struct.Value* %s, %%struct.Value* %s)\n",
                                    new_result, result, elem_val);
                        } else {
                            // 普通元素：调用 push
                            fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_push(%%struct.Value* %s, %%struct.Value* %s)\n",
                                    new_result, result, elem_val);
                        }
                        // 释放旧数组
                        fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", result);
                        free(result);
                        result = new_result;
                        free(elem_val);
                    }
                }
                return result;
            }
            
            // 无展开：使用编译时静态分配
            // 分配数组：[count x %struct.Value*]
            char *array_ptr = new_temp(gen);
            fprintf(gen->code_buf, "  %s = alloca [%zu x %%struct.Value*]  ; mixed-type array with %zu elements\n", 
                    array_ptr, count, count);
            
            // 初始化每个元素
            for (size_t i = 0; i < count; i++) {
                char *elem_val = codegen_expr(gen, arr_lit->elements[i]);
                if (elem_val) {
                    char *elem_ptr = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 %zu\n",
                            elem_ptr, count, count, array_ptr, i);
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", elem_val, elem_ptr);
                    free(elem_val);
                }
            }
            
            // 如果我们知道这个数组要赋给哪个变量，就注册它
            if (gen->current_var_name) {
                register_array(gen, gen->current_var_name, array_ptr, count);
            }
            
            // 创建一个 Value 包装数组指针
            // 将数组指针转换为 i8* 存储在 Value 中
            char *array_i8 = new_temp(gen);
            fprintf(gen->code_buf, "  %s = bitcast [%zu x %%struct.Value*]* %s to i8*\n",
                    array_i8, count, array_ptr);
            
            // 调用运行时函数创建数组 Value (需要实现 box_array)
            char *result = new_temp(gen);
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_array(i8* %s, i64 %zu)\n",
                    result, array_i8, count);
            
            free(array_i8);
            return result;
        }
        
        case AST_OBJECT_LITERAL: {
            // 实现对象字面量：创建 ObjectEntry 数组
            ASTObjectLiteral *obj_lit = (ASTObjectLiteral *)node->data;
            size_t prop_count = obj_lit->prop_count;
            
            if (prop_count == 0) {
                // 空对象
                char *temp = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_object(i8* null, i64 0)  ; empty object\n", temp);
                return temp;
            }
            
            // 检查是否有展开属性
            bool has_spread = false;
            for (size_t i = 0; i < prop_count; i++) {
                if (obj_lit->properties[i].is_spread) {
                    has_spread = true;
                    break;
                }
            }
            
            if (has_spread) {
                // 有展开：使用运行时函数动态构建
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_object(i8* null, i64 0)  ; start with empty object\n", result);
                
                for (size_t i = 0; i < prop_count; i++) {
                    ASTObjectProperty *prop = &obj_lit->properties[i];
                    
                    if (prop->is_spread) {
                        // 展开：调用 spread_into_object，返回新对象
                        char *spread_val = codegen_expr(gen, prop->value);
                        char *new_result = new_temp(gen);
                        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_spread_into_object(%%struct.Value* %s, %%struct.Value* %s)\n",
                                new_result, result, spread_val);
                        fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", result);
                        free(result);
                        result = new_result;
                        free(spread_val);
                    } else {
                        // 普通属性：调用 set_field，原地修改并返回同一对象
                        char *key_label = new_string_label(gen);
                        size_t key_len = strlen(prop->key);
                        fprintf(gen->strings_buf, "%s = private unnamed_addr constant [%zu x i8] c\"%s\\00\"\n",
                                key_label, key_len + 1, prop->key);
                        
                        char *key_ptr = new_temp(gen);
                        fprintf(gen->code_buf, "  %s = getelementptr [%zu x i8], [%zu x i8]* %s, i32 0, i32 0\n",
                                key_ptr, key_len + 1, key_len + 1, key_label);
                        
                        char *key_val = new_temp(gen);
                        fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_string(i8* %s)\n", key_val, key_ptr);
                        
                        char *prop_val = codegen_expr(gen, prop->value);
                        // value_set_field 原地修改对象并返回同一对象，不需要更新 result
                        fprintf(gen->code_buf, "  call %%struct.Value* @value_set_field(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n",
                                result, key_val, prop_val);
                        fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", key_val);
                        free(key_val);
                        free(prop_val);
                    }
                }
                return result;
            }
            
            // 无展开：使用堆分配（避免循环中 alloca 导致栈溢出）
            // 声明 malloc 和 free（如果还没声明）
            // 注意：LLVM 模块级别已有 malloc/free 声明
            
            // 计算 ObjectEntry 数组大小
            // ObjectEntry 结构: { i8* key, Value* value } = 16 bytes on 64-bit
            char *entries_alloc = new_temp(gen);
            char *entries_size = new_temp(gen);
            fprintf(gen->code_buf, "  %s = mul i64 %zu, 16  ; sizeof(ObjectEntry) * count\n",
                    entries_size, prop_count);
            fprintf(gen->code_buf, "  %s = call i8* @malloc(i64 %s)\n",
                    entries_alloc, entries_size);
            
            // 转换为 ObjectEntry* 类型
            char *entries_typed = new_temp(gen);
            fprintf(gen->code_buf, "  %s = bitcast i8* %s to %%struct.ObjectEntry*\n",
                    entries_typed, entries_alloc);
            free(entries_size);
            
            // 用于注册对象元数据的字段列表
            ObjectField *field_head = NULL;
            ObjectField *field_tail = NULL;
            
            // 为每个属性填充 ObjectEntry
            for (size_t i = 0; i < prop_count; i++) {
                ASTObjectProperty *prop = &obj_lit->properties[i];
                
                // 生成键字符串
                char *key_label = new_string_label(gen);
                size_t key_len = strlen(prop->key);
                fprintf(gen->strings_buf, "%s = private unnamed_addr constant [%zu x i8] c\"%s\\00\"\n",
                        key_label, key_len + 1, prop->key);
                
                char *key_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = getelementptr [%zu x i8], [%zu x i8]* %s, i32 0, i32 0\n",
                        key_ptr, key_len + 1, key_len + 1, key_label);
                
                // 计算属性值
                char *value = codegen_expr(gen, prop->value);
                
                // 获取 entry[i] 的指针 (使用 entries_typed 而不是 entries_alloc)
                char *entry_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = getelementptr %%struct.ObjectEntry, %%struct.ObjectEntry* %s, i64 %zu\n",
                        entry_ptr, entries_typed, i);
                
                // 设置 entry.key
                char *key_field = new_temp(gen);
                fprintf(gen->code_buf, "  %s = getelementptr %%struct.ObjectEntry, %%struct.ObjectEntry* %s, i32 0, i32 0\n",
                        key_field, entry_ptr);
                fprintf(gen->code_buf, "  store i8* %s, i8** %s\n", key_ptr, key_field);
                
                // 设置 entry.value
                char *value_field = new_temp(gen);
                fprintf(gen->code_buf, "  %s = getelementptr %%struct.ObjectEntry, %%struct.ObjectEntry* %s, i32 0, i32 1\n",
                        value_field, entry_ptr);
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", value, value_field);
                
                // 创建字段元数据（用于后续的成员访问）
                ObjectField *field = (ObjectField *)malloc(sizeof(ObjectField));
                field->field_name = strdup(prop->key);
                field->field_ptr = strdup(value_field);
                field->next = NULL;
                
                if (field_tail) {
                    field_tail->next = field;
                    field_tail = field;
                } else {
                    field_head = field_tail = field;
                }
                
                free(value);
            }
            
            // 调用 box_object（直接使用 entries_alloc，它已经是 i8*）
            char *result = new_temp(gen);
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_object(i8* %s, i64 %zu)\n",
                    result, entries_alloc, prop_count);
            
            // 释放临时分配的内存（box_object 会复制数据）
            fprintf(gen->code_buf, "  call void @free(i8* %s)\n", entries_alloc);
            
            // 清理临时变量
            free(entries_alloc);
            free(entries_typed);
            
            // 如果当前正在变量赋值中，注册对象元数据
            if (gen->current_var_name && field_head) {
                register_object(gen, gen->current_var_name, field_head);
            }
            
            return result;
        }
        
        case AST_UNARY_EXPR: {
            ASTUnaryExpr *unary = (ASTUnaryExpr *)node->data;
            
            // 处理 ++ 和 -- 运算符
            if (unary->op == TK_PLUS_PLUS || unary->op == TK_MINUS_MINUS) {
                // 获取变量名（假设 operand 是 IDENTIFIER）
                if (unary->operand->kind != AST_IDENTIFIER) {
                    fprintf(stderr, "Error: ++ and -- can only be applied to variables\n");
                    return strdup("%error");
                }
                
                ASTIdentifier *id = (ASTIdentifier *)unary->operand->data;
                char *old_val = new_temp(gen);
                char *one = new_temp(gen);
                char *new_val = new_temp(gen);
                
                // 加载当前值
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n", old_val, id->name);
                
                // 创建 1 的 Value
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double 1.0)\n", one);
                
                // 增加或减少
                if (unary->op == TK_PLUS_PLUS) {
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_add(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            new_val, old_val, one);
                } else {
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_subtract(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            new_val, old_val, one);
                }
                
                // 存储新值
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n", new_val, id->name);
                
                // 返回值：前缀返回新值，后缀返回旧值
                free(one);
                if (unary->is_postfix) {
                    free(new_val);
                    return old_val;  // i++: 返回旧值
                } else {
                    free(old_val);
                    return new_val;  // ++i: 返回新值
                }
            }
            
            // 其他一元运算符
            char *operand = codegen_expr(gen, unary->operand);
            char *result = new_temp(gen);
            
            switch (unary->op) {
                case TK_MINUS: {
                    // -x: 使用 value_multiply(x, -1)
                    char *neg_one = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double -1.0)\n", neg_one);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_multiply(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, operand, neg_one);
                    free(neg_one);
                    break;
                }
                case TK_PLUS:
                    // 一元+不改变值
                    free(result);
                    return operand;
                case TK_BANG: {
                    // !x: 使用 value_is_truthy 然后取反
                    char *truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", truthy, operand);
                    char *inverted = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", inverted, truthy);
                    char *as_i32 = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", as_i32, inverted);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, as_i32);
                    free(truthy);
                    free(inverted);
                    free(as_i32);
                    break;
                }
                default:
                    free(result);
                    free(operand);
                    return NULL;
            }
            
            free(operand);
            return result;
        }
        
        case AST_MEMBER_EXPR: {
            // 实现对象成员访问：obj.prop 或 expr.prop
            ASTMemberExpr *member = (ASTMemberExpr *)node->data;
            
            // 支持任意表达式作为对象（不仅限于标识符）
            char *obj_value = NULL;
            const char *obj_name = NULL;
            
            if (member->object->kind == AST_IDENTIFIER) {
                // 简单标识符：obj.prop
                ASTIdentifier *obj_ident = (ASTIdentifier *)member->object->data;
                obj_name = obj_ident->name;
                
                // 加载对象
                obj_value = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                        obj_value, obj_name);
            } else {
                // 复杂表达式：(expr).prop, obj.nested.prop 等
                obj_value = codegen_expr(gen, member->object);
                if (!obj_value) {
                    char *temp = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()  ; nested member expr failed\n", temp);
                    return temp;
                }
            }
            
            // 特殊处理：内置方法
            if (strcmp(member->property, "len") == 0) {
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_len(%%struct.Value* %s)\n", result, obj_value);
                free(obj_value);
                return result;
            }
            
            if (strcmp(member->property, "upper") == 0) {
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_upper(%%struct.Value* %s)\n", result, obj_value);
                free(obj_value);
                return result;
            }
            
            if (strcmp(member->property, "lower") == 0) {
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_lower(%%struct.Value* %s)\n", result, obj_value);
                free(obj_value);
                return result;
            }
            
            if (strcmp(member->property, "trim") == 0) {
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_trim(%%struct.Value* %s)\n", result, obj_value);
                free(obj_value);
                return result;
            }
            
            // 创建字段名字符串
            char *key_label = new_string_label(gen);
            size_t key_len = strlen(member->property);
            fprintf(gen->strings_buf, "%s = private unnamed_addr constant [%zu x i8] c\"%s\\00\"\n",
                    key_label, key_len + 1, member->property);
            
            char *key_ptr = new_temp(gen);
            fprintf(gen->code_buf, "  %s = getelementptr [%zu x i8], [%zu x i8]* %s, i32 0, i32 0\n",
                    key_ptr, key_len + 1, key_len + 1, key_label);
            
            char *field_name = new_temp(gen);
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_string(i8* %s)\n",
                    field_name, key_ptr);
            
            // 检查是否是未绑定方法访问 (.@)
            // 如果是未绑定访问，使用 value_get_field 不进行绑定
            // 否则使用 value_get_method 自动绑定 self
            char *result = new_temp(gen);
            if (member->is_optional) {
                // 可选链访问: obj?.prop - 如果 obj 为 null/undef 返回 undef，否则获取属性
                // 首先检查 obj 是否为 null 或 undef
                char *is_null_val = new_temp(gen);
                char *is_undef_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_null(%%struct.Value* %s)\n", 
                        is_null_val, obj_value);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_undef(%%struct.Value* %s)\n", 
                        is_undef_val, obj_value);
                char *is_null_truthy = new_temp(gen);
                char *is_undef_truthy = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                        is_null_truthy, is_null_val);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                        is_undef_truthy, is_undef_val);
                fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", is_null_val);
                fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", is_undef_val);
                char *is_null_bool = new_temp(gen);
                char *is_undef_bool = new_temp(gen);
                char *is_nullish = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_null_bool, is_null_truthy);
                fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_undef_bool, is_undef_truthy);
                fprintf(gen->code_buf, "  %s = or i1 %s, %s\n", is_nullish, is_null_bool, is_undef_bool);
                
                // 使用安全的属性访问，不存在返回 undef
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_get_field_safe(%%struct.Value* %s, %%struct.Value* %s)  ; ?.%s (optional)\n",
                        result, obj_value, field_name, member->property);
                
                // 如果对象本身是 nullish，返回 undef
                char *undef_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()\n", undef_val);
                char *final_result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = select i1 %s, %%struct.Value* %s, %%struct.Value* %s\n",
                        final_result, is_nullish, undef_val, result);
                
                free(obj_value);
                free(key_label);
                free(key_ptr);
                free(field_name);
                free(is_null_val);
                free(is_undef_val);
                free(is_null_truthy);
                free(is_undef_truthy);
                free(is_null_bool);
                free(is_undef_bool);
                free(is_nullish);
                free(undef_val);
                free(result);
                return final_result;
            } else if (member->is_unbound) {
                // 未绑定方法访问: obj.@method - 不绑定 self
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_get_field(%%struct.Value* %s, %%struct.Value* %s)  ; .@%s (unbound)\n",
                        result, obj_value, field_name, member->property);
                
                // 非可选链访问需要检查错误（等同于 getField()!）
                // 在 T> 块中：检查错误并跳转到catch标签
                // 不在 T> 块中：检查错误并终止程序
                char *is_ok = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                char *ok_bool = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                char *is_error = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                
                if (gen->in_try_catch) {
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", 
                            is_error, gen->try_catch_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(continue_label);
                } else {
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(error_label);
                    free(continue_label);
                }
                
                free(is_ok);
                free(ok_bool);
                free(is_error);
            } else {
                // 普通方法访问: obj.method - 自动绑定 self
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_get_method(%%struct.Value* %s, %%struct.Value* %s)  ; .%s (bound)\n",
                        result, obj_value, field_name, member->property);
                
                // 非可选链访问需要检查错误（等同于 getMethod()!）
                char *is_ok = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                char *ok_bool = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                char *is_error = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                
                if (gen->in_try_catch) {
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", 
                            is_error, gen->try_catch_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(continue_label);
                } else {
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(error_label);
                    free(continue_label);
                }
                
                free(is_ok);
                free(ok_bool);
                free(is_error);
            }
            
            free(obj_value);
            free(key_label);
            free(key_ptr);
            free(field_name);
            return result;
        }
        
        case AST_INDEX_EXPR: {
            // 实现数组/对象索引访问：arr[index] 或 arr@[index]（解绑）或 arr?[index]（可选链）
            // 注意：不再使用栈数组快速路径优化，因为数组可能被修改
            // (例如 arr[i] = undef 会修改堆上的数组对象，而非栈上的alloca)
            // 所以必须使用 value_index() 运行时函数来保证正确性
            ASTIndexExpr *idx_expr = (ASTIndexExpr *)node->data;
            
            // 通用路径：使用运行时函数
            char *obj_val = codegen_expr(gen, idx_expr->object);
            char *index_val = codegen_expr(gen, idx_expr->index);
            
            char *result = new_temp(gen);
            
            if (idx_expr->is_optional) {
                // 可选链访问：obj?[index] - 如果 obj 为 null/undef 返回 undef
                char *is_null_val = new_temp(gen);
                char *is_undef_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_null(%%struct.Value* %s)\n", 
                        is_null_val, obj_val);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_undef(%%struct.Value* %s)\n", 
                        is_undef_val, obj_val);
                char *is_null_truthy = new_temp(gen);
                char *is_undef_truthy = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                        is_null_truthy, is_null_val);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                        is_undef_truthy, is_undef_val);
                fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", is_null_val);
                fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", is_undef_val);
                char *is_null_bool = new_temp(gen);
                char *is_undef_bool = new_temp(gen);
                char *is_nullish = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_null_bool, is_null_truthy);
                fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_undef_bool, is_undef_truthy);
                fprintf(gen->code_buf, "  %s = or i1 %s, %s\n", is_nullish, is_null_bool, is_undef_bool);
                
                // 使用安全的索引访问
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_index_safe(%%struct.Value* %s, %%struct.Value* %s)  ; ?[] (optional)\n",
                        result, obj_val, index_val);
                
                // 如果对象本身是 nullish，返回 undef
                char *undef_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()\n", undef_val);
                char *final_result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = select i1 %s, %%struct.Value* %s, %%struct.Value* %s\n",
                        final_result, is_nullish, undef_val, result);
                
                free(obj_val);
                free(index_val);
                free(is_null_val);
                free(is_undef_val);
                free(is_null_truthy);
                free(is_undef_truthy);
                free(is_null_bool);
                free(is_undef_bool);
                free(is_nullish);
                free(undef_val);
                free(result);
                return final_result;
            } else if (idx_expr->is_unbound) {
                // 解绑访问：obj@[index] - 直接获取值，不绑定 self
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_index(%%struct.Value* %s, %%struct.Value* %s)\n",
                        result, obj_val, index_val);
                
                // 非可选链访问需要检查错误（等同于 index()!）
                char *is_ok = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                char *ok_bool = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                char *is_error = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                
                if (gen->in_try_catch) {
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", 
                            is_error, gen->try_catch_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(continue_label);
                } else {
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(error_label);
                    free(continue_label);
                }
                
                free(is_ok);
                free(ok_bool);
                free(is_error);
            } else {
                // 普通访问：obj[index] - 如果是方法则绑定 self
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_get_method_by_index(%%struct.Value* %s, %%struct.Value* %s)\n",
                        result, obj_val, index_val);
                
                // 非可选链访问需要检查错误（等同于 index()!）
                char *is_ok = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", is_ok);
                char *ok_bool = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", ok_bool, is_ok);
                char *is_error = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", is_error, ok_bool);
                
                if (gen->in_try_catch) {
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", 
                            is_error, gen->try_catch_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(continue_label);
                } else {
                    char *error_label = new_label(gen);
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_error, error_label, continue_label);
                    fprintf(gen->code_buf, "%s:\n", error_label);
                    fprintf(gen->code_buf, "  call void @value_fatal_error()\n");
                    fprintf(gen->code_buf, "  unreachable\n");
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    free(error_label);
                    free(continue_label);
                }
                
                free(is_ok);
                free(ok_bool);
                free(is_error);
            }
            
            free(obj_val);
            free(index_val);
            
            return result;
        }
        
        case AST_FUNC_DECL: {
            // 函数声明作为表达式（嵌套函数/匿名函数）
            // 将函数体写入全局区，返回函数值（包含函数指针和捕获变量）
            ASTFuncDecl *func = (ASTFuncDecl *)node->data;
            
            if (getenv("DEBUG_CLOSURE")) {
                fprintf(stderr, "[DEBUG CLOSURE] codegen_expr AST_FUNC_DECL: %s\n", func->name);
            }
            
            // 分析闭包捕获的变量
            CapturedVars *captured = analyze_captured_vars(gen, func->body, 
                                                           func->params, func->param_count);
            
            // 检查函数是否使用了 self 关键字
            func->uses_self = closure_analysis_uses_self();
            
            // 保存当前状态
            FILE *saved_code_buf = gen->code_buf;
            FILE *saved_entry_alloca = gen->entry_alloca_buf;
            TempValueStack *saved_temp_values = gen->temp_values;
            CapturedVars *saved_captured = gen->current_captured;
            
            // 设置闭包捕获变量
            gen->current_captured = captured;
            gen->temp_values = NULL;  // 函数使用新的临时值栈
            
            // 创建临时缓冲区用于生成函数代码
            FILE *func_buf = tmpfile();
            gen->code_buf = func_buf;
            // 注意：保持 entry_alloca_buf 不变，这样 codegen_stmt 能正确检测嵌套
            // 如果 entry_alloca_buf 非 NULL，表示我们在函数内部定义闭包
            
            // 生成函数定义
            codegen_stmt(gen, node);
            
            // 恢复代码缓冲区和状态
            gen->code_buf = saved_code_buf;
            gen->entry_alloca_buf = saved_entry_alloca;
            gen->temp_values = saved_temp_values;
            gen->current_captured = saved_captured;
            
            // 将函数代码追加到 globals_buf
            rewind(func_buf);
            char buf[1024];
            while (fgets(buf, sizeof(buf), func_buf)) {
                fputs(buf, gen->globals_buf);
            }
            fclose(func_buf);
            
            // 创建函数值：包含函数指针和捕获变量
            char *temp = new_temp(gen);
            
            // 计算参数数量（隐式 self + 普通参数 + 捕获变量）
            int total_params = (int)func->param_count;
            if (func->uses_self) {
                total_params += 1;  // 隐式 self 参数
            }
            if (captured && captured->count > 0) {
                total_params += (int)captured->count;
            }
            
            if (captured && captured->count > 0) {
                // 有捕获变量：创建捕获变量数组
                char *captured_array = new_temp(gen);
                fprintf(gen->code_buf, "  %s = alloca [%zu x %%struct.Value*]  ; captured vars array\n",
                        captured_array, captured->count);
                
                // 检测自引用闭包：捕获变量中是否包含当前正在赋值的变量名
                int self_ref_index = -1;
                if (gen->current_var_name) {
                    if (getenv("DEBUG_CLOSURE")) {
                        fprintf(stderr, "[DEBUG CLOSURE] current_var_name=%s, checking captured vars:\n", gen->current_var_name);
                    }
                    for (size_t i = 0; i < captured->count; i++) {
                        if (getenv("DEBUG_CLOSURE")) {
                            fprintf(stderr, "[DEBUG CLOSURE]   captured[%zu]=%s, match=%d\n", 
                                    i, captured->names[i], 
                                    strcmp(captured->names[i], gen->current_var_name) == 0);
                        }
                        if (strcmp(captured->names[i], gen->current_var_name) == 0) {
                            self_ref_index = (int)i;
                            break;
                        }
                    }
                    if (getenv("DEBUG_CLOSURE")) {
                        fprintf(stderr, "[DEBUG CLOSURE] self_ref_index=%d\n", self_ref_index);
                    }
                }
                
                // 填充捕获变量
                for (size_t i = 0; i < captured->count; i++) {
                    char *captured_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                            captured_val, captured->names[i]);
                    char *slot_ptr = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 %zu\n",
                            slot_ptr, captured->count, captured->count, captured_array, i);
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n",
                            captured_val, slot_ptr);
                    free(captured_val);
                    free(slot_ptr);
                }
                
                // 获取数组指针
                char *captured_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 0\n",
                        captured_ptr, captured->count, captured->count, captured_array);
                
                // 获取函数指针
                char *func_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = bitcast %%struct.Value* (", func_ptr);
                // 函数签名：普通参数 + 捕获变量
                for (int i = 0; i < total_params; i++) {
                    if (i > 0) fprintf(gen->code_buf, ", ");
                    fprintf(gen->code_buf, "%%struct.Value*");
                }
                fprintf(gen->code_buf, ")* @%s to i8*\n", func->name);
                
                // 计算用户可见的参数数量（不包括隐式 self，运行时会自动处理）
                size_t user_param_count = func->param_count;
                
                // 调用 box_function，传递 needs_self 标志
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_function(i8* %s, %%struct.Value** %s, i32 %zu, i32 %zu, i32 %d)  ; closure '%s'\n",
                        temp, func_ptr, captured_ptr, captured->count, user_param_count, func->uses_self ? 1 : 0, func->name);
                
                // 自引用闭包修复：在创建闭包后更新捕获的自引用变量
                // 例如: f := (n) { f(n-1) }  -- 创建时 f 是 null，需要更新为实际函数
                if (self_ref_index >= 0) {
                    fprintf(gen->code_buf, "  ; Self-referencing closure fix: update captured[%d] to point to the closure itself\n", self_ref_index);
                    fprintf(gen->code_buf, "  call void @update_closure_captured(%%struct.Value* %s, i32 %d, %%struct.Value* %s)\n",
                            temp, self_ref_index, temp);
                }
                
                free(captured_array);
                free(captured_ptr);
                free(func_ptr);
            } else {
                // 无捕获变量：直接创建函数值
                char *func_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = bitcast %%struct.Value* (", func_ptr);
                // 函数签名：隐式 self（如果有）+ 普通参数
                for (int i = 0; i < total_params; i++) {
                    if (i > 0) fprintf(gen->code_buf, ", ");
                    fprintf(gen->code_buf, "%%struct.Value*");
                }
                fprintf(gen->code_buf, ")* @%s to i8*\n", func->name);
                
                // 计算用户可见的参数数量（不包括隐式 self，运行时会自动处理）
                size_t user_param_count = func->param_count;
                
                // 调用 box_function（captured 为 null），传递 needs_self 标志
                char *null_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = inttoptr i64 0 to %%struct.Value**\n", null_ptr);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_function(i8* %s, %%struct.Value** %s, i32 0, i32 %zu, i32 %d)  ; function '%s'\n",
                        temp, func_ptr, null_ptr, user_param_count, func->uses_self ? 1 : 0, func->name);
                
                free(func_ptr);
                free(null_ptr);
            }
            
            // 清理捕获变量结构
            if (captured) {
                captured_vars_free(captured);
            }
            
            return temp;
        }
        
        default:
            return NULL;
    }
}

/* ============================================================================
 * AST预扫描 - 收集需要在entry块alloca的变量
 * ============================================================================ */
