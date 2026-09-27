program     segment
            assume cs:program, ds:program
            org 100h
_start:     
            mov     cx,cs
            mov     es,cx
            mov     ds,cx
            mov     cx,09h
            mov     dx,offset msg
            int     0E0h
            xor     cx,cx
            int     0E0h
            
msg         db "Hello from assembler (no data segment)$"
program     ends
            end _start
