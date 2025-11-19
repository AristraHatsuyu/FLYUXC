#!/bin/bash
# 生成嵌入的 runtime 对象文件二进制数组

if [ "$#" -ne 2 ]; then
    echo "Usage: generate_object_embedded.sh <input.o> <output.h>"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"

# 生成 xxd 输出，然后替换所有变量名为固定名称
xxd -i "$INPUT_FILE" | \
    sed '1s/.*/static const unsigned char runtime_object_o[] = {/' | \
    sed '$s/.*/static const unsigned int runtime_object_o_len = &;/' | \
    sed '$s/unsigned int/unsigned int/' > "$OUTPUT_FILE"

# 修正最后一行（获取实际长度）
ACTUAL_LEN=$(wc -c < "$INPUT_FILE" | tr -d ' ')
sed -i '' "s/runtime_object_o_len = .*;/runtime_object_o_len = $ACTUAL_LEN;/" "$OUTPUT_FILE"

echo "Generated $OUTPUT_FILE"
