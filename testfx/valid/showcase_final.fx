// 🚀 FLYUX语言特性展示 - 简洁而强大

main := () {
    println("╔═══════════════════════════════╗")
    println("║   🌟 FLYUX Showcase 🌟       ║")
    println("╚═══════════════════════════════╝\n")
    
    // 📝 字符串处理
    println("📝 字符串魔法:")
    text := "Hello FLYUX World"
    println("  原文:", text)
    println("  长度:", len(text))
    println("  大写:", upper(text))
    println("  小写:", lower(text))
    println("  截取(0,5):", substr(text, 0, 5))
    
    parts := split(text, " ")
    println("  分割:", parts)
    joined := join(parts, "-")
    println("  连接:", joined)
    
    idx := indexOf(text, "FLYUX")
    println("  'FLYUX'位置:", idx)
    
    rep := replace(text, "World", "Universe")
    println("  替换:", rep)
    
    tri := trim("  空格  ")
    println("  trim: '", tri, "'")
    println("")
    
    // 📦 数组变换
    println("📊 数组操作:")
    nums := [1, 2, 3]
    println("  原数组:", nums)
    
    nums2 := push(nums, 4)
    println("  push(4):", nums2)
    
    nums3 := unshift(nums, 0)
    println("  unshift(0):", nums3)
    
    nums4 := shift(nums2)
    println("  shift后:", nums4)
    
    nums5 := pop(nums2)
    println("  pop后:", nums5)
    
    sli := slice(nums2, 1, 3)
    println("  slice(1,3):", sli)
    
    com := concat(nums, [4, 5, 6])
    println("  concat:", com)
    println("")
    
    // 🔮 类型转换
    println("🔮 类型炼金:")
    T> {
        n := toNum("123")
        println("  toNum('123'):", n)
        
        int_val := toInt("456")
        println("  toInt('456'):", int_val)
        
        float_val := toFloat("78.9")
        println("  toFloat('78.9'):", float_val)
        
        b := toBl("true")
        println("  toBl('true'):", b)
        
        str_val := toStr(999)
        println("  toStr(999):", str_val)
    } (e) {
        println("  ❌ 错误:", e.message)
    }
    println("")
    
    // ⚡ 异常处理
    println("⚡ 异常捕获:")
    T> {
        bad := toNum("abc")
        println("  ❌ 不应该执行")
    } (err) {
        println("  ✅ 捕获:", err.message)
        println("     类型:", err.type)
    }
    
    T> {
        good := toNum("999")
        println("  ✅ 正常:", good)
    } (err) {
        println("  ❌ 不应该捕获")
    }
    println("")
    
    // 🏛️ 对象系统
    println("🏛️ 对象宇宙:")
    person := {
        name: "Alice",
        age: 25,
        city: "Beijing"
    }
    println("  对象:", person)
    println("  姓名:", person.name)
    println("  年龄:", person.age)
    println("  城市:", person.city)
    println("")
    
    // 🎨 综合应用
    println("🎨 综合演示:")
    
    T> {
        // 数据处理
        data := "10,20,30,40,50"
        numbers := split(data, ",")
        println("  数据:", numbers)
        
        // 计算总和
        sum := 0
        idx := 0
        L> (idx = 0; idx < len(numbers); idx = idx + 1) {
            val := charAt(numbers, idx)
            number := toInt(val)
            sum = sum + number
        }
        
        println("  总和:", sum)
        println("  平均:", sum / 5)
        
        // 字符串处理
        message := "  hello world  "
        processed := trim(message)
        processed = upper(processed)
        processed = replace(processed, "WORLD", "FLYUX")
        println("  处理:", processed)
        
        // 数组组合
        arr1 := [1, 2, 3]
        arr2 := [4, 5, 6]
        result := concat(arr1, arr2)
        println("  合并:", result)
        
        middle := slice(result, 2, 5)
        println("  中段:", middle)
        
    } (err) {
        println("  ❌ 错误:", err.message)
    }
    println("")
    
    println("╔═══════════════════════════════╗")
    println("║  ✨ FLYUX - 简洁优雅强大 ✨   ║")
    println("╚═══════════════════════════════╝")
}
