
;    -------------------------PROGRAM IDENTIFIER---------------------------
;
;    Project : PLM86
;
;    Program : INIT.ASM
;
;    Purpose : PLM86 Test Harness
;
;    Author  : Mr S.C.Agate
;
;
;    Date        Modification Details
;    ----        --------------------
;
;    14-Aug-90 : Creation
;
;    ----------------------------------------------------------------------

                NAME init

cr              EQU     0DH
lf              EQU     0AH
p_str           EQU     09H
print_chr       EQU     0EH
ascii_conv      EQU     'A'-'9'-1

                PUBLIC  param_count
                PUBLIC  param_list

                EXTRN main:NEAR

code            SEGMENT PUBLIC 'CODE'
                ASSUME CS:code, DS:data, SS:stack

power_up        PROC NEAR

begin:          MOV     AX,data
                MOV     DS,AX
                MOV     AX,stack
                MOV     SS,AX

                MOV     DX,OFFSET hello         ;Print Sign-on Message
                MOV     AH,p_str
                INT     21H

                CALL    transfer                ;Copy PSP 80H onwards

                CALL    main                    ;Enter PLM86 code
                MOV     exit_code,AL

                MOV     DX,OFFSET goodbye       ;Print Sign-off Message
                MOV     AH,p_str
                INT     21H
                MOV     AL,exit_code
                CALL    print_hex
                MOV     AL,'H'
                MOV     AH,print_chr
                INT     10H

                MOV     AH,4CH                  ;Terminate Program
                MOV     AL,exit_code
                INT     21H

power_up        ENDP

print_hex       PROC NEAR

                PUSH    AX                      ;AX=Number to print
                PUSH    BX
                PUSH    CX
                MOV     CL,12                   ;Print 16 bits
                MOV     CL,04                   ;Print 8 bits
                MOV     BX,AX
nibble_out:     MOV     AX,BX
                SHR     AX,CL
                CALL    print_nibble
                SUB     CL,4
                CMP     CL,-4
                JNZ     nibble_out
                POP     CX
                POP     BX
                POP     AX
                RET

print_nibble:   AND     AL,0FH                  ;Low nibble of AL is printed
                CMP     AL,9
                JLE     digit
                ADD     AL,ascii_conv
digit:          ADD     AL,'0'
                MOV     AH,print_chr
                INT     10H
                RET

print_hex       ENDP

transfer        PROC NEAR

                MOV     SI,80H                  ;Offset into PSP
                MOV     DI,OFFSET table_start   ;Offset into DS copy of PSP
                MOV     BX,OFFSET param_list    ;Offset into pointer table
                MOV     CL,ES:[SI]              ;Number of chars to read
                MOV     CH,1                    ;Set "Last chr a space" flag
                INC     SI                      ;Point to first char

get_next_chr:   CMP     CL,0H                   ;Any more chars?
                JZ      no_more
                MOV     AL,ES:[SI]              ;Get next char
                INC     SI
                DEC     CL
                CMP     AL,20H                  ;See if its a space
                JZ      its_a_space             ;Jump if present char a space

                CMP     CH,1                    ;Was last char a space?
                JNZ     in_param
                MOV     [BX],DI                 ;Store pointer
                ADD     BX,2
in_param:       MOV     [DI],AL                 ;Copy char to DS
                INC     DI
                MOV     CH,0
                JMP     get_next_chr

its_a_space:    CMP     CH,1                    ;Was last char a space?
                JZ      between_params
                MOV     BYTE PTR [DI],0         ;No, so put in EOL marker
                INC     DI
                INC     param_count
                CMP     param_count,10          ;Check for max no of params
                JZ      no_0_needed
between_params: MOV     CH,1H
                JMP     get_next_chr

no_more:        CMP     CH,1H
                JZ      no_0_needed
                MOV     BYTE PTR [DI],0
                INC     DI
                INC     param_count
no_0_needed:    RET

transfer        ENDP

code            ENDS

stack           SEGMENT PUBLIC 'STACK'
                DB      100H DUP(0)
stack           ENDS

data            SEGMENT PUBLIC 'DATA'

hello           DB      '======= PLM86 Test Harness =======',cr,lf
                DB      'Version 1.0   (C) S.Agate   Aug 90',cr,lf
                DB      '----------------------------------',cr,lf,'$'

goodbye         DB      cr,lf
                DB      '==================================',cr,lf
                DB      'Exit code = $'

exit_code       DB      ?

param_count     DB      0
param_list      DW      10 DUP(0)
table_start     DB      80H DUP(0)

data            ENDS

END             CS:begin, DS:data, SS:stack
