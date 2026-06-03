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

LINK_SCRIPT:=linker/linker.ld
TOOLS_DIR:=/usr/local/i386elf/bin
CC:=$(TOOLS_DIR)/i386-elf-g++
CFLAGS:= -std=c++20 -g -O0 -ffreestanding -fno-threadsafe-statics -fno-exceptions -fno-rtti -nostdlib -nostartfiles
DEFINES:=$(ARCH)
AS:=$(TOOLS_DIR)/i386-elf-as
LD:=$(TOOLS_DIR)/i386-elf-ld
LDFLAGS:=-ffreestanding

QEMU_MON_PORT := 45454

QEMU:=qemu-system-i386

all: $(OS)

$(BUILD_DIR)/%.o: %.cc $(HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(INCLUDE_DIR) -D$(DEFINES)

$(BUILD_DIR)/%.o: %.S $(HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(INCLUDE_DIR) -D$(DEFINES)

$(BINTARGET): $(OBJECTS) $(LINK_SCRIPT)
	$(LD) -T $(LINK_SCRIPT) -o $@ $(OBJECTS)

$(OS): $(BINTARGET)
	@objdump -d $(BINTARGET) > build/kernel_dump.txt
# check if TARGET is x86 multiboot
	@if grub-file --is-x86-multiboot $(BINTARGET); then \
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
	@grub-mkrescue -o $(OS) $(BUILD_DIR)/iso



run: all
	@$(QEMU) -cdrom $(OS) -m 2G -d int,cpu_reset -D qemu.log
#   -trace apic* -D qemu.log 

debug: all
	@echo running
	@$(QEMU) -s -S -m 2G -cdrom $(OS) -monitor telnet::$(QEMU_MON_PORT),server,nowait &
	@sleep 0.5
	@echo ===QEMU_READY===
	@telnet 127.0.0.1 $(QEMU_MON_PORT)

# 	gnome-terminal -- bash -c "$(QEMU) -cdrom $(OS) -s -S"
# 	gdb $(BINTARGET) -tui -ex "target remote :1234"

clean:
	@rm -rf $(BUILD_DIR)