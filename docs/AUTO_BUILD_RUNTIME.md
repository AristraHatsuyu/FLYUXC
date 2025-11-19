# FLYUX Runtime 自动构建

## ✅ 已实现自动化

Runtime 现在在编译 FLYUXC 时自动生成，**不需要手动运行脚本**！

## 工作原理

当你运行 `cmake --build build` 时，CMake 会自动：

```
1. 检测 value_runtime.c 是否修改
   ↓
2. 自动编译 runtime_object.o
   ↓
3. 自动生成 runtime_object_embedded.h（二进制数组）
   ↓
4. 自动生成 runtime_embedded.h（源码字符串）
   ↓
5. 编译 flyuxc（包含嵌入的 runtime）
```

## 使用方法

### 修改 Runtime 颜色或功能

1. **编辑源文件**：
   ```bash
   vim src/backend/runtime/value_runtime.c
   # 修改颜色定义或任何功能
   ```

2. **重新编译**（自动生成）：
   ```bash
   cmake --build build
   ```

3. **完成！**
   - Runtime 自动重新编译
   - 嵌入文件自动重新生成
   - flyuxc 自动重新链接

### 首次构建

```bash
# 配置
cmake -B build

# 编译（自动生成 runtime）
cmake --build build
```

## CMake 配置

在 `CMakeLists.txt` 中配置了三个自动生成步骤：

```cmake
# 步骤 1: 编译 runtime 对象文件
add_custom_command(
    OUTPUT ${RUNTIME_OBJECT}
    COMMAND ${CMAKE_C_COMPILER} -c -o ${RUNTIME_OBJECT} ${RUNTIME_SOURCE}
    DEPENDS ${RUNTIME_SOURCE}
)

# 步骤 2: 生成嵌入的二进制数组
add_custom_command(
    OUTPUT ${RUNTIME_OBJECT_EMBEDDED}
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/generate_object_embedded.sh ...
    DEPENDS ${RUNTIME_OBJECT}
)

# 步骤 3: 生成嵌入的源码字符串
add_custom_command(
    OUTPUT ${RUNTIME_SOURCE_EMBEDDED}
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/generate_runtime_embedded.py ...
    DEPENDS ${RUNTIME_SOURCE}
)

# flyuxc 依赖这些文件
add_dependencies(${PROJECT_NAME} generate_runtime)
```

## 辅助脚本

虽然不需要手动运行，但这些脚本被 CMake 自动调用：

- `scripts/generate_object_embedded.sh` - 生成二进制数组
- `scripts/generate_runtime_embedded.py` - 生成源码字符串

## 对比：之前 vs 现在

### 之前（手动）

```bash
# 1. 修改 runtime
vim src/backend/runtime/value_runtime.c

# 2. 手动运行脚本
./rebuild_runtime.sh

# 3. 重新编译
cmake --build build
```

### 现在（自动）

```bash
# 1. 修改 runtime
vim src/backend/runtime/value_runtime.c

# 2. 直接编译（自动完成所有步骤）
cmake --build build
```

## 增量编译

CMake 智能检测依赖关系：

| 修改内容 | 重新生成内容 |
|---------|-------------|
| `value_runtime.c` | runtime_object.o + 两个 .h + flyuxc |
| 其他源文件 | 只重新编译该文件 |
| 无修改 | 跳过（秒速完成）|

## 验证自动生成

测试修改后自动重新生成：

```bash
# 修改 runtime
echo '// test' >> src/backend/runtime/value_runtime.c

# 编译（观察自动生成）
cmake --build build 2>&1 | grep runtime
```

输出：
```
[  4%] 编译 runtime 对象文件...
[  9%] 生成 runtime_object_embedded.h...
[ 13%] 生成 runtime_embedded.h...
[ 13%] Built target generate_runtime
```

## 故障排查

### Q: 修改 runtime 后没有重新生成？

**A**: 清理并重新构建：
```bash
rm -rf build
cmake -B build
cmake --build build
```

### Q: 生成的文件在哪里？

**A**: 
- `src/backend/runtime_object.o` - 中间文件
- `src/backend/runtime_object_embedded.h` - 嵌入的二进制
- `src/backend/runtime_embedded.h` - 嵌入的源码

### Q: 能手动运行生成脚本吗？

**A**: 可以（用于调试），但不推荐：
```bash
# 生成二进制数组
./scripts/generate_object_embedded.sh src/backend/runtime_object.o output.h

# 生成源码字符串
./scripts/generate_runtime_embedded.py src/backend/runtime/value_runtime.c output.h
```

## 总结

✅ **完全自动化**：修改 runtime → 直接 `cmake --build build`
✅ **智能增量**：只重新生成修改的部分
✅ **零手动操作**：不需要运行任何脚本
✅ **依赖跟踪**：CMake 自动管理所有依赖关系

**现在你只需要关注代码，构建系统会自动处理一切！** 🎉
