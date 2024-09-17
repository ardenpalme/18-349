/**
 * @file   swi_stubs.S
 *
 * @brief  Stub functions for newlib and 349libc syscalls
 *
 * @date
 * @author Arden Diakhate-Palme
 */

.cpu cortex-m4
.syntax unified
.section .swi_stub
.thumb

#include "../../kernel/include/svc_num.h"

.global _start
_start:
  bkpt

.global _sbrk
_sbrk:
	svc		#0x0
    ldr     r0, [r13], #1
	bx 		lr

.global _write
_write:
    svc     #0x1
    bx      lr

.global _close
_close:
  bkpt

.global _fstat
_fstat:
    svc     #0x3
    bx      lr

.global _isatty
_isatty:
  bkpt

.global _lseek
_lseek:
  bkpt

.global _read
_read:
    svc     #0x6
    bx      lr

.global _gettimeofday
_gettimeofday:
  bkpt

.global _times
_times:
  bkpt

.global _getpid
_getpid:
  bkpt

.global _kill
_kill:
  bkpt

.global _exit
_exit:
    svc     #0x7
    bx      lr

.global servo_enable
servo_enable:
    svc     #0x16
    bx      lr

.global servo_set
servo_set:
    svc     #0x17
    bx      lr
