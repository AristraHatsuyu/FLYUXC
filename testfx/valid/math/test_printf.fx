/* 测试 printf 和特殊数值 */

println("=== 测试特殊数值 ===")
println(10 / 0)
println(-10 / 0)
println(0 / 0)

println("\n=== 测试 printf 基础功能 ===")
printf("Hello, %s!\n", "FLYUX")
printf("Number: %d\n", 42)
printf("Float: %f\n", 3.14159)
printf("Bool: %b\n", true)

println("\n=== 测试 printf 精度 ===")
printf("Pi with 2 decimals: %.2f\n", 3.14159265359)
printf("Pi with 6 decimals: %.6f\n", 3.14159265359)
printf("Pi with 10 decimals: %.10f\n", 3.14159265359)

println("\n=== 测试 printf 混合类型 ===")
x := 100
y := 3.14
name := "Alice"
printf("x=%d, y=%.2f, name=%s\n", x, y, name)

println("\n=== 测试 printf 值格式化 ===")
arr := [1, 2, 3]
person := { name: "Bob", age: 25 }
printf("Array: %v\n", arr)
printf("Object: %v\n", person)

println("\n=== 测试特殊数值的 printf ===")
inf_val := 10 / 0
neg_inf := -10 / 0
nan_val := 0 / 0
printf("Infinity: %f\n", inf_val)
printf("-Infinity: %f\n", neg_inf)
printf("NaN: %f\n", nan_val)

println("\n=== 测试极大数和极小数 ===")
big := 1234567890123456
small := 0.0000000000000001
printf("Big number: %.16g\n", big)
printf("Small number: %.16g\n", small)
printf("Big with %%f: %.16f\n", big)
printf("Small with %%f: %.16f\n", small)

println("\n✅ printf 测试完成")
