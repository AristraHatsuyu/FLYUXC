cmake_minimum_required(VERSION 3.13)

# 设置项目名称和语言
project(flyuxc C CXX)

# 设置 C/C++ 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)

# 收集源文件 - 模块化结构
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
