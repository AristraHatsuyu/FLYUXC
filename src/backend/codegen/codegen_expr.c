#include "codegen_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
            
            return temp;
        }
        
        case AST_BOOL_LITERAL: {
            ASTBoolLiteral *bl = (ASTBoolLiteral *)node->data;
            char *temp = new_temp(gen);
            
            // 使用 box_bool 将布尔值装箱为 Value*
            // LLVM i1/i32: false=0, true=1
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %d)\n", 
                    temp, bl->value ? 1 : 0);
            
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
            fprintf(gen->globals_buf, "%s = private unnamed_addr constant [%zu x i8] c\"%s\\00\"\n",
                    str_label, len + 1, escaped);
            
            // 获取指针
            fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x i8], [%zu x i8]* %s, i32 0, i32 0\n",
                    str_ptr, len + 1, len + 1, str_label);
            
            // 使用 box_string_with_length 装箱，传递显式长度支持\0字符串
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_string_with_length(i8* %s, i64 %zu)\n",
                    temp, str_ptr, len);
            
            free(escaped);
            free(str_label);
            free(str_ptr);
            return temp;
        }
        
        case AST_NULL_LITERAL: {
            char *temp = new_temp(gen);
            
            // 调用 box_null() 创建 null 值
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", temp);
            
            return temp;
        }
        
        case AST_UNDEF_LITERAL: {
            char *temp = new_temp(gen);
            
            // undef 使用 box_undef()
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()\n", temp);
            
            return temp;
        }
        
        case AST_IDENTIFIER: {
            ASTIdentifier *id = (ASTIdentifier *)node->data;
            char *temp = new_temp(gen);
            
            // 检查变量是否已定义
            if (!is_symbol_defined(gen, id->name)) {
                // 未定义的变量返回 undef
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_undef()  ; undef variable '%s'\n", 
                        temp, id->name);
                return temp;
            }
            
            // 加载变量值（现在是 Value*）
            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n", 
                    temp, id->name);
            
            return temp;
        }
        
        case AST_BINARY_EXPR: {
            ASTBinaryExpr *expr = (ASTBinaryExpr *)node->data;
            char *left = codegen_expr(gen, expr->left);
            char *right = codegen_expr(gen, expr->right);
            char *result = new_temp(gen);
            
            switch (expr->op) {
                case TK_PLUS:
                    // 使用 value_add 支持数字和字符串拼接
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_add(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    break;
                case TK_MINUS:
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_subtract(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    break;
                case TK_STAR:
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_multiply(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    break;
                case TK_SLASH:
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_divide(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    break;
                case TK_POWER:
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_power(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    break;
                case TK_LT:
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_less_than(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    break;
                case TK_GT:
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_greater_than(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    break;
                case TK_LE: {
                    // <= 使用 !(a > b)
                    char *gt = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_greater_than(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            gt, left, right);
                    // 对结果取反（使用 value_is_truthy 和 box_bool）
                    char *truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", truthy, gt);
                    char *inverted = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", inverted, truthy);
                    char *as_i32 = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", as_i32, inverted);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, as_i32);
                    free(gt);
                    free(truthy);
                    free(inverted);
                    free(as_i32);
                    break;
                }
                case TK_GE: {
                    // >= 使用 !(a < b)
                    char *lt = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_less_than(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            lt, left, right);
                    char *truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", truthy, lt);
                    char *inverted = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", inverted, truthy);
                    char *as_i32 = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", as_i32, inverted);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, as_i32);
                    free(lt);
                    free(truthy);
                    free(inverted);
                    free(as_i32);
                    break;
                }
                case TK_EQ_EQ:
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_equals(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            result, left, right);
                    break;
                case TK_BANG_EQ: {
                    // != 使用 !value_equals
                    char *eq = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_equals(%%struct.Value* %s, %%struct.Value* %s)\n", 
                            eq, left, right);
                    char *truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", truthy, eq);
                    char *inverted = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 0\n", inverted, truthy);
                    char *as_i32 = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = zext i1 %s to i32\n", as_i32, inverted);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_bool(i32 %s)\n", result, as_i32);
                    free(eq);
                    free(truthy);
                    free(inverted);
                    free(as_i32);
                    break;
                }
                case TK_AND_AND: {
                    // && : 逻辑与
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
                    free(left_truthy);
                    free(right_truthy);
                    free(left_bool);
                    free(right_bool);
                    free(or_result);
                    free(as_i32);
                    break;
                }
                default:
                    break;
            }
            
            free(left);
            free(right);
            return result;
        }
        
        case AST_CALL_EXPR: {
            ASTCallExpr *call = (ASTCallExpr *)node->data;
            
            // 确保callee是标识符
            if (!call->callee || call->callee->kind != AST_IDENTIFIER) {
                fprintf(stderr, "Error: callee is not an identifier\n");
                return NULL;
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
                return NULL;
            }
            
            // 特殊处理 println 函数（每个参数后换行）
            if (strcmp(callee->name, "println") == 0) {
                if (call->arg_count == 0) {
                    // println() 无参数时只输出换行
                    // 创建换行字符串常量
                    char *newline_label = new_string_label(gen);
                    fprintf(gen->globals_buf, "%s = private unnamed_addr constant [2 x i8] c\"\\0A\\00\"\n", newline_label);
                    fprintf(gen->code_buf, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* %s, i32 0, i32 0))\n", newline_label);
                    free(newline_label);
                } else {
                    for (size_t i = 0; i < call->arg_count; i++) {
                        char *arg = codegen_expr(gen, call->args[i]);
                        fprintf(gen->code_buf, "  call void @value_println(%%struct.Value* %s)\n", arg);
                        free(arg);
                    }
                }
                return NULL;
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
                return NULL;
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
                // 如果在 Try-Catch 中，什么都不做（让 Try-Catch 处理）
                
                return result;
            }
            
            if (strcmp(callee->name, "toStr") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_str(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            if (strcmp(callee->name, "toBl") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_bl(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            if (strcmp(callee->name, "toInt") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_int(%%struct.Value* %s)\n", result, arg);
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
                free(arg);
                return result;
            }
            
            // 字符串处理函数
            if (strcmp(callee->name, "len") == 0 && call->arg_count == 1) {
                char *arg = codegen_expr(gen, call->args[0]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_len(%%struct.Value* %s)\n", result, arg);
                free(arg);
                return result;
            }
            
            if (strcmp(callee->name, "charAt") == 0 && call->arg_count == 2) {
                char *str = codegen_expr(gen, call->args[0]);
                char *idx = codegen_expr(gen, call->args[1]);
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_char_at(%%struct.Value* %s, %%struct.Value* %s)\n", result, str, idx);
                free(str);
                free(idx);
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
            
            // time() - 获取Unix时间戳
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
            
            // 普通函数调用
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
            
            // 分配 ObjectEntry 数组
            char *entries_alloc = new_temp(gen);
            fprintf(gen->code_buf, "  %s = alloca [%zu x %%struct.ObjectEntry]  ; allocate object entries\n",
                    entries_alloc, prop_count);
            
            // 用于注册对象元数据的字段列表
            ObjectField *field_head = NULL;
            ObjectField *field_tail = NULL;
            
            // 为每个属性填充 ObjectEntry
            for (size_t i = 0; i < prop_count; i++) {
                ASTObjectProperty *prop = &obj_lit->properties[i];
                
                // 生成键字符串
                char *key_label = new_string_label(gen);
                size_t key_len = strlen(prop->key);
                fprintf(gen->globals_buf, "%s = private unnamed_addr constant [%zu x i8] c\"%s\\00\"\n",
                        key_label, key_len + 1, prop->key);
                
                char *key_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = getelementptr [%zu x i8], [%zu x i8]* %s, i32 0, i32 0\n",
                        key_ptr, key_len + 1, key_len + 1, key_label);
                
                // 计算属性值
                char *value = codegen_expr(gen, prop->value);
                
                // 获取 entry[i] 的指针
                char *entry_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = getelementptr [%zu x %%struct.ObjectEntry], [%zu x %%struct.ObjectEntry]* %s, i32 0, i32 %zu\n",
                        entry_ptr, prop_count, prop_count, entries_alloc, i);
                
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
            
            // 获取数组首地址
            char *entries_ptr = new_temp(gen);
            fprintf(gen->code_buf, "  %s = getelementptr [%zu x %%struct.ObjectEntry], [%zu x %%struct.ObjectEntry]* %s, i32 0, i32 0\n",
                    entries_ptr, prop_count, prop_count, entries_alloc);
            
            // 转换为 i8*
            char *entries_i8 = new_temp(gen);
            fprintf(gen->code_buf, "  %s = bitcast %%struct.ObjectEntry* %s to i8*\n",
                    entries_i8, entries_ptr);
            
            // 调用 box_object
            char *result = new_temp(gen);
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_object(i8* %s, i64 %zu)\n",
                    result, entries_i8, prop_count);
            
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
                
                // 检查是否是数组的 length 属性
                if (strcmp(member->property, "length") == 0) {
                    ArrayMetadata *arr_meta = find_array(gen, obj_name);
                    if (arr_meta) {
                        // 返回数组长度
                        char *result = new_temp(gen);
                        fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %zu.0)  ; %s.length\n",
                                result, arr_meta->elem_count, obj_name);
                        return result;
                    }
                }
                
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
            fprintf(gen->globals_buf, "%s = private unnamed_addr constant [%zu x i8] c\"%s\\00\"\n",
                    key_label, key_len + 1, member->property);
            
            char *key_ptr = new_temp(gen);
            fprintf(gen->code_buf, "  %s = getelementptr [%zu x i8], [%zu x i8]* %s, i32 0, i32 0\n",
                    key_ptr, key_len + 1, key_len + 1, key_label);
            
            char *field_name = new_temp(gen);
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_string(i8* %s)\n",
                    field_name, key_ptr);
            
            // 调用运行时函数获取字段值
            char *result = new_temp(gen);
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_get_field(%%struct.Value* %s, %%struct.Value* %s)  ; .%s\n",
                    result, obj_value, field_name, member->property);
            
            free(obj_value);
            free(key_label);
            free(key_ptr);
            free(field_name);
            return result;
        }
        
        case AST_INDEX_EXPR: {
            // 实现数组索引访问：arr[index]
            ASTIndexExpr *idx_expr = (ASTIndexExpr *)node->data;
            
            // 先尝试作为简单标识符处理（优化路径）
            if (idx_expr->object->kind == AST_IDENTIFIER) {
                ASTIdentifier *arr_ident = (ASTIdentifier *)idx_expr->object->data;
                const char *arr_name = arr_ident->name;
                
                // 查找数组元数据
                ArrayMetadata *meta = find_array(gen, arr_name);
                if (meta) {
                    // 是已知的静态数组，使用快速路径
                    // 计算索引值
                    char *index_val = codegen_expr(gen, idx_expr->index);
                    
                    // 将 Value* 索引转换为 i64（先 unbox 为 double）
                    char *index_double = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", 
                            index_double, index_val);
                    char *index_i64 = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = fptosi double %s to i64\n", index_i64, index_double);
                    
                    // 使用 getelementptr 获取元素地址（现在是 %struct.Value*）
                    char *elem_ptr = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 %s\n",
                            elem_ptr, meta->elem_count, meta->elem_count, meta->array_ptr, index_i64);
                    
                    // 加载并返回元素
                    char *elem = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s\n", elem, elem_ptr);
                    
                    free(index_val);
                    free(index_double);
                    free(index_i64);
                    free(elem_ptr);
                    
                    return elem;
                }
            }
            
            // 通用路径：对象是任意表达式，使用运行时函数
            char *obj_val = codegen_expr(gen, idx_expr->object);
            char *index_val = codegen_expr(gen, idx_expr->index);
            
            char *result = new_temp(gen);
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_index(%%struct.Value* %s, %%struct.Value* %s)\n",
                    result, obj_val, index_val);
            
            free(obj_val);
            free(index_val);
            
            return result;
        }
        
        default:
            return NULL;
    }
}

/* ============================================================================
 * AST预扫描 - 收集需要在entry块alloca的变量
 * ============================================================================ */
