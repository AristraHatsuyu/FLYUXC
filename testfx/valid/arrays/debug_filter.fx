// Debug filter test
arr := [1, 2, 3, 4, 5]

// Test 1: 直接测试比较结果
println("=== 比较测试 ===")
println(3 > 2)  // 应该是 true 或 1
println(1 > 2)  // 应该是 false 或 0

// Test 2: 测试取模
println("\n=== 取模测试 ===")
println(4 % 2)  // 应该是 0
println(3 % 2)  // 应该是 1
println(4 % 2 == 0)  // 应该是 true

// Test 3: 简单的 filter - x > 2
println("\n=== filter x > 2 ===")
gtTwo := filter(arr, (x) { R> x > 2 })
println(gtTwo)  // 应该是 [3, 4, 5]

// Test 4: filter 偶数
println("\n=== filter 偶数 ===")
evens := filter(arr, (x) { R> x % 2 == 0 })
println(evens)  // 应该是 [2, 4]
