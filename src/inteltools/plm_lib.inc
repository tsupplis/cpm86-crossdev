
   /*----------------------------------
     Program : DOS_LIB.INC
     Title   : Include file for DOS library
     Author  : Mr S.C.Agate FCD x4667
     Date    : 09-AUG-90
   ----------------------------------*/

/*              DECLARE clock STRUCTURE ( year    WORD,
                                          month   BYTE,
                                          day     BYTE,
                                          hour    BYTE,
                                          minute  BYTE,
                                          second  BYTE,
                                          csecond BYTE); */

get_clock:      PROCEDURE (ptr_to_struc) EXTERNAL;
                DECLARE ptr_to_struc POINTER;
                END get_clock;

new_line:       PROCEDURE EXTERNAL;
                END new_line;

get_string:     PROCEDURE (ptr_to_string,max_len) BYTE EXTERNAL;
                DECLARE ptr_to_string POINTER;
                DECLARE max_len BYTE;
                END get_string;

put_string:     PROCEDURE (ptr_to_string) EXTERNAL;
                DECLARE ptr_to_string POINTER;
                END put_string;

put_hex_num:    PROCEDURE (value,digits) EXTERNAL;
                DECLARE value  WORD;
                DECLARE digits BYTE;
                END put_hex_num;

put_dec_num:    PROCEDURE (value) BYTE EXTERNAL;
                DECLARE value WORD;
                END put_dec_num;

get_dec_num:    PROCEDURE WORD EXTERNAL;
                END get_dec_num;

upper:          PROCEDURE (ptr_to_string) EXTERNAL;
                DECLARE ptr_to_string POINTER;
                END upper;

asm_openf:      PROCEDURE (asciiz,access,errorc) WORD EXTERNAL;
                DECLARE asciiz POINTER,
                        access BYTE,    /* 0=Read, 1=Write, 2=R/W */
                        errorc POINTER;
                /* Returns handle if no error */
                END asm_openf;

asm_rd_chrs:    PROCEDURE (handle,buffer,length,errorc) WORD EXTERNAL;
                DECLARE handle WORD,
                        buffer POINTER,
                        length WORD,
                        errorc POINTER;
                /* Returns length read if no error */
                END asm_rd_chrs;

asm_wr_chrs:    PROCEDURE (handle,buffer,length,errorc) WORD EXTERNAL;
                DECLARE handle WORD,
                        buffer POINTER,
                        length WORD,
                        errorc POINTER;
                /* Returns length written if no error */
                END asm_wr_chrs;

asm_set_loc:    PROCEDURE (handle,access,index,errorc) DWORD EXTERNAL;
                DECLARE handle WORD,
                        access BYTE, /* 0=Beg, 1=Current, 2=End */
                        index  DWORD,
                        errorc POINTER;
                /* Returns with new offset if no error */
                END asm_set_loc;

asm_closef:     PROCEDURE (handle,errorc) EXTERNAL;
                DECLARE handle WORD,
                        errorc POINTER;
                END asm_closef;

asm_dfile:      PROCEDURE (asciiz,errorc) EXTERNAL; /* Delete File */
                DECLARE asciiz POINTER,
                        errorc POINTER;
                END asm_dfile;

asm_rfile:      PROCEDURE (aold,anew,errorc) EXTERNAL; /* Rename File */
                DECLARE aold   POINTER,
                        anew   POINTER,
                        errorc POINTER;
                END asm_rfile;

asm_cfile:      PROCEDURE (asciiz,attrib,errorc) WORD EXTERNAL; /* Create File */
                DECLARE asciiz POINTER,
                        attrib BYTE,
                        errorc POINTER;
                /* Returns handle if no error */
                END asm_cfile;

asm_get_attrib: PROCEDURE (asciiz,errorc) WORD EXTERNAL; /* Get Attribute */
                DECLARE asciiz POINTER,
                        errorc POINTER;
                /* Returns attribute byte if no error */
                END asm_get_attrib;

asm_set_attrib: PROCEDURE (asciiz,attrib,errorc) WORD EXTERNAL; /* Set Attribute */
                DECLARE asciiz POINTER,
                        attrib BYTE,
                        errorc POINTER;
                /* Returns attribute byte if no error */
                END asm_set_attrib;

asm_mkdir:      PROCEDURE (asciiz,errorc) EXTERNAL; /* Make Directory */
                DECLARE asciiz POINTER,
                        errorc POINTER;
                END asm_mkdir;

asm_rmdir:      PROCEDURE (asciiz,errorc) EXTERNAL; /* Remove Directory */
                DECLARE asciiz POINTER,
                        errorc POINTER;
                END asm_rmdir;

asm_get_dir:    PROCEDURE (buffer,byte,errorc) EXTERNAL; /* Get a drives Def Dir */
                DECLARE buffer POINTER,
                        drive  BYTE,
                        errorc POINTER;
                /* 65 byte buffer filled with def drive unless error */
                END asm_get_dir;

asm_set_dir:    PROCEDURE (asciiz,errorc) EXTERNAL; /* Sets Def Dir */
                DECLARE asciiz POINTER,
                        errorc POINTER;
                /* Def Dir is changed unless error */
                END asm_set_dir;

asm_get_drive:  PROCEDURE BYTE EXTERNAL; /* Return zero based drive code */
                END asm_get_drive;

asm_set_drive:  PROCEDURE (drive) BYTE EXTERNAL; /* Get drive */
                DECLARE drive BYTE;
                /* A one based drive code is returned */
                END asm_get_drive;

