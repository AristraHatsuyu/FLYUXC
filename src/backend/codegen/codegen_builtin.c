#include "codegen_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 内置函数处理 - 所有27个内置函数的代码生成
 * ============================================================================ */

char *codegen_builtin_call(CodeGen *gen, const char *func_name, ASTNode **args, size_t arg_count) {
    // print - 不换行，逗号分隔直接拼接
    if (strcmp(func_name, "print") == 0) {
        for (size_t i = 0; i < arg_count; i++) {
            char *arg = codegen_expr(gen, args[i]);
            fprintf(gen->code_buf, "  call void @value_print(%%struct.Value* %s)\n", arg);
            free(arg);
        }
        return NULL;  // void return
    }
    
    // println - 每个参数后换行
    if (strcmp(func_name, "println") == 0) {
        if (arg_count == 0) {
            // println() 无参数时只输出换行
            char *newline_label = new_string_label(gen);
            fprintf(gen->globals_buf, "%s = private unnamed_addr constant [2 x i8] c\"\\0A\\00\"\n", newline_label);
            fprintf(gen->code_buf, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* %s, i32 0, i32 0))\n", newline_label);
            free(newline_label);
        } else {
            for (size_t i = 0; i < arg_count; i++) {
                char *arg = codegen_expr(gen, args[i]);
                fprintf(gen->code_buf, "  call void @value_println(%%struct.Value* %s)\n", arg);
                free(arg);
            }
        }
        return NULL;  // void return
    }
    
    // printf - 格式化输出
    if (strcmp(func_name, "printf") == 0 && arg_count >= 1) {
        // 第一个参数是格式字符串
        char *fmt_arg = codegen_expr(gen, args[0]);
        
        // 创建参数数组
        if (arg_count > 1) {
            // 分配数组存储参数
            char *args_array = new_temp(gen);
            fprintf(gen->code_buf, "  %s = alloca [%zu x %%struct.Value*]\n", 
                    args_array, arg_count - 1);
            
            // 填充参数
            for (size_t i = 1; i < arg_count; i++) {
                char *arg = codegen_expr(gen, args[i]);
                char *elem_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 %zu\n",
                        elem_ptr, arg_count - 1, arg_count - 1, args_array, i - 1);
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", arg, elem_ptr);
                free(arg);
                free(elem_ptr);
            }
            
            // 转换为 Value** 并调用 value_printf
            char *args_ptr = new_temp(gen);
            fprintf(gen->code_buf, "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 0\n",
                    args_ptr, arg_count - 1, arg_count - 1, args_array);
            fprintf(gen->code_buf, "  call void @value_printf(%%struct.Value* %s, %%struct.Value** %s, i64 %zu)\n",
                    fmt_arg, args_ptr, arg_count - 1);
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
        return NULL;  // void return
    }
    
    // typeOf - 获取值的类型
    if (strcmp(func_name, "typeOf") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
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
    
    // input - 读取用户输入
    if (strcmp(func_name, "input") == 0) {
        char *result = new_temp(gen);
        
        if (arg_count == 0) {
            // 无提示符
            char *null_val = new_temp(gen);
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", null_val);
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_input(%%struct.Value* %s)\n", result, null_val);
            free(null_val);
        } else {
            // 有提示符
            char *prompt = codegen_expr(gen, args[0]);
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_input(%%struct.Value* %s)\n", result, prompt);
            free(prompt);
        }
        
        return result;
    }
    
    // toNum - 转换为数字
    if (strcmp(func_name, "toNum") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_num(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // toStr - 转换为字符串
    if (strcmp(func_name, "toStr") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_str(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // toBl - 转换为布尔
    if (strcmp(func_name, "toBl") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_bl(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // toInt - 转换为整数
    if (strcmp(func_name, "toInt") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_int(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // toFloat - 转换为浮点数
    if (strcmp(func_name, "toFloat") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_to_float(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // len - 获取长度
    if (strcmp(func_name, "len") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_len(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // charAt - 获取字符或数组元素
    if (strcmp(func_name, "charAt") == 0 && arg_count == 2) {
        char *str = codegen_expr(gen, args[0]);
        char *idx = codegen_expr(gen, args[1]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_char_at(%%struct.Value* %s, %%struct.Value* %s)\n", result, str, idx);
        free(str);
        free(idx);
        return result;
    }
    
    // substr - 子字符串
    if (strcmp(func_name, "substr") == 0 && (arg_count == 2 || arg_count == 3)) {
        char *str = codegen_expr(gen, args[0]);
        char *start = codegen_expr(gen, args[1]);
        char *len = arg_count == 3 ? codegen_expr(gen, args[2]) : NULL;
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
    
    // indexOf - 查找子字符串
    if (strcmp(func_name, "indexOf") == 0 && arg_count == 2) {
        char *str = codegen_expr(gen, args[0]);
        char *substr = codegen_expr(gen, args[1]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_index_of(%%struct.Value* %s, %%struct.Value* %s)\n", result, str, substr);
        free(str);
        free(substr);
        return result;
    }
    
    // replace - 替换字符串
    if (strcmp(func_name, "replace") == 0 && arg_count == 3) {
        char *str = codegen_expr(gen, args[0]);
        char *old = codegen_expr(gen, args[1]);
        char *new = codegen_expr(gen, args[2]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_replace(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n", result, str, old, new);
        free(str);
        free(old);
        free(new);
        return result;
    }
    
    // split - 分割字符串
    if (strcmp(func_name, "split") == 0 && (arg_count == 1 || arg_count == 2)) {
        char *str = codegen_expr(gen, args[0]);
        char *delim = arg_count == 2 ? codegen_expr(gen, args[1]) : NULL;
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
    
    // join - 连接数组为字符串
    if (strcmp(func_name, "join") == 0 && (arg_count == 1 || arg_count == 2)) {
        char *arr = codegen_expr(gen, args[0]);
        char *sep = arg_count == 2 ? codegen_expr(gen, args[1]) : NULL;
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
    
    // trim - 去除首尾空白
    if (strcmp(func_name, "trim") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_trim(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // upper - 转换为大写
    if (strcmp(func_name, "upper") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_upper(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // lower - 转换为小写
    if (strcmp(func_name, "lower") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_lower(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // push - 数组添加元素
    if (strcmp(func_name, "push") == 0 && arg_count == 2) {
        char *arr = codegen_expr(gen, args[0]);
        char *val = codegen_expr(gen, args[1]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_push(%%struct.Value* %s, %%struct.Value* %s)\n", result, arr, val);
        free(arr);
        free(val);
        return result;
    }
    
    // pop - 数组删除最后一个元素
    if (strcmp(func_name, "pop") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_pop(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // shift - 数组删除第一个元素
    if (strcmp(func_name, "shift") == 0 && arg_count == 1) {
        char *arg = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_shift(%%struct.Value* %s)\n", result, arg);
        free(arg);
        return result;
    }
    
    // unshift - 数组前插入元素
    if (strcmp(func_name, "unshift") == 0 && arg_count == 2) {
        char *arr = codegen_expr(gen, args[0]);
        char *val = codegen_expr(gen, args[1]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_unshift(%%struct.Value* %s, %%struct.Value* %s)\n", result, arr, val);
        free(arr);
        free(val);
        return result;
    }
    
    // slice - 数组切片
    if (strcmp(func_name, "slice") == 0 && (arg_count >= 1 && arg_count <= 3)) {
        char *arr = codegen_expr(gen, args[0]);
        char *start = arg_count >= 2 ? codegen_expr(gen, args[1]) : NULL;
        char *end = arg_count == 3 ? codegen_expr(gen, args[2]) : NULL;
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
    
    // concat - 连接两个数组
    if (strcmp(func_name, "concat") == 0 && arg_count == 2) {
        char *arr1 = codegen_expr(gen, args[0]);
        char *arr2 = codegen_expr(gen, args[1]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_concat(%%struct.Value* %s, %%struct.Value* %s)\n", result, arr1, arr2);
        free(arr1);
        free(arr2);
        return result;
    }
    
    // length - 获取数组长度（向后兼容）
    if (strcmp(func_name, "length") == 0 && arg_count == 1) {
        // length(arr) 应该返回数组的长度
        // 参数应该是 IDENTIFIER
        if (args[0]->kind == AST_IDENTIFIER) {
            ASTIdentifier *arr_ident = (ASTIdentifier *)args[0]->data;
            ArrayMetadata *meta = find_array(gen, arr_ident->name);
            
            if (meta) {
                // 返回数组长度作为 Value*
                char *result = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %zu.0)  ; length of %s\n",
                        result, meta->elem_count, arr_ident->name);
                return result;
            }
        }
        
        // 如果不是数组，返回 0
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double 0.0)  ; length of non-array\n", result);
        return result;
    }
    
    // setField - 动态设置对象字段
    if (strcmp(func_name, "setField") == 0 && arg_count == 3) {
        char *obj = codegen_expr(gen, args[0]);
        char *field = codegen_expr(gen, args[1]);
        char *value = codegen_expr(gen, args[2]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_set_field(%%struct.Value* %s, %%struct.Value* %s, %%struct.Value* %s)\n", result, obj, field, value);
        free(obj);
        free(field);
        free(value);
        return result;
    }
    
    // deleteField - 删除对象字段
    if (strcmp(func_name, "deleteField") == 0 && arg_count == 2) {
        char *obj = codegen_expr(gen, args[0]);
        char *field = codegen_expr(gen, args[1]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_delete_field(%%struct.Value* %s, %%struct.Value* %s)\n", result, obj, field);
        free(obj);
        free(field);
        return result;
    }
    
    // hasField - 检查字段是否存在
    if (strcmp(func_name, "hasField") == 0 && arg_count == 2) {
        char *obj = codegen_expr(gen, args[0]);
        char *field = codegen_expr(gen, args[1]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_has_field(%%struct.Value* %s, %%struct.Value* %s)\n", result, obj, field);
        free(obj);
        free(field);
        return result;
    }
    
    // keys - 获取对象的所有键名
    if (strcmp(func_name, "keys") == 0 && arg_count == 1) {
        char *obj = codegen_expr(gen, args[0]);
        char *result = new_temp(gen);
        fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_keys(%%struct.Value* %s)\n", result, obj);
        free(obj);
        return result;
    }
    
    // 不是内置函数，返回NULL表示需要普通函数调用
    return NULL;
}
