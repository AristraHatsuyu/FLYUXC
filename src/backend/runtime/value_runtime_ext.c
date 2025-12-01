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

/* Box a number into a Value */
Value* box_number(double num) {
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

/* Box a boolean into a Value */
Value* box_bool(int b) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_BOOL;
    v->declared_type = VALUE_BOOL;
    v->refcount = 1;
    v->flags = VALUE_FLAG_NONE;
    v->ext_type = EXT_TYPE_NONE;
    v->data.number = b ? 1.0 : 0.0;
    v->array_size = 0;
    v->string_length = 0;
    return v;
}

/* Box null */
Value* box_null() {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NULL;
    v->declared_type = VALUE_NULL;
    v->refcount = 1;
    v->flags = VALUE_FLAG_NONE;
    v->ext_type = EXT_TYPE_NONE;
    v->data.pointer = NULL;
    v->array_size = 0;
    v->string_length = 0;
    return v;
}

/* Box undef - for undefined variables */
Value* box_undef() {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_UNDEF;
    v->declared_type = VALUE_UNDEF;
    v->refcount = 1;
    v->flags = VALUE_FLAG_NONE;
    v->ext_type = EXT_TYPE_NONE;
    v->data.pointer = NULL;
    v->array_size = 0;
    v->string_length = 0;
    return v;
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

/* isObj(value) - 检查是否为对象类型（不包括数组） */
Value* value_is_obj(Value *v) {
    return box_bool(v && v->type == VALUE_OBJECT);
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

/* 递归打印数组内容为JSON格式，支持嵌套层级的彩虹括号 */
static void print_array_json_depth(Value **arr, long size, int depth);
static void print_object_json_depth(ObjectEntry *entries, long count, int depth);
static void print_extended_object_meta_depth(Value *v, int depth);

static void print_value_json_depth(Value *v, int depth) {
    int use_colors = should_use_colors();
    
    if (!v) {
        if (use_colors) printf("%snull%s", COLOR_GRAY, COLOR_RESET);
        else printf("null");
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
        case VALUE_ARRAY: {
            Value **nested = (Value **)v->data.pointer;
            print_array_json_depth(nested, v->array_size, depth);
            break;
        }
        case VALUE_OBJECT: {
            /* 检查是否为扩展对象类型 */
            if (v->ext_type != EXT_TYPE_NONE) {
                print_extended_object_meta_depth(v, depth);
            } else {
                ObjectEntry *entries = (ObjectEntry *)v->data.pointer;
                print_object_json_depth(entries, v->array_size, depth);
            }
            break;
        }
        default:
            if (use_colors) printf("%sunknown%s", COLOR_GRAY, COLOR_RESET);
            else printf("unknown");
    }
}

static void print_array_json_depth(Value **arr, long size, int depth) {
    int use_colors = should_use_colors();
    const char* bracket_color = use_colors ? bracket_colors[depth % NUM_BRACKET_COLORS] : "";
    
    if (use_colors) printf("%s", bracket_color);
    printf("[");
    if (use_colors) printf("%s", COLOR_RESET);
    
    for (long i = 0; i < size; i++) {
        if (i > 0) printf(", ");
        print_value_json_depth(arr[i], depth + 1);
    }
    
    if (use_colors) printf("%s", bracket_color);
    printf("]");
    if (use_colors) printf("%s", COLOR_RESET);
}

/* 递归打印数组内容为JSON格式（兼容接口）*/
static void print_array_json(Value **arr, long size) {
    print_array_json_depth(arr, size, 0);
}

/* 打印对象内容为JSON格式 */
static void print_object_json_depth(ObjectEntry *entries, long count, int depth) {
    int use_colors = should_use_colors();
    const char* bracket_color = use_colors ? bracket_colors[depth % NUM_BRACKET_COLORS] : "";
    
    if (use_colors) printf("%s", bracket_color);
    printf("{ ");
    if (use_colors) printf("%s", COLOR_RESET);
    
    for (long i = 0; i < count; i++) {
        if (i > 0) printf(", ");
        
        // 打印键（使用默认颜色）
        printf("%s: ", entries[i].key);
        
        // 打印值
        print_value_json_depth(entries[i].value, depth + 1);
    }
    
    if (use_colors) printf("%s", bracket_color);
    printf(" }");
    if (use_colors) printf("%s", COLOR_RESET);
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

/* Print a value */
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
            /* 输出JSON格式的数组 */
            Value **arr = (Value **)v->data.pointer;
            if (!arr || v->array_size == 0) {
                int use_colors = should_use_colors();
                const char* bracket_color = use_colors ? bracket_colors[0] : "";
                if (use_colors) printf("%s", bracket_color);
                printf("[]");
                if (use_colors) printf("%s", COLOR_RESET);
            } else {
                print_array_json(arr, v->array_size);
            }
            break;
        }
        case VALUE_OBJECT: {
            /* 检查是否为扩展对象类型 */
            if (v->ext_type != EXT_TYPE_NONE) {
                print_extended_object_meta(v);  /* 顶层打印使用 depth=0 */
                break;
            }
            
            /* 输出JSON格式的普通对象 */
            ObjectEntry *entries = (ObjectEntry *)v->data.pointer;
            if (!entries || v->array_size == 0) {
                int use_colors = should_use_colors();
                const char* bracket_color = use_colors ? bracket_colors[0] : "";
                if (use_colors) printf("%s", bracket_color);
                printf("{}");
                if (use_colors) printf("%s", COLOR_RESET);
            } else {
                print_object_json_depth(entries, v->array_size, 0);
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
        case VALUE_ARRAY: return "arr";
        case VALUE_OBJECT: return "obj";
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
        return box_string(result);
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

/* Array/Object index access - runtime version */
Value* value_index(Value *obj, Value *index) {
    if (!obj) {
        set_runtime_status(FLYUX_TYPE_ERROR, "Attempt to index null value");
        return box_null();
    }
    
    // For arrays with numeric index
    if (obj->type == VALUE_ARRAY && obj->data.pointer) {
        int idx = (int)unbox_number(index);
        Value **array = (Value **)obj->data.pointer;
        // Note: no bounds checking for now
        return array[idx];
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
        
        // 返回字节值（0-255）
        return box_number((double)buf->data[idx]);
    }
    
    // For objects with string index (inline implementation to avoid forward declaration)
    if (obj->type == VALUE_OBJECT && index && index->type == VALUE_STRING) {
        const char *key = (const char*)index->data.pointer;
        ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
        size_t count = obj->array_size;
        
        for (size_t i = 0; i < count; i++) {
            if (strcmp(entries[i].key, key) == 0) {
                return entries[i].value;
            }
        }
    }

    set_runtime_status(FLYUX_TYPE_ERROR, "Invalid index operation");
    return box_null();
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

