// 测试时间相关的内置函数
// FLYUX 支持的时间函数：time(), date(), sleep()

main := () {
    println("=== FLYUX 时间函数测试 ===")
    
    // time() - 获取当前 Unix 时间戳（秒）
    println("\n--- time() ---")
    timestamp := time()
    println("当前 Unix 时间戳: ", timestamp)
    
    // date() - 获取当前日期时间字符串 (格式: "YYYY-MM-DD HH:MM:SS")
    println("\n--- date() ---")
    currentDate := date()
    println("当前日期时间: ", currentDate)
    
    // sleep(seconds) - 休眠指定秒数
    println("\n--- sleep() ---")
    println("休眠 1 秒...")
    startTime := time()
    sleep(1)
    endTime := time()
    println("休眠完成，耗时: ", endTime - startTime, " 秒")
    
    // 测试小数秒
    println("\n休眠 0.5 秒...")
    startTime2 := time()
    sleep(0.5)
    endTime2 := time()
    println("休眠完成")
    
    println("\n=== 测试完成 ===")
}
