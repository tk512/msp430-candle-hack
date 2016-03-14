/*
 * Candle Hack by TK (2016-03-01)
 *
 * Uses a Software UART hack as it works with a msp430g2231
 * microcontroller that does not have a hardware UART
 *
 * 9600 bps serial communication. Can be tested with:
 *
 * $ screen /dev/ttyACM0 9600
 *
 * Type '1': turn candle on
 * Type '0': turn candle off
 *
 */
#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Candle on P1.0
 */
#define CANDLE          BIT0
#define CANDLE_DIR      P1DIR
#define CANDLE_OUT      P1OUT

/*
 * Software UART hack, based on github.com/wendlers/msp430-softuart
 */
#define TXD BIT1
#define RXD BIT2
#define FCPU 			1000000
#define BAUDRATE 		9600
#define BIT_TIME        (FCPU / BAUDRATE)
#define HALF_BIT_TIME   (BIT_TIME / 2)

static volatile uint8_t bit_count;

static volatile unsigned int tx_byte;

static volatile unsigned int rx_byte;

static volatile bool is_receiving = false;

static volatile bool has_received = false;

void uart_init(void) {
    P1SEL |= TXD;
    P1DIR |= TXD;

    P1IES |= RXD;      // RXD Hi/lo edge interrupt
    P1IFG &= ~RXD;     // Clear RXD (flag) before enabling interrupt
    P1IE  |= RXD;      // Enable RXD interrupt
}

bool uart_getc(uint8_t *c) {
    if (!has_received) {
        return false;
    }

    *c = rx_byte;
    has_received = false;

    return true;
}

void uart_putc(uint8_t c) {
    tx_byte = c;

    while(is_receiving); 					// Wait for RX completion

    CCTL0 = OUT; 							// TXD Idle as Mark
    TACTL = TASSEL_2 + MC_2; 				// SMCLK, continuous mode

    bit_count = 0xA; 						// Load Bit counter, 8 bits + ST/SP
    CCR0 = TAR; 							// Initialize compare register

    CCR0 += BIT_TIME; 						// Set time till first bit
    tx_byte |= 0x100; 						// Add stop bit to tx_byte (which is logical 1)
    tx_byte = tx_byte << 1; 				// Add start bit (which is logical 0)

    CCTL0 = CCIS_0 + OUTMOD_0 + CCIE + OUT;// Set signal, intial val, enable interrupts

    while ( CCTL0 & CCIE ); 				// Wait for previous TX completion
}

void uart_puts(const char *str)
{
    if(*str != 0) uart_putc(*str++);
    while(*str != 0) uart_putc(*str++);
}

/* ISR for RXD */
void port1_isr(void) __attribute__((interrupt (PORT1_VECTOR)));
void port1_isr(void) {
    is_receiving = true;

    P1IE &= ~RXD; 					// Disable RXD interrupt
    P1IFG &= ~RXD; 					// Clear RXD IFG (interrupt flag)

    TACTL = TASSEL_2 + MC_2; 		// SMCLK, continuous mode
    CCR0 = TAR; 					// Initialize compare register
    CCR0 += HALF_BIT_TIME; 			// Set time till first bit
    CCTL0 = OUTMOD_1 + CCIE; 		// Disable TX and enable interrupts

    rx_byte = 0; 					// Initialize rx_byte
    bit_count = 9; 					// Load Bit counter, 8 bits + start bit
}

/* ISR for TXD and RXD */
void timera0_isr(void) __attribute__((interrupt (TIMERA0_VECTOR)));
void timera0_isr(void) {
    if(!is_receiving) {
        CCR0 += BIT_TIME; 						// Add Offset to CCR0
        if ( bit_count == 0) { 					// If all bits TXed
            TACTL = TASSEL_2; 					// SMCLK, timer off (for power consumption)
            CCTL0 &= ~ CCIE ; 					// Disable interrupt
        } else {
            if (tx_byte & 0x01) {
                CCTL0 = ((CCTL0 & ~OUTMOD_7 ) | OUTMOD_1);  //OUTMOD_7 defines the 'window' of the field.
            } else {
                CCTL0 = ((CCTL0 & ~OUTMOD_7 ) | OUTMOD_5);  //OUTMOD_7 defines the 'window' of the field.
            }

            tx_byte = tx_byte >> 1;
            bit_count --;
        }
    } else {
        CCR0 += BIT_TIME; 						// Add Offset to CCR0

        if ( bit_count == 0) {

            TACTL = TASSEL_2; 					// SMCLK, timer off (for power consumption)
            CCTL0 &= ~ CCIE ; 					// Disable interrupt

            is_receiving = false;

            P1IFG &= ~RXD; 						// clear RXD IFG (interrupt flag)
            P1IE |= RXD; 						// enabled RXD interrupt

            if ( (rx_byte & 0x201) == 0x200) { 	// Validate the start and stop bits are correct
                rx_byte = rx_byte >> 1; 			// Remove start bit
                rx_byte &= 0xFF; 				// Remove stop bit
                has_received = true;
            }
        } else {
            if ( (P1IN & RXD) == RXD) { 		// If bit is set?
                rx_byte |= 0x400; 				// Set the value in the rx_byte
            }
            rx_byte = rx_byte >> 1; 				// Shift the bits down
            bit_count --;
        }
    }
}


void candle_init() {
    CANDLE_DIR |= CANDLE;
    CANDLE_OUT &= ~CANDLE;
}

int main(void) {
    WDTCTL = WDTPW + WDTHOLD; 	// Stop WDT
    BCSCTL1 = CALBC1_1MHZ;      // Set range
    DCOCTL = CALDCO_1MHZ;       // SMCLK = DCO = 1MHz

    candle_init();
    uart_init();

    __enable_interrupt();

    uint8_t c;

    while(1) {
        if(uart_getc(&c)) {
            if(c == '1') {
                CANDLE_OUT |= CANDLE;
                uart_puts("on");
                uart_putc('\n');
                uart_putc('\r');
            } else if (c == '0') {
                CANDLE_OUT &= ~CANDLE;
                uart_puts("off");
                uart_putc('\n');
                uart_putc('\r');
            }
        }
    }
}
