// 测试数组求和

main := () {
    println("=== 数组求和测试 ===\n")
    
    data := "10,20,30"
    parts := split(data, ",")
    println("数组:", parts)
    println("长度:", len(parts))
    
    // 手动访问
    v0 := charAt(parts, 0)
    println("parts[0]:", v0)
    
    n0 := toInt(v0)
    println("toInt(v0):", n0)
    
    // 循环求和
    total := 0
    i := 0
    L> (i = 0; i < len(parts); i = i + 1) {
        val := charAt(parts, i)
        println("  val:", val)
        
        number := toInt(val)
        println("  number:", number)
        total = total + number
        println("  total:", total)
    }
    
    println("\n最终总和:", total)
    
    R> 0
}
