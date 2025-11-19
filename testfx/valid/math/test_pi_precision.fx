println("=== 测试 Pi 精度 ===\n")

// 测试不同精度的 Pi 值
pi1 := 3.14159
pi2 := 3.1415926535897935
pi3 := 3.1415926535897936

println("1. 使用 print（智能格式化）：")
print(pi1)
println("")
print(pi2)
println("")
print(pi3)
println("")

println("\n2. 使用 printf %.16g（最大精度）：")
printf("pi1: %.16g\n", pi1)
printf("pi2: %.16g\n", pi2)
printf("pi3: %.16g\n", pi3)

println("\n3. 使用 printf %.2f 到 %.16f：")
printf("%.2f\n", pi2)
printf("%.4f\n", pi2)
printf("%.6f\n", pi2)
printf("%.8f\n", pi2)
printf("%.10f\n", pi2)
printf("%.12f\n", pi2)
printf("%.14f\n", pi2)
printf("%.16f\n", pi2)

println("\n4. 测试数学运算是否影响精度：")
pi_calc := 3.14159265359
printf("直接赋值: %.16g\n", pi_calc)
pi_expr := 3.0 + 0.14159265359
printf("表达式计算: %.16g\n", pi_expr)
