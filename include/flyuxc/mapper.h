#ifndef FLYUXC_MAPPER_H
#define FLYUXC_MAPPER_H

#include <stddef.h>

typedef struct {
    char *original;      // 原始标识符（UTF-8）
    char *mapped;        // 映射后的 ASCII 标识符（_XXXXX 格式）
} MapEntry;

typedef struct {
    MapEntry *entries;   // 映射条目数组
    size_t count;        // 条目数量
    size_t capacity;     // 容量
} IdentifierMapper;

// 创建新的映射器
IdentifierMapper* mapper_create(void);

// 销毁映射器并释放所有资源
void mapper_free(IdentifierMapper *mapper);

// 为标识符分配或获取映射（如果已存在则返回现有映射）
// 返回分配的映射 ASCII 标识符（_XXXXX）
const char* mapper_get_or_alloc(IdentifierMapper *mapper, const char *original);

// 输出映射表为 JSON 格式到 stdout
void mapper_output_json(IdentifierMapper *mapper);

#endif // FLYUXC_MAPPER_H
