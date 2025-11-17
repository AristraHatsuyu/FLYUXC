/* include/flyuxc/utils/string_pool.h */
#ifndef FLYUXC_STRING_POOL_H
#define FLYUXC_STRING_POOL_H

#include "arena.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* 字符串池哈希表大小（必须是2的幂） */
#define STRING_POOL_HASH_SIZE 4096

/* 字符串池条目 */
typedef struct StringPoolEntry {
    const char* str;               // 字符串指针（指向Arena）
    size_t length;                 // 字符串长度
    uint32_t hash;                 // FNV-1a哈希值
    struct StringPoolEntry* next;  // 哈希冲突链表
} StringPoolEntry;

/* 字符串池 */
typedef struct StringPool {
    Arena* arena;                              // Arena分配器
    StringPoolEntry* table[STRING_POOL_HASH_SIZE];  // 哈希表
    size_t count;                              // 字符串总数
    size_t total_length;                       // 总字符数
} StringPool;

/* 创建字符串池 */
StringPool* string_pool_create(Arena* arena);

/* 插入字符串（自动去重） */
const char* string_pool_insert(StringPool* pool, const char* str, size_t length);

/* 插入C字符串（自动计算长度） */
const char* string_pool_insert_cstr(StringPool* pool, const char* str);

/* 字符串比较（O(1)指针比较） */
static inline bool string_pool_equal(const char* a, const char* b) {
    return a == b;  // 池化后的字符串，相同内容必定指向同一地址
}

/* 获取统计信息 */
size_t string_pool_count(const StringPool* pool);
size_t string_pool_total_length(const StringPool* pool);
double string_pool_dedup_ratio(const StringPool* pool);

/* 销毁字符串池（会随Arena一起销毁） */
void string_pool_destroy(StringPool* pool);

#endif /* FLYUXC_STRING_POOL_H */
