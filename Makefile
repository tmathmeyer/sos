kernel_flags := -nostdinc -Isrc/include -fno-stack-protector -m64
arch ?= x86_64

BUILD := build
KERNEL := $(BUILD)/kernel.bin
ISO := $(BUILD)/os.iso
LINKER_SCRIPT := src/asm/$(arch)/linker.ld
GRUB_CFG := src/grub/grub.cfg
ROOTFS := rootfs


C_SRC := $(shell find $(SOURCEDIR) -name '*.c')
C_OBJ := $(patsubst ./src/%, $(BUILD)/%, $(C_SRC:%.c=%.o))
ASM_SRC := $(wildcard src/asm/$(arch)/*.asm)
ASM_OBJ := $(patsubst src/asm/$(arch)/%.asm, \
	build/asm/$(arch)/%.o, $(ASM_SRC))

qemu: $(ISO)
	@qemu-system-x86_64 -m 512M -hda $(ISO)

bochs: $(ISO)
	@bochs -q

debugq: $(ISO)
	@qemu-system-x86_64 -m 265M -d int -no-reboot -hda $(ISO)

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
	@mkdir -p build/isofiles/root
	@cp -R $(ROOTFS)/* build/isofiles/root/
	@cp $(KERNEL) build/isofiles/boot/kernel.bin
	@cp $(GRUB_CFG) build/isofiles/boot/grub
	@grub-mkrescue -d /usr/lib/grub/i386-pc -o $(ISO) build/isofiles 2>/dev/null
	@echo grub-mkrescue -o $(ISO)

clean:
	rm -rf build