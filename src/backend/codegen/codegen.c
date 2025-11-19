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

/* 转义字符串用于LLVM IR输出 - 支持包含\0的字符串 */
static char *escape_for_ir(const char *str, size_t in_len, size_t* out_len) {
    size_t len = in_len;
    // 最坏情况：每个字符都需要转义为 \xx 格式 (3字节)
    char *escaped = (char *)malloc(len * 3 + 1);
    char *out = escaped;
    
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (c == '\\') {
            *out++ = '\\';
            *out++ = '\\';
        } else if (c == '"') {
            *out++ = '\\';
            *out++ = '2';
            *out++ = '2';
        } else if (c == '\n') {
            *out++ = '\\';
            *out++ = '0';
            *out++ = 'A';
        } else if (c == '\r') {
            *out++ = '\\';
            *out++ = '0';
            *out++ = 'D';
        } else if (c == '\t') {
            *out++ = '\\';
            *out++ = '0';
            *out++ = '9';
        } else if (c < 32 || c >= 127) {
            // 非打印字符转为十六进制
            out += sprintf(out, "\\%02X", c);
        } else {
            *out++ = c;
        }
    }
    *out = '\0';
    if (out_len) *out_len = out - escaped;  /* 返回转义后的实际长度 */
    return escaped;
}

/* 注册数组元数据 */
static void register_array(CodeGen *gen, const char *var_name, const char *array_ptr, size_t elem_count) {
    ArrayMetadata *meta = (ArrayMetadata *)malloc(sizeof(ArrayMetadata));
    meta->var_name = strdup(var_name);
    meta->array_ptr = strdup(array_ptr);
    meta->elem_count = elem_count;
    meta->next = gen->arrays;
    gen->arrays = meta;
}

/* 查找数组元数据 */
static ArrayMetadata *find_array(CodeGen *gen, const char *var_name) {
    for (ArrayMetadata *meta = gen->arrays; meta != NULL; meta = meta->next) {
        if (strcmp(meta->var_name, var_name) == 0) {
            return meta;
        }
    }
    return NULL;
}

/* 注册对象元数据 */
static void register_object(CodeGen *gen, const char *var_name, ObjectField *fields) {
    ObjectMetadata *meta = (ObjectMetadata *)malloc(sizeof(ObjectMetadata));
    meta->var_name = strdup(var_name);
    meta->fields = fields;
    meta->next = gen->objects;
    gen->objects = meta;
}

/* 查找对象元数据 */
static ObjectMetadata *find_object(CodeGen *gen, const char *var_name) {
    for (ObjectMetadata *meta = gen->objects; meta != NULL; meta = meta->next) {
        if (strcmp(meta->var_name, var_name) == 0) {
            return meta;
        }
    }
    return NULL;
}

/* 在对象中查找字段 */
static ObjectField *find_field(ObjectMetadata *obj_meta, const char *field_name) {
    for (ObjectField *field = obj_meta->fields; field != NULL; field = field->next) {
        if (strcmp(field->field_name, field_name) == 0) {
            return field;
        }
    }
    return NULL;
}

/* 注册变量到符号表 */
static void register_symbol(CodeGen *gen, const char *var_name) {
    SymbolEntry *entry = (SymbolEntry *)malloc(sizeof(SymbolEntry));
    entry->name = strdup(var_name);
    entry->next = gen->symbols;
    gen->symbols = entry;
}

/* 检查变量是否已定义 */
static int is_symbol_defined(CodeGen *gen, const char *var_name) {
    for (SymbolEntry *entry = gen->symbols; entry != NULL; entry = entry->next) {
        if (strcmp(entry->name, var_name) == 0) {
            return 1;
        }
    }
    return 0;
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
    gen->arrays = NULL;
    gen->objects = NULL;
    gen->symbols = NULL;
    gen->current_var_name = NULL;
    
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
 * 表达式代码生成
 * ============================================================================ */

static char *codegen_expr(CodeGen *gen, ASTNode *node) {
    if (!node) return NULL;
    
    switch (node->kind) {
        case AST_NUM_LITERAL: {
            ASTNumLiteral *num = (ASTNumLiteral *)node->data;
            char *temp = new_temp(gen);
            
            // 使用 box_number 将数字装箱为 Value*
            fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %f)\n", 
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
            
            // 特殊处理 length 函数
            if (strcmp(callee->name, "length") == 0 && call->arg_count == 1) {
                // length(arr) 应该返回数组的长度
                // 参数应该是 IDENTIFIER
                if (call->args[0]->kind == AST_IDENTIFIER) {
                    ASTIdentifier *arr_ident = (ASTIdentifier *)call->args[0]->data;
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
            // 实现对象字面量：为每个属性分配一个 Value*
            ASTObjectLiteral *obj_lit = (ASTObjectLiteral *)node->data;
            size_t prop_count = obj_lit->prop_count;
            
            if (prop_count == 0) {
                // 空对象：返回 null
                char *temp = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()  ; empty object\n", temp);
                return temp;
            }
            
            // 为每个属性创建一个 Value* 变量并初始化
            ObjectField *fields = NULL;
            ObjectField *last_field = NULL;
            
            for (size_t i = 0; i < prop_count; i++) {
                ASTObjectProperty *prop = &obj_lit->properties[i];
                
                // 分配属性的存储空间
                char *field_ptr = new_temp(gen);
                fprintf(gen->code_buf, "  %s = alloca %%struct.Value*  ; object field '%s'\n", 
                        field_ptr, prop->key);
                
                // 计算并存储属性值
                char *value = codegen_expr(gen, prop->value);
                if (value) {
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s\n", 
                            value, field_ptr);
                    free(value);
                }
                
                // 添加到字段链表
                ObjectField *field = (ObjectField *)malloc(sizeof(ObjectField));
                field->field_name = strdup(prop->key);
                field->field_ptr = strdup(field_ptr);
                field->next = NULL;
                
                if (!fields) {
                    fields = field;
                    last_field = field;
                } else {
                    last_field->next = field;
                    last_field = field;
                }
            }
            
            // 如果我们知道这个对象要赋给哪个变量，就注册它
            if (gen->current_var_name) {
                register_object(gen, gen->current_var_name, fields);
            }
            
            // 返回第一个属性的值作为占位符
            char *result = new_temp(gen);
            if (fields && fields->field_ptr) {
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s  ; object placeholder return\n",
                        result, fields->field_ptr);
            } else {
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()  ; empty object\n", result);
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
            // 实现对象成员访问：obj.prop
            ASTMemberExpr *member = (ASTMemberExpr *)node->data;
            
            // 获取对象（应该是标识符）
            if (member->object->kind != AST_IDENTIFIER) {
                char *temp = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()  ; member expr object not identifier\n", temp);
                return temp;
            }
            
            ASTIdentifier *obj_ident = (ASTIdentifier *)member->object->data;
            const char *obj_name = obj_ident->name;
            
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
            
            // 查找对象元数据
            ObjectMetadata *obj_meta = find_object(gen, obj_name);
            if (!obj_meta) {
                char *temp = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()  ; object '%s' not found\n", 
                        temp, obj_name);
                return temp;
            }
            
            // 查找字段
            ObjectField *field = find_field(obj_meta, member->property);
            if (!field) {
                char *temp = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()  ; field '%s' not found in object '%s'\n",
                        temp, member->property, obj_name);
                return temp;
            }
            
            // 加载字段值
            char *result = new_temp(gen);
            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %s  ; %s.%s\n",
                    result, field->field_ptr, obj_name, member->property);
            
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
 * 语句代码生成
 * ============================================================================ */

static void codegen_stmt(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->kind) {
        case AST_VAR_DECL: {
            ASTVarDecl *decl = (ASTVarDecl *)node->data;
            
            // 注册变量到符号表
            register_symbol(gen, decl->name);
            
            // 分配栈空间存储 Value*
            fprintf(gen->code_buf, "  %%%s = alloca %%struct.Value*\n", decl->name);
            
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
                    
                    char *init_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null_typed(i32 %d)\n",
                            init_val, type_code);
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n",
                            init_val, decl->name);
                    free(init_val);
                } else {
                    // 普通初始化
                    gen->current_var_name = decl->name;
                    char *init_val = codegen_expr(gen, decl->init_expr);
                    gen->current_var_name = NULL;
                    
                    if (init_val) {
                        fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n",
                                init_val, decl->name);
                        free(init_val);
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
                
                // 如果变量未定义，先分配空间并注册
                if (!is_symbol_defined(gen, target->name)) {
                    register_symbol(gen, target->name);
                    fprintf(gen->code_buf, "  %%%s = alloca %%struct.Value*\n", target->name);
                }
                
                // 如果赋值的是null，需要保持变量的declared_type
                if (assign->value->kind == AST_NULL_LITERAL) {
                    // 先加载旧值获取declared_type，然后用box_null_preserve_type创建新null
                    char *old_val = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                            old_val, target->name);
                    
                    char *new_null = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null_preserve_type(%%struct.Value* %s)\n",
                            new_null, old_val);
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n",
                            new_null, target->name);
                    
                    free(old_val);
                    free(new_null);
                } else {
                    // 普通赋值
                    value = codegen_expr(gen, assign->value);
                    if (!value) break;
                    fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %%%s\n",
                            value, target->name);
                }
            }
            else if (assign->target->kind == AST_INDEX_EXPR) {
                // 数组索引赋值: arr[i] = 10
                value = codegen_expr(gen, assign->value);
                if (!value) break;
                
                ASTIndexExpr *idx_expr = (ASTIndexExpr *)assign->target->data;
                
                // 获取数组变量
                if (idx_expr->object->kind != AST_IDENTIFIER) {
                    fprintf(gen->code_buf, "  ; ERROR: array index assignment target not identifier\n");
                    free(value);
                    break;
                }
                
                ASTIdentifier *arr_ident = (ASTIdentifier *)idx_expr->object->data;
                const char *arr_name = arr_ident->name;
                
                // 查找数组元数据
                ArrayMetadata *meta = find_array(gen, arr_name);
                if (!meta) {
                    fprintf(gen->code_buf, "  ; ERROR: array '%s' not found for assignment\n", arr_name);
                    free(value);
                    break;
                }
                
                // 计算索引
                char *index_val = codegen_expr(gen, idx_expr->index);
                char *index_double = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call double @unbox_number(%%struct.Value* %s)\n",
                        index_double, index_val);
                char *index_i64 = new_temp(gen);
                fprintf(gen->code_buf, "  %s = fptosi double %s to i64\n", 
                        index_i64, index_double);
                
                // 获取元素地址
                char *elem_ptr = new_temp(gen);
                fprintf(gen->code_buf, 
                        "  %s = getelementptr inbounds [%zu x %%struct.Value*], [%zu x %%struct.Value*]* %s, i64 0, i64 %s\n",
                        elem_ptr, meta->elem_count, meta->elem_count, 
                        meta->array_ptr, index_i64);
                
                // 存储值
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s  ; arr[%s] = value\n",
                        value, elem_ptr, index_i64);
                
                free(index_val);
            }
            else if (assign->target->kind == AST_MEMBER_EXPR) {
                // 对象成员赋值: obj.prop = 10
                ASTMemberExpr *member = (ASTMemberExpr *)assign->target->data;
                
                // 获取对象变量
                if (member->object->kind != AST_IDENTIFIER) {
                    fprintf(gen->code_buf, "  ; ERROR: member assignment target not identifier\n");
                    free(value);
                    break;
                }
                
                ASTIdentifier *obj_ident = (ASTIdentifier *)member->object->data;
                const char *obj_name = obj_ident->name;
                
                // 查找对象元数据
                ObjectMetadata *obj_meta = find_object(gen, obj_name);
                if (!obj_meta) {
                    fprintf(gen->code_buf, "  ; ERROR: object '%s' not found for assignment\n", obj_name);
                    free(value);
                    break;
                }
                
                // 查找字段
                ObjectField *field = find_field(obj_meta, member->property);
                if (!field) {
                    fprintf(gen->code_buf, "  ; ERROR: field '%s' not found in object '%s'\n",
                            member->property, obj_name);
                    free(value);
                    break;
                }
                
                // 存储值
                fprintf(gen->code_buf, "  store %%struct.Value* %s, %%struct.Value** %s  ; %s.%s = value\n",
                        value, field->field_ptr, obj_name, member->property);
            }
            else {
                fprintf(gen->code_buf, "  ; ERROR: unsupported assignment target\n");
            }
            
            if (value) free(value);
            break;
        }
        
        case AST_RETURN_STMT: {
            ASTReturnStmt *ret = (ASTReturnStmt *)node->data;
            
            if (ret->value) {
                char *ret_val = codegen_expr(gen, ret->value);
                if (ret_val) {
                    fprintf(gen->code_buf, "  ret %%struct.Value* %s\n", ret_val);
                    free(ret_val);
                } else {
                    char *null_ret = new_temp(gen);
                    fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", null_ret);
                    fprintf(gen->code_buf, "  ret %%struct.Value* %s\n", null_ret);
                    free(null_ret);
                }
            } else {
                char *null_ret = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_null()\n", null_ret);
                fprintf(gen->code_buf, "  ret %%struct.Value* %s\n", null_ret);
                free(null_ret);
            }
            break;
        }
        
        case AST_IF_STMT: {
            ASTIfStmt *ifstmt = (ASTIfStmt *)node->data;
            
            if (ifstmt->cond_count > 0) {
                char *cond = codegen_expr(gen, ifstmt->conditions[0]);
                
                // 使用 value_is_truthy 将 Value* 转换为 i32
                char *truthy = new_temp(gen);
                fprintf(gen->code_buf, "  %s = call i32 @value_is_truthy(%%struct.Value* %s)\n", 
                        truthy, cond);
                
                // 转换为 i1
                char *cond_bool = new_temp(gen);
                fprintf(gen->code_buf, "  %s = icmp ne i32 %s, 0\n", cond_bool, truthy);
                
                char *then_label = new_label(gen);
                char *else_label = new_label(gen);
                char *end_label = new_label(gen);
                
                // 条件跳转
                fprintf(gen->code_buf, "  br i1 %s, label %%%s, label %%%s\n",
                        cond_bool, then_label, else_label);
                
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
                free(truthy);
                free(cond_bool);
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
            
            // 函数签名 - 使用 Value* 类型
            fprintf(gen->code_buf, "\ndefine %%struct.Value* @%s(", func->name);
            
            for (size_t i = 0; i < func->param_count; i++) {
                if (i > 0) fprintf(gen->code_buf, ", ");
                fprintf(gen->code_buf, "%%struct.Value* %%param_%s", func->params[i]);
            }
            
            fprintf(gen->code_buf, ") {\n");
            
            // 为参数创建局部变量并存储参数值
            for (size_t i = 0; i < func->param_count; i++) {
                fprintf(gen->code_buf, "  %%%s = alloca %%struct.Value*\n", func->params[i]);
                fprintf(gen->code_buf, "  store %%struct.Value* %%param_%s, %%struct.Value** %%%s\n",
                        func->params[i], func->params[i]);
            }
            
            // 函数体
            if (func->body) {
                codegen_stmt(gen, func->body);
            }
            
            // 如果没有显式返回，添加默认返回 null
            fprintf(gen->code_buf, "  %%default_ret = call %%struct.Value* @box_null()\n");
            fprintf(gen->code_buf, "  ret %%struct.Value* %%default_ret\n");
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
    
    // 2. Value 结构体定义
    fprintf(gen->output, ";; Mixed-type value system\n");
    fprintf(gen->output, "%%struct.Value = type { i32, [12 x i8] }\n\n");
    
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
    fprintf(gen->output, "declare %%struct.Value* @box_array(i8*, i64)\n\n");
    
    fprintf(gen->output, ";; Unboxing functions\n");
    fprintf(gen->output, "declare double @unbox_number(%%struct.Value*)\n");
    fprintf(gen->output, "declare i8* @unbox_string(%%struct.Value*)\n\n");
    
    fprintf(gen->output, ";; Utility functions\n");
    fprintf(gen->output, "declare i32 @value_is_truthy(%%struct.Value*)\n");
    fprintf(gen->output, "declare void @value_print(%%struct.Value*)\n");
    fprintf(gen->output, "declare void @value_println(%%struct.Value*)\n");
    fprintf(gen->output, "declare i8* @value_typeof(%%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_add(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_subtract(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_multiply(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_divide(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_power(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_equals(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_less_than(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_greater_than(%%struct.Value*, %%struct.Value*)\n");
    fprintf(gen->output, "declare %%struct.Value* @value_index(%%struct.Value*, %%struct.Value*)\n\n");
    
    // 4. 传统外部声明（保留向后兼容）
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
