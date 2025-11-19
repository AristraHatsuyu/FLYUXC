/* 简化的 demo.fx - 所有核心功能测试 */
🤪🫵:<num>=(🐙,🍄){
    R>🐙 + 🍄 * 🐙
}

main:=(){
    // 测试数组和混合类型
    😺 := ["🐘", 3, "🚄", 5]
    print("Array:", 😺[0], 😺[1], 😺[2], 😺[3])
    
    // 测试 .>len
    len := 😺.>len
    print("Length:", len)
    
    // 测试函数调用
    result := 🤪🫵(10, 20)
    print("Function result:", result)
    
    // 测试对象
    😼 := {name:"Test", value:42}
    print("Object:", 😼.name, 😼.value)
    
    // 测试循环和 ++
    L>(i := 0; i < 3; i++){
        print("Loop:", i)
    }
    
    // 测试 if
    x := 10
    if(x > 5){
        print("x is greater than 5")
    }
    
    print("Demo complete!")
}
