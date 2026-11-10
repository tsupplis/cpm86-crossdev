; Editor-only entry for the supplied DOS 2.00B CDOS image, not dospc200.com.
; SHA-256: fad63d86ab63aa3267667167b59bb50b7f7381da249794025c2d5d06cc551473
; COM addresses are file offsets + 100h. All runtime/configuration bytes,
; including the supplied 00 00 at 02FCh, are copied without modification.
; Allowed replacement windows (half-open COM addresses):
; [28E4h,29FEh), [29FEh,2A01h), [2A4Dh,2B1Fh),
; [2D0Eh,2D11h), [2EDCh,2EDFh).
bits 16
cpu 8086
org 100h

%macro original_to 1
    incbin INPUT, ($-$$), (%1)-100h-($-$$)
%endmacro

original_to 28e4h
    jmp editor_start

; Reuse only the discarded IDE startup banner, not editor/compiler storage.
parse_filename:
    cld
    xor bx, bx
    mov bl, [cs:80h]
    cmp bl, 126
    jae bad_arguments
    mov byte [cs:bx+81h], 0
    mov si, 81h
    call skip_spaces
    or al, al
    jz bad_arguments
    cmp al, '-'
    je bad_arguments
    xor di, di
.copy:
    mov al, [cs:si]
    inc si
    or al, al
    jz .end
    cmp al, ' '
    je .end
    cmp al, 9
    je .end
    cmp di, 14
    jae bad_arguments
    call 0af7h                ; Original ASCII uppercase conversion.
    mov [di], al
    inc di
    jmp .copy
.end:
    mov byte [di], 1ah
    dec si
    call skip_spaces
    or al, al
    jnz bad_arguments
    xor bx, bx
    cmp byte [1], ':'
    jne .basename
    cmp byte [0], 'A'
    jb bad_arguments
    cmp byte [0], 'O'
    ja bad_arguments
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
    jb bad_arguments
    cmp al, 7eh
    ja bad_arguments
    ; Reject DOS separators, paths, wildcards, quoting and redirection.
    push di
    mov di, forbidden
.forbidden:
    cmp al, [cs:di]
    je bad_arguments
    inc di
    cmp byte [cs:di], 0
    jne .forbidden
    pop di
    inc dl
    cmp dh, 0
    jne .ext_length
    cmp dl, 8
    ja bad_arguments
    jmp .validate
.ext_length:
    cmp dl, 3
    ja bad_arguments
    jmp .validate
.extension:
    or dh, dh
    jnz bad_arguments
    or dl, dl
    jz bad_arguments
    inc dh
    xor dl, dl
    jmp .validate
.validated:
    or dl, dl
    jz bad_arguments
    xor bx, bx
    mov cl, ' '
    mov dx, 2020h
    call 3092h                ; Native FCB parser with a blank default extension.
    mov si, 80h
    mov di, 1d1h
    call 30b4h                ; Original workfile FCB/state initialization.
    ret

skip_spaces:
    mov al, [cs:si]
    cmp al, ' '
    je .skip
    cmp al, 9
    jne .done
.skip:
    inc si
    jmp skip_spaces
.done:
    ret

original_to 29feh
    jmp near editor_exit
original_to 2a4dh

; The old menu display is unreachable. Keep its shared input helpers
; [2A2Ch,2A4Dh) and numeric/status helpers at 2B1Fh and later intact.
editor_start:
    call parse_filename
    call 201h                 ; Original terminal clear/init after startup.
    call 2dfdh                ; Original loader closes its FCB before return.
    mov byte [cs:entered], 1
    jmp 39a9h                 ; Original editor, including original key tables.

editor_exit:
    mov sp, [190h]            ; Native error unwinding also arrives here.
    cmp byte [cs:entered], 0
    je terminate
    cmp byte [cs:save_failed], 0
    je .save
    mov byte [cs:save_failed], 0
    mov ax, [370h]
    mov [0a9h], ax
    jmp 39a9h                 ; Retain the dirty buffer after native recovery.
.save:
    ; 3991h restores the original output callback before reaching 29FEh.
    ; Ask the native save question on modified workfiles; native save closes
    ; the FCB, preserves BAK behavior, and recovers from disk/directory errors.
    call 2f1ch
    call 32dh                 ; Leave the cursor on the final screen row.
terminate:
    ; CS is still the COM PSP segment, even though DS/SS are Turbo's segments.
    ; DOS 1 INT 20h restores the PSP-saved vectors (including INT 23h).
    int 20h

file_error:
    cmp byte [245h], 0
    je .prompt
    mov byte [cs:save_failed], 1
.prompt:
    jmp 1b4h                  ; Consume the original inline ESC prompt.

bad_arguments:
    push cs
    pop ds
    mov dx, usage
    mov ah, 9
    int 21h
    int 20h

forbidden: db ':/\*?"+,;=[]<>|', 0
entered: db 0
save_failed: db 0
usage:
    db 'TPE: invalid arguments', 13, 10
    db 'Usage: TPE [A:-O:]filename', 13, 10, '$'

original_to 2b1fh
original_to 2d0eh
    call file_error
original_to 2edch
    ; The original loader's missing-file callback otherwise returns to the
    ; IDE. BX is the empty buffer start. Complete its normal CR/end setup,
    ; without creating a disk file or waiting for the New File delay.
    jmp 2e32h

; Preserve all workfile/save/find/block helpers and the entire native editor
; [391Fh,5122h), as well as the compiler and any helpers it might share.
original_to 8f80h
