#include "flyuxc/backend/codegen.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

static char *new_temp(CodeGen *gen) {
    char *temp = (char *)malloc(32);
    snprintf(temp, 32, "%%t%d", gen->temp_count++);
    return temp;
}

static char *new_label(CodeGen *gen) {
    char *label = (char *)malloc(32);
    snprintf(label, 32, "label%d", gen->label_count++);
    return label;
}

static char *new_string_label(CodeGen *gen) {
    char *label = (char *)malloc(32);
    snprintf(label, 32, "@.str.%d", gen->string_count++);
    return label;
}

/* ============================================================================
 * 前向声明
 * ============================================================================ */

static char *codegen_expr(CodeGen *gen, ASTNode *node);
static void codegen_stmt(CodeGen *gen, ASTNode *node);

/* ============================================================================
 * 创建和销毁
 * ============================================================================ */

CodeGen *codegen_create(FILE *output) {
    CodeGen *gen = (CodeGen *)malloc(sizeof(CodeGen));
    if (!gen) return NULL;
    
    gen->output = output;
    gen->globals_buf = tmpfile();  // 临时文件存储全局声明
    gen->code_buf = tmpfile();     // 临时文件存储函数代码
    if (!gen->globals_buf || !gen->code_buf) {
        if (gen->globals_buf) fclose(gen->globals_buf);
        if (gen->code_buf) fclose(gen->code_buf);
        free(gen);
        return NULL;
    }
    gen->temp_count = 0;
    gen->label_count = 0;
    gen->string_count = 0;
    
    return gen;
}

void codegen_free(CodeGen *gen) {
    if (gen) {
        if (gen->globals_buf) fclose(gen->globals_buf);
        if (gen->code_buf) fclose(gen->code_buf);
        free(gen);
    }
}

/* ============================================================================
 * 表达式代码生成
 * ============================================================================ */

static char *codegen_expr(CodeGen *gen, ASTNode *node) {
    if (!node) return NULL;
    
    switch (node->kind) {
        case AST_NUM_LITERAL: {
            ASTNumLiteral *num = (ASTNumLiteral *)node->data;
            char *temp = new_temp(gen);
            
            // 将整数或浮点数转换为 double
            if (num->value == (int)num->value) {
                // 整数：转换为 double
                fprintf(gen->code_buf, "  %s = sitofp i32 %d to double\n", 
                        temp, (int)num->value);
            } else {
                // 浮点数
                fprintf(gen->code_buf, "  %s = fadd double 0.0, %f\n", 
                        temp, num->value);
            }
            
            return temp;
        }
        
        case AST_STRING_LITERAL: {
            ASTStringLiteral *str = (ASTStringLiteral *)node->data;
            char *str_label = new_string_label(gen);
            char *temp = new_temp(gen);
            
            size_t len = strlen(str->value);
            
            // 声明全局字符串常量（写入全局缓冲区）
            fprintf(gen->globals_buf, "%s = private unnamed_addr constant [%zu x i8] c\"%s\\00\"\n",
                    str_label, len + 1, str->value);
            
            // 获取指针
            fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x i8], [%zu x i8]* %s, i32 0, i32 0\n",
                    temp, len + 1, len + 1, str_label);
            
            free(str_label);
            return temp;
        }
        
        case AST_IDENTIFIER: {
            ASTIdentifier *id = (ASTIdentifier *)node->data;
            char *temp = new_temp(gen);
            
            // 加载变量值
            fprintf(gen->code_buf, "  %s = load double, double* %%%s\n", 
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
                    fprintf(gen->code_buf, "  %s = fadd double %s, %s\n", 
                            result, left, right);
                    break;
                case TK_MINUS:
                    fprintf(gen->code_buf, "  %s = fsub double %s, %s\n", 
                            result, left, right);
                    break;
                case TK_STAR:
                    fprintf(gen->code_buf, "  %s = fmul double %s, %s\n", 
                            result, left, right);
                    break;
                case TK_SLASH:
                    fprintf(gen->code_buf, "  %s = fdiv double %s, %s\n", 
                            result, left, right);
                    break;
                case TK_LT:
                    fprintf(gen->code_buf, "  %s = fcmp olt double %s, %s\n", 
                            result, left, right);
                    break;
                case TK_GT:
                    fprintf(gen->code_buf, "  %s = fcmp ogt double %s, %s\n", 
                            result, left, right);
                    break;
                case TK_LE:
                    fprintf(gen->code_buf, "  %s = fcmp ole double %s, %s\n", 
                            result, left, right);
                    break;
                case TK_GE:
                    fprintf(gen->code_buf, "  %s = fcmp oge double %s, %s\n", 
                            result, left, right);
                    break;
                case TK_EQ_EQ:
                    fprintf(gen->code_buf, "  %s = fcmp oeq double %s, %s\n", 
                            result, left, right);
                    break;
                case TK_BANG_EQ:
                    fprintf(gen->code_buf, "  %s = fcmp one double %s, %s\n", 
                            result, left, right);
                    break;
                default:
                    break;
            }
            
            free(left);
            free(right);
            return result;
        }
        
        case AST_CALL_EXPR: {
            ASTCallExpr *call = (ASTCallExpr *)node->data;
            ASTIdentifier *callee = (ASTIdentifier *)call->callee->data;
            
            // 特殊处理 print 函数
            if (strcmp(callee->name, "print") == 0) {
                // 简化实现：只打印第一个参数
                if (call->arg_count > 0) {
                    char *arg = codegen_expr(gen, call->args[0]);
                    
                    // 生成格式字符串常量（写入全局缓冲区）
                    char *fmt_label = new_string_label(gen);
                    fprintf(gen->globals_buf, "%s = private unnamed_addr constant [4 x i8] c\"%%f\\0A\\00\"\n",
                            fmt_label);
                    
                    // 获取指针并调用 printf
                    char *fmt_ptr = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = getelementptr inbounds [4 x i8], [4 x i8]* %s, i32 0, i32 0\n",
                            fmt_ptr, fmt_label);
                    
                    fprintf(gen->code_buf, "  call i32 (i8*, ...) @printf(i8* %s, double %s)\n",
                            fmt_ptr, arg);
                    
                    free(arg);
                    free(fmt_ptr);
                }
                
                return NULL;
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
            fprintf(gen->code_buf, "  %s = call double @%s(", result, callee->name);
            
            for (size_t i = 0; i < call->arg_count; i++) {
                if (i > 0) fprintf(gen->code_buf, ", ");
                fprintf(gen->code_buf, "double %s", arg_regs[i]);
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
            // 简化实现：返回一个 double 值作为占位符
            // 真实实现需要分配内存并初始化数组
            char *temp = new_temp(gen);
            fprintf(gen->code_buf, "  %s = fadd double 0.0, 0.0  ; array placeholder\n", temp);
            return temp;
        }
        
        case AST_OBJECT_LITERAL: {
            // 简化实现：返回一个 double 值作为占位符
            char *temp = new_temp(gen);
            fprintf(gen->code_buf, "  %s = fadd double 0.0, 0.0  ; object placeholder\n", temp);
            return temp;
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
                char *new_val = new_temp(gen);
                
                // 加载当前值
                fprintf(gen->code_buf, "  %s = load double, double* %%%s\n", old_val, id->name);
                
                // 增加或减少
                if (unary->op == TK_PLUS_PLUS) {
                    fprintf(gen->code_buf, "  %s = fadd double %s, 1.0\n", new_val, old_val);
                } else {
                    fprintf(gen->code_buf, "  %s = fsub double %s, 1.0\n", new_val, old_val);
                }
                
                // 存储新值
                fprintf(gen->code_buf, "  store double %s, double* %%%s\n", new_val, id->name);
                
                // 返回值：前缀返回新值，后缀返回旧值
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
                case TK_MINUS:
                    fprintf(gen->code_buf, "  %s = fsub double 0.0, %s\n", result, operand);
                    break;
                case TK_PLUS:
                    // 一元+不改变值
                    free(result);
                    return operand;
                case TK_BANG:
                    // !x => x == 0.0
                    fprintf(gen->code_buf, "  %s = fcmp oeq double %s, 0.0\n", result, operand);
                    break;
                default:
                    free(result);
                    free(operand);
                    return NULL;
            }
            
            free(operand);
            return result;
        }
        
        case AST_MEMBER_EXPR: {
            // 简化实现：访问对象属性返回占位符
            char *temp = new_temp(gen);
            fprintf(gen->code_buf, "  %s = fadd double 0.0, 0.0  ; member access placeholder\n", temp);
            return temp;
        }
        
        case AST_INDEX_EXPR: {
            // 简化实现：数组索引访问返回占位符
            char *temp = new_temp(gen);
            fprintf(gen->code_buf, "  %s = fadd double 0.0, 0.0  ; index access placeholder\n", temp);
            return temp;
        }
        
        default:
            return NULL;
    }
}

/* ============================================================================
 * 语句代码生成
 * ============================================================================ */

static void codegen_stmt(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->kind) {
        case AST_VAR_DECL: {
            ASTVarDecl *decl = (ASTVarDecl *)node->data;
            
            // 分配栈空间
            fprintf(gen->code_buf, "  %%%s = alloca double\n", decl->name);
            
            // 如果有初始化表达式
            if (decl->init_expr) {
                char *init_val = codegen_expr(gen, decl->init_expr);
                if (init_val) {
                    fprintf(gen->code_buf, "  store double %s, double* %%%s\n",
                            init_val, decl->name);
                    free(init_val);
                }
            }
            break;
        }
        
        case AST_ASSIGN_STMT: {
            ASTAssignStmt *assign = (ASTAssignStmt *)node->data;
            ASTIdentifier *target = (ASTIdentifier *)assign->target->data;
            
            char *value = codegen_expr(gen, assign->value);
            if (value) {
                fprintf(gen->code_buf, "  store double %s, double* %%%s\n",
                        value, target->name);
                free(value);
            }
            break;
        }
        
        case AST_RETURN_STMT: {
            ASTReturnStmt *ret = (ASTReturnStmt *)node->data;
            
            if (ret->value) {
                char *ret_val = codegen_expr(gen, ret->value);
                if (ret_val) {
                    fprintf(gen->code_buf, "  ret double %s\n", ret_val);
                    free(ret_val);
                } else {
                    fprintf(gen->code_buf, "  ret double 0.0\n");
                }
            } else {
                fprintf(gen->code_buf, "  ret double 0.0\n");
            }
            break;
        }
        
        case AST_IF_STMT: {
            ASTIfStmt *ifstmt = (ASTIfStmt *)node->data;
            
            if (ifstmt->cond_count > 0) {
                char *cond = codegen_expr(gen, ifstmt->conditions[0]);
                
                char *then_label = new_label(gen);
                char *else_label = new_label(gen);
                char *end_label = new_label(gen);
                
                // 条件跳转
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n",
                        cond, then_label, else_label);
                
                // Then 块
                fprintf(gen->code_buf, "\n%s:\n", then_label);
                if (ifstmt->then_blocks[0]) {
                    codegen_stmt(gen, ifstmt->then_blocks[0]);
                }
                fprintf(gen->code_buf, "  br label %%%s\n", end_label);
                
                // Else 块
                fprintf(gen->code_buf, "\n%s:\n", else_label);
                if (ifstmt->else_block) {
                    codegen_stmt(gen, ifstmt->else_block);
                }
                fprintf(gen->code_buf, "  br label %%%s\n", end_label);
                
                // End 标签
                fprintf(gen->code_buf, "\n%s:\n", end_label);
                
                free(cond);
                free(then_label);
                free(else_label);
                free(end_label);
            }
            break;
        }
        
        case AST_BLOCK: {
            ASTBlock *block = (ASTBlock *)node->data;
            for (size_t i = 0; i < block->stmt_count; i++) {
                codegen_stmt(gen, block->statements[i]);
            }
            break;
        }
        
        case AST_LOOP_STMT: {
            ASTLoopStmt *loop = (ASTLoopStmt *)node->data;
            
            if (loop->loop_type == LOOP_FOR) {
                // for循环: init; cond; update { body }
                char *loop_header = new_label(gen);
                char *loop_body = new_label(gen);
                char *loop_update = new_label(gen);
                char *loop_end = new_label(gen);
                
                // 初始化
                if (loop->loop_data.for_loop.init) {
                    codegen_stmt(gen, loop->loop_data.for_loop.init);
                }
                
                // 跳转到条件检查
                fprintf(gen->code_buf, "  br label %%%s\n", loop_header);
                fprintf(gen->code_buf, "\n%s:\n", loop_header);
                
                // 条件判断
                if (loop->loop_data.for_loop.condition) {
                    char *cond = codegen_expr(gen, loop->loop_data.for_loop.condition);
                    fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n",
                            cond, loop_body, loop_end);
                    free(cond);
                } else {
                    // 无条件则一直循环
                    fprintf(gen->code_buf, "  br label %%%s\n", loop_body);
                }
                
                // 循环体
                fprintf(gen->code_buf, "\n%s:\n", loop_body);
                if (loop->body) {
                    codegen_stmt(gen, loop->body);
                }
                fprintf(gen->code_buf, "  br label %%%s\n", loop_update);
                
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
            if (result) free(result);
            break;
        }
        
        case AST_FUNC_DECL: {
            ASTFuncDecl *func = (ASTFuncDecl *)node->data;
            
            // 函数签名
            fprintf(gen->code_buf, "\ndefine double @%s(", func->name);
            
            for (size_t i = 0; i < func->param_count; i++) {
                if (i > 0) fprintf(gen->code_buf, ", ");
                fprintf(gen->code_buf, "double %%param_%s", func->params[i]);
            }
            
            fprintf(gen->code_buf, ") {\n");
            
            // 为参数创建局部变量并存储参数值
            for (size_t i = 0; i < func->param_count; i++) {
                fprintf(gen->code_buf, "  %%%s = alloca double\n", func->params[i]);
                fprintf(gen->code_buf, "  store double %%param_%s, double* %%%s\n",
                        func->params[i], func->params[i]);
            }
            
            // 函数体
            if (func->body) {
                codegen_stmt(gen, func->body);
            }
            
            // 如果没有显式返回，添加默认返回
            fprintf(gen->code_buf, "  ret double 0.0\n");
            fprintf(gen->code_buf, "}\n");
            break;
        }
        
        default:
            break;
    }
}

/* ============================================================================
 * 主代码生成入口
 * ============================================================================ */

void codegen_generate(CodeGen *gen, ASTNode *ast) {
    if (!gen || !ast) return;
    
    // 先生成代码到临时缓冲区
    if (ast->kind == AST_PROGRAM) {
        ASTProgram *prog = (ASTProgram *)ast->data;
        
        // 首先生成所有函数声明
        for (size_t i = 0; i < prog->stmt_count; i++) {
            if (prog->statements[i]->kind == AST_FUNC_DECL) {
                codegen_stmt(gen, prog->statements[i]);
            }
        }
        
        // 生成 main 函数（如果有）
        bool has_main = false;
        for (size_t i = 0; i < prog->stmt_count; i++) {
            ASTNode *stmt = prog->statements[i];
            if (stmt->kind == AST_FUNC_DECL) {
                ASTFuncDecl *func = (ASTFuncDecl *)stmt->data;
                if (strcmp(func->name, "main") == 0) {
                    has_main = true;
                    break;
                }
            }
        }
        
        // 如果没有 main 函数，创建一个包装器
        if (!has_main) {
            fprintf(gen->code_buf, "\ndefine i32 @main() {\n");
            
            // 执行所有顶层语句
            for (size_t i = 0; i < prog->stmt_count; i++) {
                if (prog->statements[i]->kind != AST_FUNC_DECL) {
                    codegen_stmt(gen, prog->statements[i]);
                }
            }
            
            fprintf(gen->code_buf, "  ret i32 0\n");
            fprintf(gen->code_buf, "}\n");
        }
    }
    
    // 现在按正确顺序写入最终输出文件
    // 1. 头部
    fprintf(gen->output, "; ModuleID = 'flyux_module'\n");
    fprintf(gen->output, "target datalayout = \"e-m:o-i64:64-f80:128-n8:16:32:64-S128\"\n");
    fprintf(gen->output, "target triple = \"x86_64-apple-macosx10.15.0\"\n\n");
    
    // 2. 外部声明
    fprintf(gen->output, "declare i32 @printf(i8*, ...)\n\n");
    
    // 3. 全局常量（从globals_buf复制）
    rewind(gen->globals_buf);
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), gen->globals_buf)) {
        fputs(buffer, gen->output);
    }
    fprintf(gen->output, "\n");
    
    // 4. 函数定义（从code_buf复制）
    rewind(gen->code_buf);
    while (fgets(buffer, sizeof(buffer), gen->code_buf)) {
        fputs(buffer, gen->output);
    }
}
