#include "keyboard.h"

void Keyboard_Init(){
    TRISCbits.TRISC2=0;//RC2
    TRISCbits.TRISC1=0;//RC1
    TRISCbits.TRISC4=0;//RC4
    TRISGbits.TRISG6=0;//RG6
    ANSELGbits.ANSG6=0;//???????
    TRISCbits.TRISC3 =1;//RC3
    CNPUCbits.CNPUC3;
    TRISGbits.TRISG7=1;//RG7
    ANSELGbits.ANSG7=0;
    CNPUGbits.CNPUG7;
    TRISGbits.TRISG8=1;//RG8
    ANSELGbits.ANSG8=0;//???????
    CNPUGbits.CNPUG8;//
    TRISGbits.TRISG9=1;//RG9
    ANSELGbits.ANSG9=0;//???????
    CNPUGbits.CNPUG9;//????? 
}
int in_y(int a)
{ 
    int j=1,flag=0;
    if(!PORTCbits.RC3)
    { flag=1;
        j=1;
    }
   else if (!PORTGbits.RG7)
   { flag=1;
        j=2;
   }
   else  if(!PORTGbits.RG8)
        { flag=1;
        j=3;
        }
   else if(!PORTGbits.RG9)
      { flag=1; 
        j=4;
   }
    if(flag==0)
        return(0xff);
    else
       return(j|(a<<4));

}
int checkKeyboardLoop(){
    int xy;
    static int i;
         while(1)
        {
            i=1;

            PORTCbits.RC2=1;
            PORTCbits.RC1=1;
            PORTCbits.RC4=1;
            PORTGbits.RG6=1;
            
            i=1;
            PORTCbits.RC2=0;
            xy=in_y(1);
            if (xy!=0xff)
               break;
            
            PORTCbits.RC2=1;
            i=2;
            PORTCbits.RC1=0;
            xy=in_y(2);
            if (xy!=0xff)
               break;
            
            PORTCbits.RC1=1;
            i=3;
            PORTCbits.RC4=0;
            xy=in_y(3);
            if (xy!=0xff)
               break;
            
            PORTCbits.RC4=1;
            i=4;
            PORTGbits.RG6=0;
            xy=in_y(4);
            if (xy!=0xff)
               break;
            
            PORTGbits.RG6=1;
        }
    return xy;
}
int checkKeyboardOnce (){
    static int i;
    int xy;
        PORTCbits.RC2=1;
        PORTCbits.RC1=1;
        PORTCbits.RC4=1;
        PORTGbits.RG6=1;

        i=1;
        PORTCbits.RC2=0;
        xy=in_y(1);
        if (xy!=0xff)
            return xy;

        PORTCbits.RC2=1;
        i=2;
        PORTCbits.RC1=0;
        xy=in_y(2);
        if (xy!=0xff)
            return xy;

        PORTCbits.RC1=1;
        i=3;
        PORTCbits.RC4=0;
        xy=in_y(3);
        if (xy!=0xff)
            return xy;

        PORTCbits.RC4=1;
        i=4;
        PORTGbits.RG6=0;
        xy=in_y(4);
        if (xy!=0xff)
            return xy;

        PORTGbits.RG6=1;
    
    return xy;

}
    