cmake_minimum_required(VERSION 3.13)

# 设置项目名称和语言
project(flyuxc C CXX)

# 设置 C/C++ 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# 添加 LLVM 定义和头文件路径
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)

# LLVM 组件 - 精简配置
# 使用 REMOVE_DUPLICATES 自动去重，避免警告
llvm_map_components_to_libnames(llvm_libs
    Core
    IRReader
    Passes
    AllTargetsCodeGens
    AllTargetsAsmParsers
)
list(REMOVE_DUPLICATES llvm_libs)

# ============================================
# 自动生成嵌入的 Runtime 文件
# ============================================

set(RUNTIME_SOURCE "${CMAKE_SOURCE_DIR}/src/backend/runtime/value_runtime.c")
set(RUNTIME_OBJECT "${CMAKE_SOURCE_DIR}/src/backend/runtime_object.o")
set(RUNTIME_OBJECT_EMBEDDED "${CMAKE_SOURCE_DIR}/src/backend/runtime_object_embedded.h")
set(RUNTIME_SOURCE_EMBEDDED "${CMAKE_SOURCE_DIR}/src/backend/runtime_embedded.h")

# 步骤 1: 编译 runtime 对象文件 (优化:函数分段+代码大小优化)
add_custom_command(
    OUTPUT ${RUNTIME_OBJECT}
    COMMAND ${CMAKE_C_COMPILER} -c -Os -ffunction-sections -fdata-sections -o ${RUNTIME_OBJECT} ${RUNTIME_SOURCE}
    DEPENDS ${RUNTIME_SOURCE}
    COMMENT "编译 runtime 对象文件..."
)

# 步骤 2: 生成嵌入的二进制数组 (runtime_object_embedded.h)
add_custom_command(
    OUTPUT ${RUNTIME_OBJECT_EMBEDDED}
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/generate_object_embedded.sh ${RUNTIME_OBJECT} ${RUNTIME_OBJECT_EMBEDDED}
    DEPENDS ${RUNTIME_OBJECT}
    COMMENT "生成 runtime_object_embedded.h..."
)

# 步骤 3: 生成嵌入的源码字符串 (runtime_embedded.h)
add_custom_command(
    OUTPUT ${RUNTIME_SOURCE_EMBEDDED}
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/generate_runtime_embedded.py ${RUNTIME_SOURCE} ${RUNTIME_SOURCE_EMBEDDED}
    DEPENDS ${RUNTIME_SOURCE}
    COMMENT "生成 runtime_embedded.h..."
)

# 创建自定义目标，确保在编译前生成 runtime 文件
add_custom_target(generate_runtime
    DEPENDS ${RUNTIME_OBJECT} ${RUNTIME_OBJECT_EMBEDDED} ${RUNTIME_SOURCE_EMBEDDED}
)

# ============================================
# 收集源文件 - 模块化结构
# ============================================
file(GLOB_RECURSE FRONTEND_SOURCES "src/frontend/**/*.c")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/**/*.c")
file(GLOB_RECURSE MIDDLE_SOURCES "src/middle/**/*.c")
file(GLOB_RECURSE BACKEND_C_SOURCES "src/backend/**/*.c")

# 创建主可执行文件
add_executable(${PROJECT_NAME} 
    src/main.c
    src/backend/llvm_compiler.cpp
    src/backend/embed_runtime.c
    ${FRONTEND_SOURCES}
    ${UTILS_SOURCES}
    ${MIDDLE_SOURCES}
    ${BACKEND_C_SOURCES}
)

# 确保在编译前生成 runtime 文件
add_dependencies(${PROJECT_NAME} generate_runtime)

# 链接 LLVM 库
target_link_libraries(${PROJECT_NAME} ${llvm_libs})

# 抑制重复库警告（这是 LLVM 库之间依赖关系导致的正常现象）
if(APPLE)
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,-w)
endif()

# ============================================
# 编译完成后清理 runtime 中间文件
# ============================================
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f ${RUNTIME_OBJECT} ${RUNTIME_OBJECT_EMBEDDED} ${RUNTIME_SOURCE_EMBEDDED}
)
