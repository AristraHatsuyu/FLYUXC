/* src/core/arena.c */
#include "flyuxc/utils/arena.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 初始块大小：64KB */
#define ARENA_INITIAL_BLOCK_SIZE (64 * 1024)

/* 8字节对齐 */
#define ARENA_ALIGN 8
#define ALIGN_UP(n, align) (((n) + (align) - 1) & ~((align) - 1))

/* 创建新的内存块 */
static ArenaBlock* arena_block_create(size_t size) {
    ArenaBlock* block = (ArenaBlock*)malloc(sizeof(ArenaBlock));
    if (!block) return NULL;
    
    block->memory = (uint8_t*)malloc(size);
    if (!block->memory) {
        free(block);
        return NULL;
    }
    
    block->capacity = size;
    block->used = 0;
    block->next = NULL;
    
    return block;
}

/* 销毁内存块 */
static void arena_block_destroy(ArenaBlock* block) {
    if (!block) return;
    free(block->memory);
    free(block);
}

/* 创建 Arena */
Arena* arena_create(void) {
    Arena* arena = (Arena*)malloc(sizeof(Arena));
    if (!arena) return NULL;
    
    arena->first = arena_block_create(ARENA_INITIAL_BLOCK_SIZE);
    if (!arena->first) {
        free(arena);
        return NULL;
    }
    
    arena->current = arena->first;
    arena->total_allocated = ARENA_INITIAL_BLOCK_SIZE;
    arena->block_size = ARENA_INITIAL_BLOCK_SIZE;
    
    return arena;
}

/* 从 Arena 分配内存 */
void* arena_alloc(Arena* arena, size_t size) {
    if (!arena || size == 0) return NULL;
    
    /* 8字节对齐 */
    size = ALIGN_UP(size, ARENA_ALIGN);
    
    /* 如果当前块空间不足 */
    if (arena->current->used + size > arena->current->capacity) {
        /* 计算新块大小：如果请求的大小超过默认块大小的一半，使用请求大小的2倍 */
        size_t new_block_size = arena->block_size * 2;
        if (size > arena->block_size / 2) {
            new_block_size = size * 2;
        }
        
        /* 创建新块 */
        ArenaBlock* new_block = arena_block_create(new_block_size);
        if (!new_block) return NULL;
        
        /* 链接到当前块 */
        arena->current->next = new_block;
        arena->current = new_block;
        arena->total_allocated += new_block_size;
        arena->block_size = new_block_size;  /* 更新块大小（倍增） */
    }
    
    /* 分配内存 */
    void* ptr = arena->current->memory + arena->current->used;
    arena->current->used += size;
    
    return ptr;
}

/* 分配并清零 */
void* arena_alloc_zero(Arena* arena, size_t size) {
    void* ptr = arena_alloc(arena, size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/* 重置 Arena */
void arena_reset(Arena* arena) {
    if (!arena) return;
    
    /* 重置所有块的使用计数 */
    ArenaBlock* block = arena->first;
    while (block) {
        block->used = 0;
        block = block->next;
    }
    
    /* 重置当前块指针 */
    arena->current = arena->first;
}

/* 销毁 Arena */
void arena_destroy(Arena* arena) {
    if (!arena) return;
    
    /* 销毁所有块 */
    ArenaBlock* block = arena->first;
    while (block) {
        ArenaBlock* next = block->next;
        arena_block_destroy(block);
        block = next;
    }
    
    free(arena);
}

/* 获取总分配量 */
size_t arena_total_allocated(const Arena* arena) {
    return arena ? arena->total_allocated : 0;
}

/* 获取总使用量 */
size_t arena_total_used(const Arena* arena) {
    if (!arena) return 0;
    
    size_t total = 0;
    ArenaBlock* block = arena->first;
    while (block) {
        total += block->used;
        block = block->next;
    }
    
    return total;
}
