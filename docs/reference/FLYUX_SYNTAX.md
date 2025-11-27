# FLYUX 语言语法规范 - 完整参考

**更新日期**: 2025-11-27

## 📌 语法快速参考

### 1. 变量定义

#### 1.1 无类型推断的变量定义
```flyux
a := 123          // 推断为 num 类型
b := "Hello"      // 推断为 str 类型
c := true         // 推断为 bl 类型
d := [1, 2, 3]    // 推断为数组
e := {x: 1, y: 2} // 推断为 obj 类型
f := null         // 无效定义
g := undef        // 无效定义
```

#### 1.2 显式类型定义变量
```flyux
// 格式: 变量名:[类型]=初始值
c :[num]= 123.456           // 显式 num 类型的变量
d :[str]= "Hello"           // 显式 str 类型的变量
e :[bl]= true               // 显式 bl 类型的变量
f :[obj]= {aaa: "bbb", ccc: ["ddd", "eee"]}  // 显式 obj 类型
arr :[num]= [1, 2, 3]       // num 类型数组
```

#### 支持的类型
- `num` - 数字（包括整数和浮点数）
- `str` - 字符串
- `bl` - 布尔值（true/false）
- `obj` - 对象 ({key: value, ...})
- `func` - 函数 ((params) => {...})

> **注意**: `obj` 类型包含普通对象和扩展对象类型(如Buffer、FileHandle等)。扩展类型详见 [扩展对象类型](#-扩展对象类型) 章节。

### 2. 常量定义

#### 格式：`变量名 :(类型)= 值`
```flyux
// 常量必须显式指定类型且必须初始化
PI :(num)= 3.14159
GREETING :(str)= "Hello, World!"
IS_ENABLED :(bl)= true
CONFIG :(obj)= {host: "localhost", port: 8080}
```

**常量特性**：
- ✓ 必须显式指定类型
- ✓ 必须在定义时初始化
- ✗ 定义后不可重新赋值
- ✗ 不可重新定义

### 3. 函数定义

#### 3.1 无类型标注的函数定义
```flyux
// 格式: 函数名 := (参数...) { 函数体 }
add := (a, b) {
    R> a + b
}

main := () {
    // 函数体
}
```

#### 3.2 显式返回类型标注的函数定义
```flyux
// 格式: 函数名 :<返回类型>= (参数...) { 函数体 }
🤪🫵 :<num>= (🐙, 🍄) {
    R> 🐙 + 🍄 * 🐙
}
```

**注意**：`:=` 后面跟 `(` 的表达式自动推断为函数类型

### 4. 变量赋值/修改 (使用 `=`)

#### 4.1 基本赋值
```flyux
a = 456          // 修改 a 的值
b = "World"      // 修改 b 的值
```

#### 4.2 赋值为 null
```flyux
x = null         // 任何类型变量都可赋值为 null
                 // 此后 x 的值为 null，类型保持不变
```

#### 4.3 赋值为 undef（删除变量）
```flyux
y = undef        // 相当于删除变量 y
                 // 之后访问 y 会出现未定义错误
```

### 5. 控制流语句

#### if 条件语句
```flyux
if (condition) {
    // 真分支
} {
    // 假分支（可选）
}

// 多条件链式
if (a < 10) {
    // 处理 a < 10
} (a < 20) {
    // 处理 a < 20
} {
    // 处理其他情况
}
```

#### L> 循环语句
```flyux
// 重复循环
L> [10] {
    // 执行 10 次
}

// for 循环
L> (i := 0; i < 10; i++) {
    // 初始化、条件、更新
}

// 遍历循环
L> array : item {
    // 遍历数组的每个元素
}
```

#### R> 返回语句
```flyux
R> value    // 返回值
R>          // 返回 undef (隐式)
```

#### T> 错误捕获（Try-Catch）
```flyux
// 格式: T> { 代码块 } (错误变量) { 错误处理 } { 结束代码 }
T> {
    result := dangerousOperation()!  // 使用 ! 后缀抛出错误
    print(result)
} (err) {
    // 捕获错误对象
    print("Error:", err.message)
    print("Code:", err.code)
}

// 示例：文件读取错误处理
T> {
    content := readFile("/nonexistent/file.txt")!
    print(content)
} (error) {
    println("文件读取失败:", error.message)
}

// 示例：链式调用抛错处理
T> {
    text := "abc123"
    number := text.>toNum!.>println
} (error) {
    println("数字转换失败:", error.message)
}
```

**错误对象 (Error) 属性**:
- `message: str` - 错误消息
- `code: num` - 错误代码
- `type: str` - 固定为 "Error"

**注意**:
- 只有带 `!` 后缀的函数调用才会抛出可捕获的错误
- 不带 `!` 的调用失败时返回 `null` 或默认值
- `T>` 块可选错误处理分支和结束分支 `(err) { ... } { ... }`

### 6. 方法调用与属性访问

#### 方法链调用（.>）
```flyux
array.>len.>🐮🐴(2)       // 函数🐮🐴具有两个参数，.>左边作为第一个参数
object.>toStr.>toUpper
```

#### 属性访问（.）
```flyux
object.property
object.nestedObj.deepProperty
```

#### 数组/对象索引
```flyux
array[0]
object.key
object["key"]
array[idx]
object[keystr]
```

### 7. 运算符

#### 算术运算符
```flyux
a + b      // 加法
a - b      // 减法
a * b      // 乘法
a / b      // 除法
a % b      // 取模
a ** b     // a的b次方
```

#### 比较运算符
```flyux
a < b      // 小于
a > b      // 大于
a <= b     // 小于等于
a >= b     // 大于等于
a == b     // 等于
a != b     // 不等于

// 支持链式比较
0 < x <= 100    // x 在 0 到 100 之间
```

#### 逻辑运算符
```flyux
a && b     // 逻辑与
a || b     // 逻辑或
!a         // 逻辑非
```

#### 位运算符
```flyux
a & b      // 位与
a | b      // 位或
a ^ b      // 位异或
```

### 8. 字面量

#### 数字字面量
```flyux
123        // 整数
3.14       // 浮点数
-42        // 负数
```

#### 字符串字面量
```flyux
"Hello"                 // 双引号字符串
"Nested \"quote\""      // 转义字符
'Hello "FLYUX" !'       // 嵌套字符串
```

#### 数组字面量
```flyux
[]         // 空数组
[1, 2, 3]  // 数字数组
["a", "b"] // 字符串数组
[1, "a", true, null]  // 混合类型数组
```

#### 对象字面量
```flyux
{}         // 空对象
{ a: 1, b: 2 }                    // 简单对象
{ name: "Alice", age: 30 }        // 字符串键
{ [1]: "value" }                  // 数字键
{ nested: { deep: { value: 42 } } }   // 嵌套对象
```

### 9. 关键字与保留字

#### 语言关键字
- `if` - 条件语句
- `L>` - 循环
- `R>` - 返回
- `T>` - 错误捕获 (Try-Catch)
- `:=` - 变量/常量定义（推断或显式类型）
- `=` - 赋值
- `.>` - 方法链调用
- `.` - 属性访问
- `!` - 错误抛出后缀（用于函数调用）
- `:` - 对象键分隔符

#### 保留类型（不能用作变量名）
- `num` - 数字类型
- `str` - 字符串类型
- `bl` - 布尔类型
- `obj` - 对象类型
- `func` - 函数类型

#### 保留字（不能用作变量名）
- `true` - 布尔真
- `false` - 布尔假
- `null` - null 值
- `undef` - undefined 值

### 10. 注释

#### 单行注释
```flyux
// 这是单行注释
a := 123 // 行尾注释
```

#### 多行注释
```flyux
/* 这是多行注释 */
a := /* 中间注释 */ 456
```

---

## 📊 变量定义语法对比

```
无类型推断:           a := 123
显式类型推断:         a :[num]= 123
常量定义:             a :(num)= 123
赋值/修改:            a = 456
null 赋值:            a = null
undef 赋值(删除):      a = undef
```

## 🔄 变量生命周期示例

```flyux
// 1. 定义变量（无类型，推断为 num）
x := 10

// 2. 修改变量
x = 20

// 3. 赋值为 null（值为 null，类型仍为 num）
x = null

// 4. 赋值为 undef（删除变量，再访问会报错）
x = undef

// 5. 重新定义（可以，因为已删除，可重新推断类型）
x := "Hello"
```

## 🎯 函数定义示例

```flyux
// 简单函数
greet := () {
    R> "Hello"
}

// 带参数的函数
add := (a, b) {
    R> a + b
}

// 带返回类型的函数定义
multiply :<num>= (x, y) {
    R> x * y
}

// 主函数
main := () {
    result := add(5, 3)
    print(result)
}
```

---

## ⚠️ 重要规则

1. **类型保留词不能作变量名**
   - ✗ `num := 123` - 错误
   - ✗ `str := "hello"` - 错误
   - ✓ `number := 123` - 正确

2. **变量为null的情况**
   - ✓ `x :[num]` - 无值定义，自动赋值为null
   - ✓ `x :[num]= 3.14` - 重新定义并赋值
   - ✓ `x = 10; x = null` - x 的类型仍为 num，值为null
   - ✓ `x := "hi"; x = null` - x 的类型仍为 str

3. **常量必须初始化**
   - ✗ `PI :(num)` - 错误，缺少值
   - ✓ `PI :(num)= 3.14` - 正确

4. **常量不可重新赋值**
   - ✓ `PI :(num)= 3.14` - 定义
   - ✗ `PI = 3.15` - 错误

5. **函数后跟 `(` 时自动推断为函数类型**
   - ✓ `f := () { R> 1 }` - 自动推断为 func
   - ✓ `g :<num>= () { R> 2 }` - 显式声明

7. **赋值为 undef 相当于删除**
   - ✓ `x := 10; x = undef` - 删除变量 x
   - ✗ 之后访问 x 会报错

---

## 🔧 内置函数参考

FLYUX 提供了丰富的内置函数，覆盖常见的编程需求。

### 📤 输入输出

#### print(...args)
打印任意数量的参数到标准输出。

**特殊行为**: 对于扩展对象类型(Buffer、FileHandle等),仅输出元信息,不输出完整数据,避免终端刷屏。

```flyux
print("Hello")              // Hello
print("x =", x, "y =", y)   // x = 10 y = 20
print()                     // 空行

// 扩展对象输出
buffer := readBytes("large.bin")
print(buffer)               // Buffer { size: 5242880, type: "Buffer" }
```

#### input(prompt)
从标准输入读取一行文本。返回字符串类型。
```flyux
name := input("请输入姓名: ")
age := input("请输入年龄: ")
print("你好,", name)
```

---

### 📁 文件输入输出

FLYUX提供三类文件操作函数:
- **文本文件操作**: 返回/接受字符串 (readFile/writeFile/appendFile)
- **二进制文件操作**: 返回/接受Buffer对象 (readBytes/writeBytes)
- **流式文件操作**: 返回FileHandle对象 (openFile)

#### readFile(path) -> string | null
读取整个文本文件内容为字符串。适合小到中型文本文件。

**返回值**: 成功返回字符串,失败返回null并设置lastError()

```flyux
content := readFile("config.txt")
if (content != null) {
    print("文件内容:", content)
} {
    print("读取失败:", lastError())
}
```

#### writeFile(path, content) -> bool
写入字符串到文件(覆盖模式)。如果文件已存在则覆盖,不存在则创建。

**参数**:
- `path: str` - 文件路径
- `content: str` - 要写入的字符串内容

**返回值**: 成功返回true,失败返回false

```flyux
success := writeFile("output.txt", "Hello, FLYUX!")
if (success) {
    print("写入成功")
}
```

#### appendFile(path, content) -> bool
追加字符串到文件末尾。保留原有内容,如果文件不存在则创建。

```flyux
appendFile("log.txt", "2025-11-20: 系统启动\n")
appendFile("log.txt", "2025-11-20: 处理完成\n")
```

#### readBytes(path) -> Buffer | null
读取文件为二进制Buffer对象。适合任意类型文件(图片、音频、视频、二进制数据等)。

**返回值**: 成功返回Buffer对象,失败返回null

```flyux
// 读取图片文件
buffer :[obj]= readBytes("image.png")
if (buffer != null) {
    print(buffer)               // Buffer { size: 15234, type: "Buffer" }
    print("文件大小:", buffer.size, "字节")
    
    // 检查PNG文件头
    if (buffer[0] == 0x89 && buffer[1] == 0x50) {
        print("确认为PNG格式")
    }
}
```

#### writeBytes(path, data) -> bool
写入二进制数据到文件。接受Buffer对象或数字数组。

**参数**:
- `path: str` - 文件路径
- `data: Buffer | array<num>` - Buffer对象或0-255的数字数组

```flyux
// 写入数字数组
bytes := [0x89, 0x50, 0x4E, 0x47]  // PNG头部
writeBytes("test.png", bytes)

// 写入Buffer对象
buffer := readBytes("source.bin")
writeBytes("backup.bin", buffer)
```

#### openFile(path, mode) -> FileHandle | null
打开文件并返回FileHandle对象,支持流式读写。适合大文件或需要逐行/逐块处理的场景。

**参数**:
- `path: str` - 文件路径
- `mode: str` - 打开模式:
  - `"r"` - 只读模式(文本)
  - `"w"` - 写入模式(文本,覆盖)
  - `"a"` - 追加模式(文本)
  - `"rb"` - 只读模式(二进制)
  - `"wb"` - 写入模式(二进制,覆盖)

**返回值**: 成功返回FileHandle对象,失败返回null

```flyux
// 流式读取大文件
file :[obj]= openFile("large.log", "r")
if (file != null) {
    L> [10000] {  // 最多读10000行
        line := file.readLine()
        if (line == null) { break }
        print(line)
    }
    file.close()
}
```

#### fileExists(path) -> bool
检查文件是否存在。

```flyux
if (fileExists("config.json")) {
    config := readFile("config.json")
} {
    print("配置文件不存在")
}
```

#### deleteFile(path) -> bool
删除文件。成功返回true,失败返回false。

```flyux
if (deleteFile("temp.txt")) {
    print("删除成功")
} {
    print("删除失败:", lastError())
}
```

#### getFileSize(path) -> num
获取文件大小(字节数)。文件不存在返回-1。

```flyux
size := getFileSize("data.txt")
if (size > 0) {
    print("文件大小:", size, "字节")
}
```

---

### 🗂️ 目录操作

#### listDir(path) -> array<string> | null
列出目录中的所有文件和子目录名。返回名称数组,不包含完整路径。

```flyux
files :[str]= listDir("./testfx")
if (files != null) {
    L> (files : filename) {
        print("文件:", filename)
    }
}
```

#### dirExists(path) -> bool
检查目录是否存在。

```flyux
if (!dirExists("output")) {
    makeDir("output")
}
```

#### makeDir(path) -> bool
创建单级目录。目录已存在返回false。

```flyux
if (makeDir("logs")) {
    print("目录创建成功")
}
```

#### removeDir(path) -> bool
删除空目录。目录非空或不存在返回false。

```flyux
removeDir("temp_dir")
```

---

### 🔤 字符串操作

#### len(str)
返回字符串长度或数组长度。
```flyux
length := len("Hello")      // 5
length := len([1, 2, 3])    // 3
```

#### substr(str, start, length?)
提取子字符串。
```flyux
s := substr("Hello", 1, 3)  // "ell"
s := substr("Hello", 2)     // "llo"
```

#### indexOf(str, search, start?)
查找子字符串位置，未找到返回-1。
```flyux
pos := indexOf("Hello", "l")      // 2
pos := indexOf("Hello", "l", 3)   // 3
pos := indexOf("Hello", "x")      // -1
```

#### replace(str, old, new)
替换字符串中的内容。
```flyux
s := replace("Hello World", "World", "FLYUX")  // "Hello FLYUX"
```

#### split(str, delimiter)
分割字符串为数组。
```flyux
arr := split("a,b,c", ",")  // ["a", "b", "c"]
```

#### join(array, delimiter)
连接数组元素为字符串。
```flyux
s := join([1, 2, 3], ",")   // "1,2,3"
```

#### toUpper(str)
转换为大写。
```flyux
s := toUpper("hello")       // "HELLO"
```

#### toLower(str)
转换为小写。
```flyux
s := toLower("HELLO")       // "hello"
```

#### trim(str)
移除首尾空白字符。
```flyux
s := trim("  hello  ")      // "hello"
```

#### startsWith(str, prefix)
检查字符串是否以指定前缀开始。
```flyux
result := startsWith("Hello", "He")  // true
```

#### endsWith(str, suffix)
检查字符串是否以指定后缀结束。
```flyux
result := endsWith("Hello", "lo")    // true
```

---

### 🔢 数学函数

#### abs(x)
返回绝对值。
```flyux
val := abs(-5)              // 5
```

#### floor(x)
向下取整。
```flyux
val := floor(3.7)           // 3
```

#### ceil(x)
向上取整。
```flyux
val := ceil(3.2)            // 4
```

#### round(x, digits?)
四舍五入，可指定小数位数。
```flyux
val := round(3.14159)       // 3
val := round(3.14159, 2)    // 3.14
```

#### sqrt(x)
平方根。
```flyux
val := sqrt(16)             // 4
```

#### pow(base, exp)
幂运算（与 ** 运算符等价）。
```flyux
val := pow(2, 3)            // 8
```

#### min(...args)
返回最小值。
```flyux
val := min(1, 5, 3, 2)      // 1
```

#### max(...args)
返回最大值。
```flyux
val := max(1, 5, 3, 2)      // 5
```

#### random()
返回 [0, 1) 范围的随机数。
```flyux
r := random()               // 0.8472...
```

#### randomInt(min, max)
返回 [min, max] 范围的随机整数。
```flyux
r := randomInt(1, 10)       // 7
```

---

### 📋 数组操作

#### push(array, ...items)
在数组末尾添加元素。
```flyux
arr := [1, 2, 3]
push(arr, 4, 5)             // arr = [1, 2, 3, 4, 5]
```

#### pop(array)
移除并返回数组最后一个元素。
```flyux
arr := [1, 2, 3]
last := pop(arr)            // last = 3, arr = [1, 2]
```

#### shift(array)
移除并返回数组第一个元素。
```flyux
arr := [1, 2, 3]
first := shift(arr)         // first = 1, arr = [2, 3]
```

#### unshift(array, ...items)
在数组开头添加元素。
```flyux
arr := [1, 2, 3]
unshift(arr, 0)             // arr = [0, 1, 2, 3]
```

#### slice(array, start, end?)
提取数组片段。
```flyux
arr := [1, 2, 3, 4, 5]
sub := slice(arr, 1, 3)     // [2, 3]
```

#### concat(array1, array2, ...)
连接多个数组。
```flyux
arr := concat([1, 2], [3, 4], [5])  // [1, 2, 3, 4, 5]
```

#### reverse(array)
反转数组（原地修改）。
```flyux
arr := [1, 2, 3]
reverse(arr)                // arr = [3, 2, 1]
```

#### sort(array, compareFn?)
排序数组（原地修改）。
```flyux
arr := [3, 1, 2]
sort(arr)                   // arr = [1, 2, 3]

// 自定义排序
sort(arr, (a, b) { R> b - a })  // 降序
```

#### filter(array, predicate)
过滤数组元素。
```flyux
arr := [1, 2, 3, 4, 5]
even := filter(arr, (x) { R> x % 2 == 0 })  // [2, 4]
```

#### map(array, transform)
映射数组元素。
```flyux
arr := [1, 2, 3]
doubled := map(arr, (x) { R> x * 2 })  // [2, 4, 6]
```

#### reduce(array, reducer, initial?)
归约数组。
```flyux
arr := [1, 2, 3, 4]
sum := reduce(arr, (acc, x) { R> acc + x }, 0)  // 10
```

#### find(array, predicate)
查找第一个满足条件的元素。
```flyux
arr := [1, 2, 3, 4, 5]
found := find(arr, (x) { R> x > 3 })  // 4
```

#### indexOf(array, item)
查找元素索引，未找到返回-1。
```flyux
arr := [10, 20, 30]
idx := indexOf(arr, 20)     // 1
```

#### includes(array, item)
检查数组是否包含元素。
```flyux
arr := [1, 2, 3]
has := includes(arr, 2)     // true
```

---

### 🗂️ 对象操作

#### keys(obj)
返回对象所有键的数组。
```flyux
object := {a: 1, b: 2, c: 3}
k := keys(object)              // ["a", "b", "c"]
```

#### values(obj)
返回对象所有值的数组。
```flyux
object := {a: 1, b: 2, c: 3}
v := values(object)            // [1, 2, 3]
```

#### entries(obj)
返回对象键值对数组。
```flyux
object := {a: 1, b: 2}
e := entries(object)           // [["a", 1], ["b", 2]]
```

#### hasKey(obj, key)
检查对象是否有指定键。
```flyux
object := {name: "Alice", age: 30}
has := hasKey(object, "name")  // true
```

#### deleteKey(obj, key)
删除对象的指定键。
```flyux
object := {a: 1, b: 2, c: 3}
deleteKey(object, "b")
print(object)  // {a: 1, c: 3}
```

---

## 🎁 扩展对象类型

FLYUX的`obj`类型包含普通对象和扩展对象类型。扩展类型用于特殊用途(文件I/O、二进制数据等),具有以下特点:

1. **本质是obj**: 扩展类型是obj的子类型,可以赋值给obj变量
2. **安全输出**: `print()`时只显示元信息,不输出大量原始数据
3. **属性访问**: 支持通过`.`或`[]`访问对象属性
4. **类型识别**: `typeOf()`返回具体的扩展类型名(如"Buffer"、"FileHandle")

### Buffer - 二进制缓冲区

用于存储二进制数据(图片、音频、二进制文件等)。

**创建方式**:
```flyux
// 从文件读取
buffer := readBytes("image.png")

// 从数组创建
bytes := [0x48, 0x65, 0x6C, 0x6C, 0x6F]
buffer := Buffer(bytes)
```

**属性**:
```flyux
buffer := readBytes("data.bin")
print(buffer.size)         // 获取大小(字节)
print(buffer.type)         // "Buffer"
print(typeOf(buffer))      // "obj:Buffer"
```

**索引访问**:
```flyux
// 访问单个字节(0-255)
first_byte := buffer[0]
second_byte := buffer[1]

// 检查文件头
if (buffer[0] == 0xFF && buffer[1] == 0xD8) {
    print("JPEG格式")
}
```

**方法**:
```flyux
// slice(start, end?) - 切片缓冲区
header := buffer.slice(0, 10)

// toStr(encoding?) - 转换为字符串
text := buffer.toStr("utf8")

// toArray() - 转换为数字数组(仅小数据)
if (buffer.size < 100) {
    arr := buffer.toArray()
}
```

**print输出**:
```flyux
buffer := readBytes("large.bin")
print(buffer)
// 输出: Buffer { size: 5242880, type: "Buffer" }
// 不会输出MB级数据,避免终端刷屏
```

### FileHandle - 文件句柄

表示打开的文件,支持流式读写。

**创建方式**:
```flyux
file := openFile("data.txt", "r")
```

**属性**:
```flyux
print(file.path)          // 文件路径
print(file.mode)          // 打开模式 ("r"/"w"/"a")
print(file.position)      // 当前读写位置
print(file.isOpen)        // 是否打开
print(typeOf(file))       // "obj:FileHandle"
```

**方法**:
```flyux
// read(size?) - 读取数据
content := file.read()       // 读取全部
chunk := file.read(1024)     // 读取1KB

// readLine() - 读取一行
line := file.readLine()

// write(content) - 写入数据
file.write("Hello, FLYUX!\n")

// seek(position) - 移动读写位置
file.seek(0)  // 移到开头

// close() - 关闭文件
file.close()
```

**使用示例**:
```flyux
// 流式读取大文件
file := openFile("large.log", "r")
if (file != null) {
    count := 0
    L> [100000] {
        line := file.readLine()
        if (line == null) { break }
        count = count + 1
    }
    print("总行数:", count)
    file.close()
}
```

**print输出**:
```flyux
file := openFile("test.txt", "r")
print(file)
// 输出: FileHandle { path: "test.txt", mode: "r", position: 0, isOpen: true }
```

### Error - 错误对象

表示错误信息,由文件I/O等操作失败时自动创建。

**print输出**:
```flyux
print(err)
// 输出: Error { message: "文件未找到", code: 1001, errorType: "IOError" }
```

---

## 💡 扩展对象使用模式

### 模式1: 文本文件处理
```flyux
// 小文件 - 直接读取
content :[str]= readFile("config.txt")
if (content != null) {
    lines := split(content, "\n")
    print("行数:", len(lines))
}

// 大文件 - 流式处理
file :[obj]= openFile("large.log", "r")
if (file != null) {
    L> [10000] {
        line := file.readLine()
        if (line == null) { break }
        processLine(line)
    }
    file.close()
}
```

### 模式2: 二进制文件处理
```flyux
// 读取图片
buffer :[obj]= readBytes("photo.jpg")
if (buffer != null) {
    print("图片大小:", buffer.size)
    
    // 检查JPEG格式
    if (buffer[0] == 0xFF && buffer[1] == 0xD8) {
        print("JPEG格式确认")
    }
    
    // 处理数据...
    processed := processImage(buffer)
    writeBytes("output.jpg", processed)
}
```

### 模式3: 批量文件处理
```flyux
files := listDir("./data")
if (files != null) {
    L> (files : filename) {
        if (endsWith(filename, ".txt")) {
            path := "./data/" + filename
            content := readFile(path)
            if (content != null) {
                processFile(filename, content)
            }
        }
    }
}
```

---

### 🗂️ 对象操作

#### keys(obj)
返回对象所有键的数组。
检查对象是否有指定键。
```flyux
object := {a: 1, b: 2}
has := hasKey(object, "a")     // true
```

#### merge(obj1, obj2, ...)
合并多个对象（后面的覆盖前面的）。
```flyux
object := merge({a: 1}, {b: 2}, {a: 3})  // {a: 3, b: 2}
```

#### clone(obj)
浅拷贝对象。
```flyux
object1 := {a: 1, b: 2}
object2 := clone(object1)
```

#### deepClone(obj)
深拷贝对象。
```flyux
object1 := {a: {b: 1}}
object2 := deepClone(object1)
```

---

### 🔀 类型转换

#### toNum(value)
转换为数字。
```flyux
n := toNum("123")           // 123
n := toNum("3.14")          // 3.14
n := toNum(true)            // 1
n := toNum(false)           // 0
```

#### toStr(value)
转换为字符串。
```flyux
s := toStr(123)             // "123"
s := toStr(true)            // "true"
s := toStr([1, 2])          // "[1, 2]"
s := toStr(null)            // "null"
```

#### toBl(value)
转换为布尔值。
```flyux
b := toBl(1)              // true
b := toBl(0)              // false
b := toBl("")             // false
b := toBl("hello")        // true
```

#### typeOf(value)
返回值的类型字符串。
```flyux
t := typeOf(123)            // "num"
t := typeOf("hello")        // "str"
t := typeOf(true)           // "bl"
t := typeOf([1, 2])         // "obj" (数组也是对象)
t := typeOf({a: 1})         // "obj"
t := typeOf(null)           // 如有先前类型则返回，无类型返回"null"
t := typeOf(undef)          // "undef"
```

#### isNum(value)
检查是否为数字。
```flyux
result := isNum(123)        // true
```

#### isStr(value)
检查是否为字符串。
```flyux
result := isStr("hello")    // true
```

#### isBl(value)
检查是否为布尔值。
```flyux
result := isBl(true)      // true
```

#### isArr(value)
检查是否为数组。
```flyux
result := isArr([1, 2])   // true
```

#### isObj(value)
检查是否为对象。
```flyux
result := isObj({a: 1})  // true
```

#### isNull(value)
检查是否为null。
```flyux
result := isNull(null)      // true
```

#### isUndef(value)
检查是否为undef。
```flyux
result := isUndef(undef)    // true
result := isUndef(x)        // true 不存在的量值
```

---

### ⏱️ 时间函数

#### now()
返回当前时间戳（毫秒）。
```flyux
timestamp := now()          // 1700000000000
```

#### sleep(milliseconds)
暂停执行指定毫秒数。
```flyux
sleep(1000)                 // 暂停1秒
```

#### dateStr()
返回当前日期时间字符串。
```flyux
dt := dateStr()             // "2025-11-17 15:30:45"
```

---

### 🔍 实用工具

#### assert(condition, message?)
断言条件为真，否则报错。
```flyux
assert(x > 0, "x must be positive")
```

#### exit(code?)
退出程序，可选退出码。
```flyux
exit(0)                     // 正常退出
exit(1)                     // 错误退出
```

#### range(start, end, step?)
生成数字范围数组。
```flyux
arr := range(0, 5)          // [0, 1, 2, 3, 4]
arr := range(0, 10, 2)      // [0, 2, 4, 6, 8]
```

---

### 📊 内置函数总结

| 分类 | 函数数量 | 主要功能 |
|------|----------|----------|
| 输入输出 | 4 | print, input, readFile, writeFile |
| 字符串 | 11 | 操作、查找、转换 |
| 数学 | 9 | 运算、随机、取整 |
| 数组 | 16 | 增删改查、高阶函数 |
| 对象 | 7 | 键值操作、合并克隆 |
| 类型 | 10 | 转换、类型检查 |
| 时间 | 3 | 时间戳、延迟、格式化 |
| 工具 | 3 | 断言、退出、范围 |
| **总计** | **63** | 覆盖常见编程需求 |

---

### 💡 使用示例

```flyux
// 字符串处理
text := "  Hello, FLYUX!  "
text = trim(text)
text = toUpper(text)
print(text)  // "HELLO, FLYUX!"

// 数组操作
nums := [3, 1, 4, 1, 5, 9]
sort(nums)
doubled := map(nums, (x) { R> x * 2 })
sum := reduce(doubled, (a, b) { R> a + b }, 0)
print("Sum:", sum)

// 对象处理
user := {name: "Alice", age: 30, city: "NYC"}
print("Keys:", keys(user))
print("Values:", values(user))

if (hasKey(user, "email")) {
    print("Email:", user.email)
} {
    print("No email")
}

// 类型检查和转换
value := "123"
if (isStr(value)) {
    num := toNum(value)
    print("Number:", num)
}

// 文件操作
content := readFile("data.txt")
lines := split(content, "\n")
L> (lines : line) {
    print(line)
}

// 数学计算
x := random()
y := randomInt(1, 100)
result := round(sqrt(pow(x, 2) + pow(y, 2)), 2)
print("Result:", result)
```

---

**文档版本**: 3.1
**最后更新**: 2025-11-27

## 📝 更新历史

### 版本 3.1 (2025-11-27)
- 修复已知问题

### 版本 3.0 (2025-11-20)
- ✅ 添加扩展对象类型系统 (Buffer、FileHandle、Error)
- ✅ 完善文件I/O函数文档 (readFile/writeFile/readBytes/writeBytes/openFile)
- ✅ 添加目录操作函数 (listDir/dirExists/makeDir/removeDir)
- ✅ 添加扩展对象使用模式和最佳实践
- ✅ 说明扩展对象的print输出特性

### 版本 2.0 (2025-11-17)
- 完善类型系统文档
- 添加动态对象操作

### 版本 1.0 (初始版本)
- FLYUX基础语法规范
- 变量定义、函数、控制流
- 基础内置函数
