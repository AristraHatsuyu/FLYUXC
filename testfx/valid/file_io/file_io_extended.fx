/* 测试扩展的文件I/O函数 - Phase 1 */

println("=== 测试1: readLines - 逐行读取 ===")
// 创建测试文件
writeFile("lines_test.txt", "第一行\n第二行\n第三行\n最后一行")
lines := readLines("lines_test.txt")
println("读取行数:", len(lines))
println("第1行:", lines[0])
println("第2行:", lines[1])
println("第3行:", lines[2])
println("第4行:", lines[3])

println("\n=== 测试2: renameFile - 重命名文件 ===")
writeFile("old_name.txt", "重命名测试")
success := renameFile("old_name.txt", "new_name.txt")
println("重命名结果:", success)
println("新文件存在:", fileExists("new_name.txt"))
println("旧文件存在:", fileExists("old_name.txt"))
content := readFile("new_name.txt")
println("新文件内容:", content)

println("\n=== 测试3: copyFile - 复制文件 ===")
writeFile("source.txt", "这是源文件内容")
success2 := copyFile("source.txt", "copy.txt")
println("复制结果:", success2)
println("源文件内容:", readFile("source.txt"))
println("副本内容:", readFile("copy.txt"))
println("源文件大小:", getFileSize("source.txt"))
println("副本大小:", getFileSize("copy.txt"))

println("\n=== 测试4: 目录操作 ===")
// createDir
println("创建目录...")
result := createDir("test_dir")
println("创建结果:", result)
println("目录存在:", dirExists("test_dir"))

// 在目录中创建文件
writeFile("test_dir/file1.txt", "文件1")
writeFile("test_dir/file2.txt", "文件2")
writeFile("test_dir/file3.txt", "文件3")

// listDir
println("\n列出目录内容:")
files := listDir("test_dir")
println("文件数量:", len(files))
println("文件列表:", files)

// 删除目录中的文件
deleteFile("test_dir/file1.txt")
deleteFile("test_dir/file2.txt")
deleteFile("test_dir/file3.txt")

// removeDir
println("\n删除空目录...")
result2 := removeDir("test_dir")
println("删除结果:", result2)
println("目录存在:", dirExists("test_dir"))

println("\n=== 测试5: 综合场景 - 日志处理 ===")
// 创建日志目录
createDir("logs")

// 写入日志
writeFile("logs/app.log", "INFO: 应用启动\n")
appendFile("logs/app.log", "DEBUG: 初始化配置\n")
appendFile("logs/app.log", "INFO: 服务器监听端口8080\n")
appendFile("logs/app.log", "ERROR: 连接数据库失败\n")
appendFile("logs/app.log", "INFO: 重试连接...\n")

// 读取日志
println("日志内容:")
log_lines := readLines("logs/app.log")
println("日志行数:", len(log_lines))
i := 0
while i < len(log_lines) {
    println("  [", i + 1, "]", log_lines[i])
    i := i + 1
}

// 备份日志
copyFile("logs/app.log", "logs/app.log.backup")
println("\n日志已备份")

// 归档旧日志
renameFile("logs/app.log.backup", "logs/app_20231120.log")
println("日志已归档")

// 查看logs目录
println("\nlogs目录内容:")
log_files := listDir("logs")
println(log_files)

println("\n=== 测试6: 错误处理 ===")
// 读取不存在的文件
empty_lines := readLines("not_exist.txt")
println("不存在文件的readLines结果:", empty_lines)
println("长度:", len(empty_lines))

// 重命名不存在的文件
fail1 := renameFile("not_exist1.txt", "not_exist2.txt")
println("重命名不存在文件:", fail1)

// 复制不存在的文件
fail2 := copyFile("not_exist.txt", "copy.txt")
println("复制不存在文件:", fail2)

// 删除不存在的目录
fail3 := removeDir("not_exist_dir")
println("删除不存在目录:", fail3)

// 列出不存在的目录
empty_list := listDir("not_exist_dir")
println("列出不存在目录:", empty_list)

println("\n=== 清理测试文件 ===")
deleteFile("lines_test.txt")
deleteFile("new_name.txt")
deleteFile("source.txt")
deleteFile("copy.txt")
deleteFile("logs/app.log")
deleteFile("logs/app_20231120.log")
removeDir("logs")
println("清理完成!")

println("\n=== 所有测试完成 ===")
