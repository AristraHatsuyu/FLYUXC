#include "codegen_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * AST预扫描和语句代码生成
 * ============================================================================ */

void collect_catch_params(CodeGen *gen, ASTNode *node, FILE *entry_buf) {
    if (!node) return;
    
    switch (node->kind) {
        case AST_TRY_STMT: {
            ASTTryStmt *try_stmt = (ASTTryStmt *)node->data;
            if (try_stmt->catch_param && !is_symbol_defined(gen, try_stmt->catch_param)) {
                register_symbol(gen, try_stmt->catch_param);
                if (!is_ir_name_allocated(gen, try_stmt->catch_param)) {
                    fprintf(entry_buf, "  %%%s = alloca %%struct.Value*\n", try_stmt->catch_param);
                    mark_ir_name_allocated(gen, try_stmt->catch_param);
                }
            }
            // 递归扫描try/catch/finally块
            collect_catch_params(gen, try_stmt->try_block, entry_buf);
            collect_catch_params(gen, try_stmt->catch_block, entry_buf);
            collect_catch_params(gen, try_stmt->finally_block, entry_buf);
            break;
        }
        
        case AST_BLOCK: {
            ASTBlock *block = (ASTBlock *)node->data;
            for (size_t i = 0; i < block->stmt_count; i++) {
                collect_catch_params(gen, block->statements[i], entry_buf);
            }
            break;
        }
        
        case AST_IF_STMT: {
            ASTIfStmt *ifstmt = (ASTIfStmt *)node->data;
            for (size_t i = 0; i < ifstmt->cond_count; i++) {
                collect_catch_params(gen, ifstmt->then_blocks[i], entry_buf);
            }
            collect_catch_params(gen, ifstmt->else_block, entry_buf);
            break;
        }
        
        case AST_LOOP_STMT: {
            ASTLoopStmt *loop = (ASTLoopStmt *)node->data;
            collect_catch_params(gen, loop->body, entry_buf);
            break;
        }
        
        default:
            break;
    }
}

/* ============================================================================
 * 语句代码生成
 * ============================================================================ */

void codegen_stmt(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->kind) {
        case AST_VAR_DECL: {
            ASTVarDecl *decl = (ASTVarDecl *)node->data;
            
            if (getenv("DEBUG_CLOSURE")) {
                fprintf(stderr, "[DEBUG VAR_DECL] name=%s, init_expr=%p, kind=%d\n",
                        decl->name, (void*)decl->init_expr, 
                        decl->init_expr ? decl->init_expr->kind : -1);
            }
            
            // 检查是否在当前作用域重复声明
            if (is_symbol_defined_in_current_scope(gen, decl->name)) {
                // 同级重复声明是错误
                // 尝试获取原始变量名用于错误消息
                const char *original_name = codegen_lookup_original_name(gen, decl->name);
                codegen_set_error_at(gen, node->loc.orig_line, node->loc.orig_column, 
                                     node->loc.orig_length, 
                                     original_name ? original_name : decl->name,
                                     "Variable already declared in current scope");
                break;
            }
            
            // 检查是否遮蔽外层作用域的变量
            int shadows_outer = is_symbol_defined(gen, decl->name);
            
            // 注册变量并获取IR名称（如果遮蔽则生成唯一名称）
            const char *ir_name = register_symbol_with_shadow(gen, decl->name);
            
            // P2: 将变量添加到作用域跟踪器（使用IR名称）
            if (gen->scope) {
                scope_add_local(gen->scope, ir_name);
            }
            
            // Break清理：如果在循环中，也添加到循环作用域（使用IR名称）
            loop_scope_add_var(gen, ir_name);
            
            // 分配栈空间（使用IR名称）- 但只在未分配过时才分配
            if (!is_ir_name_allocated(gen, ir_name)) {
                FILE *alloca_target = gen->entry_alloca_buf ? gen->entry_alloca_buf : gen->code_buf;
                fprintf(alloca_target, "  %%%s = alloca %%struct.Value*\n", ir_name);
                // 初始化为 null，防止 release 时读到垃圾值
                fprintf(alloca_target, "  store %%struct.Value* null, %%struct.Value** %%%s\n", ir_name);
                mark_ir_name_allocated(gen, ir_name);
            }
            
            // 如果有初始化表达式
            if (decl->init_expr) {
                // 检查是否是带类型注解的null
                if (decl->type_annotation && decl->init_expr->kind == AST_NULL_LITERAL) {
                    // 需要用box_null_typed创建带声明类型的null
                    // 从type_annotation获取类型
                    int type_code = 5; // 默认VALUE_NULL
                    if (decl->type_annotation->kind == AST_TYPE_ANNOTATION) {
                        ASTTypeAnnotation *type_ann = (ASTTypeAnnotation *)decl->type_annotation->data;
                        // 映射类型token到VALUE_*常量
                        switch (type_ann->type_token) {
                            case TK_TYPE_NUM: type_code = 0; break;  // VALUE_NUMBER
                            case TK_TYPE_STR: type_code = 1; break;  // VALUE_STRING
                            case TK_TYPE_BL:  type_code = 4; break;  // VALUE_BOOL
                            case TK_TYPE_OBJ: type_code = 3; break;  // VALUE_OBJECT
                            default: type_code = 5; break;           // VALUE_NULL
                        }
                    }
                    
                    // 总是先释放旧值（循环中变量可能被多次赋值）
                    {
                        char *old_val = new_temp(gen);
                        char *check_label = new_label(gen);
                        char *release_label = new_label(gen);
                        fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                                old_val, ir_name);
                        // 检查是否为null
                        char *is_null = new_temp(gen);
                        fprintf(gen->code_buf, "  %s = icmp eq %%struct.Value* %s, null\n", is_null, old_val);
                        fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_null, check_label, release_label);
                        fprintf(gen->code_buf, "\n%s:\n", release_label);
                        fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", old_val);
                        fprintf(gen->code_buf, "  br label %%%s\n", check_label);
                        fprintf(gen->code_buf, "\n%s:\n", check_label);
                        free(old_val);
                        free(is_null);
                        free(check_label);
                        free(release_label);
                    }
                    
                    char *init_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null_typed(i32 %d)\n",
                            init_val, type_code);
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n",
                            init_val, ir_name);
                    // 释放中间值（null_typed 不是中间值，跳过）
                    temp_value_clear(gen);
                    free(init_val);
                } else {
                    // 普通初始化
                    gen->current_var_name = decl->name;
                    char *init_val = codegen_expr(gen, decl->init_expr);
                    gen->current_var_name = NULL;
                    
                    if (init_val) {
                        // 释放中间值，但保留要赋给变量的值
                        temp_value_release_except(gen, init_val);
                        
                        // 总是先释放旧值（如果非null）
                        // 这对于循环中的变量声明很重要，因为变量可能在前一次迭代被赋值
                        // 对于第一次赋值，变量是null，release(null)是空操作
                        {
                            char *old_val = new_temp(gen);
                            char *check_label = new_label(gen);
                            char *release_label = new_label(gen);
                            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                                    old_val, ir_name);
                            // 检查是否为null
                            char *is_null = new_temp(gen);
                            fprintf(gen->code_buf, "  %s = icmp eq %%struct.Value* %s, null\n", is_null, old_val);
                            fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", is_null, check_label, release_label);
                            fprintf(gen->code_buf, "\n%s:\n", release_label);
                            fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", old_val);
                            fprintf(gen->code_buf, "  br label %%%s\n", check_label);
                            fprintf(gen->code_buf, "\n%s:\n", check_label);
                            free(old_val);
                            free(is_null);
                            free(check_label);
                            free(release_label);
                        }
                        
                        // P2 Fix: 表达式结果已经遵循"调用者拥有"规则，不需要额外 retain
                        // 所有权直接转移给变量
                        fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n",
                                init_val, ir_name);
                        free(init_val);
                    } else {
                        temp_value_clear(gen);
                    }
                }
            }
            break;
        }
        
        case AST_ASSIGN_STMT: {
            ASTAssignStmt *assign = (ASTAssignStmt *)node->data;
            
            char *value = NULL;
            
            // 检查target的类型
            if (assign->target->kind == AST_IDENTIFIER) {
                // 简单变量赋值: x = 10
                ASTIdentifier *target = (ASTIdentifier *)assign->target->data;
                
                // 检查变量是否已定义
                int var_exists = is_symbol_defined(gen, target->name);
                
                // 获取变量的 IR 名称（考虑遮蔽）
                const char *ir_name;
                
                // 如果变量未定义，先分配空间并注册
                if (!var_exists) {
                    ir_name = register_symbol_with_shadow(gen, target->name);
                    
                    // 如果有entry_alloca_buf，写入到entry block；否则写入当前位置
                    // 但只在未分配过时才分配
                    if (!is_ir_name_allocated(gen, ir_name)) {
                        FILE *alloca_target = gen->entry_alloca_buf ? gen->entry_alloca_buf : gen->code_buf;
                        fprintf(alloca_target, "  %%%s = alloca %%struct.Value*\n", ir_name);
                        
                        // 初始化为 null，防止未初始化的变量被 release
                        fprintf(alloca_target, "  store %%struct.Value* null, %%struct.Value** %%%s\n", ir_name);
                        mark_ir_name_allocated(gen, ir_name);
                    }
                } else {
                    ir_name = get_symbol_ir_name(gen, target->name);
                }
                
                // 如果赋值的是null，需要保持变量的declared_type
                if (assign->value->kind == AST_NULL_LITERAL) {
                    // 先加载旧值获取declared_type，然后用box_null_preserve_type创建新null
                    char *old_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                            old_val, ir_name);
                    
                    // 释放旧值 (如果变量已存在)
                    if (var_exists) {
                        fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", old_val);
                    }
                    
                    char *new_null = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null_preserve_type(%%struct.Value* %s)\n",
                            new_null, old_val);
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n",
                            new_null, ir_name);
                    
                    free(old_val);
                    free(new_null);
                } else {
                    // 普通赋值 - 先释放旧值 (如果变量已存在)
                    if (var_exists) {
                        // 正确顺序：先计算新值，再释放旧值，最后存储
                        // 这样可以处理 x = x + 1 这种自引用的情况
                        gen->current_var_name = target->name;
                        value = codegen_expr(gen, assign->value);
                        gen->current_var_name = NULL;
                        if (!value) {
                            temp_value_clear(gen);
                            break;
                        }
                        
                        // 释放中间值，但保留要赋给变量的值
                        temp_value_release_except(gen, value);
                        
                        // 现在释放旧值（新值已经计算完毕，不再依赖旧值）
                        char *old_val = new_temp(gen);
                        fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                                old_val, ir_name);
                        fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", old_val);
                        free(old_val);
                        
                        // P2 Fix: 表达式结果已经遵循"调用者拥有"规则，不需要额外 retain
                        // 所有权直接转移给变量
                        fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n",
                                value, ir_name);
                    } else {
                        gen->current_var_name = target->name;
                        value = codegen_expr(gen, assign->value);
                        gen->current_var_name = NULL;
                        if (!value) {
                            temp_value_clear(gen);
                            break;
                        }
                        // 释放中间值，但保留要赋给变量的值
                        temp_value_release_except(gen, value);
                        // P2 Fix: 表达式结果已经遵循"调用者拥有"规则，不需要额外 retain
                        // 所有权直接转移给变量
                        fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n",
                                value, ir_name);
                    }
                }
            }
            else if (assign->target->kind == AST_INDEX_EXPR) {
                // 数组/对象索引赋值: arr[i] = 10 或 obj["key"] = 10 或 obj.arr[i] = 10
                value = codegen_expr(gen, assign->value);
                if (!value) {
                    temp_value_clear(gen);
                    break;
                }
                
                ASTIndexExpr *idx_expr = (ASTIndexExpr *)assign->target->data;
                
                // 获取对象/数组（支持任意表达式）
                char *obj_val = codegen_expr(gen, idx_expr->object);
                if (!obj_val) {
                    temp_value_clear(gen);
                    free(value);
                    break;
                }
                
                // 计算索引（支持数字和字符串）
                char *index_val = codegen_expr(gen, idx_expr->index);
                if (!index_val) {
                    temp_value_clear(gen);
                    free(obj_val);
                    free(value);
                    break;
                }
                
                // 调用运行时函数设置索引值
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_set_index(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n",
                        result, obj_val, index_val, value);
                
                // 注册 result 到临时值以便被清理
                temp_value_register(gen, result);
                
                // 释放中间值（result 是 set_index 返回的，需要释放；value 被存入数组/对象，保留）
                temp_value_release_except(gen, value);
                
                free(obj_val);
                free(index_val);
                free(result);
            }
            else if (assign->target->kind == AST_MEMBER_EXPR) {
                // 对象成员赋值: obj.prop = value 或 expr.prop = value
                // 先生成要赋的值
                value = codegen_expr(gen, assign->value);
                if (!value) {
                    temp_value_clear(gen);
                    break;
                }
                
                ASTMemberExpr *member = (ASTMemberExpr *)assign->target->data;
                
                // 获取对象 Value* （支持任意表达式）
                char *obj_var = codegen_expr(gen, member->object);
                if (!obj_var) {
                    temp_value_clear(gen);
                    free(value);
                    break;
                }
                
                // 创建字段名字符串
                char *key_label = new_string_label(gen);
                size_t key_len = strlen(member->property);
                fprintf(gen->strings_buf, "%s = private unnamed_addr constant [%zu x i8] c\"%s\\00\"\n",
                        key_label, key_len + 1, member->property);
                
                char *key_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = getelementptr [%zu x i8], [%zu x i8]* %s, i32 0, i32 0\n",
                        key_ptr, key_len + 1, key_len + 1, key_label);
                
                char *key_value = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_string(i8* %s)\n",
                        key_value, key_ptr);
                temp_value_register(gen, key_value);  // 注册 key_value
                
                // 调用 value_set_field
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_set_field(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n",
                        result, obj_var, key_value, value);
                temp_value_register(gen, result);  // 注册 result
                
                // 释放中间值（key_value 和 result 是临时创建的，需要释放）
                temp_value_release_except(gen, value);
                
                free(obj_var);
                free(key_label);
                free(key_ptr);
                free(key_value);
                free(result);
            }
            else {
                fprintf(gen->code_buf, "  ; ERROR: unsupported assignment target\n");
                temp_value_clear(gen);
            }
            
            if (value) free(value);
            break;
        }
        
        case AST_RETURN_STMT: {
            ASTReturnStmt *ret = (ASTReturnStmt *)node->data;
            
            if (ret->value) {
                char *ret_val = codegen_expr(gen, ret->value);
                if (ret_val) {
                    // 释放中间值，但保留返回值
                    temp_value_release_except(gen, ret_val);
                    
                    // P2: 返回值保护 - 在清理前 retain 返回值
                    fprintf(gen->code_buf, "  ; P2: protect return value\n");
                    fprintf(gen->code_buf, "  call void @value_retain(%%struct.Value* %s)\n", ret_val);
                    
                    // P2: 作用域清理 - 释放所有局部变量
                    if (gen->scope) {
                        scope_generate_cleanup(gen, gen->scope);
                    }
                    
                    fprintf(gen->code_buf, "  ret %%struct.Value* %s\n", ret_val);
                    gen->block_terminated = 1;  // 标记基本块已终止
                    free(ret_val);
                } else {
                    // 没有返回值表达式结果，释放所有临时值
                    temp_value_release_all(gen);
                    char *null_ret = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", null_ret);
                    
                    // P2: 作用域清理
                    if (gen->scope) {
                        scope_generate_cleanup(gen, gen->scope);
                    }
                    
                    fprintf(gen->code_buf, "  ret %%struct.Value* %s\n", null_ret);
                    gen->block_terminated = 1;
                    free(null_ret);
                }
            } else {
                // 无返回值的 return，释放所有临时值
                temp_value_release_all(gen);
                char *null_ret = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", null_ret);
                
                // P2: 作用域清理
                if (gen->scope) {
                    scope_generate_cleanup(gen, gen->scope);
                }
                
                fprintf(gen->code_buf, "  ret %%struct.Value* %s\n", null_ret);
                gen->block_terminated = 1;
                free(null_ret);
            }
            break;
        }
        
        case AST_BREAK_STMT: {
            ASTBreakStmt *break_stmt = (ASTBreakStmt *)node->data;
            const char *target_label = break_stmt ? break_stmt->target_label : NULL;
            
            if (target_label) {
                // 多级 break: B> label
                const char *break_label = loop_scope_find_break_label(gen, target_label);
                if (break_label) {
                    // 先清理当前积累的临时值（防止跳转后泄漏）
                    temp_value_release_all(gen);
                    // 清理从当前到目标循环的所有作用域
                    loop_scope_generate_multilevel_break_cleanup(gen, target_label);
                    // 跳转到目标循环的结束标签
                    fprintf(gen->code_buf, "  br label %%%s\n", break_label);
                    gen->block_terminated = 1;
                } else {
                    fprintf(stderr, "Error: undefined loop label '%s' in break statement\n", target_label);
                }
            } else if (gen->loop_end_label) {
                // 普通 break: B>（跳出当前循环）
                // 先清理当前积累的临时值（防止跳转后泄漏）
                temp_value_release_all(gen);
                loop_scope_generate_break_cleanup(gen);
                fprintf(gen->code_buf, "  br label %%%s\n", gen->loop_end_label);
                gen->block_terminated = 1;
            } else {
                fprintf(stderr, "Error: break statement outside of loop\n");
            }
            break;
        }
        
        case AST_NEXT_STMT: {
            ASTNextStmt *next_stmt = (ASTNextStmt *)node->data;
            const char *target_label = next_stmt ? next_stmt->target_label : NULL;
            
            if (target_label) {
                // 多级 next: N> label
                const char *continue_label = loop_scope_find_continue_label(gen, target_label);
                if (continue_label) {
                    // 先清理当前积累的临时值（防止跳转后泄漏）
                    temp_value_release_all(gen);
                    // 清理从当前到目标循环的作用域（不包括目标循环本身）
                    loop_scope_generate_multilevel_next_cleanup(gen, target_label);
                    // 跳转到目标循环的继续标签
                    fprintf(gen->code_buf, "  br label %%%s\n", continue_label);
                    gen->block_terminated = 1;
                } else {
                    fprintf(stderr, "Error: undefined loop label '%s' in next statement\n", target_label);
                }
            } else if (gen->loop_continue_label) {
                // 普通 next: N>（继续当前循环）
                // 先清理当前积累的临时值（防止跳转后泄漏）
                temp_value_release_all(gen);
                loop_scope_generate_next_cleanup(gen);
                fprintf(gen->code_buf, "  br label %%%s\n", gen->loop_continue_label);
                gen->block_terminated = 1;
            } else {
                fprintf(stderr, "Error: next statement outside of loop\n");
            }
            break;
        }
        
        case AST_TRY_STMT: {
            ASTTryStmt *try_stmt = (ASTTryStmt *)node->data;
            
            // 标记进入 Try-Catch 块
            int old_in_try_catch = gen->in_try_catch;
            char *old_try_catch_label = gen->try_catch_label;
            gen->in_try_catch = 1;
            
            // catch参数应该已经在函数开头alloca过了（通过预扫描）
            // 这里只需要确保它被注册到符号表
            if (try_stmt->catch_param && try_stmt->catch_block) {
                if (!is_symbol_defined(gen, try_stmt->catch_param)) {
                    register_symbol(gen, try_stmt->catch_param);
                }
            }
            
            // 清除错误状态，准备执行try块
            fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
            
            // 创建标签
            char *catch_label = new_label(gen);
            char *finally_label = new_label(gen);
            char *end_label = new_label(gen);
            
            // 设置当前的catch标签，供表达式代码生成使用
            gen->try_catch_label = catch_label;
            
            // 执行try块（需要在每条语句后检查错误）
            if (try_stmt->try_block && try_stmt->try_block->kind == AST_BLOCK) {
                ASTBlock *block = (ASTBlock *)try_stmt->try_block->data;
                
                // 进入try块的作用域
                scope_enter(gen);
                
                // 遍历try块中的每条语句
                for (size_t i = 0; i < block->stmt_count; i++) {
                    // 执行语句
                    codegen_stmt(gen, block->statements[i]);
                    
                    // 检查错误状态
                    char *ok_result = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", ok_result);
                    
                    char *truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                            truthy, ok_result);
                    
                    char *is_ok = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", is_ok, truthy);
                    
                    // 如果有错误，跳转到catch（如果有）或finally
                    char *continue_label = new_label(gen);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", 
                            is_ok, continue_label,
                            try_stmt->catch_block ? catch_label : 
                            (try_stmt->finally_block ? finally_label : end_label));
                    fprintf(gen->code_buf, "%s:\n", continue_label);
                    
                    free(ok_result);
                    free(truthy);
                    free(is_ok);
                    free(continue_label);
                }
                
                // 退出try块的作用域
                scope_generate_exit_cleanup(gen);
                scope_exit(gen);
            }
            
            // try块正常结束，跳转到finally或end
            fprintf(gen->code_buf, "  br label %%%s\n", 
                    try_stmt->finally_block ? finally_label : end_label);
            
            // catch块
            if (try_stmt->catch_block) {
                fprintf(gen->code_buf, "%s:\n", catch_label);
                
                // 创建错误对象
                if (try_stmt->catch_param) {
                    // 1. 获取错误消息
                    char *error_msg_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_last_error()\n", error_msg_val);
                    
                    // 2. 获取错误码
                    char *error_code_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_last_status()\n", error_code_val);
                    
                    // 3. 创建类型字符串
                    char *error_type = new_temp(gen);
                    char *status_num = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n", status_num, error_code_val);
                    char *status_int = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = fptosi double %s to i32\n", status_int, status_num);
                    
                    // 根据错误码创建类型名称
                    char *is_type_error = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp eq i32 %s, 3\n", is_type_error, status_int); // 3 = TYPE_ERROR
                    
                    char *type_str_ptr = new_temp(gen);
                    char *type_error_str = new_temp(gen);
                    char *generic_error_str = new_temp(gen);
                    
                    fprintf(gen->code_buf, "  %s = getelementptr [10 x i8], [10 x i8]* @str_type_error, i32 0, i32 0\n", type_error_str);
                    fprintf(gen->code_buf, "  %s = getelementptr [6 x i8], [6 x i8]* @str_error, i32 0, i32 0\n", generic_error_str);
                    fprintf(gen->code_buf, "  %s = select i1 %s, i8* %s, i8* %s\n", 
                            type_str_ptr, is_type_error, type_error_str, generic_error_str);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_string(i8* %s)\n", error_type, type_str_ptr);
                    
                    // 4. 使用运行时函数创建错误对象
                    char *error_obj = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @create_error_object(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n",
                            error_obj, error_msg_val, error_code_val, error_type);
                    
                    // 5. 存储到catch参数变量
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n", 
                            error_obj, try_stmt->catch_param);
                    
                    // 清理临时变量
                    free(error_msg_val);
                    free(error_code_val);
                    free(error_type);
                    free(status_num);
                    free(status_int);
                    free(is_type_error);
                    free(type_str_ptr);
                    free(type_error_str);
                    free(generic_error_str);
                    free(error_obj);
                }
                
                // 清除错误状态（catch已处理）
                fprintf(gen->code_buf, "  call %%struct.Value* @value_clear_error()\n");
                
                // 进入catch块的作用域
                scope_enter(gen);
                
                // 执行catch块
                codegen_stmt(gen, try_stmt->catch_block);
                
                // 退出catch块的作用域
                scope_generate_exit_cleanup(gen);
                scope_exit(gen);
                
                // catch执行完毕，跳转到finally或end
                fprintf(gen->code_buf, "  br label %%%s\n", 
                        try_stmt->finally_block ? finally_label : end_label);
            }
            
            // finally块
            if (try_stmt->finally_block) {
                fprintf(gen->code_buf, "%s:\n", finally_label);
                
                // 进入finally块的作用域
                scope_enter(gen);
                
                codegen_stmt(gen, try_stmt->finally_block);
                
                // 退出finally块的作用域
                scope_generate_exit_cleanup(gen);
                scope_exit(gen);
                
                fprintf(gen->code_buf, "  br label %%%s\n", end_label);
            }
            
            // 结束标签
            fprintf(gen->code_buf, "%s:\n", end_label);
            
            // 恢复 Try-Catch 标志和标签
            gen->in_try_catch = old_in_try_catch;
            gen->try_catch_label = old_try_catch_label;
            
            free(catch_label);
            free(finally_label);
            free(end_label);
            
            break;
        }
        
        case AST_IF_STMT: {
            ASTIfStmt *ifstmt = (ASTIfStmt *)node->data;
            
            if (ifstmt->cond_count > 0) {
                char *end_label = new_label(gen);
                
                // 处理每个条件（else-if 链）
                for (size_t i = 0; i < ifstmt->cond_count; i++) {
                    // 评估条件
                    char *cond = codegen_expr(gen, ifstmt->conditions[i]);
                    char *truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                            truthy, cond);
                    char *cond_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", cond_bool, truthy);
                    
                    char *then_label = new_label(gen);
                    char *next_label = new_label(gen);
                    
                    // 如果条件为真，跳转到 then 块，否则跳转到下一个条件
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n",
                            cond_bool, then_label, next_label);
                    
                    // Then 块
                    fprintf(gen->code_buf, "\n%s:\n", then_label);
                    gen->block_terminated = 0;  // 重置块终止标志
                    
                    // 进入新作用域
                    scope_enter(gen);
                    
                    if (ifstmt->then_blocks[i]) {
                        codegen_stmt(gen, ifstmt->then_blocks[i]);
                    }
                    
                    // 退出作用域前生成清理代码
                    if (!gen->block_terminated) {
                        scope_generate_exit_cleanup(gen);
                    }
                    scope_exit(gen);
                    
                    // 只有在块未终止时才生成跳转
                    if (!gen->block_terminated) {
                        fprintf(gen->code_buf, "  br label %%%s\n", end_label);
                    }
                    gen->block_terminated = 0;  // 重置供下一个分支使用
                    
                    // 下一个条件标签（else-if 或 else）
                    fprintf(gen->code_buf, "\n%s:\n", next_label);
                    
                    free(cond);
                    free(truthy);
                    free(cond_bool);
                    free(then_label);
                    free(next_label);
                }
                
                // 如果有 else 块，在这里执行
                if (ifstmt->else_block) {
                    gen->block_terminated = 0;
                    
                    // 进入新作用域
                    scope_enter(gen);
                    
                    codegen_stmt(gen, ifstmt->else_block);
                    
                    // 退出作用域前生成清理代码
                    if (!gen->block_terminated) {
                        scope_generate_exit_cleanup(gen);
                    }
                    scope_exit(gen);
                }
                // 只有在块未终止时才生成跳转
                if (!gen->block_terminated) {
                    fprintf(gen->code_buf, "  br label %%%s\n", end_label);
                }
                gen->block_terminated = 0;  // 重置
                
                // End 标签
                fprintf(gen->code_buf, "\n%s:\n", end_label);
                
                free(end_label);
            }
            break;
        }
        
        case AST_BLOCK: {
            ASTBlock *block = (ASTBlock *)node->data;
            // 注意：AST_BLOCK 本身不创建新作用域，
            // 作用域由包含它的语句（if、L>、T>等）管理
            for (size_t i = 0; i < block->stmt_count; i++) {
                codegen_stmt(gen, block->statements[i]);
            }
            break;
        }
        
        case AST_LOOP_STMT: {
            ASTLoopStmt *loop = (ASTLoopStmt *)node->data;
            
            if (loop->loop_type == LOOP_REPEAT) {
                // 重复循环: L> [n] { body }
                // 转换为: i=0; while(i<n) { body; i++; }
                char *loop_counter = new_temp(gen);
                char *loop_limit = codegen_expr(gen, loop->loop_data.repeat_count);
                
                // 循环控制表达式不应该作为中间值释放，清空栈
                temp_value_clear(gen);
                
                // 分配计数器变量
                fprintf(gen->code_buf, "  %s_var = alloca %%struct.Value*\n", loop_counter);
                
                // 初始化计数器为 0
                char *zero = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double 0.0)\n", zero);
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s_var\n", zero, loop_counter);
                free(zero);
                
                char *loop_header = new_label(gen);
                char *loop_body = new_label(gen);
                char *loop_update = new_label(gen);  // next 跳转目标
                char *loop_end = new_label(gen);
                
                // 跳转到条件检查
                fprintf(gen->code_buf, "  br label %%%s\n", loop_header);
                fprintf(gen->code_buf, "\n%s:\n", loop_header);
                
                // 条件: i < n
                char *counter_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s_var\n", counter_val, loop_counter);
                char *cond_result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_less_than(%%struct.Value* %s, %%struct.Value* %s)\n",
                        cond_result, counter_val, loop_limit);
                
                char *truthy = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", truthy, cond_result);
                char *cond_bool = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", cond_bool, truthy);
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", cond_bool, loop_body, loop_end);
                
                free(counter_val);
                free(cond_result);
                free(truthy);
                free(cond_bool);
                
                // 循环体
                fprintf(gen->code_buf, "\n%s:\n", loop_body);
                
                // 保存并设置 loop_end_label/loop_continue_label，同时创建循环作用域
                char *old_loop_end_repeat = gen->loop_end_label;
                char *old_loop_continue_repeat = gen->loop_continue_label;
                gen->loop_end_label = loop_end;
                gen->loop_continue_label = loop_update;  // next 跳到更新部分
                loop_scope_push(gen, loop_end, loop_update, loop->label);
                
                if (loop->body) {
                    codegen_stmt(gen, loop->body);
                }
                
                // 退出循环作用域，恢复标签
                loop_scope_pop(gen);
                gen->loop_end_label = old_loop_end_repeat;
                gen->loop_continue_label = old_loop_continue_repeat;
                
                // 跳转到更新部分（正常流程或 next 后）
                if (!gen->block_terminated) {
                    fprintf(gen->code_buf, "  br label %%%s\n", loop_update);
                }
                gen->block_terminated = 0;
                
                // 更新部分: i++
                fprintf(gen->code_buf, "\n%s:\n", loop_update);
                char *old_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s_var\n", old_val, loop_counter);
                char *one = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double 1.0)\n", one);
                char *new_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_add(%%struct.Value* %s, %%struct.Value* %s)\n",
                        new_val, old_val, one);
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s_var\n", new_val, loop_counter);
                fprintf(gen->code_buf, "  br label %%%s\n", loop_header);
                
                free(old_val);
                free(one);
                free(new_val);
                
                // 结束
                fprintf(gen->code_buf, "\n%s:\n", loop_end);
                
                free(loop_counter);
                free(loop_limit);
                free(loop_header);
                free(loop_body);
                free(loop_update);
                free(loop_end);
                
            } else if (loop->loop_type == LOOP_FOREACH) {
                // foreach循环: L> (array : item) { body }
                // 进入循环作用域（用于循环变量）
                scope_enter(gen);
                
                char *array = codegen_expr(gen, loop->loop_data.foreach_loop.iterable);
                char *item_var = loop->loop_data.foreach_loop.item_var;
                
                // 循环控制表达式不应该作为中间值释放，清空栈
                temp_value_clear(gen);
                
                // 注册循环变量（在当前循环作用域中）
                // 使用 register_symbol_with_shadow 支持遮蔽外层同名变量
                const char *item_ir_name = register_symbol_with_shadow(gen, item_var);
                
                // 如果有entry_alloca_buf，写入到entry block；否则写入当前位置
                // 但只在未分配过时才分配
                if (!is_ir_name_allocated(gen, item_ir_name)) {
                    FILE *alloca_target = gen->entry_alloca_buf ? gen->entry_alloca_buf : gen->code_buf;
                    fprintf(alloca_target, "  %%%s = alloca %%struct.Value*\n", item_ir_name);
                    fprintf(alloca_target, "  store %%struct.Value* null, %%struct.Value** %%%s\n", item_ir_name);
                    mark_ir_name_allocated(gen, item_ir_name);
                }
                
                // 获取数组长度
                char *length = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i64 @value_array_length(%%struct.Value* %s)\n", length, array);
                
                // 循环索引
                char *index_var = new_temp(gen);
                fprintf(gen->code_buf, "  %s_var = alloca i64\n", index_var);
                fprintf(gen->code_buf, "  store i64 0, i64* %s_var\n", index_var);
                
                char *loop_header = new_label(gen);
                char *loop_body = new_label(gen);
                char *loop_update = new_label(gen);  // next 跳转目标
                char *loop_end = new_label(gen);
                
                // 跳转到条件检查
                fprintf(gen->code_buf, "  br label %%%s\n", loop_header);
                fprintf(gen->code_buf, "\n%s:\n", loop_header);
                
                // 条件: index < length
                char *index_val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load i64, i64* %s_var\n", index_val, index_var);
                char *cond = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp slt i64 %s, %s\n", cond, index_val, length);
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n", cond, loop_body, loop_end);
                
                free(index_val);
                free(cond);
                
                // 循环体
                fprintf(gen->code_buf, "\n%s:\n", loop_body);
                
                // 获取当前元素: item = array[index]
                char *curr_index = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load i64, i64* %s_var\n", curr_index, index_var);
                char *index_boxed = new_temp(gen);
                fprintf(gen->code_buf, "  %s = sitofp i64 %s to double\n", index_boxed, curr_index);
                char *index_value = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %s)\n", index_value, index_boxed);
                char *element = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_array_get(%%struct.Value* %s, %%struct.Value* %s)\n",
                        element, array, index_value);
                // P2: for-each 循环变量需要 retain
                fprintf(gen->code_buf, "  call %%struct.Value* @value_retain(%%struct.Value* %s)\n", element);
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n", element, item_ir_name);
                
                free(curr_index);
                free(index_boxed);
                free(index_value);
                free(element);
                
                // 保存并设置 loop_end_label/loop_continue_label，同时创建循环作用域
                char *old_loop_end_foreach = gen->loop_end_label;
                char *old_loop_continue_foreach = gen->loop_continue_label;
                gen->loop_end_label = loop_end;
                gen->loop_continue_label = loop_update;
                loop_scope_push(gen, loop_end, loop_update, loop->label);
                
                // 执行循环体
                if (loop->body) {
                    codegen_stmt(gen, loop->body);
                }
                
                // 退出循环作用域，恢复标签
                loop_scope_pop(gen);
                gen->loop_end_label = old_loop_end_foreach;
                gen->loop_continue_label = old_loop_continue_foreach;
                
                // 跳转到更新部分（正常流程或 next 后）
                if (!gen->block_terminated) {
                    fprintf(gen->code_buf, "  br label %%%s\n", loop_update);
                }
                gen->block_terminated = 0;
                
                // 更新部分: index++
                fprintf(gen->code_buf, "\n%s:\n", loop_update);
                char *old_index = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load i64, i64* %s_var\n", old_index, index_var);
                char *new_index = new_temp(gen);
                fprintf(gen->code_buf, "  %s = add i64 %s, 1\n", new_index, old_index);
                fprintf(gen->code_buf, "  store i64 %s, i64* %s_var\n", new_index, index_var);
                fprintf(gen->code_buf, "  br label %%%s\n", loop_header);
                
                free(old_index);
                free(new_index);
                
                // 结束
                fprintf(gen->code_buf, "\n%s:\n", loop_end);
                
                // 退出循环作用域（清理循环变量）
                scope_generate_exit_cleanup(gen);
                scope_exit(gen);
                
                free(array);
                free(length);
                free(index_var);
                free(loop_header);
                free(loop_body);
                free(loop_update);
                free(loop_end);
                
            } else if (loop->loop_type == LOOP_FOR) {
                // for循环: init; cond; update { body }
                // 进入循环作用域（用于循环变量）
                scope_enter(gen);
                
                char *loop_header = new_label(gen);
                char *loop_body = new_label(gen);
                char *loop_update = new_label(gen);
                char *loop_end = new_label(gen);
                
                // 初始化（循环变量在循环作用域内）
                if (loop->loop_data.for_loop.init) {
                    codegen_stmt(gen, loop->loop_data.for_loop.init);
                }
                
                // 初始化表达式不应该作为中间值释放，清空栈
                temp_value_clear(gen);
                
                // 跳转到条件检查
                fprintf(gen->code_buf, "  br label %%%s\n", loop_header);
                fprintf(gen->code_buf, "\n%s:\n", loop_header);
                
                // 条件判断
                if (loop->loop_data.for_loop.condition) {
                    char *cond = codegen_expr(gen, loop->loop_data.for_loop.condition);
                    
                    // 使用 value_is_truthy 将 Value* 转换为 i32
                    char *truthy = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                            truthy, cond);
                    
                    // 转换为 i1
                    char *cond_bool = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", cond_bool, truthy);
                    
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n",
                            cond_bool, loop_body, loop_end);
                    free(cond);
                    free(truthy);
                    free(cond_bool);
                } else {
                    // 无条件则一直循环
                    fprintf(gen->code_buf, "  br label %%%s\n", loop_body);
                }
                
                // 循环体
                fprintf(gen->code_buf, "\n%s:\n", loop_body);
                
                // 保存并设置 loop_end_label/loop_continue_label，同时创建循环作用域
                char *old_loop_end_for = gen->loop_end_label;
                char *old_loop_continue_for = gen->loop_continue_label;
                gen->loop_end_label = loop_end;
                gen->loop_continue_label = loop_update;
                loop_scope_push(gen, loop_end, loop_update, loop->label);
                
                if (loop->body) {
                    codegen_stmt(gen, loop->body);
                }
                
                // 退出循环作用域，恢复标签
                loop_scope_pop(gen);
                gen->loop_end_label = old_loop_end_for;
                gen->loop_continue_label = old_loop_continue_for;
                
                // 跳转到更新部分（正常流程或 next 后）
                if (!gen->block_terminated) {
                    fprintf(gen->code_buf, "  br label %%%s\n", loop_update);
                }
                gen->block_terminated = 0;
                
                // 更新
                fprintf(gen->code_buf, "\n%s:\n", loop_update);
                if (loop->loop_data.for_loop.update) {
                    // update 可能是表达式或语句，尝试两种方式
                    if (loop->loop_data.for_loop.update->kind == AST_EXPR_STMT ||
                        loop->loop_data.for_loop.update->kind == AST_VAR_DECL ||
                        loop->loop_data.for_loop.update->kind == AST_ASSIGN_STMT) {
                        codegen_stmt(gen, loop->loop_data.for_loop.update);
                    } else {
                        // 作为表达式处理（如 i++）
                        char *result = codegen_expr(gen, loop->loop_data.for_loop.update);
                        if (result) free(result);
                    }
                }
                fprintf(gen->code_buf, "  br label %%%s\n", loop_header);
                
                // 结束
                fprintf(gen->code_buf, "\n%s:\n", loop_end);
                
                // 退出循环作用域（清理循环变量）
                scope_generate_exit_cleanup(gen);
                scope_exit(gen);
                
                free(loop_header);
                free(loop_body);
                free(loop_update);
                free(loop_end);
            }
            break;
        }
        
        case AST_EXPR_STMT: {
            ASTExprStmt *stmt = (ASTExprStmt *)node->data;
            char *result = codegen_expr(gen, stmt->expr);
            if (result) {
                // 释放所有中间值（但保留最终结果）
                temp_value_release_except(gen, result);
                // 表达式语句的结果不需要保留，释放它
                // 这处理了像 "test".>print 这样的表达式，防止内存泄漏
                fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", result);
                free(result);
            } else {
                // 即使没有结果，也清空中间值栈
                temp_value_clear(gen);
            }
            break;
        }
        
        case AST_FUNC_DECL: {
            ASTFuncDecl *func = (ASTFuncDecl *)node->data;
            
            if (getenv("DEBUG_CLOSURE")) {
                fprintf(stderr, "[DEBUG FUNC_DECL] name=%s in codegen_stmt\n", func->name);
            }
            
            // 注意：函数名已在 codegen_generate 中预注册（用于支持前向引用）
            // 对于嵌套函数，需要在这里注册
            // 但是：匿名函数（_anon_*）不应注册到符号表，因为它们是全局函数，
            // 没有对应的 alloca，会导致 scope_generate_exit_cleanup 生成无效代码
            if (!is_symbol_defined(gen, func->name) && strncmp(func->name, "_anon_", 6) != 0) {
                register_symbol(gen, func->name);
            }
            
            // 特殊处理: main函数需要重命名为_flyux_main,避免与LLVM入口冲突
            const char *func_llvm_name = func->name;
            if (strcmp(func->name, "main") == 0) {
                func_llvm_name = "_flyux_main";
            }
            
            // 检测是否是嵌套函数（在另一个函数内部定义）
            // 如果 entry_alloca_buf 非空，说明我们在一个函数体内
            int is_nested = (gen->entry_alloca_buf != NULL);
            
            // 为了避免嵌套函数定义交错，始终使用临时缓冲区收集函数定义
            // 然后在完成后追加到目标位置
            FILE *func_output_buf = tmpfile();  // 用于收集整个函数定义
            FILE *output_target = func_output_buf;  // 函数定义先写入临时缓冲区
            FILE *final_target;
            
            if (is_nested) {
                // 嵌套函数：最终写入全局区域
                final_target = gen->globals_buf;
            } else {
                // 顶层函数：最终写入当前代码缓冲区
                final_target = gen->code_buf;
            }
            
            // 获取或分析闭包捕获的变量
            CapturedVars *captured = gen->current_captured;
            CapturedVars *own_captured = NULL;  // 如果我们自己分析，需要在最后释放
            
            // 如果是嵌套函数且没有预设的捕获变量，需要自己分析
            if (is_nested && captured == NULL) {
                own_captured = analyze_captured_vars(gen, func->body, 
                                                     func->params, func->param_count);
                captured = own_captured;
                
                // 检查函数是否使用了 self 关键字
                func->uses_self = closure_analysis_uses_self();
            }
            
            // 函数签名 - 使用 Value* 类型
            fprintf(output_target, "\ndefine %%struct.Value* @%s(", func_llvm_name);
            
            // 如果函数使用了 self，添加隐式 self 参数作为第一个参数
            int param_idx = 0;
            if (func->uses_self) {
                fprintf(output_target, "%%struct.Value* %%param_self");
                param_idx = 1;
            }
            
            // 输出普通参数
            for (size_t i = 0; i < func->param_count; i++) {
                if (param_idx > 0) fprintf(output_target, ", ");
                fprintf(output_target, "%%struct.Value* %%param_%s", func->params[i]);
                param_idx++;
            }
            
            // 然后输出捕获的变量作为额外参数
            if (captured && captured->count > 0) {
                for (size_t i = 0; i < captured->count; i++) {
                    if (param_idx > 0) fprintf(output_target, ", ");
                    fprintf(output_target, "%%struct.Value* %%captured_%s", captured->names[i]);
                    param_idx++;
                }
            }
            
            fprintf(output_target, ") {\n");
            
            // P2: 创建函数作用域跟踪器
            ScopeTracker *func_scope = scope_tracker_create();
            
            // 创建缓冲区用于收集 alloca 和函数体代码
            FILE *func_entry_alloca_buf = tmpfile();
            FILE *func_body_buf = tmpfile();
            
            // 保存当前状态
            FILE *saved_code_buf = gen->code_buf;
            FILE *saved_entry_alloca = gen->entry_alloca_buf;
            ScopeTracker *saved_scope = gen->scope;
            int saved_block_terminated = gen->block_terminated;
            SymbolEntry *saved_symbols = gen->symbols;  // 保存符号表
            AllocatedIRName *saved_allocated_ir_names = gen->allocated_ir_names;  // 保存已分配名称
            TempValueStack *saved_temp_values = gen->temp_values;  // 保存临时值栈
            gen->allocated_ir_names = NULL;  // 新函数开始，清空已分配名称
            gen->temp_values = NULL;  // 新函数使用新的临时值栈
            
            // 注意：不清空符号表！顶层预注册的函数名需要在函数体内可见
            // 函数内的局部变量会覆盖同名的全局符号
            
            // 如果使用 self，为 self 创建局部变量
            if (func->uses_self) {
                fprintf(output_target, "  %%self = alloca %%struct.Value*\n");
                fprintf(output_target, "  store %%struct.Value* %%param_self, %%struct.Value** %%self\n");
                register_symbol(gen, "self");
                mark_ir_name_allocated(gen, "self");
            }
            
            // 为参数创建局部变量（注意：必须在清空符号表之后）
            for (size_t i = 0; i < func->param_count; i++) {
                fprintf(output_target, "  %%%s = alloca %%struct.Value*\n", func->params[i]);
                fprintf(output_target, "  store %%struct.Value* %%param_%s, %%struct.Value** %%%s\n",
                        func->params[i], func->params[i]);
                // 注册参数到符号表，使其在函数体内可见
                register_symbol(gen, func->params[i]);
                mark_ir_name_allocated(gen, func->params[i]);
                // P2 修复：参数不添加到作用域跟踪器！
                // 参数的生命周期由调用者管理，函数不应该释放参数
                // scope_add_local(func_scope, func->params[i]); // 移除
            }
            
            // 为捕获的变量创建局部变量
            if (captured && captured->count > 0) {
                for (size_t i = 0; i < captured->count; i++) {
                    fprintf(output_target, "  %%%s = alloca %%struct.Value*\n", captured->names[i]);
                    fprintf(output_target, "  store %%struct.Value* %%captured_%s, %%struct.Value** %%%s\n",
                            captured->names[i], captured->names[i]);
                    // 注册捕获变量到符号表，使其在函数体内可见
                    register_symbol(gen, captured->names[i]);
                    mark_ir_name_allocated(gen, captured->names[i]);
                    // 捕获变量不添加到作用域跟踪器（同参数一样，由调用者管理）
                }
            }
            
            // 切换到函数体缓冲区，设置函数作用域
            gen->code_buf = func_body_buf;
            gen->entry_alloca_buf = func_entry_alloca_buf;
            gen->scope = func_scope;
            gen->block_terminated = 0;  // 函数开始时，基本块未终止
            
            // 预扫描收集 catch 参数的 alloca
            if (func->body) {
                collect_catch_params(gen, func->body, func_entry_alloca_buf);
            }
            
            // 生成函数体代码
            if (func->body) {
                codegen_stmt(gen, func->body);
            }
            
            // 生成默认返回标签和代码
            // 只有在当前基本块未终止时才跳转到默认返回块
            char *default_ret_label = new_label(gen);
            if (!gen->block_terminated) {
                fprintf(func_body_buf, "  br label %%%s\n", default_ret_label);
            }
            fprintf(func_body_buf, "\n%s:\n", default_ret_label);
            
            // 重置 block_terminated 标志
            gen->block_terminated = 0;
            
            // P2: 生成默认返回前的清理代码
            fprintf(func_body_buf, "  ; P2: default return cleanup\n");
            for (LocalVarEntry *entry = func_scope->locals; entry != NULL; entry = entry->next) {
                char temp_name[64];
                snprintf(temp_name, sizeof(temp_name), "%%cleanup_%s_%d", entry->name, gen->temp_count++);
                fprintf(func_body_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                        temp_name, entry->name);
                fprintf(func_body_buf, "  call void @value_release(%%struct.Value* %s)\n", temp_name);
            }
            
            // 默认返回 null
            fprintf(func_body_buf, "  %%default_ret_%s = call %%struct.Value* @box_null()\n", func_llvm_name);
            fprintf(func_body_buf, "  ret %%struct.Value* %%default_ret_%s\n", func_llvm_name);
            
            // 恢复状态
            gen->code_buf = saved_code_buf;
            gen->entry_alloca_buf = saved_entry_alloca;
            gen->scope = saved_scope;
            gen->block_terminated = saved_block_terminated;
            gen->temp_values = saved_temp_values;  // 恢复临时值栈
            
            // 清理并恢复已分配名称集合
            clear_allocated_ir_names(gen);
            gen->allocated_ir_names = saved_allocated_ir_names;
            
            // 释放函数内新添加的符号（在 saved_symbols 之后添加的）
            // 注意：不释放 saved_symbols 及其之前的条目
            SymbolEntry *entry = gen->symbols;
            while (entry && entry != saved_symbols) {
                SymbolEntry *next = entry->next;
                free(entry->name);
                free(entry);
                entry = next;
            }
            gen->symbols = saved_symbols;
            
            // 将 entry alloca 写入输出
            rewind(func_entry_alloca_buf);
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), func_entry_alloca_buf)) {
                fputs(buffer, output_target);
            }
            
            // 将函数体代码写入输出
            rewind(func_body_buf);
            while (fgets(buffer, sizeof(buffer), func_body_buf)) {
                fputs(buffer, output_target);
            }
            
            // 清理函数体临时文件
            fclose(func_entry_alloca_buf);
            fclose(func_body_buf);
            
            // P2: 销毁函数作用域跟踪器
            scope_tracker_free(func_scope);
            
            free(default_ret_label);
            fprintf(output_target, "}\n");
            
            // 将完整的函数定义从临时缓冲区写入最终目标
            rewind(func_output_buf);
            while (fgets(buffer, sizeof(buffer), func_output_buf)) {
                fputs(buffer, final_target);
            }
            fclose(func_output_buf);
            
            // 对于嵌套函数，在父函数中创建一个本地变量来存储闭包
            // 但如果是从 codegen_expr 调用的（current_captured 已预设），
            // 则闭包创建由 codegen_expr 处理，这里不需要生成
            int called_from_expr = (gen->current_captured != NULL);
            
            if (is_nested && !called_from_expr) {
                // 在父函数的 entry_alloca 中为该嵌套函数创建局部变量
                // 但只在未分配过时才分配
                if (!is_ir_name_allocated(gen, func->name)) {
                    fprintf(gen->entry_alloca_buf, "  %%%s = alloca %%struct.Value*\n", func->name);
                    fprintf(gen->entry_alloca_buf, "  store %%struct.Value* null, %%struct.Value** %%%s\n", func->name);
                    mark_ir_name_allocated(gen, func->name);
                }
                
                // 在父函数中生成代码创建闭包并存储到变量
                if (captured && captured->count > 0) {
                    // 有捕获变量 - 创建完整闭包
                    // 分配数组存储捕获的变量
                    int arr_temp = gen->temp_count++;
                    fprintf(gen->code_buf, "  %%t%d = alloca [%zu x %%struct.Value*]\n", 
                            arr_temp, captured->count);
                    
                    // 填充捕获变量数组
                    for (size_t i = 0; i < captured->count; i++) {
                        int load_temp = gen->temp_count++;
                        int gep_temp = gen->temp_count++;
                        fprintf(gen->code_buf, "  %%t%d = load %%struct.Value*, %%struct.Value** %%%s\n",
                                load_temp, captured->names[i]);
                        fprintf(gen->code_buf, "  call %%struct.Value* @value_retain(%%struct.Value* %%t%d)\n",
                                load_temp);
                        fprintf(gen->code_buf, "  %%t%d = getelementptr inbounds [%zu x %%struct.Value*], "
                                "[%zu x %%struct.Value*]* %%t%d, i64 0, i64 %zu\n",
                                gep_temp, captured->count, captured->count, arr_temp, i);
                        fprintf(gen->code_buf, "  store %%struct.Value* %%t%d, %%struct.Value** %%t%d\n",
                                load_temp, gep_temp);
                    }
                    
                    // 获取数组起始指针
                    int ptr_temp = gen->temp_count++;
                    fprintf(gen->code_buf, "  %%t%d = getelementptr inbounds [%zu x %%struct.Value*], "
                            "[%zu x %%struct.Value*]* %%t%d, i64 0, i64 0\n",
                            ptr_temp, captured->count, captured->count, arr_temp);
                    
                    // 调用 box_function 创建闭包值
                    int func_val_temp = gen->temp_count++;
                    fprintf(gen->code_buf, "  %%t%d = call %%struct.Value* @box_function("
                            "i8* bitcast (%%struct.Value* (", func_val_temp);
                    
                    // 生成函数签名参数
                    int need_comma = 0;
                    for (size_t i = 0; i < func->param_count; i++) {
                        if (need_comma) fprintf(gen->code_buf, ", ");
                        fprintf(gen->code_buf, "%%struct.Value*");
                        need_comma = 1;
                    }
                    for (size_t i = 0; i < captured->count; i++) {
                        if (need_comma) fprintf(gen->code_buf, ", ");
                        fprintf(gen->code_buf, "%%struct.Value*");
                        need_comma = 1;
                    }
                    
                    fprintf(gen->code_buf, ")* @%s to i8*), %%struct.Value** %%t%d, i32 %zu, i32 %zu, i32 %d)\n",
                            func_llvm_name, ptr_temp, captured->count, func->param_count, func->uses_self ? 1 : 0);
                    
                    // 存储到局部变量
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_retain(%%struct.Value* %%t%d)\n",
                            func_val_temp);
                    fprintf(gen->code_buf, "  store %%struct.Value* %%t%d, %%struct.Value** %%%s\n",
                            func_val_temp, func->name);
                    
                    // 自引用闭包修复：检查捕获变量中是否包含函数自身
                    // 例如: f := (n) { f(n-1) } - 创建时 f 是 null，需要更新为实际函数
                    for (size_t i = 0; i < captured->count; i++) {
                        if (strcmp(captured->names[i], func->name) == 0) {
                            fprintf(gen->code_buf, "  ; Self-referencing closure fix: update captured[%zu] to point to the closure itself\n", i);
                            fprintf(gen->code_buf, "  call void @update_closure_captured(%%struct.Value* %%t%d, i32 %zu, %%struct.Value* %%t%d)\n",
                                    func_val_temp, i, func_val_temp);
                            break;
                        }
                    }
                } else {
                    // 无捕获变量 - 简单函数值
                    int func_val_temp = gen->temp_count++;
                    fprintf(gen->code_buf, "  %%t%d = call %%struct.Value* @box_function("
                            "i8* bitcast (%%struct.Value* (", func_val_temp);
                    
                    // 生成函数签名参数
                    for (size_t i = 0; i < func->param_count; i++) {
                        if (i > 0) fprintf(gen->code_buf, ", ");
                        fprintf(gen->code_buf, "%%struct.Value*");
                    }
                    
                    fprintf(gen->code_buf, ")* @%s to i8*), %%struct.Value** null, i32 0, i32 %zu, i32 %d)\n",
                            func_llvm_name, func->param_count, func->uses_self ? 1 : 0);
                    
                    // 存储到局部变量
                    fprintf(gen->code_buf, "  call %%struct.Value* @value_retain(%%struct.Value* %%t%d)\n",
                            func_val_temp);
                    fprintf(gen->code_buf, "  store %%struct.Value* %%t%d, %%struct.Value** %%%s\n",
                            func_val_temp, func->name);
                }
                
                // 添加到作用域跟踪器用于清理
                scope_add_local(gen->scope, func->name);
            }
            
            // 如果我们自己分析了闭包捕获，释放它
            if (own_captured) {
                captured_vars_free(own_captured);
            }
            
            break;
        }
        
        default:
            break;
    }
}
