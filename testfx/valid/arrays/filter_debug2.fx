// 最简 filter 调试
arr := [3]

println("=== 测试 3 > 2 ===")
r := 3 > 2
println("3 > 2 = ", r)

// filter with debug - 简单版
println("\n=== filter [3] with x > 2 ===")
filtered := filter(arr, (x) { 
    R> x > 2
})
println("result:", filtered)
