// 🎮 FLYUX终极演示：展示所有已实现功能

println("═══════════════════════════════════════")
println("    🚀 FLYUX 终极功能演示 🚀")
println("═══════════════════════════════════════\n")

// 📝 Part 1: 字符串魔法
println("📝 === 字符串处理展示 ===")

message := "  Hello FLYUX World  "
println("原始:", message)
println("长度:", len(message))
println("修剪:", trim(message))
println("大写:", upper(trim(message)))
println("小写:", lower(trim(message)))

// 字符提取
text := "FLYUX"
println("\n字符提取:")
println("第0个:", charAt(text, 0))
println("第2个:", charAt(text, 2))

// 子字符串
println("\nsubstr测试:")
println("substr(0,3):", substr(text, 0, 3))
println("substr(3):", substr(text, 3))

// 查找和替换
data := "Hello World, Hello FLYUX"
println("\n查找和替换:")
println("原文:", data)
println("'Hello'位置:", indexOf(data, "Hello"))
println("'FLYUX'位置:", indexOf(data, "FLYUX"))
println("替换:", replace(data, "Hello", "Hi"))

// 分割和连接
csv := "apple,banana,cherry"
println("\n分割和连接:")
println("原始:", csv)
fruits := split(csv, ",")
println("分割:", fruits)
println("连接:", join(fruits, " | "))

// 🔢 Part 2: 数组操作
println("\n🔢 === 数组操作展示 ===")

arr := [10, 20, 30]
println("原始数组:", arr)
println("长度:", len(arr))

// Push和Pop
arr2 := push(arr, 40)
println("push(40):", arr2)
last := pop(arr2)
println("pop()返回:", last)

// Unshift和Shift  
arr3 := unshift(arr, 5)
println("unshift(5):", arr3)
first := shift(arr3)
println("shift()返回:", first)

// Slice
numbers := [1, 2, 3, 4, 5, 6, 7, 8, 9]
println("\n切片操作:")
println("原数组:", numbers)
println("slice(2,5):", slice(numbers, 2, 5))
println("slice(5):", slice(numbers, 5, 9))

// Concat
a := [1, 2, 3]
b := [4, 5, 6]
println("\n数组连接:")
println("数组a:", a)
println("数组b:", b)
println("concat:", concat(a, b))

// 🎯 Part 3: 类型转换
println("\n🎯 === 类型转换展示 ===")

T> {
    // 字符串转数字
    s1 := "123"
    n1 := toNum(s1)
    println("toNum('123'):", n1)
    
    // 字符串转整数
    s2 := "3.14159"
    n2 := toInt(s2)
    println("toInt('3.14159'):", n2)
    
    // 数字转字符串
    value := 42
    result := toStr(value)
    println("toStr(42):", result)
    
    // 布尔转换
    println("toBl(1):", toBl(1))
    println("toBl(0):", toBl(0))
    println("toBl('text'):", toBl("text"))
    
} (err) {
    println("转换错误:", err.message)
}

// 🎭 Part 4: 错误处理演示
println("\n🎭 === 异常处理展示 ===")

T> {
    result := charAt("abc", 10)
    println("不会执行")
} (err) {
    println("捕获错误:", err.message)
    println("错误类型:", err.type)
    println("错误代码:", err.code)
}

T> {
    invalid := toNum("not a number")
    println("不会执行")
} (err) {
    println("转换错误:", err.message)
}

// 🏗️ Part 5: 对象系统
println("\n🏗️ === 对象系统展示 ===")

person := {
    name: "Alice",
    age: 25,
    city: "Shanghai"
}

println("对象:", person)
println("姓名:", person.name)
println("年龄:", person.age)
println("城市:", person.city)

// 📊 Part 6: 综合应用
println("\n📊 === 综合应用：文本分析 ===")

T> {
    document := "The quick brown fox jumps over the lazy dog"
    println("原文:", document)
    println("长度:", len(document))
    
    // 转换
    lower_doc := lower(document)
    println("小写:", lower_doc)
    
    // 分词
    words := split(lower_doc, " ")
    println("单词数:", len(words))
    println("单词表:", words)
    
    // 查找
    query := "fox"
    pos := indexOf(lower_doc, query)
    R> (pos >= 0) {
        println("找到'", query, "'在位置:", pos)
    } {
        println("未找到")
    }
    
    // 替换
    modified := replace(document, "fox", "cat")
    println("替换后:", modified)
    
} (err) {
    println("分析错误:", err.message)
}

// 🎨 Part 7: 嵌套结构演示
println("\n🎨 === 嵌套结构演示 ===")

matrix := [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
println("矩阵:", matrix)
println("第一行:", matrix[0])
println("第二行:", matrix[1])
println("第三行:", matrix[2])

// 🔄 Part 8: 循环演示
println("\n🔄 === 循环演示 ===")

counter := 0
L> (counter = 0; counter < 5; counter = counter + 1) {
    println("计数:", counter)
}

// 🌟 Part 9: 复杂组合操作
println("\n🌟 === 复杂组合操作 ===")

T> {
    // 创建数据
    tags := "javascript,python,c++,rust,go"
    println("原始标签:", tags)
    
    // 分割
    tag_list := split(tags, ",")
    println("标签列表:", tag_list)
    
    // 转换
    enhanced := []
    L> (i = 0; i < len(tag_list); i = i + 1) {
        tag := charAt(join(tag_list, ""), i * 3)  // 简化版本
        enhanced = push(enhanced, tag)
    }
    
    // 重组
    final := join(tag_list, " | ")
    println("美化标签:", final)
    
} (err) {
    println("操作失败:", err.message)
}

// 🎊 结束
println("\n═══════════════════════════════════════")
println("   ✨ FLYUX 功能演示完成！✨")
println("   支持的特性：")
println("   • 动态类型系统")
println("   • 字符串操作 (10+ 函数)")
println("   • 数组操作 (6+ 函数)")
println("   • 异常处理 (try-catch)")
println("   • 对象系统")
println("   • 控制流 (if/loop)")
println("═══════════════════════════════════════")
