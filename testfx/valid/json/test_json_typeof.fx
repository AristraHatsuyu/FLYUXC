// 测试 parseJSON 返回值类型

println("=== 测试 JSON parseJSON 返回值类型 ===\n")

// 测试1: 正常解析
println("--- 正常解析 ---")
result1 := parseJSON('{"a":1}')
println("result1 =", result1)
println("typeOf(result1) =", typeOf(result1))
println()

// 测试2: 解析失败
println("--- 解析失败 ---")
result2 := parseJSON("invalid json")
println("result2 =", result2)
println("typeOf(result2) =", typeOf(result2))
println()

// 测试3: 不完整的 JSON
println("--- 不完整的 JSON ---")
result3 := parseJSON('{"incomplete":')
println("result3 =", result3)
println("typeOf(result3) =", typeOf(result3))
println()

println("=== 测试完成 ===")
