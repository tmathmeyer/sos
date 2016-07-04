global long_mode
global load_idt

extern kmain
extern _divide_by_zero_handler

global divide_by_zero_handler

section .text
bits 64
long_mode:
	call kmain
    mov rax, 0x2f592f412f4b2f4f
    mov qword [0xb8000], rax
    hlt

divide_by_zero_handler:
    call _divide_by_zero_handler
    iretq
