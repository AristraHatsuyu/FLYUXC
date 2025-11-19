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
