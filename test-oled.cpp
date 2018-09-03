#include <U8x8lib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "Arduino.h"
#include "Wire.h"

void do_serial()
{
	for(int i=0; i < 200; i++) {
		if (serialEventRun) serialEventRun();
		_delay_ms(10);
	}
}

int main ()
{
	init();
	
	Serial.begin(9600);
	Serial.println("Program start");

	do_serial();
	
	U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

	u8x8.setI2CAddress(0x3c * 2);
	u8x8.begin();

	int rows = u8x8.getRows();
	int cols = u8x8.getCols();

	char buf[64];
	snprintf(buf,32,"rows=%d, cols=%d\r\n", rows, cols);
	Serial.print(buf);

	do_serial();
	
	u8x8.setFont(u8x8_font_5x7_n);
	
	do_serial();
	
	u8x8.drawString(1,1,"Hello, world");

	do_serial();

	return 0;
}
