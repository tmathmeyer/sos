global long_mode
global load_idt

extern kmain
extern interrupt_handler



section .text
bits 64
long_mode:
	call kmain
    mov rax, 0x2f592f412f4b2f4f
    mov qword [0xb8000], rax
    hlt


%macro interrupt_handler 1
    global interrupt_handler_%1
    extern _interrupt_handler_%1
    interrupt_handler_%1:
        call _interrupt_handler_%1
        iretq
%endmacro

interrupt_handler 00
interrupt_handler 21
interrupt_handler 08
interrupt_handler 0d

read_port:
mov edx, [esp + 4]
in al, dx   
ret

write_port:
mov   edx, [esp + 4]    
mov   al, [esp + 4 + 4]  
out   dx, al  
ret






%if 0


extern _divide_by_zero_handler
global divide_by_zero_handler
divide_by_zero_handler:
    call _divide_by_zero_handler
    iretq




%macro no_error_code_interrupt_handler 1
	global interrupt_handler_%1
	interrupt_handler_%1:
	push    dword 0                     ; push 0 as error code
	push    dword %1                    ; push the interrupt number
	jmp     common_interrupt_handler    ; jump to the common handler
%endmacro

%macro error_code_interrupt_handler 1
	global interrupt_handler_%1
	interrupt_handler_%1:
	push    dword %1                    ; push the interrupt number
	jmp     common_interrupt_handler    ; jump to the common handler
%endmacro

common_interrupt_handler:               ; the common parts of the generic interrupt handler
	; save the registers
	push    rax
	push    rbx
	push    rbp
	; call the C function
	call    interrupt_handler
	; restore the registers
	pop     rbp
	pop     rbx
	pop     rax
	; restore the esp
	add     esp, 8
	; return to the code that got interrupted
	iret

no_error_code_interrupt_handler 0       ; create handler for interrupt 0
no_error_code_interrupt_handler 1       ; create handler for interrupt 1
error_code_interrupt_handler              7       ; create handler for interrupt 7
%endif
