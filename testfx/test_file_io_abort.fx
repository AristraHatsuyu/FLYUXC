// Test file I/O abort scenarios

println("Test 1: writeFile! to invalid path should abort")
writeFile("/root/forbidden.txt", "data")!
println("This should NOT print")
