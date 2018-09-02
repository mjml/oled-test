#include <u8x8.h>
#include <avr/io.h>
#include "Wire.h"

u8x8_t u8x8;

uint8_t u8x8_byte_arduino_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      Wire.write((uint8_t *)arg_ptr, (int)arg_int);
      break;
    case U8X8_MSG_BYTE_INIT:
      Wire.begin();
      break;
    case U8X8_MSG_BYTE_SET_DC:
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      if ( u8x8->display_info->i2c_bus_clock_100kHz >= 4 )
      {
				Wire.setClock(400000L); 
      }
      Wire.beginTransmission(u8x8_GetI2CAddress(u8x8)>>1);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      Wire.endTransmission();
      break;
    default:
      return 0;
  }
  return 1;
}

int main ()
{
	
	
	return 0;
}
