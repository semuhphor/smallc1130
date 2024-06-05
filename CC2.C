/*
** Small-C Compiler -- Part 2 -- Front End and Miscellaneous.
** Copyright 1982, 1983, 1985, 1988 J. E. Hendrix
** All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "STDIO.H"
#include "CC.H"

static void keepch (char c);
static void ifline (void);
void inln (void);

/********************** input functions **********************/

void preprocess (void) {
  int k;
  char c;
  if (ccode) {
    line = mline;
    ifline ();
    if (eof) 
      return;
  } else {
    inln ();
    return;
  }
  pptr = -1;
  while (ch != NEWLINE && ch) {
    if (white ()) {
      keepch (' ');
      while (white ())
         gch ();
    } else if (ch == '"') {
      keepch (ch);
      gch ();
      while (ch != '"' || (* (lptr-1) == 92 && * (lptr-2) != 92)) {
        if (ch == NUL) {
          error ("no quote");
          break;
        }
        keepch (gch ());
      }
      gch ();
      keepch ('"');
    } else if (ch == 39) {
      keepch (39);
      gch ();
      while (ch != 39 || (* (lptr-1) == 92 && * (lptr-2) != 92)) {
        if (ch == NUL) {
          error ("no apostrophe");
          break;
        }
        keepch (gch ());
      }
      gch ();
      keepch (39);
    } else if (ch == '/' && nch == '*') {
      bump (2);
      while ( (ch == '*' && nch == '/') == 0) {
        if (ch)
          bump (1);
        else {
          ifline ();
          if (eof) 
             break;
        }
      }
      bump (2);
    } else if (ch == '/' && nch == '/') {     /* handle // comment */
      bump (2);
      while (ch) {
        if (ch) {
          bump (1);
        }
      }
      ifline ();
      if (eof) {
        break;
      }
    } else if (an (ch)) {
      k = 0;
      while (an (ch) && k < NAMEMAX) {
        msname[k++] = ch;
        gch ();
      }
      msname[k] = NUL;
      if (search (msname, macn, NAMESIZE+2, MACNEND, MACNBR, 0)) {
        k = getint (cptr+NAMESIZE, 2);
        while ((c = macq[k++]))
          keepch (c);
        while (an (ch))
          gch ();
      } else {
        k = 0;
        while ((c = msname[k++]))
          keepch (c);
      }
    } else 
      keepch (gch ());
  }
  if (pptr >= LINEMAX)
     error ("line too long");
  keepch (NUL);
  line = pline;
  bump (0);
}

static void keepch (char c) {
  if (pptr < LINEMAX)
    pline[++pptr] = c;
}

static void ifline (void) {
  while (1) {
    inln ();
    if (eof)
      return;
    if (match ("#ifdef")) {
      ++ iflevel;
      if (skiplevel)
        continue;
      symname (msname);
      if (search (msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0) == 0)
        skiplevel = iflevel;
      continue;
    }
    if (match ("#ifndef")) {
      ++ iflevel;
      if (skiplevel)
        continue;
      symname (msname);
      if (search (msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0))
        skiplevel = iflevel;
      continue;
      }
    if (match ("#else")) {
      if (iflevel) {
        if (skiplevel == iflevel)
          skiplevel = 0;
        else if (skiplevel == 0)
          skiplevel = iflevel;
        }
      else
        noiferr ();
      continue;
    }
    if (match ("#endif")) {
      if (iflevel) {
        if (skiplevel == iflevel)
          skiplevel = 0;
        -- iflevel;
      }
      else
        noiferr ();
      continue;
    }
    if (skiplevel)
      continue;
    if (ch == 0)
      continue;
    break;
  }
}

void inln (void) {           /* numerous revisions */
  int k;
  FILE * unit;
  poll (1);           /* allow operator interruption */
  if (input == SC_EOF)
    openfile ();
  if (eof) 
   return;
  if ((unit = input2) == SC_EOF) 
   unit = input;
  if (fgets (line, LINEMAX, unit) == NULL) {
    fclose (unit);
    if (input2 != SC_EOF)
      input2 = SC_EOF;
    else 
     input  = SC_EOF;
    *line = NUL;
  } else if (listfp) {
    if (listfp == output)
      fputc (';', output);
    fputs (line, listfp);
  }
  bump (0);
}

int inbyte (void) {
  while (ch == 0) {
    if (eof)
      return 0;
    preprocess ();
  }
  return gch ();
}

/********************* scanning functions ********************/

/*
** test if next input string is legal symbol name
*/
int symname (char * sname) {
  int k;
  char c;
  blanks ();
  if (alpha (ch) == 0)
    return (*sname = 0);
  k = 0;
  while (an (ch)) {
    sname[k] = gch ();
    if (k < NAMEMAX)
      ++k;
  }
  sname[k] = 0;
  return 1;
}

void need (char * str) {
  if (match (str) == 0)
    error ("missing token");
}

void ns (void) {
  if (match (";") == 0)
    error ("no semicolon");
  else
     errflag = 0;
}

int match (char * lit) {
  int k;
  blanks ();
  if ((k = streq (lptr, lit))) {
    bump (k);
    return 1;
  }
  return 0;
}

int streq (char str1[], char str2[]) {
  int k;
  k = 0;
  while (str2[k]) {
    if (str1[k] != str2[k])
     return 0;
    ++k;
  }
  return k;
}

int amatch (char * lit, int len) {
  int k;
  blanks ();
  if ((k = astreq (lptr, lit, len))) {
    bump (k);
    return 1;
  }
  return 0;
}

int astreq (char str1[], char str2[], int len) {
  int k;
  k = 0;
  while (k < len) {
    if (str1[k] != str2[k]) break;
    /*
    ** must detect end of symbol table names terminated by
    ** symbol length in binary
    */
    if (str2[k] < ' ') break;
    if (str1[k] < ' ') break;
    ++k;
    }
  if (an (str1[k]) || an (str2[k]))
    return 0;
  return k;
}

int nextop (char * list) {
  char op[4];
  opindex = 0;
  blanks ();
  while (1) {
    opsize = 0;
    while (*list > ' ')
      op[opsize++] = *list++;
    op[opsize] = 0;
    if ((opsize = streq (lptr, op)))
      if (* (lptr+opsize) != '=' && 
         * (lptr+opsize) != * (lptr+opsize-1))
       return 1;
    if (*list) {
      ++list;
      ++opindex;
    }
    else
      return 0;
  }
}

void blanks (void) {
  while (1) {
    while (ch) {
      if (white ())
        gch ();
      else
        return;
    }
    if (line == mline)
      return;
    preprocess ();
    if (eof)
      break;
  }
}

int white (void) {
  // XXX punt
  // avail (YES);  /* abort on stack/symbol table overflow */
  return *lptr <= ' ' && *lptr;
}

char gch (void) {
  int c;
  if ((c = ch))
    bump (1);
  return c;
}

void bump (int n) {
  if (n)
    lptr += n;
  else
    lptr  = line;
  if ((ch = nch = *lptr))
    nch = *(lptr+1);
}

void kill (void) {
  *line = 0;
  bump (0);
}

void skip (void) {
  if (an (inbyte ()))
    while (an (ch))
      gch ();
  else
    while (an (ch) == 0) {
      if (ch == 0)
        break;
      gch ();
    }
  blanks ();
}

int endst (void) {
  blanks ();
  return streq (lptr, ";") || ch == 0;
}

/*********** symbol table management functions ***********/

char * addsym(char * sname, int id, int type, int size, int value, char **lgpp, int class) {
  printf ("sname:%s id:%d type:%d size:%d value:%d, lgpp:%p, class:%d, symno:%d\n",
    sname, id, type, size, value, *lgpp, class, symno);
  if (lgpp == & glbptr) {
    if ((cptr2 = findglb (sname)))
      return cptr2;
    if (cptr == 0) {
      error ("global symbol table overflow");
      return 0;
    }
  } else {
    if (locptr > (ENDLOC-SYMMAX)) {
      error ("local symbol table overflow");
      abort (ERRCODE);
    }
    cptr = *lgpp;
  }
  cptr[IDENT] = id;
  cptr[TYPE]  = type;
  cptr[CLASS] = class;
  putint (size, cptr + SIZE, 2);
  putint (value, cptr + OFFSET, 2);
  putint (symno++, cptr + SYMNO, 2);
  cptr3 = cptr2 = cptr + NAME;
  while (an (*sname))
    *cptr2++ = *sname++;
  if (lgpp == &locptr) {
    *cptr2 = cptr2 - cptr3;         /* set length */
    *lgpp = ++cptr2;
  }
  /* printf ("current symno: %d, symbol: %s, it's symno: %d\n", symno, cptr+NAME, getint (cptr+SYMNO, 2)); */
  return cptr;
}

/*
** search for symbol match
** on return cptr points to slot found or empty slot
*/
int search (char * sname, char * buf, int len, char * end, int max, int off) {
  cptr  =
  cptr2 = buf+ ((hash (sname)% (max-1))*len);
  while (*cptr != NUL) {
    if (astreq (sname, cptr+off, NAMEMAX))
      return 1;
    if ((cptr = cptr+len) >= end)
      cptr = buf;
    if (cptr == cptr2)
      return cptr = NULL, 0;
  }
  return 0;
}

int hash (char *sname) {
  int i, c;
  i = 0;
  while ((c = *sname++))
    i = (i << 1) + c;
  return i;
}

char * findglb (char *sname) {
  if (search (sname, STARTGLB, SYMMAX, ENDGLB, NUMGLBS, NAME))
    return cptr;
  return 0;
}

char * findloc (char *sname) {
  cptr = locptr - 1;  /* search backward for block locals */
  while (cptr > STARTLOC) {
    cptr = cptr - *cptr;
    if (astreq (sname, cptr, NAMEMAX))
      return (cptr - NAME);
    cptr = cptr - NAME - 1;
  }
  return 0;
}

char * nextsym (char *entry) {
  entry = entry + NAME;
  while (*entry++ >= ' ')
    ;    /* find length byte */
  return entry;
}

/******** while queue management functions *********/  

void addwhile (int ptr[]) {
  int k;
  ptr[WQSP]   = csp;         /* and stk ptr */
  ptr[WQLOOP] = getlabel ();  /* and looping label */
  ptr[WQEXIT] = getlabel ();  /* and exit label */
  if (wqptr == WQMAX) {
    error ("control statement nesting limit");
    abort (ERRCODE);
  }
  k = 0;
  while (k < WQSIZ)
    *wqptr++ = ptr[k++];
}

char * readwhile (char *ptr) {
  if (ptr <= wq) {
    error ("out of context");
    return 0;
    }
  else
    return (ptr - WQSIZ);
}

void delwhile (void) {
  if (wqptr > wq)
    wqptr -= WQSIZ;
}

/****************** utility functions ********************/  

/*
** test if c is alphabetic
*/
int alpha (char c) {
  return isalpha (c) || c == '_';
}

/*
** test if given character is alphanumeric
*/
int an (char c) {
  return alpha (c) || isdigit (c);
}

/*
** return next avail internal label number
*/
int getlabel (void) {
  return ++ nxtlab;
}

/*
** get integer of length len from address addr
** (byte sequence set by "putint")
*/
int getint (char *addr, int len) {
  int i;
  i = *(addr + --len);  /* high order byte sign extended */
  while (len--)
    i = (i << 8) | *(addr + len) & 255;
  return i;
}

/*
** put integer i of length len into address addr
** (low byte first)
*/
void putint (int i, char * addr, int len) {
  while (len--) {
    *addr++ = i;
    i = i >> 8;
  }
}

void lout (char * line, FILE * fd) {
  fputs (line, fd);
  fputc (NEWLINE, fd);
}

/******************* error functions *********************/  

void illname (void) {
  error ("illegal symbol");
  skip ();
  }

void multidef (char *sname) {
  error ("already defined");
}

void needlval (void) {
  error ("must be lvalue");
}

void noiferr (void) {
  error ("no matching #if...");
  errflag = 0;
}

void error (char msg[]) {
  if (errflag)
    return;
  else
    errflag = 1;
  lout (line, stderr);
  errout (msg, stderr);
  if (alrm)
    fputc (7, stderr);
  if (paus)
    while (fgetc (stderr) != NEWLINE);
  if (listfp > 0)
    errout (msg, listfp);
}

void errout (char * msg, FILE * fp) {
  char * k;
  k = line + 2;
  while (k++ <= lptr)
    fputc (' ', fp);
  lout ("/\\", fp);
  fputs ("**** ", fp);
  lout (msg, fp);
}

