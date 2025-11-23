// 重现原始问题
user := {name: "Alice"}
println("初始:", user)

user.age = 25  // 添加新属性
println("添加age后:", user)

user.name = "Alice Wang"  // 修改已有属性  
println("修改name后:", user)

user.email = "alice@example.com"  // 再添加
println("添加email后:", user)
