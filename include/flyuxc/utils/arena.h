/* include/flyuxc/arena.h */
#ifndef FLYUXC_ARENA_H
#define FLYUXC_ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Arena 内存块 */
typedef struct ArenaBlock {
    uint8_t* memory;           // 内存块指针
    size_t capacity;           // 容量
    size_t used;               // 已使用
    struct ArenaBlock* next;   // 下一个块
} ArenaBlock;

/* Arena 分配器 */
typedef struct Arena {
    ArenaBlock* first;         // 第一个块
    ArenaBlock* current;       // 当前块
    size_t total_allocated;    // 总分配量
    size_t block_size;         // 块大小（初始64KB，后续倍增）
} Arena;

/* 创建 Arena，初始块大小为 64KB */
Arena* arena_create(void);

/* 从 Arena 分配内存，8字节对齐 */
void* arena_alloc(Arena* arena, size_t size);

/* 分配并清零 */
void* arena_alloc_zero(Arena* arena, size_t size);

/* 分配数组 */
#define arena_alloc_array(arena, type, count) \
    ((type*)arena_alloc((arena), sizeof(type) * (count)))

/* 分配并清零数组 */
#define arena_alloc_array_zero(arena, type, count) \
    ((type*)arena_alloc_zero((arena), sizeof(type) * (count)))

/* 重置 Arena（保留内存块，重置使用计数） */
void arena_reset(Arena* arena);

/* 销毁 Arena（释放所有内存） */
void arena_destroy(Arena* arena);

/* 获取统计信息 */
size_t arena_total_allocated(const Arena* arena);
size_t arena_total_used(const Arena* arena);

#endif /* FLYUXC_ARENA_H */
