/* src/utils/string/string_pool.c */
#include "flyuxc/utils/string_pool.h"
#include <string.h>
#include <stdlib.h>

/* FNV-1a 哈希函数 */
static uint32_t fnv1a_hash(const char* str, size_t length) {
    uint32_t hash = 2166136261u;  // FNV offset basis
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619u;  // FNV prime
    }
    return hash;
}

/* 创建字符串池 */
StringPool* string_pool_create(Arena* arena) {
    if (!arena) return NULL;
    
    StringPool* pool = (StringPool*)arena_alloc_zero(arena, sizeof(StringPool));
    if (!pool) return NULL;
    
    pool->arena = arena;
    pool->count = 0;
    pool->total_length = 0;
    
    return pool;
}

/* 插入字符串 */
const char* string_pool_insert(StringPool* pool, const char* str, size_t length) {
    if (!pool || !str) return NULL;
    
    /* 计算哈希值 */
    uint32_t hash = fnv1a_hash(str, length);
    uint32_t index = hash & (STRING_POOL_HASH_SIZE - 1);
    
    /* 查找是否已存在 */
    StringPoolEntry* entry = pool->table[index];
    while (entry) {
        if (entry->hash == hash && 
            entry->length == length && 
            memcmp(entry->str, str, length) == 0) {
            return entry->str;  // 找到重复，返回已存在的字符串
        }
        entry = entry->next;
    }
    
    /* 不存在，创建新条目 */
    entry = (StringPoolEntry*)arena_alloc(pool->arena, sizeof(StringPoolEntry));
    if (!entry) return NULL;
    
    /* 分配字符串内存（+1 for null terminator） */
    char* new_str = (char*)arena_alloc(pool->arena, length + 1);
    if (!new_str) return NULL;
    
    memcpy(new_str, str, length);
    new_str[length] = '\0';
    
    /* 初始化条目 */
    entry->str = new_str;
    entry->length = length;
    entry->hash = hash;
    entry->next = pool->table[index];
    
    /* 插入哈希表 */
    pool->table[index] = entry;
    pool->count++;
    pool->total_length += length;
    
    return new_str;
}

/* 插入C字符串 */
const char* string_pool_insert_cstr(StringPool* pool, const char* str) {
    if (!str) return NULL;
    return string_pool_insert(pool, str, strlen(str));
}

/* 获取字符串数量 */
size_t string_pool_count(const StringPool* pool) {
    return pool ? pool->count : 0;
}

/* 获取总字符数 */
size_t string_pool_total_length(const StringPool* pool) {
    return pool ? pool->total_length : 0;
}

/* 获取去重比率 */
double string_pool_dedup_ratio(const StringPool* pool) {
    if (!pool || pool->count == 0) return 0.0;
    
    /* 去重比率 = (总存储 / 唯一字符串数) */
    return (double)pool->total_length / (double)pool->count;
}

/* 销毁字符串池（实际上不需要做什么，会随Arena一起销毁） */
void string_pool_destroy(StringPool* pool) {
    /* 字符串池的内存由Arena管理，这里不需要释放 */
    (void)pool;
}
