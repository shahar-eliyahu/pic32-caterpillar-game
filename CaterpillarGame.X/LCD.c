#include "LCD.h"
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>

//int keyboard_choice;


void LCD_Display(char* str  , int len, int shift , int row){
    //LCD_setControl(1); //clear display
    int i;
    //program first row
    if(row==LCD_FIRST_ROW){
        LCD_setControl(LCD_FIRST_ROW+shift);
        for(i=0; i<len ; i++) //
            LCD_setData(str[i]);
    }
    
    else if(row==LCD_SECOND_ROW){
        //program second row
        LCD_setControl(LCD_SECOND_ROW+shift);
        for(i=0; i<len ; i++) //
                 LCD_setData(str[i]);
    }
    else return;
    }
void LCD_setControl(char command){
    PORTE = command;
    PORTBbits.RB15 = 0; // RS = 0 (control mode)
    PORTDbits.RD4=1; //enable
    PORTDbits.RD4=0;
    busy();

}
void LCD_setData(char command){
    PORTE = command;
    PORTBbits.RB15 = 1; // RS = 1 (data mode)
    PORTDbits.RD4=1;
    PORTDbits.RD4=0;
    busy();
}

void busy(void)
{
    char RD,RS;
    int STATUS_TRISE;
    int portMap;
    RD=PORTDbits.RD5;
    RS=PORTBbits.RB15;
    STATUS_TRISE=TRISE;
	PORTDbits.RD5 = 1;//w/r
	PORTBbits.RB15 = 0;//rs 
    portMap = TRISE;
	portMap |= 0x80;
	TRISE = portMap;
    do
    {
        PORTDbits.RD4=1;//enable=1
        PORTDbits.RD4=0;//enable=0
    }
    while(PORTEbits.RE7); // BF ?????
        PORTDbits.RD5=RD; 
        PORTBbits.RB15=RS;
        TRISE=STATUS_TRISE;   
}

void LCD_Init(){
 /*********** LCD ***********/

    TRISBbits.TRISB15 = 0; // RB15 (DISP_RS) set as an output
    ANSELBbits.ANSB15 = 0; // disable analog functionality on RB15 (DISP_RS)
    TRISDbits.TRISD5 = 0; // RD5 (DISP_RW) set as an output
    TRISDbits.TRISD4 = 0; // RD4 (DISP_EN) set as an output
    //TRISEbits.TRISE0 = 1; // RE0 (DB0) set as input (change 1 to 0 for
    TRISE&=0xff00;
    ANSELEbits.ANSE2 = 0;
    ANSELEbits.ANSE4 = 0;
    ANSELEbits.ANSE5 = 0;
    ANSELEbits.ANSE6 = 0;
    PORTBbits.RB15=0;//rs=0
    PORTDbits.RD5=0;//w=0
    ANSELEbits.ANSE7 = 0;
    
    char control[7]={0x38,0x38,0x38,0xe,0x6,0x1,0x40};
    for(int i=0 ; i<7 ; i++)
        LCD_setControl(control[i]);
    
    

    
}