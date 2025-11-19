// 测试函数定义
println("测试动态对象与函数")

fn addField(o, key, val) {
    setField(o, key, val)
}

myObj := {name: "Alice"}
println("原始:", myObj)

addField(myObj, "age", 25)
println("添加后:", myObj)
