/* 综合测试：if 多条件 + 链式比较 */
main := () {
    println("=== 测试 1: if 多条件 + 链式比较结合 ===")
    score := 85
    
    if (score < 0 || score > 100) {
        println("分数无效")
    } (90 <= score <= 100) {
        println("优秀 (A)")
    } (80 <= score < 90) {
        println("良好 (B) ✓")
    } (70 <= score < 80) {
        println("中等 (C)")
    } (60 <= score < 70) {
        println("及格 (D)")
    } {
        println("不及格 (F)")
    }
    
    println("\n=== 测试 2: 边界条件 ===")
    val := 90
    
    if (90 <= val <= 100) {
        println("正好 90，应该在范围内 ✓")
    } {
        println("不应该执行")
    }
    
    println("\n=== 测试 3: 复杂链式比较 ===")
    a := 5
    b := 10
    c := 15
    
    if (a < b < c) {
        println("递增序列 ✓")
    } {
        println("不应该执行")
    }
    
    println("\n=== 测试 4: 链式比较失败 ===")
    x := 20
    
    if (0 < x < 10) {
        println("不应该执行")
    } {
        println("20 不在 (0, 10) 范围内 ✓")
    }
    
    println("\n所有测试完成！")
}
