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
    int type;
    union {
        double number;
        char *string;
        void *pointer;
    } data;
} Value;

/* Box a number into a Value */
Value* box_number(double num) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NUMBER;
    v->data.number = num;
    return v;
}

/* Box a string into a Value */
Value* box_string(char *str) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_STRING;
    v->data.string = str;  // 不复制，直接使用全局常量
    return v;
}

/* Box a boolean into a Value */
Value* box_bool(int b) {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_BOOL;
    v->data.number = b ? 1.0 : 0.0;
    return v;
}

/* Box null */
Value* box_null() {
    Value *v = (Value*)malloc(sizeof(Value));
    v->type = VALUE_NULL;
    v->data.number = 0.0;
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

/* Print a value (for debugging) */
void value_print(Value *v) {
    if (!v) {
        printf("(null)\n");
        return;
    }
    
    switch (v->type) {
        case VALUE_NUMBER:
            printf("%g\n", v->data.number);
            break;
        case VALUE_STRING:
            printf("%s\n", v->data.string ? v->data.string : "(empty)");
            break;
        case VALUE_BOOL:
            printf("%s\n", v->data.number != 0.0 ? "true" : "false");
            break;
        case VALUE_NULL:
            printf("null\n");
            break;
        default:
            printf("[object]\n");
            break;
    }
}

/* Get type of value as string */
char* value_typeof(Value *v) {
    if (!v) return "undefined";
    
    switch (v->type) {
        case VALUE_NUMBER: return "number";
        case VALUE_STRING: return "string";
        case VALUE_BOOL: return "boolean";
        case VALUE_NULL: return "null";
        case VALUE_ARRAY: return "array";
        case VALUE_OBJECT: return "object";
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

/* Value comparison */
int value_equals(Value *a, Value *b) {
    if (!a || !b) return a == b;
    if (a->type != b->type) {
        // Type coercion comparison
        return unbox_number(a) == unbox_number(b);
    }
    
    switch (a->type) {
        case VALUE_NUMBER:
        case VALUE_BOOL:
            return a->data.number == b->data.number;
        case VALUE_STRING:
            if (!a->data.string || !b->data.string)
                return a->data.string == b->data.string;
            return strcmp(a->data.string, b->data.string) == 0;
        default:
            return a == b;  // reference equality
    }
}

int value_less_than(Value *a, Value *b) {
    return unbox_number(a) < unbox_number(b);
}

int value_greater_than(Value *a, Value *b) {
    return unbox_number(a) > unbox_number(b);
}

/* Free a value */
void value_free(Value *v) {
    if (v) {
        // Note: we don't free strings as they might be global constants
        free(v);
    }
}
