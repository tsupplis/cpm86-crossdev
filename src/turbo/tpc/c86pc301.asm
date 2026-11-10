; In-place CLI for the supplied Turbo Pascal 3.01A CP/M-86 CMD image.
; Keep the CMD header, configuration, generated runtime and data group.
bits 16
cpu 8086
org -80h

; Code addresses start at file offset 80h; NASM's origin accounts for it.
%macro original_to 1
    incbin INPUT, ($-$$), (%1)+80h-($-$$)
%endmacro

original_to 280eh
    times 9 nop                ; CLI does not initialize the terminal screen.
original_to 285bh
    jmp cli_start
original_to 299bh
    jmp cli_failure
original_to 2cb4h
    jmp cli_failure
original_to 3485h
    jmp cli_failure
original_to 398ah

; Enter after Turbo has initialized its data, stack and source storage.
cli_start:
    cld
    mov word [228h], cli_putchar
    mov byte [385h], 2
    mov byte [cs:source_seen], 0
    mov byte [cs:mode_seen], 0
    mov byte [cs:compiling], 0
    xor cx, cx
    mov cl, [80h]
    cmp cl, 126
    jae cli_usage
    mov si, 81h
    mov di, tail
.tail:
    jcxz .tail_end
    mov al, [si]
    inc si
    mov [cs:di], al
    inc di
    loop .tail
.tail_end:
    mov byte [cs:di], 0
    mov si, tail
.next:
    call next_token
    jc .compile
    cmp byte [cs:bx], '-'
    je .option
    cmp byte [cs:source_seen], 0
    jne cli_usage
    mov byte [cs:source_seen], 1
    mov di, 100h
    xor cx, cx
.source:
    mov al, [cs:bx]
    inc bx
    or al, al
    jz .source_end
    cmp cx, 14
    jae cli_usage
    cmp al, 'a'
    jb .copy
    cmp al, 'z'
    ja .copy
    sub al, 20h
.copy:
    mov [di], al
    inc di
    inc cx
    jmp .source
.source_end:
    mov byte [di], 0
    push si
    call validate_name
    mov bx, 100h
    call 3014h                 ; Turbo's FCB parser, default PAS extension.
    mov si, 200h
    mov di, 311h
    call 303bh
    pop si
    jmp .next
.option:
    cmp byte [cs:bx+1], 0
    je cli_usage
    mov al, [cs:bx+1]
    or al, 20h
    cmp al, 'h'
    jne .switch
    cmp byte [cs:bx+2], 0
    je cli_help
.switch:
    cmp byte [cs:bx+2], 0
    je cli_usage
    cmp byte [cs:bx+3], 0
    jne cli_usage
    mov ax, [cs:bx+1]
    or ax, 2020h
    cmp ax, 'cc'
    je .cmd
    cmp ax, 'ch'
    je .chain
    mov di, 2dfh
    cmp ax, 'sc'
    je .number
    mov di, 2e1h
    cmp ax, 'sd'
    je .number
    mov di, 2e3h
    cmp ax, 'mi'
    je .number
    mov di, 2e5h
    cmp ax, 'ma'
    jne cli_usage
.number:
    call next_token
    jc cli_usage
    xor dx, dx
    xor cx, cx
.hex:
    mov al, [cs:bx]
    inc bx
    or al, al
    jz .value
    inc cx
    cmp cx, 4
    ja cli_usage
    cmp al, 'a'
    jb .digit
    cmp al, 'f'
    ja cli_usage
    sub al, 20h
.digit:
    sub al, '0'
    cmp al, 9
    jbe .shift
    sub al, 7
    cmp al, 10
    jb cli_usage
    cmp al, 15
    ja cli_usage
.shift:
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    xor ah, ah
    or dx, ax
    jmp .hex
.value:
    or cx, cx
    jz cli_usage
    cmp di, 2dfh
    jne .data
    cmp dx, 0d88h
    ja cli_usage
    or dx, dx
    jz .store
    add dx, 278h              ; Runtime-inclusive paragraphs, as in IDE.
    jmp .store
.data:
    cmp di, 2e1h
    jne .store
    cmp dx, 0fd3h
    ja cli_usage
    or dx, dx
    jz .store
    add dx, 2dh
.store:
    mov [di], dx
    jmp .next
.cmd:
    mov al, 2
    jmp .mode
.chain:
    mov al, 3
.mode:
    cmp byte [cs:mode_seen], 0
    jne cli_usage
    mov byte [cs:mode_seen], 1
    mov [385h], al
    jmp .next
.compile:
    cmp byte [cs:source_seen], 0
    je cli_usage
    mov ax, [2e3h]
    cmp ax, [2e5h]
    ja cli_usage
    call 2d9ah                 ; Load workfile through CP/M FCB I/O.
    mov byte [cs:compiling], 1
    call 32e2h                 ; Compile and write CMD/CHN; never run it.
    xor dx, dx
    jmp cli_exit

; Reject truncation and wildcards: one optional A:-P: drive and an 8.3 name.
validate_name:
    mov bx, 100h
    cmp byte [bx+1], ':'
    jne .name
    cmp byte [bx], 'A'
    jb cli_usage
    cmp byte [bx], 'P'
    ja cli_usage
    add bx, 2
.name:
    xor cx, cx
    mov dl, 8
.char:
    mov al, [bx]
    inc bx
    or al, al
    jz .end
    cmp al, '.'
    je .extension
    cmp al, 'A'
    jb .numeric
    cmp al, 'Z'
    jbe .valid
.numeric:
    cmp al, '0'
    jb .punctuation
    cmp al, '9'
    jbe .valid
.punctuation:
    cmp al, '_'
    je .valid
    cmp al, '-'
    je .valid
    cmp al, '$'
    jne cli_usage
.valid:
    inc cl
    cmp cl, dl
    ja cli_usage
    jmp .char
.extension:
    cmp dl, 8
    jne cli_usage
    or cl, cl
    jz cli_usage
    mov dl, 3
    xor cx, cx
    jmp .char
.end:
    or cl, cl
    jz cli_usage
    ret

; Return CS:BX token from the copied base-page command tail, carry at end.
next_token:
.space:
    mov al, [cs:si]
    cmp al, ' '
    je .skip
    cmp al, 9
    jne .start
.skip:
    inc si
    jmp .space
.start:
    or al, al
    jz .end
    mov bx, si
.scan:
    mov al, [cs:si]
    or al, al
    jz .found
    cmp al, ' '
    je .terminate
    cmp al, 9
    je .terminate
    inc si
    jmp .scan
.terminate:
    mov byte [cs:si], 0
    inc si
.found:
    clc
    ret
.end:
    stc
    ret

cli_failure:
    cmp byte [cs:compiling], 0
    je .message
    cmp byte [2d1h], 2
    jb .message
    mov dx, 35ah               ; Compiler already closed the partial output.
    mov cl, 19
    int 0e0h
.message:
    mov dx, failure
    call print
    jmp cli_exit
cli_usage:
    mov dx, bad_arguments
    call print
    mov dx, usage
    call print
    jmp cli_exit
cli_help:
    mov dx, usage
    call print
cli_exit:
    ; CP/M-86 1.x has no command return-code service (unlike DOS).
    xor dx, dx
    xor cx, cx                 ; P_TERM: return to CCP, no resident process.
    int 0e0h

; Print CS-resident text through CP/M console output.
print:
    push si
    mov si, dx
.char:
    mov dl, [cs:si]
    inc si
    or dl, dl
    jz .end
    push si
    mov cl, 2
    int 0e0h
    pop si
    jmp .char
.end:
    pop si
    ret

; Pascal character callback, replacing the compiler's terminal/editor output.
cli_putchar:
    push bp
    mov bp, sp
    push ax
    push bx
    push cx
    push dx
    mov dl, [bp+4]
    mov cl, 2
    int 0e0h
    pop dx
    pop cx
    pop bx
    pop ax
    pop bp
    ret 2

source_seen: db 0
mode_seen: db 0
compiling: db 0
bad_arguments: db 'TPC: invalid arguments', 13, 10, 0
failure: db 13, 10, 'TPC: compilation failed', 13, 10, 0
usage:
    db 'Turbo Pascal 3.01A for CP/M-86 compiler', 13, 10
    db 'Usage: TPC [-cc|-ch] [-sc HEX] [-sd HEX] [-mi HEX] [-ma HEX] file[.pas]', 13, 10
    db '  -cc CMD (default), -ch CHN; -sc/-sd minimum code/data paragraphs', 13, 10
    db '  -mi/-ma minimum/maximum free memory paragraphs; HEX = 1-4 hex digits', 13, 10
    db '  -h help; CP/M-86 errors are reported on the console', 13, 10, 0
tail: times 128 db 0

original_to 4900h
; Include all remaining code padding and the complete original data group.
incbin INPUT, ($-$$)
