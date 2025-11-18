/* testfx/demo_numeric.fx - 纯数字版本的 demo */
🤪🫵:<num>=(🐙,🍄){
    R>🐙 + 🍄 * 🐙
}

main:=(){
    // 使用数字代替字符串
    😺 := [100, 3, 200, 5]
    
    // 测试数组索引
    print(😺[0])
    print(😺[1])
    
    // 测试对象
    😼 := {age: 25, score: 95}
    print(😼.age)
    
    // 测试函数调用
    result := 🤪🫵(10, 2)
    print(result)
    
    // 测试 if-else
    x := 5
    if(x > 3){
        print(1)
    }{
        print(0)
    }
    
    // 测试循环
    counter := 0
    L>(i:=0; i<3; i++){
        counter = counter + 1
        print(counter)
    }
    
    print(999)
}
