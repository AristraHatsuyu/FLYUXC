/* 简化数学测试 */

println("=== 基础整数运算 ===")
println(10 + 5)
println(10 - 5)
println(10 * 5)
println(10 / 5)
println(10 ** 2)

println("=== 小数运算 ===")
println(3.14 + 2.86)
println(10.5 - 3.2)
println(2.5 * 4.0)

println("=== 变量测试 ===")
x := 10
println(x)
y := 5
println(y)
println(x + y)
println(x - y)
println(x * y)

println("=== 变量更新 ===")
a := 10
println(a)
a = a + 5
println(a)
a = a * 2
println(a)

println("=== 数组运算 ===")
println([1 + 1, 2 * 2, 3 ** 2])

println("=== 对象运算 ===")
println({ sum: 10 + 5, prod: 10 * 5 })

println("测试完成")
