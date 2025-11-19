// FLYUX 完整展示

main := () {
    println("=== FLYUX Showcase ===\n")
    
    // 字符串
    println("📝 字符串:")
    text := "Hello FLYUX"
    println("文本:", text)
    println("长度:", len(text))
    println("大写:", upper(text))
    println("小写:", lower(text))
    
    parts := split(text, " ")
    println("分割:", parts)
    println("")
    
    // 数组
    println("📦 数组:")
    arr := [1, 2, 3]
    println("原数组:", arr)
    
    arr2 := push(arr, 4)
    println("push(4):", arr2)
    
    arr3 := shift(arr2)
    println("shift:", arr3)
    
    arr4 := pop(arr2)
    println("pop:", arr4)
    
    sliced := slice(arr2, 1, 3)
    println("slice(1,3):", sliced)
    
    combined := concat(arr, [4, 5])
    println("concat:", combined)
    println("")
    
    // 类型转换
    println("🔮 类型转换:")
    T> {
        n := toNum("123")
        println("toNum:", n)
        
        i := toInt("456")
        println("toInt:", i)
        
        f := toFloat("7.89")
        println("toFloat:", f)
    } (e) {
        println("错误:", e.message)
    }
    println("")
    
    // 异常处理
    println("⚡ 异常:")
    T> {
        bad := toNum("abc")
    } (e) {
        println("捕获:", e.message)
        println("类型:", e.type)
    }
    println("")
    
    // 对象
    println("🏛️ 对象:")
    p := {x: 10, y: 20}
    println("对象:", p)
    println("x =", p.x)
    println("y =", p.y)
    println("")
    
    // 搜索替换
    println("🔍 搜索:")
    msg := "quick fox"
    pos := indexOf(msg, "fox")
    println("位置:", pos)
    
    newmsg := replace(msg, "fox", "cat")
    println("替换:", newmsg)
    println("")
    
    println("✨ FLYUX - 简洁优雅强大 ✨")
    
    R> 0
}
