/* testfx/full_demo.fx - 完整功能演示 */

// 自定义函数
calculate:<num>=(a, b){
    R>a + b * a
}

main:=(){
    print(999)  // 测试标记
    
    // ===== 1. 数组操作 =====
    arr := [100, 3, 200, 5]
    
    print(arr[0])   // 100
    print(arr[1])   // 3
    
    // 数组赋值
    arr[0] = 10
    print(arr[0])   // 10
    
    // 数组长度
    len := arr.>len
    print(len)      // 4
    
    // ===== 2. 对象操作 =====
    person := {age: 25, score: 95}
    print(person.age)     // 25
    
    // 对象成员赋值
    person.age = 26
    print(person.age)     // 26
    
    // ===== 3. 函数调用 =====
    result := calculate(10, 2)
    print(result)         // 30 (10 + 2*10)
    
    // ===== 4. if-else 语句 =====
    x := 5
    if(x > 3){
        print(1)
    }{
        print(0)
    }
    
    // ===== 5. 逻辑运算符 =====
    if(1 && 1){
        print(1)
    }{
        print(0)
    }
    
    if(0 || 1){
        print(1)
    }{
        print(0)
    }
    
    if(!0){
        print(1)
    }{
        print(0)
    }
    
    // ===== 6. 循环和 ++ =====
    counter := 0
    L>(i:=0; i<3; i++){
        counter = counter + 1
        print(counter)
    }
    
    // ===== 7. 链式调用 =====
    chain_result := arr.>len.>calculate(2)
    print(chain_result)   // 12 (4 + 2*4)
    
    print(888)  // 结束标记
}
