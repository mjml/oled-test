#ifndef _ASYNC_UART_H
#define _ASYNC_UART_H

/** 
    A little library that allows for asynchronous serial communication over an arduino's TX0/RX0 uart interface.
		
		Note that the inclusion of the library implies ISR vector handlers for UDRE_vect and RX_vect.

		Usage is simple: call async_uart_puts and/or async_uart_gets.
		When you need to block waiting for a result, call wait_uart_send_ready() or wait_uart_recv_ready().
		When each of these returns, you can call the corresponding async call again.

		The gets/recv version will place characters in a buffer you provide, up until CR,LF,or EOF.
		The returned string will contain the CR or LF char, and will be null-terminated.
		So if you provide a buffer of length 128, the most you get is a string of length 127.
		
		
		@author: Michael Joya [mike@michaeljoya.com]
		
 */

#undef EXTERN
#ifndef _ASYNC_UART_C
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN void init_async_uart (int baud);
EXTERN void async_uart_puts (char* buf, int n);
EXTERN void async_uart_gets (char* buf, int n);

EXTERN char is_uart_send_ready ();
EXTERN char is_uart_recv_ready ();
EXTERN void wait_uart_send_ready();
EXTERN void wait_uart_recv_ready();

#endif
