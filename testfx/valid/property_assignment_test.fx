// 测试直接属性赋值语法（自动添加/修改属性）
println("=== 属性赋值语法测试 ===")

// 1. 创建初始对象
println("\n1. 创建初始对象:")
person := {name: "Alice", age: 25}
println(person)

// 2. 修改已存在的属性
println("\n2. 修改已存在的属性:")
person.age = 26
println("person.age = 26 后:", person)

// 3. 添加新属性（使用点语法）
println("\n3. 添加新属性:")
person.city = "Beijing"
println("person.city = 'Beijing' 后:", person)

person.job = "Engineer"
println("person.job = 'Engineer' 后:", person)

// 4. 添加复杂类型
println("\n4. 添加复杂类型:")
person.hobbies = ["reading", "coding", "music"]
println("添加数组后:", person)

person.address = {street: "Main St", number: 123}
println("添加对象后:", person)

// 5. 空对象添加属性
println("\n5. 空对象添加属性:")
emptyObj := {}
println("初始空对象:", emptyObj)

emptyObj.x = 10
emptyObj.y = 20
emptyObj.z = 30
println("添加 x, y, z 后:", emptyObj)

// 6. 混合使用函数和直接赋值
println("\n6. 混合使用:")
config := {host: "localhost"}
println("初始 config:", config)

config.port = 8080  // 直接赋值添加
setField(config, "timeout", 3000)  // 函数添加
config.debug = true  // 直接赋值添加
println("混合添加后:", config)

// 7. 获取所有键名验证
println("\n7. 验证所有键名:")
println("person keys:", keys(person))
println("emptyObj keys:", keys(emptyObj))
println("config keys:", keys(config))

println("\n=== 测试完成 ===")
