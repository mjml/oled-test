#include <u8x8.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <stdarg.h>

#include "i2c.h"
#include "async-uart.h"


u8x8_t g_u8x8;
uint8_t oled_audit;

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

void lightPause ()
{
	PORTB |= _BV(PORTB5);
	_delay_us(2000);
	PORTB &= ~_BV(PORTB5);
}

void checkState(char* str)
{
	if (!str) {
		println("mode=%d,swerr=%d,hwstat=0x%02x",i2c_mode,i2c_swerror,i2c_hwstatus);
	} else {
		println("%s: mode=%d,swerr=%d,hwstat=0x%02x",str,i2c_mode,i2c_swerror,i2c_hwstatus);
	}
}


uint8_t oled_byte_cb (u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)
{
  uint8_t *data;
	
  switch(msg)
  {
    case U8X8_MSG_BYTE_INIT:
			if (oled_audit) checkState("byte_init");
			i2cm_init_at_speed(I2C_SPD_100KHZ);
      break;
    case U8X8_MSG_BYTE_SET_DC:
			if (oled_audit) checkState("set_dc");
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
			if (oled_audit) println("start_transfer: arg_int=%d",arg_int);
			i2cm_start(0x3c,0);
      break;
    case U8X8_MSG_BYTE_SEND:
			if (oled_audit) {
				println("  send %d bytes", arg_int);
				/*
				for (int i=0; i < arg_int; i++) {
					print("0x%02x ", ((uint8_t*)(arg_ptr))[i]);
				}
				print("]\r\n");
			  checkState(NULL);
				*/
			}
			i2cm_write((uint8_t*)arg_ptr, arg_int);
			/*
			if (oled_audit) {
			  _delay_us(5000);
		    checkState("after 5ms");
			
				print("[ ");
				for (int i=0; i < i2c_hws_idx; i++) {
					print("0x%02x ", i2c_hws[i]);
				}
				println("]");
			}
			*/
			// ... done. Everything else is handled by an interrupt service routine
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
			if (oled_audit) {
				println("end_transfer");
			}
			i2cm_stop();
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
	oled_audit = 0;
	DDRB |= _BV(DDB5);
	PORTB &= ~_BV(DDB5);
	
	init_async_uart(19200);

	u8x8_Setup(&g_u8x8,u8x8_d_ssd1306_128x64_noname,u8x8_cad_ssd13xx_i2c,oled_byte_cb,oled_gpio_cb);

	u8x8_SetI2CAddress(&g_u8x8,0x3c<<1);
	checkState("SetI2CAddress");
	
	u8x8_InitDisplay(&g_u8x8);
	checkState("InitDisplay");

	u8x8_ClearDisplay(&g_u8x8);
	checkState("ClearDisplay");

	u8x8_SetPowerSave(&g_u8x8,0);
	checkState("SetPowerSave");

	u8x8_SetContrast(&g_u8x8,192);
	checkState("SetContrast");
	
	u8x8_SetFont(&g_u8x8,u8x8_font_5x7_f);
	checkState("SetFont");

	u8x8_DrawString(&g_u8x8, 1,3, "Hello, world!");
	checkState("DrawString");
	
	while(1) {
		asm("nop;");
	}
	
	return 0;
}
