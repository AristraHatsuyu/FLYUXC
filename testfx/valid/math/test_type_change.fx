// 测试变量重新定义时改变类型

println("=== 测试 1: 数字 → 字符串 ===")
x := 123
println("初始值（数字）:")
print(x)
println("")
printf("类型: %s, 值: ", typeOf(x))
print(x)
println("")

x := "hello"
println("\n重新定义为字符串:")
print(x)
println("")
printf("类型: %s, 值: ", typeOf(x))
print(x)
println("")

println("\n=== 测试 2: 字符串 → 布尔 ===")
y := "world"
println("初始值（字符串）:")
printf("类型: %s, 值: %s\n", typeOf(y), "world")

y := true
println("\n重新定义为布尔:")
printf("类型: %s, 值: ", typeOf(y))
print(y)
println("")

println("\n=== 测试 3: 布尔 → 数字 ===")
z := false
println("初始值（布尔）:")
printf("类型: %s, 值: ", typeOf(z))
print(z)
println("")

z := 3.14159
println("\n重新定义为数字:")
printf("类型: %s, 值: %.5f\n", typeOf(z), z)

println("\n=== 测试 4: 数组 → 对象 ===")
arr := [1, 2, 3]
println("初始值（数组）:")
printf("类型: %s, 长度: ", typeOf(arr))
print(length(arr))
println("")

arr := {name: "test", value: 100}
println("\n重新定义为对象:")
printf("类型: %s\n", typeOf(arr))
print(arr)
println("")

println("\n=== 测试 5: 对象 → 数字（通过 null 中转）===")
myobj := {x: 1, y: 2}
println("初始值（对象）:")
printf("类型: %s\n", typeOf(myobj))

myobj := 0
println("\n重新定义为数字:")
printf("类型: %s, 值: ", typeOf(myobj))
print(myobj)
println("")

println("\n=== 测试 6: 多次类型切换 ===")
multi := 42
printf("1. num: ")
print(multi)
println("")

multi := "text"
printf("2. str: ")
print(multi)
println("")

multi := true
printf("3. bl: ")
print(multi)
println("")

multi := [1, 2]
printf("4. arr: ")
print(multi)
println("")

multi := 99.9
printf("5. num: %.1f\n", multi)

println("\n✓ 所有类型转换测试完成")
