/* 🚀 FLYUX语言特性展示 - 简洁而强大 */

main := () {
    println("=== 🌟 FLYUX Showcase ===\n")
    
    /* 🎨 Part 1: 字符串魔法 */
    println("📝 字符串处理:")
    text := "Hello FLYUX World"
    println("原文:", text)
    println("长度:", len(text))
    println("大写:", upper(text))
    println("小写:", lower(text))
    println("截取(0,5):", substr(text, 0, 5))
    
    parts := split(text, " ")
    println("分割:", parts)
    joined := join(parts, "-")
    println("连接:", joined)
    println("")
    
    /* 📦 Part 2: 数组操作 */
    println("📊 数组变换:")
    nums := [1, 2, 3]
    println("原数组:", nums)
    
    nums2 := push(nums, 4)
    println("push(4):", nums2)
    
    nums3 := unshift(nums, 0)
    println("unshift(0):", nums3)
    
    nums4 := shift(nums2)
    println("shift后:", nums4)
    
    nums5 := pop(nums2)
    println("pop后:", nums5)
    
    sliced := slice(nums2, 1, 3)
    println("slice(1,3):", sliced)
    
    combined := concat(nums, [4, 5, 6])
    println("concat:", combined)
    println("")
    
    /* 🔢 Part 3: 类型转换 */
    println("🔮 类型转换:")
    s := "123"
    n := toNum(s)
    println("toNum('123'):", n)
    
    i := toInt("456")
    println("toInt('456'):", i)
    
    f := toFloat("78.9")
    println("toFloat('78.9'):", f)
    
    b := toBl("true")
    println("toBl('true'):", b)
    
    back := toStr(n)
    println("toStr(123):", back)
    println("")
    
    /* 🎯 Part 4: 异常处理 */
    println("⚡ 异常处理:")
    
    T> {
        result := toNum("abc")
        println("不应该执行")
    } > err {
        println("捕获错误:", err.message)
        println("错误代码:", err.code)
    }
    
    T> {
        valid := toNum("999")
        println("正常执行:", valid)
    } > err {
        println("不应该捕获")
    }
    println("")
    
    /* 🏛️ Part 5: 对象系统 */
    println("🎪 对象操作:")
    person := {
        name: "Alice",
        age: 25,
        city: "Beijing"
    }
    println("姓名:", person.name)
    println("年龄:", person.age)
    println("城市:", person.city)
    println("")
    
    /* 🎭 Part 6: 综合应用 - 简单数据处理 */
    println("💎 综合示例 - 数据处理:")
    
    data := "apple,banana,cherry"
    fruits := split(data, ",")
    println("水果列表:", fruits)
    
    count := len(fruits)
    println("水果数量:", count)
    
    i := 0
    L> (i := 0; i < count; i := i + 1) {
        fruit := charAt(fruits, i)
        println("  ", i, ":", fruit)
    }
    println("")
    
    /* 🌈 Part 7: 字符串搜索与替换 */
    println("🔍 搜索与替换:")
    message := "The quick brown fox"
    idx := indexOf(message, "quick")
    println("'quick'位置:", idx)
    
    replaced := replace(message, "fox", "cat")
    println("替换后:", replaced)
    
    trimmed := trim("  spaces  ")
    println("trim后: '", trimmed, "'")
    println("")
    
    /* ⚡ Part 8: 嵌套数组操作 */
    println("🎨 嵌套操作:")
    matrix := [[1, 2], [3, 4], [5, 6]]
    println("矩阵:", matrix)
    
    row1 := charAt(matrix, 0)
    println("第一行:", row1)
    
    elem := charAt(row1, 1)
    println("元素[0][1]:", elem)
    println("")
    
    /* 🎯 Part 9: 链式操作 */
    println("⚙️ 链式调用:")
    chain := "  HELLO  "
    chain := trim(chain)
    chain := lower(chain)
    chain := replace(chain, "hello", "world")
    println("链式结果:", chain)
    println("")
    
    /* 🏆 Final: 综合测试 */
    println("🏆 综合测试:")
    
    T> {
        values := ["10", "20", "30"]
        sum := 0
        
        idx := 0
        L> (idx := 0; idx < len(values); idx := idx + 1) {
            val := charAt(values, idx)
            number := toInt(val)
            sum := sum + number
        }
        
        println("字符串数组求和:", sum)
        println("平均值:", sum / 3)
    } > err {
        println("错误:", err.message)
    }
    
    println("\n✨ FLYUX - 简洁、强大、优雅 ✨")
    
    ret 0
}
