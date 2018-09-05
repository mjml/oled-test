#define F_CPU 8000000L

#include <util/delay.h>
#include <stdio.h>
#include "async-uart.h"
#include <avr/io.h>
#include <string.h>

int main ()
{
	init_async_uart(9600);

	char bufr[64];
	char bufr2[64] = "test";
	bufr[0] = 0;

	snprintf(bufr, 64, "Ok, let's play\r\n");
	async_uart_puts(bufr,strlen(bufr));
	wait_uart_send_ready();

	snprintf(bufr, 64, "Simon says:\r\n");
	async_uart_puts(bufr,strlen(bufr));
	wait_uart_send_ready();

	while (1) {
		bufr2[0] = 0;
		async_uart_gets(bufr2,64);
		wait_uart_recv_ready();
		int n = strlen(bufr2);
		if (bufr2[n-1] == '\r') bufr2[n-1] = 0;

		snprintf(bufr, 64, "Simon says [%s]\r\n", bufr2);
		async_uart_puts(bufr,strlen(bufr));
		wait_uart_send_ready();

	}
	
}
