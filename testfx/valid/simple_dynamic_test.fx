// 简单测试动态对象
println("=== 简单动态对象测试 ===")

// 创建对象
person := {name: "Alice"}
println("初始对象:", person)

// 动态添加属性
result := setField(person, "age", 25)
println("添加age后:", person)
println("返回值:", result)

// 检查属性
hasName := hasField(person, "name")
hasEmail := hasField(person, "email")
println("has name:", hasName)
println("has email:", hasEmail)

// 获取键名
allKeys := keys(person)
println("所有键:", allKeys)

// 删除属性
deleted := deleteField(person, "name")
println("删除name后:", person)
println("删除返回:", deleted)

println("=== 测试完成 ===")
