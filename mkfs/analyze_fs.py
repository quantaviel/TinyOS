#!/usr/bin/env python3
"""
TinyOS Filesystem Analysis Tool
Analyze disk.img structure
"""
import struct
import os
import sys

# ==========================================
# Constants (consistent with C code and mkfs.py)
# ==========================================
FS_MAGIC_NUMBER = 0x4F534653  # "OSFS"
FS_BLOCK_SIZE = 512
SECTOR_SIZE = 512

# File type masks
S_IFMT = 0xF000
S_IFREG = 0x8000  # Regular file
S_IFDIR = 0x4000  # Directory
S_IFCHR = 0x2000  # Character device
S_IFBLK = 0x6000  # Block device

# Permission masks
S_IRWXU = 0x700
S_IRUSR = 0x400
S_IWUSR = 0x200
S_IXUSR = 0x100

# ==========================================
# Data Structures
# ==========================================

def read_super_block(f):
    """Read super block from LBA 1"""
    f.seek(FS_BLOCK_SIZE)
    data = f.read(32)  # 8 * 4 = 32 bytes
    sb = struct.unpack("<8I", data)
    return {
        'magic_number': sb[0],
        'total_blocks': sb[1],
        'inode_blocks': sb[2],
        'data_blocks': sb[3],
        'inode_bitmap_block': sb[4],
        'data_bitmap_block': sb[5],
        'inode_table_block': sb[6],
        'data_start_block': sb[7]
    }

def read_d_inode(f, inode_num, sb):
    """Read d_inode from disk"""
    # Inodes are packed contiguously: offset = INODE_TABLE_BLK * BLOCK_SIZE + inode_num * 68
    offset = sb['inode_table_block'] * FS_BLOCK_SIZE + inode_num * 68
    f.seek(offset)
    data = f.read(68)  # d_inode: 2+2+4+4+4+4+48 = 68 bytes
    if len(data) < 68:
        return None
    inode = struct.unpack("<2H4I12I", data)
    return {
        'i_mode': inode[0],
        'i_nlinks': inode[1],
        'i_size': inode[2],
        'i_blocks': inode[3],
        'i_mtime': inode[4],
        'i_atime': inode[5],
        'i_zone': list(inode[6:18])
    }

def read_file_content(f, inode):
    """Read file content from inode's data blocks"""
    content = b''
    for block_lba in inode['i_zone']:
        if block_lba == 0:
            break
        f.seek(block_lba * FS_BLOCK_SIZE)
        content += f.read(FS_BLOCK_SIZE)
    # Trim to actual file size
    return content[:inode['i_size']]

def is_regular_file(inode):
    """Check if inode is a regular file"""
    return (inode['i_mode'] & S_IFMT) == S_IFREG

def read_dir_entry(f, block_num, sb):
    """Read directory entries from a data block"""
    # block_num is the absolute LBA (as stored directly in i_zone)
    f.seek(block_num * FS_BLOCK_SIZE)
    del sb  # suppress unused warning
    data = f.read(FS_BLOCK_SIZE)
    entries = []
    for i in range(0, FS_BLOCK_SIZE, 64):
        entry_data = data[i:i+64]
        if len(entry_data) < 64:
            break
        inode_num, name_bytes = struct.unpack("<I60s", entry_data)
        # Only break on empty entries (both inode=0 and empty name)
        if inode_num == 0 and name_bytes.rstrip(b'\x00') == b'':
            break
        name = name_bytes.rstrip(b'\x00').decode('utf-8', errors='ignore')
        entries.append({'inode': inode_num, 'name': name})
    return entries

def read_inode_bitmap(f, sb):
    """Read inode bitmap"""
    f.seek(sb['inode_bitmap_block'] * FS_BLOCK_SIZE)
    return f.read(FS_BLOCK_SIZE)

def read_data_bitmap(f, sb):
    """Read data block bitmap"""
    f.seek(sb['data_bitmap_block'] * FS_BLOCK_SIZE)
    return f.read(FS_BLOCK_SIZE)

def get_file_type(mode):
    """Get file type string"""
    file_type = mode & S_IFMT
    if file_type == S_IFREG:
        return "Regular File"
    elif file_type == S_IFDIR:
        return "Directory"
    elif file_type == S_IFCHR:
        return "Character Device"
    elif file_type == S_IFBLK:
        return "Block Device"
    else:
        return "Unknown"

def get_permissions(mode):
    """Get permission string (rwx format)"""
    perms = []
    # Owner
    perms.append('r' if (mode & S_IRUSR) else '-')
    perms.append('w' if (mode & S_IWUSR) else '-')
    perms.append('x' if (mode & S_IXUSR) else '-')
    # Group
    perms.append('r' if (mode & 0x40) else '-')
    perms.append('w' if (mode & 0x20) else '-')
    perms.append('x' if (mode & 0x10) else '-')
    # Other
    perms.append('r' if (mode & 0x4) else '-')
    perms.append('w' if (mode & 0x2) else '-')
    perms.append('x' if (mode & 0x1) else '-')
    return ''.join(perms)

def mode_to_str(mode):
    """Convert mode to type + permissions string"""
    file_type = get_file_type(mode)
    perms = get_permissions(mode)
    return f"{file_type} ({perms})"

def analyze_inode_usage(bitmap):
    """Analyze inode bitmap usage"""
    used = set()
    total = 0
    for byte_idx, byte in enumerate(bitmap):
        for bit in range(8):
            if byte & (1 << bit):
                inode_num = byte_idx * 8 + bit
                used.add(inode_num)
            total += 1
    return used, total

def analyze_block_usage(bitmap):
    """Analyze data block bitmap usage"""
    used = set()
    total = 0
    for byte_idx, byte in enumerate(bitmap):
        for bit in range(8):
            if byte & (1 << bit):
                block_num = byte_idx * 8 + bit
                used.add(block_num)
            total += 1
    return used, total

def print_super_block(sb):
    """Print super block information"""
    print("=" * 60)
    print("Super Block Information")
    print("=" * 60)
    magic_ok = sb['magic_number'] == FS_MAGIC_NUMBER
    print(f"  Magic Number:      0x{sb['magic_number']:08X} {'OK' if magic_ok else 'ERROR'}")
    print(f"  Total Blocks:      {sb['total_blocks']}")
    print(f"  Inode Blocks:      {sb['inode_blocks']}")
    print(f"  Data Blocks:       {sb['data_blocks']}")
    print(f"  Inode Bitmap:      Block #{sb['inode_bitmap_block']}")
    print(f"  Data Bitmap:       Block #{sb['data_bitmap_block']}")
    print(f"  Inode Table:       Block #{sb['inode_table_block']}")
    print(f"  Data Start: Block #{sb['data_start_block']}")
    print()

def print_bitmap_info(sb, inode_used, inode_total, data_used, data_total):
    """Print bitmap usage information"""
    print("=" * 60)
    print("Bitmap Usage")
    print("=" * 60)
    print(f"  Inode Bitmap:  {len(inode_used)} / {inode_total} used")
    print(f"  Data Bitmap:   {len(data_used)} / {data_total} used")
    print()

def print_inode_table(f, sb, used_inodes):
    """Print inode table"""
    print("=" * 60)
    print("Inode Table")
    print("=" * 60)
    for inode_num in sorted(used_inodes):
        inode = read_d_inode(f, inode_num, sb)
        if inode:
            print(f"  Inode #{inode_num}:")
            print(f"    Mode:       0x{inode['i_mode']:04X} ({mode_to_str(inode['i_mode'])})")
            print(f"    Nlinks:     {inode['i_nlinks']}")
            print(f"    Size:       {inode['i_size']} bytes")
            print(f"    Blocks:     {inode['i_blocks']}")
            print(f"    Mtime:      {inode['i_mtime']}")
            print(f"    Atime:      {inode['i_atime']}")
            print(f"    Zones:      {inode['i_zone']}")
    print()

def print_directory_tree(f, sb, used_inodes, show_content=False):
    """Print directory tree"""
    print("=" * 60)
    print("Directory Tree")
    print("=" * 60)

    def print_dir(entry_inode_num, name, indent=0):
        prefix = "  " * indent
        entry_inode = read_d_inode(f, entry_inode_num, sb)
        if not entry_inode:
            return
        block_num = entry_inode['i_zone'][0]
        print(f"{prefix}+-- {name}/ (inode={entry_inode_num}, block={block_num})")

        entries = read_dir_entry(f, block_num, sb)
        for entry in entries:
            if entry['name'] in ('.', '..'):
                continue
            entry_inode = read_d_inode(f, entry['inode'], sb)
            if entry_inode:
                is_dir = (entry_inode['i_mode'] & S_IFMT) == S_IFDIR
                if is_dir:
                    print_dir(entry['inode'], entry['name'], indent + 1)
                else:
                    print(f"{prefix}|   +-- {entry['name']} (inode={entry['inode']}, size={entry_inode['i_size']})")
                    if show_content:
                        content = read_file_content(f, entry_inode)
                        ascii_content = content.decode('utf-8', errors='replace')
                        for line in ascii_content.split('\n'):
                            print(f"{prefix}|   |   {line}")

    # Start from root inode 0
    root_inode = read_d_inode(f, 0, sb)
    if root_inode:
        root_block = root_inode['i_zone'][0]
        print("/ (root)")
        entries = read_dir_entry(f, root_block, sb)
        for entry in entries:
            if entry['name'] in ('.', '..'):
                continue
            entry_inode = read_d_inode(f, entry['inode'], sb)
            if entry_inode:
                is_dir = (entry_inode['i_mode'] & S_IFMT) == S_IFDIR
                if is_dir:
                    print_dir(entry['inode'], entry['name'], 1)
                else:
                    print(f"  +-- {entry['name']} (inode={entry['inode']}, size={entry_inode['i_size']})")
                    if show_content:
                        content = read_file_content(f, entry_inode)
                        ascii_content = content.decode('utf-8', errors='replace')
                        for line in ascii_content.split('\n'):
                            print(f"  |   {line}")
    print()

def print_raw_block(f, block_num, sb, title=None):
    """Print raw block content in hex"""
    # block_num is already the absolute LBA
    f.seek(block_num * FS_BLOCK_SIZE)
    data = f.read(FS_BLOCK_SIZE)
    del sb  # suppress unused warning

    if title:
        print("=" * 60)
        print(f"{title} (Block #{block_num})")
        print("=" * 60)
    else:
        print("=" * 60)
        print(f"Block #{block_num}")
        print("=" * 60)

    # Hex dump
    for row in range(0, FS_BLOCK_SIZE, 16):
        hex_part = ' '.join(f'{b:02X}' for b in data[row:row+16])
        ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in data[row:row+16])
        print(f"  {row:04X}: {hex_part:<48} {ascii_part}")
    print()

def main():
    # Default path
    disk_path = "build/disk.img"
    show_content = False
    verbose = False

    # Support command line arguments
    for arg in sys.argv[1:]:
        if arg in ('-v', '--verbose'):
            verbose = True
        elif arg in ('-c', '--content'):
            show_content = True
        elif arg in ('-h', '--help'):
            print(f"Usage: python3 {sys.argv[0]} [disk.img path] [-v] [-c]")
            print(f"  -v, --verbose   Show detailed hex dump")
            print(f"  -c, --content   Show file contents as ASCII")
            sys.exit(0)
        else:
            disk_path = arg

    if not os.path.exists(disk_path):
        print(f"Error: file not found: {disk_path}")
        print(f"Usage: python3 {sys.argv[0]} [disk.img path] [-v] [-c]")
        sys.exit(1)

    print(f"Analyzing filesystem: {disk_path}")
    print()

    with open(disk_path, 'rb') as f:
        # Read MBR signature
        f.seek(510)
        mbr_sig = struct.unpack("<H", f.read(2))[0]
        print(f"MBR Signature: 0x{mbr_sig:04X} {'OK' if mbr_sig == 0xAA55 else 'ERROR'}")
        print()

        # Read super block
        sb = read_super_block(f)
        print_super_block(sb)

        # Read and analyze bitmaps
        inode_bitmap = read_inode_bitmap(f, sb)
        data_bitmap = read_data_bitmap(f, sb)

        inode_used, inode_total = analyze_inode_usage(inode_bitmap)
        data_used, data_total = analyze_block_usage(data_bitmap)

        print_bitmap_info(sb, inode_used, inode_total, data_used, data_total)

        # Print inode table
        print_inode_table(f, sb, inode_used)

        # Print directory tree
        print_directory_tree(f, sb, inode_used, show_content=show_content)

        # Verbose mode
        if verbose:
            print("=" * 60)
            print("Detailed Data Block Content (Verbose Mode)")
            print("=" * 60)

            # Show root directory content
            root_inode = read_d_inode(f, 0, sb)
            if root_inode:
                print_raw_block(f, root_inode['i_zone'][0], sb, "Root Directory Content")

            # Show inode bitmap
            print("=" * 60)
            print("Inode Bitmap (LBA 2)")
            print("=" * 60)
            for row in range(0, FS_BLOCK_SIZE, 16):
                hex_part = ' '.join(f'{b:02X}' for b in inode_bitmap[row:row+16])
                print(f"  {row:04X}: {hex_part}")
            print()

            # Show data bitmap
            print("=" * 60)
            print("Data Bitmap (LBA 3)")
            print("=" * 60)
            for row in range(0, FS_BLOCK_SIZE, 16):
                hex_part = ' '.join(f'{b:02X}' for b in data_bitmap[row:row+16])
                print(f"  {row:04X}: {hex_part}")
            print()

if __name__ == "__main__":
    main()