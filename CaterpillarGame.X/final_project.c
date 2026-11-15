
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include "LCD.h"
#include "keyboard.h"
#include "characters.h"
#include <time.h>
#include <sys/attribs.h>
#include "ssd.h"
#include "rgbled.h"
#include "config.h"


#pragma config JTAGEN = OFF     
#pragma config FWDTEN = OFF
#pragma config FNOSC = FRCPLL
#pragma config FSOSCEN = OFF
#pragma config POSCMOD = EC
#pragma config OSCIOFNC = ON
#pragma config FPBDIV =     DIV_1
#pragma config FPLLIDIV = DIV_2
#pragma config FPLLMUL = MUL_20
#pragma config FPLLODIV = DIV_1

#define ADC_THRESHOLD 3

#define SYSCLK 96000000  // 96 MHz system clock
#define PBCLK 48000000   // 48 MHz peripheral clock
// Buzzer pin
#define BUZZER_PIN LATBbits.LATB14
#define BUZZER_TRIS TRISBbits.TRISB14

#define APPLE_WEIGHT 45
#define HEART_WEIGHT 15
#define BUTTERFLY_WEIGHT 40

/************************/
/*********ENUMS**********/
/************************/

typedef enum{ZERO_HP = 0x0 , ONE_HP = 0x1, TWO_HP = 0x3,
                THREE_HP = 0x7 , FOUR_HP = 0xF, FIVE_HP = 0x1F ,
                SIX_HP = 0x3F, SEVEN_HP = 0x7F , EIGHT_HP = 0xFF}
            Health_LED; //represents the health displayed on the LEDs (example: 4 health should be 00001111 = 0x0f)
            
typedef enum { APPLE, HEART, BUTTERFLY } ObstacleType;

/***********************************/
/*********GLOBAL VARIABLES**********/
/***********************************/


volatile unsigned char bColR, bColG, bColB;


ObstacleType current_obstacle = APPLE;     
int pause = 0 , finish = 0 , begin = 0;
int keyboard_choice; 
unsigned int timer2_counter = 0;
unsigned int timer2_rounds_counter = 0;

unsigned int old_adc_value;
unsigned int new_adc_value;
int caterpillar_flexing_state=0;
char caterpillar_flex[4] = {2,0,1,3};
char caterpillar_flat[4] = {2,1,1,3};
char butterfly[1] = {4};
char apple[1] = {5};
char heart[1] = {6};

int btn = 0;

int toggle_flag = 0;
int toggle_count = 0;  // Counter to track toggles
int max_toggles = 0;  // Maximum toggles for one beep


int caterpillar_row = LCD_SECOND_ROW;
int obstacle_row = LCD_FIRST_ROW;

int pwm_counter = 0;
int pwm_duty_cycle = 48000;
int timer4_cycles = 0;
int beep_done = 0;

int heart_shift_counter=0;
int apple_shift=16;
int butterfly_shift=16;
int heart_shift=16;

int speed=100;
int health=0;
int apples_counter = 0 ;

/****************************************/
/****************************************/
/*********FUNCTION DECLERATIONS**********/
/****************************************/
/****************************************/
void display_digit(int digit, int anode);

void caterpillarRun(int row);
int configure_Health();
int loseHealth();
int gainHealth();
int displayHealth();

void sendApples();
void sendButterfly();
void sendHeart();
ObstacleType getRandomObstacle();
int getRandomRow();


void PlayGame();
void PauseGame();

void Timer5_Init();
void Timer23_Init();
void Timer4_Init(int frequency);
void __ISR(_TIMER_5_VECTOR, IPL2AUTO) Timer5Handler(void);
void __ISR(_TIMER_3_VECTOR, IPL2AUTO) Timer23SR(void);
void __ISR(_TIMER_4_VECTOR, IPL3SOFT) Timer4ISR(void);

void badSound(float duration);
void setup();
void beep(int frequency, float duration);
void healthPotion(float duration);
void goodSound(float duration);

unsigned int ADC_AnalogRead(unsigned char analogPIN);
void ADC_Init();
void Seven_Segments_Init();
void Audio_Init();
void Init_Switches();
void BTN_Init();

void changeSpeed(unsigned int adc_value);


/*******************************/
/*******************************/
/*********MAIN PROGRAM**********/
/*******************************/
/*******************************/

void main() {
    srand(time(NULL));    // Initialize seed once
    /****INITIALIZATIONS****/
    ADC_Init();           //ADC (Analog Digital Control)
    Init_Switches();      //Switches
    LCD_Init();           //LCD
    TRISA &= 0xff00;      //LEDs
    Keyboard_Init();      //Keyboard
    CG_Init();            //Characters
    SSD_Init();
    //Seven_Segments_Init();
    Audio_Init();         //Audio
    Timer5_Init();        //Timer 1 responsible for keyboard sampling
    Timer23_Init();       //Timer2+3 responsible for LCD display
    /****VARIABLES***/
    old_adc_value = ADC_AnalogRead(2);
    RGBLED_Init();
    BTN_Init();
    //display_digit(7,0);
    configure_Health(); // set the health for the upcoming game

   
    RGBLED_SetValue(0,255,0);//GREEN
    RGBLED_SetValue(255,0,0);//RED
    while(1){

        PlayGame();
        
    }
    
}


/*******************************************/
/*******************************************/
/*********FUCNTION IMPLEMENTATIONS**********/
/*******************************************/
/*******************************************/


void PlayGame(){
    while(finish==0){
        while(health > 0 && pause==0 && finish==0){
            T2CONbits.ON = 1;         // Turn on Timer23
            new_adc_value = ADC_AnalogRead(2); //read analog signal from spinny thing [0 >= adc_value >= 1023]
            if (abs(new_adc_value - old_adc_value) > ADC_THRESHOLD) { 
                //change the speed only when change in the voltage is detected. 
               // this condition prevents noises in the voltage from affecting the speed unnecessarily   
                old_adc_value = new_adc_value;
                changeSpeed(new_adc_value);
            }

        }

//        
        if(pause)
            PauseGame();

        else { finish =1;
                LCD_setControl(1);//clear
                break; //game over
        }
    }
    
    if(health==0){
     LCD_Display( "Zahi said LOSER"  , 15 ,  0 ,  LCD_FIRST_ROW);   
    }
        
    T2CONbits.ON = 0;         // Turn off Timer23
    
}

void PauseGame() {
    T2CONbits.ON = 0;         // Turn off Timer23
    while(pause);             // Stay on pause until changed
    T2CONbits.ON = 0;         // Turn on Timer23
}

    
void Timer5_Init(){
        T5CONbits.ON = 0;     

        PR5 = 0xffff;                        //             set period register, generates one interrupt every 1 ms
        TMR5 = 0;                           //             initialize count to 0
        
        T5CONbits.TCKPS = 0b00;  //TCKPS=0b01              //            1:1 prescale value
        T5CONbits.TGATE = 0;                //             not gated input (the default)
        T5CONbits.TCS = 0;                  //             PCBLK input (the default)
        T5CONbits.ON = 1;     
        IFS0bits.T5IF = 0;       
        IPC5bits.T5IP = 1;        // Set interrupt priority
//             clear interrupt flag
        IEC0bits.T5IE = 1;
        INTCONbits.MVEC=1;//vector interrupt
    IPTMR=0;//INTERRUPT PROXIMITY TIMER REGISTER
    asm("ei");//on interrupt */   
    
    
    

}


//void Timer4_Init(int frequency){
//    T4CONbits.ON = 0;                   //             turn off Timer1
//    int pb_clk = 96000000; // Peripheral Bus Clock (96 MHz)
//    int period = pb_clk / (frequency*4); // Calculate timer period
//    PR4 = 1;                        //            set period register
//    TMR4 = 0;                           //             initialize count to 0
//
//    T4CONbits.TCKPS = 0b00;  //TCKPS=0b01              //            1:1 prescale value
//    T4CONbits.TGATE = 0;                //             not gated input (the default)
//    T4CONbits.TCS = 0;                  //             PCBLK input (the default)
//    IPC4bits.T4IP = 3;                  //             priority
//    IPC4bits.T4IS = 1;                  //             subpriority
//    IFS0bits.T4IF = 0;                  //             clear interrupt flag
//    IEC0bits.T4IE = 1;
//    T4CONbits.ON = 1;                   //             turn on Timer1
//    INTCONbits.MVEC=1;//vector interrupt
//    IPTMR=0;//INTERRUPT PROXIMITY TIMER REGISTER
//    asm("ei");//on interrupt */   
//}
//void __ISR(_TIMER_4_VECTOR, IPL3SOFT) Timer3Handler(void) {
//    pwm_counter++;
//    timer4_cycles++;
//
//    // Toggle the pin based on the duty cycle
//    if (pwm_counter < pwm_duty_cycle) {
//        LATBbits.LATB14 = 1; // High state for active duty cycle
//    } else {
//        LATBbits.LATB14 = 0; // Low state for remaining time
//    }
//    // Reset counter after 100% duty cycle
//    if (pwm_counter >= 96000) {
//        pwm_counter = 0;
//   //     beep_done = 1; // Signal that the beep is complete
//    }
//
//    IFS0bits.T4IF = 0; // Clear the interrupt flag
//}
void __ISR(_TIMER_5_VECTOR, IPL1AUTO) Timer5Handler(void)//Timer 5 handles checking the keyboard
{ 
    keyboard_choice = checkKeyboardOnce();
    switch(keyboard_choice){

        case ONE : // Beep(); 
                    caterpillar_row = LCD_FIRST_ROW;
                    break;
        case TWO :  //Beep(); 
                    caterpillar_row = LCD_SECOND_ROW;
                    break;
        case FOUR : pause = 0;
                    break;
        case FIVE : pause = 1;
                    break;                
                   
        case ZERO: finish=1;
                     break;
        
      //  case F: finish = !finish;//finish
      //          break;
        default :  break;
    }
    //////////////////////////////////////////////////////////////BUTTONS
    btn = PORTBbits.RB0; //read BTNL (LEFT))
    if(btn == 1)
        pause = 1;
    btn = PORTBbits.RB8; // read BTNR (RIGHT))
    if(btn == 1)
        pause = 0;
    btn = PORTFbits.RF0; // read BTNC (Central)
    if(btn == 1)
        caterpillar_row = LCD_FIRST_ROW;
    btn = PORTAbits.RA15; // read BTND (Down)
    if(btn==1)
        caterpillar_row = LCD_SECOND_ROW;
    btn = PORTBbits.RB1; // read BTNU
    if(btn==1)
        finish=1;


    ///////////////////////////////////////////////////////////////// RGB
    static unsigned short sAccR = 0, sAccG = 0, sAccB = 0;
    
    // add 8 bit color values over the accumulators
    sAccR += bColR;
    sAccG += bColG;
    sAccB += bColB;

    // take the 9'th bit (addition carry) as the PDM
    lat_LED8_R = (sAccR & 0x100) ? 1: 0;    
    lat_LED8_G = (sAccG & 0x100) ? 1: 0;
    lat_LED8_B = (sAccB & 0x100) ? 1: 0;
    
    // filter only 8 bits in the accumulator
    sAccR &= 0xFF;
    sAccG &= 0xFF;
    sAccB &= 0xFF;
    
    
    
    IFS0bits.T5IF = 0;  // Clears Timer1 interrupt flag
}


void Timer23_Init(void){
    T2CON = 0;                // Clear Timer2 control register
    T2CONbits.TCKPS = 0b111;      // 1:256 prescaler BIGGER=SLOWER
    PR2=0x0;
    PR3=2; //Timer counts until 0x0002|0000
    
    // Set period to all 32-bits possible
    TMR2=0;                 // Reset Timer2 counter
    TMR3=0;
    
    T2CONbits.T32 = 1;          // mode 32 bit timer
    IFS0bits.T3IF = 0;        // Clear Timer2 interrupt flag
    IPC3bits.T3IP = 2;        // Set interrupt priority
    IEC0bits.T3IE = 1;        // Enable Timer2 interrupt

    T2CONbits.ON = 1;         // Turn on Timer2
    asm("ei");//on interrupt */

}

void __ISR(_TIMER_3_VECTOR, IPL2AUTO) Timer23SR(void) {
    LCD_setControl(1); //clear display
    timer2_counter++;

    caterpillar_flexing_state = !caterpillar_flexing_state; // Toggle animation state
    
    if(timer2_counter==0xffffffff)
        timer2_counter=0;
    


    if (timer2_counter % 16 == 0) { // End of a round
        timer2_rounds_counter++;
        heart_shift=16;
        butterfly_shift=16;
        apple_shift=16;

        // Generate a new obstacle
        int rand_value = rand() % 3; // Generate a random number [0, 2]
        current_obstacle = getRandomObstacle();
        obstacle_row = getRandomRow();
   
    }
        // Send the current obstacle
        switch (current_obstacle) {
            case APPLE:
                sendApples();
                break;
            case HEART:
                sendHeart();
                break;
            case BUTTERFLY:
                sendButterfly();
                break;
        }
        
    // Update the caterpillar display
    if (caterpillar_flexing_state) {
        LCD_Display(caterpillar_flex, 4 , 0, caterpillar_row);
    } else {
        LCD_Display(caterpillar_flat, 4 , 0, caterpillar_row);
    }
    SSD_WriteDigitsGrouped(apples_counter + 6*(apples_counter/10),0);

    IFS0bits.T3IF = 0; // Clear the Timer3 interrupt flag
}

void sendHeart(){
 //   RGBLED_SetValue(0,0,0);
    heart_shift--;
    if(heart_shift==3 && (obstacle_row==caterpillar_row)){ //collision heart&cat
        timer2_counter+=3;
        healthPotion(0.5);
         RGBLED_SetValue(255,192,203);
        if(health<8){
            health++;
            PORTA = (PORTA<<1) | 1;
        }
    }
    else if(heart_shift==-1){
        heart_shift=16;
    }
   // if(heart_shift_counter==100)
    //    heart_shift_counter=0;
    
    //heart_shift_counter++;        
    LCD_Display(heart,1,heart_shift,obstacle_row);
    
}

void sendButterfly(){ 
  // RGBLED_SetValue(0,0,0);
   butterfly_shift--;
    if(butterfly_shift==3 && (obstacle_row==caterpillar_row)){ //collision heart&cat
        timer2_counter+=3;
        health--;
        PORTA>>=1;
        RGBLED_SetValue(255,0,0);
        badSound(0.3);
    }
    else if(butterfly_shift==-1){
        timer2_counter+=3;
    }
    LCD_Display(butterfly,1,butterfly_shift,obstacle_row);
    
}

void sendApples(){
   //RGBLED_SetValue(0,0,0);//
   apple_shift--;
    if(apple_shift==3 && (obstacle_row==caterpillar_row)){ //collision apples&cat
        timer2_counter+=3;
        goodSound(0.3);
         RGBLED_SetValue(0,255,0);
        apples_counter++;
//        Timer4_Init(1000); // Initialize the timer for the beep
//
//        beep_done = 0; // Reset the beep flag
//        while (!beep_done); // Wait for the beep to complete
//        T4CONbits.ON = 0; // Turn off Timer4
    }
    else if(apple_shift==-1){
        apple_shift=16;

    }
   // if(heart_shift_counter==100)
    //    heart_shift_counter=0;
    
    //heart_shift_counter++;        
    LCD_Display(apple,1,apple_shift,obstacle_row);
    
}

void Init_Switches(){
    TRISFbits.TRISF3 = 1; // RF3 (SW0) configured as input
    TRISFbits.TRISF5 = 1; // RF5 (SW1) configured as input
    TRISFbits.TRISF4 = 1; // RF4 (SW2) configured as input
    TRISDbits.TRISD15 = 1; // RD15 (SW3) configured as input
    TRISDbits.TRISD14 = 1; // RD14 (SW4) configured as input
    TRISBbits.TRISB11 = 1; // RB11 (SW5) configured as input
    ANSELBbits.ANSB11 = 0; // RB11 (SW5) disabled analog
    TRISBbits.TRISB10 = 1; // RB10 (SW6) configured as input
    ANSELBbits.ANSB10 = 0; // RB10 (SW6) disabled analog
    TRISBbits.TRISB9 = 1; // RB9 (SW7) configured as input
    ANSELBbits.ANSB9 = 0; // RB9 (SW7) disabled analog
}
unsigned int ADC_AnalogRead(unsigned char analogPIN){
    int adc_val = 0;
    
    IEC0bits.T2IE = 0;
    AD1CHS = analogPIN << 16;       // AD1CHS<16:19> controls which analog pin goes to the ADC
 
    AD1CON1bits.SAMP = 1;           // Begin sampling
    while( AD1CON1bits.SAMP );      // wait until acquisition is done
    while( ! AD1CON1bits.DONE );    // wait until conversion is done
 
    adc_val = ADC1BUF0;
    IEC0bits.T2IE = 1;
    return adc_val;
}
void ADC_Init(){
    TRISBbits.TRISB2 = 1; //set Spinny thing as an input (Analog Digital Control)
    ANSELBbits.ANSB2 = 1; // set spinny thing as analog (Analog Digital Control)
    AD1CON1	=0; 
    AD1CON1bits.SSRC = 7;   // Internal counter ends sampling and starts conversion (auto convert)
    AD1CON1bits.FORM = 0;   // Integer 16-bit
	// Setup for manual sampling
    AD1CSSL	=0;
    AD1CON3	=0x0002;     // ADC Conversion Clock Select bits: TAD = 6 TPB
    AD1CON2	=0;
    AD1CON2bits.VCFG = 0;   // Voltage Reference Configuration bits: VREFH = AVDD and VREFL = AVSS
	// Turn on ADC
    AD1CON1bits.ON = 1;
} 

void changeSpeed(unsigned int adc_value){   
   /*  Timer23 is responsible for the game speed. So we must reconfigure Timer23:
    * adc_value = 0 :   the game is at the slowest setting
    * adc_value = 1023 :   the game is at the fastest setting
    * We will change the PR2 and PR2 value so the Timer counts more/less
    
    *TMR counts till 0x20000 is the slowest
    *TMR counts till 0x05FFF is the fastest
    * 0x20000 - 0x5FFF = 0x1A001 
    * 1024 (Decimal) = 0x400
    * 0x1A001 / 0x400 = 0x68 ~~~~~~~~> change of 1 in the ADC value must change the PR by 0x68      */
    uint32_t new_period = 0x20000 - adc_value * 0x68;
    
    if (new_period < 0x05FFF) {
        new_period = 0x05FFF; // Fastest setting
    }
    else if (new_period > 0x20000) {
        new_period = 0x20000; // Slowest setting
    }
    
    T2CONbits.ON = 0; // Turn off Timer23
    PR2 = (uint16_t)(0xFFFF & new_period); 
    PR3 = (uint16_t)( (new_period >> 16) & 0xFFFF);    //Extract the 16 Most significant bits of the new period and set PR3
    TMR2 = 0; // Reset timer counters
    TMR3 = 0;
    T2CONbits.ON = 1; // Turn on Timer23

    // Set period to all 32-bits possible


}
ObstacleType getRandomObstacle() {
    // Calculate cumulative weights
    const int total_weight = APPLE_WEIGHT + HEART_WEIGHT + BUTTERFLY_WEIGHT;
    
    int rand_value = rand() % total_weight; // Random number [0, total_weight - 1]

    // Determine the obstacle based on weight ranges
    if (rand_value < APPLE_WEIGHT) {
        return APPLE;
    } else if (rand_value < APPLE_WEIGHT + HEART_WEIGHT) {
        return HEART;
    } else {
        return BUTTERFLY;
    }
}
int getRandomRow() {
    return (rand() % 2 == 0) ? LCD_FIRST_ROW : LCD_SECOND_ROW;
}

int configure_Health(){
    if(PORTFbits.RF3){
        PORTA = ONE_HP; //sw0
        health = 1;
    }
    else if(PORTFbits.RF5){ //sw1
        PORTA = TWO_HP;
        health = 2;
    }
     else if(PORTFbits.RF4){ //sw2
         PORTA = THREE_HP;
          health = 3;
     }
    else if(PORTDbits.RD15){ //sw3
        PORTA = FOUR_HP;
        health = 4;
    } 
     else if(PORTDbits.RD14){ //sw4
         PORTA = FIVE_HP;
          health = 5;
     } 
     else if(PORTBbits.RB11){ //sw5
        PORTA = SIX_HP;
        health = 6;
     }
     else if(PORTBbits.RB10){ //sw6
         PORTA = SEVEN_HP;
          health = 7;
     }
     else if(PORTBbits.RB9){ //sw7
         PORTA = EIGHT_HP;
            health = 8;
     }
     else{ //default
         PORTA = FIVE_HP;
          health = 5;
     }
}

void Audio_Init(){
    TRISBbits.TRISB14 = 0;// RB14 (Audio Out) configured as output
    ANSELBbits.ANSB14 = 0; // RB14 (Audio Out) disabled analog
}


//void AppleEatingSound() {
//    // First "crunch" (short, rapid beeps)
//    for (int i = 0; i < 3; i++) {
//        LATBbits.LATB14 = 1;
//        for (int j = 0; j < 500; j++); // Short high-pitched beep
//        LATBbits.LATB14 = 0;
//        for (int j = 0; j < 500; j++);
//    }
//
//   // delay(100); // Short pause between crunches
//
//    // Second "crunch" (longer, less rapid beeps)
////    for (int i = 0; i < 2; i++) {
////        LATBbits.LATB14 = 1;
////        for (int j = 0; j < 1000; j++); // Longer, deeper beep
////        LATBbits.LATB14 = 0;
////        for (int j = 0; j < 1000; j++);
////    }
//
//    //delay(200); // Longer pause after the second crunch
////
////    // Third "crunch" (mixed pattern of short and long beeps)
////    for (int i = 0; i < 4; i++) {
////        LATBbits.LATB14 = 1;
////        for (int j = 0; j < ((i % 2 == 0) ? 700 : 1200); j++); // Alternate short and long beeps
////        LATBbits.LATB14 = 0;
////        for (int j = 0; j < ((i % 2 == 0) ? 700 : 1200); j++);
////    }
//
//  //  delay(300); // Final pause to simulate chewing
//}
//
//void delay(int cycles) {
//    for (int i = 0; i < cycles; i++) {
//        for (int j = 0; j < 1000; j++); // Adjust 1000 for finer or coarser delays
//    }
//}





void __ISR(_TIMER_4_VECTOR, IPL3SOFT) Timer4ISR(void) {
    if (toggle_count < max_toggles) {
        BUZZER_PIN ^= 1;  // Toggle buzzer pin
        toggle_count++;   // Increment toggle count
    } else {
        T4CONbits.ON = 0;  // Stop Timer 2
    }
    IFS0bits.T4IF = 0;  // Clear Timer 2 interrupt flag
}

void Timer4_Init(int frequency) {
    T4CON = 0x0;  // Stop the timer and clear the control register
    TMR4 = 0;     // Clear timer register
    PR4 = PBCLK / (frequency * 2) - 1;  // Set period register
    T4CONbits.TCKPS = 0;  // Set prescaler to 1
    IFS0bits.T4IF = 0;    // Clear interrupt flag
    IEC0bits.T4IE = 1;    // Enable Timer 2 interrupt
    IPC4bits.T4IP = 3;    // Set interrupt priority
    T2CONbits.ON = 1;     // Start Timer 2
}

void setup() {
    // Configure buzzer pin as output
    BUZZER_TRIS = 0;
    BUZZER_PIN = 0;
    // Configure Timer 2 for interrupts at a specific frequency
    Timer4_Init(2000);  // 2 kHz toggling frequency
    // Enable multi-vector interrupts
    INTCONbits.MVEC = 1;
    __builtin_enable_interrupts();
}
void beep(int frequency, float duration) {
    max_toggles = (int)(duration * frequency * 2);  // Calculate total toggles
    toggle_count = 0;  // Reset toggle counter
    Timer4_Init(frequency);  // Configure timer for desired frequency
    T4CONbits.ON = 1;  // Start Timer 2
}
void goodSound(float duration) {
    // Define a deterministic pattern of frequencies (in Hz) and burst durations (in seconds)
    const int frequencies[] = {600, 800, 1000, 1200, 1400, 1600};
    const float burst_durations[] = {0.05, 0.04, 0.03, 0.03, 0.04, 0.05};
    const int pauses[] = {10, 15, 20, 15, 10, 5};  // Pause durations in milliseconds

    int pattern_size = sizeof(frequencies) / sizeof(frequencies[0]);
    int repeat_count = (int)(duration / 0.3);  // Repeat the pattern to match the total duration

    for (int i = 0; i < repeat_count; i++) {
        for (int j = 0; j < pattern_size; j++) {
            beep(frequencies[j], burst_durations[j]);  // Play the frequency for the burst duration
            while (T4CONbits.ON);  // Wait for the beep to finish

            // Add a pause after the burst
            for ( int k = 0; k < pauses[j] * 1000; k++);  // Simple delay loop
        }
    }
}
void badSound(float duration) {
    // Define a deterministic pattern of frequencies (in Hz) and burst durations (in seconds)
    const int frequencies[] = { 800, 700, 600,  500, 400, 300};
    const float burst_durations[] = {0.03, 0.02, 0.04, 0.03, 0.05, 0.02};
    const int pauses[] = {20, 30, 15, 25, 20, 10};  // Pause durations in milliseconds

    int pattern_size = sizeof(frequencies) / sizeof(frequencies[0]);
    int repeat_count = (int)(duration / 0.2);  // Repeat the pattern to match the total duration

    for (int i = 0; i < repeat_count; i++) {
        for (int j = 0; j < pattern_size; j++) {
            beep(frequencies[j], burst_durations[j]);  // Play the frequency for the burst duration
            while (T4CONbits.ON);  // Wait for the beep to finish

            // Add a pause after the burst
            for (int k = 0; k < pauses[j] * 1000; k++);  // Simple delay loop
        }
    }
}
void healthPotion(float duration) {
    // Define a deterministic pattern of frequencies (in Hz) and burst durations (in seconds)
    const int frequencies[] = {400, 500, 600, 700, 800};
    const float burst_durations[] = {0.1, 0.08, 0.06, 0.05, 0.04};
    const int pauses[] = {10, 10, 10, 10};  // Pause durations in milliseconds (between tones)

    int pattern_size = sizeof(frequencies) / sizeof(frequencies[0]);
    int total_cycles = (int)(duration / 0.5);  // Each cycle is approximately 0.5 seconds

    for (int cycle = 0; cycle < total_cycles; cycle++) {
        for (int i = 0; i < pattern_size; i++) {
            beep(frequencies[i], burst_durations[i]);  // Play the frequency for the burst duration
            while (T4CONbits.ON);  // Wait for the beep to finish

            if (i < pattern_size - 1) {
                // Add a pause only if it's not the last tone
                for (int j = 0; j < pauses[i] * 1000; j++);  // Simple delay loop
            }
        }
    }
}

void BTN_Init(){
    
    TRISBbits.TRISB1 = 1; // RB1 (BTNU) configured as input
    ANSELBbits.ANSB1 = 0; // RB1 (BTNU) disabled analog
    TRISBbits.TRISB0 = 1; // RB1 (BTNL) configured as input
    ANSELBbits.ANSB0 = 0; // RB1 (BTNL) disabled analog
    TRISFbits.TRISF4 = 1; // RF0 (BTNC) configured as input
    TRISBbits.TRISB8 = 1; // RB8 (BTNR) configured as input
    ANSELBbits.ANSB8 = 0; // RB8 (BTNR) disabled analog
    TRISAbits.TRISA15 = 1; // RA15 (BTND) configured as input

}