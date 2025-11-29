// 测试 filter 机制
arr := [3]

// 直接测试大于比较
r := 3 > 2
println("3 > 2 =", r)

// 测试匿名函数
cb := (x) { R> x > 2 }
println("cb(3) =", cb(3))
println("cb(1) =", cb(1))

// 测试 filter
filtered := filter(arr, (x) { R> x > 2 })
println("filter result:", filtered)
