#ifndef _I2C_H

#ifndef _I2C_CPP
#define EXTERN extern
#else
#define EXTERN
#endif

#ifndef I2C_BUF_SIZE
#define I2C_BUF_SIZE 48
#endif

#ifndef I2C_NUM_ASYNC_XFER
#define I2C_NUM_ASYNC_XFER 8
#endif


EXTERN volatile uint8_t i2c_hwstatus;
EXTERN volatile uint8_t i2c_swerror;
EXTERN volatile uint8_t i2c_mode;
EXTERN uint8_t i2c_write_addr;

EXTERN volatile uint8_t i2c_hws[64];
EXTERN volatile uint8_t i2c_hws_idx;

enum I2C_SPEEDS {
	I2C_SPD_1KHZ,
	I2C_SPD_2KHZ,
	I2C_SPD_4KHZ,
	I2C_SPD_10KHZ,
	I2C_SPD_20KHZ,
	I2C_SPD_40KHZ,
	I2C_SPD_100KHZ,
	I2C_SPD_200KHZ,
	I2C_SPD_400KHZ
};

enum I2C_MODES {
	I2CM_IDLE = 0,
	I2CM_SYNC,
	I2CM_ASYNC_WRITE_START_SLAW,
	I2CM_ASYNC_WRITE,
	I2CM_ASYNC_WRITE_UNDERFLOW_WAIT,
	I2CM_ASYNC_READ,
	I2CS_IDLE = 0x80,
	I2CS_RESPONDING
};

enum I2C_ERROR_MODES {
	I2C_NO_ERROR = 0,
	I2C_UNEXPECTED_INTERRUPT,
	I2C_UNEXPECTED_STATUS,
	I2C_NACK_RECEIVED,
};

typedef void i2c_cb(uint8_t sladdr, int bufsz, uint8_t* buf);

typedef uint8_t i2cerr;


inline uint8_t i2c_get_status()
{
	return (TWSR & 0xf8);
}


/* ### MASTER MODE ### */

/**
	 Initialize i2c subsystem for master mode.
	 Speed selected by explicit TWBR value and TWPS prescaler value.
	 Note that speed = F_CPU / (16 + 2 * TWBR * <prescaler value in 1,4,16,64>)
 */
void i2cm_init (uint8_t twbr, uint8_t twps);

/**
	 Initialize i2c subsystem for master mode.
	 Speed selected by convenient enum code.
 */
void i2cm_init_at_speed (enum I2C_SPEEDS speedcode);


/**
	Send a start signal. Buffers the slave address.
 */
i2cerr i2cm_start (uint8_t sladdr, uint8_t rw);

/**
	Send a stop signal.
 */
i2cerr i2cm_stop ();

/**
	Send a write signal and return when it is finished.
 */
i2cerr i2cm_write (uint8_t* buf, uint16_t bufsz);

/**
	Instruct the TWI device to emit a start signal.
 */
i2cerr i2cm_async_start (uint8_t sladdr, uint8_t rw);

/**
	Instruct the TWI device to emit a stop signal.
 */
i2cerr i2cm_async_stop ();

/**
	 Asynchronously send a complete write to a slave, enclosed by start and stop. 
	 Copies data to an internal buffer (size of I2C_BUF_SIZE).
	 Blocks on overflow.
*/
i2cerr i2cm_async_write (uint8_t* buf, uint16_t bufsz);

/**
	 Asynchronously receive bytes from a slave, enclosed by start and stop.
   Writes directly to the provided buffer. 
	 When a STOP is received, and i2c_mode is set to I2CM_IDLE.
 */
i2cerr i2cm_read (int sladdr, uint8_t* buf, uint16_t bufsz, i2c_cb cb);

/**
	 Returns 1 if the i2c state machine is ready to write or read.
 */
inline uint8_t i2cm_ready ()
{
	return i2c_mode == I2CM_IDLE;
}


/* ### SLAVE MODE ###  (unimplemented)   */

void i2cs_init ();
void i2cs_poll (uint8_t* buf, uint16_t bufsz);
void i2cs_accept (uint8_t* buf, uint16_t bufsz, i2c_cb cb);


#endif // _I2C_H
