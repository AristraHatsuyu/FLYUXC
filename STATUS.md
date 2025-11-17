# FLYUX 编译器 - 当前状态

## ✅ 已实现的功能

### 1. 基础语法
- ✅ 变量声明和赋值：`x := 10;`
- ✅ 算术运算：`+`, `-`, `*`, `/`, `%`, `**`(幂运算)
- ✅ 比较运算：`<`, `>`, `<=`, `>=`, `==`, `!=`
- ✅ 逻辑运算：`&&`, `||`, `!`
- ✅ 位运算：`&`, `|`, `^`

### 2. 控制流
- ✅ 条件语句：`if (条件) { ... }`
- ✅ for 循环：`L>(初始化; 条件; 更新) { ... }`
  - 完整支持 init, condition, increment 三部分
  - 支持循环体中的复杂语句

### 3. 函数
- ✅ 函数定义：`函数名:<返回类型>=(参数列表) { 函数体 }`
- ✅ 函数调用：`函数名(参数...)`
- ✅ return 语句：`R> 表达式;`
- ✅ 内置 print 函数：`print(值)`

### 4. 运算符
- ✅ 前缀递增/递减：`++i`, `--i`
- ✅ 后缀递增/递减：`i++`, `i--`
  - 正确区分前缀和后缀的返回值

### 5. 数据类型（部分支持）
- ✅ 数字类型：`num`
- ✅ 字符串字面量：`"text"`
- ✅ 数组字面量：`[1, 2, 3]`（解析完成，codegen 为 placeholder）
- ✅ 对象字面量：`{key: value}`（解析完成，codegen 为 placeholder）

### 6. 编译流程
- ✅ Lexical Analysis（词法分析）
- ✅ Normalization（代码规范化）
- ✅ Variable Mapping（变量重命名）
- ✅ Syntax Analysis（语法分析）
- ✅ Semantic Analysis（语义分析）
- ✅ Code Generation（LLVM IR 生成）
- ✅ 可执行文件生成（通过 clang）

## 🚀 可运行的示例

### simple_working.fx
```flyux
add:<num>=(x, y) {
    R> x + y;
};

main := () {
    x := 10;
    y := 20;
    sum := add(x, y);
    print(sum);
    
    counter := 0;
    counter++;
    print(counter);
    
    L>(j := 0; j < 3; j++) {
        print(j);
    };
};
```

**编译和运行：**
```bash
./build/flyuxc testfx/simple_working.fx
clang simple_working.ll -o simple_working
./simple_working
```

**输出：**
```
30.000000    # add(10, 20)
1.000000     # counter++ 
0.000000     # j = 0
1.000000     # j = 1
2.000000     # j = 2
```

## ⚠️ 已知限制

### 1. 保留关键字冲突
- `obj` 是类型关键字，不能作为变量名
- 其他类型关键字：`num`, `str`, `bl`, `func`

### 2. 数组和对象（Placeholder）
- 数组和对象字面量可以解析，但 codegen 只返回 0.0
- 索引访问 `arr[i]` 和成员访问 `obj.prop` 返回 0.0
- 需要实现实际的内存分配和访问逻辑

### 3. 方法链式调用
- `.>` 语法已在 lexer 识别，但未完全实现
- `arr.>length.>func()` 需要进一步开发

### 4. 内置函数有限
- 目前只有 `print(double)` 
- 需要添加：length, type checking 等

## 📝 技术栈

- **语言：** C
- **构建系统：** CMake
- **目标：** LLVM IR
- **编译器：** Clang/LLVM
- **平台：** macOS (可移植到 Linux)

## 🔧 构建

```bash
cmake -B build
cmake --build build
```

## 📊 代码统计

- 词法分析器：~1000 行
- 语法分析器：~800 行
- 代码生成器：~600 行
- 规范化模块：~500 行
- 总计：~3000+ 行 C 代码

## 🎯 下一步改进

1. 实现真实的数组/对象内存分配
2. 添加字符串类型支持
3. 实现方法链式调用
4. 添加更多内置函数
5. 错误恢复和更好的错误信息
6. 优化生成的 LLVM IR

## ✨ 核心成就

- ✅ 完整的编译器前端（词法、语法、语义）
- ✅ 可工作的 LLVM IR 后端
- ✅ 支持用户定义函数
- ✅ 支持循环和条件控制流
- ✅ 支持递增/递减运算符
- ✅ 生成可执行的本地代码

---

**状态：** 基础功能完整，可以编译和运行简单的程序！🎉
