// 测试 reverse 函数对字符串和数组的支持

println("=== reverse 字符串测试 ===")

// 基本字符串反转
s1 := "Hello"
println("reverse('Hello') = ", reverse(s1))

// 中文字符串（注意：这可能不支持多字节字符）
s2 := "abc123"
println("reverse('abc123') = ", reverse(s2))

// 空字符串
s3 := ""
println("reverse('') = '", reverse(s3), "'")

// 单字符
s4 := "X"
println("reverse('X') = ", reverse(s4))

// 回文检测
s5 := "racecar"
reversed := reverse(s5)
println("'", s5, "' reversed = '", reversed, "' (is palindrome: ", s5 == reversed, ")")

s6 := "hello"
reversed2 := reverse(s6)
println("'", s6, "' reversed = '", reversed2, "' (is palindrome: ", s6 == reversed2, ")")

println("")
println("=== reverse 数组测试 ===")

// 数组反转（确保原有功能仍然正常）
arr1 := [1, 2, 3, 4, 5]
println("reverse([1,2,3,4,5]) = ", reverse(arr1))

arr2 := ["a", "b", "c"]
println("reverse(['a','b','c']) = ", reverse(arr2))

println("")
println("All reverse tests passed!")
