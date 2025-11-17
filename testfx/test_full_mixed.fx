/* testfx/test_full_mixed.fx - 完整混合类型测试 */

main:=(){
    print(999)  // 开始标记
    
    // 数字
    x := 42
    print(x)
    
    // 将来支持：字符串
    // s := "Hello"
    // print(s)
    
    // 混合数组（目前只能用数字）
    arr := [1, 2, 3]
    print(arr[0])
    print(arr[1])
    print(arr[2])
    
    // 数组操作
    arr[0] = 10
    print(arr[0])
    
    // 对象
    person := {age: 25, score: 95}
    print(person.age)
    
    // 将来支持：对象中的数组
    // data := {nums: [1, 2, 3]}
    // print(data.nums[0])
    
    print(888)  // 结束标记
}
