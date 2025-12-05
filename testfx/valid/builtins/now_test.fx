// 测试 now() 毫秒级时间戳
main := () {
    println("=== now() 函数测试 ===")
    
    // now() - 获取毫秒级时间戳
    println("\n--- now() vs time() ---")
    msTimestamp := now()
    secTimestamp := time()
    
    println("now()  毫秒级时间戳: ", msTimestamp)
    println("time() 秒级时间戳:   ", secTimestamp)
    
    // 验证 now() 大约是 time() * 1000
    expectedMs := secTimestamp * 1000
    diff := msTimestamp - expectedMs
    println("\n差值(ms): ", diff, " (应该在 0-1000 之间)")
    
    // 精度测试
    println("\n--- 精度测试 ---")
    start := now()
    
    // 做一些简单计算
    sum := 0
    L> (i := 0; i < 10000; i = i + 1) {
        sum = sum + i
    }
    
    end := now()
    elapsed := end - start
    println("计算 10000 次加法耗时: ", elapsed, " ms")
    
    // sleep 测试
    println("\n--- sleep 精度测试 ---")
    start2 := now()
    sleep(0.1)  // sleep 100ms
    end2 := now()
    sleepElapsed := end2 - start2
    println("sleep(0.1) 实际耗时: ", sleepElapsed, " ms (应该约 100ms)")
    
    println("\n=== 测试完成 ===")
}
