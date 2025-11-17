// FLYUX 内置函数测试
// 测试所有64个内置函数的词法识别

// ===== 输入输出 =====
print("Hello, FLYUX!")
name := input("Enter name: ")
content := readFile("data.txt")
writeFile("output.txt", "test")

// ===== 字符串操作 =====
len := length("Hello")
sub := substr("Hello", 1, 3)
pos := indexOf("Hello", "l")
replaced := replace("Hello", "l", "r")
parts := split("a,b,c", ",")
joined := join([1, 2, 3], ",")
upper := toUpper("hello")
lower := toLower("HELLO")
trimmed := trim("  hello  ")
starts := startsWith("Hello", "He")
ends := endsWith("Hello", "lo")

// ===== 数学函数 =====
absolute := abs(-5)
floored := floor(3.7)
ceiled := ceil(3.2)
rounded := round(3.14159, 2)
squareRoot := sqrt(16)
power := pow(2, 3)
minimum := min(1, 5, 3)
maximum := max(1, 5, 3)
rand := random()
randInt := randomInt(1, 10)

// ===== 数组操作 =====
arr := [1, 2, 3]
push(arr, 4)
last := pop(arr)
first := shift(arr)
unshift(arr, 0)
sliced := slice(arr, 1, 3)
concatenated := concat([1, 2], [3, 4])
reverse(arr)
sort(arr)
filtered := filter(arr, (x) { R> x > 2 })
mapped := map(arr, (x) { R> x * 2 })
reduced := reduce(arr, (a, b) { R> a + b }, 0)
found := find(arr, (x) { R> x > 2 })
idx := indexOf(arr, 2)
has := includes(arr, 2)

// ===== 对象操作 =====
obj := {a: 1, b: 2, c: 3}
k := keys(obj)
v := values(obj)
e := entries(obj)
hasA := hasKey(obj, "a")
merged := merge({a: 1}, {b: 2})
cloned := clone(obj)
deepCloned := deepClone(obj)

// ===== 类型转换和检查 =====
num := toNum("123")
str := toStr(123)
bool := toBl(1)
type := typeOf(123)
isN := isNum(123)
isS := isStr("hello")
isB := isBl(true)
isA := isArr([1, 2])
isO := isObj({a: 1})
isNu := isNull(null)
isU := isUndef(undef)

// ===== 时间函数 =====
timestamp := now()
sleep(100)
date := dateStr()

// ===== 实用工具 =====
assert(true, "must be true")
// exit(0)  // 注释掉以便测试继续
numbers := range(0, 5)

print("All builtin functions tokenized successfully!")
