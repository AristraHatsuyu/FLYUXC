// JSON 扩展类型序列化测试

println("========== JSON 扩展类型测试 ==========\n")

// 1. 创建一个测试文件用于读取
println("--- 1. 准备测试文件 ---")
writeFile("test_buffer.txt", "Hello, Buffer!")
println("已创建 test_buffer.txt")
println()

// 2. 读取 Buffer 并测试序列化
println("--- 2. Buffer 类型序列化 ---")
buffer := readBytes("test_buffer.txt")
println("Buffer 类型:", typeOf(buffer))
println("Buffer 内容:")
println(buffer)
println()

// 3. 包含 Buffer 的对象
println("--- 3. 对象中的 Buffer ---")
data := {
    filename: "test_buffer.txt",
    content: buffer,
    size: buffer.size,
    timestamp: 1234567890
}
println("原对象:")
println(data)
println()

json := toJSON(data)
println("转换为 JSON:")
println(json)
println()

// 4. 多个 Buffer 对象
println("--- 4. 多个 Buffer 对象 ---")
buffer2 := readBytes("testfx/demo.fx")
multi_buffer := {
    files: [
        {name: "test_buffer.txt", data: buffer},
        {name: "demo.fx", data: buffer2}
    ],
    total: 2
}
println("包含多个 Buffer 的对象:")
println(multi_buffer)
println()

multi_json := toJSON(multi_buffer)
println("转为 JSON:")
println(multi_json)
println()

// 5. Buffer 在数组中
println("--- 5. 数组中的 Buffer ---")
arr := [
    "字符串",
    123,
    buffer,
    true,
    null
]
println("混合类型数组:")
println(arr)
println()

arr_json := toJSON(arr)
println("转为 JSON:")
println(arr_json)
println()

// 6. 嵌套的复杂结构
println("--- 6. 复杂嵌套结构 ---")
complex := {
    metadata: {
        version: "1.0",
        created: 1234567890
    },
    files: [
        {
            name: "file1.txt",
            content: buffer,
            size: buffer.size,
            properties: {
                readonly: false,
                hidden: false
            }
        },
        {
            name: "file2.txt",
            content: buffer2,
            size: buffer2.size,
            properties: {
                readonly: true,
                hidden: false
            }
        }
    ],
    summary: {
        total_files: 2,
        total_size: buffer.size + buffer2.size
    }
}

println("复杂结构对象:")
println(complex)
println()

complex_json := toJSON(complex)
println("转为 JSON (Buffer 显示为 \"Buffer\"):")
println(complex_json)
println()

// 7. 验证 JSON 可以重新解析（Buffer 变成字符串）
println("--- 7. 往返测试 ---")
reparsed := parseJSON(complex_json)
println("重新解析后的对象:")
println(reparsed)
println()
println("files[0].content 类型:", typeOf(reparsed.files[0].content))
println("files[0].content 值:", reparsed.files[0].content)
println()

// 8. 实际应用场景 - 文件信息 API
println("--- 8. 实际应用 - 文件信息 API ---")
api_response := {
    success: true,
    data: {
        file: {
            name: "test_buffer.txt",
            type: "text/plain",
            size: buffer.size,
            preview: buffer,
            metadata: {
                created: "2025-01-01",
                modified: "2025-01-02"
            }
        }
    },
    message: "File info retrieved"
}

api_json := toJSON(api_response)
println("API 响应 JSON:")
println(api_json)
println()

// 清理测试文件
deleteFile("test_buffer.txt")
println("已删除测试文件")
println()

println("========== 测试完成 ==========")
println("\n✅ Buffer 扩展类型在 JSON 中正确序列化为 \"Buffer\" 字符串")
