#include <u8x8.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <stdarg.h>


#define I2C_BUF_SIZE 32
#include "i2c.h"


#include "async-uart.h"


u8x8_t g_u8x8;

char serbuf[80] = "Test\r\n";

void print (char* fmt, ...) {
	va_list args;
	va_start(args,fmt);
	vsnprintf(serbuf,80,fmt,args);
	va_end(args);
	async_uart_puts(serbuf,strlen(serbuf));
	wait_uart_send_ready(); // block
}

void println (char* fmt, ...) {
	va_list args;
	va_start(args,fmt);
	vsnprintf(serbuf,80,fmt,args);
	strncat(serbuf,"\r\n",80);
	va_end(args);
	async_uart_puts(serbuf,strlen(serbuf));
	wait_uart_send_ready(); // block
}

void checkState(char* str)
{
	if (!str) {
		println("mode=%d,swerr=%d,hwstat=%d",i2c_mode,i2c_swerror,i2c_hwstatus);
	} else {
		println("%s: mode=%d,swerr=%d,hwstat=%#02x",str,i2c_mode,i2c_swerror,i2c_hwstatus);
	}
}


uint8_t oled_byte_cb (u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
  uint8_t *data;
	PORTB |= _BV(PORTB5);
	
  switch(msg)
  {
    case U8X8_MSG_BYTE_INIT:
			checkState("byte_init");
			i2cm_init_easy(I2C_SPD_40KHZ);
      break;
    case U8X8_MSG_BYTE_SET_DC:
			checkState("set_dc");
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
			checkState("start_transfer");
      break;
    case U8X8_MSG_BYTE_SEND:
			println("send %d bytes", arg_int);
			i2cm_write(0x3c, arg_int, (uint8_t*)arg_ptr);
			_delay_us(5000);
		  checkState("after 5ms");
			// ... done. Everything else is handled by an interrupt service routine
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
			checkState("end_transfer");
			// Since we're working asynchronously, nothing really to do here...
      break;
    default:
      return 0;
  }
  return 1;
}

uint8_t oled_gpio_cb (u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
}

int main ()
{
	DDRB |= _BV(DDB5);
	PORTB &= ~_BV(DDB5);
	
	init_async_uart(9600);
	checkState("init_async_uart");

	u8x8_Setup(&g_u8x8,u8x8_d_ssd1306_128x64_noname,u8x8_cad_ssd13xx_i2c,oled_byte_cb,oled_gpio_cb);
	
	u8x8_SetI2CAddress(&g_u8x8,0x3c<<1);
	checkState("setI2CAddress");
	
	u8x8_InitDisplay(&g_u8x8);
	checkState("initDisplay");
	
	/*
	u8x8_ClearDisplay(&g_u8x8);
	checkState("clearDisplay");
	
	u8x8_SetPowerSave(&g_u8x8,0);
	checkState("setPowerSave");

	u8x8_SetFont(&g_u8x8,u8x8_font_5x7_f);
	checkState("setFont");
	*/

	return 0;
}
