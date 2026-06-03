# TinyOS 项目总结

## 项目概述

TinyOS 是一个用 C++ 编写的 x86 32 位操作系统内核项目，采用 GRUB multiboot 引导标准，支持分页内存管理、进程管理和系统调用，目前已经具备了基础的进程调度能力和系统调用接口。

## 最近更新 (process-dev 分支)

### 新增功能

1. **进程/线程管理**
   - PCB (进程控制块) 设计：包含内核栈指针、页目录地址、PID、状态、优先级等
   - 进程创建 (`create_process`)：支持创建用户态进程
   - 进程调度器 (`schedule`)：基于时间片轮转调度
   - 就绪队列管理
   - PID 位图管理
   - 上下文切换汇编 (`arch/x86/proc_switch.S`)

2. **Copy-On-Write (COW) 写时复制**
   - 页故障处理中实现 COW 机制
   - 引用计数支持
   - 页面懒复制

3. **TSS (任务状态段)**
   - `arch/x86/tss.cc` / `includes/arch/x86/tss.h`
   - 用于 Ring0 → Ring3 特权级切换时的内核栈切换

4. **系统调用接口**
   - `kernel/sys_call.cc` / `includes/sys_call.h`
   - 中断 `0x80` 触发系统调用
   - 已实现 `fork` 和 `write` 系统调用

5. **用户态支持**
   - 用户代码段/数据段支持
   - 用户栈创建
   - `process_start_wrapper` 封装用户进程入口

### 技术架构

### 项目结构

```
tinyos/
├── arch/x86/           # x86 架构相关代码
│   ├── boot/          # 引导代码 (boot.S, prologue.cc)
│   ├── gdt.cc         # 全局描述符表
│   ├── idt.cc         # 中断描述符表
│   ├── int.S          # 中断服务程序汇编
│   ├── iroute.cc      # 中断路由配置
│   ├── tss.cc         # TSS 任务状态段
│   └── proc_switch.S  # 进程上下文切换汇编
├── hal/                # 硬件抽象层
│   ├── apic.cc         # Local APIC 驱动
│   ├── ioapic.cc      # I/O APIC 与中断路由
│   ├── cpu.cc          # CPU 相关操作 (端口 IO, MSR)
│   └── keyboard.cc     # PS/2 键盘驱动
├── kernel/             # 内核核心代码
│   ├── main.cc         # 内核入口
│   ├── proc.cc         # 进程管理实现
│   ├── sys_call.cc     # 系统调用处理
│   ├── timer.cc        # 定时器管理
│   └── mm/             # 内存管理
│       ├── pmm.cc      # 物理内存分配器 (位图算法)
│       ├── vmm.cc      # 虚拟内存管理 (页表映射)
│       └── heap.cc     # 内核堆分配器 (隐式空闲链表)
├── libs/               # 基础库
│   └── string.cc      # 字符串操作库
├── includes/            # 头文件
│   ├── arch/x86/       # x86 相关头文件 (gdt.h, idt.h, tss.h)
│   ├── hal/            # HAL 头文件
│   ├── mm/             # 内存管理头文件 (pmm.h, vmm.h, page.h)
│   ├── klib/           # 内核库头文件 (fmt.h, string.h)
│   ├── proc.h          # 进程控制块定义
│   ├── sys_call.h      # 系统调用接口
│   └── timer.h         # 定时器事件头文件
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
- **页故障处理 (COW)**：Copy-On-Write 写时复制机制

#### 4. 定时器
- **APIC 定时器**：基于 Local APIC 的定时器中断
- **RTC 校准**：使用 RTC 周期性中断校准 APIC 频率
- **定时器事件**：支持 ONESHOT 和 PERIODIC 两种模式
- **链表管理**：使用嵌入式双向链表管理多个定时器事件

#### 5. 进程管理
- **PCB (进程控制块)**：包含内核栈、PID、状态、优先级、页目录等
- **进程创建**：支持用户态进程创建
- **Copy-On-Write**：页表共享与懒复制
- **上下文切换**：汇编实现进程切换
- **调度器**：时间片轮转调度
- **TSS**：支持 Ring0 → Ring3 内核栈切换

#### 6. 系统调用
- **中断 `0x80`**：系统调用入口
- **已实现**：`fork` (进程创建)、`write` (输出)
- **用户态支持**：Ring3 用户进程运行

#### 7. 格式化输出
- 支持十进制、十六进制格式化输出
- 提供 `print`/`println` 函数

#### 8. 字符串操作
- `strlen`、`strcmp`、`strcpy`、`memcpy`、`memset` 等

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
| 虚拟内存管理 | ✅ | 页表映射/翻译，COW 写时复制 |
| 内核堆 | ✅ | 隐式空闲链表，支持扩展 |
| GDT | ✅ | Ring0/Ring3 代码段和数据段 |
| IDT | ✅ | 256 个向量，已注册异常和 IRQ |
| LAPIC/APIC | ✅ | 初始化完成，支持定时器中断 |
| I/O APIC | ✅ | 多线程中断路由支持 |
| ACPI/MADT | ✅ | 解析 ACPI 表获取中断控制器信息 |
| 定时器 | ✅ | APIC 定时器 + RTC 校准，链表管理事件 |
| 嵌入式链表 | ✅ | 通用双向链表，可复用 |
| VGA 输出 | ✅ | 文本模式，print/println |
| 进程管理 | ✅ | PCB、进程创建、调度器、上下文切换 |
| Copy-On-Write | ✅ | 写时复制，引用计数 |
| TSS | ✅ | 任务状态段，内核栈切换 |
| 系统调用 | ✅ | 中断 0x80，fork/write |
| 用户态支持 | ✅ | Ring3 进程运行 |
| 键盘驱动 | ✅ | PS/2 键盘中断处理 |
| 字符串库 | ✅ | strlen, strcpy, memcpy 等 |

## 未来规划方向

### 短期目标

1. **系统调用扩展**
   - exec, exit, wait 等系统调用
   - 文件描述符管理

2. **进程退出与回收**
   - 进程资源回收
   - 父子进程关系处理

3. **键盘输入完善**
   - 输入缓冲区完善
   - 键盘事件到用户进程的传递

### 中期目标

1. **文件系统**
   - VFS (虚拟文件系统) 层
   - 简单文件系统 (FAT16/32 或自定义)
   - 文件描述符管理

2. **块设备驱动**
   - ATA/IDE 磁盘驱动
   - 磁盘缓存机制

3. **用户态库**
   - 简化版 libc
   - 用户态 printf 实现

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