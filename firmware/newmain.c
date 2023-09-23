//////////////////////////////////////////////////////////////////////

#include <xc.h>
#include "config.h"

#pragma warning disable 520


//////////////////////////////////////////////////////////////////////

#define TICK_RATE 0xF1E0

//////////////////////////////////////////////////////////////////////

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

//////////////////////////////////////////////////////////////////////

u8 pwm_b0;
u8 pwm_b1;
u8 pwm_b2_10;

u8 rotary_state = 0;
u8 rotary_store = 0;

//////////////////////////////////////////////////////////////////////

#define valid_rotary_state_mask 0x6996

// then, to just get one increment per cycle:

// 11 .. 10 .. 00 is one way
// 00 .. 10 .. 11 is the other way

// So:
// E8 = 11,10 .. 10,00  --> one way
// 2B = 00,10 .. 10,11  <-- other way

s8 rotary_update(u8 inputs) {

    rotary_state = ((rotary_state << 2) | inputs) & 0xf;

    // many states are invalid (noisy switches) so ignore them

    if ((valid_rotary_state_mask & (1 << rotary_state)) != 0) {

        // certain state patterns mean rotation happened
        rotary_store = (u8) ((rotary_store << 4) | rotary_state);

        if (rotary_store == 0xe8) {
            return 1;
        } else if (rotary_store == 0x2b) {
            return -1;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////

void __interrupt() ISR(void) {

    if (T2IF != 0) {
        CCP1CONbits.DC1B1 = pwm_b0;
        CCP1CONbits.DC1B0 = pwm_b1;
        CCPR1L = pwm_b2_10;
        T2IF = 0;
    }
}

//////////////////////////////////////////////////////////////////////

void main(void) {

    // init Timer 0 prescaler 8, 2MHz = 976.5625 Hz tick rate
    PS0 = 0;
    PS1 = 1;
    PS2 = 0;
    PSA = 0;
    T0CS = 0;

    // init Timer 2 PWM @ ~1.95KHz on PORT C5 with 10 bit duty cycle
    TRISC5 = 1;
    PR2 = 255;
    CCP1M3 = 1;
    CCP1M2 = 1;
    T2IF = 0;
    T2CKPS1 = 0;
    T2CKPS0 = 1;
    TMR2ON = 1;
    TRISC5 = 0;

    // setup GPIO inputs C0, C1, C2
    ANSEL = 0;
    TRISC0 = 1;
    TRISC1 = 1;
    TRISC2 = 1;

    // enable interrupts
    GIE = 1;
    PEIE = 1;

    s16 brightness = 500;

    // main loop
    while (1) {

        // wait for Timer 0 tick
        while (T0IF == 0) {
        }
        T0IF = 0;

        // update brightness from rotary encoder
        u8 bits = (u8) (((u8) RC2 << 1) | RC1);
        s8 dir = rotary_update(bits);
        brightness += (u16) (dir << 5);
        if (brightness < 24) {
            brightness = 24;
        } else if (brightness > 980) {
            brightness = 980;
        }
        
        u16 x = 1023 - ((u16)(((u32)brightness * (u32)brightness) >> 10));

        // set PWM values for Timer 2 ISR
        T2IE = 0;
        pwm_b0 = (u8) ((x >> 1) & 1);
        pwm_b1 = (u8) (x & 1);
        pwm_b2_10 = (u8) (x >> 2);
        T2IE = 1;
    }
}
