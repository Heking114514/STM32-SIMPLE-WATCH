import os
from pydub import AudioSegment

# ================= 配置区域 =================
# 1. 源音乐文件夹路径 (把你的乱七八糟的歌都放这里)
SOURCE_FOLDER = 'D:\Desktop\music' 

# 2. 输出文件夹路径 (转码后的歌会放这里)
OUTPUT_FOLDER = 'D:\Desktop\mp3_processed'

# 3. 目标参数 (YX5200 完美兼容参数)
TARGET_RATE = 44100    # 采样率 44.1kHz
TARGET_BITRATE = "128k" # 码率 128kbps
# ===========================================

def batch_convert():
    # 1. 检查源文件夹是否存在
    if not os.path.exists(SOURCE_FOLDER):
        print(f"❌ 错误：找不到源文件夹 '{SOURCE_FOLDER}'，请创建该文件夹并将歌曲放入。")
        os.makedirs(SOURCE_FOLDER)
        return

    # 2. 创建输出文件夹
    if not os.path.exists(OUTPUT_FOLDER):
        os.makedirs(OUTPUT_FOLDER)

    # 3. 获取所有音频文件并排序
    # 支持的格式，可自行添加
    valid_extensions = ('.mp3', '.wav', '.flac', '.m4a', '.aac', '.ogg')
    
    file_list = [f for f in os.listdir(SOURCE_FOLDER) if f.lower().endswith(valid_extensions)]
    
    # 按文件名排序，确保顺序可控
    file_list.sort()

    print(f"🎵 扫描到 {len(file_list)} 首歌曲，准备开始转码...\n")

    success_count = 0
    
    for index, filename in enumerate(file_list):
        # 生成序号文件名：001.mp3, 002.mp3 ...
        # index+1 表示从1开始编号
        new_filename = f"{index + 1:03d}.mp3"
        
        src_path = os.path.join(SOURCE_FOLDER, filename)
        dst_path = os.path.join(OUTPUT_FOLDER, new_filename)

        print(f"[{index + 1}/{len(file_list)}] 正在处理: {filename} -> {new_filename}")

        try:
            # 加载音频
            song = AudioSegment.from_file(src_path)

            # --- 核心处理步骤 ---
            # 1. 重采样到 44100Hz
            song = song.set_frame_rate(TARGET_RATE)
            
            # 2. 设置为双声道 (防止有些单声道文件导致模块异常)
            song = song.set_channels(2)

            # 3. 导出 (指定 128k 码率)
            song.export(dst_path, format="mp3", bitrate=TARGET_BITRATE)
            
            success_count += 1
            
        except Exception as e:
            print(f"❌ 转换失败: {filename}")
            print(f"   原因: {e}")

    print(f"\n✅ 处理完成！成功: {success_count}，失败: {len(file_list) - success_count}")
    print(f"📂 请将 '{OUTPUT_FOLDER}' 文件夹里的内容【逐个】复制到 SD 卡中。")

if __name__ == '__main__':
    batch_convert()