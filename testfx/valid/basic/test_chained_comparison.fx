/* 测试链式比较运算符 - 包含边界和失败情况 */
main := () {
    println("=== Test 1: Value in range (should pass) ===")
    score := 75
    if (0 < score <= 100) {
        println("PASS: 75 is between 0 and 100")
    } {
        println("FAIL: Should have passed!")
    }
    
    println("\n=== Test 2: Value out of range (should fail) ===")
    score2 := 150
    if (0 < score2 <= 100) {
        println("FAIL: 150 should not pass!")
    } {
        println("PASS: 150 is correctly rejected")
    }
    
    println("\n=== Test 3: Value below range (should fail) ===")
    x := 11
    if (0 < x < 10) {
        println("FAIL: 11 should not be < 10!")
    } {
        println("PASS: 11 is correctly > 10")
    }
    
    println("\n=== Test 4: Negative value (should fail) ===")
    neg := -5
    if (0 < neg <= 100) {
        println("FAIL: -5 should not pass!")
    } {
        println("PASS: -5 is correctly rejected")
    }
    
    println("\n=== Test 5: Boundary test - exact lower ===")
    at_zero := 0
    if (0 < at_zero <= 100) {
        println("FAIL: 0 is not > 0!")
    } {
        println("PASS: 0 <= condition works")
    }
    
    println("\n=== Test 6: Boundary test - exact upper ===")
    at_hundred := 100
    if (0 < at_hundred <= 100) {
        println("PASS: 100 is in range")
    } {
        println("FAIL: 100 should pass!")
    }
    
    println("\n=== Test 7: Complex chain (should fail) ===")
    a := 2
    b := 1  // b < a, breaks ascending order
    c := 8
    if (a < b < c) {
        println("FAIL: 2 < 1 should be false!")
    } {
        println("PASS: Non-ascending correctly detected")
    }
    
    println("\n=== Test 8: Chain with &&, value out of range ===")
    val := 11
    valid := true
    if (0 < val < 10 && valid) {
        println("FAIL: 11 is not < 10!")
    } {
        println("PASS: Chain correctly evaluated as false")
    }
    
    println("\nAll tests completed!")
}
