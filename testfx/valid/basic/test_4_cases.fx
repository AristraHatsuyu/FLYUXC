// Complete test for ! suffix error handling

println("=== Complete ! Suffix Test ===\n")

// Case 1: With T>, without ! - should NOT be caught (error cleared)
println("--- Case 1: With T>, Without ! (should NOT catch) ---")
T> {
    result1 := toNum("abc")
    println("result1 =", result1, ", type:", typeOf(result1))
    println("Did NOT enter catch block")
} (err) {
    println("ERROR! Should NOT enter here:", err)
}
println("Program continues\n")

// Case 2: With T>, with ! - should be caught
println("--- Case 2: With T>, With ! (should catch) ---")
T> {
    result2 := toNum("xyz")!
    println("Should NOT see this line")
} (err) {
    println("Correct! Caught error:", err.message)
}
println("Program continues\n")

// Case 3: Without T>, without ! - returns typed null, program continues
println("--- Case 3: Without T>, Without ! (returns typed null) ---")
result3 := toNum("def")
println("result3 =", result3, ", type:", typeOf(result3))
println("Program continues\n")

// Case 4: Without T>, with ! - should abort immediately
println("--- Case 4: Without T>, With ! (should abort) ---")
result4 := parseJSON("invalid json")!
println("If you see this line, it did NOT abort (ERROR!)")
