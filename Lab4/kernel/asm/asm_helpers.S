.cpu cortex-m4
.syntax unified
.thumb

.section .text

.thumb_func
.global enter_user_mode
enter_user_mode:
    ldr   r4, =__psp_stack_bottom
    msr   psp, r4
    b     _crt0
    bx    lr
