#include "libk.h"
#include "multiboot.h"
#include "interrupts.h"
#include "pages.h"


extern void load_idt(void);

void error(char *err, char errno) {
    kprintf("%4fs = %4fc\n", err, errno);
    for(;;);
}


void print_memory_area(struct memory_area *area) {
    kprintf("    start: %07x, length: %07x\n", area->base_addr, area->length);
}

void print_elf_section(struct elf_section *section) {
    kprintf("  addr: %07x, size: %07x, flags: %07x\n",
            section->addr, section->size, section->flags);
}


void print_elf_and_mmap(struct multiboot_header *multiboot_info) {
    struct elf_section_tag *elf_sections = find_by_type(multiboot_info, ELF_TAG_TYPE);
    struct memory_map_tag *mmap_sections = find_by_type(multiboot_info, MMAP_TAG_TYPE);
    if (elf_sections == 0 || mmap_sections == 0) {
        error("could not find elf or mmap sections", 'S');
    }

    kprintf("memory areas:\n");
    itr_memory_mapped_locations(mmap_sections, print_memory_area);

    kprintf("kernel sections:\n");
    itr_elf_sections(elf_sections, print_elf_section);
}


int kmain(struct multiboot_header *multiboot_info) {
    init_kernel_pages();
    load_idt();

    //intel_8259_set_irq_mask(I8259_MASK_ALL);
    //intel_8259_enable_irq(1);


    kio_init();
    clear_screen();

    int ERROR = valid_multiboot(multiboot_info);
    if (ERROR) {
        error("multiboot information invalid", ERROR);
        return 1;
    }
    print_elf_and_mmap(multiboot_info);

    for(;;);
}
