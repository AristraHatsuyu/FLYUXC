// 动态对象完整功能演示
println("=== FLYUX 动态对象系统演示 ===\n")

// 1. 基础语法：直接赋值添加/修改属性
println("【1. 直接属性赋值】")
user := {name: "Alice"}
println("初始:", user)

user.age = 25  // 添加新属性
user.name = "Alice Wang"  // 修改已有属性
user.email = "alice@example.com"  // 再添加
println("直接赋值后:", user)

// 2. 空对象动态构建
println("\n【2. 空对象动态构建】")
point := {}
point.x = 100
point.y = 200
point.label = "Point A"
println("动态构建的对象:", point)

// 3. 使用函数接口
println("\n【3. 函数接口操作】")
config := {debug: false}
setField(config, "timeout", 5000)
setField(config, "retries", 3)
println("函数添加后:", config)

// 检查属性
println("hasField(config, 'debug'):", hasField(config, "debug"))
println("hasField(config, 'verbose'):", hasField(config, "verbose"))

// 获取所有键
println("所有配置项:", keys(config))

// 删除属性
deleteField(config, "retries")
println("删除 retries 后:", config)

// 4. 复杂嵌套结构
println("\n【4. 复杂对象结构】")
person := {
    name: "Bob",
    age: 30
}

person.hobbies = ["reading", "gaming", "hiking"]
person.address = {
    city: "Beijing",
    district: "Chaoyang"
}
person.address.zipcode = "100000"  // 嵌套对象也支持

println("复杂对象:", person)
println("person.hobbies[1]:", person.hobbies[1])
println("person.address.city:", person.address.city)

// 5. 函数参数传递
println("\n【5. 函数中使用动态对象】")
setupUser := (u) {
    u.id = 1001
    u.status = "active"
    u.createdAt = "2025-11-20"
}

profile := {username: "bob123"}
setupUser(profile)
println("函数设置后:", profile)

// 6. 对象转换和操作
println("\n【6. 实用操作】")
data := {}
data.items = [1, 2, 3, 4, 5]
data.count = 5
data.total = 15

allKeys := keys(data)
println("数据对象键名:", allKeys)
println("键数量:", len(allKeys))

// 7. 动态删除所有属性
println("\n【7. 清空对象】")
temp := {a: 1, b: 2, c: 3}
println("原始:", temp)

tempKeys := keys(temp)
L> (tempKeys : key) {
    deleteField(temp, key)
}
println("清空后:", temp)
println("剩余键数:", len(keys(temp)))

println("\n=== 演示完成 ===")
println("\n✨ FLYUX 现在支持像 JavaScript 一样的动态对象！")
println("   - obj.prop = value  // 直接添加/修改")
println("   - setField(obj, key, val)  // 函数接口")
println("   - deleteField(obj, key)  // 删除属性")
println("   - hasField(obj, key)  // 检查存在")
println("   - keys(obj)  // 获取所有键")
