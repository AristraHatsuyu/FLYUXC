// JSON 函数测试

println("========== JSON 测试 ==========\n")

// 1. 基础解析测试
println("--- 1. 基础解析测试 ---")
simple := parseJSON('{"name":"Alice","age":25,"active":true}')
println("解析简单对象:")
println(simple)
println("name =", simple.name)
println("age =", simple.age)
println("active =", simple.active)
println()

// 2. 解析数组
println("--- 2. 解析数组 ---")
arr := parseJSON('[1, 2, 3, "hello", true, null]')
println("解析数组:")
println(arr)
println("arr[0] =", arr[0])
println("arr[3] =", arr[3])
println("arr[5] =", arr[5])
println()

// 3. 解析嵌套对象
println("--- 3. 解析嵌套对象 ---")
nested := parseJSON('{"user":{"name":"Bob","info":{"city":"北京","age":30}}}')
println("解析嵌套对象:")
println(nested)
println("user.name =", nested.user.name)
println("user.info.city =", nested.user.info.city)
println("user.info.age =", nested.user.info.age)
println()

// 4. 对象转 JSON
println("--- 4. 对象转 JSON ---")
person := {
    name: "Charlie",
    age: 28,
    active: true,
    scores: [95, 87, 92]
}
json_str := toJSON(person)
println("对象转 JSON:")
println(json_str)
println()

// 5. 扩展类型测试 - Buffer
println("--- 5. 扩展类型测试 ---")
buffer := readBytes("testfx/demo.fx")
println("Buffer 类型:", typeOf(buffer))

data_with_buffer := {
    filename: "demo.fx",
    content: buffer,
    size: buffer.size
}
println("包含 Buffer 的对象:")
println(data_with_buffer)

json_with_buffer := toJSON(data_with_buffer)
println("转换为 JSON (Buffer 显示为类型名):")
println(json_with_buffer)
println()

// 6. 扩展类型测试 - Error
println("--- 6. 扩展类型测试 - Error ---")
err := Error("测试错误", 500, "TestError")
println("Error 类型:", typeOf(err))

response := {
    status: "error",
    error: err,
    timestamp: 1234567890
}
println("包含 Error 的对象:")
println(response)

json_with_error := toJSON(response)
println("转换为 JSON (Error 显示为类型名):")
println(json_with_error)
println()

// 7. 往返测试 (JSON → obj → JSON)
println("--- 7. 往返测试 ---")
original_json := '{"a":1,"b":"test","c":[1,2,3]}'
println("原始 JSON:", original_json)

parsed := parseJSON(original_json)
println("解析后的对象:")
println(parsed)

regenerated := toJSON(parsed)
println("重新生成的 JSON:", regenerated)
println()

// 8. 复杂嵌套测试
println("--- 8. 复杂嵌套测试 ---")
complex := {
    users: [
        {name: "Alice", age: 25, tags: ["admin", "user"]},
        {name: "Bob", age: 30, tags: ["user"]}
    ],
    config: {
        timeout: 5000,
        debug: true,
        options: {
            cache: true,
            retry: 3
        }
    }
}

complex_json := toJSON(complex)
println("复杂嵌套对象转 JSON:")
println(complex_json)
println()

reparsed := parseJSON(complex_json)
println("重新解析:")
println(reparsed)
println("users[0].name =", reparsed.users[0].name)
println("config.options.cache =", reparsed.config.options.cache)
println()

// 9. 特殊值测试
println("--- 9. 特殊值测试 ---")
special := {
    null_val: null,
    bool_true: true,
    bool_false: false,
    number_zero: 0,
    empty_str: "",
    empty_arr: [],
    empty_obj: {}
}
special_json := toJSON(special)
println("特殊值对象:")
println(special)
println("转为 JSON:")
println(special_json)
println()

// 10. 错误处理测试
println("--- 10. 错误处理测试 ---")
invalid1 := parseJSON("invalid json")
println("解析无效 JSON (1):", invalid1, "类型:", typeOf(invalid1))

invalid2 := parseJSON('{"incomplete":')
println("解析无效 JSON (2):", invalid2, "类型:", typeOf(invalid2))

invalid3 := parseJSON('[1, 2,')
println("解析无效 JSON (3):", invalid3, "类型:", typeOf(invalid3))
println()

// 11. 实际应用场景 - 配置文件
println("--- 11. 实际应用 - 配置文件 ---")
config := {
    app: "MyApp",
    version: "1.0.0",
    database: {
        host: "localhost",
        port: 5432,
        name: "mydb"
    },
    features: ["auth", "api", "cache"]
}

config_json := toJSON(config)
println("配置对象转 JSON:")
println(config_json)

// 保存到文件
writeFile("config.json", config_json)
println("已保存到 config.json")

// 从文件读取
loaded_json := readFile("config.json")
loaded_config := parseJSON(loaded_json)
println("从文件加载的配置:")
println(loaded_config)
println("app:", loaded_config.app)
println("database.host:", loaded_config.database.host)
println()

// 12. 实际应用 - API 响应
println("--- 12. 实际应用 - API 响应 ---")
api_response := {
    success: true,
    data: {
        users: [
            {id: 1, name: "用户1"},
            {id: 2, name: "用户2"}
        ],
        total: 2
    },
    message: "获取成功"
}

api_json := toJSON(api_response)
println("API 响应转 JSON:")
println(api_json)
println()

println("========== 测试完成 ==========")

