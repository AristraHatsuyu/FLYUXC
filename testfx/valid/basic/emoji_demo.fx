// 🎮 FLYUX语言终极演示：表情符号密码生成器 & 数据分析系统 🚀

println("╔═══════════════════════════════════════╗")
println("║  🎯 FLYUX 超级演示程序 v1.0          ║")
println("║  展示：类型转换 | 数组 | 字符串      ║")
println("║       对象 | 异常处理 | 控制流       ║")
println("╚═══════════════════════════════════════╝\n")

// 🔐 Part 1: 表情符号密码生成器
println("🔐 === 表情符号密码生成器 ===")

emojis := split("🌟💎🔥💧🌸⚡🌙✨", "")
println("可用表情库:", emojis)

T> {
    // 生成随机密码
    password := ""
    codes := []
    
    i := 0
    L> (i < 6) {
        idx := toInt(toNum(len(emojis)) * 0.123 * (i + 1)) % len(emojis)
        emoji := charAt(join(emojis, ""), idx)
        password = password + emoji
        codes = push(codes, idx)
        i = i + 1
    }
    
    println("生成的密码:", password)
    println("密码编码:", codes)
    
    // 验证密码强度
    strength := len(password)
    println("密码强度:", strength, "级")
    
} (err) {
    println("❌ 生成失败:", err.message)
}

// 📊 Part 2: 数据分析与可视化
println("\n📊 === 数据统计分析 ===")

T> {
    // 模拟用户数据
    data := "Hello FLYUX Language! Programming is FUN!"
    println("原始数据:", data)
    
    // 数据清洗
    cleaned := trim(lower(data))
    println("清洗后:", cleaned)
    
    // 分割单词
    words := split(cleaned, " ")
    println("单词数量:", len(words))
    println("单词列表:", words)
    
    // 查找关键词
    keyword := "flyux"
    pos := indexOf(cleaned, keyword)
    
    R> (pos >= 0) {
        println("✅ 找到关键词'", keyword, "'在位置:", pos)
    } {
        println("❌ 未找到关键词")
    }
    
    // 替换文本
    modified := replace(data, "FLYUX", "🚀FLYUX🚀")
    println("修改后:", modified)
    
} (err) {
    println("❌ 分析失败:", err.message, "类型:", err.type)
}

// 🎨 Part 3: 数组魔法操作
println("\n🎨 === 数组魔法操作 ===")

T> {
    // 创建魔法数组
    magic := [1, 2, 3, 4, 5]
    println("原始魔法:", magic)
    
    // 添加能量
    magic = push(magic, 100)
    println("加能量:", magic)
    
    // 提取最强力量
    power := pop(magic)
    println("提取力量:", power, "剩余:", magic)
    
    // 添加新的起点
    magic = unshift(magic, 0)
    println("新起点:", magic)
    
    // 获取核心段
    core := slice(magic, 1, 4)
    println("核心段:", core)
    
    // 合并能量
    extra := [10, 20]
    combined := concat(magic, extra)
    println("合并能量:", combined)
    
    // 计算统计
    total := len(combined)
    println("总能量单元:", total)
    
} (err) {
    println("❌ 魔法失败:", err.message)
}

// 🎯 Part 4: 类型转换炼金术
println("\n🎯 === 类型转换炼金术 ===")

T> {
    // 字符串 → 数字
    numStr := "42"
    num := toNum(numStr)
    intNum := toInt("3.14159")
    
    println("字符串", numStr, "→ 数字", num)
    println("圆周率 → 整数", intNum)
    
    // 数字 → 字符串
    result := toStr(num) + " 是答案"
    println("答案字符串:", result)
    
    // 布尔转换
    truth := toBl(1)
    lie := toBl(0)
    println("真值:", truth, "假值:", lie)
    
} (err) {
    println("❌ 炼金失败:", err.message)
    println("   错误代码:", err.code)
    println("   错误类型:", err.type)
}

// 🌈 Part 5: 字符串艺术画廊
println("\n🌈 === 字符串艺术画廊 ===")

art := "  ★ FLYUX ★  "
println("原始艺术:", "[" + art + "]")

// 修剪
trimmed := trim(art)
println("修剪后:", "[" + trimmed + "]")

// 大小写变换
upper_art := upper(trimmed)
lower_art := lower(trimmed)
println("大写:", upper_art)
println("小写:", lower_art)

// 字符提取
firstChar := charAt(trimmed, 0)
println("首字符:", firstChar)

// 子字符串
sub := substr(trimmed, 2, 5)
println("子串:", sub)

// 🎪 Part 6: 循环表演秀
println("\n🎪 === 循环表演秀 ===")

println("倒计时:")
count := 5
L> (count > 0) {
    println("  ", count, "...")
    count = count - 1
}
println("  🎉 发射!")

// For循环展示
println("\n矩阵输出:")
i := 0
L> (i < 3) {
    line := ""
    j := 0
    L> (j < 5) {
        line = line + "█ "
        j = j + 1
    }
    println(line)
    i = i + 1
}

// 🏆 Part 7: 对象系统展示
println("\n🏆 === 对象系统展示 ===")

user := { 
    name: "Alice",
    score: 9527,
    level: "Master",
    active: true
}

println("用户对象:", user)
println("用户名:", user.name)
println("分数:", user.score)
println("等级:", user.level)
println("活跃:", user.active)

// 🎭 Part 8: 嵌套异常处理
println("\n🎭 === 多层异常捕获 ===")

T> {
    println("外层开始...")
    
    T> {
        println("  内层开始...")
        
        // 故意触发错误
        badNum := toNum("这不是数字XYZ")
        println("  这行不会执行")
        
    } (innerErr) {
        println("  ⚠️  内层捕获:", innerErr.message)
        println("  继续外层...")
    }
    
    println("外层继续正常执行")
    
} (outerErr) {
    println("❌ 外层捕获:", outerErr.message)
}

// 🌟 Part 9: 综合应用 - 文本加密
println("\n🌟 === 简易文本加密器 ===")

T> {
    secret := "HELLO"
    println("原文:", secret)
    
    // 转小写
    lower_secret := lower(secret)
    
    // 反转
    chars := split(lower_secret, "")
    reversed := join(chars, "")
    
    // 替换字符
    encrypted := replace(replaced:= replace(replaced:= reversed, "h", "🔒"), "l", "🔑")
    
    println("加密:", encrypted)
    
} (err) {
    println("❌ 加密失败:", err.message)
}

// 🎊 结束
println("\n╔═══════════════════════════════════════╗")
println("║  ✨ 演示完成！FLYUX 功能全览 ✨     ║")
println("║  支持：异常处理 | 动态类型          ║")
println("║        字符串操作 | 数组操作        ║")
println("║        对象系统 | 控制流            ║")
println("╚═══════════════════════════════════════╝")
