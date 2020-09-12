
#include <tamtypes.h>
#include <libpad.h>
#include <kernel.h>
#include <stdio.h>
#include <loadfile.h>

#include "inputs.h"

#define ERR_IF_NULL(c, s, ...) (c)

static char pad_buffer[256] __attribute__((alligned(64)));

#define R_SIO2MAN "rom0:SIO2MAN"
#define R_PADMAN "rom0:PADMAN"


int sys_init_input()
{
    printf("Loading SIF modules\n");
    ERR_IF_NULL(SifLoadModule(R_SIO2MAN, 0, 0), "Failed to load %s sif", R_SIO2MAN);
    ERR_IF_NULL(SifLoadModule(R_PADMAN, 0, 0), "Failed to load %s sif", R_PADMAN);
    printf("Init pad 0\n");
    ERR_IF_NULL(padInit(0), "Failed to initialize controller (pad)");
    ERR_IF_NULL(padPortOpen(0, 0, &pad_buffer), "Failed to open pad 0:0");
    return 1;
}

int sys_init()
{
    sys_init_input();
    return 1;
}

static struct padButtonStatus pad_read_space = {0};

// based on PS2SDK examples
int pad_wait(int port, int slot, int tries)
{
    int now;
    int prev = -1;
    now = padGetState(port, slot);
    if(now == PAD_STATE_DISCONN) {
        // pad disconnected
        return -1;
    }
    while((now != PAD_STATE_STABLE) && (now != PAD_STATE_FINDCTP1)) {
        prev = now;
        now = padGetState(port, slot);
        tries--;
        if(tries == 0) {
            // failed to get anything;
            break;
        }
    }
    return 0;
}


static int btn_press[20];
static int btn_release[20];
static int btn_held[20];
static unsigned char joysticks[4] = {0,0,0,0};

int in_btn_held(int b) {
    return btn_held[b];
}
int in_btn_pressed(int b) {
    return btn_press[b];
}
int in_btn_release(int b) {
    return btn_release[b];
}

unsigned char * in_joystick_state() {
    return joysticks;
}

int in_frame_start() 
{
    for(int i = 0; i < 20; i++) {
        btn_press[i] = 0;
        btn_release[i] = 0;
    }
    return 0;
}
void EVENT_IF(int c, int b)
{
    if(!c) {
        return;
    }
    if(btn_held[b]) {
        btn_release[b] = 1;
        btn_held[b] = 0;
    } else {
        btn_press[b] = 1;
        btn_held[b] = 1;
    }
}


static int prev_inputs;
void sys_pad_poll()
{
    if(pad_wait(0,0,10) < 0) {
        // pad 0:0 not ready
        return;
    }
    if(padRead(0, 0, &pad_read_space) != 0) {
        int pad = 0xffff ^ pad_read_space.btns;
        int c = pad ^ prev_inputs;
        prev_inputs = pad;
        EVENT_IF(c & PAD_LEFT, DPAD_LEFT);
        EVENT_IF(c & PAD_RIGHT, DPAD_RIGHT);
        EVENT_IF(c & PAD_UP, DPAD_UP);
        EVENT_IF(c & PAD_DOWN, DPAD_DOWN);
        EVENT_IF(c & PAD_CROSS, BTN_X);
        EVENT_IF(c & PAD_SQUARE, BTN_SQUARE);
        EVENT_IF(c & PAD_TRIANGLE, BTN_TRIANGLE);
        EVENT_IF(c & PAD_CIRCLE, BTN_CIRCLE);
        EVENT_IF(c & PAD_L1, BTN_L1);
        EVENT_IF(c & PAD_R1, BTN_R1);
        joysticks[0] = pad_read_space.ljoy_h;
        joysticks[1] = pad_read_space.ljoy_v;
        joysticks[2] = pad_read_space.rjoy_h;
        joysticks[3] = pad_read_space.rjoy_v;
    }
}

void sys_error()
{
    for(int i = 0; i < 900000; i++) {}
    Exit(1);
}


