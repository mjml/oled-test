#define _I2C_CPP

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/twi.h>
#include <assert.h>
#include "i2c.h"

uint8_t i2c_buf[I2C_BUF_SIZE];
uint8_t* i2c_write_ptr;
uint16_t i2c_write_bufsz;
uint8_t* i2c_read_ptr;
uint16_t i2c_read_bufsz;

i2c_cb i2c_read_cb;

volatile uint16_t i2c_write_idx;
volatile uint16_t i2c_read_idx;


void i2cm_init (uint8_t twbr, uint8_t twps)
{
	TWSR = (twps & 0x3);   // set prescaler
	TWBR = twbr;           // x2 = 32 total scale for 100kHz operation
}

void i2cm_init_easy (enum I2C_SPEEDS speedcode)
{
	uint32_t speed = 100000;
	switch (speedcode) {
	case I2C_SPD_1KHZ:
		speed = 1000;
		break;
	case I2C_SPD_2KHZ:
		speed = 2000;
		break;
	case I2C_SPD_4KHZ:
		speed = 4000;
		break;
	case I2C_SPD_10KHZ:
		speed = 10000;
		break;
	case I2C_SPD_20KHZ:
		speed = 20000;
		break;
	case I2C_SPD_40KHZ:
		speed = 40000;
		break;
	case I2C_SPD_100KHZ:
		speed = 100000;
		break;
	case I2C_SPD_200KHZ:
		speed = 200000;
		break;
	case I2C_SPD_400KHZ:
		speed = 400000;
		break;
	}
  int twbr = (F_CPU/speed-16) / 2;
	int prescalings = 0;
	while (twbr > 256) {
		twbr /= 4;
		prescalings++;
	}
	
	TWBR = twbr;
	TWSR |= prescalings & 0x03;
		
}

void i2cm_write (int sladdr, int bufsz, uint8_t* buf)
{
	while (i2c_mode != I2CM_IDLE) {
		asm("nop;");
	}
  assert(bufsz <= I2C_BUF_SIZE);
	uint8_t* data = (uint8_t *)buf;
	for (int i=0; i < bufsz; i++)      {
		i2c_write_ptr[i++] = *data;
		data++;
	}
	i2cm_writed(sladdr, bufsz, i2c_buf);
}

void i2cm_writed (int sladdr, int bufsz, uint8_t* buf)
{
	while (i2c_mode != I2CM_IDLE) {
		asm("nop;");
	}
	
	i2c_write_ptr = buf;
	i2c_write_bufsz = bufsz;
	i2c_write_addr = sladdr;
	
	// software machine state
	i2c_write_idx = 0;
	i2c_mode = I2CM_WRITE;
	
	// hardware machine state -> send start
	TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWIE); 
	TWCR |= _BV(TWINT);
}

void i2cm_read (int sladdr, int bufsz, uint8_t* buf, i2c_cb cb)
{
	// hardware machine state -> send start
	TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWIE);
	TWCR |= _BV(TWINT);
}

ISR(TWI_vect)
{
	i2c_hwstatus = i2c_get_status();
	switch (i2c_hwstatus) {
	case TW_START:
	case TW_REP_START:
		TWDR = i2c_write_addr << 1;
		break;
	case TW_MT_SLA_ACK:
		TWDR = i2c_write_ptr[i2c_write_idx++];
		break;
	case TW_MT_SLA_NACK:
		i2c_swerror = I2C_NACK_RECEIVED;
		i2c_mode = I2CM_IDLE;
		TWCR |= _BV(TWSTO); // NOTE: Alternatives exist here. I choose to send STOP.
		break;
	case TW_MT_DATA_ACK:
		if (i2c_write_idx < i2c_write_bufsz) {
			TWCR |= _BV(TWSTA);
			TWCR &= ~_BV(TWSTO);
		} else {
			i2c_mode = I2CM_IDLE;
			TWCR |= _BV(TWSTO);
			TWCR &= ~_BV(TWSTA);
		}
		break;
	case TW_MT_DATA_NACK:
		i2c_swerror = I2C_NACK_RECEIVED;
		i2c_mode = I2CM_IDLE;
		TWCR |= _BV(TWSTO);
		TWCR &= ~_BV(TWSTA);
		break;
	}
	TWCR |= _BV(TWINT);
}


