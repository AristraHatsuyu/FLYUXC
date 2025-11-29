// 测试第一批新增内置函数
// 包括: abs, floor, ceil, round, sqrt, pow, min, max, random, startsWith, endsWith, contains

println("========== 数学函数测试 ==========")

// 1. abs - 绝对值
println("1. abs():")
println(abs(-5))        // 应该输出: 5
println(abs(3.14))      // 应该输出: 3.14
println(abs(-0))        // 应该输出: 0

// 2. floor - 向下取整
println("\n2. floor():")
println(floor(3.7))     // 应该输出: 3
println(floor(-2.3))    // 应该输出: -3
println(floor(5))       // 应该输出: 5

// 3. ceil - 向上取整
println("\n3. ceil():")
println(ceil(3.2))      // 应该输出: 4
println(ceil(-2.7))     // 应该输出: -2
println(ceil(5))        // 应该输出: 5

// 4. round - 四舍五入
println("\n4. round():")
println(round(3.4))     // 应该输出: 3
println(round(3.5))     // 应该输出: 4
println(round(-2.6))    // 应该输出: -3

// 5. sqrt - 平方根
println("\n5. sqrt():")
println(sqrt(16))       // 应该输出: 4
println(sqrt(2))        // 应该输出: 1.414...
println(sqrt(0))        // 应该输出: 0

// 6. pow - 幂运算
println("\n6. pow():")
println(pow(2, 3))      // 应该输出: 8
println(pow(5, 0))      // 应该输出: 1
println(pow(2, -1))     // 应该输出: 0.5

// 7. min - 最小值
println("\n7. min():")
println(min(3, 7))      // 应该输出: 3
println(min(-5, -2))    // 应该输出: -5
println(min(4.5, 4.5))  // 应该输出: 4.5

// 8. max - 最大值
println("\n8. max():")
println(max(3, 7))      // 应该输出: 7
println(max(-5, -2))    // 应该输出: -2
println(max(4.5, 4.5))  // 应该输出: 4.5

// 9. random - 随机数
println("\n9. random():")
r1 := random()
r2 := random()
r3 := random()
println(r1)             // 应该输出: 0到1之间
println(r2)             // 应该输出: 0到1之间
println(r3)             // 应该输出: 0到1之间
println(r1 >= 0 && r1 < 1)  // 应该输出: true

println("\n========== 字符串函数测试 ==========")

// 10. startsWith - 判断开头
println("\n10. startsWith():")
println(startsWith("hello", "he"))      // 应该输出: true
println(startsWith("hello", "lo"))      // 应该输出: false
println(startsWith("hello", "hello"))   // 应该输出: true
println(startsWith("hello", ""))        // 应该输出: true

// 11. endsWith - 判断结尾
println("\n11. endsWith():")
println(endsWith("hello", "lo"))        // 应该输出: true
println(endsWith("hello", "he"))        // 应该输出: false
println(endsWith("hello", "hello"))     // 应该输出: true
println(endsWith("hello", ""))          // 应该输出: true

// 12. contains - 包含判断
println("\n12. contains():")
println(contains("hello world", "wo"))  // 应该输出: true
println(contains("hello world", "xy"))  // 应该输出: false
println(contains("hello world", ""))    // 应该输出: true
println(contains("abc", "abcd"))        // 应该输出: false

println("\n========== 错误处理测试 ==========")

// 测试类型错误 (不加!后缀 - 应返回 typed null)
println("\n测试类型错误 (无!):")
result1 := abs("not a number")
println(result1)  // 应该输出: null

result2 := floor(true)
println(result2)  // 应该输出: null

result3 := pow(1, "not a number")
println(result3)  // 应该输出: null

result4 := startsWith(123, "test")
println(result4)  // 应该输出: false

// 测试数学错误 - sqrt负数
println("\n测试数学错误 (sqrt负数):")
neg_sqrt := sqrt(-1)
println(neg_sqrt)  // 应该输出: null

// 测试Try-Catch错误处理
println("\n测试Try-Catch (带!):")
T> {
    bad_abs := abs("string")!  // 应该抛出错误被捕获
    println("不应该执行到这里")
} (e) {
    println("Caught error!")
    println(e.name)
    println(e.message)
}

println("\n所有测试完成!")
