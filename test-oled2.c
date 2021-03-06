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

extern uint8_t  i2c_write_buf[I2C_BUF_SIZE];
extern volatile uint8_t* i2c_write_begin;
extern volatile uint8_t* i2c_write_end;
extern volatile uint16_t i2c_write_idx;
extern uint16_t  i2c_xfer_sz[I2C_NUM_ASYNC_XFER];                                                   
extern volatile uint8_t i2c_xfer_begin;
extern volatile uint8_t i2c_xfer_end;

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
	i2cerr result;
	
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
			if (oled_audit) {
				println("start_transfer #%d",i2c_xfer_end,arg_int);
			}
			i2cm_start(0x3c,0);
      break;
    case U8X8_MSG_BYTE_SEND:
			if (oled_audit) {
				println("  send %d bytes (currently at %d)", arg_int, i2c_write_idx); // might be causing an underflow
			}
			result = i2cm_write((uint8_t*)arg_ptr, arg_int);
			if (oled_audit) {
				println("  result = 0x%02x", result);
			}
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
			if (oled_audit) {
				println("end_transfer #%d size %d [%d %d]", i2c_xfer_end, i2c_xfer_sz[i2c_xfer_end], i2c_write_begin-i2c_write_buf, i2c_write_end-i2c_write_buf);
			}
			i2cm_stop();
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

	u8x8_SetContrast(&g_u8x8,255);
	checkState("SetContrast");
	
	u8x8_SetFont(&g_u8x8,u8x8_font_artossans8_r);
	checkState("SetFont");

	u8x8_DrawString(&g_u8x8, 1,3, "Hello, world!");
	checkState("DrawString");

	while(1) {
		asm("nop;");
	}
	
	return 0;
}
