bits 64

global __rax

__rax: pop rax

video_mode:
    mov ah, 0x00
    mov al, 0x13
    int 0x10
    ret
