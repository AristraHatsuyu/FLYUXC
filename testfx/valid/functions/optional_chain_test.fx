// 测试可选链访问 (?.) 和 空值合并运算符 (??)
println("=== 可选链访问 (?.) 和 空值合并运算符 (??) 测试 ===")

// 1. 测试 ?. 访问存在的属性
println("\n--- 测试 1: ?. 访问存在的属性 ---")
person := { name: "Alice", age: 30 }
println(person?.name)    // 应该输出: Alice
println(person?.age)     // 应该输出: 30

// 2. 测试 ?. 访问不存在的属性
println("\n--- 测试 2: ?. 访问不存在的属性 ---")
println(person?.address)  // 应该输出: undef

// 3. 测试 ?. 访问 null/undef 对象
println("\n--- 测试 3: ?. 访问 null/undef 对象 ---")
nullObj:[obj] = null
undefObj:[obj] = undef
println(nullObj?.name)    // 应该输出: undef
println(undefObj?.name)   // 应该输出: undef

// 4. 测试 ?? 空值合并 - 左边有值
println("\n--- 测试 4: ?? 左边有值时 ---")
value1 := "Hello"
result1 := value1 ?? "Default"
println(result1)  // 应该输出: Hello

numVal := 0
result2 := numVal ?? 100
println(result2)  // 应该输出: 0 (0 不是 null/undef)

boolVal := false
result3 := boolVal ?? true
println(result3)  // 应该输出: false (false 不是 null/undef)

// 5. 测试 ?? 空值合并 - 左边是 null/undef
println("\n--- 测试 5: ?? 左边是 null/undef 时 ---")
nullVal:[str] = null
result4 := nullVal ?? "DefaultForNull"
println(result4)  // 应该输出: DefaultForNull

undefVal:[str] = undef
result5 := undefVal ?? "DefaultForUndef"
println(result5)  // 应该输出: DefaultForUndef

// 6. 组合使用 ?. 和 ??
println("\n--- 测试 6: 组合使用 ?. 和 ?? ---")
user := { profile: { nickname: "Bob" } }
name1 := user?.profile?.nickname ?? "Anonymous"
println(name1)  // 应该输出: Bob

emptyUser := {}
name2 := emptyUser?.profile?.nickname ?? "Anonymous"
println(name2)  // 应该输出: Anonymous

nullUser:[obj] = null
name3 := nullUser?.profile ?? "No profile"
println(name3)  // 应该输出: No profile

// 7. 测试 ?[ ] 可选链索引访问
println("\n--- 测试 7: ?[] 可选链索引访问 ---")
arr := [1, 2, 3]
println(arr?[0])  // 应该输出: 1
println(arr?[5])  // 应该输出: undef (越界)

nullArr:[num] = null
println(nullArr?[0])  // 应该输出: undef

myObj := { x: 10, y: 20 }
println(myObj?["x"])  // 应该输出: 10
println(myObj?["z"])  // 应该输出: undef (不存在的键，可选链返回 undef)

// 8. 测试可选链不应被 T> 截获
println("\n--- 测试 8: 可选链不应被 T> 截获 ---")
T> {
    testObj := { a: 1 }
    result := testObj?.nonexistent
    println("可选链结果: " + result)  // 应该输出: 可选链结果: undef
    println("T> 未捕获错误 - 正确!")
} (err) {
    println("T> 错误捕获 - 这不应该发生!")
}

// 9. 测试可选链索引越界不应被 T> 截获
println("\n--- 测试 9: ?[] 越界不应被 T> 截获 ---")
T> {
    testArr := [1, 2, 3]
    result := testArr?[100]  // 可选链索引越界，返回 undef
    println("可选链索引结果: " + result)  // 应该输出: 可选链索引结果: undef
    println("T> 未捕获错误 - 正确!")
} (err) {
    println("T> 错误捕获 - 这不应该发生!")
}

// 10. 测试可选链对象键不存在不应被 T> 截获
println("\n--- 测试 10: ?[] 键不存在不应被 T> 截获 ---")
T> {
    testObj2 := { a: 1 }
    result := testObj2?["nonexistent"]  // 可选链访问不存在的键，返回 undef
    println("可选链键结果: " + result)  // 应该输出: 可选链键结果: undef
    println("T> 未捕获错误 - 正确!")
} (err) {
    println("T> 错误捕获 - 这不应该发生!")
}

// 11. 测试 ?. 访问不存在的属性不应被 T> 截获
println("\n--- 测试 11: ?. 不存在属性不应被截获 ---")
T> {
    testObj3 := { name: "test" }
    result4 := testObj3?.missing?.deep
    println("深度可选链结果: " + result4)  // 应该输出: 深度可选链结果: undef
    println("T> 未捕获错误 - 正确!")
} (err) {
    println("T> 错误捕获 - 这不应该发生!")
}

// 12. 测试非可选链索引访问在 T> 中应被截获
println("\n--- 测试 12: T> 中非可选链索引越界应被截获 ---")
T> {
    testArr2 := [1, 2, 3]
    result5 := testArr2[100]  // 非可选链访问越界
    println("这不应该执行")
} (err) {
    println("T> 成功捕获数组越界: " + err.message)
}

// 13. 测试非可选链对象键访问在 T> 中应被截获
println("\n--- 测试 13: T> 中非可选链键不存在应被截获 ---")
T> {
    testObj4 := { a: 1 }
    result6 := testObj4["nonexistent"]  // 非可选链访问不存在的键
    println("这不应该执行")
} (err) {
    println("T> 成功捕获键不存在: " + err.message)
}

// 14. 测试非可选链 . 属性访问在 T> 中应被截获
println("\n--- 测试 14: T> 中非可选链 . 属性不存在应被截获 ---")
T> {
    testObj5 := { a: 1 }
    result7 := testObj5.nonexistent  // 非可选链 . 访问不存在的属性
    println("这不应该执行")
} (err) {
    println("T> 成功捕获属性不存在: " + err.message)
}

println("\n=== 所有可选链测试完成 ===")

// 15. 测试不在 T> 中的非可选链访问不存在的键（应该终止程序）
println("\n--- 测试 15: 非可选链访问不存在的键将终止程序 ---")
println("即将测试非可选链访问不存在的键...")
finalObj := { x: 1 }
finalResult := finalObj["doesNotExist"]  // 这将终止程序
println("如果看到这行，说明有 bug")
