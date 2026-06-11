#include <driver/ata.h>

namespace ata {

int ide_read_sectors(uint32_t lba, uint8_t count, void* buf) {
    if (count == 0) return -1;

    // 1. 等待驱动器不忙
    ide_wait_ready();

    // 2. 写入设置参数
    cpu::outb(IDE_SECT_CNT, count);                      // 读多少个扇区
    cpu::outb(IDE_LBA_LOW,  (uint8_t)(lba & 0xFF));       // LBA 0-7 位
    cpu::outb(IDE_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));  // LBA 8-15 位
    cpu::outb(IDE_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF)); // LBA 16-23 位
    
    // 设置 0x1F6: LBA模式 (0xE0) | 主盘 (0x00) | LBA 24-27位
    cpu::outb(IDE_DEV_HEAD, 0xE0 | ((lba >> 24) & 0x0F)); 

    // 3. 发送读命令
    cpu::outb(IDE_CMD, IDE_CMD_READ);

    uint8_t* ptr = (uint8_t*)buf;
    
    // 4. 循环读取每个扇区的数据
    for (int i = 0; i < count; i++) {
        // 每一个扇区传输前，都必须等待硬件将 DRQ 位置 1
        if (!ide_wait_drq()) {
            return -1; // 磁盘读取发生物理错误
        }
        
        // 从 0x1F0 端口搬运一整个扇区 (512 字节 = 256 个字)
        cpu::insw(IDE_DATA, ptr, 256);
        ptr += 512;
    }

    return 0;
}

int ide_write_sector(uint32_t lba, uint8_t count, void* buf) {
    if (count == 0) return -1;

    ide_wait_ready();

    // 1. 设置参数
    cpu::outb(IDE_SECT_CNT, count);
    cpu::outb(IDE_LBA_LOW,  (uint8_t)(lba & 0xFF));
    cpu::outb(IDE_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    cpu::outb(IDE_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    cpu::outb(IDE_DEV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));

    // 2. 发送写命令
    cpu::outb(IDE_CMD, IDE_CMD_WRITE);

    const uint8_t* ptr = (const uint8_t*)buf;

    // 3. 循环写入每个扇区
    for (int i = 0; i < count; i++) {
        // 🌟 必须先等 DRQ 准备好接收数据
        if (!ide_wait_drq()) {
            return -1;
        }

        // 向 0x1F0 端口推进一个扇区的数据 (256个字)
        cpu::outsw(IDE_DATA, ptr, 256);
        ptr += 512;
    }

    return 0;
}

} // namespace ata


