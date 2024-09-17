/**
 * @file servok.h
 *
 * @brief
 *
 * @date 03/30/21
 *
 * @author Neville Chima
 */

#ifndef _SERVOK_H_
#define _SERVOK_H_
#define SERVO_FREQ 1600


/** @brief data structure enabled globally to change and track state of servo motor
 * @param en - whether the servo is enabled to rotate
 * @param ticks - count cap for how long servo pin needs to be set high
 * @param on_count - counter to time how long a pulse has been set
 * @param off_count - counter to time how long no pulse has been set
 * @param high_max - counter reload value for length of a pulse
*/
struct servo_ctrl {
  int en;
  int state;
  uint32_t ticks;
  uint32_t on_count;
  uint32_t off_count;
};

void servo_init();

int sys_servo_enable(uint8_t channel, uint8_t enabled);

int sys_servo_set(uint8_t channel, uint8_t angle);

void moveServo(struct servo_ctrl *servo, int channel);

#endif /* _SERVOK_H_ */
