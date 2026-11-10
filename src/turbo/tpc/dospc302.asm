; In-place CLI for the supplied local Turbo Pascal 3.02A DOS image.
; Addresses are COM load addresses, not file offsets. The runtime below
; 2D4Dh and the TINST configuration at the start of the image stay intact.
bits 16
cpu 8086
org 100h

; Copy untouched bytes up to the next load address.
%macro original_to 1
    incbin INPUT, ($-$$), (%1)-100h-($-$$)
%endmacro

original_to 2e29h
    jmp cli_start
original_to 2f64h
    jmp cli_failure
original_to 3478h
    jmp cli_failure
original_to 3a96h
    jmp cli_failure
original_to 40cdh

; Enter after Turbo has established DS, SS, source storage and DOS state.
cli_start:
    cld
    mov word [13ch], cli_putchar
    mov byte [34bh], 2
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
    mov di, 38h
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
    mov bx, 38h
    call 3ee4h                 ; Canonical filename, default .PAS.
    mov di, 2c9h
    call 3f3ah
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
    mov di, 270h
    cmp ax, 'sc'
    je .number
    mov di, 272h
    cmp ax, 'sd'
    je .number
    mov di, 274h
    cmp ax, 'mi'
    je .number
    mov di, 276h
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
    cmp di, 270h
    jne .data
    cmp dx, 0d2bh
    ja cli_usage
    or dx, dx
    jz .store
    add dx, 2d5h              ; Turbo stores sizes including runtime.
    jmp .store
.data:
    cmp di, 272h
    jne .store
    cmp dx, 0fdbh
    ja cli_usage
    or dx, dx
    jz .store
    add dx, 25h
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
    mov [34bh], al
    jmp .next
.compile:
    cmp byte [cs:source_seen], 0
    je cli_usage
    mov ax, [274h]
    cmp ax, [276h]
    ja cli_usage
    call 33d9h                 ; Load source using Turbo's DOS file loader.
    call 3912h                 ; Compile and write COM/CHN; never run it.
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

; DOS termination restores Turbo's saved vectors through its own runtime.
cli_failure:
    mov ax, 1
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
    jmp 0c40h

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

source_seen: db 0
mode_seen: db 0
bad_arguments: db 'TPC: invalid arguments', 13, 10, '$'
usage:
    db 'Turbo Pascal 3.02A for DOS compiler', 13, 10
    db 'Usage: TPC [-cc|-ch] [-sc HEX] [-sd HEX] [-mi HEX] [-ma HEX] file[.pas]', 13, 10
    db '  -cc COM (default), -ch CHN; -sc/-sd minimum code/data paragraphs', 13, 10
    db '  -mi/-ma minimum/maximum free memory paragraphs; HEX = 1-4 hex digits', 13, 10
    db '  -h help; exit codes: 0 success, 1 compile/I/O error, 2 usage error', 13, 10, '$'

original_to 5000h
original_to 9c33h
