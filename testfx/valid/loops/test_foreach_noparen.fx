/* 测试 ForEach 循环 - 带括号和不带括号两种语法 */
main := () {
    println("=== Test 1: Repeat Loop ===")
    L>[5]{ println("Repeat") }
    
    println("\n=== Test 2: For Loop ===")
    L>(i := 0; i < 3; i++){ println(i) }
    
    println("\n=== Test 3: ForEach with parentheses ===")
    nums1 := [10, 20, 30]
    L>(nums1:item){ println(item) }
    
    println("\n=== Test 4: ForEach without parentheses ===")
    nums2 := [40, 50, 60]
    L>nums2:item{ println(item) }
    
    println("\n=== Test 5: Complex forEach without parentheses ===")
    names := ["Alice", "Bob", "Charlie"]
    L>names:name{ 
        println("Hello,", name)
    }
    
    println("\nTest completed!")
}
