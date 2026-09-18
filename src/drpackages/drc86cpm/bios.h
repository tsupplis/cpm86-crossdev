
/****************************************************************************
       BIOS86.H - compatibilty header for CP/M-86 & the IBM-PC family

                  (C)1999 Ken Mauro, All Rights Reserved.
                          Free for non-commercial use.
****************************************************************************/

		/* CP/M & IBMPC bios related stuff */


struct{ char fn;				
	char cl,ch;
	char dl, dh; } pblk;	


bios(p)   unsigned int *p;    { return __BDOS(50,&pblk); }


	



