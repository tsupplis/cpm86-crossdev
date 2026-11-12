; Appended startup support for Personal BASIC 1.2N.
; All private state lives in the enlarged code group.
bits 16
cpu 8086
org 0bf60h
    jmp near initialize
    jmp near read_character
    jmp near finish_end
    jmp near finish_last
    jmp near prompt
    jmp near banner
initialize:
    push ax
    push cx
    push dx
    push si
    push di
    push es
    push ds
    pop es
    mov si,[di+5ah]             ; Runtime's preserved, word-addressed tail.
    shl si,1
    xor cx,cx
    mov cl,[si]
    inc si
    mov byte [cs:pending],0
    mov byte [cs:autorun],0
    call spaces
    or cx,cx
    jnz .arguments
    jmp .done
.arguments:
    mov di,command+5
    xor dx,dx
.filename:
    jcxz .filename_end
    mov al,[si]
    cmp al,' '
    je .filename_end
    cmp al,9
    je .filename_end
    cmp al,21h
    jb .invalid
    cmp al,7eh
    ja .invalid
    cmp al,'"'
    je .invalid
    cmp al,','
    je .invalid
    cmp al,'['
    je .invalid
    cmp al,']'
    je .invalid
    cmp al,'*'
    je .invalid
    cmp al,'?'
    je .invalid
    cmp al,'/'
    je .invalid
    cmp al,92
    je .invalid
    cmp dx,14                  ; D:12345678.123
    jae .invalid
    mov [cs:di],al
    inc di
    inc dx
    inc si
    dec cx
    jmp .filename
.filename_end:
    or dx,dx
    jz .invalid
    mov byte [cs:di],'"'
    mov byte [cs:di+1],13
    add di,2
    mov [cs:limit],di
    call spaces
    jcxz .ready
    lodsb
    dec cx
    cmp al,'['
    jne .invalid
    jcxz .invalid
    lodsb
    dec cx
    and al,0dfh
    cmp al,'R'
    jne .invalid
    jcxz .run
    cmp byte [si],']'
    jne .trailing
    inc si
    dec cx
.trailing:
    call spaces
    jcxz .run
    jmp .invalid
.run:
    mov byte [cs:autorun],1
    mov byte [cs:command],'R'
    mov byte [cs:command+1],'U'
    mov byte [cs:command+2],'N'
.ready:
    mov word [cs:cursor],command
    mov byte [cs:pending],1
.done:
    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop ax
    mov bx,[di+5ah]             ; Displaced startup instruction.
    ret
.invalid:
    ; Invalid CLI returns to the interpreter after an explicit diagnostic.
    push ds
    push cs
    pop ds
    mov dx,usage
    mov cl,9
    int 0e0h
    pop ds
    jmp .done
spaces:
    jcxz .end
    cmp byte [si],' '
    je .skip
    cmp byte [si],9
    jne .end
.skip:
    inc si
    dec cx
    jmp spaces
.end:
    ret
read_character:
    cmp byte [cs:pending],1
    jne .normal
    push bx
    mov bx,[cs:cursor]
    xor ax,ax
    mov al,[cs:bx]
    inc bx
    mov [cs:cursor],bx
    cmp bx,[cs:limit]
    jb .return
    mov byte [cs:pending],2     ; Drained; next command dispatch consumes this state.
.return:
    pop bx
    ret
.normal:
    cmp byte [017fh],0          ; Displaced console-read instruction.
    jmp 0254h
; END arrives after execution has unwound to the command frame.
finish_end:
    mov bx,[di+0b2h]
    cmp byte [cs:autorun],0
    je .normal
    cmp bx,0bch                 ; END, as distinct from STOP/error markers.
    je finish_last
.normal:
    jmp 02f3bh
finish_last:
    cmp byte [cs:autorun],0
    je .normal
    mov byte [cs:autorun],0
    jmp 02ffdh                 ; SYSTEM closes files and returns from BASIC.
.normal:
    jmp 02fceh
prompt:
    cmp byte [cs:pending],2     ; Only the injected command retains autorun.
    je .startup
    mov byte [cs:autorun],0
.startup:
    mov byte [cs:pending],0
    mov bx,[di+234h]            ; Displaced command-dispatch instruction.
    jmp 02b44h
; Skip only the startup banner formatter for a valid [R] request.
banner:
    cmp byte [cs:autorun],0
    jne .quiet
    lea bx,[041ah]             ; Displaced banner separator address.
    jmp 02af0h
.quiet:
    jmp 02b07h                 ; Continue interpreter initialization.
pending db 0
autorun db 0
cursor dw 0
limit dw 0
command db 'OLD "'
    times 16 db 0
usage db 13,10,'Usage: BASIC file [R]  (or [R)',13,10,'$'
