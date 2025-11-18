/* Runtime support functions for FLYUX mixed-type system */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Value type tags */
#define VALUE_NUMBER 0
#define VALUE_STRING 1
#define VALUE_ARRAY 2
#define VALUE_OBJECT 3
#define VALUE_BOOL 4
#define VALUE_NULL 5

/* Value structure */
typedef struct {
    int type;           /* 当前值的实际类型 */
    int declared_type;  /* 变量的声明类型（用于类型注解）*/
    union {
        double number;
        char *string;
        void *pointer;
    } data;
    long array_size;    /* 数组大小（仅当type==VALUE_ARRAY时有效）*/
    size_t string_length;  /* 字符串长度（支持包含\0的字符串）*/
} Value;

/* Box a number into a Value */
Value* box_number(double num) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NUMBER;
    v->declared_type = VALUE_NUMBER;  /* 默认声明类型等于实际类型 */
    v->data.number = num;
    return v;
}

/* Box a string into a Value */
Value* box_string(char *str) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_STRING;
    v->declared_type = VALUE_STRING;
    v->data.string = str;  // 不复制，直接使用全局常量
    v->string_length = str ? strlen(str) : 0;  // 保存长度，支持\0字符串
    return v;
}

/* Box a string with explicit length (supports \0 in string) */
Value* box_string_with_length(char *str, size_t len) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_STRING;
    v->declared_type = VALUE_STRING;
    v->data.string = str;  // 不复制，直接使用全局常量
    v->string_length = len;  // 使用显式长度，支持包含\0的字符串
    return v;
}

/* Box a boolean into a Value */
Value* box_bool(int b) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_BOOL;
    v->declared_type = VALUE_BOOL;
    v->data.number = b ? 1.0 : 0.0;
    return v;
}

/* Box null */
Value* box_null() {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NULL;
    v->declared_type = VALUE_NULL;  /* 默认声明类型为null */
    return v;
}

/* Box null with declared type - for typed variables */
Value* box_null_typed(int decl_type) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NULL;
    v->declared_type = decl_type;  /* 使用指定的声明类型 */
    return v;
}

/* Create null preserving declared_type from existing value */
Value* box_null_preserve_type(Value *old_val) {
    if (!old_val) return box_null();
    return box_null_typed(old_val->declared_type);
}

/* Box an array */
Value* box_array(void *array_ptr, long size) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_ARRAY;
    v->declared_type = VALUE_ARRAY;
    v->data.pointer = array_ptr;
    v->array_size = size;  /* 存储数组大小 */
    return v;
}

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
            return "null";
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
            return 0;
        default:
            return 1;  // objects/arrays are truthy
    }
}

/* Helper: Convert value to JSON string representation */
static void value_to_json_string(Value *v, char *buf, size_t size) {
    if (!v) {
        snprintf(buf, size, "null");
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER:
            snprintf(buf, size, "%g", v->data.number);
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

/* 递归打印数组内容为JSON格式 */
static void print_array_json(Value **arr, long size) {
    printf("[");
    for (long i = 0; i < size; i++) {
        if (i > 0) printf(",");
        if (!arr[i]) {
            printf("null");
        } else {
            switch (arr[i]->type) {
                case VALUE_NUMBER:
                    printf("%g", arr[i]->data.number);
                    break;
                case VALUE_STRING:
                    printf("\"%s\"", arr[i]->data.string);
                    break;
                case VALUE_BOOL:
                    printf("%s", arr[i]->data.number != 0 ? "true" : "false");
                    break;
                case VALUE_NULL:
                    printf("null");
                    break;
                case VALUE_ARRAY: {
                    Value **nested = (Value **)arr[i]->data.pointer;
                    print_array_json(nested, arr[i]->array_size);
                    break;
                }
                case VALUE_OBJECT:
                    printf("{...}"); /* 对象暂时简化 */
                    break;
                default:
                    printf("null");
            }
        }
    }
    printf("]");
}

/* Print a value */
void value_print(Value *v) {
    if (!v) {
        printf("<undef>");
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER:
            printf("%g", v->data.number);
            break;
        case VALUE_STRING:
            /* 使用fwrite支持包含\0的字符串 */
            if (v->data.string && v->string_length > 0) {
                fwrite(v->data.string, 1, v->string_length, stdout);
            }
            break;
        case VALUE_BOOL:
            printf("%s", v->data.number != 0 ? "true" : "false");
            break;
        case VALUE_NULL:
            printf("<null>");
            break;
        case VALUE_ARRAY: {
            /* 输出JSON格式的数组 */
            Value **arr = (Value **)v->data.pointer;
            if (!arr || v->array_size == 0) {
                printf("[]");
            } else {
                print_array_json(arr, v->array_size);
            }
            break;
        }
        case VALUE_OBJECT:
            printf("{...}");
            break;
        default:
            printf("<unknown>");
    }
}

/* Print value with newline */
void value_println(Value *v) {
    value_print(v);
    printf("\n");
}

/* Get type of value as string */
char* value_typeof(Value *v) {
    if (!v) return "undef";
    
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
    double divisor = unbox_number(b);
    if (divisor == 0.0) return box_number(INFINITY);
    return box_number(unbox_number(a) / divisor);
}

Value* value_power(Value *a, Value *b) {
    return box_number(pow(unbox_number(a), unbox_number(b)));
}

/* Value comparison */
Value* value_equals(Value *a, Value *b) {
    if (!a || !b) return box_bool(a == b);
    if (a->type != b->type) {
        // Type coercion comparison
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
    if (!obj) return box_null();
    
    // For arrays stored as pointers
    if (obj->type == VALUE_ARRAY && obj->data.pointer) {
        int idx = (int)unbox_number(index);
        Value **array = (Value **)obj->data.pointer;
        // Note: no bounds checking for now
        return array[idx];
    }
    
    return box_null();
}

/* Free a value */
void value_free(Value *v) {
    if (v) {
        // Note: we don't free strings as they might be global constants
        free(v);
    }
}
