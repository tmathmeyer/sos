global long_mode
global load_idt
global unwind

extern kmain
extern interrupt_handler
extern common_handler

section .text
bits 64
long_mode:
    xor rbp, rbp       ; Set %rbp to NULL
    push rbp            ; Push a NULL return address to the stack
    call kmain
    mov rax, 0x2f592f412f4b2f4f
    mov qword [0xb8000], rax
    hlt


%macro interrupt_handler 1
    global irq_%1
    irq_%1:
        cli
        mov rdi, dword %1
        jmp _precom2
%endmacro

_precom2:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rsi, rsp
    call common_handler
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    sti
    iretq
interrupt_handler 0
interrupt_handler 1
interrupt_handler 2
interrupt_handler 3
interrupt_handler 4
interrupt_handler 5
interrupt_handler 6
interrupt_handler 7
interrupt_handler 8
interrupt_handler 9
interrupt_handler 10
interrupt_handler 11
interrupt_handler 12
interrupt_handler 13
interrupt_handler 14
interrupt_handler 15
interrupt_handler 16
interrupt_handler 17
interrupt_handler 18
interrupt_handler 19
interrupt_handler 20
interrupt_handler 21
interrupt_handler 22
interrupt_handler 23
interrupt_handler 24
interrupt_handler 25
interrupt_handler 26
interrupt_handler 27
interrupt_handler 28
interrupt_handler 29
interrupt_handler 30
interrupt_handler 31
interrupt_handler 32
interrupt_handler 33


read_port:
mov edx, [esp + 4]
in al, dx   
ret

write_port:
mov   edx, [esp + 4]    
mov   al, [esp + 4 + 4]  
out   dx, al  
ret

unwind:
    push rbx
    push rbp
    ; RDI -> r8 = our array
    ; RSI -> r9 = array size
    ; r7 -> how many we found
    mov r8, rdi
    mov r9, rsi
    xor r10, r10
    xor r11, r11

.walk:
    test r9, r9
    jz .done
    dec r9

    mov rdx, [rbp + 8] ;rdx now has the previous frame's ins ptr
    test rdx, rdx
    mov [r8], r11
    jz .done

    mov rbp, [rbp + 0] ;back pointer for prev frame
    mov [r8], rdx
    add r8, 8
    inc r10
    loop .walk

.done:
    mov rax, r10
    pop rbp
    pop rbx
    ret