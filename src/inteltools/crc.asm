
;    -------------------------PROGRAM IDENTIFIER---------------------------
;
;    Project : \SCA\8086
;
;    Program : CRC.ASM
;
;    Purpose : CRC Emulation
;
;    Author  : Mr S.C.Agate
;
;
;    Date        Modification Details
;    ----        --------------------
;
;    27-Feb-90 : Creation. This code is suitable for ROM.
;
;    ----------------------------------------------------------------------


; DOS Calls used (Service number passed in AH)
; ==============
;
;
; Output Character String (Interrupt 21H, Service 09H)
; -----------------------
;
; Upon Entry - DX : Offset address of string
;              DS : Segment address
;
; Terminate string with "$". Registers returned unchanged.
;
;
; Print a Character (Interrupt 10H, Service 0EH)
; -----------------
;
; Upon Entry - AL : Contains ASCII Code to print.
;
;
; Get Key (Interrupt 16H, Service 00H)
; -------
;
; Upon Exit - AL : ASCII Character (Get next Char. Wait if required)
;
;
; Get Key Status (Interrupt 16H, Service 01H)
; --------------
;
; Upon Exit - Z Flag cleared if keypressed (Use above routine to get char)

print_string    EQU     09H                     ;Set macro definitions
print_chr       EQU     0EH
get_key         EQU     00H
get_key_status  EQU     01H

cr              EQU     0DH
lf              EQU     0AH
del             EQU     08H
null            EQU     00H
ascii_conv      EQU     "A"-"9"-1

poly            EQU     8408H

crc_segment     SEGMENT PUBLIC

                ORG     100H

                JMP     start

sign_on_mesg    DB      cr,lf
                DB      "ษอออออออออออออออออออออออออออป",cr,lf
                DB      "บ  CRC Emulation Program    บ",cr,lf
                DB      "บ  Version 1.0              บ",cr,lf
                DB      "บ  (C) S.C.Agate, Feb 1990  บ",cr,lf
                DB      "ศอออออออออออออออออออออออออออผ$"

mesg0           DB      del," ",del,"$"

mesg1           DB      cr,lf,lf,"CRC Register : $"

mesg2           DB      cr,lf,lf,"Options - (D)ata, (Z)ero, "
                DB      "(S)et, (R)everse or (E)xit ? $"

mesg3           DB      "  Input Data : $"

mesg4           DB      cr,lf,lf,"To set the CRC register, send $"

table1          DB      "DZSRE",null

table2          DW      option_D
                DW      option_Z
                DW      option_S
                DW      option_R
                DW      option_E

sign_off_mesg   DB      cr,lf,lf,"Goodbye!",cr,lf,"$"

;============================== Main Program ===============================

start:          PUSH    DS                      ;Return to DS:0 which
                XOR     AX,AX                   ;points to an INT 20H
                PUSH    AX                      ;(Terminate Program)

                MOV     AX,CS                   ;Set DS to CS
                MOV     DS,AX

                PUSH    BP                      ;Create Stack Frame
                MOV     BP,SP                   ;[BP] = Last Item Stacked
                SUB     SP,0AH

crc             EQU     WORD PTR [BP-02H]
value           EQU     WORD PTR [BP-04H]
input_string    EQU     BYTE PTR [BP-0AH]       ;Room for 6 characters
max_input       EQU     6

                MOV     DX,OFFSET sign_on_mesg  ;Print Sign-On Message
                MOV     AH,print_string
                INT     21H

option_Z:       MOV     crc,0                   ;Clear CRC Register

next_command:   MOV     DX,OFFSET mesg1         ;Print CRC Register
                MOV     AH,print_string
                INT     21H
                MOV     AX,crc
                CALL    print_hex

                MOV     DX,OFFSET mesg2         ;Print Options
                MOV     AH,print_string
                INT     21H

not_valid_key:  CALL    inkey                   ;Get the next key pressed

                MOV     BX,-1                   ;Search for Valid Key
                MOV     SI,OFFSET table1
try_next_key:   INC     BX
                CMP     BYTE PTR [SI+BX],null
                JZ      not_valid_key
                CMP     [SI+BX],AL
                JNZ     try_next_key

                MOV     AH,print_chr            ;Print Key
                INT     10H

                SAL     BX,1                    ;Jump to process key
                MOV     SI,OFFSET table2
                JMP     [SI+BX]

;---------------------------------------------------------------------------
;                             CRC CALCULATIONS
;--------------------------------------------------------------------------

option_D:       CALL    get_data
                CALL    perf_crc
                JMP     next_command

option_S:       CALL    get_data
                CALL    calc_inv
                MOV     DX,OFFSET mesg4         ;Print Result
                MOV     AH,print_string
                INT     21H
                MOV     AX,value
                CALL    print_hex
                JMP     next_command

option_R:       CALL    get_data
                CALL    calc_rev
                JMP     next_command

;---------------------------------------------------------------------------
;                              (E)XIT PROGRAM
;--------------------------------------------------------------------------

option_E:       MOV     DX,OFFSET sign_off_mesg ;Print Sign-Off Message
                MOV     AH,print_string
                INT     21H

                MOV     SP,BP                   ;Delete Stack Frame
                POP     BP

                RETF                            ;Terminate

;============================== Subroutines ================================

;---------------------------------------------------------------------------
;                            GET AN ASCII CHARACTER
;---------------------------------------------------------------------------

inkey:          MOV     AH,get_key              ;Return ASCII Chr in AL
                INT     16H
                CALL    l_to_u_case
                RET

;---------------------------------------------------------------------------
;                               GET INPUT DATA
;--------------------------------------------------------------------------

get_data:       MOV     DX,OFFSET mesg3         ;Print Prompt
                MOV     AH,print_string
                INT     21H

                CALL    get_string
                CALL    conv_to_value
                RET

;---------------------------------------------------------------------------
;                                GET STRING
;---------------------------------------------------------------------------

get_string:     MOV     SI,0H                   ;Fill input_string with ASCIIZ

need_input:     CALL    inkey                   ;Get key

                CMP     AL,cr                   ;Test for control keys
                JZ      entered
                CMP     AL,del
                JNZ     no_delete
                CMP     SI,0H
                JZ      need_input

                MOV     DX,OFFSET mesg0         ;Perform Delete
                MOV     AH,print_string
                INT     21H
                DEC     SI
                JMP     need_input

no_delete:      CMP     AL,"0"                  ;Test Alphanumeric Validity
                JL      need_input
                CMP     AL,"9"
                JLE     numeric
                CMP     AL,"F"
                JG      need_input
                CMP     AL,"A"
                JL      need_input

numeric:        MOV     input_string[SI],AL     ;Save & Print Alphanumeric key
                MOV     AH,print_chr
                INT     10H

                INC     SI                      ;Compare to limit
                CMP     SI,max_input
                JNZ     need_input

entered:        CMP     SI,0H                   ;Check for null string
                JNZ     some_input
                MOV     input_string,"0"
                MOV     SI,1H

some_input:     MOV     input_string[SI],null   ;Place End Marker
                RET

;---------------------------------------------------------------------------
;                      LOWER TO UPPER CASE CONVERSION
;---------------------------------------------------------------------------

l_to_u_case:    CMP     AL,"a"                  ;ASCII in AL --> Upper Case
                JL      no_conv_reqd
                CMP     AL,"z"
                JG      no_conv_reqd
                SUB     AL,"a"-"A"
no_conv_reqd:   RET

;---------------------------------------------------------------------------
;                      STRING TO NUMBER CONVERSION
;---------------------------------------------------------------------------

conv_to_value:  MOV     SI,0                    ;Define init values
                MOV     BX,0
                MOV     CX,4

more_to_do:     CMP     input_string[SI],null   ;Check for end of input
                JZ      end_mark_found

                SHL     BX,CL                   ;Make room for next nibble

                MOV     AL,input_string[SI]     ;Convert from ASCII
                SUB     AL,"0"
                CMP     AL,10
                JL      number
                SUB     AL,ascii_conv

number:         ADD     BL,AL                   ;Store this nibble
                INC     SI
                JMP     more_to_do

end_mark_found: MOV     value,BX                ;Store entire result
                RET

;---------------------------------------------------------------------------
;                    NON-DESTRUCTIVE HEX PRINT ROUTINES
;---------------------------------------------------------------------------

print_hex:      PUSH    AX                      ;AX=Number to print
                PUSH    BX
                PUSH    CX
                MOV     CL,12                   ;Print 16 bits
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
digit:          ADD     AL,"0"
                MOV     AH,print_chr
                INT     10H
                RET

;---------------------------------------------------------------------------
;                               PERFORM CRC
;---------------------------------------------------------------------------

perf_crc:       MOV     AX,value
                XOR     crc,AX
                MOV     CX,16
perf_crc_1:     SHR     crc,1
                JNC     perf_crc_2
                XOR     crc,poly
perf_crc_2:     LOOP    perf_crc_1
                RET

;---------------------------------------------------------------------------
;                               CALCULATE INVERSE
;---------------------------------------------------------------------------

calc_inv:       MOV     CX,16
calc_inv_1:     MOV     AX,value
                ROL     AX,1
                AND     AX,1
                JZ      calc_inv_2
                XOR     value,poly
calc_inv_2:     SHL     value,1
                ADD     value,AX
                LOOP    calc_inv_1
                MOV     AX,crc
                XOR     value,AX
                RET

;---------------------------------------------------------------------------
;                               REVERSE CRC
;---------------------------------------------------------------------------

calc_rev:       MOV     CX,16
calc_rev_1:     MOV     AX,crc
                ROL     AX,1
                AND     AX,1
                JZ      calc_rev_2
                XOR     crc,poly
calc_rev_2:     SHL     crc,1
                ADD     crc,AX
                LOOP    calc_rev_1
                MOV     AX,value
                XOR     crc,AX
                RET

crc_segment     ENDS

                END

