/* FLYUX 数学运算系统完整测试 */

println("=== 1. 基础整数运算 ===")
println(10 + 5)           // 15
println(10 - 5)           // 5
println(10 * 5)           // 50
println(10 / 5)           // 2
println(10 ** 2)          // 100

println("=== 2. 小数运算 ===")
println(3.14 + 2.86)      // 6
println(10.5 - 3.2)       // 7.3
println(2.5 * 4.0)        // 10
println(7.5 / 2.5)        // 3
println(2.5 ** 2.0)       // 6.25

println("=== 3. 整数与小数混合 ===")
println(10 + 3.5)         // 13.5
println(10.5 - 3)         // 7.5
println(5 * 2.5)          // 12.5
println(10 / 4.0)         // 2.5
println(2 ** 3.5)         // 11.313...

println("=== 4. 负数运算 ===")
println(-10 + 5)          // -5
println(-10 - 5)          // -15
println(-10 * 5)          // -50
println(-10 / 5)          // -2
println(-2 ** 2)          // 4

println("=== 5. 负数与负数 ===")
neg5 := -5
neg2 := -2
println(-10 + neg5)       // -15
println(-10 - neg5)       // -5
println(-10 * neg5)       // 50
println(-10 / neg2)       // 5
println(neg2 ** neg2)     // 0.25

println("=== 6. 零的运算 ===")
println(0 + 10)           // 10
println(0 - 10)           // -10
println(0 * 10)           // 0
println(0 / 10)           // 0
println(0 ** 2)           // 0

println("=== 7. 除以零 ===")
println(10 / 0)           // Infinity
println(-10 / 0)          // -Infinity
println(0 / 0)            // NaN

println("=== 8. 幂运算特殊情况 ===")
println(2 ** 0)           // 1
println(2 ** 1)           // 2
println(0 ** 0)           // 1
println(1 ** 100)         // 1
println(10 ** -2)         // 0.01

println("=== 9. 大数运算 ===")
println(999999 + 1)       // 1000000
println(1000000 - 1)      // 999999
println(9999 * 9999)      // 99980001
println(1000000 / 100)    // 10000
println(10 ** 6)          // 1000000

println("=== 10. 小数精度 ===")
println(0.1 + 0.2)        // 0.3
println(0.3 - 0.1)        // 0.2
println(0.1 * 3)          // 0.3
println(1.0 / 3.0)        // 0.333...
println(0.1 ** 2)         // 0.01

println("=== 11. 复杂表达式 ===")
println(2 + 3 * 4)        // 14
println((2 + 3) * 4)      // 20
println(10 - 2 * 3)       // 4
println((10 - 2) * 3)     // 24
println(2 ** 3 ** 2)      // 512

println("=== 12. 嵌套括号 ===")
println(((2 + 3) * 4) - 5)        // 15
println(2 * (3 + (4 * 5)))        // 46
println(((10 / 2) + 3) * 2)       // 16
println((2 ** (3 + 1)) / 2)       // 8

println("=== 13. 长表达式链 ===")
println(1 + 2 + 3 + 4 + 5)                // 15
println(100 - 10 - 5 - 3 - 2)             // 80
println(2 * 3 * 4 * 5)                    // 120
println(1000 / 10 / 5 / 2)                // 10
println(2 ** 2 ** 2)                      // 16

println("=== 14. 混合运算符优先级 ===")
println(2 + 3 * 4 - 5)            // 9
println(10 / 2 + 3 * 4)           // 17
println(2 ** 3 + 4 * 5)           // 28
println((2 + 3) * (4 + 5))        // 45
println(2 * 3 + 4 * 5 - 6 / 2)    // 23

println("=== 15. 变量运算 ===")
x := 10
y := 5
println(x + y)            // 15
println(x - y)            // 5
println(x * y)            // 50
println(x / y)            // 2
println(x ** y)           // 100000

println("=== 16. 变量更新运算 ===")
a := 10
println(a)                // 10
a = a + 5
println(a)                // 15
a = a * 2
println(a)                // 30
a = a / 3
println(a)                // 10
a = a ** 2
println(a)                // 100

println("=== 17. 多变量复杂运算 ===")
p := 2
q := 3
r := 4
println(p + q + r)                        // 9
println(p * q * r)                        // 24
println((p + q) * r)                      // 20
println(p ** q ** r)                      // 大数
println((p + q) * (r - p))                // 10

println("=== 18. 小数变量运算 ===")
m := 3.14
n := 2.86
println(m + n)            // 6.0
println(m - n)            // 0.28
println(m * n)            // 8.9804
println(m / n)            // 1.0979...
println(m ** 2)           // 9.8596

println("=== 19. 混合类型变量 ===")
i := 10
f := 2.5
println(i + f)            // 12.5
println(i - f)            // 7.5
println(i * f)            // 25.0
println(i / f)            // 4.0
println(i ** f)           // 316.227...

println("=== 20. 运算结果再运算 ===")
result1 := 10 + 5
println(result1)          // 15
result2 := result1 * 2
println(result2)          // 30
result3 := result2 ** 2
println(result3)          // 900
result4 := result3 / 9
println(result4)          // 100

println("=== 21. 表达式作为函数参数 ===")
println(10 + 20)          // 30
println((5 * 4) + 10)     // 30
println(2 ** (3 + 2))     // 32
println((100 / 5) * 2)    // 40

println("=== 22. 非常大的幂运算 ===")
println(2 ** 10)          // 1024
println(2 ** 20)          // 1048576
println(10 ** 3)          // 1000
println(10 ** 6)          // 1000000
println(3 ** 10)          // 59049

println("=== 23. 非常小的数 ===")
println(0.001 + 0.002)    // 0.003
println(0.1 - 0.09)       // 0.01
println(0.01 * 0.01)      // 0.0001
println(1 / 1000)         // 0.001
println(0.1 ** 3)         // 0.001

println("=== 24. 科学计数法级别 ===")
println(10 ** 9)          // 1000000000
println(10 ** -3)         // 0.001
println(2 ** 30)          // 1073741824
println(1 / (10 ** 6))    // 0.000001

println("=== 25. 边界值测试 ===")
println(1.7976931348623157 ** 2)  // 大数
println(0.0000000001 * 0.0000000001)  // 极小数
println(999999999 + 1)    // 1000000000
println(1 - 0.9999999)    // 接近0

println("=== 26. 运算符结合性测试 ===")
println(10 - 5 - 2)       // 3 (左结合)
println(100 / 10 / 2)     // 5 (左结合)
println(2 ** 3 ** 2)      // 512 (右结合)

println("=== 27. 复杂嵌套表达式 ===")
println(((10 + 5) * (20 - 10)) / ((3 + 2) * 2))  // 15
println((2 ** (3 + 1)) * (5 - 2) + (10 / 2))     // 53
println(((100 / 4) + (50 / 2)) * 2 - 10)         // 90

println("=== 28. 数组中的运算 ===")
println([1 + 1, 2 * 2, 3 ** 2, 10 / 2, 15 - 5])
println([10 + 5, 10 - 5, 10 * 5, 10 / 5])

println("=== 29. 对象中的运算 ===")
println({ sum: 10 + 5, diff: 10 - 5, prod: 10 * 5 })
println({ a: 2 ** 3, b: 100 / 4, c: 5 * (3 + 2) })

println("=== 30. 终极压力测试 ===")
mega := ((((2 ** 3) * 5) + (100 / 4)) - (30 - 15)) * 2  // 100
println(mega)

println({ 
    simple: 1 + 1,
    complex: ((10 + 5) * 2) ** 2,
    nested: [2 + 2, 3 * 3, { inner: 4 ** 2 }],
    chain: (((1 + 2) * 3) + 4) * 5
})

println("✅ 数学运算测试完成！")
