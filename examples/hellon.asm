org 100h 
 
section .text 
 
start:
    mov dx,msg 
    mov cl,0x09
    int 0xE0
    xor cx,cx
    int 0xE0
 
int 20h
 
section .data
; program data
 
msg db 'Hello'
    db '$'
 
section .bss
; uninitialized data

