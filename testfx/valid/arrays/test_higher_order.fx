// 测试高阶函数

// 1. map - 映射
println("=== map ===")
arr := [1, 2, 3, 4, 5]
mapped := map(arr, (x) { R> x * 2 })
println(mapped)  // [2, 4, 6, 8, 10]

// 2. filter - 过滤
println("\n=== filter ===")
evens := filter(arr, (x) { R> x % 2 == 0 })
println(evens)  // [2, 4]

// 3. reduce - 归约
println("\n=== reduce ===")
sum := reduce(arr, (acc, x) { R> acc + x }, 0)
println(sum)  // 15

// 4. find - 查找
println("\n=== find ===")
found := find(arr, (x) { R> x > 3 })
println(found)  // 4

// 5. sort - 排序
println("\n=== sort ===")
unsorted := [3, 1, 4, 1, 5, 9, 2, 6]
sorted := sort(unsorted)
println(sorted)  // [1, 1, 2, 3, 4, 5, 6, 9]

// 带比较函数的排序（降序）
desc := sort(unsorted, (a, b) { R> b - a })
println(desc)  // [9, 6, 5, 4, 3, 2, 1, 1]
