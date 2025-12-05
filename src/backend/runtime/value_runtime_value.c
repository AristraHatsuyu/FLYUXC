/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_value.c
 */

/* ============================================================================
 * Value 结构和基础操作
 * ============================================================================
 */

/* Memory management flags */
#define VALUE_FLAG_NONE       0x00
#define VALUE_FLAG_STATIC     0x01  /* 静态分配，不需释放 (如字符串常量) */
#define VALUE_FLAG_BORROWED   0x02  /* 借用引用，不拥有所有权 */
#define VALUE_FLAG_IMMORTAL   0x04  /* 永生对象，永不释放 (如全局单例) */

/* Value structure with reference counting */
typedef struct Value {
    /* === 类型信息 (8 bytes) === */
    int type;           /* 当前值的实际类型 */
    int declared_type;  /* 变量的声明类型（用于类型注解）*/
    
    /* === 内存管理 (8 bytes) === */
    int refcount;       /* 引用计数 (0 = 未跟踪, >0 = 活跃引用数) */
    unsigned char flags;/* 内存标志位 */
    unsigned char ext_type;  /* 扩展对象类型标识 */
    unsigned short _pad;/* 对齐填充 */
    
    /* === 数据 (16 bytes) === */
    union {
        double number;
        char *string;
        void *pointer;
    } data;
    
    /* === 元数据 (16 bytes) === */
    long array_size;       /* 数组大小（仅当type==VALUE_ARRAY时有效）*/
    size_t string_length;  /* 字符串长度（支持包含\0的字符串）*/
} Value;

/* Object key-value pair (after Value definition) */
typedef struct ObjectEntry {
    char *key;
    Value *value;
} ObjectEntry;

/* VALUE_FUNCTION type constant */
#define VALUE_FUNCTION 7

/* Function object structure for closures */
typedef struct FunctionObject {
    void *func_ptr;           /* 函数指针 */
    struct Value **captured;  /* 捕获的变量数组 */
    int captured_count;       /* 捕获变量数量 */
    int param_count;          /* 函数参数数量 */
    struct Value *bound_self; /* 绑定的 self 对象（用于方法调用）*/
} FunctionObject;

/* ============================================================================
 * 引用计数内存管理
 * ============================================================================ */

/* 调试开关 - 在编译时通过 -DDEBUG_REFCOUNT 启用 */
#ifdef DEBUG_REFCOUNT
static const char* type_names[] = {
    "NUMBER", "STRING", "ARRAY", "OBJECT", "BOOL", "NULL", "UNDEF"
};
static void debug_rc(const char *op, Value *v, const char *context) {
    if (!v) return;
    const char *type_name = (v->type >= 0 && v->type <= 6) ? type_names[v->type] : "UNKNOWN";
    fprintf(stderr, "[RC] %s: %p type=%s rc=%d flags=0x%02x %s\n",
            op, (void*)v, type_name, v->refcount, v->flags, context ? context : "");
}
#define DEBUG_RC(op, v, ctx) debug_rc(op, v, ctx)
#else
#define DEBUG_RC(op, v, ctx) ((void)0)
#endif

/* 前向声明 */
static void value_free_internal(Value *v);

/*
 * value_retain - 增加引用计数
 * 返回传入的指针，方便链式调用: x = value_retain(y)
 */
Value* value_retain(Value *v) {
    if (!v) return NULL;
    if (v->flags & (VALUE_FLAG_STATIC | VALUE_FLAG_IMMORTAL)) {
        DEBUG_RC("retain(skip)", v, "static/immortal");
        return v;
    }
    if (v->refcount > 0) {
        v->refcount++;
        DEBUG_RC("retain", v, NULL);
    }
    return v;
}

/*
 * value_release - 减少引用计数，归零时释放
 * 这是主要的内存释放入口
 */
void value_release(Value *v) {
    if (!v) return;
    if (v->flags & (VALUE_FLAG_STATIC | VALUE_FLAG_IMMORTAL)) {
        DEBUG_RC("release(skip)", v, "static/immortal");
        return;
    }
    if (v->refcount <= 0) {
        DEBUG_RC("release(skip)", v, "already freed or untracked");
        return;
    }
    
    v->refcount--;
    DEBUG_RC("release", v, v->refcount == 0 ? "-> FREE" : NULL);
    if (v->refcount == 0) {
        value_free_internal(v);
    }
}

/*
 * value_free_internal - 内部释放函数，递归释放子元素
 */
static void value_free_internal(Value *v) {
    if (!v) return;
    
    switch (v->type) {
        case VALUE_STRING:
            /* 只释放动态分配的字符串 */
            if (v->data.string && !(v->flags & VALUE_FLAG_STATIC)) {
                free(v->data.string);
            }
            break;
            
        case VALUE_ARRAY: {
            /* 递归释放数组元素 */
            Value **elements = (Value**)v->data.pointer;
            if (elements) {
                for (long i = 0; i < v->array_size; i++) {
                    value_release(elements[i]);
                }
                free(elements);
            }
            break;
        }
        
        case VALUE_OBJECT: {
            /* 递归释放对象属性 */
            ObjectEntry *entries = (ObjectEntry*)v->data.pointer;
            if (entries) {
                for (long i = 0; i < v->array_size; i++) {
                    if (entries[i].key) free(entries[i].key);
                    value_release(entries[i].value);
                }
                free(entries);
            }
            break;
        }
        
        case VALUE_FUNCTION: {
            /* 释放函数对象 */
            FunctionObject *fn = (FunctionObject*)v->data.pointer;
            if (fn) {
                /* 释放捕获的变量（注意：自引用是弱引用，不能重复释放） */
                if (fn->captured && fn->captured_count > 0) {
                    for (int i = 0; i < fn->captured_count; i++) {
                        /* 检查是否是自引用（弱引用）：captured[i] == v */
                        if (fn->captured[i] && fn->captured[i] != v) {
                            value_release(fn->captured[i]);
                        }
                        /* 自引用不释放，避免循环释放 */
                    }
                    free(fn->captured);
                }
                /* 释放绑定的 self（如果有） */
                if (fn->bound_self) {
                    value_release(fn->bound_self);
                }
                free(fn);
            }
            break;
        }
        
        default:
            /* 数字、布尔、null、undef 无需额外释放 */
            break;
    }
    
    /* 释放 Value 本身 */
    free(v);
}

/*
 * value_clone - 深拷贝一个 Value
 * 返回新的独立副本，refcount = 1
 */
Value* value_clone(Value *v);  /* 前向声明，完整实现在后面 */

/*
 * 调试辅助：打印引用计数信息
 */
#ifdef DEBUG_REFCOUNT
static void debug_refcount(const char *op, Value *v) {
    if (!v) return;
    fprintf(stderr, "[RC] %s: type=%d refcount=%d flags=0x%02x\n",
            op, v->type, v->refcount, v->flags);
}
#else
#define debug_refcount(op, v) ((void)0)
#endif

