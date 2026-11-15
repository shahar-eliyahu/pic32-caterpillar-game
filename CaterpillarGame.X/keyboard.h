#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <sys/attribs.h>
#include <string.h>
#include <p32xxxx.h>



// Enum for values 0 to 9 and A to F for keyboard choice
typedef enum {
    ZERO = 0x41,
    ONE = 0x44,
    TWO = 0x34,
    THREE = 0x24,
    FOUR = 0x43,
    FIVE = 0x33,
    SIX= 0x23,
    SEVEN= 0x42,
    EIGHT= 0x32,
    NINE= 0x22,
    A = 0x14,
    B= 0x13,
    C= 0x12,
    D= 0x11,
    E= 0x21,
    F= 0x31,
    NO_CHOICE= 0xFF
    
} HexValues;

int checkKeyboardLoop();
int checkKeyboardOnce();
void Keyboard_Init();
