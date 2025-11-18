// 综合测试：数组、对象和作用域
main := () {
    print(999);  // 开始标记
    
    // 测试数组
    numbers := [10, 20, 30];
    print(numbers[0]);  // 应该输出 10
    
    numbers[0] = 100;   // 数组赋值
    print(numbers[0]);  // 应该输出 100
    
    // 测试对象
    person := {age: 25, score: 95};
    print(person.age);    // 应该输出 25
    
    person.age = 26;      // 对象成员赋值
    print(person.age);    // 应该输出 26
    
    // 测试作用域（使用 = 而不是 :=）
    counter := 0;
    L>(i := 0; i < 3; i++) {
        counter = counter + 1;
        print(counter);   // 应该输出 1, 2, 3
    };
    print(counter);       // 应该输出 3
    
    // 测试数组在循环中访问
    L>(j := 0; j < 3; j++) {
        print(numbers[j]);  // 应该输出 100, 20, 30
    };
    
    print(888);  // 结束标记
};
