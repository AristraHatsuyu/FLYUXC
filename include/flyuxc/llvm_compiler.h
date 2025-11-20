#ifndef FLYUXC_LLVM_COMPILER_H
#define FLYUXC_LLVM_COMPILER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 编译 LLVM IR 到可执行文件
 * 
 * @param ir_file       LLVM IR 文件路径 (.ll)
 * @param runtime_obj   运行时库对象文件路径 (.o)
 * @param output_file   输出可执行文件路径
 * @param opt_level     优化级别 (0-3)
 * @return 0 表示成功，非 0 表示失败
 */
int llvm_compile_to_executable(
    const char *ir_file,
    const char *runtime_obj,
    const char *output_file,
    int opt_level
);

/**
 * 编译 LLVM IR 字符串到可执行文件
 * 
 * @param ir_code       LLVM IR 代码字符串
 * @param runtime_obj   运行时库对象文件路径 (.o)
 * @param output_file   输出可执行文件路径
 * @param opt_level     优化级别 (0-3)
 * @return 0 表示成功，非 0 表示失败
 */
int llvm_compile_string_to_executable(
    const char *ir_code,
    const char *runtime_obj,
    const char *output_file,
    int opt_level
);

/**
 * 获取最后的错误信息
 * 
 * @return 错误信息字符串
 */
const char* llvm_get_last_error(void);

/**
 * 提前初始化 LLVM （可选，用于减少首次编译的启动延迟）
 */
void llvm_initialize(void);

#ifdef __cplusplus
}
#endif

#endif // FLYUXC_LLVM_COMPILER_H
