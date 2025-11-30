println("=== Break Test 1: L> [count] ===")
i := 0
L> [10] {
    i := i + 1
    if (i == 3) { B> }
}
println("i after break1 =", i)

println("\n=== Break Test 2: L> (for) ===")
sum := 0
L> (j := 0; j < 5; j++) {
    if (j == 2) { B> }
    sum := sum + j
}
println("sum after break2 =", sum)

println("\n=== Break Test 3: L> foreach ===")
arr := [1, 2, 3, 4]
res := 0
L> arr : v {
    if (v == 3) { B> }
    res := res + v
}
println("res after break3 =", res)
