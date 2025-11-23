/* 测试 if 多条件链式（应该是 else-if 行为） */
main := () {
    println("=== Test 1: 第一个条件为真，后续不应执行 ===")
    score := 85
    
    if (score >= 80) {
        println("A: >= 80")
    } (score >= 70) {
        println("B: >= 70 (不应该输出)")
    } (score >= 60) {
        println("C: >= 60 (不应该输出)")
    } {
        println("D: < 60 (不应该输出)")
    }
    
    println("\n=== Test 2: 第二个条件为真 ===")
    score2 := 75
    
    if (score2 >= 90) {
        println("未达到 (不应该输出)")
    } (score2 >= 70) {
        println("正确: >= 70")
    } (score2 >= 60) {
        println("不应该输出")
    } {
        println("不应该输出")
    }
    
    println("\n=== Test 3: 所有条件都不满足，执行 else ===")
    score3 := 50
    
    if (score3 >= 90) {
        println("不输出")
    } (score3 >= 80) {
        println("不输出")
    } (score3 >= 70) {
        println("不输出")
    } {
        println("正确: else 分支")
    }
}
