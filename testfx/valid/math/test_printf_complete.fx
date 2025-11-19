/* FLYUX printf 函数完整测试 */

println("╔════════════════════════════════════════╗")
println("║    FLYUX printf 函数完整测试           ║")
println("╚════════════════════════════════════════╝")

println("\n=== 1. 基础格式化 ===")
printf("Hello, World!\n")
printf("单行输出\n")

println("\n=== 2. 字符串格式化 (%s) ===")
name := "FLYUX"
printf("Language: %s\n", name)
printf("Hello, %s! Welcome to %s programming.\n", "Alice", "FLYUX")
printf("Empty string: '%s'\n", "")

println("\n=== 3. 整数格式化 (%d, %i) ===")
printf("Integer: %d\n", 42)
printf("Negative: %d\n", -100)
printf("Zero: %d\n", 0)
printf("Large: %d\n", 9999999)
printf("Using %%i: %i\n", 123)

println("\n=== 4. 浮点数格式化 (%f) ===")
printf("Pi: %f\n", 3.14159)
printf("Negative float: %f\n", -2.5)
printf("Zero float: %f\n", 0.0)
printf("Small: %f\n", 0.001)

println("\n=== 5. 精度控制 (%.Nf) ===")
pi := 3.14159265359
printf("Pi with 2 decimals: %.2f\n", pi)
printf("Pi with 4 decimals: %.4f\n", pi)
printf("Pi with 6 decimals: %.6f\n", pi)
printf("Pi with 10 decimals: %.10f\n", pi)
printf("No precision: %f\n", pi)

println("\n=== 6. 科学格式 (%g) ===")
printf("Auto format: %g\n", 1234.5678)
printf("Large number: %g\n", 1234567890)
printf("Small number: %g\n", 0.00012345)
printf("With precision: %.4g\n", 3.14159)

println("\n=== 7. 布尔值格式化 (%b) ===")
printf("True: %b\n", true)
printf("False: %b\n", false)
flag := true
printf("Variable: %b\n", flag)
printf("Number as bool: %b (non-zero)\n", 42)
printf("Zero as bool: %b (zero)\n", 0)

println("\n=== 8. 值格式化 (%v) ===")
printf("Number value: %v\n", 42)
printf("String value: %v\n", "Hello")
printf("Bool value: %v\n", true)
printf("Null value: %v\n", null)

println("\n=== 9. 数组和对象 (%v) ===")
arr := [1, 2, 3]
printf("Array: %v\n", arr)
nested_arr := [1, [2, 3], 4]
printf("Nested array: %v\n", nested_arr)
person := { name: "Bob", age: 25 }
printf("Object: %v\n", person)

println("\n=== 10. 特殊数值 ===")
inf_pos := 10 / 0
inf_neg := -10 / 0
nan_val := 0 / 0
printf("Positive infinity: %f\n", inf_pos)
printf("Negative infinity: %f\n", inf_neg)
printf("NaN: %f\n", nan_val)
printf("With %%g - +Inf: %g, -Inf: %g, NaN: %g\n", inf_pos, inf_neg, nan_val)

println("\n=== 11. 混合格式化 ===")
printf("Name: %s, Age: %d, Score: %.2f\n", "Alice", 25, 95.5)
printf("Active: %b, Level: %d, Bonus: %g\n", true, 5, 123.456)
x := 10
y := 20
z := 30
printf("x=%d, y=%d, z=%d, sum=%d\n", x, y, z, x + y + z)

println("\n=== 12. 转义百分号 (%%) ===")
printf("Percentage: 100%% complete\n")
printf("Discount: 25%% off\n")
printf("Format: %%s for string, %%d for integer\n")

println("\n=== 13. 多个参数 ===")
printf("%s %s %s %s %s\n", "One", "Two", "Three", "Four", "Five")
printf("%d + %d + %d + %d = %d\n", 1, 2, 3, 4, 10)
printf("%.1f, %.2f, %.3f, %.4f\n", 1.1, 2.22, 3.333, 4.4444)

println("\n=== 14. 表达式作为参数 ===")
printf("Calculation: %d + %d = %d\n", 5, 3, 5 + 3)
printf("Result: %.2f * %.2f = %.2f\n", 2.5, 4.0, 2.5 * 4.0)
printf("Power: %d ** %d = %d\n", 2, 10, 2 ** 10)

println("\n=== 15. 类型混合 ===")
printf("String '%s' has length %d\n", "Hello", 5)
printf("Boolean %b equals number %d\n", true, 1)
printf("Float %f rounded is %d\n", 3.7, 4)

println("\n=== 16. 边界测试 ===")
printf("Very large: %g\n", 999999999999999)
printf("Very small: %g\n", 0.000000000000001)
printf("Precision edge: %.15f\n", 0.123456789012345)
printf("Max int-like: %d\n", 9007199254740992)

println("\n=== 17. 空值和未定义 ===")
printf("Null as %%v: %v\n", null)
printf("Null as %%s: %s\n", null)
printf("Null as %%d: %d\n", null)

println("\n=== 18. 复杂嵌套输出 ===")
data := {
    user: "Alice",
    stats: { score: 95, level: 10 },
    items: [1, 2, 3]
}
printf("User data: %v\n", data)
printf("Summary: %s (Level %d)\n", "Alice", 10)

println("\n=== 19. 数学常数 ===")
e := 2.71828182846
phi := 1.61803398875
printf("Euler's number: %.10f\n", e)
printf("Golden ratio: %.10f\n", phi)
printf("Pi * e: %.6f\n", 3.14159 * e)

println("\n=== 20. 格式化表格 ===")
printf("%-10s | %5s | %8s\n", "Name", "Age", "Score")
printf("%-10s | %5d | %8.2f\n", "Alice", 25, 95.5)
printf("%-10s | %5d | %8.2f\n", "Bob", 30, 87.3)
printf("%-10s | %5d | %8.2f\n", "Charlie", 22, 92.0)

println("\n=== 21. 连续输出测试 ===")
printf("Line 1\n")
printf("Line 2\n")
printf("Line 3\n")
printf("All on one line: ")
printf("Part1 ")
printf("Part2 ")
printf("Part3\n")

println("\n=== 22. 特殊字符串 ===")
printf("With spaces: '%s'\n", "Hello World")
printf("With numbers: '%s'\n", "123abc456")
printf("Mixed: '%s' and %d\n", "Number", 42)

println("\n=== 23. 浮点精度比较 ===")
a := 0.1
b := 0.2
c := a + b
printf("0.1 + 0.2 = %f (raw)\n", c)
printf("0.1 + 0.2 = %.1f (rounded)\n", c)
printf("0.1 + 0.2 = %g (auto)\n", c)

println("\n=== 24. 负数和零 ===")
printf("Negative int: %d\n", -42)
printf("Negative float: %f\n", -3.14)
printf("Negative with precision: %.2f\n", -123.456)
printf("Zero variants: %d, %f, %g\n", 0, 0.0, 0.0)

println("\n=== 25. 格式符覆盖测试 ===")
val := 42.5
printf("As %%d: %d\n", val)
printf("As %%f: %f\n", val)
printf("As %%g: %g\n", val)
printf("As %%s: %s\n", val)
printf("As %%v: %v\n", val)

println("\n=== 26. 参数不足测试 ===")
printf("Only one: %s\n", "Hello")
printf("Expected more: %s %s\n", "Only")

println("\n=== 27. 无参数测试 ===")
printf("No arguments\n")
printf("Just text with 100%% confidence\n")

println("\n=== 28. 实际应用场景 ===")
username := "Alice"
score := 1250
level := 15
accuracy := 0.875
printf("[Game] Player: %s | Score: %d | Level: %d | Accuracy: %.1f%%\n", 
       username, score, level, accuracy * 100)

temperature := 23.5
humidity := 65
printf("[Weather] Temperature: %.1f°C, Humidity: %d%%\n", temperature, humidity)

price := 99.99
discount := 0.15
final_price := price * (1 - discount)
printf("[Shop] Original: $%.2f, Discount: %.0f%%, Final: $%.2f\n", 
       price, discount * 100, final_price)

println("\n=== 29. 压力测试 ===")
printf("Many args: %d %d %d %d %d %d %d %d %d %d\n", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
printf("Long string: %s %s %s %s %s\n", 
       "First", "Second", "Third", "Fourth", "Fifth")

println("\n=== 30. 综合测试 ===")
result := {
    status: "success",
    code: 200,
    data: { count: 42, active: true },
    message: "Operation completed"
}
printf("API Response: %v\n", result)
printf("Status: %s (Code: %d)\n", "success", 200)
printf("Data count: %d, Active: %b\n", 42, true)

println("\n╔════════════════════════════════════════╗")
println("║    ✅ printf 测试完成！                ║")
println("╚════════════════════════════════════════╝")
