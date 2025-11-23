// 测试错误处理场景

println("========== 测试类型错误 (无!) ==========")

println("abs('string') - 应返回null:")
result1 := abs("string")
println(result1)

println("floor(true) - 应返回null:")
result2 := floor(true)
println(result2)

println("sqrt(-1) - 应返回null (数学错误):")
result3 := sqrt(-1)
println(result3)

println("pow(1, 'bad') - 应返回null:")
result4 := pow(1, "bad")
println(result4)

println("startsWith(123, 'test') - 应返回false:")
result5 := startsWith(123, "test")
println(result5)

println("\n========== 测试Try-Catch (带!) ==========")

T> {
    println("尝试调用 abs('string')!...")
    bad_abs := abs("string")!
    println("不应该执行到这里!")
} (e) {
    println("✓ 错误被捕获!")
    println(e)
    println(e.name)
    println(e.message)
}

println("\n========== 测试正常情况 ==========")

println("abs(-10):")
println(abs(-10))

println("sqrt(25):")
println(sqrt(25))

println("startsWith('world', 'wo'):")
println(startsWith("world", "wo"))

bad_aabs := abs("string")!

println("\n所有测试完成!")
