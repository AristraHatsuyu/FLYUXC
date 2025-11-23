// 完整测试 ! 后缀的4种情况

println("=== ! 后缀完整测试 ===\n")

// 情况1: 无 T>，无 ! - 返回带类型的null，程序继续
println("--- 情况1: 无 T>，无 ! ---")
result1 := toNum("abc")
println("result1 =", result1, ", 类型:", typeOf(result1))
println("程序继续执行\n")

// 情况2: 无 T>，有 ! - 应该直接终止运行并报错
println("--- 情况2: 无 T>，有 ! (应该直接终止) ---")
result2 := toNum("xyz")!
println("如果看到这行，说明没有终止（错误！）")
println("result2 =", result2)
