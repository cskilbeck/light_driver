//////////////////////////////////////////////////////////////////////

#include <xc.h>
#include "config.h"

#pragma warning disable 520

//////////////////////////////////////////////////////////////////////

typedef signed char s8;
typedef unsigned char u8;
typedef __bit bit;

//////////////////////////////////////////////////////////////////////

u8 const gamma[] = { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,
                     1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   4,   4,   4,
                     4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,
                     11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,
                     20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,  29,  30,  30,  31,  32,
                     33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,  43,  43,  44,  45,  46,  47,  48,
                     49,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,
                     68,  69,  70,  71,  73,  74,  75,  76,  77,  78,  79,  81,  82,  83,  84,  85,  87,  88,  89,  90,
                     91,  93,  94,  95,  97,  98,  99,  100, 102, 103, 105, 106, 107, 109, 110, 111, 113, 114, 116, 117,
                     119, 120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135, 137, 138, 140, 141, 143, 145, 146, 148,
                     149, 151, 153, 154, 156, 158, 159, 161, 163, 165, 166, 168, 170, 172, 173, 175, 177, 179, 181, 182,
                     184, 186, 188, 190, 192, 194, 196, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
                     223, 225, 227, 229, 231, 234, 236, 238, 240, 242, 244, 246, 248, 251, 253, 255 };

//////////////////////////////////////////////////////////////////////

#define MIN_BRIGHTNESS 32
#define MAX_BRIGHTNESS 252

#define BRIGHTNESS_INCREMENT 4

#define POWER_ON 1
#define POWER_OFF 0

//////////////////////////////////////////////////////////////////////

bit power_on_off;

u8 button_check;
u8 button_history;

u8 rotary_state;
u8 rotary_store;

u8 brightness;

//////////////////////////////////////////////////////////////////////

void main(void)
{
    // init Timer 0 prescaler 8 @ 2MHz = 976.5625 Hz tick rate (250000 / 256)
    PS0 = 0;
    PS1 = 1;
    PS2 = 0;
    PSA = 0;
    T0CS = 0;

    // init Timer 2 PWM @ ~1.95KHz on PORT C5 with 10 bit duty cycle
    TRISC5 = 1;
    PR2 = 255;
    CCP1CONbits.DC1B1 = 0;
    CCP1CONbits.DC1B0 = 0;
    CCP1M3 = 1;
    CCP1M2 = 1;
    T2IF = 0;
    T2CKPS1 = 0;
    T2CKPS0 = 1;
    TMR2ON = 1;
    TRISC5 = 0;

    // setup GPIO inputs C0, C1, C2 for rotary encoder + button
    ANSEL = 0;
    TRISC0 = 1;
    TRISC1 = 1;
    TRISC2 = 1;

    // output for relay mosfet on C3
    RC3 = 1;
    TRISC3 = 0;

    // output for debug led on A5
    RA5 = 1;
    TRISA5 = 0;

    brightness = 128;

    power_on_off = POWER_OFF;

    button_check = 0;
    button_history = 0;

    rotary_state = 0;
    rotary_store = 0;

    CCPR1L = brightness;
    RC3 = power_on_off;

    // main loop
    while(1) {

        // wait for Timer 0 tick
        while(T0IF == 0) {
        }
        T0IF = 0;

        // read the button with super heavy debounce
        button_check += 1;
        if(button_check == 32) {
            button_check = 0;

            RA5 = (bit)(1 - RA5);

            button_history = (u8)(button_history << 1) | RC0;

            // pressed
            if(button_history == 0xfe) {
                on_off = !power_on_off;
                RC3 = power_on_off;
            }
        }

        // update brightness from rotary encoder
        if(power_on_off == POWER_ON) {

            rotary_state &= 0x3;
            rotary_state <<= 2;
            if(RC2) {
                rotary_state |= 2;
            }
            if(RC1) {
                rotary_state |= 1;
            }

            // valid_rotary_state_mask 0x6996

            static const u8 valid[16] = { 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0 };

            if(valid[rotary_state]) {

                if(rotary_store == 0xe && rotary_state == 0x8 && brightness < MAX_BRIGHTNESS) {
                    brightness += BRIGHTNESS_INCREMENT;

                } else if(rotary_store == 0x2 && rotary_state == 0xb && brightness > MIN_BRIGHTNESS) {
                    brightness -= BRIGHTNESS_INCREMENT;
                }
                rotary_store = rotary_state;
            }
            CCPR1L = gamma[brightness];
        }
    }
}
