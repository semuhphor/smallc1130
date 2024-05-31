#define TRUE -1
#define FALSE !TRUE

wait(value)
int value;
{
#asm
************************************
* when we get here, stack (after XXssf)
*   +3: value to show in accumulator
*   +2: return address
*   +1: old frame pointer address (<- frame points here)
*   +0: (current stack location
* frame pointer = stack location + 1
************************************
	ld	 3	+2	acc = value (fp + 2)
	wait
#endasm
}

getDouble(value)
int value;
{
	return value * 2;
}

strlen(s)
char *s;
{
int len;
	len = 0;
	while (*s != 0)
	{
		len++;
		s++;
	}
	return len;
}

getSwitches()
{
#asm
	xio		gsioc
	ld		gsces
	b		gsend

gsioc	dc		gsces
	dc		/3a00
gsces	dc		*-*
gsend	equ		*
#endasm
}

astocp(c) 
char c;
{
char *chars;
int i;
unsigned int vals[40];

	chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.* ";
	i = 0;
	vals[i++] = 0x3E05; /* 	A */
	vals[i++] = 0x1A05; /* 	B */
	vals[i++] = 0x1E05; /* 	C */
	vals[i++] = 0x3205; /* 	D */
	vals[i++] = 0x3605; /* 	E */
	vals[i++] = 0x1205; /* 	F */
	vals[i++] = 0x1605; /* 	G */
	vals[i++] = 0x2605; /* 	H */
	vals[i++] = 0x2205; /* 	I */
	vals[i++] = 0xCE05; /* 	J */
	vals[i++] = 0x5A05; /* 	K */
	vals[i++] = 0x5E05; /* 	L */
	vals[i++] = 0x7205; /* 	M */
	vals[i++] = 0x7605; /* 	N */
	vals[i++] = 0x5205; /* 	O */
	vals[i++] = 0x5605; /* 	P */
	vals[i++] = 0x6605; /* 	Q */
	vals[i++] = 0x6205; /* 	R */
	vals[i++] = 0x9A05; /* 	S */
	vals[i++] = 0x9E05; /* 	T */
	vals[i++] = 0xB205; /* 	U */
	vals[i++] = 0xB605; /* 	V */
	vals[i++] = 0x9205; /* 	W */
	vals[i++] = 0x9605; /* 	X */
	vals[i++] = 0xA605; /* 	Y */
	vals[i++] = 0xA205; /* 	Z */
	vals[i++] = 0xC405; /* 	0 */
	vals[i++] = 0xFC05; /* 	1 */
	vals[i++] = 0xD805; /* 	2 */
	vals[i++] = 0xDC05; /* 	3 */
	vals[i++] = 0xF005; /* 	4 */
	vals[i++] = 0xF405; /* 	5 */
	vals[i++] = 0xD005; /* 	6 */
	vals[i++] = 0xD405; /* 	7 */
	vals[i++] = 0xE405; /* 	8 */
	vals[i++] = 0xE005; /* 	9 */
	vals[i++] = 0x8405; /* 	- */
	vals[i++] = 0x0005; /* 	. */
	vals[i++] = 0xD605; /* 	* */
	vals[i++] = 0x2105; /* 	SPACE */

	i = 0;
	while(*(chars + i))
	{
		if (*(chars + i) == c)
		{
			return vals[i];
		}
		i++;
	}
	return 0x2100;
}

conout(value)
unsigned int value;
{
#asm
	stx	 3	prtx3
	stx	 2	prtx2
	stx	 1	prtx1

	ld	 3	+2	acc = value
	sto		prtch

	ldx	I3	XXXR3 Get transfer vector address

prt1	equ		*
	libf		wrty0
	dc		/0000
	b		prt1

	libf		wrty0
	dc		/2000
	dc		prtio

prt2	equ		*
	libf		wrty0
	dc		/0000
	b		prt2
	b		prtex
prtio	dc		1
prtch	dc		*-*
prtx3	dc		*-*
prtx2	dc		*-*
prtx1	dc		*-*
prtex	equ		*
	ldx	i1	prtx1
	ldx	i2	prtx2
	ldx	i3	prtx3
	sla		16
#endasm
}

printf(string)
char *string;
{
int conch;

	while(*string != 0)
	{
		conch = astocp(*string);
		conout(conch);
		string++;
	}
}

main()
{
/*
	int n[10];
	int i;

	for (i = 0; i < 10; i++)
	{
		n[i] = i + 1;
	}

	for (i = 0; i < 10; i++)
	{
		wait(n[i]);
	}

	char* x;
	int i;
	int len;
	wait(astocp('P'));
	x = "ABCDEFGHIJ";
	len = strlen(x);
	wait(len);

	for (i = 0; i < len; i++)
	{
		wait(*(x + i));
	}
*/
	printf("HELLO WORLD.");
}
