global long_mode
global load_idt

extern kmain
extern idt_handler



section .text
bits 64
long_mode:
	call kmain
    mov rax, 0x2f592f412f4b2f4f
    mov qword [0xb8000], rax
    hlt

%macro NON_ERROR_INTERRUPT 1
    global isr_%1
    isr_%1:
        push 0 ; error code
        push %1 ; interrupt code0
        jmp idt_common_prehandler
%endmacro

%macro ERROR_INTERRUPT 1
    global isr_%1
    isr_%1:
        push %1 ; interrupt code0
        jmp idt_common_prehandler
%endmacro
NON_ERROR_INTERRUPT 0
NON_ERROR_INTERRUPT 1
NON_ERROR_INTERRUPT 2
NON_ERROR_INTERRUPT 3
NON_ERROR_INTERRUPT 4
NON_ERROR_INTERRUPT 5
NON_ERROR_INTERRUPT 6
NON_ERROR_INTERRUPT 7
ERROR_INTERRUPT 8
NON_ERROR_INTERRUPT 9
ERROR_INTERRUPT 10
ERROR_INTERRUPT 11
ERROR_INTERRUPT 12
ERROR_INTERRUPT 13
ERROR_INTERRUPT 14
NON_ERROR_INTERRUPT 15
NON_ERROR_INTERRUPT 16
NON_ERROR_INTERRUPT 17
NON_ERROR_INTERRUPT 18
NON_ERROR_INTERRUPT 19
NON_ERROR_INTERRUPT 20
NON_ERROR_INTERRUPT 21
NON_ERROR_INTERRUPT 22
NON_ERROR_INTERRUPT 23
NON_ERROR_INTERRUPT 24
NON_ERROR_INTERRUPT 25
NON_ERROR_INTERRUPT 26
NON_ERROR_INTERRUPT 27
NON_ERROR_INTERRUPT 28
NON_ERROR_INTERRUPT 29
NON_ERROR_INTERRUPT 30
NON_ERROR_INTERRUPT 31
NON_ERROR_INTERRUPT 32
NON_ERROR_INTERRUPT 33
NON_ERROR_INTERRUPT 34
NON_ERROR_INTERRUPT 35
NON_ERROR_INTERRUPT 36
NON_ERROR_INTERRUPT 37
NON_ERROR_INTERRUPT 38
NON_ERROR_INTERRUPT 39
NON_ERROR_INTERRUPT 40
NON_ERROR_INTERRUPT 41
NON_ERROR_INTERRUPT 42
NON_ERROR_INTERRUPT 43
NON_ERROR_INTERRUPT 44
NON_ERROR_INTERRUPT 45
NON_ERROR_INTERRUPT 46
NON_ERROR_INTERRUPT 47

idt_common_prehandler:
    ; Push data
    
    ; Create a pointer to ESP so that we can access
    ; the data using a struct pointer in C.
    mov rax, rsp
    push rax
    
    call idt_handler
    
    pop rax ; Pop the pointer

    add esp, 8 ; Remove the error code and the interrupt
    iret ; TODO: Change with iret once this works.





read_port:
mov edx, [esp + 4]
in al, dx   
ret

write_port:
mov   edx, [esp + 4]    
mov   al, [esp + 4 + 4]  
out   dx, al  
ret
