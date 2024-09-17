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
#include <kernel.h>

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
    void    *arg1; /**< reg used for passing external info */
} stack_frame_t;

/** @brief stack frame pushed by the SVC instruction using the PSP
 *  @param [psp] PSP process stack pointer (pointing to the just-pushed exception frame)
 */
void svc_c_handler(void *psp){
    stack_frame_t *s= (stack_frame_t*)psp;

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
        case SVC_SERVO_ENABLE:
            s->r0= sys_servo_enable((uint8_t)s->r0, (uint8_t)s->r1);
            break;
        case SVC_SERVO_SET:
            s->r0= sys_servo_set((uint8_t)s->r0, (uint8_t)s->r1);
            break;
        default:
            break;
    }
}
