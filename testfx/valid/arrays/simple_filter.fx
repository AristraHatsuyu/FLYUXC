// 最简 filter 测试
arr := [1, 2, 3]

// 测试回调返回
testCb := (x) {
    result := x > 2
    println("callback x=", x, " result=", result)
    R> result
}

// 手动检查回调
println("=== 手动调用回调 ===")
testCb(1)  // false
testCb(3)  // true

// 测试 filter
println("\n=== filter 测试 ===")
filtered := filter(arr, (x) { R> x > 2 })
println("结果:", filtered)
