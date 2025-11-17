// 数组功能测试

main := () {
    // 测试数组字面量
    numbers := [10, 20, 30, 40, 50];
    
    // 测试索引访问
    print(numbers[0]);  // 应该输出 10
    print(numbers[2]);  // 应该输出 30
    print(numbers[4]);  // 应该输出 50
    
    // 测试在循环中访问数组
    L>(idx := 0; idx < 3; idx++) {
        print(numbers[idx]);  // 应该输出 10, 20, 30
    };
    
    // 测试使用变量作为索引
    position := 1;
    value := numbers[position];  // 应该得到 20
    print(value);
    
    // 测试表达式作为索引
    print(numbers[position + 1]);  // 应该输出 30 (numbers[2])
    
    print(999);  // 标记结束
};
