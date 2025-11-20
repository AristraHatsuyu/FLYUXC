# FLYUX Parser 实现计划

**创建日期**: 2025-11-17  
**状态**: 准备开始实现

---

## 📋 实现概述

Parser将分为**6个阶段**逐步实现，每个阶段都可以独立测试。

---

## 🎯 第一阶段：基础框架（1-2小时）

### 目标
建立Parser的基本结构和工具函数。

### 任务清单

- [ ] **1.1 创建 `src/core/ast.c`**
  - [ ] 实现 `ast_node_create()`
  - [ ] 实现 `ast_node_free()` （递归释放）
  - [ ] 实现 `ast_kind_name()` （返回节点类型名称）
  - [ ] 实现 `ast_print()` （打印AST，用于调试）

- [ ] **1.2 创建 `src/core/parser.c`**
  - [ ] 实现 `parser_create()`
  - [ ] 实现 `parser_free()`
  - [ ] 实现Token操作函数：
    - [ ] `current_token()`
    - [ ] `previous_token()`
    - [ ] `peek_token()`
    - [ ] `check()`
    - [ ] `match()`
    - [ ] `advance()`
    - [ ] `expect()`
    - [ ] `is_at_end()`
  - [ ] 实现错误处理函数：
    - [ ] `parser_error()`
    - [ ] `parser_error_at_current()`
    - [ ] `synchronize()`
  - [ ] 实现工具函数：
    - [ ] `parser_strdup()`
    - [ ] `is_type_keyword()`
    - [ ] `is_statement_start()`

- [ ] **1.3 修改 `CMakeLists.txt`**
  - [ ] 添加 `src/core/ast.c`
  - [ ] 添加 `src/core/parser.c`

### 测试
```c
// 简单测试：创建Parser并打印Token
Parser *p = parser_create(tokens, count, source);
parser_debug_print_tokens(p, 10);
parser_free(p);
```

---

## 🎯 第二阶段：字面量和标识符（2-3小时）

### 目标
能够解析最基础的表达式：字面量和标识符。

### 任务清单

- [ ] **2.1 实现 `parse_primary()`**
  - [ ] 数字字面量: `123`, `3.14`, `1.5e10`
  - [ ] 字符串字面量: `"hello"`
  - [ ] 布尔字面量: `true`, `false`
  - [ ] null字面量: `null`
  - [ ] undef字面量: `undef`
  - [ ] 标识符: `x`, `foo`, `🐶`
  - [ ] 括号表达式: `(expr)`

- [ ] **2.2 实现字面量创建函数（在 `ast.c`）**
  - [ ] `ast_num_literal_create()`
  - [ ] `ast_string_literal_create()`
  - [ ] `ast_bool_literal_create()`
  - [ ] `ast_null_literal_create()`
  - [ ] `ast_undef_literal_create()`
  - [ ] `ast_identifier_create()`

### 测试
```flyux
// test_literals.fx
123
3.14
"hello"
true
false
null
undef
foo
```

期望AST:
```
Program
├── ExprStmt: NumLiteral(123)
├── ExprStmt: NumLiteral(3.14)
├── ExprStmt: StringLiteral("hello")
├── ExprStmt: BoolLiteral(true)
├── ExprStmt: BoolLiteral(false)
├── ExprStmt: NullLiteral
├── ExprStmt: UndefLiteral
└── ExprStmt: Identifier("foo")
```

---

## 🎯 第三阶段：一元和二元表达式（3-4小时）

### 目标
能够解析完整的表达式树，包括所有运算符。

### 任务清单

- [ ] **3.1 实现一元表达式**
  - [ ] `parse_unary()`: `!`, `-`, `+`
  - [ ] `ast_unary_expr_create()`

- [ ] **3.2 实现二元表达式（按优先级从低到高）**
  - [ ] `parse_logical_or()`: `||`
  - [ ] `parse_logical_and()`: `&&`
  - [ ] `parse_bitwise_or()`: `|`
  - [ ] `parse_bitwise_xor()`: `^`
  - [ ] `parse_bitwise_and()`: `&`
  - [ ] `parse_equality()`: `==`, `!=`
  - [ ] `parse_relational()`: `<`, `>`, `<=`, `>=`
  - [ ] `parse_additive()`: `+`, `-`
  - [ ] `parse_multiplicative()`: `*`, `/`, `%`
  - [ ] `parse_power()`: `**`
  - [ ] `ast_binary_expr_create()`

- [ ] **3.3 实现表达式入口**
  - [ ] `parse_expr()` → 调用 `parse_logical_or()`

### 测试
```flyux
// test_expressions.fx
a + b
a * b + c
a ** 2 + b * c
!flag
-x + y
a && b || c
a & b | c ^ d
1 + 2 * 3 ** 4
(a + b) * c
```

期望AST（示例）:
```
BinaryExpr(+)
├── Identifier(a)
└── Identifier(b)

BinaryExpr(+)
├── BinaryExpr(*)
│   ├── Identifier(a)
│   └── Identifier(b)
└── Identifier(c)
```

---

## 🎯 第四阶段：后缀表达式和字面量（3-4小时）

### 目标
支持函数调用、数组/对象访问、链式调用、数组和对象字面量。

### 任务清单

- [ ] **4.1 实现后缀操作**
  - [ ] `parse_postfix()` - 循环处理所有后缀操作
  - [ ] `parse_call_expr()`: `f(a, b)`
  - [ ] `parse_index_expr()`: `arr[i]`
  - [ ] `parse_member_expr()`: `obj.prop`
  - [ ] `parse_chain_expr()`: `obj.>method.>call`
  - [ ] `parse_arg_list()`: 解析参数列表
  - [ ] 对应的AST创建函数

- [ ] **4.2 实现复合字面量**
  - [ ] `parse_array_literal()`: `[1, 2, 3]`
  - [ ] `parse_object_literal()`: `{a: 1, b: 2}`
  - [ ] 对应的AST创建函数

### 测试
```flyux
// test_postfix.fx
f(1, 2)
array[0]
object.prop
array[0].prop
f(a, b).result
[1, 2, 3]
[1, "hello", true, [nested]]
{a: 1, b: 2}
{name: "Alice", age: 30, nested: {x: 1}}

// 链式调用
arr.>len
object.>toString.>toUpperCase
🐶.>🐮🐴(2)
```

---

## 🎯 第五阶段：语句和声明（4-5小时）

### 目标
支持所有语句类型。

### 任务清单

- [ ] **5.1 实现变量和常量声明**
  - [ ] `parse_var_decl()`: 
    - [ ] `x := 123`
    - [ ] `x :[num]= 123`
    - [ ] `x :[num]`
  - [ ] `parse_const_decl()`: `X :(num)= 123`
  - [ ] `parse_type_annotation()`: 解析类型标注
  - [ ] `ast_var_decl_create()`
  - [ ] `ast_type_annotation_create()`

- [ ] **5.2 实现函数声明**
  - [ ] `parse_func_decl()`: 
    - [ ] `f := (a, b) { ... }`
    - [ ] `f :<num>= (a, b) { ... }`
  - [ ] `parse_param_list()`: 解析参数列表
  - [ ] `ast_func_decl_create()`

- [ ] **5.3 实现赋值和表达式语句**
  - [ ] `parse_assign_stmt()`: `x = 456`
  - [ ] `parse_expr_stmt()`: `print(x);`
  - [ ] `ast_assign_stmt_create()`
  - [ ] `ast_expr_stmt_create()`

- [ ] **5.4 实现代码块**
  - [ ] `parse_block()`: `{ stmt1; stmt2; }`
  - [ ] `ast_block_create()`

- [ ] **5.5 实现 `parse_statement()`**
  - [ ] 区分声明、赋值、表达式语句
  - [ ] 通过lookahead判断语句类型

### 测试
```flyux
// test_statements.fx
x := 123
y :[num]= 456
z :[str]
PI :(num)= 3.14159

f := (a, b) {
    R> a + b
}

main := () {
    result := f(5, 3)
    print(result)
}

x = 789
object.prop = "hello"
arr[0] = 42
```

---

## 🎯 第六阶段：控制流（3-4小时）

### 目标
支持if、循环、返回语句。

### 任务清单

- [ ] **6.1 实现if语句**
  - [ ] `parse_if_stmt()`:
    - [ ] 单条件: `if (x > 0) { ... }`
    - [ ] 多条件链: `if (x < 0) { ... } (x > 100) { ... }`
    - [ ] else块: `if (cond) { ... } { ... }`
  - [ ] `ast_if_stmt_create()`

- [ ] **6.2 实现循环语句**
  - [ ] `parse_loop_stmt()`:
    - [ ] 重复循环: `L> [10] { ... }`
    - [ ] for循环: `L> (i := 0; i < 10; i++) { ... }`
    - [ ] foreach循环: `L> (arr : item) { ... }`
  - [ ] `ast_loop_stmt_create()`

- [ ] **6.3 实现返回语句**
  - [ ] `parse_return_stmt()`:
    - [ ] `R> value`
    - [ ] `R>` (返回undef)
  - [ ] `ast_return_stmt_create()`

- [ ] **6.4 实现顶层解析**
  - [ ] `parse_program()`: 循环解析语句
  - [ ] `parser_parse()`: 入口函数
  - [ ] `ast_program_create()`

### 测试
```flyux
// test_control_flow.fx
if (x > 0) {
    print("positive")
} {
    print("non-positive")
}

if (x < 0) {
    print("negative")
} (x > 100) {
    print("large")
} {
    print("normal")
}

L> [10] {
    print("repeat")
}

L> (i := 0; i < 10; i++) {
    print(i)
}

L> (arr : item) {
    print(item)
}

factorial := (n) {
    if (n <= 1) {
        R> 1
    }
    R> n * factorial(n - 1)
}
```

---

## 🧪 第七阶段：集成测试和优化（2-3小时）

### 目标
在真实代码上测试Parser，修复bug，优化性能。

### 任务清单

- [ ] **7.1 测试所有testfx/文件**
  - [ ] `testfx/demo.fx`
  - [ ] `testfx/complex_test.fx`
  - [ ] `testfx/print.fx`
  - [ ] `testfx/types_test.fx`
  - [ ] 等等...

- [ ] **7.2 错误处理完善**
  - [ ] 改进错误消息
  - [ ] 添加更多错误恢复点
  - [ ] 测试恶意输入

- [ ] **7.3 内存管理**
  - [ ] 使用valgrind检查内存泄漏
  - [ ] 确保所有AST节点正确释放

- [ ] **7.4 性能优化**
  - [ ] 测量解析速度
  - [ ] 优化热点函数
  - [ ] 减少内存分配

- [ ] **7.5 文档和注释**
  - [ ] 为复杂函数添加注释
  - [ ] 更新PARSER_DESIGN.md
  - [ ] 创建PARSER_IMPLEMENTATION.md

### 测试命令
```bash
# 构建
cmake --build build

# 测试单个文件
./build/flyuxc testfx/demo.fx --parse-only --print-ast

# 测试所有文件
for f in testfx/*.fx; do
    echo "Testing $f..."
    ./build/flyuxc "$f" --parse-only --print-ast || echo "FAILED: $f"
done

# 内存检查
valgrind --leak-check=full ./build/flyuxc testfx/demo.fx --parse-only
```

---

## 📊 进度追踪

### 阶段完成情况

| 阶段 | 任务 | 状态 | 预估时间 | 实际时间 |
|------|------|------|----------|----------|
| 1 | 基础框架 | ⬜ 未开始 | 1-2小时 | - |
| 2 | 字面量和标识符 | ⬜ 未开始 | 2-3小时 | - |
| 3 | 一元和二元表达式 | ⬜ 未开始 | 3-4小时 | - |
| 4 | 后缀表达式和字面量 | ⬜ 未开始 | 3-4小时 | - |
| 5 | 语句和声明 | ⬜ 未开始 | 4-5小时 | - |
| 6 | 控制流 | ⬜ 未开始 | 3-4小时 | - |
| 7 | 集成测试和优化 | ⬜ 未开始 | 2-3小时 | - |
| **总计** | | | **18-25小时** | - |

### 功能完成情况

- [ ] AST节点定义 ✅
- [ ] Parser头文件定义 ✅
- [ ] 基础框架
  - [ ] Token操作
  - [ ] 错误处理
  - [ ] 内存管理
- [ ] 表达式解析
  - [ ] 字面量
  - [ ] 一元表达式
  - [ ] 二元表达式
  - [ ] 后缀表达式
  - [ ] 数组和对象字面量
- [ ] 语句解析
  - [ ] 变量声明
  - [ ] 函数声明
  - [ ] 赋值语句
  - [ ] if语句
  - [ ] 循环语句
  - [ ] 返回语句
- [ ] 测试和优化
  - [ ] 单元测试
  - [ ] 集成测试
  - [ ] 内存泄漏检查
  - [ ] 性能优化

---

## 🚨 已知难点

### 1. 区分语句类型

问题：`x` 开头的行可能是：
- 变量声明: `x := 123`
- 赋值: `x = 456`
- 表达式: `x + y`

解决：通过lookahead检查第二个token

### 2. 函数声明识别

问题：需要区分：
- 变量: `x := 123`
- 函数: `f := () { ... }`

解决：检查 `:=` 后是否为 `(`

### 3. 链式调用解析

问题：`.>` 的语义是将左侧作为右侧函数的第一个参数

示例：
```flyux
arr.>len.>foo(2)
// 等价于: foo(len(arr), 2)
```

解决：构建ChainExpr节点，记录链式调用序列

### 4. 类型标注的三种形式

- `:[type]` - 变量类型标注
- `:(type)` - 常量类型标注
- `:<type>` - 函数返回类型标注

解决：在 `parse_type_annotation()` 中区分

### 5. if语句的多条件

```flyux
if (cond1) { block1 } (cond2) { block2 } { else_block }
```

解决：循环收集 `(cond) { block }` 对

---

## 🔍 调试技巧

### 1. 打印Token流
```c
parser_debug_print_tokens(p, 20);
```

### 2. 打印AST
```c
ASTNode *ast = parser_parse(p);
ast_print(ast, 0);
```

### 3. 断点调试
在关键函数设置断点：
- `parse_statement()`
- `parse_expr()`
- `parse_primary()`

### 4. 添加日志
```c
#ifdef PARSER_DEBUG
    printf("Parsing expression at line %d\n", current_token(p)->loc.line);
#endif
```

---

## 📚 下一步

完成Parser后，接下来的工作：

1. **语义分析器（Semantic Analyzer）**
   - 类型检查
   - 作用域分析
   - 变量定义和使用检查

2. **中间表示（IR）**
   - 将AST转换为更利于优化和代码生成的IR

3. **代码生成器（Code Generator）**
   - 生成目标代码（解释执行或编译为字节码）

---

**文档版本**: 1.0  
**最后更新**: 2025-11-17
