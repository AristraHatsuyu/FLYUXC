#include "codegen_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 辅助工具函数实现
 * ============================================================================ */

char *new_temp(CodeGen *gen) {
    char *temp = (char *)malloc(32);
    snprintf(temp, 32, "%%t%d", gen->temp_count++);
    return temp;
}

char *new_label(CodeGen *gen) {
    char *label = (char *)malloc(32);
    snprintf(label, 32, "label%d", gen->label_count++);
    return label;
}

char *new_string_label(CodeGen *gen) {
    char *label = (char *)malloc(32);
    snprintf(label, 32, "@.str.%d", gen->string_count++);
    return label;
}

/* 转义字符串用于LLVM IR输出 - 支持包含\0的字符串 */
char *escape_for_ir(const char *str, size_t in_len, size_t* out_len) {
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
void register_array(CodeGen *gen, const char *var_name, const char *array_ptr, size_t elem_count) {
    ArrayMetadata *meta = (ArrayMetadata *)malloc(sizeof(ArrayMetadata));
    meta->var_name = strdup(var_name);
    meta->array_ptr = strdup(array_ptr);
    meta->elem_count = elem_count;
    meta->next = gen->arrays;
    gen->arrays = meta;
}

/* 查找数组元数据 */
ArrayMetadata *find_array(CodeGen *gen, const char *var_name) {
    for (ArrayMetadata *meta = gen->arrays; meta != NULL; meta = meta->next) {
        if (strcmp(meta->var_name, var_name) == 0) {
            return meta;
        }
    }
    return NULL;
}

/* 注册对象元数据 */
void register_object(CodeGen *gen, const char *var_name, ObjectField *fields) {
    ObjectMetadata *meta = (ObjectMetadata *)malloc(sizeof(ObjectMetadata));
    meta->var_name = strdup(var_name);
    meta->fields = fields;
    meta->next = gen->objects;
    gen->objects = meta;
}

/* 查找对象元数据 */
ObjectMetadata *find_object(CodeGen *gen, const char *var_name) {
    for (ObjectMetadata *meta = gen->objects; meta != NULL; meta = meta->next) {
        if (strcmp(meta->var_name, var_name) == 0) {
            return meta;
        }
    }
    return NULL;
}

/* 在对象中查找字段 */
ObjectField *find_field(ObjectMetadata *obj_meta, const char *field_name) {
    for (ObjectField *field = obj_meta->fields; field != NULL; field = field->next) {
        if (strcmp(field->field_name, field_name) == 0) {
            return field;
        }
    }
    return NULL;
}

/* 注册变量到符号表 */
void register_symbol(CodeGen *gen, const char *var_name) {
    SymbolEntry *entry = (SymbolEntry *)malloc(sizeof(SymbolEntry));
    entry->name = strdup(var_name);
    entry->next = gen->symbols;
    gen->symbols = entry;
}

/* 检查变量是否已定义 */
int is_symbol_defined(CodeGen *gen, const char *var_name) {
    for (SymbolEntry *entry = gen->symbols; entry != NULL; entry = entry->next) {
        if (strcmp(entry->name, var_name) == 0) {
            return 1;
        }
    }
    return 0;
}

/* ============================================================================
 * 作用域跟踪函数实现 (P2: 作用域退出清理)
 * ============================================================================ */

/* 创建作用域跟踪器 */
ScopeTracker *scope_tracker_create(void) {
    ScopeTracker *scope = (ScopeTracker *)malloc(sizeof(ScopeTracker));
    scope->locals = NULL;
    return scope;
}

/* 销毁作用域跟踪器 */
void scope_tracker_free(ScopeTracker *scope) {
    if (!scope) return;
    
    LocalVarEntry *entry = scope->locals;
    while (entry) {
        LocalVarEntry *next = entry->next;
        free(entry->name);
        free(entry);
        entry = next;
    }
    free(scope);
}

/* 添加局部变量到作用域跟踪器 */
void scope_add_local(ScopeTracker *scope, const char *var_name) {
    if (!scope || !var_name) return;
    
    // 检查变量是否已在列表中（避免重复添加）
    for (LocalVarEntry *e = scope->locals; e != NULL; e = e->next) {
        if (strcmp(e->name, var_name) == 0) {
            return;  // 已存在，不重复添加
        }
    }
    
    LocalVarEntry *entry = (LocalVarEntry *)malloc(sizeof(LocalVarEntry));
    entry->name = strdup(var_name);
    entry->next = scope->locals;
    scope->locals = entry;
}

/* 生成作用域退出清理代码 - 释放所有局部变量 */
void scope_generate_cleanup(CodeGen *gen, ScopeTracker *scope) {
    scope_generate_cleanup_except(gen, scope, NULL);
}

/* 生成作用域退出清理代码，但排除指定变量（用于返回值保护） */
void scope_generate_cleanup_except(CodeGen *gen, ScopeTracker *scope, const char *except_var) {
    if (!gen || !scope) return;
    
    fprintf(gen->code_buf, "  ; === P2 Scope Cleanup Start ===\n");
    
    for (LocalVarEntry *entry = scope->locals; entry != NULL; entry = entry->next) {
        // 如果这个变量是排除变量，跳过
        if (except_var && strcmp(entry->name, except_var) == 0) {
            fprintf(gen->code_buf, "  ; skip release for return value: %s\n", entry->name);
            continue;
        }
        
        // 加载变量值并释放
        char *val = new_temp(gen);
        fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                val, entry->name);
        fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", val);
        free(val);
    }
    
    fprintf(gen->code_buf, "  ; === P2 Scope Cleanup End ===\n");
}

/* ============================================================================
 * 循环作用域函数实现 (Break/Next 清理)
 * ============================================================================ */

/* 进入循环 - 压入新的循环作用域 */
void loop_scope_push(CodeGen *gen, const char *loop_end_label, const char *loop_continue_label, const char *label) {
    if (!gen) return;
    
    LoopScopeEntry *entry = (LoopScopeEntry *)malloc(sizeof(LoopScopeEntry));
    entry->loop_scope = scope_tracker_create();
    entry->loop_end_label = strdup(loop_end_label);
    entry->loop_continue_label = strdup(loop_continue_label);
    entry->label = label ? strdup(label) : NULL;
    entry->outer = gen->loop_scope_stack;
    gen->loop_scope_stack = entry;
}

/* 退出循环 - 弹出循环作用域 */
void loop_scope_pop(CodeGen *gen) {
    if (!gen || !gen->loop_scope_stack) return;
    
    LoopScopeEntry *entry = gen->loop_scope_stack;
    gen->loop_scope_stack = entry->outer;
    
    // 释放这个循环作用域的资源
    scope_tracker_free(entry->loop_scope);
    free(entry->loop_end_label);
    free(entry->loop_continue_label);
    if (entry->label) free(entry->label);
    free(entry);
}

/* 添加变量到当前循环作用域 */
void loop_scope_add_var(CodeGen *gen, const char *var_name) {
    if (!gen || !var_name) return;
    
    // 如果在循环中，添加到循环作用域
    if (gen->loop_scope_stack) {
        scope_add_local(gen->loop_scope_stack->loop_scope, var_name);
    }
}

/* 为 break 生成循环作用域清理代码 */
void loop_scope_generate_break_cleanup(CodeGen *gen) {
    if (!gen || !gen->loop_scope_stack) return;
    
    // 只清理当前循环作用域内的变量（不清理外层循环的变量）
    ScopeTracker *loop_scope = gen->loop_scope_stack->loop_scope;
    if (loop_scope && loop_scope->locals) {
        fprintf(gen->code_buf, "  ; === Break Loop Cleanup Start ===\n");
        
        for (LocalVarEntry *entry = loop_scope->locals; entry != NULL; entry = entry->next) {
            char *val = new_temp(gen);
            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                    val, entry->name);
            fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", val);
            free(val);
        }
        
        fprintf(gen->code_buf, "  ; === Break Loop Cleanup End ===\n");
    }
}

/* 为 next (continue) 生成循环作用域清理代码 */
void loop_scope_generate_next_cleanup(CodeGen *gen) {
    if (!gen || !gen->loop_scope_stack) return;
    
    // 只清理当前循环作用域内的变量（不清理外层循环的变量）
    // next 和 break 的清理逻辑相同，但跳转目标不同
    ScopeTracker *loop_scope = gen->loop_scope_stack->loop_scope;
    if (loop_scope && loop_scope->locals) {
        fprintf(gen->code_buf, "  ; === Next Loop Cleanup Start ===\n");
        
        for (LocalVarEntry *entry = loop_scope->locals; entry != NULL; entry = entry->next) {
            char *val = new_temp(gen);
            fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                    val, entry->name);
            fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", val);
            free(val);
        }
        
        fprintf(gen->code_buf, "  ; === Next Loop Cleanup End ===\n");
    }
}

/* 按标签查找目标循环，返回目标循环的 break 标签 */
const char *loop_scope_find_break_label(CodeGen *gen, const char *target_label) {
    if (!gen || !target_label) return NULL;
    
    for (LoopScopeEntry *entry = gen->loop_scope_stack; entry != NULL; entry = entry->outer) {
        if (entry->label && strcmp(entry->label, target_label) == 0) {
            return entry->loop_end_label;
        }
    }
    return NULL;  // 未找到标签
}

/* 按标签查找目标循环，返回目标循环的 continue 标签 */
const char *loop_scope_find_continue_label(CodeGen *gen, const char *target_label) {
    if (!gen || !target_label) return NULL;
    
    for (LoopScopeEntry *entry = gen->loop_scope_stack; entry != NULL; entry = entry->outer) {
        if (entry->label && strcmp(entry->label, target_label) == 0) {
            return entry->loop_continue_label;
        }
    }
    return NULL;  // 未找到标签
}

/* 生成跨层 break 清理代码（清理从当前到目标循环之间的所有循环作用域） */
void loop_scope_generate_multilevel_break_cleanup(CodeGen *gen, const char *target_label) {
    if (!gen || !gen->loop_scope_stack || !target_label) return;
    
    fprintf(gen->code_buf, "  ; === Multilevel Break Cleanup Start (target: %s) ===\n", target_label);
    
    // 从当前循环开始，向外遍历直到找到目标循环（包括目标循环本身）
    for (LoopScopeEntry *entry = gen->loop_scope_stack; entry != NULL; entry = entry->outer) {
        // 清理这个循环作用域的变量
        ScopeTracker *loop_scope = entry->loop_scope;
        if (loop_scope && loop_scope->locals) {
            for (LocalVarEntry *var = loop_scope->locals; var != NULL; var = var->next) {
                char *val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                        val, var->name);
                fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", val);
                free(val);
            }
        }
        
        // 如果到达目标循环，停止清理
        if (entry->label && strcmp(entry->label, target_label) == 0) {
            break;
        }
    }
    
    fprintf(gen->code_buf, "  ; === Multilevel Break Cleanup End ===\n");
}

/* 生成跨层 next 清理代码（清理从当前到目标循环之间的所有循环作用域，但不包括目标循环本身） */
void loop_scope_generate_multilevel_next_cleanup(CodeGen *gen, const char *target_label) {
    if (!gen || !gen->loop_scope_stack || !target_label) return;
    
    fprintf(gen->code_buf, "  ; === Multilevel Next Cleanup Start (target: %s) ===\n", target_label);
    
    // 从当前循环开始，向外遍历直到找到目标循环（不包括目标循环本身，因为 next 继续在目标循环中执行）
    for (LoopScopeEntry *entry = gen->loop_scope_stack; entry != NULL; entry = entry->outer) {
        // 如果到达目标循环，停止清理（不清理目标循环的变量，因为还要继续迭代）
        if (entry->label && strcmp(entry->label, target_label) == 0) {
            break;
        }
        
        // 清理这个循环作用域的变量
        ScopeTracker *loop_scope = entry->loop_scope;
        if (loop_scope && loop_scope->locals) {
            for (LocalVarEntry *var = loop_scope->locals; var != NULL; var = var->next) {
                char *val = new_temp(gen);
                fprintf(gen->code_buf, "  %s = load %%struct.Value*, %%struct.Value** %%%s\n",
                        val, var->name);
                fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", val);
                free(val);
            }
        }
    }
    
    fprintf(gen->code_buf, "  ; === Multilevel Next Cleanup End ===\n");
}

/* ============================================================================
 * 中间值管理函数实现 - 跟踪表达式求值期间的临时 Value*
 * ============================================================================ */

/* 创建中间值栈 */
TempValueStack *temp_value_stack_create(void) {
    TempValueStack *stack = (TempValueStack *)malloc(sizeof(TempValueStack));
    stack->entries = NULL;
    stack->count = 0;
    return stack;
}

/* 释放中间值栈（释放栈结构和所有条目的 C 字符串） */
void temp_value_stack_free(TempValueStack *stack) {
    if (!stack) return;
    
    TempValueEntry *entry = stack->entries;
    while (entry) {
        TempValueEntry *next = entry->next;
        if (entry->temp_name) {
            free(entry->temp_name);
        }
        free(entry);
        entry = next;
    }
    
    free(stack);
}

/* 注册一个中间值到栈中 */
void temp_value_register(CodeGen *gen, const char *temp_name) {
    if (!gen || !temp_name) return;
    
    // 如果栈不存在，创建它
    if (!gen->temp_values) {
        gen->temp_values = temp_value_stack_create();
    }
    
    // 创建新条目
    TempValueEntry *entry = (TempValueEntry *)malloc(sizeof(TempValueEntry));
    entry->temp_name = strdup(temp_name);
    entry->next = gen->temp_values->entries;
    gen->temp_values->entries = entry;
    gen->temp_values->count++;
}

/* 生成中间值清理代码，释放除最终结果外的所有中间值 */
void temp_value_release_except(CodeGen *gen, const char *keep_name) {
    if (!gen || !gen->temp_values) return;
    
    TempValueStack *stack = gen->temp_values;
    if (stack->count == 0) return;
    
    // 如果只有一个值，且是要保留的，不需要清理
    if (stack->count == 1 && keep_name && 
        stack->entries && stack->entries->temp_name &&
        strcmp(stack->entries->temp_name, keep_name) == 0) {
        temp_value_clear(gen);
        return;
    }
    
    // 生成清理代码
    fprintf(gen->code_buf, "  ; --- Temp values cleanup start ---\n");
    
    TempValueEntry *entry = stack->entries;
    TempValueEntry *prev = NULL;
    
    while (entry) {
        TempValueEntry *next = entry->next;
        
        // 如果不是要保留的值，释放它
        if (!keep_name || !entry->temp_name || strcmp(entry->temp_name, keep_name) != 0) {
            fprintf(gen->code_buf, "  call void @value_release(%%struct.Value* %s)\n", 
                    entry->temp_name);
        }
        
        // 释放 C 字符串和条目
        if (entry->temp_name) {
            free(entry->temp_name);
        }
        free(entry);
        
        entry = next;
    }
    
    fprintf(gen->code_buf, "  ; --- Temp values cleanup end ---\n");
    
    // 清空栈
    stack->entries = NULL;
    stack->count = 0;
}

/* 清空中间值栈（不生成清理代码） */
void temp_value_clear(CodeGen *gen) {
    if (!gen || !gen->temp_values) return;
    
    TempValueStack *stack = gen->temp_values;
    TempValueEntry *entry = stack->entries;
    
    while (entry) {
        TempValueEntry *next = entry->next;
        if (entry->temp_name) {
            free(entry->temp_name);
        }
        free(entry);
        entry = next;
    }
    
    stack->entries = NULL;
    stack->count = 0;
}
