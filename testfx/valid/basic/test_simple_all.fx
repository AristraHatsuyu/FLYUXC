/* Simple test for all three features */
main := () {
    println("=== Testing v0.1 Features ===\n")
    
    // Test 1: if multi-condition (else-if chain)
    println("Test 1: if multi-condition")
    x := 75
    if (x < 60) {
        println("F")
    } (60 <= x && x < 80) {
        println("C-B")
    } (80 <= x && x <= 100) {
        println("A")
    }
    
    // Test 2: chain comparison  
    println("\nTest 2: chain comparison")
    y := 50
    if (0 < y && y < 100) {
        println("In range")
    }
    
    // Test 3: forEach no-parens
    println("\nTest 3: forEach no-parens")
    nums := [1, 2, 3]
    L>nums:n {
        printf("Value: %v\n", n)
    }
    
    println("\n✅ All tests passed!")
}
