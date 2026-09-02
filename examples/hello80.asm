; hello80.asm - Hello World in 8080 assembly
; Converted to 8086 with XLT86, then assembled with rasm86

        org     100h

start:
        lxi     d, msg
        mvi     c, 9
        call    5
        mvi     c, 0
        call    5

msg:    db      'Hello from 8080','$'

        end     start
