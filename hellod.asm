program     segment
            assume cs:program, ds:program
            org 100h
_start:     
            lea    dx,[_message]
            mov    cx, 09h
            int    0E0h
            xor    cx,cx
            int    0E0h
            
_message    db "Hello$"
program     ends
            end _start
