
SRC_DFS := -DMM_KERNEL

arch ?= x86_64
kernel := build/kernel.bin
iso := build/os.iso

linker_script := src/asm/$(arch)/linker.ld
grub_cfg := src/grub/grub.cfg
src_source_files := $(wildcard src/C/*.c)
src_object_files := $(patsubst src/C/%.c, \
	build/obj/%.o, $(src_source_files))
assembly_source_files := $(wildcard src/asm/$(arch)/*.asm)
assembly_object_files := $(patsubst src/asm/$(arch)/%.asm, \
	build/asm/$(arch)/%.o, $(assembly_source_files))

.PHONY: all clean run iso

all: qemu

debug: qemu-debug

kernel: $(kernel)

clean:
	@rm -fr build

qemu-debug: $(iso)
	@qemu-system-x86_64 -m 265M -d int -no-reboot -cdrom $(iso)

qemu: $(iso)
	@qemu-system-x86_64 -m 256M -cdrom $(iso)

iso: $(iso)

$(iso): $(kernel) $(grub_cfg)
	@mkdir -p build/isofiles/boot/grub
	@cp $(kernel) build/isofiles/boot/kernel.bin
	@cp $(grub_cfg) build/isofiles/boot/grub
	@grub-mkrescue -d /usr/lib/grub/i386-pc -o $(iso) build/isofiles 2>/dev/null
	@rm -r build/isofiles

$(kernel): $(assembly_object_files) $(linker_script) $(src_object_files)
	@ld -n -T $(linker_script) -o $(kernel) $(assembly_object_files) $(src_object_files)

build/asm/$(arch)/%.o: src/asm/$(arch)/%.asm
	@mkdir -p $(shell dirname $@)
	@nasm -felf64 $< -o $@

build/obj/%.o: src/C/%.c
	@mkdir -p $(shell dirname $@)
	@gcc -fno-stack-protector -m64 -c $< -o $@ $(SRC_DFS)
