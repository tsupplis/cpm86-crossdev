; Editor-only entry for the ORIGINAL Turbo Pascal 3.01A CP/M-86 image.
; Never use c86pc301.cmd: that compiler patch replaces the editor.
bits 16
cpu 8086
org -80h

%macro original_to 1
    incbin INPUT, ($-$$), (%1)+80h-($-$$)
%endmacro

; Retain terminal initialization, runtime, configuration and CMD descriptors.
original_to 285bh
    jmp editor_start
original_to 299bh
    jmp editor_done

; The payload occupies only the former IDE menu/display/option handlers.
; Preserve filename prompts at 29CEh, confirmation at 2C89h, all file I/O,
; editor initialization/exit at 38F2h/3964h, and editor/key tables to 5189h.
original_to 29fbh
editor_start:
    cld
    push ds
    pop es
    xor cx, cx
    mov cl, [80h]
    cmp cl, 126
    jae usage_error
    mov si, 81h
.leading:
    or cx, cx
    jz usage_error
    lodsb
    dec cx
    cmp al, ' '
    je .leading
    cmp al, 9
    je .leading
    mov di, 100h
    xor bx, bx
.name:
    cmp al, ' '
    je .trailing
    cmp al, 9
    je .trailing
    cmp bx, 14
    jae usage_error
    cmp al, 'a'
    jb .copy
    cmp al, 'z'
    ja .copy
    sub al, 20h
.copy:
    stosb
    inc bx
    jcxz .parsed
    lodsb
    dec cx
    jmp .name
.trailing:
    jcxz .parsed
    lodsb
    dec cx
    cmp al, ' '
    je .trailing
    cmp al, 9
    je .trailing
    jmp usage_error
.parsed:
    mov byte [di], 0
    call validate_name
    mov bx, 100h
    mov cl, ' '
    mov dx, 2020h
    call 3019h                 ; Native FCB parser with a blank default extension.
    mov si, 200h
    mov di, 311h
    call 303bh                 ; Native workfile identity/loaded flags.
    mov word [38ah], 2e71h     ; Native "New File" continuation, not ESC wait.
    call 2da0h                 ; Load or initialize a named empty workfile.
    mov byte [cs:entered], 1
    jmp 398ah                 ; Entire original editor, unchanged.

; One optionally drive-qualified 8.3 name; no wildcard/truncation/path syntax.
validate_name:
    mov bx, 100h
    cmp byte [bx+1], ':'
    jne .base
    cmp byte [bx], 'A'
    jb usage_error
    cmp byte [bx], 'P'
    ja usage_error
    add bx, 2
.base:
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
    jb .digit
    cmp al, 'Z'
    jbe .valid
.digit:
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
    jne usage_error
.valid:
    inc cl
    cmp cl, dl
    ja usage_error
    jmp .char
.extension:
    cmp dl, 8
    jne usage_error
    or cl, cl
    jz usage_error
    mov dl, 3
    xor cx, cx
    jmp .char
.end:
    or cl, cl
    jz usage_error
    ret

editor_done:
    mov sp, [2ceh]            ; Reset the native menu/error-unwind stack.
    cmp byte [cs:entered], 0
    je editor_exit
    cmp byte [cs:save_failed], 0
    je .save
    mov byte [cs:save_failed], 0
    mov ax, [4f6h]
    mov [228h], ax
    jmp 398ah                 ; Native cleanup restored the dirty workfile.
.save:
    ; The original Ctrl-K,D handler restores the console callback and EOF.
    ; Reuse the native quit-time dirty-buffer Save (Y/N)? dialog.
    call 2eabh
    jmp editor_exit
usage_error:
    mov dx, bad_arguments
    call print
    mov dx, usage
    call print
    jmp editor_exit
editor_exit:
    call 18ah                  ; Original terminal reset.
    xor dx, dx
    xor cx, cx                 ; Native BDOS P_TERM, no resident process.
    int 0e0h

file_error:
    cmp byte [38ch], 0
    je .prompt
    mov byte [cs:save_failed], 1
.prompt:
    jmp 5a2h                  ; Retain the native ESC wait and error recovery.

print:
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
    ret

bad_arguments: db 'TPE: invalid arguments', 13, 10, 0
usage: db 'Usage: TPE [A:-P:]filename', 13, 10, 0
entered: db 0
save_failed: db 0

original_to 2c89h
; Mark errors without bypassing native close/delete/backup recovery.
original_to 2cb4h
    call file_error
original_to 2cb7h
incbin INPUT, ($-$$)
