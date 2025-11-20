// 测试第二批新增函数

println("========== 时间函数测试 ==========")

println("time() - Unix时间戳:")
t := time()
println(t)
println("类型:", typeOf(t))

println("\ndate() - 当前日期时间:")
d := date()
println(d)
println("类型:", typeOf(d))

println("\nsleep(0.5) - 休眠0.5秒:")
println("开始...")
sleep(0.5)
println("结束!")

println("\n========== 系统操作测试 ==========")

println("getEnv('PATH') - 获取环境变量:")
path := getEnv("PATH")
if (path != null) {
    print("PATH存在 (长度:", len(path), ")")
} {
    print("PATH不存在")
}
println()
println("\ngetEnv('NONEXISTENT') - 不存在的变量:")
nonexist := getEnv("NONEXISTENT")
println(nonexist)
println("类型:", typeOf(nonexist))

println("\nsetEnv/getEnv - 设置和读取:")
result := setEnv("FLYUX_TEST", "Hello")
println("设置结果:", result)
value := getEnv("FLYUX_TEST")
println("读取值:", value)

println("\n========== 实用工具测试 ==========")

println("isNaN(10):")
println(isNaN(10))

println("\nisNaN(0/0):")
nan_val := 0 / 0
println(isNaN(nan_val))

println("\nisFinite(100):")
println(isFinite(100))

println("\nisFinite(1/0):")
inf_val := 1 / 0
println(isFinite(inf_val))

println("\nclamp(5, 0, 10):")
println(clamp(5, 0, 10))

println("\nclamp(-5, 0, 10):")
println(clamp(-5, 0, 10))

println("\nclamp(15, 0, 10):")
println(clamp(15, 0, 10))

println("\nclamp(7, 3, 8):")
println(clamp(7, 3, 8))

println("\n========== 错误处理测试 ==========")

println("sleep('bad') - 类型错误:")
bad_sleep := sleep("bad")
println(bad_sleep)

println("\nclamp(5, 10, 5) - min > max:")
bad_clamp := clamp(5, 10, 5)
println(bad_clamp)

println("\n所有测试完成!")
