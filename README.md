# C Project with CMake

这是一个使用 CMake 构建的基础 C 语言项目。

## 项目结构

```
.
├── CMakeLists.txt
├── include/
│   └── mylib.h
└── src/
    ├── main.c
    └── mylib.c
```

## 构建说明

1. 创建构建目录：
   ```bash
   mkdir build
   cd build
   ```

2. 配置 CMake：
   ```bash
   cmake ..
   ```

3. 构建项目：
   ```bash
   cmake --build .
   ```

4. 运行程序：
   ```bash
   ./MyProject
   ```

## 项目功能

- 基础的 C 语言项目示例
- 使用 CMake 作为构建系统
- 包含简单的函数库示例