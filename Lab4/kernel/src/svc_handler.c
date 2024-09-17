/**
 * @file   svc_handler.c
 *
 * @brief  parses SVC exceptions, and calls apprpriate syscalls
 *
 * @date   3/27/20
 *
 * @author Arden Diakhate-Palme
 */

#include <stdint.h>
#include <debug.h>
#include <svc_num.h>
#include <syscall.h>
#include <nvic.h>
#include <kernel.h>
#include "syscall_thread.h"
#include "syscall_mutex.h"

/** @brief stack frame pushed by the SVC instruction using the PSP 
 */ 
typedef struct {
    uint32_t r0;   /**< reg r0*/
    uint32_t r1;   /**< reg r1*/
    uint32_t r2;   /**< reg r2*/
    uint32_t r3;   /**< reg r3*/
    uint32_t r12;  /**< reg r12*/
    uint32_t lr;   /**< reg lr*/
    uint32_t pc;   /**< reg pc*/
    uint32_t xPSR; /**< reg xPSR*/
    void    *arg1; /**< user stack frame for passing external info */
    // void    *arg2; /**< user stack frame for passing external info */
} stack_frame_t;

/** @brief stack frame pushed by the SVC instruction using the PSP
 *  @param [psp] PSP process stack pointer (pointing to the just-pushed exception frame)
 */
void svc_c_handler(void *psp){
    stack_frame_t *s= (stack_frame_t*)psp;
    nvic_clear_pending(11);

    uint32_t *tmp= (uint32_t *)(s->pc-2);
    void *tmp1;
    uint8_t svc_number= *(tmp);
    //call different syscalls based on SVC number as specified in svc_num.h
    switch (svc_number){
        case SVC_EXIT:
            sys_exit(s->r0);
            break;
        case SVC_READ:
            s->r0= sys_read(s->r0, (char *)s->r1, s->r2);
            break;
        case SVC_WRITE:
            s->r0= sys_write(s->r0, (char *)s->r1, s->r2);
            break;
        case SVC_SBRK:
            tmp1= sys_sbrk(s->r0);
            s->arg1= tmp1;
            break;
        case SVC_FSTAT:
            break;

        /**Thread and Mutex syscalls */
        case SVC_THR_INIT:
            s->r0= sys_thread_init(s->r0, s->r1, s->arg1, s->r3, s->r12);
            break;
        case SVC_THR_CREATE: 
            s->r0= sys_thread_create((void*)(s->r0), s->r1, s->r2, s->r3, s->arg1);
            break;
        case SVC_THR_KILL:
            sys_thread_kill();
            break;
        case SVC_SCHD_START:
            s->r0= sys_scheduler_start(s->r0);
            break;
        case SVC_PRIORITY:
            s->r0= sys_get_priority();
            break;
        case SVC_TIME:
            s->r0= sys_get_time();
            break;
        case SVC_THR_TIME:
            s->r0= sys_thread_time();
            break;
        case SVC_WAIT:
            sys_wait_until_next_period();
            break;
        case SVC_MUT_INIT:
            tmp1= (void*)sys_mutex_init(s->r0);
            s->arg1= tmp1;
            break;
        case SVC_MUT_LOK:
            sys_mutex_lock((kmutex_t*)(s->arg1));
            break;
        case SVC_MUT_ULK:
            sys_mutex_unlock((kmutex_t*)(s->arg1));
            break;

        /** Deprecated Servo Functions */
        case SVC_SERVO_ENABLE:
            s->r0= -1; 
            break;
        case SVC_SERVO_SET:
            s->r0= -1;
            break;

        default:
            break;
    }
    return;
}
