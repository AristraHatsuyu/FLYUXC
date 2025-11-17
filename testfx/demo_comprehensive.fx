// 混合类型系统综合演示

// === 1. 数字操作 ===
a := 10
b := 5
print(a + b)     // 15
print(a - b)     // 5
print(a * b)     // 50

// === 2. 字符串操作 ===
first := "FLY"
second := "UX"
name := first + second
print(name)      // FLYUX

// === 3. 混合类型数组 ===
data := [42, "Hello", 99, "World"]
print(data[0])   // 42
print(data[1])   // Hello
print(data[2])   // 99
print(data[3])   // World

// === 4. 条件判断 ===
x := 10
if x > 5 {
    print(100)   // 会执行
}

// === 5. 循环 ===
i := 0
loop i < 3 {
    print(i)     // 0, 1, 2
    i = i + 1
}
