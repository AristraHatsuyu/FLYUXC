// FLYUX 功能展示

main := () {
    println("=== FLYUX 特性展示 ===\n")
    
    // 字符串处理
    println("📝 字符串:")
    text := "Hello FLYUX"
    println("原文:", text)
    println("长度:", len(text))
    println("大写:", upper(text))
    println("小写:", lower(text))
    
    parts := split(text, " ")
    println("分割:", parts)
    println("")
    
    // 数组操作
    println("📦 数组:")
    arr := [1, 2, 3]
    println("原数组:", arr)
    
    arr2 := push(arr, 4)
    println("push(4):", arr2)
    
    arr3 := unshift(arr, 0)
    println("unshift(0):", arr3)
    
    arr4 := shift(arr2)
    println("shift后:", arr4)
    
    arr5 := pop(arr2)
    println("pop后:", arr5)
    
    sliced := slice(arr2, 1, 3)
    println("slice(1,3):", sliced)
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
    }
    
    T> {
        good := toNum("999")
        println("正常:", good)
    } (e) {
        println("不应该执行")
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
    
    new_msg := replace(msg, "fox", "cat")
    println("替换:", new_msg)
    println("")
    
    println("✨ 展示完成 ✨")
    
    R> 0
}
