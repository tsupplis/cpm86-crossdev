org 100h 
 
start:
        mov cx,cs
        mov es,cx
        mov ds,cx
        mov cx, 0x09
        mov dx,msg 
        int 0xE0
        xor cx,cx
        int 0xE0

msg     db 'Hello from assembler (no data segment)$'
