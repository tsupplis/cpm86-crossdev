codeseg segment para public 'code'
    assume cs:codeseg
    public bdos_
bdos_ proc near
    push bp
    mov bp,sp
    push si
    push di
    mov cx,4[bp]
    mov dx,6[bp]
    cmp cx,12
    jne notver
    mov dl,'V'
    mov cx,2
    int 224
    mov ax,VERSION
    jmp short done
notver:
    cmp cx,24
    jne notmap
    xor ax,ax
    jmp short done
notmap:
    cmp cx,108
    jne normal
    cmp dx,37
    jne bad
    mov dl,'Y'
    jmp short report
bad:
    cmp dx,0ff00h
    jne wrong
    mov dl,'F'
    jmp short report
wrong:
    mov dl,'N'
report:
    mov cx,2
normal:
    int 224
    and ax,255
done:
    pop di
    pop si
    pop bp
    ret
bdos_ endp
codeseg ends
    end
