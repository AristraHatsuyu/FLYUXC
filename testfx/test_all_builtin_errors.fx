// 全面测试所有内置函数的错误处理

println("=== Comprehensive Built-in Functions Error Handling Test ===\n")

// 1. 字符串函数错误测试
println("--- String Functions ---")

// charAt 越界
result1 := charAt("abc", 10)
println("charAt('abc', 10):", result1, "type:", typeOf(result1))

T> {
    result2 := charAt("abc", 20)!
    println("Should NOT print")
} (err) {
    println("Caught charAt! error:", err.message)
}

// substr 越界 (应该返回空字符串或部分结果,不报错)
result3 := substr("abc", 10, 20)
println("substr('abc', 10, 20):", result3)

// indexOf 未找到 (返回 -1,不报错)
result4 := indexOf("hello", "xyz")
println("indexOf('hello', 'xyz'):", result4)

println()

// 2. 数组函数错误测试
println("--- Array Functions ---")

// pop 空数组
empty_arr := []
result5 := pop(empty_arr)
println("pop([]):", result5, "type:", typeOf(result5))

// shift 空数组
result6 := shift(empty_arr)
println("shift([]):", result6, "type:", typeOf(result6))

// slice 越界 (应该返回有效部分)
arr := [1, 2, 3]
result7 := slice(arr, 10, 20)
println("slice([1,2,3], 10, 20):", result7)

println()

// 3. 类型转换函数错误测试
println("--- Type Conversion Functions ---")

// toNum 无效字符串
result8 := toNum("not a number")
println("toNum('not a number'):", result8, "type:", typeOf(result8))

T> {
    result9 := toNum("xyz")!
    println("Should NOT print")
} (err) {
    println("Caught toNum! error:", err.message)
}

// toInt 无效字符串
result10 := toInt("abc")
println("toInt('abc'):", result10, "type:", typeOf(result10))

T> {
    result11 := toInt("xyz")!
    println("Should NOT print")
} (err) {
    println("Caught toInt! error:", err.message)
}

// toFloat 无效字符串
result12 := toFloat("not float")
println("toFloat('not float'):", result12, "type:", typeOf(result12))

println()

// 4. 文件I/O函数错误测试
println("--- File I/O Functions ---")

// readFile 不存在的文件
result13 := readFile("/nonexistent/file.txt")
println("readFile(nonexistent):", result13, "type:", typeOf(result13))

T> {
    result14 := readFile("/missing/file.txt")!
    println("Should NOT print")
} (err) {
    println("Caught readFile! error:", err.message)
}

// writeFile 无权限路径
result15 := writeFile("/root/test.txt", "data")
println("writeFile(/root/test.txt):", result15, "type:", typeOf(result15))

// appendFile 无权限路径
result16 := appendFile("/root/test.txt", "data")
println("appendFile(/root/test.txt):", result16, "type:", typeOf(result16))

// readBytes 不存在的文件
result17 := readBytes("/nonexistent.bin")
println("readBytes(nonexistent):", result17, "type:", typeOf(result17))

// writeBytes 无权限路径
buffer := [65, 66, 67]
result18 := writeBytes("/root/test.bin", buffer)
println("writeBytes(/root/test.bin):", result18, "type:", typeOf(result18))

// readLines 不存在的文件
result19 := readLines("/nonexistent.txt")
println("readLines(nonexistent):", result19, "type:", typeOf(result19))

// deleteFile 不存在的文件
result20 := deleteFile("/nonexistent/file.txt")
println("deleteFile(nonexistent):", result20, "type:", typeOf(result20))

// getFileSize 不存在的文件
result21 := getFileSize("/nonexistent.txt")
println("getFileSize(nonexistent):", result21, "type:", typeOf(result21))

// renameFile 不存在的文件
result22 := renameFile("/nonexistent1.txt", "/nonexistent2.txt")
println("renameFile(nonexistent):", result22, "type:", typeOf(result22))

// copyFile 不存在的文件
result23 := copyFile("/nonexistent1.txt", "/nonexistent2.txt")
println("copyFile(nonexistent):", result23, "type:", typeOf(result23))

// listDir 不存在的目录
result24 := listDir("/nonexistent/directory")
println("listDir(nonexistent):", result24, "type:", typeOf(result24))

// createDir 无权限路径
result25 := createDir("/root/newdir")
println("createDir(/root/newdir):", result25, "type:", typeOf(result25))

// removeDir 不存在的目录
result26 := removeDir("/nonexistent/directory")
println("removeDir(nonexistent):", result26, "type:", typeOf(result26))

println()

// 5. JSON函数错误测试
println("--- JSON Functions ---")

// parseJSON 无效JSON
result27 := parseJSON("not json")
println("parseJSON('not json'):", result27, "type:", typeOf(result27))

T> {
    result28 := parseJSON("invalid")!
    println("Should NOT print")
} (err) {
    println("Caught parseJSON! error:", err.message)
}

// parseJSON 不完整JSON
result29 := parseJSON('{"incomplete":')
println("parseJSON(incomplete):", result29, "type:", typeOf(result29))

println()

// 6. 正常情况验证(确保!后缀不影响正常功能)
println("--- Normal Cases with ! ---")

writeFile("test_temp.txt", "Test Data")

content1 := readFile("test_temp.txt")
println("readFile(valid):", content1)

content2 := readFile("test_temp.txt")!
println("readFile(valid)!:", content2)

num1 := toNum("123.45")
println("toNum('123.45'):", num1)

num2 := toNum("67.89")!
println("toNum('67.89')!:", num2)

int1 := toInt("100")
println("toInt('100'):", int1)

int2 := toInt("200")!
println("toInt('200')!:", int2)

json1 := parseJSON('{"key":"value"}')
println("parseJSON(valid):", json1)

json2 := parseJSON('{"test":123}')!
println("parseJSON(valid)!:", json2)

deleteFile("test_temp.txt")

println()

// 7. 混合场景测试
println("--- Mixed Scenarios ---")

// 场景1: 多个带!的函数调用在Try-Catch中
T> {
    data1 := readFile("/missing1.txt")!
    println("Should NOT reach here")
    data2 := readFile("/missing2.txt")!
    println("Should NOT reach here either")
} (err) {
    println("Caught first error:", err.message)
}

// 场景2: 无!函数返回null后继续使用
content := readFile("/nonexistent.txt")
if content == null {
    println("readFile returned null, handling gracefully")
}

// 场景3: 链式调用中的错误处理
arr2 := [1, 2, 3]
arr2 = push(arr2, 4)
arr2 = push(arr2, 5)
last := pop(arr2)
println("Chain operations result:", arr2, "last:", last)

println("\n=== All Tests Complete ===")
