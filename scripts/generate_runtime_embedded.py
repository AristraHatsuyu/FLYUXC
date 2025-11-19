#!/usr/bin/env python3
"""
生成嵌入的 runtime 源码字符串
"""
import sys

if len(sys.argv) != 3:
    print("Usage: generate_runtime_embedded.py <input.c> <output.h>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

# 读取源码
with open(input_file, 'r') as f:
    code = f.read()

# 转义为 C 字符串
escaped = code.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n"\n    "')

# 写入头文件
with open(output_file, 'w') as f:
    f.write('    "' + escaped + '"')

print(f"Generated {output_file}")
