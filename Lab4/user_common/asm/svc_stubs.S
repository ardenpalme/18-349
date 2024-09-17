/**
 * @file   swi_stubs.S
 *
 * @brief  Stub functions for newlib and 349libc syscalls
 *
 * @date
 * @author
 */

.cpu cortex-m4
.syntax unified
.section .swi_stub
.thumb

#include "../../kernel/include/svc_num.h"

.global _sbrk
_sbrk:
  svc	  #0x0
  ldr     r0, [r13], #1
  bx 	  lr

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

.global _exit
_exit:
    svc     #0x7
    bx      lr

.type thread_init, %function
.global thread_init
thread_init:
    ldr     r12, [r13], #1 
    str     r2, [r13], #1
    svc     #0x9
    bx      lr

.type thread_create, %function
.global thread_create
thread_create:
    svc     #0xA
    bx      lr

.type thread_kill, %function
.global thread_kill
thread_kill:
    svc     #0xB
    bx      lr

.type scheduler_start, %function
.global scheduler_start
scheduler_start:
    svc     #0xC
    bx      lr

.type mutex_init, %function
.global mutex_init
mutex_init:
    svc     #0xD
    ldr     r0, [r13], #1
    bx      lr

.type mutex_lock, %function
.global mutex_lock
mutex_lock:
    str     r0, [r13], #1
    svc     #0xE
    bx      lr
    

.type mutex_unlock, %function
.global mutex_unlock
mutex_unlock:
    str     r0, [r13], #1
    svc     #0xF
    bx      lr

.type wait_until_next_period, %function
.global wait_until_next_period
wait_until_next_period:
    svc     #0x10
    bx      lr

.global get_time
get_time:
    svc     #0x11
    bx      lr

.global get_priority
get_priority:
    svc     #0x13
    bx      lr

.global thread_time
thread_time:
    svc     #0x14
    bx      lr

.global servo_enable
servo_enable:
    svc     #0x16
    bx      lr

.global servo_set
servo_set:
    svc     #0x17
    bx      lr

/* The following stubs are not required to be implemented */

.global _start
_start:
  bkpt

.global _kill
_kill:
  bkpt

.global _gettimeofday
_gettimeofday:
  bkpt

.global _times
_times:
  bkpt

.global _getpid
_getpid:
  bkpt

.thumb_func
.global idle_default
idle_default:
  wfi
  b idle_default

