# TinyOS 项目总结

## 项目概述

TinyOS 是一个用 C++ 编写的 x86 32 位操作系统内核项目，采用 GRUB multiboot 引导标准，支持分页内存管理，目前已具备基础的内存分配器和中断控制系统。

## 技术架构

### 项目结构

```
tinyos/
├── arch/x86/           # x86 架构相关代码
│   ├── boot/          # 引导代码 (boot.S, prologue.cc)
│   ├── gdt.cc         # 全局描述符表
│   ├── idt.cc         # 中断描述符表
│   ├── int.S          # 中断服务程序汇编
│   └── iroute.cc      # 中断路由配置
├── hal/                # 硬件抽象层
│   ├── apic.cc         # Local APIC 驱动
│   ├── ioapic.cc      # I/O APIC 与中断路由
│   ├── acpi/          # ACPI 解析
│   │   ├── acpi.cc    # ACPI 基础驱动
│   │   └── parser/madt_parser.cc  # MADT 表解析
│   └── cpu.cc          # CPU 相关操作 (端口 IO, MSR)
├── kernel/             # 内核核心代码
│   ├── main.cc         # 内核入口
│   ├── mm/             # 内存管理
│   │   ├── pmm.cc      # 物理内存分配器 (位图算法)
│   │   ├── vmm.cc      # 虚拟内存管理 (页表映射)
│   │   └── heap.cc     # 内核堆分配器 (隐式空闲链表)
│   └── vga/            # VGA 文本模式驱动
├── libs/               # 基础库
│   └── fmt/            # 格式化输出库
├── includes/            # 头文件
│   ├── arch/x86/       # x86 相关头文件
│   ├── hal/            # HAL 头文件
│   ├── mm/             # 内存管理头文件
│   ├── klib/           # 内核库头文件
│   ├── list.h         # 嵌入式双向链表
│   └── timer.h        # 定时器事件头文件
├── linker/              # 链接脚本
│   └── linker.ld       # 内存布局配置
└── Makefile            # 构建配置
```

### 核心技术实现

#### 1. 引导与初始化
- **Multiboot 标准**：使用 GRUB 引导，加载后进入 32 位保护模式
- **页表初始化**：在引导阶段建立内核页表，虚拟地址 0xC0000000 开始映射
- **GDT/IDT 设置**：完成特权级切换和中断向量初始化

#### 2. 内存管理
- **物理内存分配器 (PMM)**：基于位图算法，从 multiboot 提供的内存映射中获取可用物理内存
- **虚拟内存管理 (VMM)**：支持页表映射/取消映射，虚拟地址到物理地址翻译
- **内核堆分配器 (kheap)**：隐式空闲链表分配器，支持分配、释放、合并操作

#### 3. 中断管理
- **IDT (中断描述符表)**：256 个中断向量已注册
- **LAPIC (本地 APIC)**：完成初始化，支持定时器中断
- **I/O APIC**：支持多线程处理器的中断路由
- **ACPI**：解析 MADT 表获取 LAPIC 和 I/O APIC 信息
- **中断路由**：支持自定义中断处理函数注册

#### 4. 定时器
- **APIC 定时器**：基于 Local APIC 的定时器中断
- **RTC 校准**：使用 RTC 周期性中断校准 APIC 频率
- **定时器事件**：支持 ONESHOT 和 PERIODIC 两种模式
- **链表管理**：使用嵌入式双向链表管理多个定时器事件

#### 5. 格式化输出
- 支持十进制、十六进制格式化输出
- 提供 `print`/`println` 函数

### 构建与运行

```bash
# 编译并生成 ISO
make

# 运行 (QEMU)
make run

# 调试模式
make debug
```

- 编译器：i386-elf-g++ (freestanding, C++20)
- 链接器：i386-elf-ld
- 模拟器：QEMU i386

## 已实现功能

| 模块 | 状态 | 说明 |
|------|------|------|
| 引导加载 | ✅ | Multiboot 标准，32 位保护模式 |
| 分页内存 | ✅ | 4KB 页大小，内核虚拟地址 0xC0000000+ |
| 物理内存分配 | ✅ | 位图算法 |
| 虚拟内存管理 | ✅ | 页表映射/翻译 |
| 内核堆 | ✅ | 隐式空闲链表，支持扩展 |
| GDT | ✅ | Ring0/Ring3 代码段和数据段 |
| IDT | ✅ | 256 个向量，已注册异常和 IRQ |
| LAPIC/APIC | ✅ | 初始化完成，支持定时器中断 |
| I/O APIC | ✅ | 多线程中断路由支持 |
| ACPI/MADT | ✅ | 解析 ACPI 表获取中断控制器信息 |
| 定时器 | ✅ | APIC 定时器 + RTC 校准，链表管理事件 |
| 嵌入式链表 | ✅ | 通用双向链表，可复用 |
| VGA 输出 | ✅ | 文本模式，print/println |

## 未来规划方向

### 短期目标

1. **进程/线程管理**
   - 进程控制块 (PCB) 设计
   - 线程调度器基础
   - 上下文切换实现

2. **系统调用**
   - 中断门调用机制
   - 基础系统调用接口 (write, fork, exec, exit)

3. **键盘驱动**
   - PS/2 键盘中断处理
   - 简易输入缓冲区

### 中期目标

1. **文件系统**
   - VFS (虚拟文件系统) 层
   - 简单文件系统 (FAT16/32 或自定义)
   - 文件描述符管理

2. **块设备驱动**
   - ATA/IDE 磁盘驱动
   - 磁盘缓存机制

3. **用户态支持**
   - 用户态内存区域
   - Ring3 特权级切换

### 长期目标

1. **多核支持 (SMP)**
   - APIC初始化
   - 处理器间中断 (IPI)
   - 多核调度

2. **高级内存管理**
   - 换页 (paging) 机制
   - 内存映射文件
   - SLAB/SLOB 分配器

3. **网络栈**
   - 简单网络驱动
   - TCP/IP 协议栈

4. **其他**
   - 鼠标驱动 (PS/2)
   - 声音驱动
   - GUI 基础

## 参考资料

- [OSDev Wiki](https://wiki.osdev.org/)
- [Intel® 64 and IA-32 Architectures Software Developer's Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [Multiboot2 Specification](https://www.gnu.org/software/grub/manual/multiboot2/)