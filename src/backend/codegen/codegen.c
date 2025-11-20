#include "codegen_internal.h"
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
    gen->arrays = NULL;
    gen->objects = NULL;
    gen->symbols = NULL;
    gen->current_var_name = NULL;
    gen->in_try_catch = 0;  /* 初始不在 Try-Catch 块中 */
    
    return gen;
}

void codegen_free(CodeGen *gen) {
    if (gen) {
        if (gen->globals_buf) fclose(gen->globals_buf);
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
        
        free(gen);
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
        
        // 生成 main 函数包装器
        // 如果用户定义了main函数,调用它;否则执行顶层语句
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
        
        // 始终创建 i32 @main() 作为程序入口
        fprintf(gen->code_buf, "\ndefine i32 @main() {\n");
        
        if (has_main) {
            // 调用用户定义的main函数 (重命名为_flyux_main)
            fprintf(gen->code_buf, "  %%user_main_result = call %%struct.Value* @_flyux_main()\n");
        } else {
            // 预扫描：收集所有需要在entry块alloca的变量（catch参数等）
            FILE *entry_alloca_buf = tmpfile();
            for (size_t i = 0; i < prog->stmt_count; i++) {
                if (prog->statements[i]->kind != AST_FUNC_DECL) {
                    collect_catch_params(gen, prog->statements[i], entry_alloca_buf);
                }
            }
            
            // 写入entry块的alloca语句
            rewind(entry_alloca_buf);
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), entry_alloca_buf)) {
                fputs(buffer, gen->code_buf);
            }
            fclose(entry_alloca_buf);
            
            // 执行所有顶层语句
            for (size_t i = 0; i < prog->stmt_count; i++) {
                if (prog->statements[i]->kind != AST_FUNC_DECL) {
                    codegen_stmt(gen, prog->statements[i]);
                }
            }
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
    fprintf(gen->output, "@.str.newline = private unnamed_addr constant [2 x i8] c\"\\0A\\00\"\n\n");
    
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
    fprintf(gen->output, "declare %%struct.Value* @box_object(i8*, i64)\n\n");
    
    fprintf(gen->output, ";; Unboxing functions\n");
    fprintf(gen->output, "declare double @unbox_number(%%struct.Value*)\n");
    fprintf(gen->output, "declare i8* @unbox_string(%%struct.Value*)\n\n");
    
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
    fprintf(gen->output, "declare %%struct.Value* @value_equals(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_less_than(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_greater_than(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_index(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare i64 @value_array_length(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_array_get(%%struct.Value*, %%struct.Value*)\n\n");
    
    fprintf(gen->output, ";; Input/Output functions\n");
    fprintf(gen->output, "declare %%struct.Value* @value_input(%%struct.Value*)\n\n");
    
    fprintf(gen->output, ";; Runtime state functions (internal use only)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_last_status()\n");
    fprintf(gen->output, "declare %%struct.Value* @value_last_error()\n");
    fprintf(gen->output, "declare %%struct.Value* @value_clear_error()\n");
    fprintf(gen->output, "declare %%struct.Value* @value_is_ok()\n");
    fprintf(gen->output, "declare void @value_fatal_error()\n");
    fprintf(gen->output, "declare i32 @value_needs_final_newline()\n\n");
    
    fprintf(gen->output, ";; External C library functions\n");
    fprintf(gen->output, "declare void @abort() noreturn\n\n");
    
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
    fprintf(gen->output, "declare %%struct.Value* @value_set_field(%%struct.Value*, %%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_delete_field(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_has_field(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_keys(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_set_index(%%struct.Value*, %%struct.Value*, %%struct.Value*)\n\n");
    
    // 4. 传统外部声明（保留向后兼容）
    fprintf(gen->output, "declare i32 @printf(i8*, ...)\n\n");
    
    // 5. 全局常量（从globals_buf复制）
    rewind(gen->globals_buf);
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), gen->globals_buf)) {
        fputs(buffer, gen->output);
    }
    fprintf(gen->output, "\n");
    
    // 6. 函数定义（从code_buf复制）
    rewind(gen->code_buf);
    while (fgets(buffer, sizeof(buffer), gen->code_buf)) {
        fputs(buffer, gen->output);
    }
}
