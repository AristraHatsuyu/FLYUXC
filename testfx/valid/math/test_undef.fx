// 测试 undef 的显示
println(x)

x:[num] = 123
println(x)

// 删除变量（设为 undef）
x = undef
println(x)

// 测试混合数组中的 undef
arr := [1, undef, 3]
println(arr)
println(undef)
println(abc)