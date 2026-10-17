	; HELLO80.ASM -- minimal 8080-subset program for TRANS.COM
	;
	; This uses only 8080-compatible instructions, expressed in the Z80
	; source syntax understood by the SCP Z80-to-8086 translator.  Under
	; 86-DOS, function 9 at CALL 5 displays the '$'-terminated string.
	;
	ORG	100H

	MOV	CL,9		; Console output function
	MOV	DX,MESSAGE	; DE points to the '$'-terminated text
	CALL	5
	RET

message:
	DB	'Hello from z80 via trans.com',13,10,'$'
