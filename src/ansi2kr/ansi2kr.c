
/*
  asu ansi2kr Ver 1.3 (C) Masaki Oba 2017
  9 Apr 2017
  http://www.nabeta.tk
  admin@nabeta.tk
  ANSI C to K&R C converter for BDS C , MSX-C and other K&R C Coompiler
  2-Clause BSD License

  conver ANSI C to BDS C
  > ansi2kr inputfile outputfile

  conver ANSI C to MSX-C
  > ansi2kr -m inputfile outputfile
        or
  > ansi2kr inputfile outputfile -m
        or
  > ansi2kr -vup inputfile outputfile
        or
  > ansi2kr -v -u -p inputfile outputfile
        or
  > ansi2kr inputfile outputfile -v -u -p
*/
/* if compile this source by C++ Builder on Windows
  (target Windows)
  > bcc32 ansi2kr.c
*/

/* if compile this source by BDS C
  "ansi2kr.exe(or .com) ansi2kr.c temp.c(output filename)" +
#define CPM +
  must create blank include header files. or delete include line.

  Compile by BDS C on CP/M
  (target CP/M and MSX-DOS and MSX-DOS2)
  > ansi2kr ansi2kr.c temp.c
  > cc temp.c
  > clink temp
  > pip ansi2kr.com=temp.com

  Compile by BDS C on MSX-DOS
  (target CP/M and MSX-DOS and MSX-DOS2)
  > ansi2kr ansi2kr.c temp.c
  > cc temp.c
  > clink temp
  > copy temp.com ansi2kr.com

  Compile by BDS C on Windows with CP/M program EXEcutor for Win32
  (target CP/M and MSX-DOS and MSX-DOS2)
  > ansi2kr.exe ansi2kr.c temp.c
  > cpm cc temp.c
  > cpm clink temp
  > copy temp.com ansi2kr.com

*/
/* if compile this source by MSX-C Ver 1.2
  Important!!! cf "-t" option and ansi2kr "-m" option.
  -m or -p option output "#pragma noregalo" to source code.
#define MSXC +

  > ansi2kr -m ansi2kr.c temp.c
  > cf -t temp
  > cg -k temp
  > m80 =temp/z
  > l80 ck,temp,clib/s,crun/s,cend, temp/n/e
  > copy temp.com ansi2kr.com
*/
/*This source file not support compile by HI-TECH C Ver 3.90(CP/M)
  because HI-TECH C(CP/M) can not compile big source file.
  if use HI-TECH C then must separate this source file.
*/

#include <stdio.h>
#ifdef __STDC__
/* BDS C must create blank header files or delete this 3lines */
#include <stdlib.h>
#include <string.h>
#endif
#include <ctype.h>

#ifdef MSXC
#define OS "MSX-DOS2"
#else
#ifdef CPM
#define OS "CP/M,MSX-DOS"
#else
#define OS "Windows"
#endif
#endif
#define VERSION "1.3"
#define NENGOU "2017"


#define CPMEOF 0x1a	/* CP/M End-of-text-file marker */

#define MAXBLEN 2000
#define LINEMAX 500
#define WORDLENMAX 100

int Noregalof;	 /* -p option.for MSX-C.
                "put #pragama noregalo" in source code */
int Voidf;	/* -v option. delete "void" */
int Castf;	/* -u option. cast on */
int Cppcmmtf;	/* C++ comment mode on */
int Cpmeoff; /* output CP/M EOF 0x1a(CP/M file end mark) */
FILE *Outp;
int Outf;
/*

*/
void setumei();

/* BDS C does'n t have strncmp */
/* strncmp compatible? function */
int str2ncmp(s1,s2,n)char *s1;char *s2;int n;
{
  int i;
  for(i=0;i<n;++i){
    if(*s1 != *s2){
      return *s1 - *s2;
    }
    ++s1;
    ++s2;
  }
  return 0;
}

/* BDS C,HI-TECH C,MSX-C does'nt have strstr() */
/* strstr compatible? function */
char *strst2(s,u)char *s;char *u;
{
  int i,j,len,start,awanai;
  i = 0;

  if(s == 0 || u == 0){
    return 0;
  }
  len = strlen(u);
  /* *lenm = len; */
  if(len <= 0){
    return 0;
  }
  if(s[0] == 0){
    return 0;
  }
  start = 0;
  while(s[i] != 0){
    if(s[i] == u[0]){
      /* sentou icchi */
      int a;
      start = i;
      a = 1;
      awanai = 0;
      for(j=1;a < len;j++,a++){
        if(s[start + a] != u[j]){
          start = 0;
          awanai = 1;
          break;
        }
      }
      if(awanai == 0){
        break;
      }
    }
    ++i;
  }
  if(start == 0){
    return 0;
  }
  return &s[start];
}

/* (Japanese)char mojiretsu kara shitei moji wo
kensaku shite ichi wo kaesu.
mitsukaranai bawai wa -1 wo kaesu.
Like find or strstr.
if not found return -1; */
int srch_a(s,u,lenm)char *s;char *u;int *lenm;
{
  int i,j,len,start,awanai;
  i = 0;

  if(s == 0 || u == 0){
    return -1;
  }
  len = strlen(u);
  *lenm = len;
  if(len <= 0){
    return -1;
  }
  if(s[0] == 0){
    return -1;
  }
  start = -1;
  while(s[i] != 0){
    if(s[i] == u[0]){
      /* sentou icchi */
      int a;
      start = i;
      a = 1;
      awanai = 0;
      for(j=1;a < len;j++,a++){
        if(s[start + a] != u[j]){
          start = -1;
          awanai = 1;
          break;
        }
      }
      if(awanai == 0){
        break;
      }
    }
    ++i;
  }
  return start;
}

/*
replace character string.
be carefull Overwrite source string.
replase all char *
change s to m in g.
overwrite g.
s/src/dst/g
  char str[1000];
  strcpy(str,"aABCbbbABCccc");
  repl_a_all(str,"ABC","def");
  printf(str);
  adefbbbdefccc
*/
void repl_a_all(str,m,g)char *str;char *m;char *g;
{
  int a,mlen,slen;
  int glen;
  char go[MAXBLEN+50];
  glen = strlen(g);


  while(1){
    go[0] = 0;
    a = srch_a(str,m,&mlen);
    if(a >= 0){
      slen = strlen(str);
      if(slen > MAXBLEN){
        str[MAXBLEN] = 0;
      }
      if(a + mlen < slen){
        strcpy(go,&str[a + mlen]);
      }
      strcpy(&str[a],g);
      strcpy(&str[a + glen],go);
    }
    else{
      break;
    }
  }
}

int iscmoji(a)char a;
{
  if(a >= 'a' && a <= 'z'){
    return 1;
  }
  if(a >= 'A' && a <= 'Z'){
    return 1;
  }
  if(a >= '0' && a <= '9'){
    return 1;
  }
  if(a == '_'){
    return 1;
  }
  return 0;
}

int iskuuhaku(a)char a;
{
  if(a == ' ' || a == '\t'){
    return 1;
  }
  return 0;
}


/* (Japanese)
char mojiretsu kara zengo ni rinsetsu sita eisuu to _ wo
fukumanai mojiretsu wo kensaku shite ichi wo kaesu.
mitsukaranai bawai wa -1 wo kaesu.
search word.
if not found return -1;
"ancdefeg" search "def" not found.
"anc defeg" search "def" not found.
"ancdef eg" search "def" not found.
"anc def eg" search "def" found.
s/\<src\>/dst/g
*/
int srch_tango_a(s,u,lenm)char *s;char *u;int *lenm;
{
  int i,j,len,start,awanai;
  int a,kkmf,kkgf;

  kkmf = 0;
  kkgf = 0;
  i = 0;

  if(s == 0 || u == 0){
    return -1;
  }
  len = strlen(u);
  *lenm = len;
  if(len <= 0){
    return -1;
  }
  if(s[0] == 0){
    return -1;
  }
  if(u[0] == '('){
    kkmf = 1;
  }
  if(len > 1){
    if(u[len-1] == ')'){
      kkgf = 1;
    }
  }
  while(1){
    start = -1;
    if(s[i] == 0){
      break;
    }
    if(s[i] == u[0]){
      if(i > 0){
        if(s[i-1] == 0x22 /* " */){
          break;
        }
        else
        if(kkmf){
        }
        else
        if(iskuuhaku(s[i-1]) || s[i-1] == '(' ||
          s[i-1] == ',' || s[i-1] == '*' ||
          s[i-1] == '&' || s[i-1] == ';'){
        }
        else{
          ++i;
          continue;
        }
      }
      if(s[i+len] == 0x22 /* " */){
        break;
      }

      if(kkgf){
      }
      else
      if(iskuuhaku(s[i+len]) || s[i+len] ==')' ||
        s[i+len] == ',' || s[i+len] == '*'){
        /* separate syntax for MSX-C heap over fllow */
      }
      else
      if(s[i+len] == '&' || s[i+len] == ';' ||
        s[i+len] == 0 || s[i+len] == '\r' || s[i+len] == '\n'){
      }
      else{
        ++i;
        continue;
      }
      /* sentou icchi */
      start = i;
      a = 1;
      awanai = 0;
      for(j=1;a < len;j++,a++){
        if(s[start + a] != u[j]){
          start = -1;
          awanai = 1;
          break;
        }
      }
      if(awanai == 0){
        break;
      }
    }
    ++i;
  }
  return start;
}


/* replace charcter string
be carefull Overwrite source string.
replase all char *
change s to m in g.
overwrite g.
s/\<src\>/dst/g
  char str[1000];
  strcpy(str,"aABCbbbABCccc");
  repl_a_all(str,"ABC","def");
  printf(str);
  adefbbbABCccc
  strcpy(str,"aABCbbb ABC ccc");
  repl_a_all(str,"ABC","def");
  printf(str);
  adefbbb def ccc
*/
void repl_ta_all(str,m,g)char *str;char *m;char *g;
{
  int a,mlen,slen;
  int glen;
  char go[MAXBLEN+50];
  glen = strlen(g);


  while(1){
    go[0] = 0;
    a = srch_tango_a(str,m,&mlen);
    if(a >= 0){
      slen = strlen(str);
      if(slen > MAXBLEN){
        str[MAXBLEN] = 0;
      }
      if(a + mlen < slen){
        strcpy(go,&str[a + mlen]);
      }
      strcpy(&str[a],g);
      strcpy(&str[a + glen],go);
    }
    else{
      break;
    }
  }
}

#ifdef MSXC
void fputc(c,fpm)char c;FILE *fpm;
{
  if(c == '%'){
    fprintf(fpm,"%%");
  }
  else{
    fprintf(fpm,"%c",c);
  }
}
#endif

/* support '%' and convert */
void puts_p(str)char *str;
{
  repl_ta_all(str,"const","");
  repl_ta_all(str,"unsigned char","char");
  repl_ta_all(str,"short int","int");
  repl_ta_all(str,"unsigned short","unsigned");
  repl_ta_all(str,"unsigned int","unsigned");
  repl_ta_all(str,"short unsigned","unsigned");
  repl_ta_all(str,"short","int");
  repl_ta_all(str,"signed int","int");

  if(Castf == 0){
    repl_ta_all(str,"(char)","");
    repl_ta_all(str,"(char*)","");
    repl_ta_all(str,"(char *)","");
    repl_ta_all(str,"(int)","");
    repl_ta_all(str,"(int*)","");
    repl_ta_all(str,"(int *)","");
    repl_ta_all(str,"(unsigned)","");
    repl_ta_all(str,"(unsigned*)","");
    repl_ta_all(str,"(unsigned *)","");
    repl_ta_all(str,"(FILE*)","");
    repl_ta_all(str,"(FILE *)","");
  }

  while(*str){
    if(Outf){
      fputc(*str,Outp);
    }
    else{
      putchar(*str);
    }
    ++str;
  }
}

/* support '%' */
void putsp2(str)char *str;
{
  while(*str){
    if(Outf){
      fputc(*str,Outp);
    }
    else{
      putchar(*str);
    }
    ++str;
  }
}

void setumei() 
{
  /* setumei hyouji */
  fprintf(stderr,"asu ansi2kr\n");
  fprintf(stderr,"ANSI C declaration to K&R C declaration. for BDS C , MSX-C\n");
  fprintf(stderr,"usage: ansi2kr [-cvupme] inputfile [outputfile]\n");
  fprintf(stderr,"      -c no use C++ coment\n");
  fprintf(stderr,"      -v delete \"void\"\n");
  fprintf(stderr,"      -u use cast.(no cut cast)\n");
  fprintf(stderr,"      -p output \"#pragma noregalo\".(for MSX-C)\n");
  fprintf(stderr,"      -m support MSX-C. same -vup option.\n");
  fprintf(stderr,"      -e no output CP/M EOF 0x1a(CP/M File end mark)\n");
  fprintf(stderr,"\"int function(int a,int b)\" to \"int function(a,b)int a;int b;\"\n");
  fprintf(stderr,"\"//comment\" OK. \"  //comment\" OK. \"xxx //comment\" no support\n");
  fprintf(stderr,"\"unsigned char\" to \"char\"\n");
  fprintf(stderr,"\"unsigned int\" to \"unsigned\"\n");
  fprintf(stderr,"\"unsigned short\" to \"unsigned\"\n");
  fprintf(stderr,"\"const int\" to \"int\"\n");
  fprintf(stderr,"\"signed int\" to \"int\"\n");
  fprintf(stderr,"\"short int\" to \"int\"\n");
  fprintf(stderr,"\n     ansi2kr ver ");
  fprintf(stderr,VERSION);
  fprintf(stderr," for %s",OS);
  fprintf(stderr," (C) Masaki Oba ");
  fprintf(stderr,NENGOU);
  fprintf(stderr,"\n");
  putchar(7);
}

int issentou(a)char a;
{
  if(a >= 'a' && a <= 'z'){
    return 1;
  }
  if(a >= 'A' && a <= 'Z'){
    return 1;
  }
  if(a == '_'){
    return 1;
  }
  return 0;
}


int getword(str,word)char *str;char *word;
{
  int cnt;
  int st;
  *word = 0;
  cnt = 0;
  while(1){
    word[cnt] = *str++;
    if(word[cnt] == 0){
      /* tsuzuki ga nai */
      st = 5;
      break;
    }
    if(word[cnt] == '\r' || word[cnt] == '\n'){
      word[cnt] = 0;
      st = 4;	/* line end */
      break;
    }
    if(word[cnt] == ' ' || word[cnt] == '\t'){
      st = 1;
      break;
    }
    if(word[cnt] == '*'){
      /* pointer? */
      st = 2; /* pointer? '*' */
      break;
    }
    /* check character of C */
    if(iscmoji(word[cnt])==0){
      st = 3;
      break;
    }

    ++cnt;
    if(cnt >= WORDLENMAX){
      st = 0;
      break;
    }
  }
  word[cnt] = 0;
  return st;
}

int yoyakuck(yk)char *yk;
{
  if(strcmp(yk,"for")==0) return 1;
  if(strcmp(yk,"while")==0) return 1;
  if(strcmp(yk,"do")==0) return 1;
  if(strcmp(yk,"break")==0) return 1;
  if(strcmp(yk,"continue")==0) return 1;
  if(strcmp(yk,"switch")==0) return 1;
  if(strcmp(yk,"case")==0) return 1;
  if(strcmp(yk,"if")==0) return 1;
  if(strcmp(yk,"else")==0) return 1;
  if(strcmp(yk,"goto")==0) return 1;
  if(strcmp(yk,"default")==0) return 1;
  if(strcmp(yk,"volatile")==0) return 1;
  if(strcmp(yk,"ragister")==0) return 1;
  if(strcmp(yk,"return")==0) return 1;
  if(strcmp(yk,"enum")==0) return 1;
  if(strcmp(yk,"typedef")==0) return 1;
  if(strcmp(yk,"sizeof")==0) return 1;
  if(strcmp(yk,"static")==0) return 1;
  if(strcmp(yk,"auto")==0) return 1;
  return 0;
}

int katack(yk)char *yk;
{
  if(strcmp(yk,"void")==0) return 2;
  if(strcmp(yk,"int")==0) return 3;
  if(strcmp(yk,"char")==0) return 4;
  if(strcmp(yk,"unsigned")==0) return 5;
  if(strcmp(yk,"float")==0) return 6;
  if(strcmp(yk,"long")==0) return 7;
  if(strcmp(yk,"short")==0) return 8;
  if(strcmp(yk,"double")==0) return 9;
  if(strcmp(yk,"FILE")==0) return 10;
  if(strcmp(yk,"const")==0) return 11;
  if(strcmp(yk,"signed")==0) return 12;
  return 0;
}

void chg_center(str)char *str;
{
  repl_a_all(str,"&"," ");
  repl_a_all(str,"*"," ");
  repl_ta_all(str,"void","");
  repl_ta_all(str,"char","");
  repl_ta_all(str,"int","");
  repl_ta_all(str,"unsigned","");
  repl_ta_all(str,"float","");
  repl_ta_all(str,"long","");
  repl_ta_all(str,"short","");
  repl_ta_all(str,"double","");
  repl_ta_all(str,"FILE","");
  repl_ta_all(str,"const","");
  repl_ta_all(str,"signed","");
  repl_ta_all(str,"struct","");
  repl_ta_all(str,"union","");
  repl_a_all(str," ","");
  repl_ta_all(str,"\t","");
}

/* mode 0:head  1:center */
void chg_kata(str,mode,voidf)char *str;int mode;int voidf;
{
  /* for BDS C and MSX-C */
  if(voidf){
    repl_ta_all(str,"void","");
  }
  else
  if(mode == 1){
    repl_ta_all(str,"void","");
  }
  repl_ta_all(str,"const","");
  repl_ta_all(str,"unsigned char","char");
  repl_ta_all(str,"short int","int");
  repl_ta_all(str,"unsigned short","unsigned");
  repl_ta_all(str,"unsigned int","unsigned");
  repl_ta_all(str,"short unsigned","unsigned");
  repl_ta_all(str,"short","int");
  repl_ta_all(str,"signed int","int");
}

int check_center(s)char *s;
{
  char *b,*t;
  char bak;
  int naf;
  int len;
  naf = 0;
  while(iskuuhaku(*s)){
    ++s;
  }
  if(*s == 0){
    /* "()" */
    /* ANSI C function? */
    return 1;
  }
  if(str2ncmp(s,"void",4) == 0){
    t = s+4;
    while(iskuuhaku(*t)){
      ++t;
    }
    if(*t == 0){
      /* "(void)" */
      /* ANSI C function? */
      return 1;
    }
  }
  if((b = strst2(s,",")) != 0){
    t = b - 1;
    while(iskuuhaku(*t)){
      --t;
    }
    bak = *(t+1);
    *(t+1) = 0;
    if(strst2(s," ") || strst2(s,"\t")){
      /* ANSI C function? */
    }
    else{
      /* not ANSI C function */
      naf = 1;
    }
    *(t+1) = bak;
  }
  else{
    len = strlen(s);
    t = s + len - 1;
    while(iskuuhaku(*t)){
      --t;
    }
    bak = *(t+1);
    *(t+1) = 0;
    if(strst2(s," ") || strst2(s,"\t")){
      /* ANSI C function? */
    }
    else{
      /* not ANSI C function */
      naf = 1;
    }
    *(t+1) = bak;
  }
  if(naf){
    /* not ANSI C function */
    return -1;
  }
  return 0;
}

int ssub2(ac,ep,head,word,buf,wordbuf)int ac;char *ep;char *head;char *word;char *buf;char *wordbuf;
{
  int acbak,lef;
  acbak = ac;
  if(strcmp(word,"extern")==0){
    ep = head + strlen("extern");
    while(*ep== ' ' || *ep == '\t'){
      ++ac;
      ++ep;
    }
    lef = getword(ep,word);
    if(lef != 1 && lef != 2){
      /* not function */
      puts_p(buf);
      return -1;
    }
    ac += strlen(word);
  }

  if(strcmp(word,"signed")==0){
    ep = head + strlen("signed");
    while(*ep== ' ' || *ep == '\t'){
      ++ac;
      ++ep;
    }

    lef = getword(ep,word);
    if(lef != 1 && lef != 2 && lef != 5){
      /* not function */
      return -1;
    }
    if(katack(word) == 0){
      strcpy(word,wordbuf);
      ac = acbak;
    }
    else{
      ac += strlen(word);
    }
  }
  else
  if(strcmp(word,"unsigned")==0){
    ep = head + strlen("unsigned");
    while(*ep== ' ' || *ep == '\t'){
      ++ac;
      ++ep;
    }

    lef = getword(ep,word);
    if(lef != 1 && lef != 2 && lef != 5){
      /* not function */
      puts_p(buf);
      return -1;
    }
    if(katack(word) == 0){
      strcpy(word,wordbuf);
      ac = acbak;
    }
    else{
      ac += strlen(word);
    }
  }

  acbak = ac;

  if(strcmp(word,"long")==0){
    ep = head + strlen("long");
    while(*ep== ' ' || *ep == '\t'){
      ++ac;
      ++ep;
    }

    lef = getword(ep,word);
    if(lef != 1 && lef != 2){
      /* not function */
      puts_p(buf);
      return -1;
    }
    if(katack(word) == 0){
      strcpy(word,wordbuf);
      ac = acbak;
    }
    else{
      ac += strlen(word);
    }
  }
  return ac;
}

/* separate function from syori()
for MSX-C compile error over flow */
int syorisub(head,word,buf,tmpbuf)char *head;char *word;char *buf;char *tmpbuf;
{
  int stunif;
  int lef;
  char *ep;
  int nlen,ac;
  char wordbuf[100];

  stunif = 0; /* struct or union */
  ac = strlen(word);
  strcpy(wordbuf,word);

  ac = ssub2(ac,ep,head,word,buf,wordbuf);

  if(ac < 0){
    return -1;
  }

  if(strcmp(word,"struct")==0){
    stunif = 1;
    ep = head + strlen("struct");
    while(*ep== ' ' || *ep == '\t'){
      ++ac;
      ++ep;
    }
    lef = getword(ep,word);
    if(lef != 1 && lef != 2){
      /* not function */
      puts_p(buf);
      return -1;
    }
    ac += strlen(word);
  }
  else
  if(strcmp(word,"union")==0){
    stunif = 1;
    ep = head + strlen("union");
    while(*ep== ' ' || *ep == '\t'){
      ++ac;
      ++ep;
    }
    lef = getword(ep,word);
    if(lef != 1 && lef != 2){
      /* not function */
      puts_p(buf);
      return -1;
    }
    ac += strlen(word);
  }

  ep = head + ac;
  while(*ep== ' ' || *ep == '\t' || *ep == '*'){
    ++ac;
    ++ep;
  }
  getword(ep,tmpbuf);
  ac += strlen(tmpbuf);
  ep = head + ac;
  while(*ep== ' ' || *ep == '\t'){
    ++ep;
  }
  if(*ep != 0){
    /* not function */
    puts_p(buf);
    return -1;
  }

  if(stunif == 0){
    /* not struct and union */
    if(katack(word) == 0){
      /* not int, not char, not long... */
      puts_p(buf);
      return -1;
    }
  }
  return 0;
}

int syori(str)char *str;
{
  /* "int func_name(int a,int b){" */
  int rt,vdf;
  int lef;
  char *buf;
  int semicf;
  int kf; 
  char *p;
  char *kk1,*kk2;
  char head[200]; /* "int func_name" */
  char center[200]; /* "int a,int b" */
  char bottom[200]; /* "{" or ";" */
  char tmpbuf[500];
  char word[100];

  buf = str;
  vdf = 0;



  p = buf;

  kf = 0;
  while(*p== ' ' || *p == '\t'){
    kf = 1;
    ++p;
  }
  if(Cppcmmtf){
    if(str2ncmp(p,"//",2)==0){
      /* C++ comment out */
      putsp2("\n");
      return 1;
    }
  }
  if(*p == '#'){
    /* #define, #if, #ifdef... */
    /* delete spaces of head.(ANSI C's #define to K&R C's #define) */
    putsp2(p);
    return 0;
  }
  if(kf){
    puts_p(buf);
    return 0;
  }
  if(issentou(*str) == 0){
    /* sentou ni sengen nasi */

    /* 1st column no declaration */
    puts_p(buf);
    return 0;
  }

  semicf = 0;
  if((kk1 = strst2(buf,"(")) != 0){
    *kk1 = 0;
    strcpy(head,buf);
    *kk1 = '(';
    if((kk2 = strst2(kk1+1,")")) != 0){
      /* () function? or function declaration? */
      *kk2 = 0;
      strcpy(center,kk1+1);
      *kk2 = ')';
      rt = check_center(center);
      if(rt < 0){
        puts_p(buf);
        return 0;
      }
      if(rt == 1){
        /* (void) */
        vdf = 1;
      }
      strcpy(bottom,kk2+1);
      p = kk2+1;
      while(iskuuhaku(*p)){
        ++p;
      }
      if(*p == ';'){
        /* function declaration? */
        semicf = 1;
      }

    }
    else{
      /* not function */
      puts_p(buf);
      return 0;
    }
  }
  else{
    /* not function */
    puts_p(buf);
    return 0;
  }

  lef = getword(head,word);

  if(lef != 1 && lef != 2){
    /* not function */
    puts_p(buf);
    return 0;
  }

  if(yoyakuck(word)){
    /* if, else, for, while, switch, break... */
    puts_p(buf);
    return 0;
  }

  if(syorisub(head,word,buf,tmpbuf)<0){
    return 0;
  }


  strcpy(tmpbuf,head);
  chg_kata(tmpbuf,0,Voidf);
  putsp2(tmpbuf);

  putsp2("(");
  if(semicf == 0){
    strcpy(tmpbuf,center);
    chg_center(tmpbuf);
    putsp2(tmpbuf);
  }
  putsp2(")");

  if(semicf == 0 && vdf == 0){
    strcpy(tmpbuf,center);
    repl_a_all(tmpbuf, "," , ";");
    chg_kata(tmpbuf,1,Voidf);
    putsp2(tmpbuf);
    if(tmpbuf[0] != 0){
      putsp2(";");
    }
  }
  puts_p(bottom);

  return 2;
}

int iseigo(ccc)char ccc;
{
  /* eiji? */
  if((ccc>64 && ccc<91) || (ccc>96 && ccc<123)) return 0; else return -1;
}

int option(argc,argv)int argc;char **argv;
{
  int i;
  int tugia;
  int lof;	/* arg option flag */

  /* getopt() not use */
  for(i=1;i<argc;++i){
    tugia = 0;
    lof = 0;
    if(argv[i][tugia]=='-'){
     while(1){
       switch (argv[i][tugia+1]){

         case 'p':
           Noregalof = 1; /* for MSX.input "#pragama noregalo" in source code */
           break;
         case 'v':
           Voidf = 1;	/* delete "void" */
           break;
         case 'u':
           Castf = 1;	/* use cast.(no cut cast)  */
           break;
         case 'e':
           Cpmeoff = 0;	/* no output CPMEOF  */
           break;
         case 'c':
           Cppcmmtf = 0;	/* no use C++ comment  */
           break;
         case 'm':
           /* for MSX-C mode. same -vup option. */
           Voidf = 1;	/* delete "void" */
           Castf = 1;	/* use cast.(no cut cast)  */
           Noregalof = 1; /* for MSX.input "#pragama noregalo" in source code */
           break;
         default:
             fprintf(stderr,"Invalid option '-%c'",argv[i][tugia+1]);
             return -1;
      }
       if(lof){
         break;
       }
       else
       if(iseigo(argv[i][tugia+2]))
         break;
       else
         ++tugia;
     }
    }
    else{
      /*
      fprintf(stderr,"Invalid option '%s'",&argv[i][0]);
      return -1;
      */
    }
  }
  return 0;
}

void putopt()
{
  if(Castf){
    fprintf(stderr,"cast on ,");
  }
  else{
    fprintf(stderr,"cast off ,");
  }
  if(Cppcmmtf){
    fprintf(stderr,"C++ comment on ,");
  }
  else{
    fprintf(stderr,"C++ comment off ,");
  }

  if(Voidf){
    fprintf(stderr,"delete \"void\" on ,");
  }
  else{
    fprintf(stderr,"delete \"void\" off ,");
  }

  if(Cpmeoff){
    fprintf(stderr,"CP/M EOF on");
  }
  else{
    fprintf(stderr,"CP/M EOF off");
  }
}

int main(argc,argv)int argc;char **argv;
{
  FILE *fp;
  char *infile;
  char *outfile;
  int i;
  int fer,fnc;
  char buf[LINEMAX * 2 +20];
  int saisyo;

  Voidf = 0;	/* delete "void" mode off */
  Noregalof = 0; /* put "#pragma noregalo" mode off */
  Castf = 0;    /* cast off */
  Cppcmmtf = 1;	/* C++ comment mode on */
  Cpmeoff = 1; /* output CP/M EOF 0x1a(CP/M file end mark) */
  Outf = 0;
  saisyo = 1;

  if(argc < 2){
    setumei();
    return -1;
  }
  if(argc > 6){
    setumei();
    return -1;
  }

  fnc = 0;
  for(i=1;i<argc;++i){
    if(argv[i][0] != '-'){
      ++fnc;
      if(fnc == 1){
        infile = argv[i];
      }
      else
      if(fnc == 2){
        outfile = argv[i];
        Outf = 1;
      }
      else{
        setumei();
        return -1;
      }
    }
  }

  fnc = 0;
  if(argc > 1){
    for(i=1;i<argc;++i){
      if(argv[i][0] != '-'){
        ++fnc;
      }
    }

    if(fnc > 2){
        setumei();
        return -1;
    }
    if(option(argc,argv)){
      setumei();
      return -1;
    }
  }

  fprintf(stderr,"asu ansi2kr ver [%s] (C) Masaki Oba 2017\n",VERSION);

  if(0 == (fp = fopen(infile,"r"))){
    fprintf(stderr,"Error: input file can't open[%s]%c\n",infile,7);
    return -1;
  }
  if(Outf){
    if(0 == (Outp = fopen(outfile,"w"))){
      fprintf(stderr,"Error: output file can't open[%s]%c\n",outfile,7);
      return -1;
    }
  }

  fer = 0;
  while(1){
    buf[0] = 0;
    if(0 == fgets(buf,WORDLENMAX,fp)){
      fer = 1;
      if(buf[0] == 0){
        break;
      }
    }
    if(saisyo){
      if(Noregalof){
        /* -p option.for MSX-C. */
        char *s;
        putsp2("#pragma noregalo");	/* for  MSX-C compiler */
        s = buf;
        while(iskuuhaku(*s)){
          ++s;
        }
        if(*s == 0 || *s == '\r' || *s == '\n'){
          /* line end */
          putsp2("\n");
          saisyo = 0;
          continue;
        }
        if(Cppcmmtf){
          if(*s == '/' && *(s+1) == '/'){
            /* C++ Comment */
            putsp2("\n");
            saisyo = 0;
            continue;
          }
        }
        if(*s == '/' && *(s+1) == '*'){
          /* C Comment */
          putsp2("\t");
          putsp2(s);
          saisyo = 0;
          continue;
        }
        putsp2("\n");
      }
    }
    saisyo = 0;
    syori(buf);
    if(fer){
      break;
    } 
  }
  if(Cpmeoff){
    if(Outf){
      fprintf(Outp,"%c",CPMEOF);	/* for CP/M C compiler */
    }
    else{
      printf("%c",CPMEOF);	/* for CP/M C compiler */
    }
  }
  fclose(fp);

  putopt();

  fprintf(stderr,"\n");
  if(Outf){
    fclose(Outp);
    fprintf(stderr,"Output K&R C source.[%s]\n",outfile);
  }
  return 0;
}
