*list
*print symbol table
*********************************************************
* OVERVIEW OF C Program Operation
*
* The following operation occur to execute a c program
*	XXent is called to setup the environment.
*		high memory is deteremined
*		stack is set up (xr2)
*		routine Main gets called
*
*	XXpsh pushes the value in ACC onto the stack
*	XXpop pops a value from the stack into ACC
*	XXcal calls a routine storing return address on stack
*	XXrtn return from a routine based on next stack entry
*	XXsst setup stack frame sets xr3 -> return address on stack
*********************************************************

	abs
	org		/1000
*********************************************************
* initial values
*********************************************************
XXha	dc		*-*		highest memory address for stack
XX3	dc		3		three
XX1	dc		1		one
XXBX	DC		*-*		"BX" Register
XXXR3	dc		*-*		XR3 at entry

*********************************************************
* entry point from system
*
* This is where it all begins. Upon entry, the program:
*	1. Determines the highest memory address
*	2. Stores it (Used to ensure no stack underflow)
*	3. Inits SP (XR2)
*	4. Set up initial stack frame (none, or zero).
*	5. Transfers control to main.
*
* For now, when main returns, XXent simply waits.
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 28Apr2002 Initial implementation
* RVF 21DEC2007 Use push to go to main
* RVF 27DEC2007 Init stack frame pointer to zero
*********************************************************
XXent	equ		*		main enty point
	stx	 3	XXXR3		save entry XR3
	bsi		XXgha		get highest address in XR2
	stx	 2	XXha		save base stack address
	ldx	 3	0		old stack frame = none
	
	bsi	L	XXcal		call ...
	dc		main		.. main
	
XXend	wait				wait
	exit				.. return to dm2
	
*********************************************************
* Get highest memory address
*
* determines the stack base (high memory)
*	1. Saves a value in (4K, 8K, 16K, 32K) -1 
*	2. Return high address (stack pointer) in XR2
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 28Apr2002 Initial implementation
*********************************************************
	
XXgha	dc		*-*		get highest address
	ld	L	/7fff		get high memory value
	sto		XXsvh		save high memory value
	
	ldx	L1	XX3		xr1 = 3 (loop counter)
	ld		XX4k		acc = 4k
	
XXgh1	equ		*		loop limit
	s		XX1		acc = nK - 1
	sto	L	2		XR2 -> potential high addr (pha)
	rte		16		.. save pha in ext
		
	ld	 2	0		acc = *pha
	sto		XXsv		XXsv = acc

	sla		16		acc = 0
	sto	 2	0		*pha = 0
	
	stx	L2	/7fff		save pha in highest addr
	
	ld	 2	0		acc = *pha
	
	s	L	2		q. pha = *pha?
	bz		XXgh5		a. yes .. we have hi mem addr (hma)
	
	ld		XXsv		load the saved value
	sto	 2	0		.. save in pha
	
	rte		16		acc = pha
	a		XX1		acc = memory size
	sla		1		.. multiply by 2
	
	mdx	 1	-1		q. loop finished?
	mdx		XXgh1		a. no .. continue looping
	
XXgh5	equ		*		come here with XR2 = hma
	mdx	 2	-128		back off 256 words 	
	mdx	 2	-128		...	
	ld		XXsvh		reload hma value
	sto	 2	0		.. save in hma
	bsc	I	XXgha		.. and return to caller

XX4k	dc		/1000		4K memory
XXsv	dc		*-*		save for old value
XXsvh	dc		*-*		save for high memory
XXlpc	dc		*-*		loop counter
XXbad	dc		/bad0		bad stack

*********************************************************
* push accumulator
*	1. push accumulator on stack (XR2->current stack)
*	2. decrement XR2
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 28Apr2002 Initial implementation
*********************************************************
	
XXpsh	dc		*-*		push acc
	sto	 2	0		push acc
	mdx	 2	-1		.. next location
	bsc	I	XXpsh		.. and return
	
*********************************************************
* pop accumulator
*	1. increment XR2 (stack pointer)
*	2. load acc with *XR2
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 28Apr2002 Initial implementation
*********************************************************
	
XXpop	dc		*-*		pop acc
	ld	L	XXha		get hma
	
	s	L	2		q. stack empty?
	bnz		XXpo1		a. no .. ok to pop
	
	ld		XXbad		show bad stack
	b		XXend		.. and end program
	
XXpo1	equ		*		stack ok
	mdx	 2	+1		increment stack pointer
	ld	 2	0		.. and pop acc
	bsc	i	XXpop		.. and return to caller

*********************************************************
* pop BX
*	1. Save ACC
*	2. pop ACC
*	3. Save ACC to BX
*	4. Restore ACC
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 24DEC2007 Initial implementation
*********************************************************
	
XXpoB	dc		*-*		pop BX
	sto		XSPOB		Save ACC
	bsi		XXpop		.. pop ACC
	sto	L	XXBX		.. save in "BX"
	ld		XSPOB		Restore ACC
	bsc	i	XXpoB		.. return to caller
	
XSpoB	DC		*-*		ACC Save Word

*********************************************************
* call a routine
*	1. acc -> return address - 1
*	2. acc += 1
*	3. push acc
*	4. iar = routine address (branch to routine)
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 21DEC2007 Initial implementation
*********************************************************
	
XXcal	dc		*-*		call routine
	ld		XXcal		acc -> return address - 1
	a		XX1		acc -> return address
	bsi		XXpsh		.. push return value
*
	ld	I	XXcal		acc -> routine
	sto		XXcal		.. save goto address
	bsc	I	XXcal		.. and go to routine
	
*********************************************************
* return from a routine
*	1. pop return address
*	2. save in call address
*	3. go to return address
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 21DEC2007 Initial implementation
*********************************************************
	
XXrtn	dc		*-*		call routine
XXrtx	sto		XXrt1		save acc
	bsi		XXpop		acc = return address
	sto		XXrtn		.. save it
	ld		XXrt1		.. restore acc value
	bsc	I	XXrtn		.. and return to caller
XXrt1	dc		*-*		acc saved here

*********************************************************
* ssf - setup stack frame
*	1. Set xr3 -> word after return address
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 21DEC2007 Initial implementation
* RVF 27DEC2007 Made XR3 (stack frame) point at old sf address
*********************************************************
XXssf	dc		*-*
	sto		XXss1		save acc
	ld	L	3		acc = xr3
	stx	L2	3		xr3 = SP (new frame)
	bsi		XXpsh		.. save old stack frame
	ld		XXss1		restore acc
	bsc	i	XXssf		return
XXss1	dc		*-*

*********************************************************
* rsf - restore stack frame
*	1. Set xr3 -> previous stack frame
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 21DEC2007 Initial implementation
*********************************************************
XXrsf	dc		*-*
	sto		XXrs1		save acc
	bsi		XXpop		acc = previous stack frame
	sto	L	3		.. restore old sf value
	ld		XXrs1		restore acc
	bsc	i	XXrsf		return
XXrs1	dc		*-*

*********************************************************
* cmr - setup for return from compare by stack
*	This allows a common routine for true/false
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 30DEC2007 Initial implementation
*********************************************************
XXcmr	dc		*-*		Setup compare return
	sto		XXcms		save acc
	ldx	I1	XXcmr		XR1 -> return addr
	ld	 1	-2		acc = return address
	bsi		XXpsh		save ultimate return addr
	ld		XXcms		restore acc
	bsc	i	XXcmr		return to caller
XXcms	dc		*-*

*********************************************************
* eq - compare acc : bx, return NZ if EQUAL
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 30DEC2007 Initial implementation
*********************************************************
XXeq	dc		*-*
	bsi		XXcmr		setup return by stack
	eor	l	XXBX		q. same?
	bz		XXTRU		a. yes.. return true
	b		XXFLS		.. otherwise .. false
	
XXTRV	dc		-1		true value
XXTRU	equ		*		return true
	LD		XXTRV		.. acc = true value
	B		XXRTX		.. return (using stack)

XXFLS	equ		*		return false
	sla		16		.. zero acc
	b		XXRTX		.. return (using stack)


*********************************************************
* NE - compare acc : bx, return Z if NOT EQUAL
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 30DEC2007 Initial implementation
*********************************************************
XXne	dc		*-*
	bsi		XXcmr		setup return by stack
	eor	l	XXBX		.. compare values
	bz		XXRTX		.. return 0 if equal
	b		XXTRU

*********************************************************
* LT - compare acc : bx, return NZ if XXBX LT ACC
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 30DEC2007 Initial implementation
*********************************************************
XXlt	dc		*-*
	bsi		XXcmr		setup return by stack
	s	l	XXBX		.. subtract BX
	bp		XXTRU		.. if LT, true
	b		XXFLS		.. else .. false

*********************************************************
* GT - compare acc : bx, return NZ if XXBX GT ACC
*********************************************************
* Modifications
*
* WHO DDMMMYYYY Description
* --- --------- -----------
* RVF 01DEC2008 Initial implementation
*********************************************************
XXgt	dc		*-*
	bsi		XXcmr		setup return by stack
	s	l	XXBX		.. subtract BX
	bn		XXTRU		.. if GT, true
	b		XXFLS		.. else .. false

*********************************************************
* START OF GENERATED CODE 
*********************************************************
