#define _I2C_CPP

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "i2c.h"

uint8_t  i2c_write_buf[I2C_BUF_SIZE];
volatile uint8_t* i2c_write_begin;
volatile uint8_t* i2c_write_end;
volatile uint16_t i2c_write_idx;
uint8_t  i2c_sladdr;
uint16_t  i2c_xfer_sz[I2C_NUM_ASYNC_XFER];
volatile uint8_t i2c_xfer_begin;
volatile uint8_t i2c_xfer_end;

uint8_t i2c_twbr_cache;
uint8_t i2c_twsr_cache;

i2c_cb i2c_read_cb;
volatile uint16_t i2c_read_idx;

#define INC_RING(n,max) ((n+1)%max)
#define INC_CIRCBUF(p,base,size) ((p+1==base+size)?base:p+1)

void i2cm_init (uint8_t twbr, uint8_t twps)
{
	i2c_write_begin = i2c_write_end = i2c_write_buf;
	i2c_write_idx = 0;
	i2c_sladdr = 0;
	i2c_xfer_begin = i2c_xfer_end = 0;
	i2c_mode = I2CM_IDLE;

	i2c_twbr_cache = twbr;
	i2c_twsr_cache = twps & 0x03;
	
	TWSR = i2c_twsr_cache;   // set prescaler
	TWBR = i2c_twbr_cache;           // x2 = 32 total scale for 100kHz operation
}

void i2cm_init_at_speed (enum I2C_SPEEDS speedcode)
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

	i2c_write_begin = i2c_write_end = i2c_write_buf;
	i2c_write_idx = 0;
	i2c_sladdr = 0;
	i2c_xfer_begin = i2c_xfer_end = 0;
	i2c_mode = I2CM_IDLE;
	
	TWBR = twbr;
	TWSR |= prescalings & 0x03;
}

i2cerr i2cm_start (uint8_t sladdr, uint8_t rw)
{
	while (i2c_mode != I2CM_IDLE ) asm("nop;");
	i2c_mode = I2CM_SYNC;
	i2c_sladdr = sladdr;
	TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWINT);
	
	int hwstatus = 0;
	do {
		hwstatus = i2c_get_status();
	} while ((hwstatus != TW_START && hwstatus != TW_REP_START) ||
					 ~TWCR & _BV(TWINT));
	
	TWDR = i2c_sladdr << 1 | rw;
	TWCR = _BV(TWEN) | _BV(TWINT);
	
	do {
		hwstatus = TWSR & ~0x03;
	} while (~TWCR & _BV(TWINT) && hwstatus != TW_MT_SLA_ACK && TW_MT_SLA_NACK);
	
	if (hwstatus == TW_MT_SLA_NACK) {
		TWCR &= ~_BV(TWSTA);
		TWCR |= _BV(TWSTO);
		return I2C_NACK_RECEIVED;
	}
}

i2cerr i2cm_stop ()
{
	TWCR = _BV(TWSTO) | _BV(TWEN) | _BV(TWINT);
	i2c_mode = I2CM_IDLE;
	return 0;
}

i2cerr i2cm_write (uint8_t* buf, uint16_t bufsz)
{
	do {
		i2c_hwstatus = i2c_get_status();
	} while (i2c_hwstatus != TW_MT_SLA_ACK && i2c_hwstatus != TW_MT_DATA_ACK);
	
	for (int i=0; i < bufsz; i++) {
		TWDR = buf[i];
		TWCR |= _BV(TWINT);
		while (i2c_hwstatus = i2c_get_status() != TW_MT_DATA_ACK) asm("nop;");
	}
	return 0;
}

i2cerr i2cm_async_start (uint8_t sladdr, uint8_t rw)
{
	// reset transfer bytes
	cli();
	i2c_xfer_sz[i2c_xfer_end] = 0;
	if (i2c_xfer_begin == i2c_xfer_end && i2c_write_begin == i2c_write_end) {
		// transfer AND buffer underflow condition at start, so send start signal to get things moving
		i2c_mode = I2CM_ASYNC_WRITE_START_SLAW;
		i2c_sladdr = sladdr;
		TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWIE) | _BV(TWINT);
	}
	sei();
	
	return 0;
}

i2cerr i2cm_async_stop ()
{
	cli();

	// check for write buffer underflow
	uint8_t underflow = i2c_write_idx == i2c_xfer_sz[i2c_xfer_begin];
	if (underflow) {
		// send an explicit stop since we're done sending data
		i2c_mode = I2CM_IDLE;
		TWCR = _BV(TWSTO) | _BV(TWEN) | _BV(TWINT) | _BV(TWIE);
		_delay_us(1); // wait briefly to ensure the STOP signal went through
	}
	
	// set the sign bit of the xfer size to negative to indicate an explicit stop,
	// (even if stop was already sent, and even if we just sent the stop signal,
	//      for consistency)
	if (i2c_xfer_sz[i2c_xfer_end] > 0) {
	  i2c_xfer_sz[i2c_xfer_end] *= -1;
	}

	// check for transfer overflow
	uint8_t new_xfer_end = INC_RING(i2c_xfer_end,I2C_NUM_ASYNC_XFER);
	if (new_xfer_end == i2c_xfer_begin) {
		sei();
		while (new_xfer_end == i2c_xfer_begin) asm("nop;");
		cli();
	}
	i2c_xfer_end = new_xfer_end;

	sei();
	return 0;
}

i2cerr i2cm_async_write (uint8_t* buf, uint16_t bufsz)
{
	int i=0;
  volatile uint8_t* new_write_end;
	cli();
	uint8_t underflow = i2c_write_idx == i2c_xfer_sz[i2c_xfer_begin];
	while (i < bufsz) {
		*i2c_write_end = buf[i];
		i2c_xfer_sz[i2c_xfer_end]++;
		new_write_end = INC_CIRCBUF(i2c_write_end,i2c_write_buf,I2C_BUF_SIZE);
		if (new_write_end == i2c_write_begin) {
			// data overflow -- must block until i2c_write_begin has caught up
			underflow = 0;
			sei();
			while (new_write_end == i2c_write_begin) asm("nop;");
			cli();
		}
		i2c_write_end = new_write_end;
		i++;
	}
	if (underflow && i2c_mode == I2CM_ASYNC_WRITE_UNDERFLOW_WAIT) {
		PORTB ^= _BV(PORTB5);
		int hwstatus = 0;
		TWCR = _BV(TWEN);
		//do {
			hwstatus = i2c_get_status();
			//} while (i-- > 0 && hwstatus != TW_MT_SLA_ACK && hwstatus != TW_MT_DATA_ACK);
		if (hwstatus != TW_MT_SLA_ACK && hwstatus != TW_MT_DATA_ACK) {
			return (i2cerr)hwstatus;
		}
		PORTB &= ~_BV(PORTB5);
		i2c_mode = I2CM_ASYNC_WRITE;
		TWDR = *i2c_write_begin;
		TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT);
	}
	sei();
	
	return 0;
}

i2cerr i2cm_read (int sladdr, uint8_t* buf, uint16_t bufsz, i2c_cb cb)
{
	// TODO
	
	return 0;
}

ISR(TWI_vect)
{
	//i2c_hws[i2c_hws_idx++] =
	i2c_hwstatus = i2c_get_status();
	int16_t cur_xfer_sz = abs(i2c_xfer_sz[i2c_xfer_begin]);
	uint8_t cur_xfer_complete = i2c_xfer_sz[i2c_xfer_begin] < 0;
	switch (i2c_hwstatus) {
	case TW_START:
	case TW_REP_START:
		TWDR = i2c_sladdr << 1;
		TWCR &= ~_BV(TWSTA);
		break;
	case TW_MT_DATA_ACK:
		i2c_write_begin = INC_CIRCBUF(i2c_write_begin,i2c_write_buf,I2C_BUF_SIZE);
		i2c_write_idx++;
	case TW_MT_SLA_ACK:
		if (i2c_write_idx != cur_xfer_sz) {
			// transfer in progress, send next byte
			TWDR = *i2c_write_begin;
			i2c_mode = I2CM_ASYNC_WRITE;
		} else if (cur_xfer_complete) {
			i2c_write_idx = 0;
			i2c_xfer_sz[i2c_xfer_begin] = 0;
			i2c_xfer_begin = INC_RING(i2c_xfer_begin,I2C_NUM_ASYNC_XFER);
			if (i2c_xfer_begin != i2c_xfer_end) {
				// continue pending transfers: send a repeat start
				TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWIE);
				TWCR |= _BV(TWINT);
				i2c_mode = I2CM_ASYNC_WRITE;
			} else {
				// transfer buffer underflow: this is our full stop condition
				TWCR = _BV(TWSTO) | _BV(TWEN) | _BV(TWIE);
				i2c_mode = I2CM_IDLE;
				break;
			}
		} else {
			// write buffer underflow.
			// to avoid spamming TWDR, just turn off the TW device
			// a subsequent write will re-enable it and send a byte
			PORTB |= _BV(PORTB5);
			TWCR &= ~_BV(TWEN);
			i2c_mode = I2CM_ASYNC_WRITE_UNDERFLOW_WAIT;
			return;
		}
		break;
	case TW_MT_SLA_NACK:
	case TW_MT_DATA_NACK:
		i2c_swerror = I2C_NACK_RECEIVED;
		i2c_mode = I2CM_IDLE;
		TWCR |= _BV(TWSTO);
		TWCR &= ~_BV(TWSTA);
		break;
	}
	TWCR |= _BV(TWINT);
}


