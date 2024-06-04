CC = clang
#CCFLAGS = -std=c89 -x c
CCFLAGS = -x c -m32
LDFLAGS = -m32

all : cc1130

cc1130 : CC1.o CC2.o CC3.o CC4.o GETARG.o
	clang $(LDFLAGS) CC1.o CC2.o CC3.o CC4.o GETARG.o -o cc1130

CC1.o : CC1.C CC.H STDIO.H
	$(CC) $(CCFLAGS) -c CC1.C

CC2.o : CC2.C CC.H STDIO.H
	$(CC) $(CCFLAGS) -c CC2.C

CC3.o : CC3.C CC.H STDIO.H
	$(CC) $(CCFLAGS) -c CC3.C

CC4.o : CC4.C CC.H STDIO.H
	$(CC) $(CCFLAGS) -c CC4.C

GETARG.o : GETARG.C STDIO.H
	$(CC) $(CCFLAGS) -c GETARG.C

#cc  cc1 -m -a -p
#if errorlevel 1 goto exit
#asm cc1 /p
#if errorlevel 1 goto exit
#: del cc1.asm
#
#cc  cc2 -m -a -p
#if errorlevel 1 goto exit
#asm cc2 /p
#if errorlevel 1 goto exit
#: del cc2.asm
#
#cc  cc3 -m -a -p
#if errorlevel 1 goto exit
#asm cc3 /p
#if errorlevel 1 goto exit
#: del cc3.asm
#
#cc  cc4 -m -a -p
#if errorlevel 1 goto exit
#asm cc4 /p
#if errorlevel 1 goto exit
#: del cc4.asm
#
#link cc1 cc2 cc3 cc4,cc1130,cc1130,clib.lib
#
#:exit
#pause
#
