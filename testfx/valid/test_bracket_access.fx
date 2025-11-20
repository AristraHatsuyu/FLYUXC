// 测试 obj["key"] 形式的访问和修改
println("=== 测试对象索引访问 obj['key'] ===")

user := {name: "Alice", age: 25}
println("初始 user:", user)

// 使用字符串索引访问
println("user['name']:", user["name"])
println("user['age']:", user["age"])

// 使用字符串索引修改
user["age"] = 26
println("修改 user['age'] 后:", user)

// 使用字符串索引添加新属性
user["email"] = "alice@example.com"
println("添加 user['email'] 后:", user)

// 动态字符串键
println("\n=== 动态字符串键 ===")
key := "city"
user[key] = "Beijing"
println("user[key] (key='city') 后:", user)

// 混合使用点访问和索引访问
println("\n=== 混合访问方式 ===")
data := {info: {title: "Test"}}
println("data.info['title']:", data.info["title"])

data.info["description"] = "A test object"
println("修改后 data:", data)

// 数组和对象混合
println("\n=== 数组和对象混合 ===")
config := {
    servers: [{host: "localhost", port: 8080}]
}
println("config:", config)
println("config.servers[0]['host']:", config.servers[0]["host"])

config.servers[0]["port"] = 9000
println("修改后 config:", config)
println("修改后 port:", config.servers[0]["port"])

println("\n=== 测试完成 ===", [], {})
