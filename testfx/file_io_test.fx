/* 文件I/O和扩展对象类型测试 */

// 测试1: 基本文本文件操作
println("=== 测试1: 文本文件写入和读取 ===")
success := writeFile("test_output.txt", "Hello, FLYUX!\n这是第二行。")
println("写入结果:", success)

content := readFile("test_output.txt")
println("读取内容:")
println(content)

// 测试2: 文件追加
println("\n=== 测试2: 文件追加 ===")
appendFile("test_output.txt", "追加的内容。\n")
content2 := readFile("test_output.txt")
println("追加后内容:")
println(content2)

// 测试3: 文件查询
println("\n=== 测试3: 文件查询 ===")
exists := fileExists("test_output.txt")
println("文件存在:", exists)

size := getFileSize("test_output.txt")
println("文件大小:", size, "字节")

// 测试4: Buffer类型和typeOf
println("\n=== 测试4: Buffer类型测试 ===")
writeFile("binary_test.bin", "ABCDEFGH")
buffer := readBytes("binary_test.bin")
println("buffer类型:", typeOf(buffer))
println("buffer对象:", buffer)

// 测试5: Buffer属性访问
println("\n=== 测试5: Buffer属性访问 ===")
println("buffer.size:", buffer.size)
println("buffer.capacity:", buffer.capacity)
println("buffer.type:", buffer.type)

// 测试6: Buffer索引访问
println("\n=== 测试6: Buffer索引访问 ===")
println("buffer[0]:", buffer[0])
println("buffer[1]:", buffer[1])
println("buffer[2]:", buffer[2])

// 测试7: 用Buffer写文件
println("\n=== 测试7: 用Buffer写入文件 ===")
writeBytes("output_binary.bin", buffer)
println("写入完成")

// 测试8: 用数组写二进制文件
println("\n=== 测试8: 用数组写二进制文件 ===")
arr := [72, 101, 108, 108, 111]  // "Hello" in ASCII
writeBytes("array_output.bin", arr)
content3 := readFile("array_output.bin")
println("数组写入结果:", content3)

// 测试9: 删除测试文件
println("\n=== 测试9: 清理测试文件 ===")
deleteFile("test_output.txt")
deleteFile("binary_test.bin")
deleteFile("output_binary.bin")
deleteFile("array_output.bin")
println("清理完成")

// 测试10: Buffer作为对象属性
println("\n=== 测试10: Buffer作为对象属性 ===")
writeFile("data.bin", "123456")
data := readBytes("data.bin")
myObj := { buffer: data, name: "test" }
println("对象中的buffer:", myObj.buffer)
println("buffer类型:", typeOf(myObj.buffer))
println("buffer.size:", myObj.buffer.size)
deleteFile("data.bin")

// 测试11: Buffer数组
println("\n=== 测试11: Buffer数组 ===")
writeFile("a.bin", "AAA")
writeFile("b.bin", "BBB")
buf1 := readBytes("a.bin")
buf2 := readBytes("b.bin")
buffers := [buf1, buf2]
println("buffers[0]类型:", typeOf(buffers[0]))
println("buffers[0].size:", buffers[0].size)
println("buffers[1].size:", buffers[1].size)
deleteFile("a.bin")
deleteFile("b.bin")

println("\n=== 所有测试完成 ===")
