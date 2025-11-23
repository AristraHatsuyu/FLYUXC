// 测试 ! 后缀异常处理

println("=== 测试 ! 后缀异常处理 ===\n")

// 测试1: 无 ! 时返回带类型 null
println("--- 测试1: 无 ! 返回带类型 null ---")
result1 := parseJSON("invalid")
println("result1 =", result1)
println("typeOf(result1) =", typeOf(result1))
println("程序继续执行")
println()

// 测试2: 有 ! 时应该抛出异常，被 try-catch 捕获
println("--- 测试2: 有 ! 被 try-catch 捕获 ---")
T> {
    result2 := parseJSON("invalid")!
    println("不应该执行到这里")
} (err) {
    println("捕获到错误:", err)
}
println("程序继续执行")
println()

// 测试3: 正常解析，无论有无 !
println("--- 测试3: 正常解析 ---")
result3 := parseJSON('{"a":1}')!
println("result3 =", result3)
println("typeOf(result3) =", typeOf(result3))
println()

println("=== 测试完成 ===")
