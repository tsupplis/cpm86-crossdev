; In-place CLI for turms301.com, identifying itself as 3.01A for MS-DOS.
; COM addresses are file offsets plus 100h. Keep the runtime and TINST data.
bits 16
cpu 8086
org 100h

; Copy untouched bytes up to the next load address.
%macro original_to 1
    incbin INPUT, ($-$$), (%1)-100h-($-$$)
%endmacro

original_to 2ae9h
    times 6 nop                ; Do not initialize/finalize the terminal.
original_to 2b30h
    jmp cli_start
original_to 2c70h
    jmp cli_failure
original_to 3163h
    jmp cli_failure
original_to 3781h
    jmp cli_failure
original_to 3db8h

; Enter after Turbo has established DS, SS, source storage and DOS state.
cli_start:
    cld
    mov word [114h], cli_keypoll
    mov word [118h], cli_putchar
    mov byte [327h], 2
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
    mov di, 14h
    xor cx, cx
.source:
    mov al, [cs:bx]
    inc bx
    or al, al
    jz .source_end
    cmp al, '/'
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
    mov byte [di], 0
    push si
    mov bx, 14h
    call 3bcfh                 ; Canonical filename, default .PAS.
    mov di, 2a5h
    call 3c25h
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
    mov di, 24ch
    cmp ax, 'sc'
    je .number
    mov di, 24eh
    cmp ax, 'sd'
    je .number
    mov di, 250h
    cmp ax, 'mi'
    je .number
    mov di, 252h
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
    cmp di, 24ch
    jne .data
    cmp dx, 0d5ah
    ja cli_usage
    or dx, dx
    jz .store
    add dx, 2a6h              ; Turbo stores sizes including runtime.
    jmp .store
.data:
    cmp di, 24eh
    jne .store
    cmp dx, 0fdeh
    ja cli_usage
    or dx, dx
    jz .store
    add dx, 22h
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
    mov [327h], al
    jmp .next
.compile:
    cmp byte [cs:source_seen], 0
    je cli_usage
    mov ax, [250h]
    cmp ax, [252h]
    ja cli_usage
    call 30c4h                 ; Load source using Turbo's DOS file loader.
    call 35fdh                 ; Compile and write COM/CHN; never run it.
    xor ax, ax
    jmp cli_exit

; Tokenize the bounded PSP tail in place. Return CS:BX, carry at end.
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
    mov di, 138h               ; Close both Turbo runtime file lists.
    call 212bh
    mov di, 144h
    call 212bh
    xor ax, ax                 ; Restore Turbo's saved divide-error vector.
    mov es, ax
    mov ax, [16ch]
    mov [es:0], ax
    mov ax, [16eh]
    mov [es:2], ax
    int 20h                    ; CS is the PSP; DOS restores its saved vectors.

; Print our CS-resident text without changing the compiler's data segment.
print:
    push ds
    push cs
    pop ds
    mov ah, 9
    int 21h
    pop ds
    ret

; The compiler uses a Pascal character callback; use redirected DOS stdout,
; not the IDE's screen/editor callback. Generated runtime bytes are untouched.
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

; A redirected DOS input handle can report EOF as a pending terminal key.
cli_keypoll:
    xor al, al
    ret 1                      ; Consume the caller's reserved result byte.

source_seen: db 0
mode_seen: db 0
bad_arguments: db 'TPC: invalid arguments', 13, 10, '$'
compilation_failed: db 13, 10, 'TPC: compilation failed', 13, 10, '$'
usage:
    db 'Turbo Pascal 3.01A for MS-DOS compiler', 13, 10
    db 'Usage: TPC [-cc|-ch] [-sc HEX] [-sd HEX] [-mi HEX] [-ma HEX] file[.pas]', 13, 10
    db '  -cc COM (default), -ch CHN; -sc/-sd minimum code/data paragraphs', 13, 10
    db '  -mi/-ma minimum/maximum free memory paragraphs; HEX = 1-4 hex digits', 13, 10
    db '  -h help; errors print TPC: invalid arguments or TPC: compilation failed', 13, 10, '$'

original_to 4cebh
original_to 9600h
