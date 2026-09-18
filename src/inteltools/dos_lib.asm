
;    -------------------------PROGRAM IDENTIFIER---------------------------
;
;    Project : PLM86
;
;    Program : DOS_LIB.ASM
;
;    Purpose : DOS Interface Module
;
;    Author  : Mr S.C.Agate
;
;
;    Date        Modification Details
;    ----        --------------------
;
;    18-Sep-90 : Creation
;
;    ----------------------------------------------------------------------

date_and_time   STRUC
                year    DW ? ; 1980-2099
                month   DB ? ; 1-12
                day     DB ? ; 1-31
                dow     DB ? ; 0=Su, 1=Mo, etc...
                hour    DB ? ; 0-23
                minute  DB ? ; 0-59
                second  DB ? ; 0-59
                csecond DB ? ; 0-99
date_and_time   ENDS

                NAME dos_lib

gchr            EQU     00H
pchr            EQU     0EH
gtime           EQU     2CH
gdate           EQU     2AH

                PUBLIC  asm_get_chr
                PUBLIC  asm_put_chr
                PUBLIC  asm_get_date
                PUBLIC  asm_get_time

                PUBLIC  asm_openf
                PUBLIC  asm_rd_chrs
                PUBLIC  asm_wr_chrs
                PUBLIC  asm_set_loc
                PUBLIC  asm_closef

                PUBLIC  asm_dfile
                PUBLIC  asm_rfile
                PUBLIC  asm_cfile

                PUBLIC  asm_get_attrib  ;Untested
                PUBLIC  asm_set_attrib  ;Untested

                PUBLIC  asm_mkdir       ;Untested
                PUBLIC  asm_rmdir       ;Untested
                PUBLIC  asm_get_dir     ;Untested
                PUBLIC  asm_set_dir     ;Untested
                PUBLIC  asm_get_drive   ;Untested
                PUBLIC  asm_set_drive   ;Untested


code            SEGMENT BYTE PUBLIC 'code'
                ASSUME CS:code, SS:stack

asm_get_chr     PROC NEAR               ;Return keyboard char in AX
                MOV     AH,gchr
                INT     16H
                RET
asm_get_chr     ENDP

asm_put_chr     PROC NEAR               ;Print to screen param
                PUSH    BP
                MOV     BP,SP
                MOV     AL,BYTE PTR [BP+4]
                MOV     AH,pchr
                INT     10H
                POP     BP
                RET     2H
asm_put_chr     ENDP

asm_get_time    PROC NEAR               ;Fill struc with time
                PUSH    BP
                MOV     BP,SP
                MOV     AH,gtime
                INT     21H
                MOV     BP,WORD PTR [BP+4]
                MOV     DS:csecond[BP],DL
                MOV     DS:second[BP],DH
                MOV     DS:minute[BP],CL
                MOV     DS:hour[BP],CH
                POP     BP
                RET     2H
asm_get_time    ENDP

asm_get_date    PROC NEAR               ;Fill struc with date
                PUSH    BP
                MOV     BP,SP
                MOV     AH,gdate
                INT     21H
                MOV     BP,WORD PTR [BP+4]
                MOV     DS:dow[BP],AL
                MOV     DS:day[BP],DL
                MOV     DS:month[BP],DH
                MOV     DS:year[BP],CX
                POP     BP
                RET     2H
asm_get_date    ENDP

asm_openf       PROC NEAR
                ;Open file (handle)
                ;
                ;Enter: AH = 3DH
                ;       AL = Access code
                ;            00 - Read
                ;            01 - Write
                ;            02 - R&W
                ;    DS:DX = ASCIIZ pointer to filename
                ;
                ;Exit: C Clear - AX = File handle
                ;      C Set   - AX = Error code
                ;                     1 - invalid function
                ;                     2 - file not found
                ;                     3 - path not found
                ;                     4 - no handles available
                ;                     5 - access denied
                ;                     C - invalid access code
                ;
                ;Usage: Can open file in any directory or drive.
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,3DH
                MOV     AL,BYTE PTR [BP+6]  ;Access code
                MOV     DX,WORD PTR [BP+8]  ;Pointer to ASCIIZ
                INT     21H                 ;AX=Handle or Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_openf_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_openf_ok:   POP     BP
                RET     6H
asm_openf       ENDP

asm_rd_chrs     PROC NEAR
                ;Read from file (handle)
                ;File pointer is moved to next byte
                ;
                ;Enter: AH = 3FH
                ;       BX = File handle
                ;       CX = Number of bytes
                ;    DS:DX = Pointer to buffer
                ;
                ;Exit: C Clear - AX = Number of bytes read (past EOF?)
                ;      C Set   - AX = Error code
                ;                     0 - EOF
                ;                     5 - access denied
                ;                     6 - invalid handle
                PUSH    BP
                MOV     BP,SP
                MOV     AH,3FH
                MOV     BX,WORD PTR [BP+10] ;File handle
                MOV     CX,WORD PTR [BP+6]  ;Number of bytes
                MOV     DX,WORD PTR [BP+8]  ;Pointer
                INT     21H                 ;AX=Length or Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_rd_chrs_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_rd_chrs_ok: POP     BP
                RET     8H
asm_rd_chrs     ENDP

asm_wr_chrs     PROC NEAR
                ;Write to file (handle)
                ;File pointer is moved to next byte
                ;
                ;Enter: AH = 40H
                ;       BX = File handle
                ;       CX = Number of bytes
                ;    DS:DX = Pointer to buffer
                ;
                ;Exit: C Clear - AX = Number of bytes written (disc full?)
                ;      C Set   - AX = Error code
                ;                     5 - access denied
                ;                     6 - invalid handle
                ;
                ;Usage: If CX is zero, file size is set to the current locn.
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,40H
                MOV     BX,WORD PTR [BP+10] ;File handle
                MOV     CX,WORD PTR [BP+6]  ;Number of bytes
                MOV     DX,WORD PTR [BP+8]  ;Pointer
                INT     21H                 ;AX=Length or Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_wr_chrs_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_wr_chrs_ok: POP     BP
                RET     8H
asm_wr_chrs     ENDP

asm_set_loc     PROC NEAR
                ;Set pointer into file (handle)
                ;
                ;Enter: AH = 42H
                ;       AL = Access mode
                ;            00 - Offset from Beginning
                ;            01 - Offset from Current Loc
                ;            02 - Offset from End
                ;       BX = Handle
                ;    CX:DX = Offset
                ;
                ;Exit: C Clear - DX:AX = New location in file
                ;      C Set   - AX = Error code
                ;                     1 - invalid function
                ;                     6 - invalid handle
                ;
                ;Usage: Set Offset=0 and CX:DX to zero to return LOF in bytes.
                ;       Next write will then be to past existing EOF
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,42H
                MOV     AL,BYTE PTR [BP+10] ;File access mode
                MOV     BX,WORD PTR [BP+12] ;Handle
                MOV     CX,WORD PTR [BP+8]  ;Offset High
                MOV     DX,WORD PTR [BP+6]  ;Offset Low
                INT     21H                 ;AX=Length or Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_set_loc_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_set_loc_ok: POP     BP
                RET     0AH
asm_set_loc     ENDP

asm_closef      PROC NEAR
                ;Close file (handle)
                ;
                ;Enter: AH = 3EH
                ;       BX = File handle
                ;
                ;Exit: C Clear - File closed OK
                ;      C Set   - AX = Error code
                ;                     6 - invalid handle
                PUSH    BP
                MOV     BP,SP
                MOV     AH,3EH
                MOV     BX,WORD PTR [BP+6]  ;Handle
                INT     21H                 ;AX=Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_closef_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_closef_ok:  POP     BP
                RET     4H
asm_closef      ENDP

asm_dfile       PROC NEAR
                ;Delete file
                ;
                ;Enter: AH = 41H
                ;    DS:DX = Pointer to ASCIIZ
                ;
                ;Exit: C Clear - File deleted OK
                ;      C Set   - AX = Error code
                ;                     2 - File not found
                ;                     5 - Access denied
                ;
                ;Usage: No wild cards, but can access other dirs and drives.
                ;       Access denied if R/O file.
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,41H
                MOV     DX,WORD PTR [BP+6]  ;Pointer to ASCIIZ
                INT     21H                 ;AX=Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_dfile_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_dfile_ok:   POP     BP
                RET     4H
asm_dfile       ENDP

asm_rfile       PROC NEAR
                ;Rename file
                ;
                ;Enter: AH = 56H
                ;    DS:DX = Pointer to present ASCIIZ filename
                ;    ES:DI = Pointer to new     ASCIIZ filename
                ;
                ;Exit: C Clear - File renamed OK
                ;      C Set   - AX = Error code
                ;                     02H - file not found
                ;                     03H - path not found
                ;                     05H - access denied
                ;                     11H - not same device
                ;
                ;Usage: No wild cards allowed.
                ;       Can move file between directories, but not drives.
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AX,DS
                MOV     ES,AX
                MOV     AH,56H
                MOV     DX,WORD PTR [BP+8]  ;Pointer to ASCIIZ old name
                MOV     DI,WORD PTR [BP+6]  ;Pointer to ASCIIZ new name
                INT     21H                 ;AX=Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_rfile_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_rfile_ok:   POP     BP
                RET     6H
asm_rfile       ENDP

asm_cfile       PROC NEAR
                ;Create file, or truncate to zero (handle)
                ;
                ;Enter: AH = 3CH
                ;       CX = File attribute required
                ;            Bit 1 - Set if hidden reqd
                ;            Bit 2 - Set is system reqd
                ;            All other bits zero
                ;    DS:DX = ASCIIZ pointer to filename
                ;
                ;Exit: C Clear - AX = File handle
                ;      C Set   - AX = Error code
                ;                     3 - path not found
                ;                     4 - no handles available
                ;                     5 - access denied
                ;
                ;Usage: Can make file in any directory or drive.
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,3CH
                MOV     CL,BYTE PTR [BP+6]  ;Access code
                MOV     CH,0H
                MOV     DX,WORD PTR [BP+8]  ;Pointer to ASCIIZ
                INT     21H                 ;AX=Handle or Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_cfile_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_cfile_ok:   POP     BP
                RET     6H
asm_cfile       ENDP

asm_get_attrib  PROC NEAR
                ;Get attributes for a file (handle)
                ;
                ;Enter: AH = 43H
                ;       AL = 00H
                ;    DS:DX = ASCIIZ pointer to filename
                ;
                ;Exit: C Clear - AX = Attribute byte
                ;                     Bit 0 = R/O
                ;                         1 = Hidden
                ;                         2 = System
                ;                         3 = Volume label
                ;                         4 = Directory
                ;                         5 = Archive
                ;      C Set   - AX = Error code
                ;                     01 - invalid function
                ;                     02 - file not found
                ;                     03 - path not found
                ;                     05 - access denied
                ;
                ;Usage: volume label & subdirectories ???
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,43H
                MOV     AL,00H
                MOV     DX,WORD PTR [BP+6]  ;Pointer to ASCIIZ
                INT     21H                 ;AX=Handle or Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_get_a_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_get_a_ok:   POP     BP
                RET     4H
asm_get_attrib  ENDP

asm_set_attrib  PROC NEAR
                ;Set attributes for a file (handle)
                ;
                ;Enter: AH = 43H
                ;       AL = 01H
                ;       CX = Attribute byte (only these 4 allowed)
                ;            Bit 0 = R/O
                ;                1 = Hidden
                ;                2 = System
                ;                5 = Arch
                ;    DS:DX = ASCIIZ pointer to filename
                ;
                ;Exit: C Clear - CX = Attribute byte
                ;                     Bit 0 = R/O
                ;                         1 = Hidden
                ;                         2 = System
                ;                         3 = Volume label
                ;                         4 = Directory
                ;                         5 = Archive
                ;      C Set   - AX = Error code
                ;                     01 - invalid function
                ;                     02 - file not found
                ;                     03 - path not found
                ;                     05 - access denied
                ;
                ;Usage: volume label & subdirectories ???
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,43H
                MOV     AL,00H
                MOV     DX,WORD PTR [BP+8]  ;Pointer to ASCIIZ
                MOV     CL,BYTE PTR [BP+6]  ;???????????????? word OR byte
                MOV     CX,0                ;???????????????? DITTO GET A
                INT     21H                 ;AX=Handle or Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_set_a_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_set_a_ok:   POP     BP
                RET     6H
asm_set_attrib  ENDP

asm_get_fdt     PROC NEAR               ;Files date and time
                ;ah=57h
                ;al=00
                ;bx=handle
                ;
                ;carry clear - ok
                ;              cx=time
                ;              dx=date
                ;carry set - ax = error code
                ;                 01 - invalid function
                ;                 06 - invalid handle
                ;
                ;time: bits 0-4 seconds/2
                ;           5-a minutes
                ;           b-f hours
                ;date: bits 0-4 day
                ;           5-8 month
                ;           8-f year (from 1980)
asm_get_fdt     ENDP

asm_set_fdt     PROC NEAR
                ;ah=57h
                ;al=00
                ;bx=handle
                ;
                ;carry clear - ok
                ;              cx=time
                ;              dx=date
                ;carry set - ax = error code
                ;                 01 - invalid function
                ;                 06 - invalid handle
                ;
                ;for strucure see asm_set_fdt
asm_set_fdt     ENDP

asm_mkdir       PROC NEAR
                ;Make directory
                ;
                ;Enter: AH = 39H
                ;    DS:DX = Pointer to ASCIIZ
                ;
                ;Exit: C Clear - Directroy created OK
                ;      C Set   - AX = Error code
                ;                     3 - Path not found
                ;                     5 - Access denied
                ;
                ;Usage: Can be used to create directories on other drives?
                ;       Access denied if directory exists or path not found.
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,39H
                MOV     DX,WORD PTR [BP+6]  ;Pointer to ASCIIZ
                INT     21H                 ;AX=Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_mkdir_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_mkdir_ok:   POP     BP
                RET     4H
asm_mkdir       ENDP

asm_rmdir       PROC NEAR
                ;Remove directory
                ;
                ;Enter: AH = 3AH
                ;    DS:DX = Pointer to ASCIIZ
                ;
                ;Exit: C Clear - Directroy removed OK
                ;      C Set   - AX = Error code
                ;                     03H - Path not found
                ;                     05H - Access denied
                ;                     10H - Current Dir
                ;
                ;Usage: Can be used to remove directories on other drives?
                ;       Access denied if specified directory is current or
                ;       if it does not exist.
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,3AH
                MOV     DX,WORD PTR [BP+6]  ;Pointer to ASCIIZ
                INT     21H                 ;AX=Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_rmdir_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_rmdir_ok:   POP     BP
                RET     4H
asm_rmdir       ENDP

asm_get_dir     PROC NEAR
                ;Get directory
                ;
                ;Enter: AH = 47H
                ;       DL = Drive (0=Default, 1=A, 2=B, etc)
                ;    DS:SI = Pointer to buffet to fill (65 chars max)
                ;
                ;Exit: C Clear - Buffer filled with ASCIIZ
                ;      C Set   - AX = Error code
                ;                     0FH - Invalid drive
                ;
                ;Usage: Can be used to get current dir for other drives
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,47H
                MOV     DL,BYTE PTR [BP+6]  ;Pointer to drive code
                MOV     SI,WORD PTR [BP+8]  ;Pointer to buffer
                INT     21H                 ;AX=Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_get_dir_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_get_dir_ok: POP     BP
                RET     6H
asm_get_dir     ENDP

asm_set_dir     PROC NEAR
                ;Set directory
                ;
                ;Enter: AH = 3BH
                ;    DS:DX = Pointer to ASCIIZ
                ;
                ;Exit: C Clear - Directory set OK
                ;      C Set   - AX = Error code
                ;                     03H - Path not found
                ;
                ;Usage: Cannot be used to set other drives dirs?
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,47H
                MOV     DX,WORD PTR [BP+6]  ;Pointer to ASCIIZ
                INT     21H                 ;AX=Error code
                MOV     BP,WORD PTR [BP+4]  ;Pointer to ERROR CODE
                MOV     WORD PTR DS:[BP],0  ;Clear ERROR CODE
                JNC     asm_set_dir_ok
                MOV     WORD PTR DS:[BP],AX ;Set ERROR CODE
asm_set_dir_ok: POP     BP
                RET     4H
asm_set_dir     ENDP

asm_get_dir_spc PROC NEAR
                ;ah=36h
                ;dl=drive (0=def, 1=a etc)
                ;
                ;ax=sectors per cluster (or ffffh if drive invalid)
                ;bx=no of avail clusters
                ;cx=bytes per sector
                ;dx=tot num of clusters
                ;
                ;free space=bx*ax*cx
                ;capacity  =dx*ax*cx
asm_get_dir_spc ENDP

asm_get_drive   PROC NEAR
                ;Get Drive
                ;
                ;Enter: AH = 19H
                ;
                ;Exit:  AL = Current Drive (A=0, B=1, etc)
                ;
                ;Usage: This routine cannot fail!
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,19H
                INT     21H
                POP     BP
                RET
asm_get_drive   ENDP

asm_set_drive   PROC NEAR
                ;Set drive
                ;
                ;Enter: AH = 0EH
                ;       AL = Drive number (A=0, B=1, etc)
                ;
                ;Exit:  AL = Last drive number (A=1, B=2, etc)
                ;
                ;Usage: Change all references to a drive code to 1 based?
                ;       SO if request change to drive zero then do nothing?
                ;       This routine cannot fail - but what if it seletct
                ;                                  a non-existant drive?
                ;
                PUSH    BP
                MOV     BP,SP
                MOV     AH,0EH
                MOV     AL,BYTE PTR [BP+4]  ;Drive number
                INT     21H
                POP     BP
                RET     2H
asm_set_drive   ENDP

asm_get_dta     PROC NEAR
                ;ah=2fh
                ;
                ;es:bx points to DTA
asm_get_dta     ENDP

asm_set_dta     PROC NEAR
                ;ah=1ah
                ;ds:dx=new DTA
asm_set_dta     ENDP

asm_sff         PROC NEAR
                ;ah=4eh
                ;cx=search attrib
                ;   00h - normal
                ;   02h - normal & hidden
                ;   04h - normal & system
                ;   06h - normal, hidden & system
                ;   08h - volume labels
                ;   10h - directories
                ;ds:dx=pointer to asciiz
                ;
                ;carry clear if sucessfull
                ;carry set if error : ax = error code
                ;                          02h - file not found
                ;                          03h - invalid path
                ;                          12h - no more files
                ;
                ;DTA gets filled
                ;       Bytes 0-14h        : reserved for dos use
                ;       Byte at offset 15h : attribute of matched file
                ;       word at offset 16h : time (see asm_get_fdt)
                ;       word at offset 18h : date ( ditto         )
                ;       dword at offset 1ah: file size
                ;       13 bytes from offset 1eh : asciiz of filename.
asm_sff         ENDP

asm_sfn         PROC NEAR
                ;ah=4fh
                ;
                ;carry clear if success
                ;carry set if error - AX=error code
                ;                        12h - no more files
asm_sfn         ENDP

code            ENDS

stack           SEGMENT PUBLIC 'stack'
stack           ENDS

END
