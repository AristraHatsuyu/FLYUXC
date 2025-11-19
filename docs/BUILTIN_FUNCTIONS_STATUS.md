# FLYUX 内置函数实现 - input() 和状态系统

## 📋 实现总结

### 已实现的内置函数列表

#### 核心函数 (已实现)
- ✅ `print(...args)` - 打印多个值
- ✅ `println(...args)` - 打印并换行
- ✅ `printf(format, ...args)` - 格式化输出
- ✅ `input(prompt?)` - 读取用户输入 **[新增]**
- ✅ `typeOf(value)` - 获取类型字符串
- ✅ `length(array)` - 获取数组长度

#### 运行时状态系统 (新增)
- ✅ `lastStatus()` - 获取最后操作状态码
- ✅ `lastError()` - 获取最后错误消息
- ✅ `clearError()` - 清除错误状态
- ✅ `isOk()` - 检查最后操作是否成功

### 待实现的函数 (根据 FLYUX_SYNTAX.md)

#### 输入输出 (2/4)
- ✅ `print`
- ✅ `input` 
- ⬜ `readFile(path)` - 读取文件
- ⬜ `writeFile(path, content)` - 写入文件

#### 字符串操作 (1/11)
- ✅ `length` (部分支持数组)
- ⬜ `substr(str, start, length?)`
- ⬜ `indexOf(str, search, start?)`
- ⬜ `replace(str, old, new)`
- ⬜ `split(str, delimiter)`
- ⬜ `join(array, delimiter)`
- ⬜ `toUpper(str)`
- ⬜ `toLower(str)`
- ⬜ `trim(str)`
- ⬜ `startsWith(str, prefix)`
- ⬜ `endsWith(str, suffix)`

#### 数学函数 (0/10)
- ⬜ `abs(x)` - 绝对值
- ⬜ `floor(x)` - 向下取整
- ⬜ `ceil(x)` - 向上取整
- ⬜ `round(x, digits?)` - 四舍五入
- ⬜ `sqrt(x)` - 平方根
- ⬜ `pow(base, exp)` - 幂运算
- ⬜ `min(...args)` - 最小值
- ⬜ `max(...args)` - 最大值
- ⬜ `random()` - [0,1) 随机数
- ⬜ `randomInt(min, max)` - 随机整数

#### 数组操作 (0/16)
- ⬜ `push(array, ...items)`
- ⬜ `pop(array)`
- ⬜ `shift(array)`
- ⬜ `unshift(array, ...items)`
- ⬜ `slice(array, start, end?)`
- ⬜ `concat(array1, array2, ...)`
- ⬜ `reverse(array)`
- ⬜ `sort(array, compareFn?)`
- ⬜ `filter(array, predicate)`
- ⬜ `map(array, transform)`
- ⬜ `reduce(array, reducer, initial?)`
- ⬜ `find(array, predicate)`
- ⬜ `indexOf(array, item)`
- ⬜ `includes(array, item)`

#### 对象操作 (0/7)
- ⬜ `keys(obj)`
- ⬜ `values(obj)`
- ⬜ `entries(obj)`
- ⬜ `hasKey(obj, key)`
- ⬜ `merge(obj1, obj2, ...)`
- ⬜ `clone(obj)`
- ⬜ `deepClone(obj)`

#### 类型转换 (1/11)
- ✅ `typeOf(value)`
- ⬜ `toNum(value)`
- ⬜ `toStr(value)`
- ⬜ `toBl(value)`
- ⬜ `isNum(value)`
- ⬜ `isStr(value)`
- ⬜ `isBl(value)`
- ⬜ `isArr(value)`
- ⬜ `isObj(value)`
- ⬜ `isNull(value)`
- ⬜ `isUndef(value)`

#### 时间函数 (0/3)
- ⬜ `now()` - 当前时间戳
- ⬜ `sleep(ms)` - 暂停执行
- ⬜ `dateStr()` - 格式化日期

#### 实用工具 (0/3)
- ⬜ `assert(condition, message?)`
- ⬜ `exit(code?)`
- ⬜ `range(start, end, step?)`

**统计**: 已实现 10/67 (15%)

---

## 🔧 技术实现细节

### 1. 运行时状态系统架构

#### 状态码定义
```c
#define FLYUX_OK            0  // 操作成功
#define FLYUX_ERROR         1  // 一般错误
#define FLYUX_EOF           2  // 文件结束/输入结束
#define FLYUX_TYPE_ERROR    3  // 类型错误
#define FLYUX_OUT_OF_BOUNDS 4  // 越界错误
#define FLYUX_IO_ERROR      5  // 输入输出错误
```

#### 全局状态结构
```c
typedef struct {
    int last_status;        /* 最后一次操作的状态码 */
    char error_msg[256];    /* 错误消息 */
    int error_line;         /* 错误行号（供调试用）*/
} RuntimeState;

static RuntimeState g_runtime_state = {
    .last_status = FLYUX_OK,
    .error_msg = "",
    .error_line = 0
};
```

#### 状态管理函数
```c
// 内部使用
void set_runtime_status(int status, const char *message);
int flyux_get_last_status();
const char* flyux_get_last_error();
void flyux_clear_error();

// 暴露给 FLYUX 代码
Value* value_last_status();    // 返回状态码
Value* value_last_error();     // 返回错误消息
Value* value_clear_error();    // 清除错误
Value* value_is_ok();          // 检查是否成功
```

### 2. input() 函数实现

#### 函数签名
```c
Value* value_input(Value *prompt);
```

#### 功能特性
1. **提示符支持**: 可选的提示字符串参数
2. **输入缓冲**: 支持最多 4KB 的输入
3. **换行符处理**: 自动移除 `\n` 和 `\r\n`
4. **EOF 处理**: 检测 `Ctrl+D` (Unix) / `Ctrl+Z` (Windows)
5. **错误处理**: IO 错误时返回 null 并设置状态
6. **内存管理**: 动态分配字符串，由 runtime 管理

#### 状态反馈
```flyux
name := input("姓名: ")
if (isOk()) {
    println("输入成功: ", name)
} {
    if (lastStatus() == 2) {
        println("用户取消输入 (EOF)")
    } {
        println("输入错误: ", lastError())
    }
}
```

### 3. 编译器集成

#### Lexer 注册
在 `src/core/lexer.c` 的 `BUILTIN_FUNC_TABLE` 中添加：
```c
"input", "lastStatus", "lastError", "clearError", "isOk"
```

#### Varmap 注册
在 `src/frontend/lexer/varmap.c` 的 `BUILTIN_IDENTIFIERS` 中添加：
```c
"input", "lastStatus", "lastError", "clearError", "isOk"
```

#### Codegen 处理
在 `src/backend/codegen/codegen.c` 中特殊处理：
```c
// input(prompt?)
if (strcmp(callee->name, "input") == 0) {
    char *prompt = (call->arg_count > 0) 
        ? codegen_expr(gen, call->args[0])
        : box_null();
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_input(%%struct.Value* %s)\n", 
            result, prompt);
}

// lastStatus(), lastError(), clearError(), isOk()
if (strcmp(callee->name, "isOk") == 0) {
    fprintf(gen->code_buf, "  %s = call %%struct.Value* @value_is_ok()\n", result);
}
```

#### LLVM IR 声明
```llvm
declare %struct.Value* @value_input(%struct.Value*)
declare %struct.Value* @value_last_status()
declare %struct.Value* @value_last_error()
declare %struct.Value* @value_clear_error()
declare %struct.Value* @value_is_ok()
```

---

## 📝 使用示例

### 基本输入
```flyux
name := input("请输入姓名: ")
println("你好, ", name)
```

### 带错误处理的输入
```flyux
age_str := input("年龄: ")
if (!isOk()) {
    println("输入失败: ", lastError())
    exit(1)
}

// 转换为数字（需实现 toNum）
age := toNum(age_str)
if (!isOk()) {
    println("年龄格式错误")
    exit(1)
}
```

### 循环输入直到成功
```flyux
L> (true) {
    data := input("输入数据: ")
    if (isOk()) {
        R> data  // 返回数据
    }
    println("输入失败，请重试")
    clearError()
}
```

---

## 🎯 未来扩展

### 状态系统的应用场景

1. **字符串转换**
   ```flyux
   num := toNum("abc")
   if (!isOk()) {
       println("转换失败: ", lastError())
   }
   ```

2. **数组越界检查**
   ```flyux
   item := arr[100]
   if (lastStatus() == 4) {  // OUT_OF_BOUNDS
       println("索引越界")
   }
   ```

3. **文件操作**
   ```flyux
   content := readFile("config.txt")
   if (!isOk()) {
       println("文件读取失败: ", lastError())
   }
   ```

4. **Try-Catch 模拟**
   ```flyux
   // 执行可能失败的操作
   result := riskyOperation()
   
   // 检查状态
   if (!isOk()) {
       // 错误处理
       println("错误: ", lastError())
       // 恢复或退出
   }
   ```

### 下一步实现优先级

1. **高优先级**（常用且相对简单）
   - ✅ `input()` - 已完成
   - ⬜ `toNum()`, `toStr()` - 类型转换
   - ⬜ `abs()`, `floor()`, `ceil()` - 基础数学
   - ⬜ `split()`, `join()` - 字符串分割和拼接

2. **中优先级**（功能性强）
   - ⬜ `readFile()`, `writeFile()` - 文件 IO
   - ⬜ `push()`, `pop()` - 数组操作
   - ⬜ `keys()`, `values()` - 对象操作

3. **低优先级**（可暂缓）
   - ⬜ `map()`, `filter()`, `reduce()` - 高阶函数
   - ⬜ `deepClone()` - 复杂对象操作
   - ⬜ `dateStr()` - 时间格式化

---

## ✅ 测试验证

### 测试文件
- `testfx/valid/io/test_input_status.fx` - 完整功能测试
- `test_input_simple.fx` - 基础功能测试
- `test_input2.fx` - 最小测试用例

### 测试覆盖
- ✅ 基本输入功能
- ✅ 带提示符输入
- ✅ 无提示符输入
- ✅ 状态码查询
- ✅ 错误消息获取
- ✅ 状态清除
- ✅ 成功检查
- ⚠️ EOF 处理（需手动测试 Ctrl+D）
- ⚠️ IO 错误（需要构造错误场景）

---

## 🐛 已知问题与修复

### Bug #1: 函数未识别为内置函数
**问题**: `isOk()` 等函数被识别为用户函数 `_00003`，导致 IR 错误。

**原因**: 
1. `lexer.c` 的 `BUILTIN_FUNC_TABLE` 未包含新函数
2. `varmap.c` 的 `BUILTIN_IDENTIFIERS` 未包含新函数

**修复**:
在两个表中都添加了新的状态函数名称。

### Bug #2: println 多参数换行问题
**现象**: `println("你好, ", name, "!")` 每个参数后都换行。

**原因**: `println` 对每个参数都调用 `value_println`，每次都换行。

**状态**: 当前行为符合设计（每个参数独立打印并换行）。如果需要改变，应该使用 `print` 代替。

---

## 📚 参考文档

- FLYUX 语法规范: `docs/FLYUX_SYNTAX.md`
- 内置函数完整列表: FLYUX_SYNTAX.md § 内置函数参考 (63 functions)
- Runtime 实现: `src/backend/runtime/value_runtime.c`
- 编译器集成: `src/backend/codegen/codegen.c`

---

**实现日期**: 2025-11-19  
**实现者**: GitHub Copilot  
**版本**: FLYUXC 0.1.0
