global long_mode
global load_idt
global kernel_segment
global keyboard_interrupt_handler

section .text
bits 64
long_mode:
	extern kmain
	call kmain
    mov rax, 0x2f592f412f4b2f4f
    mov qword [0xb8000], rax
    hlt

load_idt:
    mov edx, [esp + 4]
    lidt [edx]
    sti                 ;turn on interrupts
    ret

kernel_segment:
    mov eax, cs
    ret

keyboard_interrupt_handler:
    extern keyboard_handler
    call keyboard_handler
    iretd
