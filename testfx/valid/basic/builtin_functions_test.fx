// 测试新增的内置函数

println("=== 字符串处理函数测试 ===")

// len() - 获取长度
text := "Hello World"
println("len('Hello World'):", len(text))

// charAt() - 获取字符
println("charAt('Hello', 1):", charAt("Hello", 1))

// substr() - 子字符串
println("substr('Hello World', 0, 5):", substr("Hello World", 0, 5))
println("substr('Hello World', 6):", substr("Hello World", 6))

// indexOf() - 查找位置
println("indexOf('Hello World', 'World'):", indexOf("Hello World", "World"))
println("indexOf('Hello World', 'xyz'):", indexOf("Hello World", "xyz"))

// replace() - 替换
println("replace('Hello World', 'World', 'FLYUX'):", replace("Hello World", "World", "FLYUX"))

// split() - 分割
words := split("a,b,c", ",")
println("split('a,b,c', ','):", words)

// join() - 连接
println("join(words, '-'):", join(words, "-"))

// trim() - 去除空白
println("trim('  hello  '):", trim("  hello  "))

// upper/lower - 大小写转换
println("upper('hello'):", upper("hello"))
println("lower('WORLD'):", lower("WORLD"))

println("\n=== 数组操作函数测试 ===")

// 创建数组
arr := [1, 2, 3]
println("原始数组:", arr)

// push() - 添加到末尾
arr = push(arr, 4)
println("push(arr, 4):", arr)

// pop() - 移除最后一个
last := pop(arr)
println("pop(arr):", last)
println("数组仍为:", arr)

// unshift() - 添加到开头
arr = unshift(arr, 0)
println("unshift(arr, 0):", arr)

// shift() - 移除第一个
first := shift(arr)
println("shift(arr):", first)
println("数组仍为:", arr)

// slice() - 切片
arr2 := [10, 20, 30, 40, 50]
println("arr2:", arr2)
println("slice(arr2, 1, 4):", slice(arr2, 1, 4))

// concat() - 连接数组
arr3 := [100, 200]
println("concat([1,2,3], [100,200]):", concat([1, 2, 3], arr3))

println("\n=== 类型转换函数测试 ===")

// toInt() - 转为整数
println("toInt(3.14):", toInt(3.14))
println("toInt('42'):", toInt("42"))

// toFloat() - 转为浮点数
println("toFloat('3.14'):", toFloat("3.14"))

println("\n=== 错误处理测试 ===")

T> {
    result := charAt("abc", 10)  // 越界访问
    println("这行应该输出")
} (err) {
    println("不应该输出错误:", err.message)
    println("不应该输出错误类型:", err.type)
}

T> {
    result := toInt("not a number")!
    println("这行不应该输出")
} (err) {
    println("捕获到转换错误:", err.message)
}

println("\n=== 所有测试完成 ===")
