# FLYUX 扩展对象类型系统设计

**设计日期**: 2025-11-20  
**版本**: 1.0  
**状态**: 待实现

## 📋 设计概述

FLYUX的基础类型系统中,`obj` 是通用对象类型。为了支持特殊用途(如文件I/O、二进制数据处理),我们引入**扩展对象类型**机制。这些扩展类型本质上是obj,但带有特殊的内部标识和受限的输出行为。

## 🎯 设计原则

1. **类型兼容性**: 扩展类型是obj的子类型,可以赋值给obj变量
2. **安全输出**: print()输出扩展对象时只显示元信息,不输出大量原始数据
3. **属性访问**: 支持通过`.`或`[]`访问对象属性
4. **类型识别**: `typeOf()`返回具体的扩展类型名
5. **垃圾回收**: 自动管理内存,避免泄漏

## 🔧 扩展类型列表

### 1. Buffer - 二进制缓冲区

**用途**: 存储文件读取的二进制数据、网络数据包、图像数据等

**创建方式**:
```flyux
// 从文件读取二进制数据
buffer := readBytes("image.png")

// 创建指定大小的空Buffer
buffer := createBuffer(1024)

// 从数组创建Buffer
bytes := [0x48, 0x65, 0x6C, 0x6C, 0x6F]
buffer := Buffer(bytes)
```

**内部结构**:
```c
typedef struct {
    char *data;           // 原始二进制数据
    size_t size;          // 数据大小(字节)
    size_t capacity;      // 分配容量
    char *_type_tag;      // 固定为 "Buffer"
} BufferObject;
```

**属性**:
- `size: num` - 缓冲区大小(字节)
- `type: str` - 固定为 "Buffer"
- 可通过索引访问: `buffer[0]` 返回第0个字节(0-255)

**方法**:
- `slice(start, end?)` - 切片缓冲区
- `toString(encoding?)` - 转换为字符串(默认UTF-8)
- `toArray()` - 转换为数字数组

**print输出**:
```
Buffer { size: 1024, type: "Buffer" }
```

**示例**:
```flyux
// 读取图片文件
img := readBytes("photo.jpg")
print(img)  // 输出: Buffer { size: 15234, type: "Buffer" }
print("图片大小:", img.size, "字节")

// 访问第一个字节
first_byte := img[0]
print("Magic number:", first_byte)  // 255 (JPEG标识)

// 转换为数组(仅适合小数据)
if img.size < 100 {
    arr := img.toArray()
    print("前10字节:", arr[0], arr[1], arr[2])
}
```

---

### 2. FileHandle - 文件句柄

**用途**: 表示打开的文件,支持流式读写、大文件处理

**创建方式**:
```flyux
// 打开文件读取
file := openFile("data.txt", "r")

// 打开文件写入
file := openFile("output.log", "w")

// 追加模式
file := openFile("log.txt", "a")
```

**内部结构**:
```c
typedef struct {
    FILE *fp;             // C文件指针
    char *path;           // 文件路径
    char *mode;           // 打开模式 (r/w/a/rb/wb)
    int is_open;          // 是否打开
    long position;        // 当前位置
    char *_type_tag;      // 固定为 "FileHandle"
} FileHandleObject;
```

**属性**:
- `path: str` - 文件路径
- `mode: str` - 打开模式
- `position: num` - 当前读写位置
- `isOpen: bl` - 是否打开
- `type: str` - 固定为 "FileHandle"

**方法**:
- `read(size?)` - 读取指定字节数(默认全部)
- `readLine()` - 读取一行
- `write(content)` - 写入内容
- `seek(position)` - 移动读写位置
- `close()` - 关闭文件

**print输出**:
```
FileHandle { path: "data.txt", mode: "r", position: 0, isOpen: true }
```

**示例**:
```flyux
// 流式读取大文件
file := openFile("large.log", "r")
if file != null {
    L> [1000] {  // 最多读1000行
        line := file.readLine()
        if line == null { break }
        print(line)
    }
    file.close()
} else {
    print("无法打开文件:", lastError())
}

// 逐块写入文件
file := openFile("output.bin", "wb")
L> (i := 0; i < 100; i++) {
    data := generateData(i)
    file.write(data)
}
file.close()
```

---

### 3. Stream - 数据流

**用途**: 表示可读/可写的数据流(标准输入输出、网络连接等)

**创建方式**:
```flyux
// 标准输入输出流
stdin := getStdin()
stdout := getStdout()
stderr := getStderr()

// 从Buffer创建流
stream := createStream(buffer)
```

**内部结构**:
```c
typedef struct {
    void *source;         // 数据源(FILE*或Buffer*)
    int stream_type;      // 流类型(stdin/stdout/file/buffer)
    int is_readable;      // 是否可读
    int is_writable;      // 是否可写
    long position;        // 当前位置
    char *_type_tag;      // 固定为 "Stream"
} StreamObject;
```

**属性**:
- `readable: bl` - 是否可读
- `writable: bl` - 是否可写
- `position: num` - 当前位置
- `type: str` - 固定为 "Stream"

**方法**:
- `read(size)` - 读取数据
- `write(data)` - 写入数据
- `pipe(destStream)` - 管道到另一个流
- `close()` - 关闭流

**print输出**:
```
Stream { readable: true, writable: false, position: 0 }
```

**示例**:
```flyux
// 重定向输出到文件
outFile := openFile("output.txt", "w")
stdout_backup := getStdout()
setStdout(outFile)
print("这会写入文件")
setStdout(stdout_backup)
outFile.close()
```

---

### 4. Error - 错误对象

**用途**: 表示错误信息,支持异常处理

**创建方式**:
```flyux
// 手动创建错误
err := createError("文件未找到", 404)

// 系统自动创建(文件操作失败时)
content := readFile("missing.txt")
if content == null {
    err := lastErrorObj()  // 获取Error对象
    print(err)
}
```

**内部结构**:
```c
typedef struct {
    char *message;        // 错误消息
    int code;             // 错误代码
    char *type;           // 错误类型(Error/TypeError/IOError)
    char *stack;          // 调用栈(可选)
    char *_type_tag;      // 固定为 "Error"
} ErrorObject;
```

**属性**:
- `message: str` - 错误消息
- `code: num` - 错误代码
- `errorType: str` - 错误类型
- `type: str` - 固定为 "Error"

**print输出**:
```
Error { message: "文件未找到", code: 1001, errorType: "IOError" }
```

**示例**:
```flyux
// 错误处理
result := readFile("config.json")
if result == null {
    err := lastErrorObj()
    if err.code == 1001 {
        print("文件不存在:", err.message)
    } else {
        print("未知错误:", err)
    }
}
```

---

### 5. Directory - 目录对象

**用途**: 表示文件系统目录,支持遍历和查询

**创建方式**:
```flyux
dir := openDir("./data")
```

**内部结构**:
```c
typedef struct {
    char *path;           // 目录路径
    void *dir_ptr;        // DIR* 指针
    int is_open;          // 是否打开
    char *_type_tag;      // 固定为 "Directory"
} DirectoryObject;
```

**属性**:
- `path: str` - 目录路径
- `isOpen: bl` - 是否打开
- `type: str` - 固定为 "Directory"

**方法**:
- `readNext()` - 读取下一个文件/目录名
- `list()` - 返回所有文件名数组
- `close()` - 关闭目录句柄

**print输出**:
```
Directory { path: "./data", isOpen: true }
```

**示例**:
```flyux
dir := openDir("./testfx")
if dir != null {
    files := dir.list()
    L> (files : filename) {
        print("文件:", filename)
    }
    dir.close()
}
```

---

## 🔍 类型识别机制

### typeOf() 函数扩展

```flyux
buffer := readBytes("data.bin")
print(typeOf(buffer))        // "Buffer"

file := openFile("test.txt", "r")
print(typeOf(file))          // "FileHandle"

err := createError("test", 1)
print(typeOf(err))           // "Error"

normal := { a: 1, b: 2 }
print(typeOf(normal))        // "obj"
```

### 实现机制

在 `Value` 结构中添加扩展类型标识:

```c
typedef struct {
    int type;              // VALUE_OBJECT
    int declared_type;     // VALUE_OBJECT
    int ext_type;          // 扩展类型标识
    union {
        double number;
        char *string;
        void *pointer;     // 指向扩展对象结构
    } data;
    long array_size;
    size_t string_length;
} Value;

// 扩展类型标识
#define EXT_TYPE_NONE      0  // 普通obj
#define EXT_TYPE_BUFFER    1  // Buffer类型
#define EXT_TYPE_FILE      2  // FileHandle类型
#define EXT_TYPE_STREAM    3  // Stream类型
#define EXT_TYPE_ERROR     4  // Error类型
#define EXT_TYPE_DIRECTORY 5  // Directory类型
```

---

## 📤 print() 输出行为

### 普通对象输出
```flyux
user := { name: "Alice", age: 30 }
print(user)
// 输出: { name: "Alice", age: 30 }
```

### 扩展对象输出(元信息)
```flyux
buffer := readBytes("large.bin")
print(buffer)
// 输出: Buffer { size: 5242880, type: "Buffer" }
// 不输出实际数据,避免终端刷屏

file := openFile("data.txt", "r")
print(file)
// 输出: FileHandle { path: "data.txt", mode: "r", position: 0, isOpen: true }
```

### 实现机制

修改 `value_print()` 函数:

```c
void value_print(Value *v) {
    if (v->type == VALUE_OBJECT && v->ext_type != EXT_TYPE_NONE) {
        // 扩展对象类型,仅输出元信息
        print_extended_object_meta(v);
    } else {
        // 普通对象,正常输出
        print_object_json_depth(entries, v->array_size, 0);
    }
}

static void print_extended_object_meta(Value *v) {
    int use_colors = should_use_colors();
    const char* type_color = use_colors ? BRACKET_GOLD : "";
    const char* reset = use_colors ? COLOR_RESET : "";
    
    switch (v->ext_type) {
        case EXT_TYPE_BUFFER: {
            BufferObject *buf = (BufferObject*)v->data.pointer;
            printf("%sBuffer%s { size: %zu, type: \"Buffer\" }", 
                   type_color, reset, buf->size);
            break;
        }
        case EXT_TYPE_FILE: {
            FileHandleObject *file = (FileHandleObject*)v->data.pointer;
            printf("%sFileHandle%s { path: \"%s\", mode: \"%s\", position: %ld, isOpen: %s }", 
                   type_color, reset, file->path, file->mode, file->position,
                   file->is_open ? "true" : "false");
            break;
        }
        case EXT_TYPE_ERROR: {
            ErrorObject *err = (ErrorObject*)v->data.pointer;
            printf("%sError%s { message: \"%s\", code: %d, errorType: \"%s\" }",
                   type_color, reset, err->message, err->code, err->type);
            break;
        }
        // ... 其他类型
    }
}
```

---

## 🔗 属性访问机制

扩展对象支持属性访问,就像普通对象:

```flyux
buffer := readBytes("data.bin")
size := buffer.size           // 访问size属性
type_name := buffer.type      // 访问type属性

file := openFile("test.txt", "r")
path := file.path             // 访问path属性
pos := file.position          // 访问position属性
```

### 实现机制

扩展 `value_get_field()` 函数:

```c
Value* value_get_field(Value *obj, Value *key) {
    if (!obj || obj->type != VALUE_OBJECT) return box_null();
    
    const char *field = (const char*)key->data.pointer;
    
    // 处理扩展对象类型
    if (obj->ext_type == EXT_TYPE_BUFFER) {
        BufferObject *buf = (BufferObject*)obj->data.pointer;
        if (strcmp(field, "size") == 0) {
            return box_number((double)buf->size);
        } else if (strcmp(field, "type") == 0) {
            return box_string("Buffer");
        }
    } else if (obj->ext_type == EXT_TYPE_FILE) {
        FileHandleObject *file = (FileHandleObject*)obj->data.pointer;
        if (strcmp(field, "path") == 0) {
            return box_string(file->path);
        } else if (strcmp(field, "mode") == 0) {
            return box_string(file->mode);
        } else if (strcmp(field, "position") == 0) {
            return box_number((double)file->position);
        } else if (strcmp(field, "isOpen") == 0) {
            return box_bool(file->is_open);
        } else if (strcmp(field, "type") == 0) {
            return box_string("FileHandle");
        }
    }
    // ... 其他扩展类型
    
    // 普通对象,正常查找
    ObjectEntry *entries = (ObjectEntry*)obj->data.pointer;
    // ... 原有逻辑
}
```

---

## 📚 文件I/O函数与扩展类型的配合

### 基础文本文件操作(返回string)

```flyux
// readFile - 返回字符串(小文件)
content :[str]= readFile("config.txt")
if content != null {
    print("文件内容:", content)
}

// writeFile - 接受字符串
success := writeFile("output.txt", "Hello, FLYUX!")

// appendFile - 接受字符串
appendFile("log.txt", "新日志行\n")
```

### 二进制文件操作(返回Buffer)

```flyux
// readBytes - 返回Buffer对象
buffer :[obj]= readBytes("image.png")
if buffer != null {
    print(buffer)  // Buffer { size: 15234, type: "Buffer" }
    print("文件大小:", buffer.size)
    
    // 访问字节
    first := buffer[0]
    second := buffer[1]
}

// writeBytes - 接受Buffer或数组
bytes := [0x89, 0x50, 0x4E, 0x47]  // PNG头
writeBytes("test.png", bytes)
```

### 流式文件操作(返回FileHandle)

```flyux
// openFile - 返回FileHandle对象
file :[obj]= openFile("large.log", "r")
if file != null {
    print(file)  // FileHandle { path: "large.log", mode: "r", ... }
    
    // 逐行读取
    L> [1000] {
        line := file.readLine()
        if line == null { break }
        processLine(line)
    }
    
    file.close()
}
```

### 目录操作(返回array或Directory)

```flyux
// listDir - 返回字符串数组(简单)
files :[str]= listDir("./data")
L> (files : name) {
    print("文件:", name)
}

// openDir - 返回Directory对象(高级)
dir :[obj]= openDir("./data")
if dir != null {
    L> [100] {
        entry := dir.readNext()
        if entry == null { break }
        print("条目:", entry)
    }
    dir.close()
}
```

---

## 🛠️ 实现计划

### Phase 1: 核心基础设施
1. **扩展Value结构**
   - 添加 `ext_type` 字段
   - 定义扩展类型常量
   
2. **修改typeOf()函数**
   - 识别扩展类型
   - 返回正确的类型名
   
3. **修改value_print()函数**
   - 检测扩展类型
   - 仅输出元信息

4. **修改value_get_field()函数**
   - 支持扩展对象属性访问
   - 虚拟属性实现

### Phase 2: Buffer类型实现
1. 定义 `BufferObject` 结构
2. 实现 `readBytes()` - 返回Buffer
3. 实现 `writeBytes()` - 接受Buffer或数组
4. 实现 `createBuffer(size)` - 创建空Buffer
5. 实现Buffer方法:
   - `slice(start, end?)`
   - `toString(encoding?)`
   - `toArray()`
6. 实现Buffer索引访问 `buffer[index]`

### Phase 3: 基础文件I/O
1. 实现 `readFile()` - 返回string
2. 实现 `writeFile()` - 接受string
3. 实现 `appendFile()` - 接受string
4. 实现 `fileExists()`
5. 实现 `deleteFile()`
6. 实现 `getFileSize()`

### Phase 4: FileHandle类型实现
1. 定义 `FileHandleObject` 结构
2. 实现 `openFile(path, mode)` - 返回FileHandle
3. 实现FileHandle方法:
   - `read(size?)`
   - `readLine()`
   - `write(content)`
   - `seek(position)`
   - `close()`

### Phase 5: Error类型实现
1. 定义 `ErrorObject` 结构
2. 实现 `createError(message, code)`
3. 实现 `lastErrorObj()` - 获取Error对象
4. 集成到文件I/O错误处理

### Phase 6: 其他扩展类型(可选)
1. Directory类型
2. Stream类型
3. 网络相关类型(Socket等)

---

## 🧪 测试用例

### 测试1: Buffer基础操作
```flyux
# test_buffer.fx

// 读取二进制文件
buffer := readBytes("testfx/data/test.bin")
print("Buffer:", buffer)
print("大小:", buffer.size)
print("类型:", typeOf(buffer))

// 访问字节
print("第一个字节:", buffer[0])
print("第二个字节:", buffer[1])

// 转换为数组
if buffer.size < 20 {
    arr := buffer.toArray()
    print("数组:", arr)
}

// 切片
slice := buffer.slice(0, 10)
print("切片:", slice)

// 写入Buffer
bytes := [0x48, 0x65, 0x6C, 0x6C, 0x6F]  // "Hello"
writeBytes("test_output.bin", bytes)
```

### 测试2: 文本文件操作
```flyux
# test_text_file.fx

// 写入文本
success := writeFile("test.txt", "Hello, FLYUX!\nLine 2\n")
print("写入:", success)

// 读取文本
content := readFile("test.txt")
print("内容:", content)
print("类型:", typeOf(content))  // "str"

// 追加文本
appendFile("test.txt", "Line 3\n")

// 重新读取
content = readFile("test.txt")
print("追加后:", content)

// 检查文件
exists := fileExists("test.txt")
print("文件存在:", exists)

// 获取大小
size := getFileSize("test.txt")
print("文件大小:", size)

// 删除文件
deleted := deleteFile("test.txt")
print("删除成功:", deleted)
```

### 测试3: FileHandle流式操作
```flyux
# test_file_handle.fx

// 写入大文件
file := openFile("large.txt", "w")
if file != null {
    print("打开文件:", file)
    
    L> (i := 0; i < 100; i++) {
        line := "Line " + str(i) + "\n"
        file.write(line)
    }
    
    print("写入位置:", file.position)
    file.close()
    print("关闭后:", file.isOpen)
}

// 逐行读取
file = openFile("large.txt", "r")
if file != null {
    count := 0
    L> [1000] {
        line := file.readLine()
        if line == null { break }
        count = count + 1
    }
    print("读取行数:", count)
    file.close()
}
```

### 测试4: 扩展对象print行为
```flyux
# test_extended_print.fx

// 普通对象 - 完整输出
user := { name: "Alice", age: 30, hobbies: ["coding", "reading"] }
print("普通对象:", user)

// Buffer - 仅元信息
buffer := readBytes("large.bin")
print("Buffer对象:", buffer)  // 不会输出MB级数据

// FileHandle - 仅元信息
file := openFile("test.txt", "r")
print("FileHandle对象:", file)
file.close()

// Error - 仅元信息
err := createError("测试错误", 999)
print("Error对象:", err)
```

### 测试5: 属性访问
```flyux
# test_property_access.fx

buffer := readBytes("data.bin")

// 直接属性访问
print("Buffer大小:", buffer.size)
print("Buffer类型:", buffer.type)

// 索引访问
print("第一字节:", buffer[0])
print("第十字节:", buffer[9])

// FileHandle属性
file := openFile("test.txt", "r")
print("文件路径:", file.path)
print("打开模式:", file.mode)
print("是否打开:", file.isOpen)
print("当前位置:", file.position)

file.readLine()
print("读取后位置:", file.position)
file.close()
```

---

## 📐 语法集成示例

结合FLYUX语法特性的完整示例:

### 示例1: 图片处理流水线
```flyux
# image_processor.fx

processImage := (inputPath, outputPath) {
    // 读取图片文件
    buffer :[obj]= readBytes(inputPath)
    
    if buffer == null {
        print("无法读取图片:", lastError())
        R> false
    }
    
    print("处理图片:", inputPath)
    print(buffer)  // Buffer { size: 15234, type: "Buffer" }
    
    // 检查文件头(JPEG)
    if buffer[0] == 0xFF && buffer[1] == 0xD8 {
        print("确认为JPEG格式")
    } else {
        print("警告: 非JPEG格式")
    }
    
    // 处理图片数据...
    processed := processImageData(buffer)
    
    // 写入输出文件
    success := writeBytes(outputPath, processed)
    R> success
}

main := () {
    result := processImage("input.jpg", "output.jpg")
    if result {
        print("处理成功")
    } else {
        print("处理失败")
    }
}
```

### 示例2: 日志分析器
```flyux
# log_analyzer.fx

analyzeLog := (logPath) {
    file :[obj]= openFile(logPath, "r")
    
    if file == null {
        print("无法打开日志:", lastError())
        R> null
    }
    
    print("分析日志文件:", file.path)
    
    // 统计数据
    stats := {
        total: 0,
        errors: 0,
        warnings: 0
    }
    
    // 逐行分析
    L> [100000] {  // 最多10万行
        line := file.readLine()
        if line == null { break }
        
        stats.total = stats.total + 1
        
        if indexOf(line, "ERROR") >= 0 {
            stats.errors = stats.errors + 1
        } (indexOf(line, "WARN") >= 0) {
            stats.warnings = stats.warnings + 1
        }
    }
    
    file.close()
    R> stats
}

main := () {
    result := analyzeLog("app.log")
    if result != null {
        print("日志统计:")
        print("总行数:", result.total)
        print("错误数:", result.errors)
        print("警告数:", result.warnings)
    }
}
```

### 示例3: 配置文件管理器
```flyux
# config_manager.fx

CONFIG_FILE :(str)= "app.config"

loadConfig := () {
    if !fileExists(CONFIG_FILE) {
        print("配置文件不存在,创建默认配置")
        defaultConfig := {
            host: "localhost",
            port: 8080,
            debug: true
        }
        saveConfig(defaultConfig)
        R> defaultConfig
    }
    
    content := readFile(CONFIG_FILE)
    if content == null {
        print("读取配置失败:", lastError())
        R> null
    }
    
    // 解析配置(简单的key=value格式)
    config := {}
    lines := split(content, "\n")
    
    L> (lines : line) {
        if length(line) == 0 { continue }
        if charAt(line, 0) == "#" { continue }
        
        parts := split(line, "=")
        if length(parts) == 2 {
            key := trim(parts[0])
            value := trim(parts[1])
            config[key] = value
        }
    }
    
    R> config
}

saveConfig := (config) {
    content := ""
    content = content + "# Application Configuration\n"
    content = content + "host=" + config.host + "\n"
    content = content + "port=" + str(config.port) + "\n"
    content = content + "debug=" + str(config.debug) + "\n"
    
    success := writeFile(CONFIG_FILE, content)
    R> success
}

main := () {
    config := loadConfig()
    print("配置:", config)
    
    // 修改配置
    config.port = 9000
    config.debug = false
    
    // 保存配置
    if saveConfig(config) {
        print("配置已保存")
    } else {
        print("保存失败")
    }
}
```

---

## 🔄 类型转换和兼容性

### Buffer ↔ Array
```flyux
// Array to Buffer
bytes := [0x48, 0x65, 0x6C, 0x6C, 0x6F]
buffer := Buffer(bytes)  // 或 writeBytes + readBytes

// Buffer to Array
buffer := readBytes("data.bin")
if buffer.size < 1000 {  // 仅小数据
    array := buffer.toArray()
}
```

### Buffer ↔ String
```flyux
// String to Buffer (通过文件)
text := "Hello, FLYUX!"
writeFile("temp.txt", text)
buffer := readBytes("temp.txt")

// Buffer to String
buffer := readBytes("text.txt")
text := buffer.toString("utf8")
```

### FileHandle → String
```flyux
file := openFile("data.txt", "r")
content := file.read()  // 读取全部为字符串
file.close()
```

---

## 📝 语法文档更新清单

需要在 `docs/FLYUX_SYNTAX.md` 中添加以下章节:

1. **扩展对象类型** (新章节)
   - Buffer类型说明
   - FileHandle类型说明
   - Error类型说明
   - 类型识别机制

2. **文件I/O函数** (更新现有章节)
   - 基础文件读写
   - 二进制文件操作
   - 流式文件操作
   - 文件系统查询

3. **对象属性访问** (补充说明)
   - 扩展对象属性访问
   - 虚拟属性概念

4. **print()行为** (补充说明)
   - 扩展对象的特殊输出
   - 元信息输出机制

---

## 🎯 实现优先级总结

### 🔴 高优先级 (立即实现)
1. Value结构扩展 (ext_type字段)
2. Buffer类型完整实现
3. readFile/writeFile/appendFile (string版本)
4. readBytes/writeBytes (Buffer版本)
5. typeOf()扩展
6. value_print()扩展
7. value_get_field()扩展

### 🟡 中优先级 (第二阶段)
1. FileHandle类型实现
2. openFile()函数
3. FileHandle方法(read/write/close)
4. Error类型实现
5. fileExists/deleteFile/getFileSize

### 🟢 低优先级 (后续优化)
1. Directory类型
2. Stream类型
3. Buffer高级方法(slice/toString)
4. 目录操作(listDir/makeDir)
5. 路径操作函数

---

**下一步**: 更新 `FLYUX_SYNTAX.md` 文档,添加扩展类型语法说明

