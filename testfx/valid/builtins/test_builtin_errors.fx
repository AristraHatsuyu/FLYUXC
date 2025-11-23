// Test error handling for built-in functions

println("=== Built-in Functions Error Handling Test ===\n")

println("--- 1. readFile ---")
content1 := readFile("/nonexistent/file.txt")
println("readFile(nonexistent):", content1)
println("Type:", typeOf(content1))
println("Program continues\n")

T> {
    content2 := readFile("/another/missing.txt")
    println("With T> without !, got:", content2)
    println("Did NOT enter catch")
} (err) {
    println("ERROR! Should NOT catch:", err)
}
println()

T> {
    content3 := readFile("/missing/file.txt")!
    println("Should NOT see this")
} (err) {
    println("Correct! Caught error:", err.message)
}
println()

println("--- 2. readBytes ---")
bytes1 := readBytes("/nonexistent.bin")
println("readBytes(nonexistent):", bytes1)
println("Type:", typeOf(bytes1))
println()

println("--- 3. readLines ---")
lines1 := readLines("/nonexistent.txt")
println("readLines(nonexistent):", lines1)
println("Type:", typeOf(lines1))
println()

println("--- 4. listDir ---")
files1 := listDir("/nonexistent/directory")
println("listDir(nonexistent):", files1)
println("Type:", typeOf(files1))
println()

println("--- 5. toInt ---")
int1 := toInt("not_a_number")
println("toInt(invalid):", int1)
println("Type:", typeOf(int1))
println()

T> {
    int2 := toInt("xyz")!
    println("Should NOT see this")
} (err) {
    println("Correct! toInt! caught:", err.message)
}
println()

println("--- 6. parseJSON ---")
json1 := parseJSON("incomplete")
println("parseJSON(invalid):", json1)
println("Type:", typeOf(json1))
println()

T> {
    json2 := parseJSON("bad json")!
    println("Should NOT see this")
} (err) {
    println("Correct! parseJSON! caught:", err.message)
}
println()

println("--- 7. Normal cases ---")

writeFile("test_temp.txt", "Hello World")
content_ok := readFile("test_temp.txt")
println("readFile(valid):", content_ok)

content_ok2 := readFile("test_temp.txt")!
println("readFile(valid)!:", content_ok2)

num_ok := toNum("123")
println("toNum(123):", num_ok)

num_ok2 := toNum("456")!
println("toNum(456)!:", num_ok2)

json_ok := parseJSON('{"a":1}')
println("parseJSON(valid):", json_ok)

json_ok2 := parseJSON('{"b":2}')!
println("parseJSON(valid)!:", json_ok2)

deleteFile("test_temp.txt")

println("\n=== All Tests Complete ===")
