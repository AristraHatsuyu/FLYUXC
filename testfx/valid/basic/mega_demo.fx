// 🎨 FLYUX 语言终极演示：超浓缩功能展示
// 用最少代码展示最多特性！

println("╔══════════════════════════════════════╗")
println("║   🚀 FLYUX 全功能演示                ║")
println("╚══════════════════════════════════════╝\n")

// 🌈 Part 1: 字符串炼金术
println("🌈 === 字符串魔法 ===")
spell := "  ✨FLYUX✨  "
println("原始咒语:", spell)
println("净化:", trim(spell))
println("强化:", upper(trim(spell)))
println("温和:", lower(trim(spell)))
println("长度:", len(spell), "→", len(trim(spell)))

// 拆解与重组
code := "F-L-Y-U-X"
parts := split(code, "-")
println("\n密码分解:", parts)
println("密码重组:", join(parts, ""))
println("美化:", join(parts, " • "))

// 查找与替换
motto := "Make Programming Fun Again"
println("\n宣言:", motto)
println("查找'Fun':", indexOf(motto, "Fun"))
println("改造:", replace(motto, "Fun", "🎮FUN🎮"))

// 📦 Part 2: 数组魔方
println("\n📦 === 数组操作 ===")
cube := [1, 2, 3, 4, 5]
println("魔方:", cube)

// 变形术
expanded := push(cube, 999)
println("扩展:", expanded)
println("切割[1:4]:", slice(cube, 1, 4))

bonus := [10, 20]
merged := concat(cube, bonus)
println("合并:", merged)

// 📊 Part 3: 数据炼金
println("\n📊 === 类型炼金 ===")

T> {
    // 转换链
    alpha := "42"
    beta := toNum(alpha)
    gamma := toStr(beta)
    delta := toInt("3.99")
    
    println("字符串→数字:", alpha, "→", beta)
    println("数字→字符串:", beta, "→", gamma)
    println("截断:", "3.99", "→", delta)
    println("布尔值:", toBl(beta), toBl(0))
    
} (err) {
    println("⚠️ 炼金失败:", err.message)
}

// 🎯 Part 4: 异常捕手
println("\n🎯 === 异常处理 ===")

T> {
    danger := charAt("abc", 999)
} (err) {
    println("✓ 捕获:", err.type, "-", err.message)
}

T> {
    poison := toNum("xyz#@!")
} (err) {
    println("✓ 捕获:", err.type, "- 代码", err.code)
}

// 🏛️ Part 5: 对象宇宙
println("\n🏛️ === 对象系统 ===")
hero := {
    name: "Alice",
    level: 99,
    power: 9527,
    skills: ["fire", "ice", "wind"]
}
println("英雄:", hero)
println("姓名:", hero.name, "| 等级:", hero.level)
println("力量:", hero.power)
println("技能:", hero.skills)

// 🔮 Part 6: 综合炼金术
println("\n🔮 === 综合炼术 ===")

T> {
    // 数据提取与分析
    raw := "JavaScript,Python,C++,Rust,Go"
    println("原始数据:", raw)
    
    // 解析
    langs := split(raw, ",")
    total := len(langs)
    println("语言数量:", total)
    println("列表:", langs)
    
    // 转换
    lower_langs := lower(raw)
    println("标准化:", lower_langs)
    
    // 定位
    target := "rust"
    pos := indexOf(lower_langs, target)
    R> (pos >= 0) {
        println("✓ 找到", target, "在位置", pos)
    } {
        println("✗ 未找到", target)
    }
    
    // 替换
    branded := replace(raw, "C++", "C++🔥")
    println("品牌化:", branded)
    
    // 重组
    formatted := join(langs, " | ")
    println("格式化:", formatted)
    
} (err) {
    println("⚠️ 操作失败:", err.message)
}

// 🎪 Part 7: 嵌套异常捕获
println("\n🎪 === 异常嵌套 ===")

T> {
    println("外层开始...")
    
    T> {
        println("  内层开始...")
        bad := toNum("###")
        println("  不应该执行")
    } (inner) {
        println("  ✓ 内层捕获:", inner.type)
        println("  继续执行...")
    }
    
    println("外层成功完成")
    
} (outer) {
    println("✗ 外层失败:", outer.message)
}

// 🌟 Part 8: 高级应用 - URL解析器
println("\n🌟 === URL解析器 ===")

T> {
    url := "https://flyux.lang/docs?page=1&type=guide"
    println("URL:", url)
    
    // 提取协议
    proto_end := indexOf(url, "://")
    protocol := substr(url, 0, proto_end)
    println("协议:", protocol)
    
    // 提取域名
    domain_start := proto_end + 3
    path_start := indexOf(url, "/docs")
    domain := substr(url, domain_start, path_start - domain_start)
    println("域名:", domain)
    
    // 提取路径
    query_start := indexOf(url, "?")
    path := substr(url, path_start, query_start - path_start)
    println("路径:", path)
    
    // 解析查询参数
    query := substr(url, query_start + 1)
    params := split(query, "&")
    println("参数:", params)
    
} (err) {
    println("⚠️ 解析失败:", err.message)
}

// 💎 Part 9: 数据处理管道
println("\n💎 === 数据管道 ===")

T> {
    // 原始数据
    raw_data := "  APPLE, banana ,  CHERRY  "
    println("输入:", raw_data)
    
    // 管道处理
    step1 := trim(raw_data)
    step2 := lower(step1)
    step3 := split(step2, ",")
    
    println("步骤1-清理:", step1)
    println("步骤2-小写:", step2)
    println("步骤3-分割:", step3)
    
    // 每个元素清理
    cleaned := []
    idx := 0
    part1 := trim(charAt(join(step3, ""), 0))  // 简化示例
    cleaned = push(cleaned, part1)
    
    println("最终结果:", cleaned)
    
} (err) {
    println("⚠️ 管道失败:", err.message)
}

// 🎭 Part 10: 文本加密器
println("\n🎭 === 简易加密器 ===")

T> {
    plaintext := "SECRET"
    println("明文:", plaintext)
    
    // 转换1: 小写
    encrypted := lower(plaintext)
    
    // 转换2: 字符替换
    encrypted = replace(encrypted, "s", "5")
    encrypted = replace(encrypted, "e", "3")
    encrypted = replace(encrypted, "r", "r")
    encrypted = replace(encrypted, "c", "c")
    encrypted = replace(encrypted, "t", "7")
    
    // 转换3: 大写
    encrypted = upper(encrypted)
    
    println("密文:", encrypted)
    
} (err) {
    println("⚠️ 加密失败:", err.message)
}

// 🎊 尾声
println("\n╔══════════════════════════════════════╗")
println("║  ✨ 功能全览完成！✨                ║")
println("║                                      ║")
println("║  📝 字符串: len/upper/lower/trim     ║")
println("║            split/join/replace       ║")
println("║            charAt/substr/indexOf    ║")
println("║  📦 数组:   push/pop/shift/unshift   ║")
println("║            slice/concat             ║")
println("║  🔄 转换:   toNum/toStr/toInt/toBl   ║")
println("║  🎯 异常:   T>{} (err){}             ║")
println("║  🏛️  对象:   {key: value}            ║")
println("║  💡 控制:   R>{}/{} L>(){}           ║")
println("║                                      ║")
println("║  代码精简 | 功能强大 | 类型动态     ║")
println("╚══════════════════════════════════════╝")
