# 数组索引访问实现技术文档

## 概述

本文档描述了 FLYUX 编译器中数组索引读取功能的完整实现方案。该实现允许程序通过 `arr[index]` 语法访问数组元素，支持常量索引、变量索引和表达式索引。

## 问题背景

### 初始状态
在实现之前，数组功能存在以下问题：
1. 数组字面量能够正确分配和初始化
2. 生成的 LLVM IR 显示数组内存布局正确
3. 但索引访问 `arr[i]` 始终返回占位符 0.0

### 根本原因
FLYUX 的类型系统只支持 `double` 类型。当数组字面量被计算时，codegen_expr() 必须返回一个 `double` 值。原实现返回第一个元素的值，但数组的 **指针信息丢失了**。

后续的索引访问无法获取数组的：
- 数组基地址指针
- 数组大小（元素数量）

## 解决方案

### 核心思路
使用 **数组元数据表** 记录变量名到数组信息的映射：

```
变量名 → (数组指针, 元素数量)
```

当变量声明时，如果初始化表达式是数组字面量，就注册元数据。  
当索引访问时，查找元数据获取数组信息。

### 架构设计

#### 1. 数据结构

```c
/* 数组元数据 - 记录数组的实际信息 */
typedef struct ArrayMetadata {
    char *var_name;         /* 变量名（如 "arr"） */
    char *array_ptr;        /* LLVM 中的数组指针（如 "%t18"） */
    size_t elem_count;      /* 元素数量 */
    struct ArrayMetadata *next;  /* 链表的下一个节点 */
} ArrayMetadata;

/* 代码生成器 - 添加数组追踪 */
typedef struct CodeGen {
    // ... 其他字段 ...
    ArrayMetadata *arrays;         /* 数组元数据链表 */
    const char *current_var_name;  /* 当前正在赋值的变量名 */
} CodeGen;
```

#### 2. 辅助函数

```c
/* 注册数组元数据 */
static void register_array(CodeGen *gen, const char *var_name, 
                          const char *array_ptr, size_t elem_count) {
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata));
    meta->var_name = strdup(var_name);
    meta->array_ptr = strdup(array_ptr);
    meta->elem_count = elem_count;
    meta->next = gen->arrays;
    gen->arrays = meta;  // 头插法
}

/* 查找数组元数据 */
static ArrayMetadata *find_array(CodeGen *gen, const char *var_name) {
    for (ArrayMetadata *meta = gen->arrays; meta != NULL; meta = meta->next) {
        if (strcmp(meta->var_name, var_name) == 0) {
            return meta;
        }
    }
    return NULL;
}
```

### 实现细节

#### 步骤 1: 数组字面量注册

在 `AST_ARRAY_LITERAL` 的 codegen 中：

```c
case AST_ARRAY_LITERAL: {
    ASTArrayLiteral *arr_lit = (ASTArrayLiteral *)node->data;
    size_t count = arr_lit->elem_count;
    
    // 1. 分配数组
    char *array_ptr = new_temp(gen);
    fprintf(gen->code_buf, "  %s = alloca [%zu x double]\n", 
            array_ptr, count);
    
    // 2. 初始化每个元素
    for (size_t i = 0; i < count; i++) {
        char *elem_val = codegen_expr(gen, arr_lit->elements[i]);
        char *elem_ptr = new_temp(gen);
        fprintf(gen->code_buf, 
            "  %s = getelementptr inbounds [%zu x double], "
            "[%zu x double]* %s, i64 0, i64 %zu\n",
            elem_ptr, count, count, array_ptr, i);
        fprintf(gen->code_buf, "  store double %s, double* %s\n", 
                elem_val, elem_ptr);
    }
    
    // 3. 如果知道变量名，就注册元数据
    if (gen->current_var_name) {
        register_array(gen, gen->current_var_name, array_ptr, count);
    }
    
    // 4. 返回第一个元素的值（类型系统限制）
    char *result = new_temp(gen);
    char *first_ptr = new_temp(gen);
    fprintf(gen->code_buf, 
        "  %s = getelementptr inbounds [%zu x double], "
        "[%zu x double]* %s, i64 0, i64 0\n",
        first_ptr, count, count, array_ptr);
    fprintf(gen->code_buf, "  %s = load double, double* %s\n", 
            result, first_ptr);
    return result;
}
```

#### 步骤 2: 变量声明时设置上下文

在 `AST_VAR_DECL` 中：

```c
case AST_VAR_DECL: {
    ASTVarDecl *decl = (ASTVarDecl *)node->data;
    
    fprintf(gen->code_buf, "  %%%s = alloca double\n", decl->name);
    
    if (decl->init_expr) {
        // 设置当前变量名，让数组字面量能注册自己
        gen->current_var_name = decl->name;
        char *init_val = codegen_expr(gen, decl->init_expr);
        gen->current_var_name = NULL;  // 重置
        
        if (init_val) {
            fprintf(gen->code_buf, "  store double %s, double* %%%s\n",
                    init_val, decl->name);
        }
    }
}
```

#### 步骤 3: 索引访问实现

在 `AST_INDEX_EXPR` 中：

```c
case AST_INDEX_EXPR: {
    ASTIndexExpr *idx_expr = (ASTIndexExpr *)node->data;
    
    // 1. 检查对象是否为标识符
    if (idx_expr->object->kind != AST_IDENTIFIER) {
        // 不支持的情况，返回占位符
        char *temp = new_temp(gen);
        fprintf(gen->code_buf, 
            "  %s = fadd double 0.0, 0.0  ; not identifier\n", temp);
        return temp;
    }
    
    // 2. 获取数组变量名
    ASTIdentifier *arr_ident = (ASTIdentifier *)idx_expr->object->data;
    const char *arr_name = arr_ident->name;
    
    // 3. 查找数组元数据
    ArrayMetadata *meta = find_array(gen, arr_name);
    if (!meta) {
        // 变量不是已知的数组
        char *temp = new_temp(gen);
        fprintf(gen->code_buf, 
            "  %s = fadd double 0.0, 0.0  ; array not found\n", temp);
        return temp;
    }
    
    // 4. 计算索引值
    char *index_val = codegen_expr(gen, idx_expr->index);
    
    // 5. 将 double 索引转换为 i64
    char *index_i64 = new_temp(gen);
    fprintf(gen->code_buf, "  %s = fptosi double %s to i64\n", 
            index_i64, index_val);
    
    // 6. 使用 getelementptr 获取元素地址
    char *elem_ptr = new_temp(gen);
    fprintf(gen->code_buf, 
        "  %s = getelementptr inbounds [%zu x double], "
        "[%zu x double]* %s, i64 0, i64 %s\n",
        elem_ptr, meta->elem_count, meta->elem_count, 
        meta->array_ptr, index_i64);
    
    // 7. 加载元素值
    char *result = new_temp(gen);
    fprintf(gen->code_buf, "  %s = load double, double* %s\n", 
            result, elem_ptr);
    
    free(index_val);
    return result;
}
```

## 生成的 LLVM IR

### 示例代码
```flyux
numbers := [10, 20, 30];
value := numbers[1];
print(value);
```

### 生成的 IR
```llvm
; 1. 分配数组
%t1 = alloca [3 x double]  ; array with 3 elements

; 2. 初始化 numbers[0] = 10
%t2 = sitofp i32 10 to double
%t3 = getelementptr inbounds [3 x double], [3 x double]* %t1, i64 0, i64 0
store double %t2, double* %t3

; 3. 初始化 numbers[1] = 20
%t4 = sitofp i32 20 to double
%t5 = getelementptr inbounds [3 x double], [3 x double]* %t1, i64 0, i64 1
store double %t4, double* %t5

; 4. 初始化 numbers[2] = 30
%t6 = sitofp i32 30 to double
%t7 = getelementptr inbounds [3 x double], [3 x double]* %t1, i64 0, i64 2
store double %t6, double* %t7

; 5. 元数据注册：numbers → (%t1, 3)

; 6. 访问 numbers[1]
%t10 = sitofp i32 1 to double       ; 索引值
%t11 = fptosi double %t10 to i64    ; 转换为 i64
%t12 = getelementptr inbounds [3 x double], [3 x double]* %t1, i64 0, i64 %t11
%t13 = load double, double* %t12    ; 结果: 20.0

; 7. 打印
call i32 (i8*, ...) @printf(i8* @.str, double %t13)
```

## 支持的功能

### 1. 常量索引
```flyux
arr := [10, 20, 30];
print(arr[0]);  // 10
print(arr[2]);  // 30
```

### 2. 变量索引
```flyux
arr := [10, 20, 30];
idx := 1;
value := arr[idx];
print(value);  // 20
```

### 3. 表达式索引
```flyux
arr := [10, 20, 30];
idx := 0;
print(arr[idx + 1]);  // 20
```

### 4. 循环中访问
```flyux
arr := [10, 20, 30];
L>(i := 0; i < 3; i++) {
    print(arr[i]);  // 10, 20, 30
};
```

## 限制和已知问题

### 当前限制
1. **数组赋值不支持**: `arr[i] = value` 会导致 normalize 阶段无限循环
2. **类型系统单一**: 只支持 `double` 类型的数组
3. **静态大小**: 数组大小在编译时确定，不支持动态调整
4. **作用域限制**: 数组元数据表是全局的，不区分作用域

### 边界检查
当前实现 **不进行边界检查**。访问越界索引会导致：
- LLVM IR 正常生成
- 运行时行为未定义（可能崩溃或返回垃圾值）

### 性能考虑
- 元数据查找：O(n) 线性查找（可优化为哈希表）
- 索引访问：需要 double→i64 转换（轻微开销）
- 内存：每个数组占用额外 24-32 字节元数据

## 测试结果

### array_test.fx
```flyux
main := () {
    numbers := [10, 20, 30, 40, 50];
    
    print(numbers[0]);  // ✓ 10.0
    print(numbers[2]);  // ✓ 30.0
    print(numbers[4]);  // ✓ 50.0
    
    L>(idx := 0; idx < 3; idx++) {
        print(numbers[idx]);  // ✓ 10, 20, 30
    };
    
    position := 1;
    value := numbers[position];
    print(value);  // ✓ 20.0
    
    print(numbers[position + 1]);  // ✓ 30.0
};
```

**输出:**
```
10.000000
30.000000
50.000000
10.000000
20.000000
30.000000
20.000000
30.000000
```

✅ **所有测试通过！**

## 未来改进方向

### 短期改进
1. **数组赋值支持**: 修复 normalize 阶段的 bug
2. **边界检查**: 添加运行时边界检查
3. **作用域管理**: 数组元数据应该区分作用域

### 中期改进
4. **优化查找**: 使用哈希表代替线性链表
6. **多维数组**: 支持 `arr[i][j]` 语法

### 长期改进
7. **类型系统**: 支持不同类型的数组
8. **动态数组**: 支持 `push`, `pop` 等操作
9. **指针类型**: 让数组变量真正存储指针

## 实现的文件

### 修改的文件
1. `include/flyuxc/backend/codegen.h`: 添加 ArrayMetadata 和相关字段
2. `src/backend/codegen/codegen.c`:
   - 添加 `register_array()` 和 `find_array()`
   - 修改 `AST_ARRAY_LITERAL` 注册数组
   - 修改 `AST_VAR_DECL` 设置上下文
   - 实现 `AST_INDEX_EXPR` 真实访问

### 新增的测试
1. `testfx/array_test.fx`: 全面的数组功能测试

## 总结

本实现通过 **元数据表** 解决了类型系统限制下的数组访问问题。核心思想是：

1. 在数组分配时记录指针和大小
2. 在索引访问时查找元数据
3. 使用 LLVM `getelementptr` 和 `load` 指令访问元素

该方案简洁、高效，并且为未来的改进留下了扩展空间。
