{
.. Any line starting with a control character is used for printing out
.. the program nicely using PC-Write.  They do not affect compilation.
.. These sections are used to print out psh.pas nicely using PC-Write
L---+---T1----+-T--2----T----3--T-+----4T---+---T5----+-T--6----R----7--T-+--r
.. Use line printer font when printing this out, and indent 15 spaces
.r:c
.x:15
.-

  psh.pas:

  (c) Copyright 1987 Jim Frost
  All Rights Reserved

  This program is copyrighted under the laws of the United States and
  other countries.  See accompanying document PSH.DOC for more
  information and details on this copyright.  Version number is
  given in the const section.

  This is designed with minimal stack overhead in mind.  It will have no
  problems at all with only a minimal stack.  Recommended  compilation
  options: mIn = 100 mAx = 100.  Note that you MUST set mAx or nothing
  will run because it will allocate all the memory when executed, then
  stop dead when it tries to allocate its work areas.

  This particular source is for version 1.4.  It contains sections that
  are not complete (intended for version 2.0).  The incomplete sections
  are commented out using (* and *).
}

{$p128,g128,i-,c-,u-}
program psh;

const envsiz : integer = 1024; { typed constants so we can change if necessary }
      cmdsiz : integer = 1024;
      alssiz : integer = 1024;
      hstsiz : integer = 1024;
      maxdepth = 8;            { can have 8 source commands nested }
      argbreak : set of char = [^I,' ',#13];
      alphanumeric : set of char = ['a'..'z','A'..'Z','0'..'9'];
      version = 'PSH Version 1.5 - (c) Copyright 1987 Jim Frost - All Rights Reserved';

type register = record
                  ax,bx,cx,dx,bp,si,di,ds,es,flags : integer
                end;
     parmblock = record
       envseg,
       argofs,
       argseg,
       fcb1ofs,
       fcb1seg,
       fcb2ofs,
       fcb2seg : integer
     end;
     c_str = array[0..maxint] of char;
     str = ^c_str;

var als,                          { pointer to alias area }
    wrk,                          { work area (for aliasing, etc) }
    env,                          { pointer to environment area }
    cmd,                          { pointer to command line area }
    hst : str;                    { pointer to history area }
    interactive,                  { true if we are reading from cmd line }
    topipe,                       { true if we're sending to a pipe }
    frompipe,                     { true if we're receiving from a pipe }
    brkpipe,                      { true if broken pipe (some kind of error) }
    perm,                         { true if this is a permanent shell }
    done    : boolean;            { done all processing }
    srcdepth,
    p,
    hstbeg,                       { start number of history }
    hstend  : integer;            { end number of history }
    params  : array[0..256] of char; { parameter area used for exec }
    parm    : parmblock;          { parameter area used by exec for dos exec }
    r       : register;           { register struct for msdos calls }
(*    handles : array[0..4] of integer; { array of std file handles }
    fnames  : array[0..4] of str; { array of ptrs to fnames }
*)
{

}
{***********************************************************************}
{ This section contains C-like declarations that are used throughout
  psh. }

const null = #0;

var errno : integer;

{ These functions are used to access the DOS memory allocation
  routines.  It returns a pointer to a character array.  The actual
  size allocated must be a multiple of 16, so it is wise to allocate
  in large amounts to avoid fragmentation. }

type mem_reg = record
                 ax,bx,cx,dx,bp,si,di,ds,es,flags : integer
               end;

(*
function fopen(s,m : str) : integer;
begin
  r.ax:= $3D00;
  if strcmp(m,strcnv('r'))=0 then begin end
  else if strcmp(m,strcnv('w'))=0 then r.ax:= r.ax+1;
  r.ds:= seg(s^);
  r.dx:= ofs(s^);
  msdos(r);
  if (flags and 1)=1 then begin
    errno:= r.ax;
    fopen:= -1
  end
  else
    fopen:= r.ax
end;

fclose(f : integer);
begin
  r.ax:= $3E00;
  r.bx:= f;
  msdos(r);
  if (flags and 1)=1 then
    errno:= r.ax
  else
    errno:= 0
end;

function dup(f : cfile) : integer;
var r : cf_reg;
begin
  r.ax:= $4500;
  r.bx:= f;
  msdos(r);
  if (r.flags and 1)=1 then begin
    fdup:= -1;
    errno:= r.ax
  end
  else
    fdup:= r.ax
end;

function fdup2(f,old : integer) : integer;
begin
  r.ax:= $4600;
  r.bx:= f;
  r.cx:= old;
  msdos(r);
  if (r.flags and 1)=1 then begin
    fdup2:= -1;
    errno:= r.ax
  end
  else
    fdup2:= r.ax
end;
*)
{ allocate memory block of n bytes.  returns nill pointer if it
  couldn't do it, along with the DOS error number.  Generally any
  failure means that the memory wasn't available. }

function malloc(n : integer) : str;
var r : mem_reg;
begin
  r.ax:= $4800;           { MS-DOS allocation function }
  if (n mod 16)=0 then    { make sure it's a multiple of 16 bytes }
    r.bx:= n div 16
  else
    r.bx:= (n div 16)+1;
  msdos(r);
  if (r.flags and 1)=1 then begin { error }
    malloc:= nil;
    errno:= r.ax
  end
  else begin
    errno:= 0;
    malloc:= addr(mem[r.ax:0])
  end
end;

{ free up an allocated block.  errno can be checked for errors. }

procedure free(m : str);
var r : mem_reg;
begin
  r.ax:= $4900;   { MS-DOS free memory function }
  r.es:= seg(m^);
  msdos(r);
  if (r.flags and 1)=1 then { error }
    errno:= r.ax
  else
    errno:= 0
end;

{ This section is mostly string routines }

type c_tpstr = string[255];

function strcnv(s : c_tpstr) : str; { convert TPascal string to C string }
const tpstr : c_tpstr = '';
begin
  tpstr:= s+null;
  strcnv:= addr(tpstr[1])
end;

function tpstr(s : str) : c_tpstr; { convert C string to TPascal string }
var t : string[255];
    p : integer;
begin
  t:= '';
  p:= 0;
  while (s^[p]<>null) and (p<255) do begin
    t:= t+s^[p];
    p:= p+1
  end;
  tpstr:= t
end;

procedure putstr(s : str); { dump string to screen }
var n : integer;
begin
  n:= 0;
  while s^[n]<>null do begin
    write(s^[n]);
    n:= n+1
  end
end;

function strlen(s : str) : integer; { return length of a string }
var n : integer;
begin
  n:= 0;
  while s^[n]<>null do
    n:= n+1;
  strlen:= n
end;

procedure strcpy(s1,s2 : str); { copy s2 into s1 }
var n : integer;
begin
  n:= -1;
  repeat
    n:= n+1;
    s1^[n]:= s2^[n]
  until s2^[n]=null
end;

procedure strcat(s1,s2 : str); { add s2 to s1 }
var n,p : integer;
begin
  n:= strlen(s1);
  p:= 0;
  while s2^[p]<>null do begin
    s1^[n]:= s2^[p];
    p:= p+1;
    n:= n+1
  end;
  s1^[n]:= null;
end;

procedure strncpy(s1,s2 : str; l : integer); { copy up to l bytes from s2 to s1 }
var n : integer;
begin
  n:= -1;
  repeat
    n:= n+1;
    if n=l
      then exit;
    s1^[n]:= s2^[n]
  until (s2^[n]=null)
end;

{ compare s1 and s2.  returns -1 if s1 < s2, 0 if s1 == s2, 1 if s1 > s2 }

function strcmp(s1,s2 : str) : integer;
var n : integer;
begin
  n:= 0;
  while (s1^[n]=s2^[n]) and (s1^[n]<>null) do
    n:= n+1;
  if s1^[n]=s2^[n] then
    strcmp:= 0
  else if s1^[n]<s2^[n] then
    strcmp:= -1
  else
    strcmp:= 1
end;

{ compare s1 and s2 for up to l bytes }

function strncmp(s1,s2 : str; l : integer) : integer;
var n : integer;
begin
  n:= 0;
  while (s1^[n]=s2^[n]) and (s1^[n]<>null) and (n<l-1) do
    n:= n+1;
  if s1^[n]=s2^[n] then
    strncmp:= 0
  else if s1^[n]<s2^[n] then
    strncmp:= -1
  else
    strncmp:= 1
end;

function strsrch(s1,s2 : str) : integer; { look for s2 in s1 }
var l1,l2,n : integer;
begin
  l1:= strlen(s1);
  l2:= strlen(s2);
  n:= 0;
  while (n<=l1-l2) and (strncmp(addr(s1^[n]),s2,l2)<>0) do
    n:= n+1;
  if strncmp(addr(s1^[n]),s2,l2)=0 then
    strsrch:= n
  else
    strsrch:= -1
end;

function atoi(s : str) : integer; { convert string to an integer }
const spaces : set of char = [' ',^I,#10,#13];
var e,p,v : integer;
begin
  p:= 0;
  v:= 0;
  e:= 1;
  while (s^[p] in spaces) and (s^[p]<>null) do
    p:= p+1;
  while s^[p]<> null do begin
    if s^[p] in ['0'..'9'] then
      v:= (v*10)+ord(s^[p])-ord('0')
    else if s^[p] in spaces then { stop at first space }
      exit
    else
      e:= 0;
    p:= p+1
  end;
  atoi:= v*e { if something stupid was there, then this zeroes the value }
end;

function itoa(i : integer) : str; { convert integer to string }
const s : array[0..5] of char = (null,null,null,null,null,null);
var s2  : string[5];
    a   : integer;
begin
  str(i,s2);
  for a:= 1 to length(s2) do
    s[a-1]:= s2[a];
  s[length(s2)]:= null;
  itoa:= addr(s)
end;

{ this procedure provides an exit to parent, with exit status }

procedure freemem; forward; { needed within errout() }

procedure errout(e : integer);
begin
  freemem;
  r.ax:= $4C00+e;
  msdos(r)
end;

{ this function gets the return code of a child }

function wait : integer;
begin
  r.ax:= $4D00;
  msdos(r);
  wait:= lo(r.ax)
end;

{

}
{***********************************************************************}
{ This section contains base routines used throughout psh }

{ this procedure initializes the first two bytes of an array to null }

procedure initarray(a : str);
begin
  a^[0]:= null;
  a^[1]:= null
end;

{ this procedure is used to pack an array of strings.  it is intended to
  be used when deleting a string out of the array.  It will delete the
  first string, so you should pass it a pointer to the actual string you
  want deleted. }

procedure packarray(a : str);
var p,q : integer;
begin
  p:= 0;
  q:= 0;
  while a^[q]<>null do         { skip to first null }
    q:= q+1;
  q:= q+1;
  while a^[q]<>null do begin   { copy strings until a double null }
    while a^[q]<>null do begin { copy current string up to null }
      a^[p]:= a^[q];
      p:= p+1;
      q:= q+1
    end;
    a^[p]:= a^[q];             { copy the null and advance pointer }
    p:= p+1;
    q:= q+1
  end;
  a^[p]:= null                 { last char must be null }
end;

{ this procedure dumps out the contents of an array.  It was originally
  used for debugging, but now it is used by 'set' }

procedure showarray(a : str);
var p : integer;
begin
  p:= 0;
  while a^[p]<>null do begin
    while a^[p]<>null do begin
      write(a^[p]);
      p:= p+1
    end;
    writeln;
    p:= p+1
  end
end;

{ this procedure adds a new string onto the end of an array of strings }

procedure addarray(a,s : str);
var p,q : integer;
begin
  p:= 0;
  while a^[p]<>null do begin
    while a^[p]<>null do      { find end of current string }
      p:= p+1;
    p:= p+1                   { skip the null }
  end;
  q:= 0;
  while s^[q]<>null do begin  { copy the string onto the end }
    a^[p]:= s^[q];
    p:= p+1;
    q:= q+1
  end;
  a^[p]:= null;               { terminate the string }
  a^[p+1]:= null              { terminate the array }
end;

{ this procedure copies one array into another array }

procedure cpyarray(a1,a2 : str);
var p : integer;
begin
  p:= 0;
  while a2^[p]<>null do begin   { copy all strings }
    while a2^[p]<>null do begin { copy each string }
      a1^[p]:= a2^[p];
      p:= p+1
    end;
    a1^[p]:= a2^[p];            { copy null at end of string }
    p:= p+1
  end;
  a1^[p]:= null                 { copy null at end of array }
end;

{ this function returns number of bytes avail in an array.  note that
  it tries to find out what array it is before returning anything. }

function freearray(a : str) : integer;
var p,s : integer;
begin
  if a=env then s:= envsiz
  else if a=als then s:= alssiz
  else if a=hst then s:= hstsiz;
  if (a=env) or (a=als) or (a=hst) then begin
    p:= 0;
    while a^[p]<>null do begin
      while a^[p]<>null do
        p:= p+1;
      p:= p+1;
    end;
    freearray:= s-p-2 { 2 bytes - 1st byte and last null - aren't counted }
  end
  else begin
    writeln('freearray: unknown array');
    freearray:= 0
  end
end;

{ this function pulls entries from the environment and returns a pointer
  to them }

function getenv(s : str) : str;
var p     : integer;
    found : boolean;
begin
  p:= 0;
  found:= false;
  while env^[p]<>null do begin
    if (strncmp(addr(env^[p]),s,strlen(s))=0)  { if we have a match }
       and (env^[p+strlen(s)]='=') then begin
      getenv:= addr(env^[p+strlen(s)+1]);      { then set pointer and flag }
      found:= true
    end;
    while env^[p]<>null do                     { otherwise skip this string }
      p:= p+1;
    p:= p+1
  end;
  if not found then                            { found nothing so return null }
    getenv:= addr(env^[p])
end;

{

}
{***********************************************************************}
{ Forward declarations to allow this thing to compile, and also because
  there are some pretty hairy calls in here }

procedure parse(var command,arg,stuff : str); forward;
procedure unalias(s : str); forward;
procedure source(s : str); forward;
procedure docommand; forward;

{***********************************************************************}
{ This section contains functions to handle built-in commands }

{ this procedure handles the setting up of aliases.  it looks for alias
  loops before actually adding aliases. }

procedure alias(s : str);
var a,
    wrk2,
    command,
    args,
    stuff : str;
    c     : char;
    p     : integer;
    loop  : boolean;
begin
  if strcmp(s,strcnv(''))=0 then begin
    a:= als;
    while a^[0]<>null do begin
      putstr(a);
      write(copy('        ',(strlen(a) mod 8)+1,255)); { sort of tab it out }
      a:= addr(a^[strlen(a)+1]);
      putstr(a);
      a:= addr(a^[strlen(a)+1]);
      writeln
    end
  end
  else begin
    wrk2:= malloc(cmdsiz);
    if wrk2=nil then begin
      writeln('alias: cannot allocate work area to test for alias loops');
      exit
    end;
    strcpy(wrk2,s);                     { save it }
    p:= 0;
    while not ((wrk2^[p] in argbreak) or (wrk2^[p]=null)) do
      p:= p+1;
    wrk2^[p]:= null;
    unalias(wrk2);                      { destroy the old alias }

{ look for alias loops }

    strcpy(cmd,addr(s^[p+1]));          { pretend this is our command }
    loop:= false;
    while (cmd^[0]<>null) and (not loop) do begin
      parse(command,args,stuff);
      if strcmp(command,wrk2)=0 then begin
        writeln('alias loop');          { wow -- found a loop! }
        loop:= true
      end;
      if not loop then                  { nothing yet, on to next command }
        strcpy(cmd,stuff)
    end;
    if loop then begin
      free(wrk2);
      cmd^[0]:= null;                   { tell caller not to bother }
      exit
    end;

{ no alias loops, so go along our merry way }

    addarray(als,wrk2);
    s:= addr(wrk2^[p+1]);
    p:= 0;
    while (s^[p] in argbreak) and (s^[p]<>null) do
      p:= p+1;
    addarray(als,addr(s^[p]));
    free(wrk2)
  end;
  cmd^[0]:= null
end;

{ this is the 'cd' and 'chdir' commands }

procedure chdir(s : str);
var r : register; { biggest local variable }
begin
  if strcmp(s,strcnv(''))=0 then begin
    s:= getenv(strcnv('home'));        { go to "home" if var set }
    if s^[0]=null then begin           { no "home" var so print cwd }
      putstr(getenv(strcnv('cwd')));
      writeln;
      exit
    end
  end;
  if s^[1]=':' then begin              { if there's a drive specifier, then }
    r.ax:= $0E00;                      { try to switch to that drive }
    r.dx:= ord(upcase(s^[0]))-ord('A');
    msdos(r);
    s:= addr(s^[2]);                   { set pointer to path following drive }
    r.ax:= $1900;
    msdos(r);
    if lo(r.ax)<>lo(r.dx) then begin   { brainless user tried an out of bounds }
      writeln('chdir: nonexistent drive'); { drive, so yell at her (sexist!) }
      s:= strcnv('')
    end;
  end;
  if s^[0]=null then                   { no real chdir; just drive }
    exit;
  r.ax:= $3B00;                        { do chdir }
  r.ds:= seg(s^);
  r.dx:= ofs(s^);
  msdos(r);
  if (r.flags and 1)=1 then begin
    putstr(s);
    writeln(': path not found')
  end
end;

procedure echo(s : str);
begin
  putstr(s);
  writeln
end;

procedure doexit;
begin
  if not perm then
    done:= true
  else begin
    writeln('exit: cannot exit from permanent shell (use logout)');
  end;                       { "'Welcome,' said the doorman,        }
end;                         { 'We are programmed to receive;       }
                             { you can check out any time you like, }
                             { but you can never leave.'"           }

procedure history;
var a,p : integer;
begin
  p:= 0;
  for a:= hstbeg to hstend-1 do begin
    write(a : 5,' ');
    while hst^[p]<>null do begin
      write(hst^[p]);
      p:= p+1
    end;
    p:= p+1;
    writeln
  end
end;

procedure logout;
begin
  if perm then begin
    perm:= false;               { hey, no longer permanent! }
    done:= true;                { true forces error message off in source }
    strcpy(wrk,getenv(strcnv('home')));
    if wrk^[0]<>null then
      if wrk^[strlen(wrk)]<>'\' then
        strcat(wrk,strcnv('\'));
    strcat(wrk,strcnv('logout.sh'));
    source(wrk)                 { do our thing on the way out }
  end
  else
    writeln('logout: not login shell')
end;

{ this is the 'set' command }

procedure envset(s : str);
var w   : str;
    p   : integer;
begin
  w:= wrk;                             { need a work area }
  if strcmp(s,strcnv(''))=0 then       { no args, so print environ }
    showarray(env)
  else begin
    p:= strsrch(s,strcnv('='));        { arg w/ out =, so print env entry }
    if p<0 then begin
      w:= getenv(s);
      if w^[0]<>null then begin        { entry exists }
        putstr(addr(w^[-strlen(s)-1]));
        writeln
      end
    end
    else begin                         { arg w/ =, so delete current entry }
      strncpy(w,s,p);
      w^[p]:= null;
      w:= getenv(w);
      if w^[0]<>null then              { found entry to delete }
        packarray(addr(w^[-p-1]));
      if s^[p+1]<>null then            { something follows =, so add entry }
        addarray(env,s)
    end
  end
end;

{ this procedure is used to read a file into THIS shell as if it had
  been typed. }

procedure source;
var p : integer;
    f : text;
    i : boolean;
begin
  if srcdepth=maxdepth then begin
    writeln('source:  maximum source depth exceeded');
    exit
  end;
  i:= interactive;
  interactive:= false;
  srcdepth:= srcdepth+1;
  assign(f,tpstr(s));       { open file }
  reset(f);
  if ioresult<>0 then begin { error opening file so assume not found }
    if not done then begin  { done flag = true if psh.rc }
      putstr(s);
      writeln(': not found')
    end;
    exit
  end;
  p:= 0;
  while not eof(f) do begin { process each char }
    read(f,cmd^[p]);
    case cmd^[p] of
      #10,#13 : begin       { end of a line }
        cmd^[p]:= null;     { terminate the string }
        docommand;          { do it }
        p:= 0;              { reset pointer to beginning of line }
        cmd^[0]:= null      { nullify line }
      end;
      else p:= p+1          { increment character pointer }
    end;
  end;
  close(f);
  cmd^[p]:= null;           { if last line not terminated by CR or LF, then }
  docommand;                { this will trap that and run it anyway }
  interactive:= i;
  srcdepth:= srcdepth-1
end;

procedure shstat;
var r : register;
begin
  writeln('Command line size  = ',cmdsiz:6,' bytes');
  writeln('Environment size   = ',envsiz:6,' bytes; ',freearray(env):6,' free');
  writeln('History size       = ',hstsiz:6,' bytes; ',freearray(hst):6,' free');
  writeln('Alias area size    = ',alssiz:6,' bytes; ',freearray(als):6,' free');

{ try to allocate a HUGE amount of memory so alloc returns mem avail }

  r.ax:= $4800;
  r.bx:= $FFFF;
  msdos(r);
  writeln('System free memory = ',(((hi(r.bx)*256.0)+lo(r.bx))*16.0):6:0,' bytes')
end;

procedure unalias;
var a : str;
    p : integer;
begin
  a:= als;
  while (s^[0] in argbreak) and (s^[0]<>null) do
    s:= addr(s^[1]);
  p:= 0;
  while not ((s^[p] in argbreak) or (s^[p]=null)) do
    p:= p+1;
  s^[p]:= null;
  while a^[0]<>null do
    if strcmp(a,s)=0 then begin
      packarray(a);
      packarray(a)
    end
    else begin
      a:= addr(a^[strlen(a)+1]);
      a:= addr(a^[strlen(a)+1])
    end
end;

{

}
{***********************************************************************}
{ This section contains routines used when parsing command lines }

{ this function and the parse procedure are very closely matched.  parse
  parses out the command, copies everything else into a temporary area,
  and calls doalias.  doalias looks for the command in the alias list,
  makes a change if necessary, adds on all the other stuff that is in the
  work area, then tells parse whether or not to reparse and call alias
  again. }

function doalias : boolean;
var p     : integer;
    found : boolean;
begin
  p:= 0;
  found:= false;
  while (als^[p]<>null) and (not found) do
    if (strcmp(addr(als^[p]),cmd)=0) then         { compare command to alias }
      found:= true                                { both true, so we found it }
    else begin                                    { not true, so skip to next alias }
      while (als^[p]<>null) do                    { skip alias name }
        p:=p+1;
      p:= p+1;
      while (als^[p]<>null) do                    { skip alias value }
        p:= p+1;
      p:= p+1
    end;
  if found then begin                             { found it so replace command }
    doalias:= true;
    strcpy(cmd,addr(als^[p+strlen(addr(als^[p]))+1]))
  end
  else                                            { not found -- leave command }
    doalias:= false;
  if strlen(cmd)+strlen(wrk)>cmdsiz-1 then begin
    writeln('alias: alias substitution too large for work area');
    cmd^[0]:= null
  end
  else
    strcat(cmd,wrk);                              { rebuild command area and drop out }
end;

{ this procedure is called after all alias translating in order to insert
  variables into the command line }

procedure expand;
var p,q : integer;
begin
  p:= strlen(cmd)-1;
  while p>=0 do begin
    if cmd^[p]='$' then begin
      q:= p+1;
      while cmd^[q] in (alphanumeric+['*']) do
        q:= q+1;
      strcpy(wrk,addr(cmd^[q]));
      cmd^[q]:= null;
      strcpy(addr(cmd^[p]),getenv(addr(cmd^[p+1])));
      if strlen(cmd)+strlen(wrk)>cmdsiz-1 then begin
        writeln('expand: variable expansion is too large for work area');
        cmd^[0]:= null
      end
      else
        strcat(addr(cmd^[p]),wrk)
    end;
    p:= p-1
  end
end;

{ this function attempts to execute a program using the DOS exec function.
  it passes the default FCB's used by psh to the other program because I
  am to lazy to actually build real ones like COMMAND.COM does.  never
  had a problem with it, so why not. }

function exec(cmd,arg : str) : integer;
var fcb : array[1..2,0..15] of char;
    a,
    b,
    c   : integer;
    ext : boolean;
begin
  errno:= 0;

{ because many programs seem to be brain-dead and actually EXPECT you to
  give them formatted FCB's for the first 2 parms, this section does FCB
  formatting.  Example programs that require this are chkdsk and edlin.
  This takes care of the problem.  Stupid programmers should do it the
  *real* way and save me a whole lot of work.  Note that this doesn't
  correctly handle non-chars, but I don't care because those programs
  that look for switches MUST do it the right way.  also note that the
  whole deal has to get converted to uppercase.  stupid MS-DOS! }

  for a:= 1 to 2 do begin       { initialize FCB's }
    fcb[a,0]:= chr(0);
    for b:= 1 to 11 do
      fcb[a,b]:= ' ';
    for b:= 12 to 15 do
      fcb[a,b]:= chr(0)
  end;
  a:= 0;                        { pointer within arg line }
  b:= 1;                        { fcb number }
  while b<3 do begin            { loop for all fcbs }
    if (arg^[a+1]=':') and (arg^[a]<>null) then begin { set drive }
      fcb[b,0]:= chr(ord(upcase(arg^[a]))-ord('@'));
      a:= a+2
    end;
    c:= 1;                      { pointer within fcb }
    ext:= false;

{ args are delimited bye a trailing colon, meaning reserved device, an arg
  break [ie space] or the end of the command line. }

    while (not (arg^[a] in argbreak)) and (arg^[a]<>':') and (arg^[a]<>null) do begin
      if arg^[a]='.' then begin { look for extension delimiter }
        if not ext then begin   { only one of 'em allowed! }
          ext:= true;
          c:= 9
        end
        else
          c:= 12                { if more than 1 '.', ignore everything }
      end
      else if ((c<9) and (not ext)) or ((c<12) and ext) then begin { set name }
        fcb[b,c]:= upcase(arg^[a]);
        c:= c+1
      end;
      a:= a+1
    end;
    while arg^[a] in argbreak do { skip to next arg }
      a:= a+1;
    b:= b+1
  end;

{ here we set up the parameter string.  this is trickier than it should be
  because DOS only allows up to 127 chars in the parm line.  in addition,
  many DOS programs REQUIRE a leading space, but only if there are arguments
  on the command line.  this is brain-dead, but so is DOS to begin with.
  two such programs are edlin and chkdsk.  this will correctly handle them. }

  if strlen(arg)=0 then
    params[0]:= #0
  else begin
    params[1]:= ' ';
    strncpy(addr(params[2]),arg,126); { duplicate beginning of arg string }
    if strlen(arg)>126 then           { string too long, truncate it }
      params[0]:= #127
    else
      params[0]:= chr(strlen(arg)+1)  { adjust len+1 for leading space }
  end;

{ now that all that stuff is set up, set up the parameter block for the
  exec function and try it }

  parm.envseg:= seg(env^);      { get pointer to our environment }
  parm.argseg:= seg(params);    { get pointer to parameter string }
  parm.argofs:= ofs(params);
  parm.fcb1seg:= seg(fcb[1]);   { get pointers to FCB's }
  parm.fcb1ofs:= ofs(fcb[1]);
  parm.fcb2seg:= seg(fcb[2]);
  parm.fcb2ofs:= ofs(fcb[2]);
  r.ax:= $4B00;                 { tell DOS what function }
  r.ds:= seg(cmd^);             { tell DOS what command to try }
  r.dx:= ofs(cmd^);
  r.es:= seg(parm);             { tell DOS where our parameter table is }
  r.bx:= ofs(parm);
  msdos(r);                     { try it }
  if (r.flags and 1)=1 then begin { we got an error }
    if r.ax<>0 then begin
      if r.ax=3 then            { treat path errors as "not found" }
        errno:= 2
      else
        errno:= r.ax;           { weird errors return as if they had been run }
      if errno=2 then           { return -1 only if not found }
        exec:= -1
      else
        exec:= 0
    end
  end
  else
    exec:= 0;
end;

{ really this doesn't do a fork because brain-dead DOS won't do multi-
  processing.  it searches the path for the executable until it finds one. }

function fork(cmd,arg : str) : integer;
var path   : str;
    found  : boolean;
    excmd  : str;
    p,
    q      : integer;
    f      : file;
begin
  excmd:= wrk;                           { use work area to store command name }
  found:= false;                         { haven't found anything }
  if (strsrch(cmd,strcnv('\'))>=0)
     or (strsrch(cmd,strcnv(':'))>=0) then begin { if a path is given then just do it }
    strcpy(excmd,cmd);
    strcat(excmd,strcnv('.com'));        { try it as a .COM file }
    found:= exec(excmd,arg)=0;
    if not found then begin              { not .COM so }
      strcpy(excmd,cmd);
      strcat(excmd,strcnv('.exe'));      { try it as a .EXE file }
      found:= exec(excmd,arg)=0
    end
  end
  else begin
    path:= getenv(strcnv('path'));       { get current path }
    if path^[0]=null then begin          { no path -- exit out }
      writeln('no path');
      exit
    end;
    while (not found) and (path^[0]<>null) do begin { keep looking down path }
      p:= strsrch(path,strcnv(';'));     { find end of this entry }
      if p=(-1) then                     { path entries end in ";" }
        p:= strlen(path);                { no ";" so use whole string }
      strncpy(excmd,path,p);             { copy current path entry }
      excmd^[p]:= null;                  { end it in a null }
      if (excmd^[strlen(excmd)-1]<>'\') then
        strcat(excmd,strcnv('\'));       { add the stupid slash if necessary }
      strcat(excmd,cmd);                 { put in the command }
      q:= strlen(excmd);                 { save end of string for later }
      strcat(excmd,strcnv('.com'));      { look for .COM file }
      found:= exec(excmd,arg)=0;
      if not found then begin
        strcpy(addr(excmd^[q]),strcnv('.exe')); { look for .EXE }
        found:= exec(excmd,arg)=0
      end;
      if not found then begin
        strcpy(addr(excmd^[q]),strcnv('.sh')); { look for .SH }
        assign(f,tpstr(excmd));
        reset(f);
        if ioresult=0 then begin
          found:= true;
          close(f);
          found:= true;
          q:= strlen(excmd)+1;
          if strlen(excmd)>cmdsiz-(strlen(excmd)+strlen(arg)+1) then begin
            writeln('exec: too many arguments (work space full)');
            errno:= 0
          end
          else begin
            strcpy(addr(excmd^[q]),excmd);              { duplicate command }
            strcpy(excmd,strcnv(' /s '));               { prepend /s switch }
            strcat(excmd,addr(excmd^[q]));
            strcat(excmd,strcnv(' '));
            strcat(excmd,arg);                          { add arguments }
            if exec(getenv(strcnv('shell')),excmd)<>0 then begin
              writeln('psh: could not find psh executable (possibly "shell" variable undefined)');
              errno:= 0
            end;
            excmd:= wrk { restore work address }
          end
        end
      end;
      if not found then                  { not found so try next path entry }
        if path^[p]=null then
          path:= addr(path^[p])
        else
          path:= addr(path^[p+1])
    end
  end;
  if (not found) or (errno<>0) then      { if we couldn't find anything or had an error }
    fork:= -1                            { but found something, tell main program }
  else begin
    strcpy(addr(parm),strcnv('status=')); { get child status }
    strcat(addr(parm),itoa(wait));        { parm just happens to be a  }
    envset(addr(parm));                   { big enough area to stuff   }
    fork:= 0                              { the value in, so it's used }
  end
end;

function hstnum(n : integer) : str;
var a,p : integer;
begin
  p:= 0;
  a:= hstbeg;
  while (a<n) and (hst^[p]<>null) do begin
    while hst^[p]<>null do
      p:= p+1;
    p:= p+1;
    a:= a+1
  end;
  hstnum:= addr(hst^[p])
end;

{ this looks for edit stuff on the line and replaces as necessary from
  the history }

procedure edline;
var p,q,r : integer;
    s     : str;
    done,            { "done" indicates that the edit was successful }
    found,           { found a match }
    err   : boolean; { encountered an error }
begin
  p:= strlen(cmd)-1;
  done:= false;
  while p>=0 do begin
    found:= false;
    err:= false;
    case cmd^[p] of
      '!' : begin
        if (p>0) and (cmd^[p-1]='!') then begin
          strcpy(wrk,addr(cmd^[p+1]));
          strcpy(addr(cmd^[p-1]),hstnum(hstend-1));
          strcat(addr(cmd^[p-1]),wrk);
          found:= true
        end
        else begin
          q:= p+1;
          while cmd^[q] in alphanumeric do
            q:= q+1;
          strcpy(wrk,addr(cmd^[q]));
          cmd^[q]:= null;
          if (atoi(addr(cmd^[p+1]))>=hstbeg) and (atoi(addr(cmd^[p+1]))<hstend) then begin
            found:= true;
            strcpy(addr(cmd^[p]),hstnum(atoi(addr(cmd^[p+1]))))
          end
          else begin
            r:= hstend-1;
            while (r>=hstbeg) and (not found) do begin
              if strncmp(addr(cmd^[p+1]),hstnum(r),strlen(addr(cmd^[p+1])))=0 then begin
                strcpy(addr(cmd^[p]),hstnum(r));
                found:= true
              end;
              r:= r-1
            end
          end;
          if found then
            strcat(addr(cmd^[p]),wrk)
          else begin
            err:= true;
            putstr(addr(cmd^[p+1]));
            writeln(': event not found')
          end
        end
      end

{ I was going to implement carat editing, but this is too much of a pain }

    end;
    if err then begin
      found:= false; { make sure we don't think we found anything }
      done:= false;  { cancels any previous matches and bombs out }
      p:= 0;
      cmd^[0]:= null;
      s:= hstnum(hstend);
      packarray(s);
      hstend:= hstend-1
    end;
    if found then
      done:= true;  { tell it that we had a match }
    p:= p-1
  end;
  if done then begin
    s:= hstnum(hstend);
    initarray(s);   { why do anything complicated?  truncate it here }
    addarray(s,cmd);
    putstr(cmd);
    writeln
  end
end;

{ this returns true if the current command is one of the single line commands
  (commands which MUST use the rest of the line, like 'set' because path needs
  the silly semicolons)}

function singlecmd(s : str) : boolean;
var p   : integer;
begin
  p:= 0;
  while (not ((s^[p] in argbreak) or (s^[p]=';') or (s^[p]=null))) and (p<10) do begin
    wrk^[p]:= s^[p];
    p:= p+1;
  end;
  wrk^[p]:= null;
  singlecmd:= (strcmp(wrk,strcnv('set'))=0)
           or (strcmp(wrk,strcnv('source'))=0)
           or (strcmp(wrk,strcnv('alias'))=0)
           or (strcmp(wrk,strcnv('echo'))=0)
           or (strcmp(wrk,strcnv('#'))=0)
end;

{ this procedure works with alias to handle command line parsing.  it:
    1) isolates the current command from the rest of the arguments/
       commands on the current line
    2) calls doalias to rebuild the command line
    3) loops until alias makes no changes
    4) parses out command and arguments from remaining stuff }

procedure parse;
var p : integer;
begin

{ loop to handle multiple aliases }

  repeat
    p:= 0;
    while (cmd^[p]<>null) and (cmd^[p] in argbreak) do
      p:= p+1;                                { strip off leading spaces, etc }
    command:= addr(cmd^[p]);
    while not ((cmd^[p] in argbreak) or (cmd^[p]=';')
       or (cmd^[p]=null)) do                  { skip to end of command }
      p:= p+1;
    strcpy(wrk,addr(cmd^[p]));                { save args, etc, in wrk }
    cmd^[p]:= null;                           { terminate command }
    strcpy(cmd,command)                       { flush command in cmd area }
  until not doalias;                          { try to find an alias }

{ now break up the command and drop back }

  p:= 0;
  command:= cmd;                              { set command pointer }
  if singlecmd(command) then begin            { check for while line commands }
    while not ((cmd^[p] in argbreak) or (cmd^[p]=';')
       or (cmd^[p]=null)) do                  { skip to end of command }
      p:= p+1;
    if cmd^[p]=null then                      { no args so set args to null }
      arg:= addr(cmd^[p])
    else                                      { set arg pointer }
      arg:= addr(cmd^[p+1]);
    cmd^[p]:= null;                           { terminate command }
    stuff:= addr(cmd^[p])                     { no trailing stuff for this command }
  end
  else begin                                  { this is not a while line command }
    while not ((cmd^[p] in argbreak) or (cmd^[p]=';')
       or (cmd^[p]='|') or (cmd^[p]=null)) do { skip to end of command }
      p:= p+1;
    if (cmd^[p]=';') or (cmd^[p]=null) or (cmd^[p]='|') then begin { look for terminating mark }
      if cmd^[p]='|' then
        topipe:= true                         { tell everyone we're using a pipe }
      else
        topipe:= false;
      if (cmd^[p]=';') or (cmd^[p]='|') then  { we have another command coming }
        stuff:= addr(cmd^[p+1])               { so set pointer }
      else                                    { we are at end of command string }
        stuff:= addr(cmd^[p]);                { so no trailing stuff }
      cmd^[p]:= null;                         { terminate command }
      arg:= addr(cmd^[p]);                    { no arguments }
    end
    else begin                                { no terminating mark -- we have args }
      cmd^[p]:= null;
      p:= p+1;
      while (cmd^[p]<>null) and (cmd^[p] in argbreak) and (cmd^[p]<>';') do
        p:= p+1;                              { skip stuff until not a break char }
      arg:= addr(cmd^[p]);                    { set arg pointer to arg string }
      while (cmd^[p]<>';') and (cmd^[p]<>null) do { look for end of command }
        p:= p+1;
      if cmd^[p]=';' then                     { have another cmd so set pointer }
        stuff:= addr(cmd^[p+1])
      else
        stuff:= addr(cmd^[p]);                { no trailing string }
      cmd^[p]:= null                          { terminate command }
    end
  end
end;

{ this procedure sets the 'cwd' variable }

procedure setcwd;
var cwd : array[0..71] of char; { 64 byte area+7+null }
begin
  r.ax:= $1900;
  msdos(r);
  strcpy(addr(cwd),strcnv('cwd='));
  cwd[4]:= chr(ord('A')+lo(r.ax));
  cwd[5]:= ':';
  cwd[6]:= '\';
  r.ax:= $4700;
  r.ds:= seg(cwd[7]);
  r.si:= ofs(cwd[7]);
  r.dx:= 0;
  msdos(r);
  if (r.flags and 1)=1 then
    cwd[4]:= null;
  envset(addr(cwd));
end;

(*
{ this procedure sets up file handles for piping and redirection }

procedure redirect(args : str);
var a,b,c,h : integer;
begin
  if fnames[0]<>nil then                       { tell normio to munch file }
    frompipe:= true
  else
    frompipe:= false;
  if topipe then begin

{ get temp file to put stuff in }

    strcpy(wrk,getenv(strcnv('pipedir')));
    if wrk^[strlen(wrk)]<>'\' then
      strcat(wrk,strcnv('\'));
    r.ax:= $5A00;
    r.cx:= 0;                                 { attrib is just norm file }
    r.ds:= seg(wrk^);
    r.dx:= ofs(wrk^);
    msdos(r);
    fnames[1]:= malloc(strlen(addr(mem[r.ds:r.dx]))+1);
    strcpy(fnames[1],addr(mem[r.ds:r.dx]))    { save file name }
  end;

{ parse argument list and strip out redirections }

  a:= 0;
  while args^[a]<>null do begin
    if (args^[a]='<') or (args^[a]='>') then begin { got one }
      if args^[a-1] in ['1'..'4'] then             { redirect specific handle? }
        if (a=1) or ((a>1) and (args^[a-2] in argbreak)) then
          h:= ord(args^[a-1])-ord('0')
        else if args^[a]='<' then                  { default file handles }
          h:= 0                                    { stdin }
        else
          h:= 1;                                   { stdout }
      if fnames[h]<>nil then begin                 { trap idiots }
        writeln('multiple redirection');
        a:= 0;
        args^[0]:= null
      end
      else begin                                   { parse out filename }
        b:= a+1;
        while (args^[b] in argbreak) and (args^[b]<>'<')
           and (args^[b]<>'>') do                  { find start of fname }
          b:= b+1;
        c:= b;
        while not (args^[c] in argbreak) do        { find end of fname }
          c:= c+1;
        if c=b then begin                          { more idiot trapping }
          writeln('null redirection');
          a:= 0;
          args^[0]:= null
        end
        else begin                                 { save filename }
          fnames[h]:= malloc(c-b+2);
          strncpy(fnames[h],addr(args^[b]),c-b+1);
          fnames[h]^[c-b+1]:= null;
          a:= a+1
        end
      end
    end
    else
      a:= a+1;
  end;

{ open all files for redirection and save file handles }

  for a:= 0 to 4 do
    if fnames[a]<>nil then begin
      handles[a]:= dup(a);           { save file handle }
      if a=0 then
        f:= open(fnames[a],'r')     { open input file for read }
      else
        f:= open(fnames[a],'w');    { open output file for write }
      if f=(-1) then begin          { big problems!  crash out }
        putstr(fnames[a]);
        writeln(': could not open file');
        for b:= 1 to a do begin
          dup2(b,handles[b]);
          free(fnames[b]);
          fnames[b]:= nil
        end;
        args^[0]:= null;
        exit
      end;
      dup2(a,f);                    { dup into file handle }
      close(f)                      { forget old file handle to this file }
    end
end;
*)

(*{ this procedure resets file handles following pipes or redirection }

procedure normio;
begin
  if brkpipe then begin            { broken pipe, so wipe out file }
    unlink(fnames[1]);
    topipe:= false
  end;

{ close all file handles we were using and reassign old ones }

  for a:= 0 to 4 do begin
    if fnames[a]<>nil then
      dup2(a,handles[a]);           { automatically closes the file while duping }
    if (a=0) and frompipe then
      unlink(fnames[0]);           { erase the temp file }

{ free up memory used to save file names.  also hold on to file name used
  for pipe output if necessary and set stdin file name to point to this.
  this causes redirect to assume input redirection from this file
  automagically.  saves lots of work.  note that file handle 0 (stdin)
  will have been closed and deallocated before stdout is processed.  this
  allows you to just swap in the file name from stdout without having to
  worry about deallocating it and closing it and all that complicated stuff }

    if (a=1) and topipe then begin
      fnames[0]:= fnames[1];       { save pipe file name }
      fnames[1]:= nil              { don't wipe out memory area }
    end;
    if fnames[a]<>nil then
      free(fnames[a])
  end
end;
*)
procedure docommand;
var command,args,stuff : str;
    nullcmd : boolean;
    cwd : array[0..68] of char;
begin
  nullcmd:= true;
  expand;   { expand shell variables }
  edline;   { do line editing -- previous line replacement/substitution }
  brkpipe:= false;
  repeat
    parse(command,args,stuff);
(*    if brkpipe and topipe then { stop execution of everything along a }
      nullcmd:= true           { pipe if the pipe gets broken somehow }
    else begin
      brkpipe:= false;
      redirect(args);          { look for redirection and stuff }
      nullcmd:= nullcmd and (strcmp(command,strcnv(''))=0)
    end;
    if nullcmd and (topipe or frompipe) then
      brkpipe:= true;
*)
    nullcmd:= nullcmd and (strcmp(command,strcnv(''))=0); { necessary in v1.4 until pipes work }

{ process built in commands }

    if nullcmd then begin end                                      { null }
    else if strncmp(command,strcnv('#'),1)=0 then begin end        { comment }
    else if strcmp(command,strcnv('alias'))=0 then alias(args)     { alias }
    else if strcmp(command,strcnv('cd'))=0 then chdir(args)        { chdir }
    else if strcmp(command,strcnv('chdir'))=0 then chdir(args)
    else if strcmp(command,strcnv('echo'))=0 then echo(args)       { echo }
    else if strcmp(command,strcnv('exit'))=0 then doexit           { exit }
    else if strcmp(command,strcnv('history'))=0 then history       { history }
    else if strcmp(command,strcnv('logout'))=0 then logout         { logout }
    else if strcmp(command,strcnv('set'))=0 then envset(args)      { set }
    else if strcmp(command,strcnv('shstat'))=0 then shstat         { shstat }
    else if strcmp(command,strcnv('source'))=0 then source(args)   { source }
    else if strcmp(command,strcnv('unalias'))=0 then unalias(args) { unalias }
    else if strcmp(command,strcnv('ver'))=0 then writeln(version)  { ver }

{ not a built-in, so try to run it. }

    else
      if fork(command,args)<0 then begin
        putstr(command);
        write(': ');
        case errno of
          2 :
            writeln('not found');
          8 :
            writeln('not enough memory to execute');
          10 :
            writeln('bad environment (cannot execute)');
          11 :
            writeln('bad file format')
          else
            writeln('unknown error while executing (',errno,')')
        end;
(*        if topipe then begin
          writeln('pipe broken');
          brkpipe:= true;
          topipe:= false
        end
*)      end;

(*    normio;                    { fix I/O handles from redirection }
*)
    setcwd;

{ flush remaining stuff in command line }

    if done then
      cmd^[0]:= null
    else if cmd^[0]<>null then { this traps problems when we need the command }
      strcpy(cmd,stuff)        { line area for a buffer, as with alias }
  until cmd^[0]=null;
  if nullcmd and interactive then begin { we only save interactive commands, }
    initarray(hstnum(hstend));          { so don't trash 'em if it's not     }
    hstend:= hstend-1                   { through the keyboard right now.    }
  end
end;

procedure getstr(s : str);
var done : boolean;
    a,
    h,
    l,
    p    : integer;
    c    : char;
    t    : str;

  procedure fwd;
  begin
    if s^[p]<' ' then
      write('^',chr(ord(s^[p])+ord('@')))
    else
      write(s^[p]);
    p:= p+1
  end;

  procedure bwd;
  begin
    if p>0 then begin
      p:= p-1;
      if s^[p]<' ' then
        write(^H);
      write(^H)
    end
  end;

  procedure delchar;
  var a : integer;
  begin
    if p>0 then begin
      bwd;                 { back up to prev char }
      a:= p;               { save char ptr }
      l:= l-1;             { line is now 1 shorter }
      while p<l do begin   { move command line following cursor }
        s^[p]:= s^[p+1];
        fwd
      end;
      write('  ',^H,^H);   { blank out last char(s) }
      p:= l;               { set ptr to last char }
      while p>a do         { backup to cursor position }
        bwd
    end
  end;

begin
  done:= false;
  p:= 0;
  l:= 0;
  t:= hstnum(hstend);
  h:= hstend+1;
  repeat
    if p>l then l:= l+1;
    read(kbd,c);
    case c of
      ^J,^M : begin
        s^[l]:= null;
        done:= true;
        writeln
      end;
      ^H,#127 :
        delchar;
      ^A :
        while p>0 do
          bwd;
      ^B :
        bwd;
      ^D :
        if (l>0) and (p<l) then begin
          fwd;
          delchar
        end;
      ^E :
        while p<l do
          fwd;
      ^F :
        if p<l then
          fwd;
      ^K : begin
        a:= p;
        while p<l do
          fwd;
        while l>a do
          delchar
      end;
      ^R : begin
        writeln('^R');
        a:= p;
        p:= 0;
        while p<l do
          fwd;
        while p>a do
          bwd
      end;
      ^U :
        while p>0 do
          delchar;
      ^W : begin
        while (p>0) and (s^[p]<>' ') do
          delchar;
        while (s^[p]=' ') do
          delchar
      end;
      ^[ :
        if keypressed then begin
          read(kbd,c);
          case c of
            ';' : { f1 }
              if strlen(t)>p then begin
                s^[p]:= t^[p];
                fwd;
                p:= p+1
              end;
            '=' : begin { f3 }
              while strlen(t)>p do begin
                s^[p]:= t^[p];
                fwd
              end;
              l:= strlen(t)
            end;
            'M' : { crsr rt }
              if p<l then
                fwd;
            'K' : { crsr lt }
              bwd;
            'H' : { crsr up }
              if h>hstbeg then begin
                h:= h-1;
                while p>0 do
                  delchar;
                strcpy(s,hstnum(h));
                l:= strlen(s);
                while p<l do
                  fwd
              end;
            'P' : { crsr dn }
              if h<hstend then begin
                h:= h+1;
                while p>0 do
                  delchar;
                strcpy(s,hstnum(h));
                l:= strlen(s);
                while p<l do
                  fwd
              end;
            'G' : { home }
              while p>0 do
                bwd;
            'O' : { end }
              while p<l do
                fwd;
          end;
        end
        else
          while p>0 do
            delchar;
      else
        if p<cmdsiz then begin
          s^[p]:= c;
          fwd
        end
        else
          write(^G)
    end
  until done
end;

{ DOS manual says that we should free any memory allocated during runtime,
  so this does it.  note that testing showed no problems if you forget to
  do this, but we'll play it by the book.  this function should be called
  before an errout() or any other program exit. }

procedure freemem;
begin
  if env<>nil then
    free(env);
  if als<>nil then
    free(als);
  if wrk<>nil then
    free(wrk);
  if cmd<>nil then
    free(cmd);
  if hst<>nil then
    free(hst)
end;

{

}
{***********************************************************************}
{ This section contains main program loops and things }

begin
  done:= true;
  interactive:= false;
  srcdepth:= 0;
  env:= nil; { calls to errout() need these nil if not allocated yet }
  als:= nil;
  wrk:= nil;
  cmd:= nil;
  hst:= nil;
  env:= malloc(envsiz);
  if env=nil then begin
    writeln('psh: malloc: could not allocate memory for environment: ',envsiz);
    errout(4)
  end;
  if (paramstr(1)='/p') or (paramstr(1)='/P') then begin
    initarray(env);         { start clean if this is a permanent shell }
    perm:= true
  end
  else begin
    cpyarray(env,addr(mem[memw[cseg:$2C]:0])); { copy parent's environment }
    perm:= false
  end;
  wrk:= getenv(strcnv('prompt'));
  if wrk^[0]=null then
    addarray(env,strcnv('prompt=% ')); { default prompt }
  als:= malloc(alssiz);
  if als=nil then begin
    writeln('psh: malloc: could not allocate memory for aliases');
    errout(4)
  end;
  cmd:= malloc(cmdsiz);
  if cmd=nil then begin
    writeln('psh: malloc: could not allocate memory for command line');
    errout(4)
  end;
  wrk:= malloc(cmdsiz);
  if wrk=nil then begin
    writeln('psh: malloc: could not allocate memory for work area');
    errout(4)
  end;
  hst:= malloc(hstsiz);
  if hst=nil then begin
    writeln('psh: malloc: could not allocate memory for history');
    errout(4)
  end;
  initarray(als);
  initarray(hst);
  setcwd;
  strcpy(wrk,getenv(strcnv('home')));
  if wrk^[0]<>null then
    if wrk^[strlen(wrk)]<>'\' then
      strcat(wrk,strcnv('\'));
  strcat(wrk,strcnv('pshrc.sh'));
  source(wrk);                { try to read in a defaults file }
  if perm then
    source(strcnv('login.sh'));
  done:= false;               { we're not done yet! }

{ this shell was invoked to run a shell script }

  if paramstr(1)='/s' then begin
    if paramstr(2)='' then begin
      writeln('psh: no script name specified');
      errout(4)
    end
    else begin                           { set up environment entries }
      strcpy(cmd,strcnv('0='));          { script name }
      strcat(cmd,strcnv(paramstr(2)));
      envset(cmd);
      strcpy(cmd,strcnv('*='));          { build list of all args }
      strcat(cmd,strcnv(paramstr(p)));
      for p:= 3 to paramcount do begin
        strcat(cmd,strcnv(' '));
        strcat(cmd,strcnv(paramstr(p)))
      end;
      envset(cmd);
      for p:= 3 to paramcount do begin   { build entry for each arg }
        strcpy(cmd,itoa(p-2));
        strcat(cmd,strcnv('='));
        strcat(cmd,strcnv(paramstr(p)));
        envset(cmd)
      end;
      envset(cmd);                       { set list of all args in environment }
      source(strcnv(paramstr(2)));
    end;
    errout(0)
  end

{ this shell was invoked to run a command }

  else if paramstr(1)='/c' then begin
    cmd^[0]:= null;
    for p:= 2 to paramcount do
      strcat(cmd,strcnv(paramstr(p)));
    docommand
  end
  else begin
    writeln(version);
    hstbeg:= 1;
    hstend:= 0;
    repeat
      interactive:= true;
      repeat
        putstr(getenv(strcnv('prompt')));
        getstr(cmd)
      until strlen(cmd)>0;
      while freearray(hst)<strlen(cmd)+1 do begin { pack up the history when full }
        packarray(hst);
        hstbeg:= hstbeg+1;
        if hstbeg>30000 then begin { rollover just in case }
          hstend:= hstend-hstbeg;
          hstbeg:= 1
        end
      end;
      addarray(hst,cmd);
      hstend:= hstend+1;
      docommand;
    until done                  { keep it up until we yell stop }
  end;
  freemem
end.
