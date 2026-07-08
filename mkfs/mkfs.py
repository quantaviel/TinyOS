#!/usr/bin/env python3
import struct
import os

# ==========================================
# 📊 像素级配置参数
# ==========================================
DISK_NAME = "disk.img"
DISK_SIZE_MB = 64
SECTOR_SIZE = 512
BLOCK_SIZE = 512  # 让一个块等于一个扇区

TOTAL_BLOCKS = (DISK_SIZE_MB * 1024 * 1024) // BLOCK_SIZE

# 🌟 文件系统风水布局 (与 C 语言结构完全对齐)
MAGIC_NUM = 0x4F534653        # "OSFS" 魔数
INODE_BITMAP_BLK = 2          # LBA 0=引导块, LBA 1=超级块
DATA_BITMAP_BLK = 3
INODE_TABLE_BLK = 4
INODE_BLOCKS = 64             # 给 inode 数组分配 64 个块 (可存 1024 个文件)
DATA_START_BLK = INODE_TABLE_BLK + INODE_BLOCKS
DATA_BLOCKS = TOTAL_BLOCKS - DATA_START_BLK

# 每个数据块可容纳的目录项数目
ENTRIES_PER_BLOCK = BLOCK_SIZE // 64  # 512 / 64 = 8

print(f"🛠️  开始构建虚拟磁盘: {DISK_NAME}")
print(f"📦 大小: {DISK_SIZE_MB}MB, 总块数 (LBA范围): 0 ~ {TOTAL_BLOCKS - 1}")

# ==========================================
# 🔨 第一步：凭空创建全黑的磁盘基础文件
# ==========================================
with open(DISK_NAME, "wb") as f:
    f.seek(TOTAL_BLOCKS * BLOCK_SIZE - 1)
    f.write(b'\x00')

# ==========================================
# 🏛️ 第二步：向磁盘雕刻数据结构 (打包二进制)
# ==========================================
with open(DISK_NAME, "r+b") as f:

    # ---- 1. 雕刻 LBA 0: 引导块 (MBR 结尾必须是 55 AA) ----
    f.seek(0)
    f.seek(510)
    f.write(struct.pack("<H", 0xAA55))

    # ---- 2. 组装并写入 LBA 1: 超级块 ----
    super_block_data = struct.pack(
        "<8I",
        MAGIC_NUM,
        TOTAL_BLOCKS,
        INODE_BLOCKS,
        DATA_BLOCKS,
        INODE_BITMAP_BLK,
        DATA_BITMAP_BLK,
        INODE_TABLE_BLK,
        DATA_START_BLK
    )
    f.seek(1 * BLOCK_SIZE)
    f.write(super_block_data)

    # ---- 3. 初始化位图 (LBA 2 和 LBA 3) ----
    # 初始时只有根目录使用 inode 0 和数据块 0
    bitmap_data = b'\x01' + b'\x00' * (BLOCK_SIZE - 1)

    f.seek(INODE_BITMAP_BLK * BLOCK_SIZE)
    f.write(bitmap_data)

    f.seek(DATA_BITMAP_BLK * BLOCK_SIZE)
    f.write(bitmap_data)

# ==========================================
#📁 文件系统结构定义
# ==========================================

# inode 磁盘格式 (d_inode): 128 bytes (对齐到 128 便于管理)
# struct pack format: <2H4I12I + 60 padding = 68 + 60 = 128 bytes
# 512 / 128 = 4 个 inode 可以存放在一个块中

INODE_SIZE = 128  # 每个 inode 的大小 (字节)

def pack_inode(mode, nlinks, size, blocks, mtime, atime, zones):
    """将 inode 属性打包为 128 字节的二进制"""
    # <2H4I12I = 2 shorts + 4 ints + 12 ints = 68 bytes
    inode_data = struct.pack("<2H4I12I",
        mode, nlinks, size, blocks, mtime, atime,
        zones[0], zones[1], zones[2], zones[3],
        zones[4], zones[5], zones[6], zones[7],
        zones[8], zones[9], zones[10], zones[11]
    )
    # 填充 padding 到 128 字节
    return inode_data + b'\x00' * (INODE_SIZE - 68)

def pack_dir_entry(inode_num, name):
    """将目录项打包为 64 字节的二进制"""
    name_bytes = name.encode('utf-8')[:60].ljust(60, b'\x00')
    return struct.pack("<I60s", inode_num, name_bytes)

def allocate_inode():
    """分配一个新的 inode 编号"""
    global next_inode
    inode_num = next_inode
    next_inode += 1
    return inode_num

def allocate_data_block():
    """分配一个新的数据块编号"""
    global next_data_block
    block_num = next_data_block
    next_data_block += 1
    return block_num

def mark_inode_used(inode_num):
    """标记 inode 为已使用"""
    global used_inodes
    used_inodes.add(inode_num)

def mark_data_used(block_num):
    """标记数据块为已使用"""
    global used_blocks
    used_blocks.add(block_num)

def write_inode(inode_num, mode, nlinks, size, blocks, mtime, atime, zones):
    """将 inode 写入磁盘的 inode 表中"""
    inode_data = pack_inode(mode, nlinks, size, blocks, mtime, atime, zones)
    offset = INODE_TABLE_BLK * BLOCK_SIZE + inode_num * INODE_SIZE
    with open(DISK_NAME, "r+b") as f:
        f.seek(offset)
        f.write(inode_data)
    mark_inode_used(inode_num)

def write_dir_block(block_num, entries, inode_num):
    """将目录项列表写入指定的数据块，并更新目录的 i_size"""
    data = b''
    for entry_inode_num, name in entries:
        data += pack_dir_entry(entry_inode_num, name)
    offset = block_num * BLOCK_SIZE
    with open(DISK_NAME, "r+b") as f:
        f.seek(offset)
        f.write(data)

    # 更新目录 inode 的 i_size
    dir_size = len(entries) * 64
    inode_offset = INODE_TABLE_BLK * BLOCK_SIZE + inode_num * INODE_SIZE + 4  # i_size 偏移量
    with open(DISK_NAME, "r+b") as f:
        f.seek(inode_offset)
        f.write(struct.pack("<I", dir_size))

def update_bitmaps():
    """更新 inode 和数据块位图"""
    # 构建 inode 位图
    inode_bitmap = bytearray(BLOCK_SIZE)
    for i in used_inodes:
        inode_bitmap[i // 8] |= (1 << (i % 8))

    # 构建数据块位图
    data_bitmap = bytearray(BLOCK_SIZE)
    for i in used_blocks:
        data_bitmap[i // 8] |= (1 << (i % 8))
    for i in range(0, DATA_START_BLK):
        data_bitmap[i // 8] |= (1 << (i % 8))

    with open(DISK_NAME, "r+b") as f:
        f.seek(INODE_BITMAP_BLK * BLOCK_SIZE)
        f.write(bytes(inode_bitmap))

        f.seek(DATA_BITMAP_BLK * BLOCK_SIZE)
        f.write(bytes(data_bitmap))

# ==========================================
# 🌲 构建目录树
# ==========================================

# 全局分配器状态
next_inode = 0
next_data_block = DATA_START_BLK
used_inodes = set()
used_blocks = set()

# 文件系统布局:
# /
# ├── bin/
# │   └── bash (可执行文件模拟)
# ├── etc/
# │   └── passwd
# ├── home/
# │   └── user/
# │       └── readme.txt
# └── var/
#     └── log/
#         └── messages

# ---- 根目录 / ----
root_inode_num = allocate_inode()  # inode 0
root_block_num = allocate_data_block()  # 数据块 0 (相对于 DATA_START_BLK)
mark_inode_used(root_inode_num)
mark_data_used(root_block_num)

# 根目录 inode: S_IFDIR | 0755 = 0x41ED (目录权限)
root_mode = 0x41ED
root_nlinks = 2  # . 和 ..
root_size = 2 * 64  # 两个目录项
root_blocks = 1
root_mtime = 0
root_atime = 0
root_zones = [root_block_num] + [0] * 11

write_inode(root_inode_num, root_mode, root_nlinks, root_size, root_blocks,
             root_mtime, root_atime, root_zones)

# ---- /bin 目录 ----
bin_inode_num = allocate_inode()
bin_block_num = allocate_data_block()
mark_inode_used(bin_inode_num)
mark_data_used(bin_block_num)

bin_mode = 0x41ED # S_IFDIR | 0755
bin_nlinks = 3  # . .. bash
bin_size = 2 * 64
bin_blocks = 1
bin_zones = [bin_block_num] + [0] * 11

write_inode(bin_inode_num, bin_mode, bin_nlinks, bin_size, bin_blocks,
             root_mtime, root_atime, bin_zones)

# ---- /bin/bash 文件 ----
bash_inode_num = allocate_inode()
bash_block_num = allocate_data_block()
mark_inode_used(bash_inode_num)
mark_data_used(bash_block_num)

bash_content = b"#!/bin/bash\n# This is a simulated bash executable\necho 'Hello from TinyOS!'\n"
bash_mode = 0x81ED  # S_IFREG | 0755 (可执行普通文件)
bash_nlinks = 1
bash_size = len(bash_content)
bash_blocks = 1
bash_zones = [bash_block_num] + [0] * 11

write_inode(bash_inode_num, bash_mode, bash_nlinks, bash_size, bash_blocks,
             root_mtime, root_atime, bash_zones)

# 写入 bash 文件内容
with open(DISK_NAME, "r+b") as f:
    f.seek(bash_block_num * BLOCK_SIZE)
    f.write(bash_content.ljust(BLOCK_SIZE, b'\x00'))

# ---- /etc目录 ----
etc_inode_num = allocate_inode()
etc_block_num = allocate_data_block()
mark_inode_used(etc_inode_num)
mark_data_used(etc_block_num)

etc_mode = 0x41ED
etc_nlinks = 2
etc_size = 1 * 64  # 只有 . 和 ..
etc_blocks = 1
etc_zones = [etc_block_num] + [0] * 11

write_inode(etc_inode_num, etc_mode, etc_nlinks, etc_size, etc_blocks,
             root_mtime, root_atime, etc_zones)

# ---- /etc/passwd 文件 ----
passwd_inode_num = allocate_inode()
passwd_block_num = allocate_data_block()
mark_inode_used(passwd_inode_num)
mark_data_used(passwd_block_num)

passwd_content = b"""root:x:0:0:root:/root:/bin/bash
daemon:x:1:1:daemon:/usr/sbin:/usr/sbin/nologin
bin:x:2:2:bin:/bin:/usr/sbin/nologin
sys:x:3:3:sys:/dev:/usr/sbin/nologin
"""
passwd_mode = 0x81A4  # S_IFREG | 0644 (rw-r--r--)
passwd_nlinks = 1
passwd_size = len(passwd_content)
passwd_blocks = 1
passwd_zones = [passwd_block_num] + [0] * 11

write_inode(passwd_inode_num, passwd_mode, passwd_nlinks, passwd_size, passwd_blocks,
             root_mtime, root_atime, passwd_zones)

# 写入 passwd 文件内容
with open(DISK_NAME, "r+b") as f:
    f.seek(passwd_block_num * BLOCK_SIZE)
    f.write(passwd_content.ljust(BLOCK_SIZE, b'\x00'))

# ---- /home 目录 ----
home_inode_num = allocate_inode()
home_block_num = allocate_data_block()
mark_inode_used(home_inode_num)
mark_data_used(home_block_num)

home_mode = 0x41ED
home_nlinks = 3 # . .. user
home_size = 2 * 64
home_blocks = 1
home_zones = [home_block_num] + [0] * 11

write_inode(home_inode_num, home_mode, home_nlinks, home_size, home_blocks,
             root_mtime, root_atime, home_zones)

# ---- /home/user 目录 ----
user_inode_num = allocate_inode()
user_block_num = allocate_data_block()
mark_inode_used(user_inode_num)
mark_data_used(user_block_num)

user_mode = 0x41ED
user_nlinks = 2
user_size = 2 * 64
user_blocks = 1
user_zones = [user_block_num] + [0] * 11

write_inode(user_inode_num, user_mode, user_nlinks, user_size, user_blocks,
             root_mtime, root_atime, user_zones)

# ---- /home/user/readme.txt 文件 ----
readme_inode_num = allocate_inode()
readme_block_num = allocate_data_block()
mark_inode_used(readme_inode_num)
mark_data_used(readme_block_num)

readme_content = b"""Welcome to TinyOS!
====================

This is a test file created by mkfs.py.

Directory structure:
/bin/bash     - Simulated bash executable
/etc/passwd  - User account information
/home/user/ - User home directory
/var/log/ - System logs

Enjoy exploring TinyOS!
"""
readme_mode = 0x81A4  # S_IFREG | 0644
readme_nlinks = 1
readme_size = len(readme_content)
readme_blocks = 1
readme_zones = [readme_block_num] + [0] * 11

write_inode(readme_inode_num, readme_mode, readme_nlinks, readme_size, readme_size // BLOCK_SIZE + 1,
             root_mtime, root_atime, readme_zones)

# 写入 readme 文件内容
with open(DISK_NAME, "r+b") as f:
    f.seek(readme_block_num * BLOCK_SIZE)
    f.write(readme_content.ljust(BLOCK_SIZE, b'\x00'))

# ---- /var 目录 ----
var_inode_num = allocate_inode()
var_block_num = allocate_data_block()
mark_inode_used(var_inode_num)
mark_data_used(var_block_num)

var_mode = 0x41ED
var_nlinks = 3 # . .. log
var_size = 2 * 64
var_blocks = 1
var_zones = [var_block_num] + [0] * 11

write_inode(var_inode_num, var_mode, var_nlinks, var_size, var_blocks,
             root_mtime, root_atime, var_zones)

# ---- /var/log 目录 ----
log_inode_num = allocate_inode()
log_block_num = allocate_data_block()
mark_inode_used(log_inode_num)
mark_data_used(log_block_num)

log_mode = 0x41ED
log_nlinks = 2
log_size = 2 * 64
log_blocks = 1
log_zones = [log_block_num] + [0] * 11

write_inode(log_inode_num, log_mode, log_nlinks, log_size, log_blocks,
             root_mtime, root_atime, log_zones)

# ---- /var/log/messages 文件 ----
messages_inode_num = allocate_inode()
messages_block_num = allocate_data_block()
mark_inode_used(messages_inode_num)
mark_data_used(messages_block_num)

messages_content = b"""Jun 10 10:00:01 tinyos kernel: Boot OK
Jun 10 10:00:02 tinyos systemd[1]: Started TinyOS Init
Jun 10 10:00:03 tinyos kernel: CPU: x86_64
Jun 10 10:00:04 tinyos kernel: Memory: 64MB
Jun 10 10:00:05 tinyos bash[42]: Hello from TinyOS bash!
"""
messages_mode = 0x81A4  # S_IFREG | 0644
messages_nlinks = 1
messages_size = len(messages_content)
messages_blocks = 1
messages_zones = [messages_block_num] + [0] * 11

write_inode(messages_inode_num, messages_mode, messages_nlinks, messages_size, messages_blocks,
             root_mtime, root_atime, messages_zones)

# 写入 messages 文件内容
with open(DISK_NAME, "r+b") as f:
    f.seek(messages_block_num * BLOCK_SIZE)
    f.write(messages_content.ljust(BLOCK_SIZE, b'\x00'))

# ==========================================
# 📝 写入目录内容
# ==========================================

# 根目录 /
write_dir_block(root_block_num, [
    (root_inode_num, "."),
    (root_inode_num, ".."),
    (bin_inode_num, "bin"),
    (etc_inode_num, "etc"),
    (home_inode_num, "home"),
    (var_inode_num, "var"),
], root_inode_num)

# /bin
write_dir_block(bin_block_num, [
    (bin_inode_num, "."),
    (root_inode_num, ".."),
    (bash_inode_num, "bash"),
], bin_inode_num)

# /etc
write_dir_block(etc_block_num, [
    (etc_inode_num, "."),
    (root_inode_num, ".."),
    (passwd_inode_num, "passwd"),
], etc_inode_num)

# /home
write_dir_block(home_block_num, [
    (home_inode_num, "."),
    (root_inode_num, ".."),
    (user_inode_num, "user"),
], home_inode_num)

# /home/user
write_dir_block(user_block_num, [
    (user_inode_num, "."),
    (home_inode_num, ".."),
    (readme_inode_num, "readme.txt"),
], user_inode_num)

# /var
write_dir_block(var_block_num, [
    (var_inode_num, "."),
    (root_inode_num, ".."),
    (log_inode_num, "log"),
], var_inode_num)

# /var/log
write_dir_block(log_block_num, [
    (log_inode_num, "."),
    (var_inode_num, ".."),
    (messages_inode_num, "messages"),
], log_inode_num)

# ==========================================
# 🔄 更新位图
# ==========================================
update_bitmaps()

print("🎉 虚拟磁盘镜像制作并格式化成功！")
print(f"📁 文件系统结构:")
print(f"   /")
print(f"   ├── /bin/")
print(f"   │   └── /bin/bash (simulated executable)")
print(f"   ├── /etc/")
print(f"   │   └── /etc/passwd (user accounts)")
print(f"   ├── /home/")
print(f"   │   └── /home/user/")
print(f"   │       └── /home/user/readme.txt")
print(f"   └── /var/")
print(f"       └── /var/log/")
print(f"           └── /var/log/messages (system log)")
print(f"")
print(f"📊 统计:")
print(f"   inode 使用: {len(used_inodes)} /1024")
print(f"   数据块使用: {len(used_blocks)} / {DATA_BLOCKS}")