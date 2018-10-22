#include <msp430.h>
/**
 * main.c
 *
 * Authors: Dylan Dancel, David Russo
 * Created: 10/15/2018
 * Last Edited: 10/22/2018
 *
 * MILESTONE 1
 * -----------
 *
 * This code allows for the controlling of a common anode RGB LED through the MSP430F5529 processor through the RealTerm interface system.
 * Majority of the control is used through the processors timerA0 capture compare registers.
 *
 */

// SETUP FUNCTIONS CALLED IN MAIN
void externalRGBLEDsetup();                             // sets up RGB LEDS
void timerA0Setup();                                    // sets up timer control for RGB LEDS
void UARTsetup();                                       // sets up the UART interface

// GLOBAL VARIABLES
unsigned char state = 1;                                // state machine initialization: "state" keeps track of how to process the byte
unsigned char i;                                        // variable that tracks how many bytes left to process for USB-to-pin UART interaction
unsigned char j;                                        // tracks how many bytes left to process for pin-to-pin UART interaction

// MAIN FUNCTION
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                           // stop watchdog timer

    externalRGBLEDsetup();                              // calls RGB LED setup function that declares and initializes LEDs
    timerA0Setup();                                     // previews function that declares and initializes timer A0
    UARTsetup();                                        // previews function that declares and initializes setup for UART interaction

    _BIS_SR(GIE);                                       // enables global interrupts
    while(1);                                           // infinite loop (instead of going into low-power mode)
}

// INTERRUPT VECTORS
#pragma vector=TIMER0_A0_VECTOR                         // interrupt service routine for Timer A0 CCR0
__interrupt void TIMER0_A0(void)
{
/**
 * This interrupt triggers every 256us (period of the software PWM)
 * - only turns on the LEDs that have non-zero values in their capture-compare registers
 * - if TA0CCRx == 0, then the corresponding LED color should be OFF 100% of the time     *
 */

    //control for LEDS
    if (TA0CCR1)
        P1OUT |= BIT2;                                  // turns on the LED connected to port 2 (red)
    if (TA0CCR2)
        P1OUT |= BIT3;                                  // turns on the LED connected to port 3 (green)
    if (TA0CCR3)
        P1OUT |= BIT4;                                  // turns on the LED connected to port 4 (blue)


}

#pragma vector=TIMER0_A1_VECTOR                         // interrupt service routine for Timer A0 CCR1, CCR2, CCR3
__interrupt void TIMER0_A1(void)
{
/**
 * This interrupt triggers at the end of each LED's duty cycle
 * - triggers 3x per period, (256us) one for each capture-compare register
 * - a switch statement keeps track of where the interrupt came from
 */
    switch (TA0IV)                                      // turns off LED depending on the timer A0 interrupt vector
    {
        case TA0IV_TA0CCR1:                             // interrupt vector set by CCR1
        {

            if(TA0CCR0 <= TA0CCR1)                      // if red LED has a maximum duty cycle, then do not shut off the LED at all
                break;
            P1OUT &= ~BIT2;                             // turns off color connected to port 2 (red)
            break;
        }
        case TA0IV_TA0CCR2:                             // interrupt vector set by CCR2
        {

            if(TA0CCR0 <= TA0CCR2)                      // if green LED has a maximum duty cycle, then do not shut off the LED at all
                break;
            P1OUT &= ~BIT3;                             // turns off color connected to port 3
            break;
        }
        case TA0IV_TA0CCR3:                             // interrupt vector set by CCR3
        {
            if(TA0CCR0 <= TA0CCR3)                      // if blue LED has a maximum duty cycle, then do not shut off the LED at all
                break;
            P1OUT &= ~BIT4;                             // turns off color connected to port 4
            break;
        }
        default:                                        // safety case: do nothing
        {
            break;
        }
    }
}

// USB-to-pin UART interrupt service routine
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_interrupt(void)
{
/**
 * This interrupt triggers when the CPU receives a byte of data through a USB-to-pin receive buffer
 * - triggers every time a new byte of the package is received
 * - a switch statement keeps track of how to process the data
 *
 * Cases -- the incoming data is processed as...
 * - case 1 : the number of bytes in the incoming package
 * - case 2 : the intensity of the red LED
 * - case 3 : the intensity of the green LED
 * - case 4 : the intensity of the blue LED
 * - case 5 : data is passed along without any processing
 *
 * Registers of interest for UART:
 * - UCA1RXBUF : USCI A1 receive buffer  : 8-bit register that receives data from USB-to-pin
 * - UCA0TXBUF : USCI A0 transmit buffer : 8-bit register that transmits data from pin-to-pin
 * *NOTE* '1' denotes USB-to-pin interaction, and '0' denotes pin-to-pin interaction
 */

    switch (__even_in_range(UCA1IV, 4))
    {
    case 0:
        break;                                          // Vector 0 - no interrupt
    case 2:                                             // Vector 2 - RXIFG
        while (!(UCA1IFG & UCTXIFG));                   // USCI_A1 TX buffer ready?

        switch (state)                                  // CPU processes the data using a state-machine-type architecture
        {
        case 1:                                         // data in this state is processed as the number of bytes in the incoming package
            i = UCA1RXBUF + 1;                          // stores the length to the receiving buffer
            state = 2;                                  // proceeds to process the next byte in state 2

            if(i >= 6)                                  // when package size >= 6 bytes, sends (package size - 3) out through pin-to-pin transmit buffer
            {
                i =  0;                                 // clears package size --> WHY WOULD WE DO THIS?
                UCA0TXBUF = UCA1RXBUF -3;               // pin transmit buffer set equal to the USB receiver buffer register - 3
            }
            else
                UCA1TXBUF = 0x00;                       // else return that the transmit buffer is 0x00
            break;

        case 2:                                         // data in this state is processed as the intensity of the red LED
            TA0CCR1 = UCA1RXBUF;                        // stores the byte of data in CCR1, which sets the duty cycle of the red LED
            i--;                                        // decrements i
            state = 3;                                  // proceeds to process the next byte in state 3
            break;

        case 3:                                         // data in this state is processed as the intensity of the green LED
            TA0CCR2 = UCA1RXBUF;                        // stores the byte of data in CCR2, which sets the duty cycle of the red LED
            i--;                                        // decrements i
            state = 4;                                  // proceeds to process the next byte in state 4
            break;

        case 4:                                         // data in this state is processed as the intensity of the blue LED
            TA0CCR3 = UCA1RXBUF;                        // stores the byte of data in CCR3, which sets the duty cycle of the blue LED
            i--;                                        // decrements i
            if(i == 1)                                  // if i = 1 than return to state 1 else proceed to state 5
                state = 1;                              // proceeds to process the next byte in state 1
            else
                state = 5;                              // proceeds to process the next byte in state 5
            break;

        case 5:                                         // data in this state is passed along without any processing
            i--;                                        // decrements i
            if (i == 1)                                 // return statement
                state = 1;                              // proceeds to process the next byte in state 1
            else
                UCA0TXBUF = UCA1RXBUF;                  // pin-to-pin transmit buffer register is assigned the value of USB-to-pin receive buffer register
            break;

        default:                                        // just in case the value of state became something other than 1-5, do nothing
            break;                                      // in future implementations, we could reset the state to state 1 as an extra safeguard
        }

        break;
    case 4:
        break;                                          // Vector 4 - TXIFG
    default:
        break;
    }
}

// For Pin to Pin interrupt service routine
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_interrupt(void)
{
/**
 * This interrupt triggers when the CPU receives a byte of data through a pin-to-pin receive buffer
 * - almost identical to the "USCI_A1_interrupt"
 * - instead of receiving data from USB, this interrupt focuses on processing data received from pins
 *
 * Cases -- the incoming data is processed as...
 * - case 1 : the number of bytes in the incoming package
 * - case 2 : the intensity of the red LED
 * - case 3 : the intensity of the green LED
 * - case 4 : the intensity of the blue LED
 * - case 5 : data is passed along without any processing
 *
 * Registers of interest for UART:
 * - UCA0RXBUF : USCI A0 receive buffer  : 8-bit register that receives data from pin-to-pin
 * - UCA0TXBUF : USCI A0 transmit buffer : 8-bit register that transmits data from pin-to-pin
 */
    switch (__even_in_range(UCA0IV, 4))
    {
    case 0:
        break;                                          // Vector 0 - no interrupt
    case 2:                                             // Vector 2 - RXIFG
        while (!(UCA0IFG & UCTXIFG));                   // USCI_A1 TX buffer ready?

        switch (state)                                  // CPU processes the data using a state-machine-type architecture
        {
        case 1:                                         // data in this state is processed as the number of bytes in the incoming package
            j = UCA0RXBUF + 1;                          // stores the byte of data in CCR2, which sets the duty cycle of the red LED
            state = 2;                                  // proceeds to process the next byte in state 2
            if(j >= 6)                                  // when package size >= 6 bytes, sends (package size - 3) out through pin-to-pin transmit buffer
                UCA0TXBUF = UCA0RXBUF -3;               // pin transmit buffer set equal to the USB receiver buffer register - 3
            else
                UCA0TXBUF = 0x00;                       // else return that the transmit buffer is 0x00
            break;

        case 2:                                         // data in this state is processed as the intensity of the red LED
            TA0CCR1 = UCA0RXBUF;                        // stores the byte of data in CCR1, which sets the duty cycle of the red LED
            j--;                                        // decrements j
            state = 3;                                  // proceeds to process the next byte in state 3
            break;

        case 3:                                         // data in this state is processed as the intensity of the green LED
            TA0CCR2 = UCA0RXBUF;                        // stores the byte of data in CCR2, which sets the duty cycle of the red LED
            j--;                                        // decrements j
            state = 4;                                  // proceeds to process the next byte in state 4
            break;

        case 4:                                         // data in this state is processed as the intensity of the blue LED
            TA0CCR3 = UCA0RXBUF;                        // stores the byte of data in CCR3, which sets the duty cycle of the blue LED
            j--;                                        // decrements j
            if(j == 1)                                  // if j = 1 than return to state 1 else proceed to state 5
                state = 1;                              // proceeds to process the next byte in state 1
            else
                state = 5;                              // proceeds to process the next byte in state 5
            break;

        case 5:                                         // data in this state is passed along without any processing
            j--;                                        // decrements j
            if (j == 1)                                 // return statement
                state = 1;                              // proceeds to process the next byte in state 1
            else
                UCA0TXBUF = UCA0RXBUF;                  // pin-to-pin transmit buffer register is assigned the value of USB-to-pin receive buffer register
            break;

        default:                                        // just in case the value of state became something other than 1-5, do nothing
            break;                                      // in future implementations, we could reset the state to state 1 as an extra safeguard
        }
        break;
    case 4:
        break;                                          // Vector 4 - TXIFG
    default:
        break;
    }
}




void externalRGBLEDsetup()
{
    P1DIR = (BIT2 | BIT3 | BIT4); // 0b0001_1100        // configures P1.2, P1.3, and P1.4 as outputs
}

void timerA0Setup()
{
    TA0CCTL0 = CCIE;                                    // TA0CCR0 enabled
    TA0CCTL1 = CCIE;                                    // TA0CCR1 enabled
    TA0CCTL2 = CCIE;                                    // TA0CCR0 enabled
    TA0CCTL3 = CCIE;                                    // TA0CCR1 enabled

    TA0CTL = TASSEL_2 | MC_1 | TAIE;                    // Timer A Control set to: SMCLK, Up-mode, no internal divider
                                                        // Value stored in Timer A register initialized to zero

    TA0CCR0 = 255;                                      //TimerA0 CCR0  initialized at 255
    TA0CCR1 = 0;                                        //TimerA0 CCR1 initialized at 0
    TA0CCR2 = 0;                                        //TimerA0 CCR2 initialized at 0
    TA0CCR3 = 0;                                        //TimerA0 CCR3 initialized at 0
}

void UARTsetup()
{
    P3SEL |= BIT3 + BIT4;                               // P3.3,4 = USCI_A0 TXD/RXD
    P4SEL |= BIT4 + BIT5;                               // P4.4,4.5 =USCI_A1 TXD/RXD

    // USB-to-pin UART setup
    UCA0CTL1 |= UCSWRST;                                // **Put state machine in reset**
    UCA0CTL1 |= UCSSEL_2;                               // SMCLK
    UCA0BR0 = 104;                                      // 1MHz, 9600 Baud Rate
    UCA0BR1 = 0;                                        // 1MHz, 9600 Baud Rate
    UCA0MCTL |= UCBRF_0 + UCBRS_1;                      // Modulation UCBRSx=1, UCBRFx=0
    UCA0CTL1 &= ~UCSWRST;                               // **Initialize USCI state machine**
    UCA0IE |= UCRXIE;                                   // Enable USCI_A0 RX interrupt

    // pin-to-pin UART setup
    UCA1CTL1 |= UCSWRST;                                // **Put state machine in reset**
    UCA1CTL1 |= UCSSEL_2;                               // SMCLK
    UCA1BR0 = 104;                                      // 1MHz, 9600 Baud Rate
    UCA1BR1 = 0;                                        // 1MHz 9600 Baud Rate
    UCA1MCTL |= UCBRF_0 + UCBRS_1;                      // Modulation UCBRSx=1, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST;                               // **Initialize USCI state machine**
    UCA1IE |= UCRXIE;                                   // Enable USCI_A1 RX interrupt

}
