/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_ext.c
 */

/* ============================================================================
 * 扩展对象类型结构定义
 * ============================================================================ */

/* Buffer对象 - 二进制数据容器 */
typedef struct {
    unsigned char *data;  /* 原始二进制数据 */
    size_t size;          /* 数据大小(字节) */
    size_t capacity;      /* 分配容量 */
} BufferObject;

/* FileHandle对象 - 文件句柄 */
typedef struct {
    FILE *fp;             /* C文件指针 */
    char *path;           /* 文件路径 */
    char *mode;           /* 打开模式 */
    int is_open;          /* 是否打开 */
    long position;        /* 当前位置 */
} FileHandleObject;

/* Error对象 - 错误信息 */
typedef struct {
    char *message;        /* 错误消息 */
    int code;             /* 错误代码 */
    char *error_type;     /* 错误类型(Error/TypeError/IOError) */
} ErrorObject;

/* ============================================================================
 * Box 函数 - 将原始值包装为 Value (带引用计数)
 * 所有 box 函数创建的 Value 初始 refcount = 1
 * ============================================================================ */

/* 小整数缓存: [-128, 256] 共 385 个预分配的不可变 Value */
#define SMALL_INT_MIN -128
#define SMALL_INT_MAX 256
#define SMALL_INT_CACHE_SIZE (SMALL_INT_MAX - SMALL_INT_MIN + 1)
static Value *small_int_cache[SMALL_INT_CACHE_SIZE] = {0};
static int small_int_cache_initialized = 0;

/* 初始化小整数缓存 */
static void init_small_int_cache(void) {
    if (small_int_cache_initialized) return;
    for (int i = 0; i < SMALL_INT_CACHE_SIZE; i++) {
        Value *v = (Value*)malloc(sizeof(Value));
        v->type = VALUE_NUMBER;
        v->declared_type = VALUE_NUMBER;
        v->refcount = 1;
        v->flags = VALUE_FLAG_IMMORTAL;  // 永不释放
        v->ext_type = EXT_TYPE_NONE;
        v->data.number = (double)(i + SMALL_INT_MIN);
        v->array_size = 0;
        v->string_length = 0;
        small_int_cache[i] = v;
    }
    small_int_cache_initialized = 1;
}

/* Box a number into a Value - 使用小整数缓存优化 */
Value* box_number(double num) {
    // 检查是否是小整数
    if (num == (int)num) {
        int inum = (int)num;
        if (inum >= SMALL_INT_MIN && inum <= SMALL_INT_MAX) {
            if (!small_int_cache_initialized) {
                init_small_int_cache();
            }
            return small_int_cache[inum - SMALL_INT_MIN];
        }
    }
    
    // 非小整数，分配新的 Value
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NUMBER;
    v->declared_type = VALUE_NUMBER;
    v->refcount = 1;
    v->flags = VALUE_FLAG_NONE;
    v->ext_type = EXT_TYPE_NONE;
    v->data.number = num;
    v->array_size = 0;
    v->string_length = 0;
    return v;
}

/* Box a string into a Value (静态字符串，不会被释放) */
Value* box_string(char *str) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_STRING;
    v->declared_type = VALUE_STRING;
    v->refcount = 1;
    v->flags = VALUE_FLAG_STATIC;  /* 默认假设是静态字符串常量 */
    v->ext_type = EXT_TYPE_NONE;
    v->data.string = str;
    v->array_size = 0;
    v->string_length = str ? strlen(str) : 0;
    return v;
}

/* Box a dynamically allocated string (会在释放时 free) */
Value* box_string_owned(char *str) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_STRING;
    v->declared_type = VALUE_STRING;
    v->refcount = 1;
    v->flags = VALUE_FLAG_NONE;  /* 拥有所有权，释放时 free */
    v->ext_type = EXT_TYPE_NONE;
    v->data.string = str;
    v->array_size = 0;
    v->string_length = str ? strlen(str) : 0;
    return v;
}

/* Box a string with explicit length (supports \0 in string) */
Value* box_string_with_length(char *str, size_t len) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_STRING;
    v->declared_type = VALUE_STRING;
    v->refcount = 1;
    v->flags = VALUE_FLAG_STATIC;
    v->ext_type = EXT_TYPE_NONE;
    v->data.string = str;
    v->array_size = 0;
    v->string_length = len;
    return v;
}

/* 布尔值缓存: true 和 false */
static Value *cached_true = NULL;
static Value *cached_false = NULL;

/* Box a boolean into a Value - 使用缓存 */
Value* box_bool(int b) {
    if (b) {
        if (!cached_true) {
            Value *v = (Value*)malloc(sizeof(Value));
            v->type = VALUE_BOOL;
            v->declared_type = VALUE_BOOL;
            v->refcount = 1;
            v->flags = VALUE_FLAG_IMMORTAL;  // 永不释放
            v->ext_type = EXT_TYPE_NONE;
            v->data.number = 1.0;
            v->array_size = 0;
            v->string_length = 0;
            cached_true = v;
        }
        return cached_true;
    } else {
        if (!cached_false) {
            Value *v = (Value*)malloc(sizeof(Value));
            v->type = VALUE_BOOL;
            v->declared_type = VALUE_BOOL;
            v->refcount = 1;
            v->flags = VALUE_FLAG_IMMORTAL;  // 永不释放
            v->ext_type = EXT_TYPE_NONE;
            v->data.number = 0.0;
            v->array_size = 0;
            v->string_length = 0;
            cached_false = v;
        }
        return cached_false;
    }
}

/* null 缓存 */
static Value *cached_null = NULL;

/* Box null - 使用缓存 */
Value* box_null() {
    if (!cached_null) {
        Value *v = (Value*)malloc(sizeof(Value));
        v->type = VALUE_NULL;
        v->declared_type = VALUE_NULL;
        v->refcount = 1;
        v->flags = VALUE_FLAG_IMMORTAL;  // 永不释放
        v->ext_type = EXT_TYPE_NONE;
        v->data.pointer = NULL;
        v->array_size = 0;
        v->string_length = 0;
        cached_null = v;
    }
    return cached_null;
}

/* undef 缓存 */
static Value *cached_undef = NULL;

/* Box undef - for undefined variables - 使用缓存 */
Value* box_undef() {
    if (!cached_undef) {
        Value *v = (Value*)malloc(sizeof(Value));
        v->type = VALUE_UNDEF;
        v->declared_type = VALUE_UNDEF;
        v->refcount = 1;
        v->flags = VALUE_FLAG_IMMORTAL;  // 永不释放
        v->ext_type = EXT_TYPE_NONE;
        v->data.pointer = NULL;
        v->array_size = 0;
        v->string_length = 0;
        cached_undef = v;
    }
    return cached_undef;
}

/* ============================================================================
 * 函数值 (VALUE_FUNCTION) 支持
 * 用于闭包和高阶函数：存储函数指针 + 捕获的变量
 * ============================================================================ */

/* FunctionObject 结构体已在 value_runtime_value.c 中定义 */
/* VALUE_FUNCTION 的类型常量已在 value_runtime_value.c 中定义 */

/* Box function - 创建一个函数值
 * @param func_ptr: 函数指针
 * @param captured: 捕获的变量数组（可为 NULL）
 * @param captured_count: 捕获变量数量
 * @param param_count: 函数参数数量
 * @param needs_self: 是否需要 self 参数
 * @param capture_by_ref: 是否按引用捕获（1 = captured 是 Value**, 0 = Value*）
 */
Value* box_function_ex(void *func_ptr, Value **captured, int captured_count, int param_count, int needs_self, int capture_by_ref) {
    FunctionObject *fn = (FunctionObject*)malloc(sizeof(FunctionObject));
    fn->func_ptr = func_ptr;
    fn->param_count = param_count;
    fn->captured_count = captured_count;
    fn->bound_self = NULL;  /* 初始没有绑定的 self */
    fn->needs_self = needs_self;  /* 记录函数是否需要 self */
    fn->capture_by_ref = capture_by_ref;  /* 记录是否按引用捕获 */
    
    /* 复制捕获的变量引用 */
    if (captured_count > 0 && captured) {
        fn->captured = (Value**)malloc(sizeof(Value*) * captured_count);
        for (int i = 0; i < captured_count; i++) {
            fn->captured[i] = captured[i];
            /* 只有按值捕获时才增加引用计数 */
            /* 按引用捕获时，captured[i] 实际上是 Value**（伪装成 Value*），不能 retain */
            if (!capture_by_ref && fn->captured[i]) {
                value_retain(fn->captured[i]);
            }
        }
    } else {
        fn->captured = NULL;
    }
    
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_FUNCTION;
    v->declared_type = VALUE_FUNCTION;
    v->refcount = 1;
    v->flags = VALUE_FLAG_NONE;
    v->ext_type = EXT_TYPE_NONE;
    v->data.pointer = fn;
    v->array_size = 0;
    v->string_length = 0;
    return v;
}

/* 兼容旧版本：默认按值捕获 */
Value* box_function(void *func_ptr, Value **captured, int captured_count, int param_count, int needs_self) {
    return box_function_ex(func_ptr, captured, captured_count, param_count, needs_self, 0);
}

/* 获取函数指针 */
void* unbox_function_ptr(Value *v) {
    if (!v || v->type != VALUE_FUNCTION) return NULL;
    FunctionObject *fn = (FunctionObject*)v->data.pointer;
    return fn ? fn->func_ptr : NULL;
}

/* 获取捕获的变量数组 */
Value** get_function_captured(Value *v) {
    if (!v || v->type != VALUE_FUNCTION) return NULL;
    FunctionObject *fn = (FunctionObject*)v->data.pointer;
    return fn ? fn->captured : NULL;
}

/*
 * 更新闘包中某个捕获变量的值（用于自引用闭包）
 * 用于处理自引用闭包：f := (n) { f(n-1) }
 * 在创建闭包时 f 还是 null，创建后需要更新捕获的 f 为实际函数值
 * 
 * 重要：这里使用弱引用（不增加引用计数）来避免循环引用！
 * 闭包捕获自身会形成：闭包 -> 捕获数组 -> 闭包 的循环
 * 如果用强引用，闭包永远无法被释放
 * 
 * @param closure: 闭包函数值
 * @param index: 要更新的捕获变量索引
 * @param new_value: 新值（自引用时就是 closure 本身）
 */
void update_closure_captured(Value *closure, int index, Value *new_value) {
    if (!closure || closure->type != VALUE_FUNCTION) return;
    FunctionObject *fn = (FunctionObject*)closure->data.pointer;
    if (!fn || !fn->captured || index < 0 || index >= fn->captured_count) return;
    
    /* 如果是按引用捕获，captured 数组中的"值"实际上是 Value** 指针，不能 release */
    if (!fn->capture_by_ref) {
        /* 按值捕获：释放旧值的引用（通常是 null） */
        if (fn->captured[index]) {
            value_release(fn->captured[index]);
        }
    }
    
    /* 设置新值 - 使用弱引用（不增加引用计数）避免循环引用 */
    /* 因为 new_value 就是 closure 本身，如果 retain 会形成循环 */
    fn->captured[index] = new_value;
    /* 不调用 value_retain - 这是故意的弱引用 */
}

/* 获取捕获变量数量 */
int get_function_captured_count(Value *v) {
    if (!v || v->type != VALUE_FUNCTION) return 0;
    FunctionObject *fn = (FunctionObject*)v->data.pointer;
    return fn ? fn->captured_count : 0;
}

/* 获取参数数量 */
int get_function_param_count(Value *v) {
    if (!v || v->type != VALUE_FUNCTION) return 0;
    FunctionObject *fn = (FunctionObject*)v->data.pointer;
    return fn ? fn->param_count : 0;
}

/* 检查是否是函数值 */
int value_is_function(Value *v) {
    return v && v->type == VALUE_FUNCTION;
}

/* 获取绑定的 self 对象 */
Value* get_function_bound_self(Value *v) {
    if (!v || v->type != VALUE_FUNCTION) return NULL;
    FunctionObject *fn = (FunctionObject*)v->data.pointer;
    return fn ? fn->bound_self : NULL;
}

/* 绑定方法 - 创建一个新的函数值，其中 self 被绑定到指定对象
 * @param func_val: 原函数值
 * @param self_obj: 要绑定的 self 对象
 * @return: 新的绑定方法函数值
 * 
 * 重要：如果函数已有 bound_self，不会重新绑定，直接返回原函数。
 * 这确保了已绑定方法赋值到其他对象时保持原有绑定。
 */
Value* bind_method(Value *func_val, Value *self_obj) {
    if (!func_val || func_val->type != VALUE_FUNCTION || !self_obj) {
        return func_val;  /* 如果不能绑定，返回原函数 */
    }
    
    FunctionObject *orig_fn = (FunctionObject*)func_val->data.pointer;
    if (!orig_fn) return func_val;
    
    /* 如果已经绑定了 self，不重新绑定，保持原有绑定 */
    if (orig_fn->bound_self != NULL) {
        return func_val;
    }
    
    /* 创建新的 FunctionObject，复制原有属性 */
    FunctionObject *new_fn = (FunctionObject*)malloc(sizeof(FunctionObject));
    new_fn->func_ptr = orig_fn->func_ptr;
    new_fn->param_count = orig_fn->param_count;
    new_fn->captured_count = orig_fn->captured_count;
    new_fn->needs_self = orig_fn->needs_self;  /* 复制 needs_self 标志 */
    new_fn->capture_by_ref = orig_fn->capture_by_ref;  /* 复制 capture_by_ref 标志 */
    
    /* 复制捕获的变量 */
    if (orig_fn->captured_count > 0 && orig_fn->captured) {
        new_fn->captured = (Value**)malloc(sizeof(Value*) * orig_fn->captured_count);
        for (int i = 0; i < orig_fn->captured_count; i++) {
            new_fn->captured[i] = orig_fn->captured[i];
            /* 只有按值捕获时才 retain */
            if (!new_fn->capture_by_ref && new_fn->captured[i]) {
                value_retain(new_fn->captured[i]);
            }
        }
    } else {
        new_fn->captured = NULL;
    }
    
    /* 绑定 self */
    new_fn->bound_self = self_obj;
    value_retain(self_obj);
    
    /* 创建新的 Value */
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_FUNCTION;
    v->declared_type = VALUE_FUNCTION;
    v->refcount = 1;
    v->flags = VALUE_FLAG_NONE;
    v->ext_type = EXT_TYPE_NONE;
    v->data.pointer = new_fn;
    v->array_size = 0;
    v->string_length = 0;
    return v;
}

/* 间接调用函数值
 * @param func_val: 函数值（VALUE_FUNCTION 类型）
 * @param args: 参数数组
 * @param arg_count: 参数数量
 * @return: 函数返回值
 *
 * 实现说明：
 * 由于C语言无法直接进行可变参数的间接函数调用，
 * 我们使用函数指针类型转换来实现。
 * 支持0-10个参数的函数调用。
 * 
 * 如果函数有 bound_self，它会作为第一个参数传入。
 */
Value* call_function_value(Value *func_val, Value **args, int arg_count) {
    FunctionObject *fn = (FunctionObject*)func_val->data.pointer;
    
    // 快速路径：无 bound_self 或不需要 self 的常见情况
    if (!fn->bound_self || !fn->needs_self) {
        int total = arg_count + fn->captured_count;
        
        // 快速路径：1 参数 + 2 捕获 = 3 参数（fib 常见情况）
        if (total == 3 && arg_count == 1 && fn->captured_count == 2) {
            typedef Value* (*Func3)(Value*, Value*, Value*);
            return ((Func3)fn->func_ptr)(args[0], fn->captured[0], fn->captured[1]);
        }
        
        // 快速路径：1 参数 + 1 捕获 = 2 参数
        if (total == 2 && arg_count == 1 && fn->captured_count == 1) {
            typedef Value* (*Func2)(Value*, Value*);
            return ((Func2)fn->func_ptr)(args[0], fn->captured[0]);
        }
        
        // 快速路径：1 参数 + 0 捕获 = 1 参数
        if (total == 1 && arg_count == 1 && fn->captured_count == 0) {
            typedef Value* (*Func1)(Value*);
            return ((Func1)fn->func_ptr)(args[0]);
        }
        
        // 快速路径：0 参数 + 0 捕获 = 0 参数
        if (total == 0) {
            typedef Value* (*Func0)(void);
            return ((Func0)fn->func_ptr)();
        }
        
        // 快速路径：0 参数 + 1 捕获 = 1 参数（闭包常见情况）
        if (total == 1 && arg_count == 0 && fn->captured_count == 1) {
            typedef Value* (*Func1)(Value*);
            return ((Func1)fn->func_ptr)(fn->captured[0]);
        }
    }
    
    // 慢速路径：通用情况
    // 只有当函数需要 self 且有 bound_self 时才传入 self
    int has_bound_self = (fn->bound_self != NULL && fn->needs_self) ? 1 : 0;
    int total_args = has_bound_self + arg_count + fn->captured_count;
    
    // 使用栈上数组避免 malloc（最多支持 16 个参数）
    Value *full_args[16];
    if (total_args > 16) {
        fprintf(stderr, "Error: call_function_value does not support more than 16 arguments\n");
        return box_undef();
    }
    
    int idx = 0;
    
    // 首先添加 bound_self（如果有且需要）
    if (has_bound_self) {
        full_args[idx++] = fn->bound_self;
    }
    
    // 再复制传入参数
    for (int i = 0; i < arg_count; i++) {
        full_args[idx++] = args ? args[i] : NULL;
    }
    
    // 最后复制捕获变量
    for (int i = 0; i < fn->captured_count; i++) {
        full_args[idx++] = fn->captured ? fn->captured[i] : NULL;
    }
    
    // 根据参数数量调用函数
    Value* result = NULL;
    
    typedef Value* (*Func0)(void);
    typedef Value* (*Func1)(Value*);
    typedef Value* (*Func2)(Value*, Value*);
    typedef Value* (*Func3)(Value*, Value*, Value*);
    typedef Value* (*Func4)(Value*, Value*, Value*, Value*);
    typedef Value* (*Func5)(Value*, Value*, Value*, Value*, Value*);
    typedef Value* (*Func6)(Value*, Value*, Value*, Value*, Value*, Value*);
    typedef Value* (*Func7)(Value*, Value*, Value*, Value*, Value*, Value*, Value*);
    typedef Value* (*Func8)(Value*, Value*, Value*, Value*, Value*, Value*, Value*, Value*);
    typedef Value* (*Func9)(Value*, Value*, Value*, Value*, Value*, Value*, Value*, Value*, Value*);
    typedef Value* (*Func10)(Value*, Value*, Value*, Value*, Value*, Value*, Value*, Value*, Value*, Value*);
    
    switch (total_args) {
        case 0:
            result = ((Func0)fn->func_ptr)();
            break;
        case 1:
            result = ((Func1)fn->func_ptr)(full_args[0]);
            break;
        case 2:
            result = ((Func2)fn->func_ptr)(full_args[0], full_args[1]);
            break;
        case 3:
            result = ((Func3)fn->func_ptr)(full_args[0], full_args[1], full_args[2]);
            break;
        case 4:
            result = ((Func4)fn->func_ptr)(full_args[0], full_args[1], full_args[2], full_args[3]);
            break;
        case 5:
            result = ((Func5)fn->func_ptr)(full_args[0], full_args[1], full_args[2], full_args[3], full_args[4]);
            break;
        case 6:
            result = ((Func6)fn->func_ptr)(full_args[0], full_args[1], full_args[2], full_args[3], full_args[4], full_args[5]);
            break;
        case 7:
            result = ((Func7)fn->func_ptr)(full_args[0], full_args[1], full_args[2], full_args[3], full_args[4], full_args[5], full_args[6]);
            break;
        case 8:
            result = ((Func8)fn->func_ptr)(full_args[0], full_args[1], full_args[2], full_args[3], full_args[4], full_args[5], full_args[6], full_args[7]);
            break;
        case 9:
            result = ((Func9)fn->func_ptr)(full_args[0], full_args[1], full_args[2], full_args[3], full_args[4], full_args[5], full_args[6], full_args[7], full_args[8]);
            break;
        case 10:
            result = ((Func10)fn->func_ptr)(full_args[0], full_args[1], full_args[2], full_args[3], full_args[4], full_args[5], full_args[6], full_args[7], full_args[8], full_args[9]);
            break;
        default:
            fprintf(stderr, "Error: call_function_value does not support %d arguments (max 10)\n", total_args);
            result = box_undef();
            break;
    }
    
    // 栈上数组不需要 free
    
    return result ? result : box_undef();
}

/* value_call_function - 供 codegen 使用的间接调用包装
 * @param func_val: 函数值（VALUE_FUNCTION 类型）
 * @param args: 参数数组
 * @param arg_count: 参数数量 (i64)
 * @return: 函数返回值
 */
Value* value_call_function(Value *func_val, Value **args, long arg_count) {
    return call_function_value(func_val, args, (int)arg_count);
}

/* Box null with declared type - for typed variables */
Value* box_null_typed(int decl_type) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NULL;
    v->declared_type = decl_type;
    v->refcount = 1;
    v->flags = VALUE_FLAG_NONE;
    v->ext_type = EXT_TYPE_NONE;
    v->data.pointer = NULL;
    v->array_size = 0;
    v->string_length = 0;
    return v;
}

/* Create null preserving declared_type from existing value */
Value* box_null_preserve_type(Value *old_val) {
    if (!old_val) return box_null();
    return box_null_typed(old_val->declared_type);
}

/* Box an array (从栈上拷贝到堆上并获得所有权) */
Value* box_array(void *array_ptr, long size) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_ARRAY;
    v->declared_type = VALUE_ARRAY;
    v->refcount = 1;
    v->flags = VALUE_FLAG_NONE;
    v->ext_type = EXT_TYPE_NONE;
    
    /* 重要：复制数组元素到堆上，因为传入的可能是栈上的临时数组 */
    if (size > 0 && array_ptr) {
        Value **src = (Value**)array_ptr;
        Value **dst = (Value**)malloc(sizeof(Value*) * size);
        for (long i = 0; i < size; i++) {
            dst[i] = src[i];
            /* P2 修复：对每个元素进行 retain，因为数组持有元素的引用 */
            if (dst[i]) {
                value_retain(dst[i]);
            }
        }
        v->data.pointer = dst;
    } else {
        v->data.pointer = NULL;
    }
    
    v->array_size = size;
    v->string_length = 0;
    return v;
}

/* ObjectEntry 结构在 value_runtime_value.c 中定义，这里用前向声明 */
struct ObjectEntry;  /* forward declaration */

/* Box an object - takes array of ObjectEntry (从栈上拷贝到堆上) */
Value* box_object(void *entries_ptr, long count) {
    /* 内部定义与外部相同的结构，用于访问字段 */
    typedef struct { char *key; Value *value; } ObjEntry;
    
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_OBJECT;
    v->declared_type = VALUE_OBJECT;
    v->refcount = 1;
    v->flags = VALUE_FLAG_NONE;
    v->ext_type = EXT_TYPE_NONE;
    
    /* 重要：复制 entries 到堆上，因为传入的可能是栈上的临时数组 */
    if (count > 0 && entries_ptr) {
        ObjEntry *src = (ObjEntry*)entries_ptr;
        ObjEntry *dst = (ObjEntry*)malloc(sizeof(ObjEntry) * count);
        for (long i = 0; i < count; i++) {
            /* 复制 key（需要 strdup 因为原 key 可能在栈上）*/
            dst[i].key = src[i].key ? strdup(src[i].key) : NULL;
            dst[i].value = src[i].value;
            /* P2 修复：对每个 value 进行 retain，因为对象持有 value 的引用 */
            if (dst[i].value) {
                value_retain(dst[i].value);
            }
        }
        v->data.pointer = dst;
    } else {
        v->data.pointer = NULL;
    }
    
    v->array_size = count;
    v->string_length = 0;
    return v;
}

/* ============================================================================
 * Unbox 函数 - 从 Value 提取原始值
 * ============================================================================ */

/* Unbox to number (with type coercion) */
double unbox_number(Value *v) {
    if (!v) return 0.0;
    
    switch (v->type) {
        case VALUE_NUMBER:
        case VALUE_BOOL:
            return v->data.number;
        case VALUE_STRING:
            // 尝试解析字符串为数字
            if (v->data.string) {
                double result = 0.0;
                sscanf(v->data.string, "%lf", &result);
                return result;
            }
            return 0.0;
        case VALUE_NULL:
        case VALUE_UNDEF:
            return 0.0;
        default:
            return 0.0;
    }
}

/* Unbox to string (with type coercion) */
char* unbox_string(Value *v) {
    if (!v) return "(null)";
    
    switch (v->type) {
        case VALUE_STRING:
            return v->data.string ? v->data.string : "(empty)";
        case VALUE_NUMBER:
        case VALUE_BOOL: {
            char *buf = (char*)malloc(32);
            snprintf(buf, 32, "%g", v->data.number);
            return buf;
        }
        case VALUE_NULL:
        case VALUE_UNDEF:
            return "";
        default:
            return "(object)";
    }
}

/* Check if value is truthy */
int value_is_truthy(Value *v) {
    if (!v) return 0;
    
    switch (v->type) {
        case VALUE_NUMBER:
        case VALUE_BOOL:
            return v->data.number != 0.0;
        case VALUE_STRING:
            return v->data.string && v->data.string[0] != '\0';
        case VALUE_NULL:
        case VALUE_UNDEF:
            return 0;
        default:
            return 1;  // objects/arrays are truthy
    }
}

/* ========================================
 * Type checking functions
 * ======================================== */

/* isNum(value) - 检查是否为数字类型 */
Value* value_is_num(Value *v) {
    return box_bool(v && v->type == VALUE_NUMBER);
}

/* isStr(value) - 检查是否为字符串类型 */
Value* value_is_str(Value *v) {
    return box_bool(v && v->type == VALUE_STRING);
}

/* isBl(value) - 检查是否为布尔类型 */
Value* value_is_bl(Value *v) {
    return box_bool(v && v->type == VALUE_BOOL);
}

/* isArr(value) - 检查是否为数组类型 */
Value* value_is_arr(Value *v) {
    return box_bool(v && v->type == VALUE_ARRAY);
}

/* isObj(value) - 检查是否为对象类型（包括数组，因为数组是对象的子类型） */
Value* value_is_obj(Value *v) {
    return box_bool(v && (v->type == VALUE_OBJECT || v->type == VALUE_ARRAY));
}

/* isNull(value) - 检查是否为 null */
Value* value_is_null(Value *v) {
    return box_bool(!v || v->type == VALUE_NULL);
}

/* isUndef(value) - 检查是否为 undefined */
Value* value_is_undef(Value *v) {
    return box_bool(!v || v->type == VALUE_UNDEF);
}

/* ========================================
 * Utility functions
 * ======================================== */

/* range(start, end, step) - 生成数字范围数组 */
Value* value_range(Value *start_val, Value *end_val, Value *step_val) {
    set_runtime_status(FLYUX_OK, NULL);
    
    double start = start_val ? unbox_number(start_val) : 0;
    double end = end_val ? unbox_number(end_val) : 0;
    double step = step_val && step_val->type != VALUE_UNDEF && step_val->type != VALUE_NULL 
                  ? unbox_number(step_val) : 1;
    
    // 防止无限循环
    if (step == 0) {
        set_runtime_status(FLYUX_ERROR, "range: step cannot be 0");
        return box_null();
    }
    
    // 计算元素数量
    int64_t count = 0;
    if ((step > 0 && start < end) || (step < 0 && start > end)) {
        count = (int64_t)ceil(fabs((end - start) / step));
    }
    
    // 创建数组
    Value **elements = NULL;
    if (count > 0) {
        elements = (Value**)malloc((size_t)count * sizeof(Value*));
        double val = start;
        for (int64_t i = 0; i < count; i++) {
            elements[i] = box_number(val);
            val += step;
        }
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = elements;
    result->array_size = (size_t)count;
    
    return result;
}

/* assert(condition, message?) - 断言，失败时终止程序 */
Value* value_assert(Value *condition, Value *message) {
    int is_true = value_is_truthy(condition);
    
    if (!is_true) {
        fprintf(stderr, "Assertion failed");
        if (message && message->type == VALUE_STRING && message->data.string) {
            fprintf(stderr, ": %s", message->data.string);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
    
    return box_bool(1);  // 返回 true 表示断言通过
}

/* Helper: Convert value to JSON string representation */
static void value_to_json_string(Value *v, char *buf, size_t size) {
    if (!v) {
        snprintf(buf, size, "null");
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER:
            if (isinf(v->data.number)) {
                snprintf(buf, size, "%s", v->data.number > 0 ? "+Inf" : "-Inf");
            } else if (isnan(v->data.number)) {
                snprintf(buf, size, "NaN");
            } else {
                snprintf(buf, size, "%.16g", v->data.number);
            }
            break;
        case VALUE_STRING:
            snprintf(buf, size, "\"%s\"", v->data.string);
            break;
        case VALUE_BOOL:
            snprintf(buf, size, "%s", v->data.number != 0 ? "true" : "false");
            break;
        case VALUE_NULL:
            snprintf(buf, size, "null");
            break;
        case VALUE_ARRAY:
            snprintf(buf, size, "[...]");
            break;
        case VALUE_OBJECT:
            snprintf(buf, size, "{...}");
            break;
        default:
            snprintf(buf, size, "<unknown>");
    }
}

/* 智能数字格式化 - 自动清理浮点误差 */
static void print_smart_number(double num, int use_colors) {
    // 特殊值检查
    if (isinf(num)) {
        if (use_colors) {
            printf("%s%s%s", COLOR_NUM, num > 0 ? "+Inf" : "-Inf", COLOR_RESET);
        } else {
            printf("%s", num > 0 ? "+Inf" : "-Inf");
        }
        return;
    } else if (isnan(num)) {
        if (use_colors) {
            printf("%sNaN%s", COLOR_NUM, COLOR_RESET);
        } else {
            printf("NaN");
        }
        return;
    }
    
    // 对于非常接近整数的值，显示为整数
    double rounded = round(num);
    if (fabs(num - rounded) < 1e-9 && fabs(rounded) < 1e15) {
        if (use_colors) {
            printf("%s%.0f%s", COLOR_NUM, rounded, COLOR_RESET);
        } else {
            printf("%.0f", rounded);
        }
        return;
    }
    
    // 对于小数，尝试找到合理的精度
    // 使用 %.16g 但去除尾部无意义的 0 和浮点误差
    char buf[64];
    snprintf(buf, sizeof(buf), "%.16g", num);
    
    // 如果数字非常小或非常大，保持科学计数法
    if (fabs(num) < 1e-4 || fabs(num) >= 1e15) {
        if (use_colors) {
            printf("%s%s%s", COLOR_NUM, buf, COLOR_RESET);
        } else {
            printf("%s", buf);
        }
        return;
    }
    
    // 对于普通小数，尝试用更少的精度重新格式化以避免浮点误差
    // 尝试 1-15 位小数，找到最短的精确表示
    // 使用截断而不是四舍五入，避免精度边界问题
    for (int precision = 1; precision <= 15; precision++) {
        // 先用高精度格式化（precision+2 保证有足够的位数）
        char temp_buf[64];
        snprintf(temp_buf, sizeof(temp_buf), "%.*f", precision + 2, num);
        
        // 手动截断到目标精度（不四舍五入）
        char *dot = strchr(temp_buf, '.');
        if (dot && strlen(dot) > precision + 1) {
            dot[precision + 1] = '\0';  // 截断到目标精度
        }
        
        // 检查截断后的值是否足够接近原值
        double reparsed = atof(temp_buf);
        if (fabs(reparsed - num) < 1e-15) {
            // 去除尾部的 0
            size_t len = strlen(temp_buf);
            while (len > 0 && temp_buf[len-1] == '0') {
                temp_buf[--len] = '\0';
            }
            // 如果最后是小数点，也去掉
            if (len > 0 && temp_buf[len-1] == '.') {
                temp_buf[--len] = '\0';
            }
            
            if (use_colors) {
                printf("%s%s%s", COLOR_NUM, temp_buf, COLOR_RESET);
            } else {
                printf("%s", temp_buf);
            }
            return;
        }
    }
    
    // 降级方案：使用 %.16g
    snprintf(buf, sizeof(buf), "%.16g", num);
    if (use_colors) {
        printf("%s%s%s", COLOR_NUM, buf, COLOR_RESET);
    } else {
        printf("%s", buf);
    }
}

/* 循环引用检测 - 用于打印时追踪正在访问的对象/数组 */
#define MAX_PRINT_DEPTH 64
typedef struct {
    Value* visited[MAX_PRINT_DEPTH];
    int count;
} PrintVisitedStack;

static int print_is_circular(Value* v, PrintVisitedStack* stack) {
    if (!v || (v->type != VALUE_ARRAY && v->type != VALUE_OBJECT)) {
        return 0;
    }
    for (int i = 0; i < stack->count; i++) {
        if (stack->visited[i] == v) {
            return 1;
        }
    }
    return 0;
}

static int print_push_visited(Value* v, PrintVisitedStack* stack) {
    if (stack->count >= MAX_PRINT_DEPTH) {
        return 0;  // 达到深度限制
    }
    stack->visited[stack->count++] = v;
    return 1;
}

static void print_pop_visited(PrintVisitedStack* stack) {
    if (stack->count > 0) {
        stack->count--;
    }
}

/* 带循环检测的打印函数前向声明 */
static void print_array_json_depth_safe(Value *arr_value, int depth, PrintVisitedStack *stack);
static void print_object_json_depth_safe(Value *obj_value, int depth, PrintVisitedStack *stack);
static void print_extended_object_meta_depth(Value *v, int depth);
static void print_value_json_depth_safe(Value *v, int depth, PrintVisitedStack *stack);

static void print_value_json_depth_safe(Value *v, int depth, PrintVisitedStack *stack) {
    int use_colors = should_use_colors();
    
    if (!v) {
        if (use_colors) printf("%snull%s", COLOR_GRAY, COLOR_RESET);
        else printf("null");
        return;
    }
    
    // 检查循环引用
    if (print_is_circular(v, stack)) {
        if (use_colors) printf("%s[Circular]%s", COLOR_GRAY, COLOR_RESET);
        else printf("[Circular]");
        return;
    }
    
    // 检查深度限制
    if (depth >= MAX_PRINT_DEPTH) {
        if (use_colors) printf("%s[...]%s", COLOR_GRAY, COLOR_RESET);
        else printf("[...]");
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER: {
            print_smart_number(v->data.number, use_colors);
            break;
        }
        case VALUE_STRING:
            if (use_colors) {
                printf("%s\"%s\"%s", ANSI_RED_BROWN, v->data.string, COLOR_RESET);
            } else {
                printf("\"%s\"", v->data.string);
            }
            break;
        case VALUE_BOOL:
            if (use_colors) {
                printf("%s%s%s", ANSI_BLUE, v->data.number != 0 ? "true" : "false", COLOR_RESET);
            } else {
                printf("%s", v->data.number != 0 ? "true" : "false");
            }
            break;
        case VALUE_NULL:
            if (use_colors) printf("%snull%s", COLOR_GRAY, COLOR_RESET);
            else printf("null");
            break;
        case VALUE_UNDEF:
            if (use_colors) printf("%sundef%s", COLOR_GRAY, COLOR_RESET);
            else printf("undef");
            break;
        case VALUE_FUNCTION: {
            // 打印函数类型
            FunctionObject *fn = (FunctionObject*)v->data.pointer;
            if (fn) {
                if (use_colors) {
                    printf("\033[36m[Function: %p, params=%d, captured=%d]\033[0m",
                           fn->func_ptr, fn->param_count, fn->captured_count);
                } else {
                    printf("[Function: %p, params=%d, captured=%d]",
                           fn->func_ptr, fn->param_count, fn->captured_count);
                }
            } else {
                if (use_colors) printf("\033[36m[Function]\033[0m");
                else printf("[Function]");
            }
            break;
        }
        case VALUE_ARRAY: {
            print_array_json_depth_safe(v, depth, stack);
            break;
        }
        case VALUE_OBJECT: {
            /* 检查是否为扩展对象类型 */
            if (v->ext_type != EXT_TYPE_NONE) {
                print_extended_object_meta_depth(v, depth);
            } else {
                print_object_json_depth_safe(v, depth, stack);
            }
            break;
        }
        default:
            if (use_colors) printf("%sunknown%s", COLOR_GRAY, COLOR_RESET);
            else printf("unknown");
    }
}

static void print_array_json_depth_safe(Value *arr_value, int depth, PrintVisitedStack *stack) {
    int use_colors = should_use_colors();
    const char* bracket_color = use_colors ? bracket_colors[depth % NUM_BRACKET_COLORS] : "";
    
    Value **arr = (Value **)arr_value->data.pointer;
    long size = arr_value->array_size;
    
    // 将数组加入访问栈
    print_push_visited(arr_value, stack);
    
    if (use_colors) printf("%s", bracket_color);
    printf("[");
    if (use_colors) printf("%s", COLOR_RESET);
    
    for (long i = 0; i < size; i++) {
        if (i > 0) printf(", ");
        print_value_json_depth_safe(arr[i], depth + 1, stack);
    }
    
    if (use_colors) printf("%s", bracket_color);
    printf("]");
    if (use_colors) printf("%s", COLOR_RESET);
    
    // 从访问栈移除
    print_pop_visited(stack);
}

static void print_object_json_depth_safe(Value *obj_value, int depth, PrintVisitedStack *stack) {
    int use_colors = should_use_colors();
    const char* bracket_color = use_colors ? bracket_colors[depth % NUM_BRACKET_COLORS] : "";
    
    ObjectEntry *entries = (ObjectEntry *)obj_value->data.pointer;
    long count = obj_value->array_size;
    
    // 将对象加入访问栈
    print_push_visited(obj_value, stack);
    
    if (use_colors) printf("%s", bracket_color);
    printf("{ ");
    if (use_colors) printf("%s", COLOR_RESET);
    
    for (long i = 0; i < count; i++) {
        if (i > 0) printf(", ");
        
        // 打印键（使用默认颜色）
        printf("%s: ", entries[i].key);
        
        // 打印值
        print_value_json_depth_safe(entries[i].value, depth + 1, stack);
    }
    
    if (use_colors) printf("%s", bracket_color);
    printf(" }");
    if (use_colors) printf("%s", COLOR_RESET);
    
    // 从访问栈移除
    print_pop_visited(stack);
}

/* 兼容接口 - 创建新的访问栈 */
static void print_value_json_depth(Value *v, int depth) {
    PrintVisitedStack stack = {0};
    print_value_json_depth_safe(v, depth, &stack);
}

static void print_array_json_depth(Value **arr, long size, int depth) {
    // 创建临时 Value 来追踪
    Value temp_arr;
    temp_arr.type = VALUE_ARRAY;
    temp_arr.data.pointer = arr;
    temp_arr.array_size = size;
    
    PrintVisitedStack stack = {0};
    print_array_json_depth_safe(&temp_arr, depth, &stack);
}

/* 递归打印数组内容为JSON格式（兼容接口）*/
static void print_array_json(Value **arr, long size) {
    print_array_json_depth(arr, size, 0);
}

/* 打印对象内容为JSON格式 */
static void print_object_json_depth(ObjectEntry *entries, long count, int depth) {
    // 创建临时 Value 来追踪
    Value temp_obj;
    temp_obj.type = VALUE_OBJECT;
    temp_obj.data.pointer = entries;
    temp_obj.array_size = count;
    temp_obj.ext_type = EXT_TYPE_NONE;
    
    PrintVisitedStack stack = {0};
    print_object_json_depth_safe(&temp_obj, depth, &stack);
}

/* Print extended object meta information */
static void print_extended_object_meta_depth(Value *v, int depth) {
    int use_colors = should_use_colors();
    const char* type_color = use_colors ? COLOR_GREEN : "";
    const char* bracket_color = use_colors ? bracket_colors[depth % NUM_BRACKET_COLORS] : "";
    const char* string_color = use_colors ? ANSI_RED_BROWN : "";
    const char* number_color = use_colors ? COLOR_NUM : "";
    const char* bool_color = use_colors ? ANSI_BLUE : "";
    const char* reset = use_colors ? COLOR_RESET : "";
    
    switch (v->ext_type) {
        case EXT_TYPE_BUFFER: {
            BufferObject *buf = (BufferObject*)v->data.pointer;
            printf("%sBuffer%s %s{%s size: %s%zu%s, type: %s\"Buffer\"%s %s}%s", 
                   type_color, reset, bracket_color, reset,
                   number_color, buf ? buf->size : 0, reset,
                   string_color, reset,
                   bracket_color, reset);
            break;
        }
        case EXT_TYPE_FILE: {
            FileHandleObject *file = (FileHandleObject*)v->data.pointer;
            if (file) {
                printf("%sFileHandle%s %s{%s path: %s\"%s\"%s, mode: %s\"%s\"%s, position: %s%ld%s, isOpen: %s%s%s %s}%s", 
                       type_color, reset, bracket_color, reset,
                       string_color, file->path ? file->path : "", reset,
                       string_color, file->mode ? file->mode : "", reset,
                       number_color, file->position, reset,
                       bool_color, file->is_open ? "true" : "false", reset,
                       bracket_color, reset);
            } else {
                printf("%sFileHandle%s %s{%s %s}%s", type_color, reset, bracket_color, reset, bracket_color, reset);
            }
            break;
        }
        case EXT_TYPE_ERROR: {
            ErrorObject *err = (ErrorObject*)v->data.pointer;
            if (err) {
                printf("%sError%s %s{%s message: %s\"%s\"%s, code: %s%d%s, errorType: %s\"%s\"%s %s}%s",
                       type_color, reset, bracket_color, reset,
                       string_color, err->message ? err->message : "", reset,
                       number_color, err->code, reset,
                       string_color, err->error_type ? err->error_type : "Error", reset,
                       bracket_color, reset);
            } else {
                printf("%sError%s %s{%s %s}%s", type_color, reset, bracket_color, reset, bracket_color, reset);
            }
            break;
        }
        default:
            printf("%sExtendedObject%s %s{%s type: %s%d%s %s}%s", 
                   type_color, reset, bracket_color, reset, 
                   number_color, v->ext_type, reset,
                   bracket_color, reset);
            break;
    }
}

/* Print extended object meta information (wrapper for depth 0) */
static void print_extended_object_meta(Value *v) {
    print_extended_object_meta_depth(v, 0);
}

/* Print a value with circular reference detection */
void value_print(Value *v) {
    int use_colors = should_use_colors();
    
    if (!v) {
        if (use_colors) printf("%sundef%s", COLOR_GRAY, COLOR_RESET);
        else printf("undef");
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER:
            print_smart_number(v->data.number, use_colors);
            break;
        case VALUE_STRING:
            /* 直接的字符串不变色，保持默认终端颜色 */
            if (v->data.string && v->string_length > 0) {
                fwrite(v->data.string, 1, v->string_length, stdout);
            }
            break;
        case VALUE_BOOL:
            if (use_colors) {
                printf("%s%s%s", ANSI_BLUE, v->data.number != 0 ? "true" : "false", COLOR_RESET);
            } else {
                printf("%s", v->data.number != 0 ? "true" : "false");
            }
            break;
        case VALUE_NULL:
            if (use_colors) printf("%snull%s", COLOR_GRAY, COLOR_RESET);
            else printf("null");
            break;
        case VALUE_UNDEF:
            if (use_colors) printf("%sundef%s", COLOR_GRAY, COLOR_RESET);
            else printf("undef");
            break;
        case VALUE_ARRAY: {
            /* 输出JSON格式的数组（带循环检测）*/
            Value **arr = (Value **)v->data.pointer;
            if (!arr || v->array_size == 0) {
                const char* bracket_color = use_colors ? bracket_colors[0] : "";
                if (use_colors) printf("%s", bracket_color);
                printf("[]");
                if (use_colors) printf("%s", COLOR_RESET);
            } else {
                PrintVisitedStack stack = {0};
                print_array_json_depth_safe(v, 0, &stack);
            }
            break;
        }
        case VALUE_OBJECT: {
            /* 检查是否为扩展对象类型 */
            if (v->ext_type != EXT_TYPE_NONE) {
                print_extended_object_meta(v);  /* 顶层打印使用 depth=0 */
                break;
            }
            
            /* 输出JSON格式的普通对象（带循环检测）*/
            ObjectEntry *entries = (ObjectEntry *)v->data.pointer;
            if (!entries || v->array_size == 0) {
                const char* bracket_color = use_colors ? bracket_colors[0] : "";
                if (use_colors) printf("%s", bracket_color);
                printf("{}");
                if (use_colors) printf("%s", COLOR_RESET);
            } else {
                PrintVisitedStack stack = {0};
                print_object_json_depth_safe(v, 0, &stack);
            }
            break;
        }
        case VALUE_FUNCTION: {
            /* 打印函数信息 */
            FunctionObject *fn = (FunctionObject*)v->data.pointer;
            if (fn) {
                if (use_colors) {
                    printf("\033[36m[Function: %p, params=%d, captured=%d]\033[0m",
                           fn->func_ptr, fn->param_count, fn->captured_count);
                } else {
                    printf("[Function: %p, params=%d, captured=%d]",
                           fn->func_ptr, fn->param_count, fn->captured_count);
                }
            } else {
                if (use_colors) printf("\033[36m[Function]\033[0m");
                else printf("[Function]");
            }
            break;
        }
        default:
            if (use_colors) printf("%sunknown%s", COLOR_GRAY, COLOR_RESET);
            else printf("unknown");
    }
    
    // 标记最后输出不是换行
    g_runtime_state.last_output_was_newline = 0;
}

/* Print value with newline */
void value_println(Value *v) {
    if (v) {
        value_print(v);
    }
    printf("\n");
    
    // 标记最后输出是换行
    g_runtime_state.last_output_was_newline = 1;
}

/* 检查最后输出是否需要换行 (供程序结束时使用) */
int value_needs_final_newline() {
    return !g_runtime_state.last_output_was_newline;
}

/* 打印致命错误并退出 */
/* 打印致命错误并退出 */
void value_fatal_error() {
    const char *platform;

    /* 简单的平台与架构检测 */
    #if defined(__APPLE__)
        #if defined(__aarch64__) || defined(__arm64__)
            platform = "macOS-arm64";
        #elif defined(__x86_64__)
            platform = "macOS-x86_64";
        #else
            platform = "macOS-unknown";
        #endif
    #elif defined(__linux__)
        #if defined(__aarch64__) || defined(__arm64__)
            platform = "Linux-arm64";
        #elif defined(__x86_64__)
            platform = "Linux-x86_64";
        #else
            platform = "Linux-unknown";
        #endif
    #elif defined(_WIN32)
        #if defined(_WIN64)
            platform = "Windows-x64";
        #else
            platform = "Windows-x86";
        #endif
    #else
        platform = "unknown-platform";
    #endif

    /* 获取当前本地时间并格式化 */
    time_t now = time(NULL);
    const char *time_str = "unknown-time";
    char time_buf[32];

    if (now != (time_t)-1) {
        struct tm tm_info;
        /* 线程安全版本：Windows / 其他平台分别处理 */
        #if defined(_WIN32)
            if (localtime_s(&tm_info, &now) == 0) {
                if (strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info) > 0) {
                    time_str = time_buf;
                }
            }
        #else
            if (localtime_r(&now, &tm_info) != NULL) {
                if (strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info) > 0) {
                    time_str = time_buf;
                }
            }
        #endif
    }

    fprintf(stderr,
            "\n\033[38;5;27mFLYUX\033[38;5;39m %s\033[0m (%s)\n"
            "\033[31m[Err]\033[0m Fatal Error: %s\n"
            "\033[33mExecution Terminated.\033[0m  [%s]\n",
            FLYUXC_VERSION,
            platform,
            g_runtime_state.error_msg,
            time_str);

    exit(g_runtime_state.last_status);
}

/* ============================================================================
 * throwErr - 用户主动抛出错误
 * ============================================================================
 * 接受 1-2 个参数:
 *   throwErr(errorType)              - 仅错误类型
 *   throwErr(errorType, message)     - 错误类型 + 详细消息
 * 
 * 错误类型映射:
 *   "error"  -> FLYUX_ERROR (1)
 *   "type"   -> FLYUX_TYPE_ERROR (3)
 *   "bounds" -> FLYUX_OUT_OF_BOUNDS (4)
 *   "io"     -> FLYUX_IO_ERROR (5)
 *   "math"   -> FLYUX_MATH_ERROR (6)
 *   其他     -> FLYUX_ERROR (1)
 * ============================================================================
 */
Value* throwErr(Value **args, int arg_count) {
    if (arg_count < 1) {
        set_runtime_status(FLYUX_ERROR, "(throwErr) At least 1 argument required");
        return box_null();
    }
    
    // 第一个参数: 错误类型（字符串）
    Value *type_val = args[0];
    if (!type_val || type_val->type != VALUE_STRING) {
        set_runtime_status(FLYUX_ERROR, "(throwErr) First argument must be a string (error type)");
        return box_null();
    }
    
    const char *error_type = type_val->data.string;
    int status = FLYUX_ERROR;  // 默认错误类型
    
    // 映射错误类型字符串到状态码
    if (strcmp(error_type, "error") == 0) {
        status = FLYUX_ERROR;
    } else if (strcmp(error_type, "type") == 0) {
        status = FLYUX_TYPE_ERROR;
    } else if (strcmp(error_type, "bounds") == 0) {
        status = FLYUX_OUT_OF_BOUNDS;
    } else if (strcmp(error_type, "io") == 0) {
        status = FLYUX_IO_ERROR;
    } else if (strcmp(error_type, "math") == 0) {
        status = FLYUX_MATH_ERROR;
    } else if (strcmp(error_type, "eof") == 0) {
        status = FLYUX_EOF;
    } else {
        // 未知类型，使用默认 FLYUX_ERROR
        status = FLYUX_ERROR;
    }
    
    // 第二个参数（可选）: 错误消息
    const char *message = NULL;
    if (arg_count >= 2) {
        Value *msg_val = args[1];
        if (msg_val && msg_val->type == VALUE_STRING) {
            message = msg_val->data.string;
        }
    }
    
    // 设置错误状态
    if (message) {
        set_runtime_status(status, message);
    } else {
        // 如果没有提供消息，使用默认消息
        char default_msg[128];
        snprintf(default_msg, sizeof(default_msg), "User threw '%s' error", error_type);
        set_runtime_status(status, default_msg);
    }
    
    return box_null();
}

/* ============================================================================
 * sysinfo - 获取系统信息
 * ============================================================================
 * 返回包含系统信息的对象
 * ============================================================================
 */
Value* value_sysinfo() {
    set_runtime_status(FLYUX_OK, NULL);
    
    // 定义与 box_object 兼容的结构
    typedef struct { char *key; Value *value; } ObjEntry;
    
    #define MAX_FIELDS 16
    ObjEntry entries[MAX_FIELDS];
    int count = 0;
    
    // 1. CPU架构
    entries[count].key = "arch";
    #if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
        entries[count].value = box_string("arm64");
    #elif defined(__x86_64__) || defined(_M_X64)
        entries[count].value = box_string("x86_64");
    #elif defined(__i386__) || defined(_M_IX86)
        entries[count].value = box_string("i386");
    #else
        entries[count].value = box_string("unknown");
    #endif
    count++;
    
    // 2. 操作系统
    entries[count].key = "os";
    #if defined(__APPLE__) && defined(__MACH__)
        entries[count].value = box_string("darwin");
    #elif defined(__linux__)
        entries[count].value = box_string("linux");
    #elif defined(_WIN32) || defined(_WIN64)
        entries[count].value = box_string("windows");
    #else
        entries[count].value = box_string("unknown");
    #endif
    count++;
    
    // 3. 环境变量（作为对象）
    entries[count].key = "env";
    extern char **environ;
    int env_count = 0;
    for (char **env = environ; *env != NULL; env++) {
        env_count++;
    }
    
    ObjEntry *env_entries = (ObjEntry*)malloc(sizeof(ObjEntry) * env_count);
    int env_idx = 0;
    for (char **env = environ; *env != NULL && env_idx < env_count; env++) {
        char *eq = strchr(*env, '=');
        if (eq) {
            size_t key_len = eq - *env;
            env_entries[env_idx].key = strndup(*env, key_len);
            env_entries[env_idx].value = box_string(strdup(eq + 1));
            env_idx++;
        }
    }
    entries[count].value = box_object(env_entries, env_idx);
    free(env_entries);
    count++;
    
    // 4. 当前工作目录
    entries[count].key = "cwd";
    char cwd_buf[4096];
    if (getcwd(cwd_buf, sizeof(cwd_buf))) {
        entries[count].value = box_string(strdup(cwd_buf));
    } else {
        entries[count].value = box_null();
    }
    count++;
    
    // 5. 用户主目录
    entries[count].key = "home";
    const char *home = getenv("HOME");
    entries[count].value = home ? box_string((char*)home) : box_null();
    count++;
    
    // 6. 临时目录
    entries[count].key = "temp";
    const char *tmpdir = getenv("TMPDIR");
    if (!tmpdir) tmpdir = getenv("TEMP");
    if (!tmpdir) tmpdir = getenv("TMP");
    if (!tmpdir) tmpdir = "/tmp";
    entries[count].value = box_string((char*)tmpdir);
    count++;
    
    // 7. 进程信息
    entries[count].key = "process";
    ObjEntry proc_entries[4];
    proc_entries[0].key = "pid";
    proc_entries[0].value = box_number((double)getpid());
    proc_entries[1].key = "ppid";
    proc_entries[1].value = box_number((double)getppid());
    proc_entries[2].key = "uid";
    proc_entries[2].value = box_number((double)getuid());
    proc_entries[3].key = "gid";
    proc_entries[3].value = box_number((double)getgid());
    entries[count].value = box_object(proc_entries, 4);
    count++;
    
    // 8. 主机名
    entries[count].key = "hostname";
    char hostname_buf[256];
    if (gethostname(hostname_buf, sizeof(hostname_buf)) == 0) {
        entries[count].value = box_string(strdup(hostname_buf));
    } else {
        entries[count].value = box_null();
    }
    count++;
    
    // 9. 用户名
    entries[count].key = "username";
    const char *user = getenv("USER");
    if (!user) user = getenv("USERNAME");
    entries[count].value = user ? box_string((char*)user) : box_null();
    count++;
    
    // 10. 当前时间戳
    entries[count].key = "timestamp";
    entries[count].value = box_number((double)time(NULL));
    count++;
    
    // 构建最终对象
    return box_object(entries, count);
}

/* Get type of value as string */
char* value_typeof(Value *v) {
    if (!v) return "undef";
    
    /* 检查扩展对象类型 */
    if (v->declared_type == VALUE_OBJECT && v->ext_type != EXT_TYPE_NONE) {
        static char type_buf[64];
        const char *ext_name = NULL;
        
        switch (v->ext_type) {
            case EXT_TYPE_BUFFER:
                ext_name = "Buffer";
                break;
            case EXT_TYPE_FILE:
                ext_name = "FileHandle";
                break;
            case EXT_TYPE_ERROR:
                ext_name = "Error";
                break;
            default:
                ext_name = "Extended";
                break;
        }
        
        snprintf(type_buf, sizeof(type_buf), "obj:%s", ext_name);
        return type_buf;
    }
    
    /* 返回声明类型而不是实际类型 */
    switch (v->declared_type) {
        case VALUE_NUMBER: return "num";
        case VALUE_STRING: return "str";
        case VALUE_BOOL: return "bl";
        case VALUE_NULL: return "null";
        case VALUE_ARRAY: return "obj";  // 数组也是对象类型
        case VALUE_OBJECT: return "obj";
        case VALUE_FUNCTION: return "func";  // 函数类型
        default: return "unknown";
    }
}

/* Value arithmetic operations */
Value* value_add(Value *a, Value *b) {
    // String concatenation
    if (a->type == VALUE_STRING || b->type == VALUE_STRING) {
        char *sa = unbox_string(a);
        char *sb = unbox_string(b);
        size_t len = strlen(sa) + strlen(sb) + 1;
        char *result = (char*)malloc(len);
        strcpy(result, sa);
        strcat(result, sb);
        // 使用 box_string_owned 因为 result 是动态分配的
        return box_string_owned(result);
    }
    
    // Numeric addition
    double na = unbox_number(a);
    double nb = unbox_number(b);
    return box_number(na + nb);
}

Value* value_subtract(Value *a, Value *b) {
    return box_number(unbox_number(a) - unbox_number(b));
}

Value* value_multiply(Value *a, Value *b) {
    return box_number(unbox_number(a) * unbox_number(b));
}

Value* value_divide(Value *a, Value *b) {
    double dividend = unbox_number(a);
    double divisor = unbox_number(b);
    
    if (divisor == 0.0) {
        if (dividend == 0.0) {
            // 0 / 0 = NaN
            return box_number(NAN);
        } else if (dividend > 0.0) {
            // 正数 / 0 = Infinity
            return box_number(INFINITY);
        } else {
            // 负数 / 0 = -Infinity
            return box_number(-INFINITY);
        }
    }
    
    return box_number(dividend / divisor);
}

Value* value_power(Value *a, Value *b) {
    return box_number(pow(unbox_number(a), unbox_number(b)));
}

Value* value_modulo(Value *a, Value *b) {
    double divisor = unbox_number(b);
    if (divisor == 0) {
        return box_number(0.0 / 0.0);  // NaN for mod by zero
    }
    return box_number(fmod(unbox_number(a), divisor));
}

/* Value comparison */
Value* value_equals(Value *a, Value *b) {
    if (!a || !b) return box_bool(a == b);
    
    // Handle null comparisons explicitly
    if (a->type == VALUE_NULL || b->type == VALUE_NULL) {
        return box_bool(a->type == VALUE_NULL && b->type == VALUE_NULL);
    }
    
    if (a->type != b->type) {
        // Type coercion comparison (for non-null types)
        return box_bool(unbox_number(a) == unbox_number(b));
    }
    
    switch (a->type) {
        case VALUE_NUMBER:
        case VALUE_BOOL:
            return box_bool(a->data.number == b->data.number);
        case VALUE_STRING:
            if (!a->data.string || !b->data.string)
                return box_bool(a->data.string == b->data.string);
            return box_bool(strcmp(a->data.string, b->data.string) == 0);
        default:
            return box_bool(a == b);  // reference equality
    }
}

Value* value_less_than(Value *a, Value *b) {
    return box_bool(unbox_number(a) < unbox_number(b));
}

Value* value_greater_than(Value *a, Value *b) {
    return box_bool(unbox_number(a) > unbox_number(b));
}

/* Array/Object index access - runtime version
 * 设置错误状态，由调用方决定是否终止程序
 * 
 * 注意：返回的值会增加引用计数，调用方负责在使用后调用 value_release */
Value* value_index(Value *obj, Value *index) {
    if (!obj) {
        set_runtime_status(FLYUX_TYPE_ERROR, "Attempt to index null value");
        return box_null();
    }
    
    // For arrays with numeric index
    if (obj->type == VALUE_ARRAY && obj->data.pointer) {
        int idx = (int)unbox_number(index);
        Value **array = (Value **)obj->data.pointer;
        size_t arr_size = obj->array_size;
        
        // 边界检查
        if (idx < 0 || (size_t)idx >= arr_size) {
            set_runtime_status(FLYUX_OUT_OF_BOUNDS, "Array index out of bounds");
            return box_null();
        }
        // 增加引用计数，调用方负责释放
        return value_retain(array[idx]);
    }
    
    // For Buffer objects with numeric index
    if (obj->type == VALUE_OBJECT && obj->ext_type == EXT_TYPE_BUFFER && index && index->type == VALUE_NUMBER) {
        BufferObject *buf = (BufferObject*)obj->data.pointer;
        int idx = (int)unbox_number(index);
        
        // 边界检查
        if (idx < 0 || (size_t)idx >= buf->size) {
            set_runtime_status(FLYUX_OUT_OF_BOUNDS, "Buffer index out of bounds");
            return box_null();
        }
        
        // 返回字节值（0-255）- 新创建的值，无需额外 retain
        return box_number((double)buf->data[idx]);
    }
    
    // For objects with string index (inline implementation to avoid forward declaration)
    if (obj->type == VALUE_OBJECT && index && index->type == VALUE_STRING) {
        const char *key = (const char*)index->data.pointer;
        ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
        
        // 检查是否是哈希模式（string_length > 0 表示哈希容量）
        if (obj->string_length > 0) {
            // 哈希模式：使用哈希查找
            size_t capacity = obj->string_length;
            unsigned long hash = 14695981039346656037UL;  // FNV offset basis
            const char *s = key;
            while (*s) {
                hash ^= (unsigned char)*s++;
                hash *= 1099511628211UL;  // FNV prime
            }
            
            size_t idx = hash % capacity;
            size_t start = idx;
            
            do {
                if (entries[idx].key == NULL) {
                    // 空槽，键不存在
                    break;
                }
                if (entries[idx].key != (char*)(intptr_t)-1 && strcmp(entries[idx].key, key) == 0) {
                    return value_retain(entries[idx].value);
                }
                idx = (idx + 1) % capacity;
            } while (idx != start);
            
            // 键不存在
            set_runtime_status(FLYUX_TYPE_ERROR, "Object key not found");
            return box_null();
        } else {
            // 线性模式
            size_t count = obj->array_size;
            for (size_t i = 0; i < count; i++) {
                if (strcmp(entries[i].key, key) == 0) {
                    return value_retain(entries[i].value);
                }
            }
            // 键不存在
            set_runtime_status(FLYUX_TYPE_ERROR, "Object key not found");
            return box_null();
        }
    }

    set_runtime_status(FLYUX_TYPE_ERROR, "Invalid index operation");
    return box_null();
}

/* Array/Object index access - safe version for optional chaining (?[])
 * 注意：返回的值会增加引用计数，调用方负责在使用后调用 value_release */
Value* value_index_safe(Value *obj, Value *index) {
    if (!obj || !index) {
        return box_undef();
    }
    
    // For arrays with numeric index
    if (obj->type == VALUE_ARRAY && obj->data.pointer) {
        int idx = (int)unbox_number(index);
        Value **array = (Value **)obj->data.pointer;
        size_t arr_size = obj->array_size;
        
        // 边界检查 - 越界返回 undef
        if (idx < 0 || (size_t)idx >= arr_size) {
            return box_undef();
        }
        // 增加引用计数，调用方负责释放
        return value_retain(array[idx]);
    }
    
    // For Buffer objects with numeric index
    if (obj->type == VALUE_OBJECT && obj->ext_type == EXT_TYPE_BUFFER && index->type == VALUE_NUMBER) {
        BufferObject *buf = (BufferObject*)obj->data.pointer;
        int idx = (int)unbox_number(index);
        
        // 边界检查 - 越界返回 undef
        if (idx < 0 || (size_t)idx >= buf->size) {
            return box_undef();
        }
        
        return box_number((double)buf->data[idx]);
    }
    
    // For objects with string index
    if (obj->type == VALUE_OBJECT && index->type == VALUE_STRING) {
        const char *key = (const char*)index->data.pointer;
        ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
        
        // 检查是否是哈希模式（string_length > 0 表示哈希容量）
        if (obj->string_length > 0) {
            // 哈希模式：使用哈希查找
            size_t capacity = obj->string_length;
            unsigned long hash = 14695981039346656037UL;  // FNV offset basis
            const char *s = key;
            while (*s) {
                hash ^= (unsigned char)*s++;
                hash *= 1099511628211UL;  // FNV prime
            }
            
            size_t idx = hash % capacity;
            size_t start = idx;
            
            do {
                if (entries[idx].key == NULL) {
                    // 空槽，键不存在
                    break;
                }
                if (entries[idx].key != (char*)(intptr_t)-1 && strcmp(entries[idx].key, key) == 0) {
                    return value_retain(entries[idx].value);
                }
                idx = (idx + 1) % capacity;
            } while (idx != start);
        } else {
            // 线性模式
            size_t count = obj->array_size;
            for (size_t i = 0; i < count; i++) {
                if (strcmp(entries[i].key, key) == 0) {
                    return value_retain(entries[i].value);
                }
            }
        }
    }

    // 无法访问，返回 undef（不报错）
    return box_undef();
}

/* Free a value */
void value_free(Value *v) {
    if (v) {
        // Note: we don't free strings as they might be global constants
        free(v);
    }
}

/*
 * 将值转换为字符串（用于 %s 格式符，不带引号）
 */
static void value_to_string(Value *v, char *buf, size_t size) {
    if (!v || !buf || size == 0) {
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER:
            if (isinf(v->data.number)) {
                snprintf(buf, size, "%s", v->data.number > 0 ? "+Inf" : "-Inf");
            } else if (isnan(v->data.number)) {
                snprintf(buf, size, "NaN");
            } else {
                snprintf(buf, size, "%.16g", v->data.number);
            }
            break;
        case VALUE_STRING:
            snprintf(buf, size, "%s", v->data.string);  // 不带引号
            break;
        case VALUE_BOOL:
            snprintf(buf, size, "%s", v->data.number != 0 ? "true" : "false");
            break;
        case VALUE_NULL:
            snprintf(buf, size, "null");
            break;
        case VALUE_UNDEF:
            snprintf(buf, size, "undef");
            break;
        case VALUE_ARRAY:
            snprintf(buf, size, "[...]");
            break;
        case VALUE_OBJECT:
            snprintf(buf, size, "{...}");
            break;
        default:
            snprintf(buf, size, "unknown");
            break;
    }
}

/*
 * 格式化浮点数到指定精度（截断而不是四舍五入）
 * 用于 printf 的 %f 格式符
 */
static void format_double_truncate(char *buf, size_t buf_size, double num, int precision) {
    if (precision < 0) precision = 6;  // 默认精度
    
    // 使用更高的精度先格式化
    char temp[256];
    snprintf(temp, sizeof(temp), "%.*f", precision + 2, num);
    
    // 找到小数点
    char *dot = strchr(temp, '.');
    if (dot && precision >= 0) {
        // 截断到目标精度（不四舍五入）
        size_t dot_pos = dot - temp;
        if (precision == 0) {
            // precision=0 时不输出小数点
            temp[dot_pos] = '\0';
        } else {
            // precision>0 时保留小数点和小数位
            size_t target_len = dot_pos + 1 + precision;  // 整数部分 + . + 小数位
            if (target_len < strlen(temp)) {
                temp[target_len] = '\0';
            }
        }
    }
    
    snprintf(buf, buf_size, "%s", temp);
}

/* 
 * FLYUX printf 函数
 * 支持格式化输出，类似 C 的 printf
 * 格式说明符：
 *   %d, %i - 整数
 *   %f - 浮点数（默认6位小数）
 *   %.Nf - N位小数的浮点数
 *   %s - 字符串（支持宽度和对齐，如 %10s, %-10s）
 *   %b - 布尔值
 *   %v - 值（自动判断类型，JSON格式）
 *   %% - 百分号字面量
 */
void value_printf(Value *format, Value **args, long arg_count) {
    if (!format || format->type != VALUE_STRING) {
        return;
    }
    
    char *fmt = format->data.string;
    size_t fmt_len = format->string_length;
    long arg_index = 0;
    
    for (size_t i = 0; i < fmt_len; i++) {
        if (fmt[i] == '%' && i + 1 < fmt_len) {
            char spec = fmt[i + 1];
            
            if (spec == '%') {
                // %% -> %
                putchar('%');
                i++;
                continue;
            }
            
            if (arg_index >= arg_count) {
                // 参数不足，打印原样
                putchar('%');
                continue;
            }
            
            Value *arg = args[arg_index];
            
            // 解析格式符：支持 %[-][width][.precision]type
            // 例如：%-10s, %5d, %8.2f
            int left_align = 0;     // 是否左对齐
            int width = 0;          // 宽度
            int precision = -1;     // 精度
            size_t j = i + 1;       // 从 % 后开始解析
            
            // 1. 检查对齐标志
            if (j < fmt_len && fmt[j] == '-') {
                left_align = 1;
                j++;
            }
            
            // 2. 解析宽度
            while (j < fmt_len && isdigit(fmt[j])) {
                width = width * 10 + (fmt[j] - '0');
                j++;
            }
            
            // 3. 解析精度
            if (j < fmt_len && fmt[j] == '.') {
                j++;
                precision = 0;
                while (j < fmt_len && isdigit(fmt[j])) {
                    precision = precision * 10 + (fmt[j] - '0');
                    j++;
                }
            }
            
            // 4. 获取类型符
            if (j >= fmt_len) {
                putchar('%');
                continue;
            }
            spec = fmt[j];
            i = j;  // 更新位置到格式符
            
            switch (spec) {
                case 'd':
                case 'i': {
                    // 整数（带颜色，支持宽度）
                    double num = unbox_number(arg);
                    int use_colors = should_use_colors();
                    
                    char temp[64];
                    snprintf(temp, sizeof(temp), "%lld", (long long)num);
                    
                    if (use_colors) printf(COLOR_NUM);
                    if (width > 0) {
                        printf("%*s", left_align ? -width : width, temp);
                    } else {
                        printf("%s", temp);
                    }
                    if (use_colors) printf(COLOR_RESET);
                    break;
                }
                case 'f':
                case 'g': {
                    // 浮点数（带颜色，支持宽度和精度）
                    double num = unbox_number(arg);
                    int use_colors = should_use_colors();
                    
                    char temp[128];
                    if (isinf(num)) {
                        snprintf(temp, sizeof(temp), "%s", num > 0 ? "+Inf" : "-Inf");
                    } else if (isnan(num)) {
                        snprintf(temp, sizeof(temp), "NaN");
                    } else {
                        if (precision >= 0) {
                            if (spec == 'f') {
                                // 使用截断而不是四舍五入
                                format_double_truncate(temp, sizeof(temp), num, precision);
                            } else {  // 'g'
                                snprintf(temp, sizeof(temp), "%.*g", precision, num);
                            }
                        } else {
                            snprintf(temp, sizeof(temp), "%.16g", num);
                        }
                    }
                    
                    if (use_colors) printf(COLOR_NUM);
                    if (width > 0) {
                        printf("%*s", left_align ? -width : width, temp);
                    } else {
                        printf("%s", temp);
                    }
                    if (use_colors) printf(COLOR_RESET);
                    break;
                }
                case 's': {
                    // 字符串（带颜色，支持宽度和对齐）
                    int use_colors = should_use_colors();
                    char temp_buf[256];
                    
                    // 转换为纯文本字符串（不带引号）
                    value_to_string(arg, temp_buf, sizeof(temp_buf));
                    
                    if (use_colors) printf(ANSI_RED_BROWN);
                    if (width > 0) {
                        printf("%*s", left_align ? -width : width, temp_buf);
                    } else {
                        printf("%s", temp_buf);
                    }
                    if (use_colors) printf(COLOR_RESET);
                    break;
                }
                case 'b': {
                    // 布尔值（带颜色）
                    int use_colors = should_use_colors();
                    
                    char temp_buf[8];
                    if (arg && arg->type == VALUE_BOOL) {
                        snprintf(temp_buf, sizeof(temp_buf), "%s", arg->data.number != 0 ? "true" : "false");
                    } else {
                        snprintf(temp_buf, sizeof(temp_buf), "%s", unbox_number(arg) != 0 ? "true" : "false");
                    }
                    
                    if (use_colors) printf(ANSI_BLUE);
                    if (width > 0) {
                        printf("%*s", left_align ? -width : width, temp_buf);
                    } else {
                        printf("%s", temp_buf);
                    }
                    if (use_colors) printf(COLOR_RESET);
                    break;
                }
                case 'v': {
                    // 值（JSON格式，带颜色）
                    print_value_json_depth(arg, 0);
                    break;
                }
                default:
                    // 未知格式符，打印原样
                    putchar('%');
                    putchar(spec);
                    arg_index--;  // 不消耗参数
                    break;
            }
            
            arg_index++;
            // i 已经指向格式符位置，for 循环会自动 i++
        } else {
            // 普通字符
            putchar(fmt[i]);
        }
    }
}

/* 获取数组长度 */
long value_array_length(Value *v) {
    if (!v || v->type != VALUE_ARRAY) {
        return 0;
    }
    return v->array_size;
}

/* 数组元素访问 */
Value* value_array_get(Value *array, Value *index) {
    if (!array || array->type != VALUE_ARRAY) {
        return box_undef();
    }
    
    double idx = unbox_number(index);
    long i = (long)idx;
    
    if (i < 0 || i >= array->array_size) {
        return box_undef();  // 越界返回 undef
    }
    
    Value **elements = (Value**)array->data.pointer;
    return elements[i];
}

/* ============================================================================
 * 对象/数组拷贝函数
 * ============================================================================ */

/*
 * value_shallow_clone - 浅拷贝对象或数组
 * 对于对象：创建新对象，顶层属性复制，嵌套对象仍是引用
 * 对于数组：创建新数组，元素仍是原引用
 * 对于基本类型：直接返回（因为基本类型语义上不可变）
 */
Value* value_shallow_clone(Value *v) {
    if (!v) return box_null();
    
    switch (v->type) {
        case VALUE_NUMBER:
            return box_number(v->data.number);
            
        case VALUE_STRING:
            /* 字符串：复制字符串内容（因为字符串在 FLYUX 中应该是不可变的） */
            if (v->data.string) {
                char *new_str = strdup(v->data.string);
                return box_string_owned(new_str);
            }
            return box_string(NULL);
            
        case VALUE_BOOL:
            return box_bool((int)v->data.number);
            
        case VALUE_NULL:
            return box_null();
            
        case VALUE_UNDEF:
            return box_undef();
            
        case VALUE_ARRAY: {
            /* 浅拷贝数组：创建新数组，但元素仍是原引用 */
            Value **old_elements = (Value**)v->data.pointer;
            long size = v->array_size;
            
            Value **new_elements = NULL;
            if (size > 0 && old_elements) {
                new_elements = (Value**)malloc(sizeof(Value*) * size);
                for (long i = 0; i < size; i++) {
                    new_elements[i] = old_elements[i];
                    /* 增加引用计数，因为新数组也持有引用 */
                    if (new_elements[i]) {
                        value_retain(new_elements[i]);
                    }
                }
            }
            
            Value *result = (Value*)malloc(sizeof(Value));
            result->type = VALUE_ARRAY;
            result->declared_type = VALUE_ARRAY;
            result->refcount = 1;
            result->flags = VALUE_FLAG_NONE;
            result->ext_type = EXT_TYPE_NONE;
            result->data.pointer = new_elements;
            result->array_size = size;
            result->string_length = 0;
            return result;
        }
            
        case VALUE_OBJECT: {
            /* 浅拷贝对象：创建新对象，但嵌套对象仍是原引用 */
            typedef struct { char *key; Value *value; } ObjEntry;
            ObjEntry *old_entries = (ObjEntry*)v->data.pointer;
            long count = v->array_size;
            
            ObjEntry *new_entries = NULL;
            if (count > 0 && old_entries) {
                new_entries = (ObjEntry*)malloc(sizeof(ObjEntry) * count);
                for (long i = 0; i < count; i++) {
                    /* 复制 key */
                    new_entries[i].key = old_entries[i].key ? strdup(old_entries[i].key) : NULL;
                    /* value 仍是引用，增加引用计数 */
                    new_entries[i].value = old_entries[i].value;
                    if (new_entries[i].value) {
                        value_retain(new_entries[i].value);
                    }
                }
            }
            
            Value *result = (Value*)malloc(sizeof(Value));
            result->type = VALUE_OBJECT;
            result->declared_type = VALUE_OBJECT;
            result->refcount = 1;
            result->flags = VALUE_FLAG_NONE;
            result->ext_type = v->ext_type;  /* 保留扩展类型 */
            result->data.pointer = new_entries;
            result->array_size = count;
            result->string_length = 0;
            return result;
        }
            
        case VALUE_FUNCTION:
            /* 函数：增加引用计数并返回同一个函数（函数不可变） */
            value_retain(v);
            return v;
            
        default:
            return box_null();
    }
}

/*
 * value_deep_clone - 深拷贝对象或数组
 * 递归复制所有嵌套对象和数组，创建完全独立的副本
 */
Value* value_deep_clone(Value *v) {
    if (!v) return box_null();
    
    switch (v->type) {
        case VALUE_NUMBER:
            return box_number(v->data.number);
            
        case VALUE_STRING:
            if (v->data.string) {
                char *new_str = strdup(v->data.string);
                return box_string_owned(new_str);
            }
            return box_string(NULL);
            
        case VALUE_BOOL:
            return box_bool((int)v->data.number);
            
        case VALUE_NULL:
            return box_null();
            
        case VALUE_UNDEF:
            return box_undef();
            
        case VALUE_ARRAY: {
            /* 深拷贝数组：递归复制每个元素 */
            Value **old_elements = (Value**)v->data.pointer;
            long size = v->array_size;
            
            Value **new_elements = NULL;
            if (size > 0 && old_elements) {
                new_elements = (Value**)malloc(sizeof(Value*) * size);
                for (long i = 0; i < size; i++) {
                    /* 递归深拷贝每个元素 */
                    new_elements[i] = value_deep_clone(old_elements[i]);
                }
            }
            
            Value *result = (Value*)malloc(sizeof(Value));
            result->type = VALUE_ARRAY;
            result->declared_type = VALUE_ARRAY;
            result->refcount = 1;
            result->flags = VALUE_FLAG_NONE;
            result->ext_type = EXT_TYPE_NONE;
            result->data.pointer = new_elements;
            result->array_size = size;
            result->string_length = 0;
            return result;
        }
            
        case VALUE_OBJECT: {
            /* 深拷贝对象：递归复制每个属性值 */
            typedef struct { char *key; Value *value; } ObjEntry;
            ObjEntry *old_entries = (ObjEntry*)v->data.pointer;
            long count = v->array_size;
            
            ObjEntry *new_entries = NULL;
            if (count > 0 && old_entries) {
                new_entries = (ObjEntry*)malloc(sizeof(ObjEntry) * count);
                for (long i = 0; i < count; i++) {
                    /* 复制 key */
                    new_entries[i].key = old_entries[i].key ? strdup(old_entries[i].key) : NULL;
                    /* 递归深拷贝 value */
                    new_entries[i].value = value_deep_clone(old_entries[i].value);
                }
            }
            
            Value *result = (Value*)malloc(sizeof(Value));
            result->type = VALUE_OBJECT;
            result->declared_type = VALUE_OBJECT;
            result->refcount = 1;
            result->flags = VALUE_FLAG_NONE;
            result->ext_type = v->ext_type;  /* 保留扩展类型 */
            result->data.pointer = new_entries;
            result->array_size = count;
            result->string_length = 0;
            return result;
        }
            
        case VALUE_FUNCTION:
            /* 函数：不深拷贝，增加引用计数并返回（函数是不可变的） */
            value_retain(v);
            return v;
            
        default:
            return box_null();
    }
}

/* ============================================================================
 * 展开运算符支持函数
 * ============================================================================ */

/*
 * value_spread_into_object - 将一个对象的所有属性展开到目标对象中
 * 返回包含合并后属性的新对象
 */
Value* value_spread_into_object(Value *target, Value *source) {
    if (!target || target->type != VALUE_OBJECT) {
        return source ? value_shallow_clone(source) : box_object(NULL, 0);
    }
    if (!source || source->type != VALUE_OBJECT) {
        return value_shallow_clone(target);
    }
    
    typedef struct { char *key; Value *value; } ObjEntry;
    ObjEntry *target_entries = (ObjEntry*)target->data.pointer;
    ObjEntry *source_entries = (ObjEntry*)source->data.pointer;
    long target_count = target->array_size;
    long source_count = source->array_size;
    
    // 分配足够大的数组（最坏情况：所有属性都不重复）
    long max_count = target_count + source_count;
    ObjEntry *new_entries = (ObjEntry*)malloc(sizeof(ObjEntry) * max_count);
    long new_count = 0;
    
    // 先复制目标对象的所有属性
    for (long i = 0; i < target_count; i++) {
        new_entries[new_count].key = target_entries[i].key ? strdup(target_entries[i].key) : NULL;
        new_entries[new_count].value = target_entries[i].value;
        if (new_entries[new_count].value) value_retain(new_entries[new_count].value);
        new_count++;
    }
    
    // 然后添加/覆盖源对象的属性
    for (long i = 0; i < source_count; i++) {
        char *key = source_entries[i].key;
        Value *val = source_entries[i].value;
        
        // 查找是否已存在
        int found = 0;
        for (long j = 0; j < new_count; j++) {
            if (new_entries[j].key && key && strcmp(new_entries[j].key, key) == 0) {
                // 覆盖现有值
                if (new_entries[j].value) value_release(new_entries[j].value);
                new_entries[j].value = val;
                if (val) value_retain(val);
                found = 1;
                break;
            }
        }
        
        if (!found) {
            new_entries[new_count].key = key ? strdup(key) : NULL;
            new_entries[new_count].value = val;
            if (val) value_retain(val);
            new_count++;
        }
    }
    
    // 缩小数组到实际大小
    if (new_count < max_count) {
        new_entries = (ObjEntry*)realloc(new_entries, sizeof(ObjEntry) * new_count);
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_OBJECT;
    result->declared_type = VALUE_OBJECT;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_entries;
    result->array_size = new_count;
    result->string_length = 0;
    return result;
}

/*
 * value_spread_into_array - 将一个数组的所有元素展开到目标数组中
 * 返回包含所有元素的新数组
 */
Value* value_spread_into_array(Value *target, Value *source) {
    if (!target || target->type != VALUE_ARRAY) {
        return source && source->type == VALUE_ARRAY ? value_shallow_clone(source) : box_array(NULL, 0);
    }
    if (!source || source->type != VALUE_ARRAY) {
        return value_shallow_clone(target);
    }
    
    Value **target_elems = (Value**)target->data.pointer;
    Value **source_elems = (Value**)source->data.pointer;
    long target_count = target->array_size;
    long source_count = source->array_size;
    long new_count = target_count + source_count;
    
    Value **new_elems = NULL;
    if (new_count > 0) {
        new_elems = (Value**)malloc(sizeof(Value*) * new_count);
        
        // 复制目标数组元素
        for (long i = 0; i < target_count; i++) {
            new_elems[i] = target_elems[i];
            if (new_elems[i]) value_retain(new_elems[i]);
        }
        
        // 复制源数组元素
        for (long i = 0; i < source_count; i++) {
            new_elems[target_count + i] = source_elems[i];
            if (new_elems[target_count + i]) value_retain(new_elems[target_count + i]);
        }
    }
    
    Value *result = (Value*)malloc(sizeof(Value));
    result->type = VALUE_ARRAY;
    result->declared_type = VALUE_ARRAY;
    result->refcount = 1;
    result->flags = VALUE_FLAG_NONE;
    result->ext_type = EXT_TYPE_NONE;
    result->data.pointer = new_elems;
    result->array_size = new_count;
    result->string_length = 0;
    return result;
}

/* ============================================================================
 * 引用盒子 (Reference Box) - 用于闭包捕获可变变量
 * ============================================================================ */

/**
 * 创建引用盒子：将 Value* 包装在堆分配的 Value** 中
 * 这样闭包可以捕获这个 Value**，即使父函数返回后仍然有效
 * 
 * @param value: 要包装的值
 * @return: 包装后的 Value**（伪装成 Value*）
 */
Value* box_ref(Value *value) {
    /* 分配堆上的 Value** */
    Value **ref_box = (Value**)malloc(sizeof(Value*));
    *ref_box = value;
    
    /* Retain value to keep it alive */
    if (value) {
        value_retain(value);
    }
    
    /* 将 Value** 伪装成 Value* 返回 */
    return (Value*)ref_box;
}

/**
 * 从引用盒子中获取值
 * 
 * @param ref_box: 引用盒子（实际上是 Value**）
 * @return: 包装的值
 */
Value* ref_get(Value *ref_box) {
    if (!ref_box) return box_null();
    
    Value **box = (Value**)ref_box;
    Value *value = *box;
    
    /* 返回时增加引用计数 */
    if (value) {
        value_retain(value);
    }
    
    return value ? value : box_null();
}

/**
 * 设置引用盒子中的值
 * 
 * @param ref_box: 引用盒子（实际上是 Value**）
 * @param new_value: 新值
 */
void ref_set(Value *ref_box, Value *new_value) {
    if (!ref_box) return;
    
    Value **box = (Value**)ref_box;
    Value *old_value = *box;
    
    /* 设置新值并增加引用计数 */
    *box = new_value;
    if (new_value) {
        value_retain(new_value);
    }
    
    /* 释放旧值 */
    if (old_value) {
        value_release(old_value);
    }
}

/**
 * 释放引用盒子
 * 
 * @param ref_box: 引用盒子（实际上是 Value**）
 */
void ref_free(Value *ref_box) {
    if (!ref_box) return;
    
    Value **box = (Value**)ref_box;
    Value *value = *box;
    
    /* 释放包装的值 */
    if (value) {
        value_release(value);
    }
    
    /* 释放盒子本身 */
    free(box);
}

