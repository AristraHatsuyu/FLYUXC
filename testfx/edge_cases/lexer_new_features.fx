/* 测试新增的Lexer功能 */

// 1. 浮点数测试
pi := 3.14159
half := 0.5
large := 1.23e10
small := 4.56e-8
neg_exp := 7.89E-3
pos_exp := 2.5E+6

// 2. 幂运算符测试
power1 := 2 ** 3      // 2的3次方 = 8
power2 := 5 ** 2      // 5的2次方 = 25
power3 := 10 ** -2    // 10的-2次方 = 0.01

// 3. 位运算符测试
bit_and := 0xFF & 0x0F     // 位与
bit_or := 0xF0 | 0x0F      // 位或
bit_xor := 0xFF ^ 0xAA     // 位异或

// 4. 混合运算
result1 := 3.14 * 2.0 ** 3
result2 := (5 & 3) | (2 ^ 1)
result3 := 1.5e2 + 2.5e-1

// 5. 复杂表达式
complex := (3.14 ** 2) * (0.5 + 1.5e-1) & 0xFF | (10 ^ 5)
