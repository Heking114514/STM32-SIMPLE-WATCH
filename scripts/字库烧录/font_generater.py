import os
from PIL import Image, ImageFont, ImageDraw
from tqdm import tqdm

# ================= 配置区域 =================

# 字体路径
FONT_PATH = os.path.expandvars(r'%WINDIR%\Fonts\simsun.ttc')

# 生成的字号列表
FONT_SIZES = [16]

# 输出文件分割数量
SPLIT_PARTS = 7

# 输出目录
OUTPUT_DIR = "D:\Desktop"

# ===========================================

def get_gb2312_chars_info():
    """生成 GB2312 所有编码对应的字符信息"""
    # 返回列表: [(区位码字符串, 字符), ...]
    chars_info = []
    for high in range(0xA1, 0xF8): 
        for low in range(0xA1, 0xFF):
            val = bytes([high, low])
            code_str = f"0x{high:02X}{low:02X}"
            try:
                char = val.decode('gb2312')
                chars_info.append((code_str, char))
            except:
                chars_info.append((code_str, "  ")) # 空位
    return chars_info

def get_font_bitmap(char, size, font):
    """生成单个字符的字模数据"""
    image = Image.new("1", (size, size), color=0)
    draw = ImageDraw.Draw(image)
    
    offset_y = 0 
    if size == 12: 
        offset_y = -2  # 12号字特调：向上提2像素 (SimSun宋体专用)
    elif size == 16:
        offset_y = 0   # 16号字通常不需要动
    
    draw.text((0, offset_y), char, fill=1, font=font)
    
    buffer = bytearray()
    w, h = size, size
    pages = (h + 7) // 8
    
    for p in range(pages):
        for x in range(w):
            byte_val = 0
            for bit in range(8):
                y = p * 8 + bit
                if y < h:
                    if image.getpixel((x, y)) > 0:
                        byte_val |= (1 << bit)
            buffer.append(byte_val)
    
    return buffer

def main():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    print("正在初始化字符集 (GB2312)...")
    chars_info_list = get_gb2312_chars_info()
    total_chars = len(chars_info_list)
    print(f"字符总数: {total_chars}")

    # 1. 预先计算总字节数，以便均匀分割
    print("正在计算总数据量...")
    total_bytes_needed = 0
    for size in FONT_SIZES:
        bytes_per_char = ((size + 7) // 8) * size
        total_bytes_needed += bytes_per_char * total_chars
    
    bytes_per_part = (total_bytes_needed + SPLIT_PARTS - 1) // SPLIT_PARTS
    print(f"预计总大小: {total_bytes_needed} 字节")
    print(f"每个文件约: {bytes_per_part} 字节")

    # 2. 开始生成
    current_part = 1
    current_part_bytes = 0
    current_offset = 0
    offset_map = {} # 记录每个字号的物理起始地址

    # 打开第一个文件
    f = open(os.path.join(OUTPUT_DIR, f"font_part_{current_part}.h"), "w", encoding="gb2312")
    f.write(f'#include "stdint.h"\n')
    f.write(f"// Part {current_part} (Total 8)\n")
    f.write(f"static const uint8_t Font_Part_{current_part}[] = {{\n")

    for size in FONT_SIZES:
        print(f"\n正在处理 {size}x{size} 字体...")
        try:
            font = ImageFont.truetype(FONT_PATH, size)
        except:
            print("字体加载失败")
            return

        # 记录地址映射
        offset_map[size] = current_offset
        bytes_per_char = ((size + 7) // 8) * size

        # 遍历所有汉字
        for (code_str, char) in tqdm(chars_info_list):
            
            # 获取字模数据
            bitmap = get_font_bitmap(char, size, font)
            
            # 格式化为 Hex 字符串: "0x00, 0x01, ..."
            hex_str = ", ".join([f"0x{b:02X}" for b in bitmap])
            
            # 构造这一行的文本
            # 格式: 数据 , // [字] 编码
            line_str = f"    {hex_str}, // {size}px [{char}] {code_str}\n"
            
            # 检查是否需要切换文件
            # 注意：我们尽量不要在同一个汉字的数据中间切断文件，那样很难看
            # 所以我们在写入一个完整汉字前检查大小
            if current_part_bytes + len(bitmap) > bytes_per_part and current_part < SPLIT_PARTS:
                # 结束当前文件
                f.write("};\n")
                f.close()
                print(f"已生成 Part {current_part}, 切换到下一个文件...")
                
                # 开启新文件
                current_part += 1
                current_part_bytes = 0
                f = open(os.path.join(OUTPUT_DIR, f"font_part_{current_part}.h"), "w", encoding="gb2312")
                f.write(f'#include "stdint.h"\n')
                f.write(f"// Part {current_part} (Total 8)\n")
                f.write(f"static const uint8_t Font_Part_{current_part}[] = {{\n")
            
            # 写入数据
            f.write(line_str)
            
            # 更新计数器
            current_part_bytes += len(bitmap)
            current_offset += len(bitmap)

    # 收尾
    f.write("};\n")
    f.close()
    
    print("\n================ 生成完成 ================")
    print("各字号在 Flash 中的起始绝对地址 (Hex):")
    for size, addr in offset_map.items():
        print(f"  {size}x{size}: 0x{addr:08X}")
    print("==========================================")

if __name__ == "__main__":
    main()