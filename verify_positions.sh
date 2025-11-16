#!/bin/bash

# 全面验证所有位置映射的正确性

echo "================================"
echo "位置映射全面验证"
echo "================================"
echo ""

# 测试 1: simple_obj.fx - 基本对象字面量
echo "【测试 1: simple_obj.fx】"
echo "原文: w:={a:1};"
echo "期望: w在1:1, :=在1:3, {在1:6, a在1:7, :在1:8, 1在1:10, }在1:11"
./build/flyuxc testfx/simple_obj.fx | grep "IDENT\|DEFINE\|L_BRACE\|COLON\|NUM\|R_BRACE" | head -7
echo ""

# 测试 2: types_test.fx - 类型注解
echo "【测试 2: types_test.fx 第5行】"
echo "原文: w :[obj]= {a: 1};"
echo "期望: w在5:1, {在5:11, a在5:12(对象属性不映射), :在5:13, 1在5:15, }在5:16"
./build/flyuxc testfx/types_test.fx | grep "^IDENT.*\"_00004\"" -A 6
echo ""

# 测试 3: print.fx - 字符串内空格和UTF-8注释位置
echo "【测试 3: print.fx 第4行】"
echo "原文: print(\"Hello /* 行内注释 */World!\")"
echo "期望: print在4:1, (在4:6, 字符串在4:7长度24(含注释), )在4:31"
./build/flyuxc testfx/print.fx | grep "BUILTIN_FUNC.*\"print\"" | head -1
./build/flyuxc testfx/print.fx | grep "L_PAREN" | head -1
./build/flyuxc testfx/print.fx | grep "STRING.*Hello" | head -1
./build/flyuxc testfx/print.fx | grep "R_PAREN" | head -1
echo ""

echo "【测试 4: print.fx 第6行 - UTF-8字符列号】"
echo "原文: print (\"Hello World!\"/* 行内需移除注释 */) // ..."
echo "期望: print在6:1, (在6:7, 字符串在6:8长度14, )在6:35(不是6:50!)"
./build/flyuxc testfx/print.fx | grep "BUILTIN_FUNC.*\"print\"" | tail -1
./build/flyuxc testfx/print.fx | grep "L_PAREN" | tail -1
./build/flyuxc testfx/print.fx | grep "STRING.*Hello" | tail -1
./build/flyuxc testfx/print.fx | grep "R_PAREN" | tail -1
echo ""

# 测试 5: demo.fx - Emoji位置(4字节UTF-8)
echo "【测试 5: demo.fx 第2行 - Emoji】"
echo "原文: 🤪🫵:<num>=(🐙,🍄){...}"
echo "期望: 🤪🫵在2:1长度4, :<在2:3长度2, num在2:5, >=在2:8"
./build/flyuxc testfx/demo.fx | grep "IDENT.*\"_00001\"" | head -1
./build/flyuxc testfx/demo.fx | grep "FUNC_TYPE_START" | head -1
./build/flyuxc testfx/demo.fx | grep "TYPE_NUM" | head -1
./build/flyuxc testfx/demo.fx | grep "FUNC_TYPE_END" | head -1
echo ""

echo "【测试 6: demo.fx 第8行 - 混合emoji和符号】"
echo "原文: 😼 := {😀し:\"🐢\", 🛸:[7, \"🛫\"]};🐋 := ..."
echo "期望: 😼在8:5, {在8:10, 😀し在8:11(日文混合), :在8:13, 🐢在8:14"
./build/flyuxc testfx/demo.fx | grep "IDENT.*\"_00005\"" | head -1
./build/flyuxc testfx/demo.fx | grep "8:10" | head -1
./build/flyuxc testfx/demo.fx | grep "😀し"
echo ""

# 测试 7: complex_test.fx - 对象属性vs变量
echo "【测试 7: complex_test.fx 第4行】"
echo "原文: arr :[obj]= [{a:1},{b:2}];"
echo "期望: a在4:15(对象key不映射), b在4:21(对象key不映射)"
./build/flyuxc testfx/complex_test.fx | grep "4:15"
./build/flyuxc testfx/complex_test.fx | grep "4:21"
echo ""

echo "【测试 8: complex_test.fx 第16行】"
echo "原文: y := arr[0].a"
echo "期望: a在16:17(属性访问不映射)"
./build/flyuxc testfx/complex_test.fx | grep "16:17"
echo ""

# 测试 9: 多字符操作符
echo "【测试 9: 多字符操作符长度】"
echo "期望: := 长度+2, :< 长度+2, >= 长度+2, .> 长度+2, R> 长度+2, L> 长度+2"
./build/flyuxc testfx/types_test.fx | grep "FUNC_TYPE_START\|FUNC_TYPE_END\|DEFINE" | head -3
echo ""

# 测试 10: Synthetic tokens
echo "【测试 10: 自动插入的分号】"
echo "期望: 所有自动插入的分号显示 (synthetic)"
./build/flyuxc testfx/simple_obj.fx | grep "SEMI"
echo ""

echo "================================"
echo "验证完成！请检查上述输出是否符合期望"
echo "================================"
