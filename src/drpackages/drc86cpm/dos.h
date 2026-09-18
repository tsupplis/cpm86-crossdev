
/****************************************************************************
        DOS86.H - compatibilty header for CP/M-86 & the IBM-PC family

                  (C)1998 Ken Mauro, All Rights Reserved.
                          Free for non-commercial use.
****************************************************************************/

/* Currently, BIOS.A86 (ibmpc bios module) only supports the LARGE MODEL.  */


#ifndef CPM86
#define CPM86	1
#endif

extern int _OS_VERSION;			/* DRC defined in startup.a86        */
extern int _OS_ABILITY;			/* both may not agree with bdos(12)  */
                                        /* when running under a CPM emulator */

struct	WORDREGS{ unsigned int ax, bx, cx, dx, si, di, cflag, flags; };
struct	BYTEREGS{ char al, ah, bl, bh, cl, ch, dl, dh; };
struct	SREGS	{ unsigned int	es, cs, ss, ds;	};
struct	REGPACK { unsigned r_ax, r_bx, r_cx, r_dx;
		  unsigned r_bp, r_si, r_di, r_ds, r_es, r_flags; };


union	REGS	{struct	WORDREGS x; struct BYTEREGS h;};


#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned size_t;
#endif

#define FP_OFF(ofs)	((unsigned)(ofs))
#define FP_SEG(segs)	((unsigned)((long int)(segs)>>16))
#define MK_FP(segs,ofs)	(((long int)(segs)<<16)|(unsigned int)(ofs))

bdos(f,p) unsigned int *f,*p; { return __BDOS(f,p); }

memcpy(dest,src,n) char *dest, *src; int n; { blkmove(dest, src, n); }
memset(dest,src,n) char *dest,  src; int n; { blkfill(dest, src, n); }


