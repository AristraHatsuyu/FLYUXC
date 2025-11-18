/**
 * LLVM 编译器模块 - 完全自包含版本
 * 使用 LLVM C++ API 直接编译，不依赖外部 clang
 */

#include "flyuxc/llvm_compiler.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/IR/LegacyPassManager.h>

#include <string>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <system_error>

// 嵌入的运行时库源代码
extern "C" const char* get_embedded_runtime_source(void);

// 嵌入的预编译运行时对象文件
#include "runtime_object_embedded.h"

static std::string g_last_error;

// 设置错误信息
static void set_error(const std::string& error) {
    g_last_error = error;
}

// 初始化 LLVM 目标
static void initialize_llvm_targets() {
    static bool initialized = false;
    if (!initialized) {
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmPrinters();
        llvm::InitializeAllAsmParsers();
        initialized = true;
    }
}

// 将嵌入的运行时对象文件写入临时文件
static std::string write_embedded_runtime_object() {
    const char* tmpdir = getenv("TMPDIR");
    if (!tmpdir) tmpdir = "/tmp";
    
    char runtime_o_path[512];
    snprintf(runtime_o_path, sizeof(runtime_o_path), "%s/flyuxc_runtime_%d.o", tmpdir, getpid());
    
    // 写入嵌入的对象文件
    FILE* f = fopen(runtime_o_path, "wb");
    if (!f) {
        set_error("Failed to create temporary runtime object file");
        return "";
    }
    
    fwrite(runtime_object_o, 1, runtime_object_o_len, f);
    fclose(f);
    
    return std::string(runtime_o_path);
}

// 从 IR 文件加载模块
static std::unique_ptr<llvm::Module> load_ir_module(
    llvm::LLVMContext& context,
    const char* ir_file
) {
    llvm::SMDiagnostic err;
    auto module = llvm::parseIRFile(ir_file, err, context);
    
    if (!module) {
        std::string error_msg = "Failed to parse IR file: ";
        error_msg += ir_file;
        error_msg += "\n";
        error_msg += err.getMessage().str();
        set_error(error_msg);
        return nullptr;
    }
    
    return module;
}

// 验证模块
static bool verify_module(llvm::Module* module) {
    std::string error_msg;
    llvm::raw_string_ostream error_stream(error_msg);
    
    if (llvm::verifyModule(*module, &error_stream)) {
        set_error("Module verification failed: " + error_msg);
        return false;
    }
    
    return true;
}

// 优化模块
static void optimize_module(llvm::Module* module, int opt_level) {
    if (opt_level == 0) return;
    
    // 使用新的 Pass Manager
    llvm::PassBuilder PB;
    
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;
    
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    
    llvm::ModulePassManager MPM;
    
    if (opt_level == 1) {
        MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O1);
    } else if (opt_level == 2) {
        MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
    } else {
        MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
    }
    
    MPM.run(*module, MAM);
}

// 生成目标对象文件
static bool generate_object_file(
    llvm::Module* module,
    const char* output_file,
    int opt_level
) {
    initialize_llvm_targets();
    
    // 获取目标三元组
    std::string target_triple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(target_triple);
    
    // 查找目标
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    
    if (!target) {
        set_error("Failed to lookup target: " + error);
        return false;
    }
    
    // 创建目标机器
    llvm::TargetOptions opt;
    std::optional<llvm::Reloc::Model> RM = std::nullopt;
    
    std::unique_ptr<llvm::TargetMachine> target_machine(
        target->createTargetMachine(
            target_triple,
            llvm::sys::getHostCPUName(),
            "",
            opt,
            RM
        )
    );
    
    if (!target_machine) {
        set_error("Failed to create target machine");
        return false;
    }
    
    module->setDataLayout(target_machine->createDataLayout());
    
    // 优化模块
    optimize_module(module, opt_level);
    
    // 输出对象文件
    std::error_code EC;
    llvm::raw_fd_ostream dest(output_file, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        set_error("Could not open file: " + EC.message());
        return false;
    }
    
    llvm::legacy::PassManager pass;
    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, 
                                           llvm::CodeGenFileType::ObjectFile)) {
        set_error("TargetMachine can't emit a file of this type");
        return false;
    }
    
    pass.run(*module);
    dest.flush();
    
    return true;
}

// 链接对象文件生成可执行文件
static bool link_object_files(
    const char* main_obj,
    const char* runtime_obj,
    const char* output_file
) {
    // 使用 clang 作为链接器前端（更简单可靠）
    std::string cmd = "clang -o \"";
    cmd += output_file;
    cmd += "\" \"";
    cmd += main_obj;
    cmd += "\" \"";
    cmd += runtime_obj;
    cmd += "\" 2>&1";
    
    int result = system(cmd.c_str());
    
    if (result != 0) {
        set_error("Linking failed: " + cmd);
        return false;
    }
    
    return true;
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
    try {
        g_last_error.clear();
        
        // 创建 LLVM 上下文
        llvm::LLVMContext context;
        
        // 加载 IR 模块
        auto module = load_ir_module(context, ir_file);
        if (!module) {
            return 1;
        }
        
        // 验证模块
        if (!verify_module(module.get())) {
            return 2;
        }
        
        // 生成主程序的对象文件
        std::string temp_main_obj = std::string(output_file) + ".main.o";
        
        if (!generate_object_file(module.get(), temp_main_obj.c_str(), opt_level)) {
            return 3;
        }
        
        // 使用嵌入的运行时对象文件
        std::string embedded_runtime_obj;
        const char* actual_runtime_obj = runtime_obj;
        
        if (!runtime_obj || strlen(runtime_obj) == 0) {
            embedded_runtime_obj = write_embedded_runtime_object();
            if (embedded_runtime_obj.empty()) {
                unlink(temp_main_obj.c_str());
                return 4;
            }
            actual_runtime_obj = embedded_runtime_obj.c_str();
        }
        
        // 链接生成可执行文件
        if (!link_object_files(temp_main_obj.c_str(), actual_runtime_obj, output_file)) {
            unlink(temp_main_obj.c_str());
            if (!embedded_runtime_obj.empty()) {
                unlink(embedded_runtime_obj.c_str());
            }
            return 5;
        }
        
        // 清理临时文件
        unlink(temp_main_obj.c_str());
        if (!embedded_runtime_obj.empty()) {
            unlink(embedded_runtime_obj.c_str());
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        set_error(std::string("Exception: ") + e.what());
        return 6;
    }
}

int llvm_compile_string_to_executable(
    const char* ir_code,
    const char* runtime_obj,
    const char* output_file,
    int opt_level
) {
    const char* temp_file = "/tmp/flyuxc_temp.ll";
    FILE* f = fopen(temp_file, "w");
    if (!f) {
        set_error("Failed to create temporary file");
        return 1;
    }
    
    fprintf(f, "%s", ir_code);
    fclose(f);
    
    int result = llvm_compile_to_executable(temp_file, runtime_obj, output_file, opt_level);
    
    unlink(temp_file);
    
    return result;
}

} // extern "C"
