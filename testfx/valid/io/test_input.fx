// 测试 input() 函数和状态系统

println("╔════════════════════════════════════════╗")
println("║     FLYUX Input 函数测试               ║")
println("╚════════════════════════════════════════╝")
println()

// 测试 1: 基本输入
println("=== 测试 1: 基本输入 ===")
name := input("请输入你的名字: ")
println("你好, ", name, "!")
println()

// 测试 2: 检查状态
println("=== 测试 2: 状态检查 ===")
age := input("请输入年龄: ")

if (isOk()) {
    println("输入成功! 年龄: ", age)
    println("状态码: ", lastStatus())
} {
    println("输入失败!")
    println("状态码: ", lastStatus())
    println("错误信息: ", lastError())
}
println()

// 测试 3: 多次输入
println("=== 测试 3: 多次输入 ===")
city := input("城市: ")
country := input("国家: ")
println("你来自 ", country, " 的 ", city)
println()

// 测试 4: 空提示符
println("=== 测试 4: 无提示符输入 ===")
println("请随便输入点什么:")
data := input()
println("你输入了: [", data, "]")
println()

// 测试 5: 状态系统
println("=== 测试 5: 状态清除 ===")
println("当前状态: ", lastStatus())
clearError()
println("清除后状态: ", lastStatus())
println()

println("✓ 测试完成")
