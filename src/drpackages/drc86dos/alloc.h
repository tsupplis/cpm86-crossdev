
/***************************************************************************
    ALLOC86.H - compatibilty header for CP/M-86 & the IBM-PC family.

		(C)1999 Ken Mauro, All rights reserved.
		        Free for non-commercial use only.
****************************************************************************/

/* Use for direct access to the far heap (640k - minus CP/M-86 & programs    */ 
 

struct {				/* global mc_alloc structure */
unsigned base;
unsigned len;
    char ext;
} mcb;

long int farmalloc(size)		/* not a true allocate function */
unsigned size;				/* only one call per program    */
{					/* SEE NOTES at bottom of page */

	mcb.base = 0;
	mcb.len  = (size>>4)+1*((size%16)!=0) ; /* convert to paragraphs+1 */
	mcb.ext  = 0;

	__BDOS(55,&mcb);		/* Bdos #55 = allocmem */

	return((long)mcb.base<<16);

}


farfree(p)				/* dummy arg for this version */
long int *p;
{

	mcb.ext  = 0;		/* 0 = release only farmalloc() memory*/
	__BDOS(57,&mcb);

}

/* -------------------------------------------------------------------------- 
   NOTE1  This is not a true implemation, but was intended to at least be a 
          syntax compatible farmalloc(), for Turbo C -> DRC porting purposes.
	  DRC 1.11 by default appears to only allows a malloc() of about 30k 
          (64k-program size?) 
          Even if ES expansion via LINK86 is used, the largest malloc is 64k, 
          the technique bay cause problems with the dos emulator  

   NOTE2  This version only supports one farmalloc and one farfree at a time. 

   NOTE2: CPM.EXE 1.3 (MSDOS CP/M-86 emulator) requires an extra 128 bytes 
          added (8 paragraphs) to the requested size (or mcb.len) to prevent 
          msdos from crashing after exit & during command.com reload.

          This value was arrived at by experimentation (lots of crashing..)
          and does not seem to be related to program size etc. It is not clear
          at this time if the fault is DRC, the emulator or my programming.

	  While running under real CP/M-86, programs appear to exit normally. 	
  ----------------------------------------------------------------------------*/


