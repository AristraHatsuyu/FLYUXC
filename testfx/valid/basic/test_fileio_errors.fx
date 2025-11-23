// 测试更多文件I/O函数的 ! 后缀

println("=== 文件I/O函数 ! 后缀测试 ===\n")

// 测试 readBytes
println("--- readBytes ---")
bytes1 := readBytes("/nonexistent.bin")
println("readBytes(不存在) =", bytes1)
println("类型:", typeOf(bytes1))
println()

// 测试 readLines  
println("--- readLines ---")
lines1 := readLines("/nonexistent.txt")
println("readLines(不存在) =", lines1)
println("类型:", typeOf(lines1))
println()

// 测试 listDir
println("--- listDir ---")
files1 := listDir("/nonexistent/dir")
println("listDir(不存在) =", files1)
println("类型:", typeOf(files1))
println()

// 测试带 ! 的情况
println("--- 带 ! 的文件操作 ---")
T> {
    bytes2 := readBytes("/nonexistent.bin")!
    println("不应该执行到这里")
} (err) {
    println("readBytes! 错误:", err.message)
}

T> {
    lines2 := readLines("/nonexistent.txt")!
    println("不应该执行到这里")
} (err) {
    println("readLines! 错误:", err.message)
}

T> {
    files2 := listDir("/nonexistent/dir")!
    println("不应该执行到这里")
} (err) {
    println("listDir! 错误:", err.message)
}

println("\n=== 所有测试完成 ===")
