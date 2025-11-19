println("=== 测试换行符 ===")

// 测试 printf 中的 \n
printf("Line 1\n")
printf("Line 2\n")
printf("Line 3\n")

// 测试 %s 中的 null
printf("Null as %%v: %v\n", null)
printf("Null as %%s: %s\n", null)
printf("Null as %%d: %d\n", null)

// 测试数字转字符串
val := 42.5
printf("Number as %%s: %s\n", val)

// 测试表格
println("\n=== 表格测试 ===")
printf("%-10s | %5s | %8s\n", "Name", "Age", "Score")
printf("%-10s | %5d | %8.2f\n", "Alice", 25, 95.5)
printf("%-10s | %5d | %8.2f\n", "Bob", 30, 87.3)
