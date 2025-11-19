// 完整测试 undef, null 和对象的彩虹括号

// 1. 测试访问未定义变量
println(x)

// 2. 定义后访问
x:[num] = 123
println(x)

// 3. 赋值为 undef
x = undef
println(x)

// 4. 直接输出 undef 字面量
println(undef)

// 5. 访问不存在的变量
println(abc)

// 6. 测试 null（不是 undef）
null_val:[num] = null
println(null_val)

// 7. 测试数组中的 undef
arr := [1, undef, 3]
println(arr)

// 8. 测试嵌套数组的彩虹括号
nested := [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]
println(nested)

// 9. 测试对象的彩虹括号
object := {name: "Alice", age: 30, active: true}
println(object)

// 10. 测试嵌套对象（单行）
user := {name: "Bob", info: {age: 25, city: "NYC"}}
println(user)
