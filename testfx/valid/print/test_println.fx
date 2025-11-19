// 测试 println 和 printf 的修复

println("=== 测试 printf %.0f 格式化 ===")
println()

num1 := 150.789
printf("原始值: %f\n", num1)
printf("%%.0f 格式 (应该是 150): %.0f\n", num1)
printf("%%.1f 格式: %.1f\n", num1)
printf("%%.2f 格式: %.2f\n", num1)
println()

num2 := 42.0
printf("整数值 %%.0f 格式: %.0f\n", num2)
println()

num3 := 0.999
printf("接近1的值 %%.0f 格式: %.0f\n", num3)
println()

println("=== 测试 println() 无参数换行 ===")
println("第一行")
println()
println("第二行（上面应该有空行）")
println()
println()
println("第三行（上面应该有两个空行）")
println()

println("✓ 测试完成")
