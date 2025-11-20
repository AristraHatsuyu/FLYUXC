# FLYUX 文件I/O与扩展对象 - 快速开始指南

**版本**: 1.0  
**日期**: 2025-11-20  
**状态**: 设计完成,准备实现

---

## 🎯 项目目标

为FLYUX语言添加完整的文件I/O系统和扩展对象类型支持,实现类似JavaScript/Python的文件操作能力。

---

## 📋 核心特性

### 1. 扩展对象类型系统
- **Buffer**: 二进制数据容器,用于图片/音频/二进制文件
- **FileHandle**: 文件句柄,支持流式读写
- **Error**: 错误对象,增强错误处理

### 2. 文件I/O函数
- **文本文件**: `readFile()`, `writeFile()`, `appendFile()`
- **二进制文件**: `readBytes()`, `writeBytes()`
- **流式操作**: `openFile()` + FileHandle方法
- **文件系统**: `fileExists()`, `deleteFile()`, `getFileSize()`

### 3. 目录操作
- `listDir()`, `dirExists()`, `makeDir()`, `removeDir()`

---

## 📖 语法示例

### 读写文本文件
```flyux
// 写入文件
writeFile("config.txt", "host=localhost\nport=8080")

// 读取文件
content := readFile("config.txt")
print(content)
```

### 读写二进制文件
```flyux
// 读取图片
buffer := readBytes("photo.jpg")
print(buffer)  // Buffer { size: 15234, type: "Buffer" }
print("大小:", buffer.size)

// 检查文件头
if buffer[0] == 0xFF && buffer[1] == 0xD8 {
    print("JPEG格式")
}
```

### 流式处理大文件
```flyux
file := openFile("large.log", "r")
if file != null {
    L> [10000] {
        line := file.readLine()
        if line == null { break }
        print(line)
    }
    file.close()
}
```

### 目录操作
```flyux
files := listDir("./data")
L> (files : filename) {
    print("文件:", filename)
}
```

---

## 🎨 设计亮点

### 1. 安全的print输出
扩展对象print时只显示元信息,不会刷屏:
```flyux
buffer := readBytes("10MB.bin")
print(buffer)  // Buffer { size: 10485760, type: "Buffer" }
```

### 2. 属性访问
像普通对象一样访问扩展对象属性:
```flyux
file := openFile("test.txt", "r")
print(file.path)        // 文件路径
print(file.isOpen)      // true
print(file.position)    // 当前位置
```

### 3. 类型识别
```flyux
buffer := readBytes("data.bin")
print(typeOf(buffer))   // "Buffer"

file := openFile("test.txt", "r")
print(typeOf(file))     // "FileHandle"
```

---

## 📂 文档结构

### 设计文档
- **EXTENDED_OBJECT_TYPES.md** - 扩展类型系统完整设计
- **FILE_IO_DESIGN.md** - 文件I/O详细设计
- **FLYUX_SYNTAX.md** (v3.0) - 更新的语法文档

### 实现文档
- **IMPLEMENTATION_CHECKLIST.md** - 详细实现清单和代码示例

---

## 🚀 实现计划

### Phase 1: 基础设施 (1-2天)
- 扩展Value结构
- 修改typeOf/print/get_field
- 定义扩展对象结构

### Phase 2: 文本文件I/O (1天)
- readFile/writeFile/appendFile
- fileExists/deleteFile/getFileSize

### Phase 3: Buffer和二进制I/O (1-2天)
- Buffer对象实现
- readBytes/writeBytes
- Buffer属性和索引访问

### Phase 4: FileHandle流式操作 (1-2天)
- FileHandle对象实现
- openFile/readLine/close

### Phase 5: 目录操作 (0.5-1天)
- listDir/dirExists/makeDir/removeDir

### Phase 6: 错误对象 (0.5天)
- Error对象和lastErrorObj()

### Phase 7: 测试和文档 (0.5-1天)
- 集成测试
- 性能测试
- 文档完善

**预计总工期**: 5-7天

---

## 📝 需要修改的文件

### 核心文件
1. **src/backend/runtime/value_runtime.c**
   - 添加ext_type字段
   - 定义扩展对象结构
   - 修改typeOf/print/get_field/index
   - 实现所有文件I/O函数

2. **src/backend/codegen/codegen.c**
   - 添加文件I/O函数声明

3. **src/frontend/lexer/varmap.c**
   - 添加内置函数标识

### 测试文件
- testfx/file_io/*.fx (新建目录)

---

## ✅ 当前状态

- ✅ 设计文档完成
- ✅ 语法文档更新
- ✅ API设计完成
- ✅ 实现清单准备
- ⏳ 代码实现 (下一步)

---

## 📞 快速参考

### 查看完整设计
```bash
# 扩展类型系统
cat docs/EXTENDED_OBJECT_TYPES.md

# 文件I/O设计
cat docs/FILE_IO_DESIGN.md

# 语法文档
cat docs/FLYUX_SYNTAX.md

# 实现清单
cat docs/IMPLEMENTATION_CHECKLIST.md
```

### 开始实现
1. 阅读 `IMPLEMENTATION_CHECKLIST.md`
2. 按Phase顺序实现
3. 每个Phase完成后测试
4. 更新文档状态

---

**设计完成度**: 100%  
**准备度**: 100%  
**可以开始实现**: ✅

