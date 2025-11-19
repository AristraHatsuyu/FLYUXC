/* 
 * FLYUX 高精度计算完整测试
 * 测试范围：基础运算、复杂表达式、边界值、精度保持
 */

println("╔════════════════════════════════════════════════════════════╗")
println("║        FLYUX 高精度数值计算完整测试套件                   ║")
println("╚════════════════════════════════════════════════════════════╝\n")

// ============ 第一部分：基础算术运算 ============
println("=== 1. 基础算术运算精度测试 ===\n")

a := 0.1
b := 0.2
c := a + b
println("测试 1.1：浮点数加法")
printf("  0.1 + 0.2 = %.16f\n", c)
printf("  print输出: ")
print(c)
println("")

d := 1.0 / 3.0
println("\n测试 1.2：除法精度")
printf("  1.0 / 3.0 = %.16f\n", d)
printf("  print输出: ")
print(d)
println("")

e := 2.0 / 3.0
println("\n测试 1.3：另一个除法")
printf("  2.0 / 3.0 = %.16f\n", e)
printf("  print输出: ")
print(e)
println("")

// ============ 第二部分：复杂表达式 ============
println("\n=== 2. 复杂表达式精度测试 ===\n")

pi := 3.141592653589793
println("测试 2.1：圆周率相关计算")
printf("  Pi = %.15f\n", pi)
radius := 5.0
area := pi * radius * radius
printf("  半径 = %.1f\n", radius)
printf("  面积 = Pi * r^2 = %.12f\n", area)
circumference := 2.0 * pi * radius
printf("  周长 = 2 * Pi * r = %.12f\n", circumference)

println("\n测试 2.2：嵌套计算")
x := 1.5
y := 2.3
z := 3.7
temp1 := x + y
temp2 := temp1 * z
temp3 := x * y
temp4 := temp3 / z
result1 := temp2 - temp4
printf("  (%.1f + %.1f) * %.1f - (%.1f * %.1f) / %.1f = %.12f\n", x, y, z, x, y, z, result1)

// ============ 第三部分：幂运算 ============
println("\n=== 3. 幂运算精度测试 ===\n")

println("测试 3.1：小数幂运算")
base1 := 1.5
exp1 := 2.0
power1 := base1 ** exp1
printf("  1.5^2 = %.12f\n", power1)

base2 := 2.0
exp2 := 10.0
power2 := base2 ** exp2
printf("  2^10 = %.0f\n", power2)

base3 := 0.5
exp3 := 3.0
power3 := base3 ** exp3
printf("  0.5^3 = %.12f\n", power3)

// ============ 第四部分：连续运算累积误差测试 ============
println("\n=== 4. 连续运算累积误差测试 ===\n")

println("测试 4.1：重复加法")
sum := 0.0
L> (i := 0; i < 10; i = i + 1) {
    sum = sum + 0.1
}
printf("  0.1 加 10 次 = %.16f\n", sum)
printf("  理论值: 1.0, 误差: %.16e\n", sum - 1.0)

println("\n测试 4.2：重复乘法")
product := 1.0
L> (j := 0; j < 10; j = j + 1) {
    product = product * 1.1
}
printf("  1.1 连乘 10 次 = %.12f\n", product)
expected := 2.5937424601
printf("  理论值: %.10f, 相对误差: %.2e\n", expected, (product - expected) / expected)

// ============ 第五部分：三角函数近似（泰勒级数）============
println("\n=== 5. 数学常数精度测试 ===\n")

e := 2.718281828459045
println("测试 5.1：欧拉数")
printf("  e = %.15f\n", e)

golden := 1.618033988749895
println("\n测试 5.2：黄金比例")
printf("  φ = %.15f\n", golden)

sqrt2 := 1.414213562373095
println("\n测试 5.3：√2")
printf("  √2 = %.15f\n", sqrt2)

// ============ 第六部分：边界值测试 ============
println("\n=== 6. 边界值与特殊值测试 ===\n")

println("测试 6.1：极小数")
tiny1 := 0.0000000000000001
tiny2 := 0.00000000000000001
printf("  1e-16 = %.17e\n", tiny1)
printf("  1e-17 = %.17e\n", tiny2)
printf("  它们的和 = %.17e\n", tiny1 + tiny2)

println("\n测试 6.2：极大数")
huge1 := 1234567890123456.0
huge2 := 9876543210987654.0
printf("  大数1 = %.0f\n", huge1)
printf("  大数2 = %.0f\n", huge2)
printf("  它们的和 = %.0f\n", huge1 + huge2)

println("\n测试 6.3：特殊值运算")
inf := 10.0 / 0.0
neg_inf := -10.0 / 0.0
nan := 0.0 / 0.0
printf("  10/0 = %s\n", "+Inf")
print(inf)
println("")
printf("  -10/0 = %s\n", "-Inf")
print(neg_inf)
println("")
printf("  0/0 = %s\n", "NaN")
print(nan)
println("")

// ============ 第七部分：精度截断测试 ============
println("\n=== 7. 精度截断测试（不应四舍五入）===\n")

println("测试 7.1：16位小数精度")
num1 := 0.1234567890123456
printf("  原数: 0.1234567890123456\n")
printf("  %.16f (应截断最后一位): %.16f\n", num1, num1)
printf("  %.15f (应截断): %.15f\n", num1, num1)
printf("  print输出: ")
print(num1)
println("")

println("\n测试 7.2：最后一位是9的情况")
num2 := 0.1234567890123459
printf("  原数: 0.1234567890123459\n")
printf("  %.15f (应截断9): %.15f\n", num2, num2)
printf("  print输出: ")
print(num2)
println("")

println("\n测试 7.3：接近1的数")
num3 := 0.9999999999999999
printf("  原数: 0.9999999999999999\n")
printf("  print输出（智能判断）: ")
print(num3)
println("")

// ============ 第八部分：实际应用场景 ============
println("\n=== 8. 实际应用场景测试 ===\n")

println("测试 8.1：金融计算（货币精度）")
price := 19.99
quantity := 3.0
subtotal := price * quantity
tax_rate := 0.08
tax := subtotal * tax_rate
total := subtotal + tax
printf("  单价: $%.2f\n", price)
printf("  数量: %.0f\n", quantity)
printf("  小计: $%.2f\n", subtotal)
printf("  税率: %.0f%%\n", tax_rate * 100.0)
printf("  税额: $%.2f\n", tax)
printf("  总计: $%.2f\n", total)

println("\n测试 8.2：物理计算（速度、距离、时间）")
distance := 123.456
time := 5.5
speed := distance / time
printf("  距离: %.3f km\n", distance)
printf("  时间: %.1f h\n", time)
printf("  速度: %.6f km/h\n", speed)

println("\n测试 8.3：统计计算（平均值）")
data := [98.5, 87.3, 92.1, 88.9, 95.7]
sum_data := data[0] + data[1] + data[2] + data[3] + data[4]
count := 5.0
avg := sum_data / count
printf("  数据: [%.1f, %.1f, %.1f, %.1f, %.1f]\n", 
    data[0], data[1], data[2], data[3], data[4])
printf("  总和: %.1f\n", sum_data)
printf("  平均值: %.6f\n", avg)

// ============ 第九部分：复杂嵌套计算 ============
println("\n=== 9. 复杂嵌套计算测试 ===\n")

println("测试 9.1：二次方程求根公式")
a_coef := 1.0
b_coef := -5.0
c_coef := 6.0
discriminant := b_coef * b_coef - 4.0 * a_coef * c_coef
printf("  方程: %.0fx² + %.0fx + %.0f = 0\n", a_coef, b_coef, c_coef)
printf("  判别式: b² - 4ac = %.6f\n", discriminant)

println("\n测试 9.2：复合利息计算")
principal := 10000.0
rate := 0.05
years := 10.0
amount := principal * (1.0 + rate) ** years
printf("  本金: $%.2f\n", principal)
printf("  年利率: %.0f%%\n", rate * 100.0)
printf("  年数: %.0f\n", years)
printf("  最终金额: $%.2f\n", amount)
interest := amount - principal
printf("  利息: $%.2f\n", interest)

// ============ 测试总结 ============
println("\n╔════════════════════════════════════════════════════════════╗")
println("║                     测试完成！                             ║")
println("╠════════════════════════════════════════════════════════════╣")
println("║  测试覆盖：                                                ║")
println("║    ✓ 基础算术运算（加减乘除）                             ║")
println("║    ✓ 复杂表达式计算                                       ║")
println("║    ✓ 幂运算                                                ║")
println("║    ✓ 连续运算累积误差                                     ║")
println("║    ✓ 数学常数精度                                         ║")
println("║    ✓ 边界值与特殊值                                       ║")
println("║    ✓ 精度截断（非四舍五入）                               ║")
println("║    ✓ 实际应用场景                                         ║")
println("║    ✓ 复杂嵌套计算                                         ║")
println("╚════════════════════════════════════════════════════════════╝")
