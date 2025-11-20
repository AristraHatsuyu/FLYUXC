// Test abort behavior when using ! outside Try-Catch

println("This line will print")
println("About to call readFile! on nonexistent file...")

content := readFile("/nonexistent/file.txt")!

println("This line should NEVER print")
