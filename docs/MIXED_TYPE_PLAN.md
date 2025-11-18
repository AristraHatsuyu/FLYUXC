# 混合类型系统实现方案

## 方案：Tagged Pointer (简化版)

### 核心思想
使用 64 位指针，低 3 位作为类型标记：
- `xxx...000` (0): 堆上的 Value 结构体指针
- `xxx...001` (1): 小整数 (右移 3 位得到值)
- `xxx...010` (2): 特殊值 (null, undefined, true, false)
- 其他：保留

### Value 结构体 (堆分配)
```c
struct Value {
    i32 type;      // VALUE_NUMBER, VALUE_STRING, VALUE_ARRAY, VALUE_OBJECT
    union {
        double number;
        i8* string;
        i8* pointer;
    } data;
}
```

### 运行时函数
```
%struct.Value = type { i32, [12 x i8] }

declare %struct.Value* @box_number(double)
declare %struct.Value* @box_string(i8*)
declare double @unbox_number(%struct.Value*)
declare i8* @unbox_string(%struct.Value*)
declare i32 @value_is_truthy(%struct.Value*)
declare void @value_print(%struct.Value*)
```

### 迁移策略
1. **阶段 1**: 所有变量改为 `%struct.Value*` 类型
2. **阶段 2**: 数字字面量用 `box_number` 包装
3. **阶段 3**: 字符串字面量用 `box_string` 包装  
4. **阶段 4**: 数组元素改为 `[n x %struct.Value*]`
5. **阶段 5**: 运算符使用 `unbox_*` 和 `value_*` 函数

### 示例转换

**FLYUX 代码:**
```
x := 42
s := "hello"
arr := [1, "two", 3]
print(x)
```

**生成的 LLVM IR:**
```llvm
%x = alloca %struct.Value*
%t0 = call %struct.Value* @box_number(double 42.0)
store %struct.Value* %t0, %struct.Value** %x

%s = alloca %struct.Value*
%t1 = call %struct.Value* @box_string(i8* @.str.0)
store %struct.Value* %t1, %struct.Value** %s

%arr = alloca [3 x %struct.Value*]
%t2 = call %struct.Value* @box_number(double 1.0)
%t3 = getelementptr [3 x %struct.Value*], [3 x %struct.Value*]* %arr, i64 0, i64 0
store %struct.Value* %t2, %struct.Value** %t3

%t4 = load %struct.Value*, %struct.Value** %x
call void @value_print(%struct.Value* %t4)
```

## 实现步骤

### Step 1: 生成运行时函数声明
在 codegen_generate 开头添加：
```c
fprintf(gen->output, "%%struct.Value = type { i32, [12 x i8] }\\n\\n");
fprintf(gen->output, "declare %%struct.Value* @box_number(double)\\n");
fprintf(gen->output, "declare %%struct.Value* @box_string(i8*)\\n");
// ...
```

### Step 2: 修改变量分配
```c
// 旧: %x = alloca double
// 新: %x = alloca %struct.Value*
fprintf(gen->code_buf, "  %%%s = alloca %%struct.Value*\\n", name);
```

### Step 3: 修改数字字面量
```c
// 旧: %t0 = sitofp i32 42 to double
// 新: %t0 = call %struct.Value* @box_number(double 42.0)
char *temp = new_temp(gen);
fprintf(gen->code_buf, "  %s = call %%struct.Value* @box_number(double %f)\\n", 
        temp, num->value);
```

### Step 4: 修改运算符
```c
// 旧: %t2 = fadd double %t0, %t1
// 新: 
//   %t2 = call double @unbox_number(%struct.Value* %t0)
//   %t3 = call double @unbox_number(%struct.Value* %t1)
//   %t4 = fadd double %t2, %t3
//   %t5 = call %struct.Value* @box_number(double %t4)
```

### Step 5: 修改 print
```c
// 旧: call i32 (i8*, ...) @printf(i8* %fmt, double %val)
// 新: call void @value_print(%struct.Value* %val)
```

## 向后兼容测试
运行所有现有测试确保不破坏现有功能。

