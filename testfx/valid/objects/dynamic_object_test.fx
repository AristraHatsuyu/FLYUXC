// 动态对象功能测试
// 测试运行时添加、修改、删除对象属性

println("=== 动态对象测试 ===")

// 1. 创建一个空对象
myObj := {}
println("1. 初始空对象:")
println(myObj)

// 2. 动态添加属性
println("\n2. 动态添加属性:")
setField(myObj, "name", "Alice")
setField(myObj, "age", 25)
setField(myObj, "city", "Beijing")
println(myObj)

// 3. 修改已存在的属性
println("\n3. 修改属性:")
setField(myObj, "age", 26)
println(myObj)

// 4. 检查属性是否存在
println("\n4. 检查属性:")
println("hasField(myObj, 'name'):", hasField(myObj, "name"))
println("hasField(myObj, 'email'):", hasField(myObj, "email"))

// 5. 获取所有键名
println("\n5. 获取所有键名:")
allKeys := keys(myObj)
println("keys:", allKeys)

// 6. 删除属性
println("\n6. 删除属性:")
deleted := deleteField(myObj, "city")
println("删除 'city':", deleted)
println(myObj)

// 7. 尝试删除不存在的属性
println("\n7. 删除不存在的属性:")
deleted2 := deleteField(myObj, "email")
println("删除 'email':", deleted2)

// 8. 动态添加复杂类型
println("\n8. 添加复杂类型:")
setField(myObj, "hobbies", ["reading", "coding", "music"])
setField(myObj, "address", {street: "Main St", number: 123})
println(myObj)

// 9. 获取更新后的键名
println("\n9. 更新后的键名:")
newKeys := keys(myObj)
println("keys:", newKeys)

// 10. 动态对象作为函数参数
println("\n10. 动态对象作为函数参数:")
addFieldFunc := (o, key, val) {
    setField(o, key, val)
}

addFieldFunc(myObj, "country", "China")
println(myObj)

// 11. 测试对象创建时就有属性，然后动态修改
println("\n11. 初始对象动态修改:")
person := {name: "Bob", age: 30}
println("原始:", person)
person.age = 31  // 使用直接赋值修改
person.job = "Engineer"  // 使用直接赋值添加
println("修改后:", person)

// 12. 删除所有属性
println("\n12. 删除所有属性:")
deleteField(person, "name")
deleteField(person, "age")
deleteField(person, "job")
println("删除所有属性后:", person)
println("keys:", keys(person))

println("\n=== 测试完成 ===")
