.PHONY: all clean

NAME:=tiny

BUILD_DIR:=build
INCLUDE_DIR:=includes
SRC_DIR:=arch kernel libs hal
HEADERS:=$(shell find $(INCLUDE_DIR) -name '*.h')
SOURCES:=$(shell find $(SRC_DIR) -name '*.cc' -o -name '*.S' -o -name '*.c')
OBJECTS:=$(patsubst %.cc, $(BUILD_DIR)/%.o, $(SOURCES))
OBJECTS:=$(patsubst %.S, $(BUILD_DIR)/%.o, $(OBJECTS))

ARCH:=ARCH_IA32

BINTARGET:=$(BUILD_DIR)/$(NAME).bin
OS:=$(BUILD_DIR)/$(NAME).iso
DISK:=$(BUILD_DIR)/disk.img

LINK_SCRIPT:=linker/linker.ld
CROSS_PREFIX?=i686-elf-
CC:=$(CROSS_PREFIX)g++
CFLAGS:= -std=c++20 -g -O0 -ffreestanding -fno-threadsafe-statics -fno-exceptions -fno-rtti -nostdlib -nostartfiles
DEFINES:=$(ARCH)
AS:=$(CROSS_PREFIX)as
LD:=$(CROSS_PREFIX)ld
OBJDUMP:=$(CROSS_PREFIX)objdump
GRUB_FILE:=$(CROSS_PREFIX)grub-file
GRUB_MKRESCUE:=$(CROSS_PREFIX)grub-mkrescue
LDFLAGS:=-ffreestanding

QEMU_MON_PORT := 45454

QEMU:=qemu-system-i386

all: $(OS) $(DISK)

$(BUILD_DIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(INCLUDE_DIR) -D$(DEFINES)

$(BUILD_DIR)/%.o: %.S $(HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(INCLUDE_DIR) -D$(DEFINES)

$(BINTARGET): $(OBJECTS) $(LINK_SCRIPT)
	$(LD) -T $(LINK_SCRIPT) -o $@ $(OBJECTS)

$(OS): $(BINTARGET)
	@$(OBJDUMP) -d $(BINTARGET) > build/kernel_dump.txt
# check if TARGET is x86 multiboot
	@if $(GRUB_FILE) --is-x86-multiboot $(BINTARGET); then \
		echo "multiboot confirmed"; \
	else \
		echo "Error: the file is not multiboot" >&2; \
		exit 1; \
	fi
	@mkdir -p $(BUILD_DIR)/iso/boot/grub
# make grub.cfg
	@printf "menuentry '%s' {\n" "$(NAME)" > $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@printf "\tmultiboot /boot/%s.bin\n" "$(NAME)" >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@printf "}\n" >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@cp $(BINTARGET) $(BUILD_DIR)/iso/boot/$(NAME).bin
	@$(GRUB_MKRESCUE) -o $(OS) $(BUILD_DIR)/iso

$(DISK): mkfs/mkfs.py
	python3 mkfs/mkfs.py
	mv disk.img $(BUILD_DIR)/disk.img

run: all
	@$(QEMU) -boot d -cdrom $(OS) -m 2G -drive file=$(DISK),format=raw,if=ide,index=0
#   -trace apic* -D qemu.log 

debug: all
	@echo running
	@$(QEMU) -s -S -m 2G -boot d -cdrom $(OS) -drive file=$(DISK),format=raw,if=ide,index=0 -monitor telnet::$(QEMU_MON_PORT),server,nowait &
	@sleep 0.5
	@echo ===QEMU_READY===
	@telnet 127.0.0.1 $(QEMU_MON_PORT)

# 	gnome-terminal -- bash -c "$(QEMU) -cdrom $(OS) -s -S"
# 	gdb $(BINTARGET) -tui -ex "target remote :1234"

clean:
	@rm -rf $(BUILD_DIR)
