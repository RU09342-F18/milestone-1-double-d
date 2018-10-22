#include <msp430.h>
/**
 * main.c
 *
 *  Author: Dylan Dancel, David Russo
 *  Created: 10/15/2018
 *  Last Edited: 10/19/2018
 *
 * DESCRIPTION OF MILESTONE 1
 *
 * This code allows for the controlling of a common anode RGB LED through the MSP430F5529 processor through the RealTerm interface system.
 * Majority of the control is used through the processors timerA0 capture compare registers.
 *
 */

void externalRGBLEDsetup();  //function that sets up RGB LEDS
void timerA0Setup();         //timer control for RGB LEDS
void UARTsetup();            //sets up the UART interface
unsigned char state = 1;     //state machine initialization

unsigned char i;             //variable used to store data from UART for usb to pin interaction
unsigned char j;             //variable used to store data from UART for pin to pin interaction
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;         // stop watchdog timer

    externalRGBLEDsetup();            // declares and initializes LEDs (function below) 0
    timerA0Setup();                   // declares and initializes timer A0 (function below)
    UARTsetup();                      //declares and initializes setup for UART interaction

    _BIS_SR(GIE);                     //enables global interrupts




    while(1); //infinite loop

}

#pragma vector=TIMER0_A0_VECTOR                     // interrupt service routine for Timer 0 CCR0
__interrupt void TIMER0_A0(void)
{

    //control for LEDS
    if (TA0CCR1)
            P1OUT |= BIT2;                              // Turns on the LED connected to port 2(red)
    if (TA0CCR2)
            P1OUT |= BIT3;                              //Turns on the LED connected to port 3(green)
     if (TA0CCR3)
            P1OUT |= BIT4;                              //Turns on the LED connected to port 4(blue)


}

#pragma vector=TIMER0_A1_VECTOR                 // interrupt service routine for Timer 0 CCR1, CCR2, CCR3
__interrupt void TIMER0_A1(void)
{

    switch (TA0IV)                              //turns off LED depending on the timerA0 interrupt vector
    {
        case TA0IV_TA0CCR1:                     //interrupt vector set by CCR1
        {

            if(TA0CCR0 <= TA0CCR1)
            break;

            P1OUT &= ~BIT2;                     //turns off color connected to port 2

            break;
        }
        case TA0IV_TA0CCR2:                     //interrupt vector set by CCR2
        {

            if(TA0CCR0 <= TA0CCR2)
            break;

            P1OUT &= ~BIT3;                     //turns off color connected to port 3
            break;
        }
        case TA0IV_TA0CCR3:                     //interrupt vector set by CCR3
        {
            if(TA0CCR0 <= TA0CCR3)
            break;

            P1OUT &= ~BIT4;                     //turns off color connected to port 4
            break;
        }
        default:
        {
            break;
        }
    }
}

// For USB to PIN interrupt service routine
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_interrupt(void)
{

    switch (__even_in_range(UCA1IV, 4))
    {
    case 0:
        break;                             // Vector 0 - no interrupt
    case 2:                                // Vector 2 - RXIFG
        while (!(UCA1IFG & UCTXIFG));      // USCI_A1 TX buffer ready?

        switch (state)
        {

        case 1:

                                            //detects the length about to be stored from the data coming in

            i = UCA1RXBUF + 1;              //stores the length to the receiving buffer


            state = 2;                      //proceed to state to 2



            if(i >= 6)                      //when package size is greater than or equal to 6 set transmit buffer for the pins to receiving buffer from the usb
            {

             i =  0;                        //clears package size

            UCA0TXBUF = UCA1RXBUF -3;       //pin transmit buffer set equal to the USB receiver buffer register
            }
            else
                UCA1TXBUF = 0x00;           //else return that the transmit buffer is 0x00

            break;

        case 2:                             //port LED receiving data(red)

            TA0CCR1 = UCA1RXBUF;            //stores 2 bytes of data in CCR1 and to the LED through port 2
            i--;                            //decrements
            state = 3;
            break;
        case 3:                             //port LED receiving data(green)

            TA0CCR2 = UCA1RXBUF;            //stores 2 bytes of data in CCR2 and to the LED through port 3
            i--;
            state = 4;

            break;
        case 4:                             //port LED receiving data(blue)

            TA0CCR3 = UCA1RXBUF;            //store 2 bytes data in CCR3 and to the LED through port 4
            i--;

            if(i == 1)                      //if i = 1 than return to state 1 else proceed to state 5
            state = 1;
            else
            state = 5;
            break;
        case 5:
            i--;
            if (i == 1) //return statement
            {

                state = 1; //return to state 1
            }

            else
            {
                UCA0TXBUF = UCA1RXBUF; //transmit from pins is equal to receiver buffer from usb

            }
            break;

        default:
            break;
        }

        break;
    case 4:
        break;                             // Vector 4 - TXIFG
    default:
        break;
    }
}

// For Pin to Pin interrupt service routine
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_interrupt(void)
{

    switch (__even_in_range(UCA0IV, 4))
    {
    case 0:
        break;                             // Vector 0 - no interrupt
    case 2:                                // Vector 2 - RXIFG
        while (!(UCA0IFG & UCTXIFG));      // USCI_A1 TX buffer ready?

        switch (state)
        {

        case 1:

                                            //detects the length about to be stored from the data coming in

            j = UCA0RXBUF + 1;              //stores the length to the receiving buffer


            state = 2;                      //change state to 2


            if(j >= 6)
            {

            UCA0TXBUF = UCA0RXBUF -3;       //transmit buffer register set equal to register buffer from the pins - 3
            }
            else
                UCA0TXBUF = 0x00;           //else return 0 to the transmit buffer

            break;

        case 2:                             //port LED receiving data(red)

            TA0CCR1 = UCA0RXBUF;            //stores 2 bytes of data in CCR1 and to the LED through port 2
            j--;                             //decrements
            state = 3;                      //proceeds to next state
            break;
        case 3:                             //port LED receiving data(green)

            TA0CCR2 = UCA0RXBUF;            //stores 2 bytes of data in CCR2 and to the LED through port 3
            j--;
            state = 4;

            break;
        case 4:                             //port LED receiving data (blue)
            TA0CCR3 = UCA0RXBUF;            //store 2 bytes data in CCR3 and to the LED through port 4
            j--;

            if(j == 1)                      //if j = 1 return to state 1
            state = 1;
            else                            //else j proceeds to state 5
            state = 5;
            break;
        case 5:
            j--;
            if (j == 1) //return statement
            {

                state = 1;
            }

            else
            {
                UCA0TXBUF = UCA0RXBUF; //sets transmit buffer of the pins equal to receiver buffer of the pins


            }
            break;

        default:
            break;
        }

        break;
    case 4:
        break;                             // Vector 4 - TXIFG
    default:
        break;
    }
}




void externalRGBLEDsetup()
{
    P1DIR = (BIT2 | BIT3 | BIT4); // 0b0001_1100      // configures P1.2, P1.3, and P1.4 as outputs
}

void timerA0Setup()
{
    TA0CCTL0 = CCIE;                                // TA0CCR0 enabled
    TA0CCTL1 = CCIE;                                // TA0CCR1 enabled
    TA0CCTL2 = CCIE;                                // TA0CCR0 enabled
    TA0CCTL3 = CCIE;                                // TA0CCR1 enabled

    TA0CTL = TASSEL_2 | MC_1 | TAIE;                 // Timer A Control set to: SMCLK, Up-mode, no internal divider
                                                     // Value stored in Timer A register initialized to zero

    TA0CCR0 = 255;                                   //TimerA0 CCR0  initialized at 255
    TA0CCR1 = 0;                                     //TimerA0 CCR1 initialized at 0
    TA0CCR2 = 0;                                     //TimerA0 CCR2 initialized at 0
    TA0CCR3 = 0;                                     //TimerA0 CCR3 initialized at 0
}

void UARTsetup()
{
    P3SEL |= BIT3 + BIT4;                     // P3.3,4 = USCI_A0 TXD/RXD
    P4SEL |= BIT4 + BIT5;                     // P4.4,4.5 =USCI_A1 TXD/RXD

    UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 104;                            // 1MHz, 9600 Baud Rate
    UCA0BR1 = 0;                              // 1MHz, 9600 Baud Rate
    UCA0MCTL |= UCBRF_0 + UCBRS_1;            // Modulation UCBRSx=1, UCBRFx=0
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt

    UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA1CTL1 |= UCSSEL_2;                     // SMCLK
    UCA1BR0 = 104;                            // 1MHz, 9600 Baud Rate
    UCA1BR1 = 0;                              // 1MHz 9600 Baud Rate
    UCA1MCTL |= UCBRF_0 + UCBRS_1;            // Modulation UCBRSx=1, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    UCA1IE |= UCRXIE;                         // Enable USCI_A1 RX interrupt

}
