#include "codegen_internal.h"
#include "flyuxc/frontend/varmap.h"
#include "flyuxc/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 核心功能 - 创建/销毁/生成
 * ============================================================================ */

CodeGen *codegen_create(FILE *output) {
    CodeGen *gen = (CodeGen *)malloc(sizeof(CodeGen));
    if (!gen) return NULL;
    
    gen->output = output;
    gen->globals_buf = tmpfile();  // 临时文件存储全局声明
    gen->strings_buf = tmpfile();  // 临时文件存储字符串常量
    gen->code_buf = tmpfile();     // 临时文件存储函数代码
    if (!gen->globals_buf || !gen->strings_buf || !gen->code_buf) {
        if (gen->globals_buf) fclose(gen->globals_buf);
        if (gen->strings_buf) fclose(gen->strings_buf);
        if (gen->code_buf) fclose(gen->code_buf);
        free(gen);
        return NULL;
    }
    gen->temp_count = 0;
    gen->label_count = 0;
    gen->string_count = 0;
    gen->arrays = NULL;
    gen->objects = NULL;
    gen->symbols = NULL;
    gen->globals = NULL;  /* 初始无全局变量 */
    gen->scope_level = 0;  /* 初始作用域层级为0 */
    gen->shadow_count = 0;  /* 初始遮蔽计数为0 */
    gen->functions = NULL;  /* 初始无函数表 */
    gen->closure_mappings = NULL;  /* 初始无闭包映射 */
    gen->current_var_name = NULL;
    gen->in_try_catch = 0;  /* 初始不在 Try-Catch 块中 */
    gen->try_catch_label = NULL;
    gen->loop_end_label = NULL;  /* 初始不在循环中 */
    gen->loop_continue_label = NULL;  /* 初始不在循环中 */
    gen->entry_alloca_buf = NULL;  /* 初始无函数入口alloca缓冲区 */
    gen->scope = NULL;  /* P2: 初始无作用域跟踪器（在函数内部设置） */
    gen->loop_scope_stack = NULL;  /* 循环作用域栈初始为空 */
    gen->block_terminated = 0;  /* 初始基本块未终止 */
    gen->temp_values = NULL;  /* 中间值栈初始为空 */
    gen->has_error = 0;  /* 初始无错误 */
    gen->error_message = NULL;  /* 初始无错误消息 */
    gen->varmap_entries = NULL;  /* 初始无映射表 */
    gen->varmap_count = 0;  /* 初始映射表大小为0 */
    gen->original_source = NULL;  /* 初始无原始源代码 */
    gen->current_captured = NULL;  /* 初始无闭包捕获变量 */
    gen->allocated_ir_names = NULL;  /* 初始无已分配 IR 名称 */
    gen->refbox_vars = NULL;  /* 初始无引用盒子变量 */
    
    return gen;
}

/* 设置变量映射表 */
void codegen_set_varmap(CodeGen *gen, void *entries, size_t count) {
    if (gen) {
        gen->varmap_entries = entries;
        gen->varmap_count = count;
    }
}

/* 设置原始源代码 */
void codegen_set_original_source(CodeGen *gen, const char *source) {
    if (gen) {
        gen->original_source = source;
    }
}

void codegen_free(CodeGen *gen) {
    if (gen) {
        if (gen->globals_buf) fclose(gen->globals_buf);
        if (gen->strings_buf) fclose(gen->strings_buf);
        if (gen->code_buf) fclose(gen->code_buf);
        
        // 释放数组元数据
        ArrayMetadata *meta = gen->arrays;
        while (meta) {
            ArrayMetadata *next = meta->next;
            free(meta->var_name);
            free(meta->array_ptr);
            free(meta);
            meta = next;
        }
        
        // 释放对象元数据
        ObjectMetadata *obj_meta = gen->objects;
        while (obj_meta) {
            ObjectMetadata *next_obj = obj_meta->next;
            free(obj_meta->var_name);
            
            // 释放字段链表
            ObjectField *field = obj_meta->fields;
            while (field) {
                ObjectField *next_field = field->next;
                free(field->field_name);
                free(field->field_ptr);
                free(field);
                field = next_field;
            }
            
            free(obj_meta);
            obj_meta = next_obj;
        }
        
        // 释放符号表
        SymbolEntry *sym = gen->symbols;
        while (sym) {
            SymbolEntry *next_sym = sym->next;
            free(sym->name);
            free(sym);
            sym = next_sym;
        }
        
        // 释放函数表
        SymbolEntry *func = gen->functions;
        while (func) {
            SymbolEntry *next_func = func->next;
            free(func->name);
            free(func);
            func = next_func;
        }
        
        // 释放闭包映射
        ClosureMapping *mapping = gen->closure_mappings;
        while (mapping) {
            ClosureMapping *next_mapping = mapping->next;
            free(mapping->var_name);
            free(mapping->func_name);
            if (mapping->captured) {
                captured_vars_free(mapping->captured);
            }
            free(mapping);
            mapping = next_mapping;
        }
        
        // 释放中间值栈
        if (gen->temp_values) {
            temp_value_stack_free(gen->temp_values);
        }
        
        // 释放错误消息
        if (gen->error_message) {
            free(gen->error_message);
        }
        
        free(gen);
    }
}

/* 检查是否有错误 */
int codegen_has_error(CodeGen *gen) {
    return gen ? gen->has_error : 0;
}

/* 获取错误消息 */
const char *codegen_get_error(CodeGen *gen) {
    return gen ? gen->error_message : NULL;
}

/* 设置错误 */
void codegen_set_error(CodeGen *gen, const char *message) {
    if (gen && !gen->has_error) {  /* 只记录第一个错误 */
        gen->has_error = 1;
        gen->error_message = strdup(message);
    }
}

/* 获取指定行的源代码 */
static char *get_source_line(const char *source, int line_num) {
    if (!source || line_num < 1) return NULL;
    
    const char *p = source;
    int current_line = 1;
    
    // 找到目标行的开始
    while (*p && current_line < line_num) {
        if (*p == '\n') {
            current_line++;
        }
        p++;
    }
    
    if (!*p && current_line < line_num) return NULL;
    
    // 找到行末
    const char *line_start = p;
    while (*p && *p != '\n') {
        p++;
    }
    
    size_t line_len = p - line_start;
    char *line = malloc(line_len + 1);
    if (!line) return NULL;
    
    memcpy(line, line_start, line_len);
    line[line_len] = '\0';
    return line;
}

/* 设置带位置信息的编译错误 - 直接输出到 stderr */
void codegen_set_error_at(CodeGen *gen, int line, int column, int length,
                          const char *var_name, const char *message) {
    if (gen && !gen->has_error) {
        gen->has_error = 1;
        
        // 构建消息（带变量名）
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "%s '%s'", message, var_name);
        
        // 使用传入的长度，如果为0则默认使用变量名长度
        int error_length = length > 0 ? length : (int)strlen(var_name);
        
        // 直接使用统一的错误报告接口输出
        report_error_at(ERR_ERROR, PHASE_CODEGEN,
                        gen->original_source,
                        line, column, error_length,
                        full_message);
        
        // 保存简单的错误标记（不再需要完整消息）
        gen->error_message = strdup(full_message);
    }
}

/* 从映射表中查找映射后名字对应的原始名字 */
const char *codegen_lookup_original_name(CodeGen *gen, const char *mapped_name) {
    if (!gen || !gen->varmap_entries || !mapped_name) return NULL;
    
    VarMapEntry *entries = (VarMapEntry *)gen->varmap_entries;
    for (size_t i = 0; i < gen->varmap_count; i++) {
        if (entries[i].mapped && strcmp(entries[i].mapped, mapped_name) == 0) {
            return entries[i].original;
        }
    }
    return NULL;
}

/* ============================================================================
 * 主代码生成入口
 * ============================================================================ */

void codegen_generate(CodeGen *gen, ASTNode *ast) {
    if (!gen || !ast) return;
    
    // 先生成代码到临时缓冲区
    if (ast->kind == AST_PROGRAM) {
        ASTProgram *prog = (ASTProgram *)ast->data;
        
        // 检查是否有用户定义的main函数
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
        
        // 如果有main函数，先处理全局变量
        if (has_main) {
            for (size_t i = 0; i < prog->stmt_count; i++) {
                ASTNode *stmt = prog->statements[i];
                if (stmt->kind == AST_VAR_DECL || stmt->kind == AST_CONST_DECL) {
                    ASTVarDecl *decl = (ASTVarDecl *)stmt->data;
                    // 检查是否是函数定义（初始值是函数表达式），函数定义允许
                    if (decl->init_expr && decl->init_expr->kind == AST_FUNC_DECL) {
                        continue;  // 函数定义允许
                    }
                    // 检查是否是字面量值
                    if (decl->init_expr) {
                        ASTNodeKind init_kind = decl->init_expr->kind;
                        if (init_kind != AST_NUM_LITERAL && 
                            init_kind != AST_STRING_LITERAL &&
                            init_kind != AST_BOOL_LITERAL &&
                            init_kind != AST_NULL_LITERAL &&
                            init_kind != AST_ARRAY_LITERAL &&
                            init_kind != AST_OBJECT_LITERAL) {
                            // 不是字面量，报错
                            const char *original_name = codegen_lookup_original_name(gen, decl->name);
                            const char *display_name = original_name ? original_name : decl->name;
                            codegen_set_error_at(gen, stmt->loc.orig_line, stmt->loc.orig_column,
                                                 stmt->loc.orig_length,
                                                 display_name,
                                                 "Global variable must be initialized with a literal value");
                            return;
                        }
                    }
                    // 注册为全局变量，生成LLVM全局变量声明
                    register_global(gen, decl->name, decl->is_const);
                    fprintf(gen->globals_buf, "@%s = global %%struct.Value* null\n", decl->name);
                }
            }
        }
        
        // 预注册所有函数名（用于支持前向引用）
        // 包括直接的 AST_FUNC_DECL 和作为 AST_VAR_DECL 初始值的函数
        for (size_t i = 0; i < prog->stmt_count; i++) {
            ASTNode *stmt = prog->statements[i];
            if (getenv("DEBUG_CODEGEN")) {
                const char *kind_name = "UNKNOWN";
                switch (stmt->kind) {
                    case AST_FUNC_DECL: kind_name = "FUNC_DECL"; break;
                    case AST_VAR_DECL: kind_name = "VAR_DECL"; break;
                    default: break;
                }
                fprintf(stderr, "[DEBUG CODEGEN] stmt[%zu] kind = %s\n", i, kind_name);
            }
            if (stmt->kind == AST_FUNC_DECL) {
                ASTFuncDecl *func = (ASTFuncDecl *)stmt->data;
                if (getenv("DEBUG_CODEGEN")) {
                    fprintf(stderr, "[DEBUG CODEGEN] Registering function: %s\n", func->name);
                }
                register_symbol(gen, func->name);
                register_function(gen, func->name);  // 同时注册到函数表
            } else if (stmt->kind == AST_VAR_DECL) {
                // 检查是否是函数赋值（name := (params) { body }）
                ASTVarDecl *decl = (ASTVarDecl *)stmt->data;
                if (decl->init_expr && decl->init_expr->kind == AST_FUNC_DECL) {
                    // 这是一个函数定义，注册变量名
                    if (getenv("DEBUG_CODEGEN")) {
                        fprintf(stderr, "[DEBUG CODEGEN] Registering VAR_DECL function: %s\n", decl->name);
                    }
                    register_symbol(gen, decl->name);
                    register_function(gen, decl->name);  // 同时注册到函数表
                }
            }
        }
        
        // 预扫描：注册所有全局变量/常量（在生成函数代码之前）
        // 这样函数内的闭包分析才能找到这些全局常量
        for (size_t i = 0; i < prog->stmt_count; i++) {
            ASTNode *stmt = prog->statements[i];
            if (stmt->kind == AST_VAR_DECL || stmt->kind == AST_CONST_DECL) {
                ASTVarDecl *decl = (ASTVarDecl *)stmt->data;
                // 跳过函数定义
                if (decl->init_expr && decl->init_expr->kind == AST_FUNC_DECL) {
                    continue;
                }
                // 注册为全局变量（供闭包分析使用）
                register_global(gen, decl->name, decl->is_const);
                if (getenv("DEBUG_GLOBALS")) {
                    fprintf(stderr, "[PRE-SCAN EARLY] Registered global: %s (const=%d)\n", 
                            decl->name, decl->is_const);
                }
            }
        }
        
        // 然后生成所有函数代码
        for (size_t i = 0; i < prog->stmt_count; i++) {
            if (prog->statements[i]->kind == AST_FUNC_DECL) {
                codegen_stmt(gen, prog->statements[i]);
            }
        }
        
        // 始终创建 i32 @main() 作为程序入口
        if (has_main) {
            // 有用户定义的main函数，先初始化全局变量
            fprintf(gen->code_buf, "\ndefine i32 @main() {\n");
            
            // 初始化全局变量
            for (size_t i = 0; i < prog->stmt_count; i++) {
                ASTNode *stmt = prog->statements[i];
                if ((stmt->kind == AST_VAR_DECL || stmt->kind == AST_CONST_DECL)) {
                    ASTVarDecl *decl = (ASTVarDecl *)stmt->data;
                    // 跳过函数定义
                    if (decl->init_expr && decl->init_expr->kind == AST_FUNC_DECL) {
                        continue;
                    }
                    // 生成初始化代码
                    if (decl->init_expr) {
                        char *init_val = codegen_expr(gen, decl->init_expr);
                        if (init_val) {
                            fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** @%s\n",
                                    init_val, decl->name);
                            free(init_val);
                        }
                    }
                }
            }
            
            // 调用用户定义的main函数 (重命名为_flyux_main)
            fprintf(gen->code_buf, "  %%user_main_result = call %%struct.Value* @_flyux_main()\n");
        } else {
            // 没有main函数的情况：顶层变量作为真正的全局变量处理
            // 预扫描：注册并生成全局变量/常量
            for (size_t i = 0; i < prog->stmt_count; i++) {
                ASTNode *stmt = prog->statements[i];
                if (stmt->kind == AST_VAR_DECL || stmt->kind == AST_CONST_DECL) {
                    ASTVarDecl *decl = (ASTVarDecl *)stmt->data;
                    // 跳过函数定义
                    if (decl->init_expr && decl->init_expr->kind == AST_FUNC_DECL) {
                        continue;
                    }
                    // 注册为全局变量
                    register_global(gen, decl->name, decl->is_const);
                    // 生成LLVM全局变量声明
                    fprintf(gen->globals_buf, "@%s = global %%struct.Value* null\n", decl->name);
                    if (getenv("DEBUG_GLOBALS")) {
                        fprintf(stderr, "[NO-MAIN PRE-SCAN] Registered and declared global: %s (const=%d)\n", 
                                decl->name, decl->is_const);
                    }
                }
            }
            
            // 预扫描：收集所有需要在entry块alloca的变量（catch参数等）
            FILE *entry_alloca_buf = tmpfile();
            FILE *main_body_buf = tmpfile();  // 单独的缓冲区保存main body
            
            // 暂时将code_buf指向main_body_buf，这样codegen_stmt写入的代码会到这个缓冲区
            FILE *old_code_buf = gen->code_buf;
            gen->code_buf = main_body_buf;
            gen->entry_alloca_buf = entry_alloca_buf;  // 保存到gen中供codegen_stmt使用
            
            for (size_t i = 0; i < prog->stmt_count; i++) {
                if (prog->statements[i]->kind != AST_FUNC_DECL) {
                    collect_catch_params(gen, prog->statements[i], entry_alloca_buf);
                }
            }
            
            // 执行所有顶层语句（跳过已经作为全局处理的变量声明）
            for (size_t i = 0; i < prog->stmt_count; i++) {
                ASTNode *stmt = prog->statements[i];
                // 跳过函数声明
                if (stmt->kind == AST_FUNC_DECL) {
                    continue;
                }
                // 跳过非函数的变量/常量声明（它们已经作为全局变量处理）
                if (stmt->kind == AST_VAR_DECL || stmt->kind == AST_CONST_DECL) {
                    ASTVarDecl *decl = (ASTVarDecl *)stmt->data;
                    // 只跳过非函数变量
                    if (!decl->init_expr || decl->init_expr->kind != AST_FUNC_DECL) {
                        continue;
                    }
                    // 函数变量需要处理（虽然前面已经生成了函数代码，这里生成变量赋值）
                }
                codegen_stmt(gen, stmt);
            }
            
            // 恢复code_buf
            gen->code_buf = old_code_buf;
            gen->entry_alloca_buf = NULL;
            
            // 现在开始写入main函数
            fprintf(gen->code_buf, "\ndefine i32 @main() {\n");
            
            // 初始化全局变量（就像has_main分支一样）
            for (size_t i = 0; i < prog->stmt_count; i++) {
                ASTNode *stmt = prog->statements[i];
                if ((stmt->kind == AST_VAR_DECL || stmt->kind == AST_CONST_DECL)) {
                    ASTVarDecl *decl = (ASTVarDecl *)stmt->data;
                    // 跳过函数定义
                    if (decl->init_expr && decl->init_expr->kind == AST_FUNC_DECL) {
                        continue;
                    }
                    // 生成初始化代码
                    if (decl->init_expr) {
                        char *init_val = codegen_expr(gen, decl->init_expr);
                        if (init_val) {
                            fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** @%s\n",
                                    init_val, decl->name);
                            free(init_val);
                        }
                    }
                }
            }
            
            // 1. 先写入所有alloca
            rewind(entry_alloca_buf);
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), entry_alloca_buf)) {
                fputs(buffer, gen->code_buf);
            }
            
            // 2. 再写入函数体代码（但跳过变量声明，因为已经作为全局处理）
            rewind(main_body_buf);
            while (fgets(buffer, sizeof(buffer), main_body_buf)) {
                fputs(buffer, gen->code_buf);
            }
            
            fclose(entry_alloca_buf);
            fclose(main_body_buf);
        }
        
        // 在程序结束前条件性输出换行,防止shell提示符与输出混在一起
        // 只有当最后输出不是换行时才添加换行
        fprintf(gen->code_buf, "  %%needs_newline = call i32 @value_needs_final_newline()\n");
        fprintf(gen->code_buf, "  %%should_print_newline = icmp ne i32 %%needs_newline, 0\n");
        fprintf(gen->code_buf, "  br i1 %%should_print_newline, label %%print_newline, label %%skip_newline\n");
        fprintf(gen->code_buf, "print_newline:\n");
        fprintf(gen->code_buf, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.newline, i32 0, i32 0))\n");
        fprintf(gen->code_buf, "  br label %%skip_newline\n");
        fprintf(gen->code_buf, "skip_newline:\n");
        fprintf(gen->code_buf, "  ret i32 0\n");
        fprintf(gen->code_buf, "}\n");
    }
    
    // 现在按正确顺序写入最终输出文件
    // 1. 头部
    fprintf(gen->output, "; ModuleID = 'flyux_module'\n");
    fprintf(gen->output, "target datalayout = \"e-m:o-i64:64-f80:128-n8:16:32:64-S128\"\n");
    fprintf(gen->output, "target triple = \"x86_64-apple-macosx10.15.0\"\n\n");
    
    // 1.5. 错误对象字段名称常量
    fprintf(gen->output, ";; Error object field names\n");
    fprintf(gen->output, "@str_message = private unnamed_addr constant [8 x i8] c\"message\\00\"\n");
    fprintf(gen->output, "@str_code = private unnamed_addr constant [5 x i8] c\"code\\00\"\n");
    fprintf(gen->output, "@str_type = private unnamed_addr constant [5 x i8] c\"type\\00\"\n");
    fprintf(gen->output, "@str_type_error = private unnamed_addr constant [10 x i8] c\"TypeError\\00\"\n");
    fprintf(gen->output, "@str_error = private unnamed_addr constant [6 x i8] c\"Error\\00\"\n");
    fprintf(gen->output, "@.str.newline = private unnamed_addr constant [2 x i8] c\"\\0A\\00\"\n");
    fprintf(gen->output, "@.str.not_callable = private unnamed_addr constant [30 x i8] c\"Error: value is not callable\\0A\\00\"\n\n");
    
    // 2. Value 结构体定义
    fprintf(gen->output, ";; Mixed-type value system\n");
    fprintf(gen->output, "%%struct.Value = type { i32, [12 x i8] }\n");
    fprintf(gen->output, "%%struct.ObjectEntry = type { i8*, %%struct.Value* }\n");
    fprintf(gen->output, "%%struct.ObjectPair = type { i8*, %%struct.Value* }\n\n");
    
    // 3. 运行时函数声明
    fprintf(gen->output, ";; Boxing functions\n");
    fprintf(gen->output, "declare %%struct.Value* @box_number(double)\n");
    fprintf(gen->output, "declare %%struct.Value* @box_string(i8*)\n");
    fprintf(gen->output, "declare %%struct.Value* @box_string_with_length(i8*, i64)\n");
    fprintf(gen->output, "declare %%struct.Value* @box_bool(i32)\n");
    fprintf(gen->output, "declare %%struct.Value* @box_null()\n");
    fprintf(gen->output, "declare %%struct.Value* @box_undef()\n");
    fprintf(gen->output, "declare %%struct.Value* @box_null_typed(i32)\n");
    fprintf(gen->output, "declare %%struct.Value* @box_null_preserve_type(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @box_array(i8*, i64)\n");
    fprintf(gen->output, "declare %%struct.Value* @box_object(i8*, i64)\n");
    fprintf(gen->output, "declare %%struct.Value* @box_function(i8*, %%struct.Value**, i32, i32, i32)\n");  // 添加 needs_self 参数
    fprintf(gen->output, "declare %%struct.Value* @box_function_ex(i8*, %%struct.Value**, i32, i32, i32, i32)\n");  // 扩展版本：添加 capture_by_ref 参数
    fprintf(gen->output, "declare void @update_closure_captured(%%struct.Value*, i32, %%struct.Value*)\n");
    
    fprintf(gen->output, ";; Reference box functions for closure capture\n");
    fprintf(gen->output, "declare %%struct.Value* @box_ref(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @ref_get(%%struct.Value*)\n");
    fprintf(gen->output, "declare void @ref_set(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare void @ref_free(%%struct.Value*)\n\n");
    
    fprintf(gen->output, ";; Unboxing functions\n");
    fprintf(gen->output, "declare double @unbox_number(%%struct.Value*)\n");
    fprintf(gen->output, "declare i8* @unbox_string(%%struct.Value*)\n");
    fprintf(gen->output, "declare i8* @unbox_function_ptr(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value** @get_function_captured(%%struct.Value*)\n");
    fprintf(gen->output, "declare i32 @get_function_captured_count(%%struct.Value*)\n");
    fprintf(gen->output, "declare i32 @get_function_param_count(%%struct.Value*)\n");
    fprintf(gen->output, "declare i32 @value_is_function(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @call_function_value(%%struct.Value*, %%struct.Value**, i32)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_call_function(%%struct.Value*, %%struct.Value**, i64)\n\n");
    
    fprintf(gen->output, ";; Utility functions\n");
    fprintf(gen->output, "declare i32 @value_is_truthy(%%struct.Value*)\n");
    fprintf(gen->output, "declare void @value_print(%%struct.Value*)\n");
    fprintf(gen->output, "declare void @value_println(%%struct.Value*)\n");
    fprintf(gen->output, "declare void @value_printf(%%struct.Value*, %%struct.Value**, i64)\n");
    fprintf(gen->output, "declare i8* @value_typeof(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_add(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_subtract(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_multiply(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_divide(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_power(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_modulo(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_equals(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_less_than(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_greater_than(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_index(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_index_safe(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare i64 @value_array_length(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_array_get(%%struct.Value*, %%struct.Value*)\n\n");
    
    fprintf(gen->output, ";; Memory management functions (Reference Counting)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_retain(%%struct.Value*)\n");
    fprintf(gen->output, "declare void @value_release(%%struct.Value*)\n\n");
    
    fprintf(gen->output, ";; Input/Output functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_input(%%struct.Value*)\n\n");
    
    fprintf(gen->output, ";; Runtime state functions (internal use only)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_last_status()\n");
    fprintf(gen->output, "declare %%struct.Value* @value_last_error()\n");
    fprintf(gen->output, "declare %%struct.Value* @value_clear_error()\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_ok()\n");
    fprintf(gen->output, "declare void @value_fatal_error()\n");
    fprintf(gen->output, "declare i32 @value_needs_final_newline()\n");
    fprintf(gen->output, "declare %%struct.Value* @throwErr(%%struct.Value**, i32)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_sysinfo()\n\n");
    
    fprintf(gen->output, ";; External C library functions\n");
    fprintf(gen->output, "declare void @abort() noreturn\n");
    fprintf(gen->output, "declare i8* @malloc(i64)\n");
    fprintf(gen->output, "declare void @free(i8*)\n\n");
    
    fprintf(gen->output, ";; Type conversion functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_to_num(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_to_str(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_to_bl(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_to_int(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_to_float(%%struct.Value*)\n\n");
    
    fprintf(gen->output, ";; String manipulation functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_len(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_char_at(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_substr(%%struct.Value*, %%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_index_of(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_replace(%%struct.Value*, %%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_split(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_join(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_trim(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_upper(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_lower(%%struct.Value*)\n\n");
    
    fprintf(gen->output, ";; Array manipulation functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_push(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_pop(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_shift(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_unshift(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_slice(%%struct.Value*, %%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_concat(%%struct.Value*, %%struct.Value*)\n\n");
    
    fprintf(gen->output, ";; File I/O functions (Extended Object Types)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_read_file(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_write_file(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_append_file(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_file_exists(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_delete_file(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_get_file_size(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_read_bytes(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_write_bytes(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_read_lines(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_rename_file(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_copy_file(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_create_dir(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_remove_dir(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_list_dir(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_dir_exists(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_parse_json(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_to_json(%%struct.Value*)\n");
    
    fprintf(gen->output, ";; Math functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_abs(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_floor(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_ceil(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_round(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_round2(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_sqrt(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_pow(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_min(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_max(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_random()\n");
    
    fprintf(gen->output, ";; String enhancement functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_starts_with(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_ends_with(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_contains(%%struct.Value*, %%struct.Value*)\n");
    
    fprintf(gen->output, ";; Time functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_now()\n");
    fprintf(gen->output, "declare %%struct.Value* @value_time()\n");
    fprintf(gen->output, "declare %%struct.Value* @value_sleep(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_date()\n");
    
    fprintf(gen->output, ";; System functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_exit(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_get_env(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_set_env(%%struct.Value*, %%struct.Value*)\n");
    
    fprintf(gen->output, ";; Utility functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_nan(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_finite(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_clamp(%%struct.Value*, %%struct.Value*, %%struct.Value*)\n\n");

    // Error object creation and field access
    fprintf(gen->output, ";; Error object creation and field access\n");
    fprintf(gen->output, "declare %%struct.Value* @create_error_object(%%struct.Value*, %%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_get_field(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_get_field_safe(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_get_method(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_get_method_by_index(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_set_field(%%struct.Value*, %%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_delete_field(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_has_field(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_keys(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_values(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_entries(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_set_index(%%struct.Value*, %%struct.Value*, %%struct.Value*)\n");
    
    // Method binding support
    fprintf(gen->output, "\n;; Method binding support\n");
    fprintf(gen->output, "declare %%struct.Value* @bind_method(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @get_function_bound_self(%%struct.Value*)\n");
    
    // Object/Array clone functions
    fprintf(gen->output, "\n;; Object/Array clone functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_shallow_clone(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_deep_clone(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_spread_into_object(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_spread_into_array(%%struct.Value*, %%struct.Value*)\n\n");
    
    // Array extension functions
    fprintf(gen->output, ";; Array extension functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_reverse(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_index_of_array(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_includes(%%struct.Value*, %%struct.Value*)\n");
    
    // Higher-order array functions
    fprintf(gen->output, "\n;; Higher-order array functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_create_array(i64)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_sort(%%struct.Value*, %%struct.Value* (%%struct.Value*, %%struct.Value*)*)\n");
    
    // Type checking functions
    fprintf(gen->output, "\n;; Type checking functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_num(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_str(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_bl(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_arr(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_obj(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_null(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_undef(%%struct.Value*)\n");
    
    // Utility functions
    fprintf(gen->output, "\n;; Utility functions (range, assert)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_range(%%struct.Value*, %%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_assert(%%struct.Value*, %%struct.Value*)\n\n");
    
    // 4. 传统外部声明（保留向后兼容）
    fprintf(gen->output, "declare i32 @printf(i8*, ...)\n\n");
    
    // 5. 字符串常量（从strings_buf复制）
    rewind(gen->strings_buf);
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), gen->strings_buf)) {
        fputs(buffer, gen->output);
    }
    
    // 6. 全局声明（从globals_buf复制 - 嵌套函数定义等）
    rewind(gen->globals_buf);
    while (fgets(buffer, sizeof(buffer), gen->globals_buf)) {
        fputs(buffer, gen->output);
    }
    fprintf(gen->output, "\n");
    
    // 7. 函数定义（从code_buf复制）
    rewind(gen->code_buf);
    while (fgets(buffer, sizeof(buffer), gen->code_buf)) {
        fputs(buffer, gen->output);
    }
}
