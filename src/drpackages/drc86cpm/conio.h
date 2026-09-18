
/****************************************************************************
    CONIO86.H - compatibilty header for CP/M-86 & the IBM-PC family.

		(C)1998 Ken Mauro, All rights reserved.
		        Free for non-commercial use.
****************************************************************************/

/* This set of routines should permit OS independant console input from       */
/* cpm86, ccpm86, dosplus & cdos by direct access of the IBM keyboard port    */
/* for fkey and keypad while still using BDOS input for everything else.      */

/* getch()   should ignore cp/m-86 pfk strings while under program control.   */

/* If IBMPC=1  requires PCBIOS.A86 support. (Large model only - for now)      */
/* pfk/csr/term codes vary between OS families - re-write as required         */


#define IBMPC	1			/* 1 = IBMPC low-level keyb handler */
					/* 0 = BDOS generic console handler */

extern int _OS_VERSION;			/* Set by DRC startup routines      */
extern int _OS_ABILITY;			/* Can be used for program query    */ 


kbhit() { return( __BDOS(6,254) ); }			/* was a key pressed? */


getch()
{
	int i,c,d;
	static int s=0;			/* static holds (s) for re-entry    */

	if(s>0){ 			/* if s>0, the second fkey char was  */ 
		c=s;			/* waiting as (s) from last entry.   */
		s=0; 			/* reset special second char flag    */
		return(c); 		/* return the second IBM fkey char   */ 
		}				

	while(!(c=__BDOS(6,255)) );			/* wait for non-zero */

#if IBMPC
	s=keyb();					/* get IBMPC keyport */
	if( s>58 && s<69 || s>70 && s<84 ) c=0;		/* IBM special  c=0  */
	else s=0;					/* regular key  s=0  */
#endif

	while( (d=__BDOS(6,255)) );			/* eat up any pfkeys */

	return(c);

}


/*---------------------------------------------------------------------------- 
	    Generic terminal code routines ( passed by value ) 
		codes can be edited for non-IBM systems 
------------------------------------------------------------------------------*/

clrscr() { putchar(27); putchar('E'); }			/* cls */

cursor(n) int n; {
	putchar(27);
	if(n==0x4700) putchar('H');			/* home cursor	*/
	if(n==0) putchar('m');				/* cursor on	*/
	if(n==1) putchar('n');				/* cursor off	*/
	if(n==2) putchar('j');				/* save x,y	*/
	if(n==3) putchar('k');				/* restore x,y	*/
	}

statline(n) int n; {
	putchar(27);
	if(n==0) putchar(49);				/* statline on	*/
	if(n==1) putchar(48);				/* statline off	*/
	}

gotoxy(x,y)					/* set cursor to row, column */
int x, y;
{
	putchar(27);
	putchar('Y');
	putchar(32+x);
	putchar(32+y);
}

color(f,b)						/* choices: 0 - 15 */
char f,b;
{
	if(f<16) putchar(27); putchar('b'); putchar(f);
	if(b<16) putchar(27); putchar('c'); putchar(b);
}

esc(code)					/* single terminal codes area */
char code;
{ 
	putchar(27); putchar(code); 
}


