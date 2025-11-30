# FLYUX 内存管理系统

**版本**: 1.2  
**日期**: 2025-11-30

## 📊 概述

FLYUX 使用**引用计数 (Reference Counting)** 进行内存管理，类似于 Swift 和 Objective-C 的 ARC (Automatic Reference Counting)。

### 核心特性

| 特性 | 描述 |
|------|------|
| **引用计数** | 每个 Value 有 `refcount` 字段跟踪引用数量 |
| **自动释放** | refcount 归零时自动释放内存 |
| **静态标记** | 静态字符串不会被释放 |
| **递归释放** | 数组/对象释放时递归释放子元素 |
| **Codegen 集成** | 变量重新赋值时自动插入 release 调用 |
| **中间值管理** | 表达式求值中的临时值自动追踪和释放 |

---

## 🔧 Value 结构

```c
typedef struct Value {
    /* 类型信息 */
    int type;           // 实际类型
    int declared_type;  // 声明类型
    
    /* 内存管理 */
    int refcount;       // 引用计数 (0=未跟踪, >0=活跃)
    unsigned char flags;// 内存标志
    unsigned char ext_type;
    
    /* 数据 */
    union {
        double number;
        char *string;
        void *pointer;
    } data;
    
    long array_size;
    size_t string_length;
} Value;
```

### 内存标志位

| 标志 | 值 | 描述 |
|------|---|------|
| `VALUE_FLAG_NONE` | 0x00 | 普通动态分配，可释放 |
| `VALUE_FLAG_STATIC` | 0x01 | 静态分配，不释放（如字符串常量）|
| `VALUE_FLAG_BORROWED` | 0x02 | 借用引用，不拥有所有权 |
| `VALUE_FLAG_IMMORTAL` | 0x04 | 永生对象，永不释放 |

---

## 📌 API 参考

### 引用计数操作

```c
// 增加引用计数，返回传入的指针
Value* value_retain(Value *v);

// 减少引用计数，归零时释放
void value_release(Value *v);
```

### Box 函数

所有 box 函数创建的 Value 初始 `refcount = 1`：

```c
Value* box_number(double num);         // 数字
Value* box_string(char *str);          // 静态字符串 (不释放)
Value* box_string_owned(char *str);    // 动态字符串 (会释放)
Value* box_bool(int b);                // 布尔值
Value* box_null();                     // null
Value* box_array(void *ptr, long size);// 数组
Value* box_object(void *ptr, long n);  // 对象
```

### 关键区别

| 函数 | 字符串所有权 | 释放时行为 |
|------|-------------|-----------|
| `box_string(str)` | 不拥有 | 不释放字符串 |
| `box_string_owned(str)` | 拥有 | 释放字符串 |

---

## 🔄 内存生命周期

### 1. 创建

```c
Value *v = box_number(42);  // refcount = 1
```

### 2. 共享引用

```c
Value *copy = value_retain(v);  // refcount = 2
```

### 3. 释放引用

```c
value_release(copy);  // refcount = 1
value_release(v);     // refcount = 0, 释放内存
```

### 4. 数组/对象递归释放

```c
// 释放数组时会递归 release 每个元素
static void value_free_internal(Value *v) {
    if (v->type == VALUE_ARRAY) {
        for (int i = 0; i < v->array_size; i++) {
            value_release(elements[i]);
        }
        free(elements);
    }
    free(v);
}
```

---

## ⚠️ 使用规范

### 正确做法 ✅

```c
// 1. 函数返回新创建的值 (refcount=1)
Value* create_value() {
    return box_number(42);  // 调用者负责释放
}

// 2. 存储到数组时增加引用
arr[i] = value_retain(elem);

// 3. 动态字符串使用 box_string_owned
char *str = malloc(100);
sprintf(str, "Hello %d", n);
return box_string_owned(str);  // 释放时会 free(str)
```

### 错误做法 ❌

```c
// 1. 对静态字符串使用 box_string_owned
return box_string_owned("static");  // 崩溃: 尝试 free 常量

// 2. 忘记增加引用计数
arr[i] = elem;  // 危险: 如果 elem 被释放，arr[i] 成为悬空指针

// 3. 重复释放
value_release(v);
value_release(v);  // 双重释放!
```

---

## 🛡️ 安全机制

### 1. 静态对象保护

```c
void value_release(Value *v) {
    if (v->flags & VALUE_FLAG_STATIC) return;  // 不释放
    // ...
}
```

### 2. 空指针安全

```c
Value* value_retain(Value *v) {
    if (!v) return NULL;  // 安全处理 NULL
    // ...
}
```

### 3. 已释放检测

```c
void value_release(Value *v) {
    if (v->refcount <= 0) return;  // 防止重复释放
    // ...
}
```

---

## 🔗 Codegen 自动内存管理

### 变量赋值语句的处理

当变量被重新赋值时，codegen 自动插入 `value_release` 调用释放旧值：

```llvm
; FLYUX 代码: x = newValue
; 
; 生成的 LLVM IR（变量已存在的情况）:
%new_val = call %struct.Value* @some_expr(...)   ; 1. 先计算新值
%old_val = load %struct.Value*, %struct.Value** %x ; 2. 加载旧值
call void @value_release(%struct.Value* %old_val) ; 3. 释放旧值
store %struct.Value* %new_val, %struct.Value** %x ; 4. 存储新值
```

### 变量初始化

新变量首次分配时初始化为 null，防止释放未初始化的垃圾指针：

```llvm
; FLYUX 代码: x := 42
;
; 生成的 LLVM IR:
%x = alloca %struct.Value*
store %struct.Value* null, %struct.Value** %x    ; 初始化为 null
%t1 = call %struct.Value* @box_number(double 42)
store %struct.Value* %t1, %struct.Value** %x
```

### 自引用安全

对于 `x = x + 1` 这种自引用表达式，先计算新值再释放旧值：

```llvm
; FLYUX 代码: x = x + 1
;
; 正确顺序（当前实现）:
%x_val = load %struct.Value*, %struct.Value** %x
%one = call %struct.Value* @box_number(double 1)
%new_val = call %struct.Value* @value_add(%x_val, %one)  ; 使用 x 计算
%old_val = load %struct.Value*, %struct.Value** %x
call void @value_release(%struct.Value* %old_val)        ; 然后释放
store %struct.Value* %new_val, %struct.Value** %x
```

---

## 🔄 中间值管理 (v1.2 新增)

### 问题

在复杂表达式如 `a + b + c + d` 中，会产生多个中间 Value*：
- `a + b` 产生临时 Value* t1
- `t1 + c` 产生临时 Value* t2
- `t2 + d` 产生最终结果 t3

如果不释放 t1 和 t2，会导致内存泄漏。

### 解决方案

Codegen 使用 **TempValueStack** 追踪表达式求值期间创建的所有中间值：

```c
// 中间值栈结构
typedef struct TempValueEntry {
    char *temp_name;                // 临时变量名（LLVM %tN 格式）
    struct TempValueEntry *next;
} TempValueEntry;

typedef struct TempValueStack {
    TempValueEntry *entries;        // 临时值链表头
    int count;                      // 临时值数量
} TempValueStack;
```

### 工作流程

1. **注册**: 每次 `box_*` 或 `value_*` 调用创建新 Value* 后，调用 `temp_value_register(gen, temp_name)`
2. **清理**: 语句执行完成后，调用 `temp_value_release_except(gen, keep_name)` 释放中间值，保留最终结果

### 生成的代码

```llvm
; FLYUX 代码: 1 + 2 + 3
;
; 生成的 LLVM IR:
%t0 = call %struct.Value* @box_number(double 1)   ; 注册 t0
%t1 = call %struct.Value* @box_number(double 2)   ; 注册 t1  
%t2 = call %struct.Value* @value_add(%t0, %t1)    ; 注册 t2 (中间结果)
%t3 = call %struct.Value* @box_number(double 3)   ; 注册 t3
%t4 = call %struct.Value* @value_add(%t2, %t3)    ; 注册 t4 (最终结果)

; --- Temp values cleanup start ---
call void @value_release(%struct.Value* %t3)      ; 释放 3
call void @value_release(%struct.Value* %t2)      ; 释放 1+2 的中间结果
call void @value_release(%struct.Value* %t1)      ; 释放 2
call void @value_release(%struct.Value* %t0)      ; 释放 1
; --- Temp values cleanup end ---
call void @value_release(%struct.Value* %t4)      ; 释放最终结果 (表达式语句)
```

### 处理的语句类型

| 语句类型 | 处理方式 |
|----------|----------|
| `AST_EXPR_STMT` | 释放所有中间值，然后释放最终结果 |
| `AST_VAR_DECL` | 释放中间值，保留赋给变量的值 |
| `AST_ASSIGN_STMT` | 释放中间值，保留赋给变量的值 |
| `AST_RETURN_STMT` | 释放中间值，保留返回值 |

### 特殊情况：变量加载

从变量加载的值（`load %struct.Value*`）不会注册到中间值栈，因为：
- 它不是新创建的值
- 变量仍然持有该值的引用
- 释放它会导致变量引用悬空

---

## 📈 未来改进

### Phase 2: 循环引用检测

```c
// 计划中：标记清除算法处理循环引用
void gc_collect();
```

### Phase 3: 分代 GC

```c
// 计划中：分代收集提升性能
typedef enum {
    GC_GEN_YOUNG,
    GC_GEN_OLD
} GCGeneration;
```

---

## 🧪 调试支持

启用引用计数调试：

```c
#define DEBUG_REFCOUNT

// 编译时启用后，每次 retain/release 会打印日志
// [RC] retain: type=1 refcount=2 flags=0x00
// [RC] release: type=1 refcount=1 flags=0x00
```

---

**文档版本**: 1.2  
**作者**: FLYUXC Team  
**最后更新**: 2025-11-30
