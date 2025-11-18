/**
 * LLVM 编译器模块 - 自包含版本
 * 内嵌运行时库，只依赖系统 clang
 */

#include "flyuxc/llvm_compiler.h"
#include <string>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>

// 嵌入的运行时库源代码
extern "C" const char* get_embedded_runtime_source(void);

static std::string g_last_error;

// 编译嵌入的运行时库到临时对象文件
static std::string compile_embedded_runtime() {
    // 创建临时目录
    const char* tmpdir = getenv("TMPDIR");
    if (!tmpdir) tmpdir = "/tmp";
    
    char runtime_c_path[512];
    char runtime_o_path[512];
    snprintf(runtime_c_path, sizeof(runtime_c_path), "%s/flyuxc_runtime_%d.c", tmpdir, getpid());
    snprintf(runtime_o_path, sizeof(runtime_o_path), "%s/flyuxc_runtime_%d.o", tmpdir, getpid());
    
    // 写入运行时源代码
    FILE* f = fopen(runtime_c_path, "w");
    if (!f) {
        g_last_error = "Failed to create temporary runtime source file";
        return "";
    }
    
    const char* runtime_src = get_embedded_runtime_source();
    fprintf(f, "%s", runtime_src);
    fclose(f);
    
    // 编译运行时库
    char compile_cmd[1024];
    snprintf(compile_cmd, sizeof(compile_cmd), 
             "clang -c \"%s\" -o \"%s\" 2>&1", 
             runtime_c_path, runtime_o_path);
    
    int result = system(compile_cmd);
    
    // 删除临时源文件
    unlink(runtime_c_path);
    
    if (result != 0) {
        g_last_error = "Failed to compile embedded runtime library";
        return "";
    }
    
    return std::string(runtime_o_path);
}

extern "C" {

const char* llvm_get_last_error(void) {
    return g_last_error.c_str();
}

int llvm_compile_to_executable(
    const char* ir_file,
    const char* runtime_obj,
    const char* output_file,
    int opt_level
) {
    // 如果没有提供运行时库，使用嵌入的版本
    std::string embedded_runtime_obj;
    const char* actual_runtime_obj = runtime_obj;
    
    if (!runtime_obj || strlen(runtime_obj) == 0) {
        embedded_runtime_obj = compile_embedded_runtime();
        if (embedded_runtime_obj.empty()) {
            return 1;
        }
        actual_runtime_obj = embedded_runtime_obj.c_str();
    }
    
    // 构建编译命令
    std::string cmd = "clang ";
    
    // 添加优化级别
    if (opt_level > 0) {
        cmd += "-O";
        cmd += std::to_string(opt_level);
        cmd += " ";
    }
    
    // 添加输入文件
    cmd += "\"";
    cmd += ir_file;
    cmd += "\" \"";
    cmd += actual_runtime_obj;
    cmd += "\" -o \"";
    cmd += output_file;
    cmd += "\" 2>&1";
    
    // 执行编译
    int result = system(cmd.c_str());
    
    // 清理嵌入的运行时对象文件
    if (!embedded_runtime_obj.empty()) {
        unlink(embedded_runtime_obj.c_str());
    }
    
    if (result != 0) {
        g_last_error = "Compilation command failed: " + cmd;
        return 1;
    }
    
    return 0;
}

int llvm_compile_string_to_executable(
    const char* ir_code,
    const char* runtime_obj,
    const char* output_file,
    int opt_level
) {
    // 将 IR 代码写入临时文件
    const char* temp_file = "/tmp/flyuxc_temp.ll";
    FILE* f = fopen(temp_file, "w");
    if (!f) {
        g_last_error = "Failed to create temporary file";
        return 1;
    }
    
    fprintf(f, "%s", ir_code);
    fclose(f);
    
    // 调用文件编译函数
    int result = llvm_compile_to_executable(temp_file, runtime_obj, output_file, opt_level);
    
    // 删除临时文件
    remove(temp_file);
    
    return result;
}

} // extern "C"
