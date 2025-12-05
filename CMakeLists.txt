cmake_minimum_required(VERSION 3.13)

# 设置项目名称和语言
project(flyuxc C CXX)

# 设置 C/C++ 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 默认使用 Release 构建以获得优化
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# 设置优化选项
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 查找 LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# macOS上尽量使用静态库,但系统库保持动态链接
# Apple不支持完全静态链接,但我们可以静态链接第三方库
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".dylib" ".tbd")

# 添加 LLVM 定义和头文件路径
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)

# LLVM 组件 - 精简配置,只使用本地目标
# 使用 --link-static 确保使用静态 LLVM 库
execute_process(
    COMMAND llvm-config --link-static --libs core irreader passes native
    OUTPUT_VARIABLE LLVM_STATIC_LIBS_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND llvm-config --libdir
    OUTPUT_VARIABLE LLVM_LIB_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# 将 -lLibName 转换为绝对路径 /path/to/libLibName.a
string(REPLACE " " ";" LLVM_LIB_LIST ${LLVM_STATIC_LIBS_RAW})
set(llvm_libs "")
foreach(lib ${LLVM_LIB_LIST})
    string(REGEX REPLACE "^-l(.+)$" "${LLVM_LIB_DIR}/lib\\1.a" lib_path ${lib})
    list(APPEND llvm_libs ${lib_path})
endforeach()

message(STATUS "Using ${LLVM_LIB_DIR} for LLVM static libraries")

# 查找静态版本的第三方依赖库
find_library(ZSTD_STATIC NAMES libzstd.a PATHS /opt/homebrew/opt/zstd/lib REQUIRED)

message(STATUS "Using static library:")
message(STATUS "  ZSTD: ${ZSTD_STATIC}")
message(STATUS "Note: System libraries (libz, libSystem, libc++) are dynamically linked (macOS requirement)")

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
file(GLOB CORE_SOURCES "src/core/*.c")

# Backend 源文件 - 排除 runtime 子目录（因为 runtime 使用 #include 聚合）
file(GLOB BACKEND_CODEGEN_SOURCES "src/backend/codegen/*.c")
set(BACKEND_C_SOURCES 
    ${BACKEND_CODEGEN_SOURCES}
    src/backend/runtime/value_runtime.c
)

# 创建主可执行文件
add_executable(${PROJECT_NAME} 
    src/main.c
    src/backend/llvm_compiler.cpp
    src/backend/embed_runtime.c
    ${FRONTEND_SOURCES}
    ${UTILS_SOURCES}
    ${MIDDLE_SOURCES}
    ${CORE_SOURCES}
    ${BACKEND_C_SOURCES}
)

# 确保在编译前生成 runtime 文件
add_dependencies(${PROJECT_NAME} generate_runtime)

# 链接 LLVM 库和依赖库
target_link_libraries(${PROJECT_NAME} 
    ${llvm_libs}
    ${ZSTD_STATIC}  # 静态链接 zstd
    -lz             # 系统 zlib(动态)
    # 不链接 z3 和 xml2,编译器不需要它们
)

# macOS 特定设置
if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        "-framework CoreFoundation"
    )
    
    # 链接选项:抑制警告,优先使用静态库
    target_link_options(${PROJECT_NAME} PRIVATE 
        -Wl,-w
        -Wl,-search_paths_first
    )
endif()

# ============================================
# 编译完成后清理 runtime 中间文件
# ============================================
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f ${RUNTIME_OBJECT} ${RUNTIME_OBJECT_EMBEDDED} ${RUNTIME_SOURCE_EMBEDDED}
)
