
ARC86 - An Archive Utility for CP/M-86

Ported to CP/M-86 by D. J. Weatherall (November 1987)
from the original CP/M-80 ARCCPM source code by Reimer Mellin

              Version 1.0 - Turbo Pascal 3.00A, August 1987
              Placed in the public domain by  Reimer Mellin
              Sulenstr.8,  D-8000 Muenchen 71, West Germany
              RBBS: T-BUS, Fido Net 507/14 Phone 089/293881


              > NOTE: FOR PRIVATE NON-COMMERCIAL USE ONLY <

                                 U S A G E :

           ARC {amxevld} [bnwst] [g<passw>] <arch> [<flist>..]
           a   = add files to archive   m   = move files to archive
           x,e = extract from archive   d   = delete from archive
           l,v = list files in archive  b   = keep backup of archive
           w   = suppress warnings      n   = suppress notes
           s   = store only             t   = set date and time
           g   = Encrypt/decrypt entry  z   = use extended bios


==============================================================================

[Following are D. J. Weatherall's original comments on the program.  Please
 note that ARC86.CMD will work on any generic Intel 8086/8088 or 80x86-based
 computer, running either single-user or Concurrent CP/M-86 as its operating
 system.  The notes regarding the extended BIOS function of the DEC Rainbow
 computer do NOT apply to "normal" IBM-compatible machines, so don't use the
 "Z" option; it will cause the machine to irrecoverably "hang."  The "T" op-
 tion, at least under single-user CP/M-86, does NOT fetch the current date
 and time from the machine's time-of-day clock.  Rather, it prompts you to
 manually enter the date and time.  Also note that User Areas are not sup-
 ported in ARC86.  The "bug" which Weatherall mentions has not, so far,
 been noticed on generic IBM-clone 8088 machines.]

 
ARC86 is a CP/M-86 implementation of Reimer Mellin's CP/M ARRCPM program.

As far as I can tell it works correctly under Concurrent CP/M-86 and will
use the time set by the DATE program.

My Rainbow has an extended BIOS function (No. 154) which performs the
BDOS function 105. If you have a similar facility then you can use the 
'z' qualifier.

                ARC86 ATZ TEST B:NEWFILE

There is one bug remaining that I know of which is proving difficult to
isolate. On some occasions you may get Turbo Pascal's I/O error 91 because
ARC has calculated a sector above the last file it is playing with.

You can work around this by inserting the files in a different sequence.
(Preferably longest first). Afterwards a list will tell you that a file
has a bad header. This, in my short experience, can be ignored and your
files have been archived and can be extracted safely.

Thanks to Reimer Mellin and Peter Spaeth2 for the original effort on the
CP/M version.

        D.J.Weatherall 22-Dec-87
