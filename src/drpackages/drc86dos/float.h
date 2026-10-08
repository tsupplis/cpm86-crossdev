
/****************************************************************************
        FLOAT.H - compatibilty header for CP/M-86 & the IBM-PC family

                  (C)1998 Ken Mauro, All Rights Reserved.
                          Free for non-commercial use.
****************************************************************************/

/* by moving these definitions from STDIO.H, we only link in when required */ 
/* this will reduce the size of the compiled exceutable.                   */

extern double	atof();
extern double	sqrt();
extern double	cos(), sin();
extern double	exp(), fabs();
extern double	tan(), atan(); 
extern double	log(), log10();






