// 测试第一和第二部分

println("╔════════════════════════════════════════════════════════════╗")
println("║        FLYUX 高精度数值计算测试 - Part 1                  ║")
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
