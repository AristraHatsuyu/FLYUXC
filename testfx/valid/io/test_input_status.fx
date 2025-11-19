// FLYUX input() 和状态系统完整测试

println("╔════════════════════════════════════════╗")
println("║   FLYUX Input & Status System 测试    ║")
println("╚════════════════════════════════════════╝")
println()

// 测试 1: 基本输入
println("=== 测试 1: 基本输入 ===")
name := input("请输入姓名: ")
println("你好, ", name, "!")
println()

// 测试 2: 状态检查
println("=== 测试 2: 状态检查 ===")
println("输入操作后的状态码: ", lastStatus())
println("是否成功: ", isOk())
println()

// 测试 3: 无提示符输入
println("=== 测试 3: 无提示符 ===")
data := input()
println("你输入了: [", data, "]")
println()

// 测试 4: 状态清除
println("=== 测试 4: 状态管理 ===")
println("当前状态: ", lastStatus())
println("当前错误: ", lastError())
clearError()
println("清除后状态: ", lastStatus())
println()

println("✓ 所有测试完成!")
println()
println("提示: 状态系统可用于:")
println("  - 检测输入是否成功")
println("  - 处理 EOF (Ctrl+D)")
println("  - 未来的字符串转换、文件操作等")
