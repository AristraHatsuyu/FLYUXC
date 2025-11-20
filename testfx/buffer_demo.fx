/* 扩展对象类型演示 - Buffer类型 */

// 1. 创建一个文本文件
writeFile("demo.txt", "Hello, FLYUX World!")

// 2. 读取为Buffer（二进制）
buffer := readBytes("demo.txt")

// 3. Buffer的类型检查
println("类型:", typeOf(buffer))  // 输出 "obj:Buffer"

// 4. Buffer属性访问
println("大小:", buffer.size, "字节")
println("容量:", buffer.capacity, "字节")

// 5. Buffer索引访问（获取字节值）
println("\n前5个字节:")
println("buffer[0] =", buffer[0], "(ASCII:", buffer[0], ")")
println("buffer[1] =", buffer[1])
println("buffer[2] =", buffer[2])
println("buffer[3] =", buffer[3])
println("buffer[4] =", buffer[4])

// 6. Buffer可以放入对象
data := {
    file: "demo.txt",
    content: buffer,
    size: buffer.size
}
println("\n对象中的Buffer:")
println("data.content:", data)
println("data.content:", data.content)
println("data.size:", data.size)

// 7. Buffer可以放入数组
buffers := [buffer]
println("\n数组中的Buffer:")
println("buffers[0]:", buffer)
println("buffers[0]:", buffers)
println("buffers[0].size:", buffers[0].size)

// 8. 清理
// deleteFile("demo.txt")
println("\n演示完成！")
