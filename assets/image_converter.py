#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
================================================================================
脚本名称：image_converter.py
脚本功能：
  1. 支持将 Image2Display 取模软件导出的图像数据转换为 0.96 寸 SSD1306 OLED 
     专用的标准页寻址列行式（Page-Mode LSB）格式。
  2. 支持直接输入 PNG 图片（无需额外第三方依赖，纯内置库解析）进行转换。
  3. 支持命令行指定宽高、旋转、镜像（水平/垂直翻转）、反色与格式模式。
  4. 转换完成后自动在终端打印 ASCII 效果预览，确保方向正确。
  5. 自动同步更新写入 BSP/OLED_Data.h 与 BSP/OLED_Data.c。
================================================================================
"""

import sys
import os
import re
import zlib
import struct
import argparse

def parse_png(png_path):
    """
    内置轻量级 PNG 单色/RGBA 解析器（无需安装 PIL/Pillow）。
    :return: (grid, width, height) 二维像素矩阵 (1 为亮，0 为暗)
    """
    with open(png_path, "rb") as f:
        data = f.read()

    pos = 8
    idat = bytearray()
    w, h = 0, 0
    color_type = 0
    bit_depth = 8

    while pos < len(data):
        length, chunk_type = struct.unpack(">I4s", data[pos:pos+8])
        pos += 8
        chunk_data = data[pos:pos+length]
        pos += length + 4
        if chunk_type == b"IHDR":
            w, h, bit_depth, color_type = struct.unpack(">IIBB", chunk_data[:10])
        elif chunk_type == b"IDAT":
            idat.extend(chunk_data)
        elif chunk_type == b"IEND":
            break

    decomp = zlib.decompress(idat)
    grid = [[0]*w for _ in range(h)]

    # 根据颜色类型计算每行字节跨度
    channels = 4 if color_type == 6 else (3 if color_type == 2 else 1)
    bytes_per_pixel = (bit_depth * channels + 7) // 8
    stride = w * bytes_per_pixel
    d_pos = 0

    for y in range(h):
        filter_type = decomp[d_pos]
        d_pos += 1
        line = decomp[d_pos:d_pos+stride]
        d_pos += stride
        for x in range(w):
            offset = x * bytes_per_pixel
            if color_type == 6:  # RGBA
                r, g, b, a = line[offset:offset+4]
                # 有效非黑像素判定（支持半透明白色或带 alpha 图层）
                is_lit = 1 if (r > 100 or g > 100 or b > 100 or a == 0) else 0
            elif color_type == 2:  # RGB
                r, g, b = line[offset:offset+3]
                is_lit = 1 if (r > 100 or g > 100 or b > 100) else 0
            else:  # Grayscale
                v = line[offset]
                is_lit = 1 if v > 100 else 0
            grid[y][x] = is_lit

    return grid, w, h

def parse_c_array(c_file_path):
    """
    从 C 语言源文件/文本文件中提取十六进制或十进制字节数组。
    """
    with open(c_file_path, "r", encoding="utf-8", errors="ignore") as f:
        text = f.read()

    # 优先匹配 0x 格式十六进制
    hex_matches = re.findall(r"0x[0-9a-fA-F]{1,2}", text)
    if hex_matches:
        return [int(x, 16) for x in hex_matches]

    # 兜底匹配逗号分隔的十进制数字
    num_matches = re.findall(r"\b\d+\b", text)
    return [int(x) for x in num_matches]

def convert_to_grid(raw_bytes, width, height, mode="col_msb"):
    """
    将取模字节流按照指定的遍历顺序解码为标准的 2D 像素矩阵 grid[y][x]。
    模式支持：
      - col_msb: 纵向列优先 (↓→)，最高有效位在前 (Image2Display 常见模式)
      - col_lsb: 纵向列优先 (↓→)，最低有效位在前
      - row_msb: 横向行优先 (→↓)，最高有效位在前
      - row_lsb: 横向行优先 (→↓)，最低有效位在前
    """
    pages = (height + 7) // 8
    row_bytes = (width + 7) // 8
    grid = [[0]*width for _ in range(height)]

    for y in range(height):
        for x in range(width):
            p = y // 8
            bit = y % 8
            if mode == "col_msb":
                idx = x * pages + p
                b = raw_bytes[idx] if idx < len(raw_bytes) else 0
                grid[y][x] = (b >> (7 - bit)) & 1
            elif mode == "col_lsb":
                idx = x * pages + p
                b = raw_bytes[idx] if idx < len(raw_bytes) else 0
                grid[y][x] = (b >> bit) & 1
            elif mode == "row_msb":
                idx = y * row_bytes + (x // 8)
                b = raw_bytes[idx] if idx < len(raw_bytes) else 0
                grid[y][x] = (b >> (7 - (x % 8))) & 1
            elif mode == "row_lsb":
                idx = y * row_bytes + (x // 8)
                b = raw_bytes[idx] if idx < len(raw_bytes) else 0
                grid[y][x] = (b >> (x % 8)) & 1

    return grid

def encode_grid_to_ssd1306(grid, width, height):
    """
    将 2D 像素矩阵 grid[y][x] 编码为 SSD1306 页寻址标准列行式（Page-Mode LSB）字节数组。
    """
    pages = (height + 7) // 8
    ssd_bytes = []

    for p in range(pages):
        for x in range(width):
            val = 0
            for bit in range(8):
                y = p * 8 + bit
                if y < height and grid[y][x]:
                    val |= (1 << bit)
            ssd_bytes.append(val)

    return ssd_bytes

def print_ascii_preview(grid, width, height):
    """
    在终端打印 ASCII 效果图，方便开发者直观核验图像方向与细节。
    """
    print("┌" + "─" * width + "┐")
    for r in range(0, height, 2):
        row_str = ""
        for c in range(width):
            top = grid[r][c] if r < height else 0
            bottom = grid[r+1][c] if r+1 < height else 0
            if top and bottom:
                row_str += "█"
            elif top and not bottom:
                row_str += "▀"
            elif not top and bottom:
                row_str += "▄"
            else:
                row_str += " "
        print("│" + row_str + "│")
    print("└" + "─" * width + "┘")

def format_c_array(array_name, width, height, data):
    """
    将字节列表格式化为整洁的 C 语言数组定义与声明字符串。
    """
    total_len = len(data)
    lines = []
    lines.append(f"/* 图像名称: {array_name}，尺寸: {width}x{height}，字节数: {total_len} (SSD1306 页寻址列行式) */")
    lines.append(f"const uint8_t {array_name}[{total_len}] = {{")
    for i in range(0, total_len, 16):
        chunk = data[i:i+16]
        hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
        if i + 16 < total_len:
            lines.append(f"    {hex_str},")
        else:
            lines.append(f"    {hex_str}")
    lines.append("};")
    c_def = "\n".join(lines)
    c_decl = f"extern const uint8_t {array_name}[{total_len}];"
    return c_def, c_decl

def update_oled_data_files(bsp_dir, array_name, c_def, c_decl):
    """
    自动将声明追加/更新到 BSP/OLED_Data.h，将定义追加/更新到 BSP/OLED_Data.c。
    """
    h_path = os.path.join(bsp_dir, "OLED_Data.h")
    c_path = os.path.join(bsp_dir, "OLED_Data.c")

    if not os.path.exists(h_path) or not os.path.exists(c_path):
        print(f"[警告] 未找到 {h_path} 或 {c_path}，跳过写入。")
        return False

    # 1. 更新 OLED_Data.h
    with open(h_path, "rb") as f:
        h_content = f.read().decode("gbk", errors="ignore")

    decl_pattern = rf"extern\s+const\s+uint8_t\s+{re.escape(array_name)}\[\d+\];"
    if re.search(decl_pattern, h_content):
        h_content = re.sub(decl_pattern, c_decl, h_content)
        print(f"[更新] 已在 {h_path} 中更新声明：{c_decl}")
    else:
        endif_pos = h_content.rfind("#endif")
        if endif_pos != -1:
            h_content = h_content[:endif_pos] + c_decl + "\r\n" + h_content[endif_pos:]
            print(f"[追加] 已向 {h_path} 追加声明：{c_decl}")
        else:
            h_content += "\r\n" + c_decl + "\r\n"

    with open(h_path, "wb") as f:
        f.write(h_content.encode("gbk"))

    # 2. 更新 OLED_Data.c
    with open(c_path, "rb") as f:
        c_content = f.read().decode("gbk", errors="ignore")

    def_pattern = rf"/\* 图像名称: {re.escape(array_name)}.*?\*/\s*const\s+uint8_t\s+{re.escape(array_name)}\[\d+\]\s*=\s*\{{.*?\}};"
    if re.search(def_pattern, c_content, flags=re.DOTALL):
        c_content = re.sub(def_pattern, c_def.replace("\n", "\r\n"), c_content, flags=re.DOTALL)
        print(f"[更新] 已在 {c_path} 中更新数组定义：{array_name}")
    else:
        c_content = c_content.rstrip() + "\r\n\r\n" + c_def.replace("\n", "\r\n") + "\r\n"
        print(f"[追加] 已向 {c_path} 追加数组定义：{array_name}")

    with open(c_path, "wb") as f:
        f.write(c_content.encode("gbk"))

    return True

def main():
    parser = argparse.ArgumentParser(description="Image2Display / PNG 图像取模转 SSD1306 标准格式工具")
    parser.add_argument("-i", "--input", default="assets/image_data.c", help="输入文件路径 (.c / .txt / .png)")
    parser.add_argument("-n", "--name", default="gImage_SrP_64x64", help="生成的 C 语言数组名 (默认: gImage_SrP_64x64)")
    parser.add_argument("-w", "--width", type=int, default=64, help="图像宽度 (默认: 64)")
    parser.add_argument("-H", "--height", type=int, default=64, help="图像高度 (默认: 64)")
    parser.add_argument("-m", "--mode", choices=["col_msb", "col_lsb", "row_msb", "row_lsb"], default="col_msb",
                        help="取模数据解析模式 (默认: col_msb，对应 Image2Display 纵向↓→扫描)")
    parser.add_argument("--flip-h", action="store_true", help="水平镜像翻转 (左右颠倒)")
    parser.add_argument("--flip-v", action="store_true", help="垂直镜像翻转 (上下颠倒)")
    parser.add_argument("--invert", action="store_true", help="颜色反转 (黑白互换)")
    parser.add_argument("--bsp", default="BSP", help="BSP 目录路径 (默认: BSP)")
    parser.add_argument("--no-write", action="store_true", help="仅预览和打印结果，不写入文件")

    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"[错误] 输入文件不存在: {args.input}")
        sys.exit(1)

    # 1. 解析输入源
    if args.input.lower().endswith(".png"):
        print(f"[*] 正在解析 PNG 图像: {args.input}")
        grid, w, h = parse_png(args.input)
        width, height = w, h
    else:
        print(f"[*] 正在解析取模数组文件: {args.input} (模式: {args.mode})")
        raw_bytes = parse_c_array(args.input)
        width, height = args.width, args.height
        expected = height * ((width + 7) // 8)
        if len(raw_bytes) < expected:
            raw_bytes.extend([0] * (expected - len(raw_bytes)))
        elif len(raw_bytes) > expected:
            raw_bytes = raw_bytes[:expected]
        grid = convert_to_grid(raw_bytes, width, height, args.mode)

    # 2. 应用几何变换（若有）
    if args.flip_h:
        grid = [[grid[y][width - 1 - x] for x in range(width)] for y in range(height)]
    if args.flip_v:
        grid = [[grid[height - 1 - y][x] for x in range(width)] for y in range(height)]
    if args.invert:
        grid = [[1 - grid[y][x] for x in range(width)] for y in range(height)]

    # 3. 终端打印 ASCII 效果核验
    print("\n--- [OLED 显示效果 ASCII 模拟预览] ---")
    print_ascii_preview(grid, width, height)

    # 4. 编码为 SSD1306 Page-Mode LSB 数组
    ssd_bytes = encode_grid_to_ssd1306(grid, width, height)
    print(f"\n[+] 转换完成，生成 SSD1306 格式数据共 {len(ssd_bytes)} 字节")

    # 5. 生成 C 代码并写入
    c_def, c_decl = format_c_array(args.name, width, height, ssd_bytes)

    if not args.no_write:
        update_oled_data_files(args.bsp, args.name, c_def, c_decl)
        print(f"[✔] 成功更新 {args.name} 至 BSP/OLED_Data.h 与 BSP/OLED_Data.c！")
        print(f"    在 main 中直接调用：")
        print(f"    OLED_ShowImage(32, 0, {width}, {height}, {args.name});")
        print(f"    OLED_Update();\n")
    else:
        print("\n=== 声明 (OLED_Data.h) ===")
        print(c_decl)
        print("\n=== 定义 (OLED_Data.c) ===")
        print(c_def)

if __name__ == "__main__":
    main()
