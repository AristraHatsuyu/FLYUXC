println("╔════════════════════════════════════════╗")
println("║     FLYUX 三种循环形态测试             ║")
println("╚════════════════════════════════════════╝\n")

// ============ 测试1：重复循环 L> [n] { } ============
println("=== 测试 1：重复循环 L> [n] { } ===\n")

println("测试 1.1：固定次数循环")
count := 0
L> [5] {
    count = count + 1
    printf("  执行第 %d 次\n", count)
}
printf("总共执行了 %.0f 次\n\n", count)

println("测试 1.2：使用变量指定次数")
times := 3.0
L> [times] {
    println("  重复执行")
}
println("")

// ============ 测试2：C风格for循环 L> (init; cond; update) { } ============
println("=== 测试 2：C风格 for 循环 ===\n")

println("测试 2.1：基础计数")
L> (i := 0; i < 5; i++) {
    printf("  i = %.0f\n", i)
}
println("")

println("测试 2.2：步长为2")
L> (j := 0; j < 10; j = j + 2) {
    printf("  j = %.0f\n", j)
}
println("")

println("测试 2.3：倒序循环")
L> (k := 5; k > 0; k--) {
    printf("  k = %.0f\n", k)
}
println("")

// ============ 测试3：foreach循环 L> (array : item) { } ============
println("=== 测试 3：foreach 遍历循环 ===\n")

println("测试 3.1：遍历数字数组")
numbers := [10, 20, 30, 40, 50]
sum := 0
L> (numbers : n) {
    printf("  当前元素: %.0f\n", n)
    sum = sum + n
}
printf("数组总和: %.0f\n\n", sum)

println("测试 3.2：遍历字符串数组")
fruits := ["apple", "banana", "orange"]
L> (fruits : fruit) {
    printf("  水果: %s\n", fruit)
}
println("")

println("测试 3.3：遍历混合类型数组")
mixed := [1, "hello", 3.14, true, null, [1, 2], {key: "value"}, undef]
L> (mixed : item) {
    print("  元素: ")
    println(item)
}
println("")

// ============ 测试4：嵌套循环 ============
println("=== 测试 4：嵌套循环 ===\n")

println("测试 4.1：重复循环嵌套")
L> [3] {
    L> [2] {
        print("* ")
    }
    println("")
}
println("")

println("测试 4.2：for循环嵌套")
L> (row := 1; row <= 3; row = row + 1) {
    L> (col := 1; col <= row; col = col + 1) {
        printf("%d ", row * col)
    }
    println("")
}
println("")

println("测试 4.3：foreach中嵌套for")
matrix := [[1, 2, 3], [4, 5, 6]]
L> (matrix : row_arr) {
    L> (row_arr : val) {
        printf("%d ", val)
    }
    println("")
}

println("\n╔════════════════════════════════════════╗")
println("║           测试完成！                   ║")
println("╠════════════════════════════════════════╣")
println("║  ✓ 重复循环 L> [n] { }                 ║")
println("║  ✓ for循环 L> (i:=0; i<n; i++) { }     ║")
println("║  ✓ foreach循环 L> (arr : item) { }     ║")
println("╚════════════════════════════════════════╝")
