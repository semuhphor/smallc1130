/*
** Small-C Compiler -- Part 1 --  Top End.
** Copyright 1982, 1983, 1985, 1988 J. E. Hendrix
** All rights reserved.
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "STDIO.H"
#include "NOTICE.H"
#include "CC.H"

/*
** miscellaneous storage
*/
static int nogo;     /* disable goto statements? */
static int noloc;    /* disable block locals? */
int opindex;  /* index to matched operator */
int opsize;   /* size of operator in characters */
static int swactive; /* inside a switch? */
static int swdefault;/* default label #; else 0 */
static int*swnext;   /* address of next entry */
static int*swend;    /* address of last entry */
int*stage;    /* staging buffer address */
char *wq;       /* while queue */
static int argcs;    /* static argc */
static char **argvs;    /* static argv */
char *wqptr;    /* ptr to next entry */
int litptr;   /* ptr to next entry */
int macptr;   /* macro buffer index */
int pptr;     /* ptr to parsing buffer */
int ch;       /* current character of input line */
int nch;      /* next character of input line */
static int declared; /* # of local bytes to declare; -1 when declared */
int iflevel;  /* #if... nest level */
int skiplevel;/* level at which #if... skipping started */
int nxtlab;   /* next avail label # */
int litlab;   /* label # assigned to literal pool */
int csp;      /* compiler relative stk ptr */
static int argstk;   /* function arg sp */
static int argtop;   /* highest formal argument offset */
static int ncmp;     /* # open compound statements */
int errflag;  /* true after 1st error in statement */
int eof;      /* true on final input eof */
FILE * output;   /* fd for output file */
static int files;    /* true if file list specified on cmd line */
static int filearg;  /* cur file arg index */
FILE * input   = SC_EOF; /* fd for input file */
FILE * input2  = SC_EOF; /* fd for "#include" file */
int usexpr  = YES; /* true if value of expression is used */
int ccode   = YES; /* true while parsing C code */
int*snext;    /* next addr in stage */
int*stail;    /* last addr of data in stage */
int*slast;    /* last addr in stage */
FILE * listfp;   /* file pointer to list device */
static int lastst;   /* last parsed statement type */
int oldseg;   /* current segment (0, DATASEG, CODESEG) */
int symno = 1;/* symbol number */

char optimize; /* optimize output of staging buffer? */
char alrm;    /* audible alarm on errors? */
static char monitor;  /* monitor function headers? */
char paus;    /* pause for operator on errors? */
char *symtab;   /* symbol table */
char *litq;     /* literal pool */
char *macn;     /* macro name buffer */
char *macq;     /* macro string buffer */
char *pline;    /* parsing buffer */
char *mline;    /* macro buffer */
char *line;     /* ptr to pline or mline */
char *lptr;     /* ptr to current character in "line" */
char *glbptr;   /* global symbol table */
char *locptr;   /* next local symbol table entry */
char quote[2] = {'"'}; /* literal string for '"' */
char *cptr;     /* work ptrs to any char buffer */
char *cptr2;
char *cptr3;
char msname[NAMESIZE];   /* macro symbol name */
char ssname[NAMESIZE];   /* static symbol name */

int op[16] = {   /* p-codes of signed binary operators */
   OR12,                        /* level5 */
   XOR12,                       /* level6 */
   AND12,                       /* level7 */
   EQ12,   NE12,                /* level8 */
   LE12,   GE12,  LT12,  GT12,  /* level9 */
   ASR12,  ASL12,               /* level10 */
   ADD12,  SUB12,               /* level11 */
   MUL12,  DIV12, MOD12         /* level12 */
};

int op2[16] = {  /* p-codes of unsigned binary operators */
   OR12,                        /* level5 */
   XOR12,                       /* level6 */
   AND12,                       /* level7 */
   EQ12,   NE12,                /* level8 */
   LE12u,  GE12u, LT12u, GT12u, /* level9 */
   ASR12,  ASL12,               /* level10 */
   ADD12,  SUB12,               /* level11 */
   MUL12u, DIV12u, MOD12u       /* level12 */
};

static int needsub (void);
static int dolabel (void);
static void doargs (int type);
static int decl (int type, int aid, int *id, int *sz);
static int statement (void);
static void declloc (int type);
static void compound (void);
static void doif (void);
static void dowhile (void);
static void dodo (void);
static void dofor (void);
static void doswitch (void);
static void docase (void);
static void dodefault (void);
static void dogoto (void);
static int addlabel (int def);
static void doreturn (void);
static void dobreak (void);
static void docont (void);
static void doasm (void);
static void doexpr (int use);

/*
** execution begins here
*/
int main (int argc, char * argv[]) {
  fputs (VERSION, stderr);
  fputs (CRIGHT1, stderr);
  argcs   = argc;
  argvs   = argv;
  swnext  = calloc (SWTABSZ, 1);
  swend   = swnext + (SWTABSZ - SWSIZ);
  stage   = calloc (STAGESIZE, 2 * BPW);
  wqptr   =
  wq      = calloc (WQTABSZ, BPW);
  litq    = calloc (LITABSZ, 1);
  macn    = calloc (MACNSIZE, 1);
  macq    = calloc (MACQSIZE, 1);
  pline   = calloc (LINESIZE, 1);
  mline   = calloc (LINESIZE, 1);
  slast   = stage+ (STAGESIZE * 2 * BPW);
  symtab  = calloc ((NUMLOCS * SYMAVG + NUMGLBS * SYMMAX), 1);
  locptr  = STARTLOC;
  glbptr  = STARTGLB;
 
  ask ();                /* get user options */
  openfile ();              /* and initial input file */
  preprocess ();            /* fetch first line */
  header ();              /* intro code */
  setcodes ();              /* initialize code pointer array */
  parse ();              /* process ALL input */
  trailer ();              /* follow-up code */
  fclose (output);            /* explicitly close output */
  return 0;
}

/******************** high level parsing *******************/

/*
** process all input text
**
** At this level, only static declarations,
**      defines, includes and function
**      definitions are legal...
*/
void parse (void) {
  while (eof == 0) {
    if (amatch ("extern", 6))   
      dodeclare (EXTERNAL);
       
    else if (dodeclare (STATIC))
      ;
   
    else if (match ("#asm"))     
      doasm ();
   
    else if (match ("#include"))   
      doinclude ();
   
    else if (match ("#define"))  
      dodefine ();
   
    else                        
      dofunction ();
   
    blanks ();                 /* force eof if pending */
  }
}

/*
** test for global declarations
*/
int dodeclare (int class) {
  if (amatch ("char", 4))   
    declglb (CHR, class);
 
  else if (amatch ("unsigned", 8)) {
    if (amatch ("char", 4))   
      declglb (UCHR, class);
    else {
      amatch ("int", 3);   
      declglb (UINT, class);
    }
  }
  else if (amatch ("int", 3) || class == EXTERNAL)   
    declglb (INT,  class);
 
  else
    return 0;

  ns ();
 
  return 1;
}

/*
** declare a static variable
*/
void declglb (int type, int class) {
  int id, dim;
  while (1) {
    if (endst ())
      return;  /* do line */

    if (match ("*"))       {
      id = POINTER; 
      dim = 0;
    }
   
    else                 {
      id = VARIABLE;
      dim = 1;
    }

    if (symname (ssname) == 0)
      illname ();
   
    if (findglb (ssname))
      multidef (ssname);

    if (id == VARIABLE) {
      if (match ("(")) {
        id = FUNCTION;
        need (")");
      }
     
      else if (match ("[")) {
        id = ARRAY;
        dim = needsub ();
      }
    }
    if (class == EXTERNAL)
      external (ssname, type >> 2, id);

    else if (id != FUNCTION)
      initials (type >> 2, id, dim);
   
    if (id == POINTER)
      addsym (ssname, id, type, BPW, 0, &glbptr, class);
   
    else addsym (ssname, id, type, dim * (type >> 2), 0, &glbptr, class);
   
    if (match (",") == 0)
      return;
  }
}

/*
** initialize global objects
*/
void initials (int size, int ident, int dim) {
  int savedim;
 
  litptr = 0;
 
  if (dim == 0)
    dim = -1;         /* *... or ...[] */
 
  savedim = dim;
  public (ident);
 
  if (match ("=")) {
    if (match ("{")) {
      while (dim) {
        init (size, ident, & dim);

        if (match (",") == 0)
          break;
      }
     
      need ("}");
    }
    else
      init (size, ident, & dim);
  }
 
  if (savedim == -1 && dim == -1) {
    if (ident == ARRAY)
      error ("need array size");
   
    stowlit (0, size = BPW);
  }
 
  dumplits (size);
  dumpzero (size, dim);           /* only if dim > 0 */
}

/*
** evaluate one initializer
*/
void init (int size, int ident, int * dim) {
  int value;

  if (string (& value)) {
    if (ident == VARIABLE || size != 1)
      error ("must assign to char pointer or char array");

    *dim -= (litptr - value);
   
    if (ident == POINTER)
      point ();
    }
 
  else if (constexpr (& value)) {
    if (ident == POINTER)
      error ("cannot assign to pointer");
   
    stowlit (value, size);
    *dim -= 1;
  }
}

/*
** get required array size
*/
static int needsub (void) {
  int val;
 
  if (match ("]"))
    return 0;      /* null size */
 
  if (constexpr (& val) == 0)
    val = 1;

  if (val < 0) {
    error ("negative size illegal");
    val = -val;
  }
 
  need ("]");               /* force single dimension */
  return val;              /* and return size */
}

/*
** open an include file
*/
void doinclude (void) {
  int i;
  char str[30];
 
  blanks ();       /* skip over to name */

  if (*lptr == '"' || *lptr == '<')
    ++lptr;

  i = 0;

  while (lptr[i]
      && lptr[i] != '"'
      && lptr[i] != '>'
      && lptr[i] != '\n') {
    str[i] = lptr[i];
    ++ i;
  }

 
  str [i] = '\0';
 
  if ((input2 = fopen (str,"r")) == NULL) {
    input2 = SC_EOF;
    error ("open failure on include file");
  }

  kill ();   /* make next read come from new file (if open) */
}

/*
** define a macro symbol
*/
void dodefine (void) {
  int k;
 
  if (symname (msname) == 0) {
    illname ();
    kill ();
    return;
  }

 
  k = 0;
 
  if (search (msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0) == 0) {
    cptr2 = cptr;
    if (cptr2)
      while (( * cptr2++ = msname[k++]))
        ;
   
    else {
      error ("macro name table full");
      return;

    }
  }

  putint (macptr, cptr + NAMESIZE, 2);

  while (white ())
    gch ();

  while (putmac (gch ()))
    ;

  if (macptr >= MACMAX) {
    error ("macro string queue full");
        abort (ERRCODE);
  }
}

char putmac (char c) {
  macq[macptr] = c;
 
  if (macptr < MACMAX)
    ++macptr;
 
  return c;
}

/*
** begin a function
**
** called from "parse" and tries to make a function
** out of the following text
*/
void dofunction (void) {
  char *ptr;

  nogo   =                    /* enable goto statements */
  noloc  =                    /* enable block-local declarations */
  lastst =                    /* no statement yet */
  litptr = 0;                 /* clear lit pool */
  litlab = getlabel ();       /* label next lit pool */
  locptr = STARTLOC;          /* clear local variables */
   
  if (match ("void"))
    blanks ();        /* skip "void" & locate header */

  if (monitor)
    lout (line, stderr);

  if (symname (ssname) == 0) {
    error ("illegal function or declaration");
    errflag = 0;
    kill ();                 /* invalidate line */
    return;
  }

  if ((ptr = findglb (ssname))) { /* already in symbol table? */
    if (ptr[CLASS] == AUTOEXT)
      ptr[CLASS] = STATIC;

    else
      multidef (ssname);
  }
  else
    addsym (ssname, FUNCTION, INT, 0, 0, &glbptr, STATIC);

  public (FUNCTION);
  argstk = 0;                  /* init arg count */
 
  if (match ("(") == 0)
    error ("no open paren");

  while (match (")") == 0) {      /* then count args */
    if (symname (ssname)) {
      if (findloc (ssname))
        multidef (ssname);
      else {
        addsym (ssname, 0, 0, 0, argstk, &locptr, AUTOMATIC);
        argstk += BPW;
      }
    }
   
    else {
      error ("illegal argument name");
      skip ();
    }
   
    blanks ();

    if (streq (lptr,")") == 0 && match (",") == 0)
      error ("no comma");

    if (endst ())
      break;
  }
 
  csp = 0;                     /* preset stack ptr */
  argtop = argstk + BPW;       /* account for the pushed BP */

  while (argstk) {
    if (amatch ("char", 4)) {
      doargs (CHR); 
      ns ();
    }

    else if (amatch ("int", 3)) {
      doargs (INT); 
      ns ();
    }
   
    else if (amatch ("unsigned", 8)) {
      if (amatch ("char", 4)) {
        doargs (UCHR);
        ns ();
      }

      else {
        amatch ("int", 3);
        doargs (UINT);
        ns ();
      }
    }
   
    else {
      error ("wrong number of arguments");
      break;
    }
  }
 
  gen (ENTER, 0);
  statement ();
 
  if (lastst != STRETURN && lastst != STGOTO)
    gen (RETURN, 0);

  if (litptr) {
    toseg (DATASEG);
    gen (REFm, litlab);
    dumplits (1);               /* dump literals */
  }
}

/*
** declare argument types
*/
static void doargs (int type) {
  int id, sz;
  char c, *ptr;

  while (1)
  {
    if (argstk == 0)
      return;           /* no arguments */

    if (decl (type, POINTER, &id, &sz))
    {
      if ((ptr = findloc (ssname)))
      {
        ptr[IDENT] = id;
        ptr[TYPE]  = type;
        putint (sz, ptr + SIZE, 2);
        putint (argtop - getint (ptr + OFFSET, 2), ptr + OFFSET, 2);
      }
      else
        error ("not an argument");
    }
   
    argstk = argstk - BPW;            /* cnt down */

    if (endst ())
      return;

    if (match (",") == 0)
      error ("no comma");
  }
}

/*
** parse next local or argument declaration
*/
static int decl (int type, int aid, int *id, int *sz) {
  int n, p;

  if (match ("("))
    p = 1;

  else
    p = 0;

  if (match ("*")) {
    *id = POINTER; 
    *sz  = BPW;
  }
 
  else {
    *id = VARIABLE;
    *sz  = type >> 2;
  }
 
  if ((n = symname (ssname)) == 0)
    illname ();

  if (p && match (")"))
    ;

  if (match ("(")) {
    if (!p || *id != POINTER)
      error ("try (*...)()");
   
    need (")");
  }
 
  else if (*id == VARIABLE && match ("[")) {
    *id = aid;
   
    if ((*sz *= needsub ()) == 0) {
      if (aid == ARRAY)
        error ("need array size");

      *sz  = BPW;      /* size of pointer argument */
    }
  }
 
  return n;
}

/******************** start 2nd level parsing *******************/

/*
** statement parser
*/
static int statement (void) {
  if (ch == 0 && eof)
    return 0;  // CAC Added '0'

  else if (amatch ("char",     4)) {
    declloc (CHR);   
    ns ();
  }
 
  else if (amatch ("int",      3)) {
    declloc (INT);   
    ns ();
  }

  else if (amatch ("unsigned", 8)) {
    if (amatch ("char",     4)) {
      declloc (UCHR);  
      ns ();
    }

    else {
      amatch ("int",      3); 
      declloc (UINT);  
      ns ();
    }
  }

  else {
    if (declared >= 0) {
      if (ncmp > 1)
        nogo = declared;   /* disable goto */

      gen (ADDSP, csp - declared);
      declared = -1;
    }

    if (match ("{"))        
      compound ();

    else if (amatch ("if",       2)) {
      doif (); 
      lastst = STIF;
    }

    else if (amatch ("while",    5)) {
      dowhile (); 
      lastst = STWHILE;
    }

    else if (amatch ("do",       2)) {
      dodo ();
      lastst = STDO;
    }

    else if (amatch ("for",      3)) {
      dofor ();
      lastst = STFOR;
    }

    else if (amatch ("switch",   6)) {
      doswitch ();
      lastst = STSWITCH;
    }

    else if (amatch ("case",     4)) {
      docase ();     
      lastst = STCASE;
    }

    else if (amatch ("default",  7)) {
      dodefault ();  
      lastst = STDEF;
    }

    else if (amatch ("goto",     4)) {
      dogoto ();  
      lastst = STGOTO;
    }
   
    else if (dolabel ()) 
      lastst = STLABEL;

    else if (amatch ("return",   6)) {
      doreturn ();
      ns ();
      lastst = STRETURN;
    }
    else if (amatch ("break",    5)) {
      dobreak (); 
      ns ();
      lastst = STBREAK;
    }
   
    else if (amatch ("continue", 8)) {
      docont ();  
      ns ();
      lastst = STCONT;
    }

    else if (match (";"))
      errflag = 0;

    else if (match ("#asm"))        {
      doasm ();         
      lastst = STASM;
    }

    else {
      doexpr (NO);
      ns ();
      lastst = STEXPR;
    }
  }
 
  return lastst;
}

/*
** declare local variables
*/
static void declloc (int type) {
  int id, sz;

  if (swactive)   
    error ("not allowed in switch");

  if (noloc)
    error ("not allowed with goto");
 
  if (declared < 0)
    error ("must declare first in block");
 
  while (1) {
    if (endst ())
      return;

    decl (type, ARRAY, & id, &sz);
    declared += sz;
    addsym (ssname, id, type,  sz, csp - declared, & locptr, AUTOMATIC);

    if (match (",") == 0)
      return;
  }
}

static void compound (void) {
  int savcsp;
  char *savloc;

  savcsp = csp;
  savloc = locptr;
  declared = 0;           /* may now declare local variables */
  ++ ncmp;                 /* new level open */
 
  while (match ("}") == 0) {
    if (eof) {
      error ("no final }");
      break;
    }
    else
      statement ();     /* do one */
  }
  if (--ncmp               /* close current level */
      && lastst != STRETURN
      && lastst != STGOTO)
  gen (ADDSP, savcsp);   /* delete local variable space */

  cptr = savloc;          /* retain labels */
 
  while (cptr < locptr) {
    cptr2 = nextsym (cptr);
   
    if (cptr[IDENT] == LABEL) {
      while (cptr < cptr2)
        *savloc++ = *cptr++;
    }
   
    else
      cptr = cptr2;
  }

  locptr = savloc;        /* delete local symbols */
  declared = -1;          /* may not declare variables */
}

static void doif (void) {
  int flab1, flab2;

  test (flab1 = getlabel (), YES);  /* get expr, and branch false */
  statement ();                    /* if true, do a statement */
  if (amatch ("else", 4) == 0)     /* if...else ? */
  {
    /* simple "if"...print false label */
    gen (LABm, flab1);
    return;                       /* and exit */
  }
 
  flab2 = getlabel ();
  if (lastst != STRETURN && lastst != STGOTO)
    gen (JMPm, flab2);
 
  gen (LABm, flab1);    /* print false label */
  statement ();         /* and do "else" clause */
  gen (LABm, flab2);    /* print true label */
}

static void dowhile (void) {
  int wq[4];              /* allocate local queue */

  addwhile (wq);           /* add entry to queue for "break" */
  gen (LABm, wq[WQLOOP]);  /* loop label */
  test (wq[WQEXIT], YES);  /* see if true */
  statement ();            /* if so, do a statement */
  gen (JMPm, wq[WQLOOP]);  /* loop to label */
  gen (LABm, wq[WQEXIT]);  /* exit label */
  delwhile ();             /* delete queue entry */
}

static void dodo (void) {
  int wq[4];

  addwhile (wq);
  gen  (LABm, wq[WQLOOP]);
  statement  ();
  need  ("while");
  test  (wq[WQEXIT], YES);
  gen  (JMPm, wq[WQLOOP]);
  gen  (LABm, wq[WQEXIT]);
  delwhile ();
  ns  ();
}

static void dofor (void) {
  int wq[4], lab1, lab2;

  addwhile (wq);
  lab1 = getlabel ();
  lab2 = getlabel ();
  need ("(");

  if (match (";") == 0) {
    doexpr (NO);           /* expr 1 */
    ns ();
  }

  gen (LABm, lab1);

  if (match (";") == 0) {
    test (wq[WQEXIT], NO); /* expr 2 */
    ns ();
  }
 
  gen (JMPm, lab2);
  gen (LABm, wq[WQLOOP]);

  if (match (")") == 0) {
    doexpr (NO);           /* expr 3 */
    need (")");
  }

  gen (JMPm, lab1);
  gen (LABm, lab2);
  statement ();
  gen (JMPm, wq[WQLOOP]);
  gen (LABm, wq[WQEXIT]);
  delwhile ();
}

static void doswitch (void) {
  int wq[4], endlab, swact, swdef, *swnex, *swptr;

  swact = swactive;
  swdef = swdefault;
  swnex = swptr = swnext;
  addwhile (wq);
  *(wqptr + WQLOOP - WQSIZ) = 0;
  need  ("(");
  doexpr  (YES);                /* evaluate switch expression */
  need  (")");
  swdefault = 0;
  swactive = 1;
  gen  (JMPm, endlab = getlabel  ());
  statement  ();                /* cases, etc. */
  gen  (JMPm, wq[WQEXIT]);
  gen  (LABm, endlab);
  gen  (SWITCH, 0);             /* match cases */
 
  while (swptr < swnext) {
    gen (NEARm, *swptr++);
    gen (WORDn,  *swptr++);    /* case value */
  }

  gen (WORDn, 0);


  if (swdefault)
    gen (JMPm, swdefault);

  gen (LABm, wq[WQEXIT]);
  delwhile ();
  swnext    = swnex;
  swdefault = swdef;
  swactive  = swact;
}

static void docase (void) {
  if (swactive == 0)
    error ("not in switch");
 
  if (swnext > swend) {
    error ("too many cases");
    return;
  }

  gen (LABm, *swnext++ = getlabel ());
  constexpr (swnext++);
  need (":");
}

static void dodefault (void) {
  if (swactive) {
    if (swdefault)
      error ("multiple defaults");
    }
  else
    error ("not in switch");

  need (":");
  gen (LABm, swdefault = getlabel ());
}

static void dogoto (void) {
  if (nogo > 0)
    error ("not allowed with block-locals");

  else
    noloc = 1;
 
  if (symname (ssname))
    gen (JMPm, addlabel (NO));

  else
    error ("bad label");
 
  ns ();
}

static int dolabel (void) {
  char *savelptr;

  blanks ();
  savelptr = lptr;

  if (symname (ssname))
  {
    if (gch () == ':')
    {
      gen (LABm, addlabel (YES));
      return 1;
    }

    else
      bump (savelptr-lptr);
  }
  return 0;
}

static int addlabel (int def) {
  if ( (cptr = findloc (ssname))) {
    if (cptr[IDENT] != LABEL)
      error ("not a label");
    else if (def) {
      if (cptr[TYPE])
        error ("duplicate label");
      else
        cptr[TYPE] = YES;
    }
  }
  else
    cptr = addsym (ssname, LABEL, def, 0, getlabel (), & locptr, LABEL);

  return getint (cptr+OFFSET, 2);
}

static void doreturn (void) {
  int savcsp;

  if (endst () == 0)
    doexpr (YES);

  savcsp = csp;
  gen (RETURN, 0);
  csp = savcsp;
}

static void dobreak (void) {
  char *ptr;

  if ((ptr = readwhile (wqptr)) == 0)
    return;

  gen (ADDSP, ptr[WQSP]);
  gen (JMPm, ptr[WQEXIT]);
}

static void docont (void) {
  char *ptr;

  ptr = wqptr;
  while (1) {
    if ((ptr = readwhile (ptr)) == 0)
      return;

    if (ptr[WQLOOP])
      break;
  }

  gen (ADDSP, ptr[WQSP]);
  gen (JMPm, ptr[WQLOOP]);
}

static void doasm (void) {
  ccode = 0;           /* mark mode as "asm" */
 
  while (1) {
    inln ();

    if (match ("#endasm"))
      break;

    if (eof)
      break;
   
    fputs (line, output);
  }
 
  kill ();
  ccode = 1;
}

static void doexpr (int use) {
  int konst, val;
  int *before, *start;
 
  usexpr = use;        /* tell isfree() whether expr value is used */
 
  while (1) {
    setstage (& before, & start);
    expression (& konst, & val);
    clearstage (before, start);
   
    if (ch != ',')
      break;
   
    bump (1);
  }
 
  usexpr = YES;        /* return to normal value */
}

/******************** miscellaneous functions *******************/

/*
** get run options
*/
void ask (void) {

  int i;

  i = nxtlab = 0;
  listfp = NULL;
  output = stdout;
  optimize = YES;
  alrm = monitor = paus = NO;
  line = mline;

  while (getarg (++i, line, LINESIZE, argcs, argvs) != -1) {
    if (line[0] != '-' && line[0] != '/')
      continue;
   
// XX lose "-L unit_number"; needs to be "-L filename", and do the proper fopen
#if 0
    if (toupper (line[1]) == 'L'
        && isdigit (line[2])
        && line[3] <= ' ') {
      listfp = line[2]-'0';
      continue;
    }
#endif
   
    if (toupper (line[1]) == 'N'
        && toupper (line[2]) == 'O'
        && line[3] <= ' ') {
      optimize = NO;
      continue;
    }
   
    if (line[2] <= ' ') {
      if (toupper (line[1]) == 'A') {
        alrm   = YES;
        continue;
      }
     
      if (toupper (line[1]) == 'M') {
        monitor = YES;
        continue;
      }

      if (toupper (line[1]) == 'P') {
        paus   = YES;
        continue;
      }
    }

    fputs ("usage: cc [file]... [-m] [-a] [-p] [-l#] [-no]\n", stderr);
    abort (ERRCODE);
  }
}

/*
** input and output file opens
*/
void openfile (void) {                /* entire function revised */
  char outfn[15];
    int i, j, ext;

  input = SC_EOF;
 
  while (getarg (++filearg, pline, LINESIZE, argcs, argvs) != -1) {
    if (pline[0] == '-' || pline[0] == '/')
      continue;


    ext = NO;
    i = -1;
    j = 0;
   
    while (pline[++i]) {
      if (pline[i] == '.') { ext = YES;
        break;
      }
     
      if (j < 10)
        outfn[j++] = pline[i];
    }
   
    if (!ext)
      strcpy (pline + i, ".C");
   
    input = mustopen (pline, "r");
    if (!files && /*iscons (stdout)*/ isatty (fileno (stdout))) { // XXX replaced iscons 
      strcpy (outfn + j, ".ASM");
      output = mustopen (outfn, "w");
    }
   
    files = YES;
    kill ();
    return;
  }
 
  if (files++)
    eof = YES;

  else
    input = stdin;

  kill ();
}

/*
** open a file with error checking
*/
FILE * mustopen (char * fn, char * mode) {
  FILE * fd;

  if ((fd = fopen (fn, mode)))
    return fd;
 
  fputs ("open error on ", stderr);
  lout (fn, stderr);
  abort (ERRCODE);
}
