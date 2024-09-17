/**
 * @file syscall.h
 *
 * @brief  System calls for kernel implementation (function prototypes )
 *
 * @date   3/27/20
 *
 * @author Arden Diakhate-Palme
 */

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

void *sys_sbrk(int incr);

int sys_write(int file, char *ptr, int len);

int sys_read(int file, char *ptr, int len);

void sys_exit(int status);

int sys_servo_enable(uint8_t channel, uint8_t enabled);

int sys_servo_set(uint8_t channel, uint8_t angle);

#endif /* _SYSCALLS_H_ */
