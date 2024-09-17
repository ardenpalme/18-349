@		 Tips
@ 1. The code in this state is almost impossible to understand, trying to do
@ 	 is not a good use of your time.
@ 2. Try to do small optimizations that reduce the number of instructions.
@ 3. You have 14 GP registers. You don't need so many stack operations.
@ 4. Once you have reduced the size of the code to a manageable level, you
@    will have to understand it and restructure the code.
@ 5. The programmers manual is you friend, if you are unsure about what an
@    insturction does, consult the manual!
@ 5. HINT: The source code had a lot of things declared volatile, things that
@		 did not have to be volatile. Consider removing unnecessary loads.
@ 6. If you are attempting the bonus, talk to a TA/post privately on piazza 
@    before you do.

	.syntax unified
	.cpu cortex-m4
	.fpu softvfp
	.thumb

	.global	optimize_me
	.thumb_func
optimize_me:
	adds	r2, r0, #996		@ get last ele address
	movw	r3, #0xbeef
	movt	r3, #0xdead

.L9:
	ldr	r8, [r0]		@ get array values high 2 bytes in r3, low in r8
	lsr	r5, r8, #16
	uxth	r8, r8
	add	r4, r5, r5, LSL #2
	sub	r4, r8, r4
	add 	r4, r4, r4, LSL #5	
	add	r5, r3, r4		
	str	r5, [r0], #4		@arra[i] <-- 0xdeadbeef-33(r8-5(r3)) 
	cmp	r0, r2
	ble	.L9
	adds	r7, r7, #28		@end of program
	bx 	lr
