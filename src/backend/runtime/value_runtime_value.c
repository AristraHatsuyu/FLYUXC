/*
 * Auto-generated fragment from value_runtime.c
 * Module: value_runtime_value.c
 */

/* ============================================================================
 * Value 结构和基础操作
 * ============================================================================
 */

/* Value structure */
typedef struct {
    int type;           /* 当前值的实际类型 */
    int declared_type;  /* 变量的声明类型（用于类型注解）*/
    int ext_type;       /* 扩展对象类型标识 (仅当type==VALUE_OBJECT时有效) */
    union {
        double number;
        char *string;
        void *pointer;
    } data;
    long array_size;    /* 数组大小（仅当type==VALUE_ARRAY时有效）*/
    size_t string_length;  /* 字符串长度（支持包含\0的字符串）*/
} Value;

/* Object key-value pair (after Value definition) */
typedef struct ObjectEntry {
    char *key;
    Value *value;
} ObjectEntry;

