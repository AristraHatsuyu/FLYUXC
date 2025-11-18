# FLYUXC 警告系统文档

## 概述

FLYUXC 编译器现在包含完整的警告系统，用于检测代码质量问题。与错误不同，警告不会阻止编译，但会提醒开发者潜在的代码风格或最佳实践问题。

## 警告显示格式

警告使用黄色 ANSI 颜色显示，格式与错误类似：

```
Warning at line X, column Y: 警告消息
  X | 源代码行
    | ^^^^^  (黄色高亮指示器)
```

## 已实现的警告类型

### 1. 数组字面量尾随逗号
**触发条件**: 数组的最后一个元素后有逗号

**示例**:
```javascript
arr := [1, 2, 3,]  // 警告：Trailing comma in array literal
```

**正确写法**:
```javascript
arr := [1, 2, 3]
```

### 2. 对象字面量尾随逗号
**触发条件**: 对象的最后一个属性后有逗号

**示例**:
```javascript
obj := {a: 1, b: 2,}  // 警告：Trailing comma in object literal
```

**正确写法**:
```javascript
obj := {a: 1, b: 2}
```

### 3. 函数调用尾随逗号
**触发条件**: 函数参数列表的最后一个参数后有逗号

**示例**:
```javascript
print(1, 2, 3,)  // 警告：Trailing comma in function call
```

**正确写法**:
```javascript
print(1, 2, 3)
```

## 实现细节

### 代码结构

1. **Parser 结构体** (`include/flyuxc/frontend/parser.h`)
   - 新增 `int warning_count` 字段跟踪警告数量
   - 独立于 `error_count`，警告不会设置 `had_error` 标志

2. **warning_at() 函数** (`src/frontend/parser/parser.c`, 约 206-341 行)
   - 类似 `error_at()` 但使用黄色颜色
   - 显示 "Warning" 而非 "Error"
   - 递增 `warning_count` 但不设置 `had_error`
   - 支持 UTF-8 和 Emoji 的视觉宽度计算

3. **警告检查逻辑**
   - **数组解析** (约 501-515 行)
     ```c
     } while (match(p, TK_COMMA) && !check(p, TK_R_BRACKET));
     if (previous token is comma && current is ]) 
         warning_at(..., "Trailing comma in array literal");
     ```
   
   - **对象解析** (约 582-590 行)
     ```c
     } while (match(p, TK_COMMA) && !check(p, TK_R_BRACE));
     if (previous token is comma && current is }) 
         warning_at(..., "Trailing comma in object literal");
     ```
   
   - **函数调用解析** (约 466-478 行 和 659-671 行)
     - 有两处：普通函数调用和方法调用
     ```c
     } while (match(p, TK_COMMA) && !check(p, TK_R_PAREN));
     if (previous token is comma && current is )) 
         warning_at(..., "Trailing comma in function call");
     ```

4. **主程序显示** (`src/main.c`, 约 126-145 行)
   ```c
   bool has_warnings = (parser->warning_count > 0);
   if (has_warnings) {
       fprintf(stderr, "\n⚠️  %d warning(s)\n", parser->warning_count);
   }
   // 警告不会阻止编译继续
   ```

## 警告与错误的区别

| 特性 | 错误 (Error) | 警告 (Warning) |
|------|-------------|---------------|
| 颜色 | 红色 (`\033[31m`) | 黄色 (`\033[33m`) |
| 标题 | "Error" | "Warning" |
| 计数字段 | `error_count` | `warning_count` |
| 设置标志 | `had_error = true` | 不设置 `had_error` |
| 阻止编译 | 是 | 否 |
| 图标 | ❌ (隐式) | ⚠️  |

## 编译示例

### 有警告但编译成功
```bash
$ ./build/flyuxc testfx/test_warnings.fx

FLYUXC 0.1.0
-----------------------------------
⚡ Compiling testfx/test_warnings.fx

Warning at line 4, column 16: Trailing comma in array literal
    4 | arr := [1, 2, 3,]
      |                ^
Warning at line 7, column 21: Trailing comma in object literal
    7 | myObj := {a: 1, b: 2,}
      |                     ^
Warning at line 10, column 14: Trailing comma in function call
   10 | print(1, 2, 3,)
      |              ^

⚠️  3 warning(s)
Parsing: 0.08ms

✨ Compilation successful! (85.41ms)
Binary: test_warnings
```

### 有错误且编译失败
```bash
$ ./build/flyuxc testfx/error_test.fx

Error at line 5, column 10: Expected ';' after statement
    5 | x := 10
      |          ^

Parsing failed with 1 error(s)
```

## 未来扩展

可以添加的其他警告类型：

1. **未使用的变量**
   - `x := 10` 但从未使用 `x`
   - 需要符号表跟踪

2. **变量遮蔽 (Shadowing)**
   - 内层作用域重新声明外层变量
   - 需要作用域栈跟踪

3. **隐式类型转换**
   - `x := 10 + "20"` (数字 + 字符串)
   - 需要类型检查系统

4. **死代码 (Dead Code)**
   - `R> 42` 后的代码永远不会执行
   - 需要控制流分析

5. **空语句块**
   - `if (x) {}` 空的 if 语句体
   - 简单的 AST 检查

6. **赋值与比较混淆**
   - `if (x = 10)` 应该是 `if (x == 10)`
   - 需要上下文分析

7. **多余的括号**
   - `x := ((10))` 可以简化为 `x := 10`
   - AST 优化建议

8. **未初始化的变量使用**
   - 变量声明但未赋值就使用
   - 需要数据流分析

## 技术说明

### 尾随逗号检测逻辑

关键是在 `do-while` 循环的条件中同时匹配逗号和检查结束符：

```c
do {
    // 解析元素
    parse_element();
} while (match(p, TK_COMMA) && !check(p, TK_END_TOKEN));

// 检查尾随逗号
if (p->current > 0 && 
    p->tokens[p->current - 1].kind == TK_COMMA && 
    check(p, TK_END_TOKEN)) {
    warning_at(p, previous(p), "Trailing comma...");
}
```

**为什么有效**:
1. `match(p, TK_COMMA)` 消费逗号并返回 true
2. `!check(p, TK_END_TOKEN)` 检查是否到达结束符
3. 如果看到结束符（`]`, `}`, `)`），条件为 false，退出循环
4. 循环后检查前一个 token 是否为逗号

### UTF-8 和 Emoji 支持

警告显示继承了错误系统的 UTF-8/Emoji 处理：

```c
// 计算视觉宽度
if (c < 0x80) {
    visual_column++;
    ptr++;
} else if ((c & 0xE0) == 0xC0) {
    visual_column++;      // 2 字节字符（如中文）
    ptr += 2;
} else if ((c & 0xF0) == 0xE0) {
    visual_column++;      // 3 字节字符
    ptr += 3;
} else if ((c & 0xF8) == 0xF0) {
    visual_column += 2;   // 4 字节字符（Emoji 占 2 列）
    ptr += 4;
}
```

## 测试文件

- `testfx/test_warnings.fx` - 综合警告测试
- `testfx/test_warnings_inline.fx` - 单行版本测试
- `testfx/test_func_call.fx` - 函数调用尾随逗号测试

## 版本历史

- **v0.1.0** (当前版本)
  - ✅ 实现基础警告系统框架
  - ✅ 添加尾随逗号警告（数组、对象、函数调用）
  - ✅ 黄色 ANSI 颜色显示
  - ✅ 独立的警告计数
  - ✅ 警告不阻止编译

## 相关文件

- `include/flyuxc/frontend/parser.h` - Parser 结构体定义
- `src/frontend/parser/parser.c` - 警告实现
- `src/main.c` - 警告显示逻辑
- `testfx/test_warnings*.fx` - 测试文件
