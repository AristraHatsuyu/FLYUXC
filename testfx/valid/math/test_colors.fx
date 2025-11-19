/* test_colors.fx */
n := 42
s := "Hello"
b := true
null_val:[num] = null

// 嵌套数组
arr1 := [1, null, undef]
arr2 := { a: 1, b: arr, x: [null, { e: "World" }, 4] }

println(n)
println(s)
println(b)
println(null_val)
println(arr)
println(arr1)
println(arr2)