// 简化的新函数测试 - 不使用Try-Catch

println("========== 数学函数测试 ==========")

println("abs(-5):")
println(abs(-5))        // 5

println("floor(3.7):")
println(floor(3.7))     // 3

println("ceil(3.2):")
println(ceil(3.2))      // 4

println("round(3.5):")
println(round(3.5))     // 4

println("sqrt(16):")
println(sqrt(16))       // 4

println("pow(2, 3):")
println(pow(2, 3))      // 8

println("min(3, 7):")
println(min(3, 7))      // 3

println("max(3, 7):")
println(max(3, 7))      // 7

println("random():")
r := random()
println(r)              // 0到1之间
println(r >= 0 && r < 1)  // true

println("\n========== 字符串函数测试 ==========")

println("startsWith('hello', 'he'):")
println(startsWith("hello", "he"))      // true

println("endsWith('hello', 'lo'):")
println(endsWith("hello", "lo"))        // true

println("contains('hello world', 'wo'):")
println(contains("hello world", "wo"))  // true

println("\n========== 错误处理测试 ==========")

println("abs('string') - 应返回null:")
println(abs("string"))

println("sqrt(-1) - 应返回null:")
println(sqrt(-1))

println("\n所有测试完成!")
