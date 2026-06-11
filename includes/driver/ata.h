#pragma once

#include <stdint-gcc.h>

#include <hal/cpu.h>

#define IDE_DATA        0x1F0
#define IDE_ERR         0x1F1
#define IDE_SECT_CNT    0x1F2
#define IDE_LBA_LOW     0x1F3
#define IDE_LBA_MID     0x1F4
#define IDE_LBA_HIGH    0x1F5
#define IDE_DEV_HEAD    0x1F6
#define IDE_STATUS      0x1F7
#define IDE_CMD         0x1F7

#define IDE_CMD_READ    0x20    // 读扇区命令
#define IDE_CMD_WRITE   0x30    // 写扇区命令

#define IDE_STATUS_BSY  0x80    // 忙
#define IDE_STATUS_DRDY 0x40    // 就绪
#define IDE_STATUS_DRQ  0x08    // 数据请求

namespace ata {

// 等待硬盘非忙状态
static void ide_wait_ready() {
    while ((cpu::inb(IDE_STATUS) & IDE_STATUS_BSY)) {
        // 轮询等待 BSY 位清零
    }
}

// 等待硬盘准备好传输数据 (DRQ置1且BSY清零)
static int ide_wait_drq() {
    while (1) {
        uint8_t status = cpu::inb(IDE_STATUS);
        if (!(status & IDE_STATUS_BSY) && (status & IDE_STATUS_DRQ)) {
            return 1; // 成功
        }
        if (status & 0x01) { // ERR 位
            return 0; // 失败
        }
    }
}

/**
 * ide_read_sectors - 读取磁盘扇区
 * @lba:        28位的逻辑块地址
 * @count:      要读取的扇区数 (1-255)
 * @buf:        目标内存缓冲区
 * /// TODO: diffrent dev read
 */
int ide_read_sectors(uint32_t lba, uint8_t count, void* buf);

int ide_write_sector(uint32_t lba, uint8_t count, void* buf);

};