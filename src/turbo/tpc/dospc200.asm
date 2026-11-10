; CLI for the supplied turbo200.com: its own banner identifies DOS 2.00B.
; SHA-256: fad63d86ab63aa3267667167b59bb50b7f7381da249794025c2d5d06cc551473
; User-supplied CDOS image retains 00 00 at 02FCh (ADD [BX+SI],AL).
; Original length 36480. COM addresses below map to file offset address-100h.
; Preserve runtime [100h,2837h), configuration, loader and compiler templates.
bits 16
cpu 8086
org 100h

%macro original_to 1
    incbin INPUT, ($-$$), (%1)-100h-($-$$)
%endmacro

original_to 28e4h
    jmp cli_start
original_to 29feh
    jmp cli_failure
original_to 2d0eh
    jmp cli_failure
original_to 356fh
    jmp cli_failure
original_to 39a9h

; Startup has initialized DS/SS, source buffers, DOS version and current drive.
; Menu E dispatches to 39A9h. Its callbacks and re-entry are editor-only.
; The compiler begins at 5122h; no generated runtime bytes are replaced.
cli_start:
    cld
    mov word [0a9h], cli_putchar
    mov byte [23eh], 2
    mov byte [cs:source_seen], 0
    mov byte [cs:mode_seen], 0
    xor bx, bx
    mov bl, [cs:80h]
    cmp bl, 126
    jae cli_usage
    mov byte [cs:bx+81h], 0
    mov si, 81h
.next:
    call next_token
    jc .compile
    cmp byte [cs:bx], '-'
    je .option
    cmp byte [cs:source_seen], 0
    jne cli_usage
    mov byte [cs:source_seen], 1
    xor di, di
    xor cx, cx
.source:
    mov al, [cs:bx]
    inc bx
    or al, al
    jz .source_end
    cmp al, '/'
    je cli_usage
    cmp al, '\'
    je cli_usage
    cmp al, '*'
    je cli_usage
    cmp al, '?'
    je cli_usage
    cmp al, 'a'
    jb .copy
    cmp al, 'z'
    ja .copy
    sub al, 20h
.copy:
    cmp cx, 59
    jae cli_usage
    mov [di], al
    inc di
    inc cx
    jmp .source
.source_end:
    mov byte [di], 1ah
    ; This release's loader uses FCBs: reject paths or truncated 8.3 names.
    xor bx, bx
    cmp byte [1], ':'
    jne .basename
    cmp byte [0], 'A'
    jb cli_usage
    cmp byte [0], 'O'
    ja cli_usage
    mov bx, 2
.basename:
    xor dx, dx
.validate:
    mov al, [bx]
    inc bx
    cmp al, 1ah
    je .validated
    cmp al, '.'
    je .extension
    cmp al, 21h
    jb cli_usage
    cmp al, ':'
    je cli_usage
    cmp al, ','
    je cli_usage
    cmp al, ';'
    je cli_usage
    cmp al, '='
    je cli_usage
    cmp al, '['
    je cli_usage
    cmp al, ']'
    je cli_usage
    cmp al, '<'
    je cli_usage
    cmp al, '>'
    je cli_usage
    inc dl
    cmp dh, 0
    jne .ext_length
    cmp dl, 8
    ja cli_usage
    jmp .validate
.ext_length:
    cmp dl, 3
    ja cli_usage
    jmp .validate
.extension:
    or dh, dh
    jnz cli_usage
    or dl, dl
    jz cli_usage
    inc dh
    xor dl, dl
    jmp .validate
.validated:
    or dl, dl
    jz cli_usage
    push si
    xor bx, bx
    call 308dh                 ; Parse DS:BX, default PAS, result FCB DS:80h.
    mov si, 80h
    mov di, 1d1h
    call 30b4h                 ; Copy 12-byte FCB; clear remaining FCB state.
    pop si
    jmp .next
.option:
    cmp byte [cs:bx+1], 'h'
    jne .switch
    cmp byte [cs:bx+2], 0
    je cli_help
.switch:
    cmp byte [cs:bx+1], 0
    je cli_usage
    cmp byte [cs:bx+2], 0
    je cli_usage
    cmp byte [cs:bx+3], 0
    jne cli_usage
    mov ax, [cs:bx+1]
    cmp ax, 'cc'
    je .com
    cmp ax, 'ch'
    je .chain
    mov di, 19eh
    cmp ax, 'sc'
    je .number
    mov di, 1a0h
    cmp ax, 'sd'
    je .number
    mov di, 1a2h
    cmp ax, 'mi'
    je .number
    mov di, 1a4h
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
    cmp di, 19eh
    jne .data
    cmp dx, 0d7dh
    ja cli_usage
    or dx, dx
    jz .store
    add dx, 283h               ; Native option handler: 2837h >> 4.
    jmp .store
.data:
    cmp di, 1a0h
    jne .store
    cmp dx, 0fe7h
    ja cli_usage
    or dx, dx
    jz .store
    add dx, 19h                ; Native option handler: 019Ah >> 4.
.store:
    mov [di], dx
    jmp .next
.com:
    mov al, 2
    jmp .mode
.chain:
    mov al, 3
.mode:
    cmp byte [cs:mode_seen], 0
    jne cli_usage
    mov byte [cs:mode_seen], 1
    mov [23eh], al
    jmp .next
.compile:
    cmp byte [cs:source_seen], 0
    je cli_usage
    mov ax, [1a2h]
    cmp ax, [1a4h]
    ja cli_usage
    call 2dfdh                 ; Original workfile loader, including I/O exits.
    call 33c6h                 ; Original COM/CHN compile/write handler.
    xor ax, ax
    jmp cli_exit

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
    mov dx, compilation_failed
    call print
    jmp cli_exit
cli_usage:
    mov dx, bad_arguments
    call print
    mov dx, usage
    call print
    mov ax, 2
    jmp cli_exit
cli_help:
    mov dx, usage
    call print
    xor ax, ax
cli_exit:
    ; CS remains the COM PSP segment. DOS 1 termination restores its saved
    ; vectors, including Turbo's INT 23h, without requiring DOS 2 AH=4Ch.
    int 20h

print:
    push ds
    push cs
    pop ds
    mov ah, 9
    int 21h
    pop ds
    ret

; Native Pascal character callback, replacing IDE screen output only.
cli_putchar:
    push bp
    mov bp, sp
    push ax
    push dx
    mov dl, [bp+4]
    mov ah, 2
    int 21h
    pop dx
    pop ax
    pop bp
    ret 2

source_seen: db 0
mode_seen: db 0
bad_arguments: db 'TPC: invalid arguments', 13, 10, '$'
compilation_failed: db 13, 10, 'TPC: compilation failed', 13, 10, '$'
usage:
    db 'Turbo Pascal 2.00B for DOS compiler', 13, 10
    db 'Usage: TPC [-cc|-ch] [-sc HEX] [-sd HEX] [-mi HEX] [-ma HEX] file[.pas]', 13, 10
    db '  -cc COM (default), -ch CHN; -sc/-sd minimum code/data paragraphs', 13, 10
    db '  -mi/-ma minimum/maximum free memory paragraphs; HEX = 1-4 hex digits', 13, 10
    db '  -h help; errors print TPC: invalid arguments or TPC: compilation failed', 13, 10, '$'

original_to 3e00h
original_to 8f80h
