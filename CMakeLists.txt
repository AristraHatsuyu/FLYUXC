cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(MyProject C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 添加源文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 添加所有源文件
file(GLOB_RECURSE SOURCES "src/*.c")

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})