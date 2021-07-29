
#ifndef INPUTS_H
#define INPUTS_H

#define DPAD_UP 0
#define DPAD_LEFT 1
#define DPAD_RIGHT 2
#define DPAD_DOWN 3
#define BTN_X 4
#define BTN_SQUARE 5
#define BTN_CIRCLE 6
#define BTN_TRIANGLE 7
#define BTN_L1 8
#define BTN_L2 9
#define BTN_R1 10
#define BTN_R2 11
#define BTN_START 12
#define BTN_SELECT 13

int in_btn_held(int btn);
int in_btn_pressed(int b);
int in_btn_release(int b);
int in_frame_start();
unsigned char *in_joystick_state();

void sys_pad_poll();
int sys_init_input();

#endif
