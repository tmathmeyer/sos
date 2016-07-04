bits 64

global read_port
global write_port

read_port:
    mov edx, [esp + 4]
    in al, dx
    ret

write_port:
    mov edx, [esp + 4]
    mov al, [esp + 4 + 4]
    out dx, al
    ret

video_mode:
    mov ah, 0x00
    mov al, 0x13
    int 0x10
    ret
