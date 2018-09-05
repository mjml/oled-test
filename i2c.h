#ifndef _I2C_H

#ifndef I2C_BUF_SIZE
#define I2C_BUF_SIZE 32
#endif

#ifndef _I2C_CPP
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN volatile uint8_t i2c_hwstatus;
EXTERN volatile uint8_t i2c_swerror;
EXTERN uint8_t i2c_write_addr;
EXTERN volatile uint8_t i2c_mode;

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
	I2CM_WRITE,
	I2CM_READ,
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
void i2cm_init_easy (enum I2C_SPEEDS speedcode);

/**
	 Write bytes to a slave. Copies data to an internal buffer (size of I2C_BUF_SIZE).
 */
void i2cm_write (int sladdr, int bufsz, uint8_t* buf);

/**
	 Write bytes to a slave. Writes directly from the buffer given with no copy.
 */
void i2cm_writed (int sladdr, int bufsz, uint8_t* buf);

/**
	 Request to read bytes from a slave. Read bytes are written asynchronously
   to the provided buffer. When a STOP is received, and i2c_mode is set to I2CM_IDLE.
 */
void i2cm_read (int sladdr, int bufsz, uint8_t* buf, i2c_cb cb);

/**
	 Returns 1 if the i2c state machine is ready to write or read.
 */
inline uint8_t i2cm_ready ()
{
	return i2c_mode == I2CM_IDLE;
}


/* ### SLAVE MODE ###  (unimplemented)   */

void i2cs_init ();
void i2cs_poll (int bufsz, uint8_t* buf);
void i2cs_accept (int bufsz, uint8_t* buf, i2c_cb cb);


#endif // _I2C_H
