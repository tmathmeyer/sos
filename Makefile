######
#
# This stuff is all for building the kernel itself
#
######
kernel_flags := -nostdinc -Iinclude -fno-stack-protector -m64 -g -O0
arch ?= x86_64

BUILD := build
KERNEL := $(BUILD)/kernel.bin
ISO := $(BUILD)/os.iso
LINKER_SCRIPT := src/asm/$(arch)/linker.ld
GRUB_CFG := src/grub/grub.cfg
SFSDISK := $(BUILD)/sfsdisk
SOURCEDIR := ./src

C_SRC := $(shell find $(SOURCEDIR) -name '*.c')
C_OBJ := $(patsubst ./src/%, $(BUILD)/%, $(C_SRC:%.c=%.o))
ASM_SRC := $(wildcard src/asm/$(arch)/*.asm)
ASM_OBJ := $(patsubst src/asm/$(arch)/%.asm, \
	build/asm/$(arch)/%.o, $(ASM_SRC))

qemu: $(ISO) $(SFSDISK)
	@qemu-system-x86_64 -m 512M -hda $(ISO) -hdb $(SFSDISK)

bochs: $(ISO)
	@bochs -q

debugq: $(ISO) $(SFSDISK)
	@objcopy --only-keep-debug $(KERNEL) $(BUILD)/symbols
	@qemu-system-x86_64 -m 265M -d int -no-reboot -hda $(ISO) -hdb $(SFSDISK) -s -S &
	@gdb

$(SFSDISK): 
	dd if=/dev/zero of=$@  bs=200K  count=1

$(KERNEL): $(C_OBJ) $(ASM_OBJ)
	@ld -n -T $(LINKER_SCRIPT) -o $(KERNEL) $(C_OBJ) $(ASM_OBJ)
	@echo ld [objects] -o $(KERNEL)

$(BUILD)/%.o: src/%.c 
	@mkdir -p $(shell dirname $@)
	@gcc $(kernel_flags) -c $< -o $@
	@echo gcc -c $<

$(BUILD)/asm/$(arch)/%.o: src/asm/$(arch)/%.asm
	@mkdir -p $(shell dirname $@)
	@nasm -felf64 $< -o $@
	@echo nasm -felf64 $<

$(ISO): $(KERNEL) $(GRUB_CFG)
	@mkdir -p build/isofiles/boot/grub
	@cp $(KERNEL) build/isofiles/boot/kernel.bin
	@cp $(GRUB_CFG) build/isofiles/boot/grub
	@grub-mkrescue -d /usr/lib/grub/i386-pc -o $(ISO) build/isofiles 2>/dev/null
	@echo grub-mkrescue -o $(ISO)

clean:
	rm -rf build