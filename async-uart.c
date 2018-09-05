#define _ASYNC_UART_C
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include "async-uart.h"

static volatile char* uart_tx_fifo = NULL;
static volatile char* uart_tx_fifo_end = NULL;
static volatile char* uart_rx_fifo = NULL;
static volatile char* uart_rx_fifo_end = NULL;
static volatile char rx_rd = 1;

void init_async_uart (int baud)
{
	cli();
	
	unsigned int ubrr = (F_CPU / 16 / baud) - 1;
	
	// baud rate selector
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	// frame configuration: one stop bit, no parity (this could be parameterized)
	// rx enable, tx enable
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	
  sei();
}

char is_uart_send_ready () {
	return (uart_tx_fifo < uart_tx_fifo_end);
}

char is_uart_receiving () {
	return uart_rx_fifo < (uart_rx_fifo_end-1);
}

void wait_uart_send_ready()
{
	while (is_uart_send_ready()) {
		asm("nop;");
	}
}

void wait_uart_recv_ready()
{
	while (is_uart_receiving() && rx_rd==0) {
		asm("nop;");
	}
}

void async_uart_puts (char* buf, int n)
{
	uart_tx_fifo = buf;
	uart_tx_fifo_end = buf + n;
	UCSR0B |= (1<<UDRIE0);
}

void async_uart_gets (char* buf, int n)
{
	uart_rx_fifo = buf;
	uart_rx_fifo_end = buf + n;
	rx_rd = 0;
	UCSR0B |= (1<<RXCIE0);
}

ISR(USART_UDRE_vect)
{
	char c = 0;
	if (uart_tx_fifo < uart_tx_fifo_end) {
		c = *uart_tx_fifo++;
		UDR0 = c;
	} else {
		UCSR0B &= ~(1<<UDRIE0);
	}
}

ISR(USART_RX_vect)
{
	char c = 0;
	c = UDR0;

	if (c != 0 && uart_rx_fifo < (uart_rx_fifo_end-1)) {
		*uart_rx_fifo++ = c;
		rx_rd = (c == '\n' || c == '\r');
	}
	if (c==0 || rx_rd || uart_rx_fifo >= uart_rx_fifo_end-1) {
		*uart_rx_fifo = (char)0;
		rx_rd = 1;
		UCSR0B &= ~(1<<RXCIE0);
	}
}

