/* drivers/sharp/shfpsensor/shfpsensor_ioctl.h
 *
 * Copyright (c) 2014-2015, Sharp. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef SHFPSENSOR_IOCTL_H_
#define SHFPSENSOR_IOCTL_H_

/*
 * Definitions of structures which are used by IOCTL commands
 */

/* Pass to VFSSPI_IOCTL_SET_USER_DATA and VFSSPI_IOCTL_GET_USER_DATA commands */
typedef struct vfsspi_iocUserData {
	void *buffer;
	unsigned int len;
} vfsspi_iocUserData_t;

/**
 * vfsspi_iocTransfer - structure to pass to VFSSPI_IOCTL_RW_SPI_MESSAGE command
 * @rxBuffer:pointer to retrieved data
 * @txBuffer:pointer to transmitted data
 * @len:transmitted/retrieved data size
 */
typedef struct vfsspi_iocTransfer {
	unsigned char *rxBuffer;
	unsigned char *txBuffer;
	unsigned int len;
} vfsspi_iocTransfer_t;

/* Pass to VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL command */
/**
 * vfsspi_iocRegSignal - structure to pass to VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL
 *			command
 * @userPID:Process ID to which SPI driver sends signal indicating that DRDY
 *			is asserted
 * @signalID:signalID
*/
typedef struct vfsspi_iocRegSignal {
#ifdef __KERNEL__
	u32 userPID;
	u32 signalID;
#else /* __KERNEL__ */
	unsigned int userPID;
	unsigned int signalID;
#endif /* __KERNEL__ */
} vfsspi_iocRegSignal_t;

/* Pass to VFSSPI_IOCTL_GET_FREQ_TABLE command */
/**
* vfsspi_iocFreqTable - structure to get supported SPI baud rates
*
* @table:table which contains supported SPI baud rates
* @tblSize:table size
*/
typedef struct vfsspi_iocFreqTable {
    unsigned int *table;
    unsigned int  tblSize;
} vfsspi_iocFreqTable_t;

/* Magic number of IOCTL command */
#define VFSSPI_IOCTL_MAGIC    'k'

/*
 * IOCTL commands definitions
 */

/* Transmit data to the device
and retrieve data from it simultaneously */
#define VFSSPI_IOCTL_RW_SPI_MESSAGE	\
	_IOWR(VFSSPI_IOCTL_MAGIC, 1, unsigned int)

/* Hard reset the device */
#define VFSSPI_IOCTL_DEVICE_RESET	\
	_IO(VFSSPI_IOCTL_MAGIC,   2)

/* Set the baud rate of SPI master clock */
#define VFSSPI_IOCTL_SET_CLK	\
	_IOW(VFSSPI_IOCTL_MAGIC,  3, unsigned int)

/* Get level state of DRDY GPIO */
#define VFSSPI_IOCTL_CHECK_DRDY	\
	_IO(VFSSPI_IOCTL_MAGIC,   4)

/* Register DRDY signal. It is used by SPI driver
 * for indicating host that DRDY signal is asserted. */
#define VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL	\
	_IOW(VFSSPI_IOCTL_MAGIC,  5, unsigned int)

/* Store the user data into the SPI driver. Currently user data is a
 * device info data, which is obtained from announce packet. */
#define VFSSPI_IOCTL_SET_USER_DATA	\
	_IOW(VFSSPI_IOCTL_MAGIC,  6, unsigned int)

/* Retrieve user data from the SPI driver*/
#define VFSSPI_IOCTL_GET_USER_DATA	\
	_IOWR(VFSSPI_IOCTL_MAGIC, 7, unsigned int)

/* Enable/disable DRDY interrupt handling in the SPI driver */
#define VFSSPI_IOCTL_SET_DRDY_INT	\
	_IOW(VFSSPI_IOCTL_MAGIC,  8, unsigned int)

/* Put device in Low power mode */
#define VFSSPI_IOCTL_DEVICE_SUSPEND	\
	_IO(VFSSPI_IOCTL_MAGIC,	9)

/* Indicate the fingerprint buffer size for read */
#define VFSSPI_IOCTL_STREAM_READ_START	\
	_IOW(VFSSPI_IOCTL_MAGIC, 10, unsigned int)

/* Indicate that fingerprint acquisition is completed */
#define VFSSPI_IOCTL_STREAM_READ_STOP	\
	_IO(VFSSPI_IOCTL_MAGIC,   11)

/* Retrieve supported SPI baud rate table */
#define VFSSPI_IOCTL_GET_FREQ_TABLE	\
	_IOWR(VFSSPI_IOCTL_MAGIC, 12, unsigned int)

/* Turn on the power to the sensor */
#define VFSSPI_IOCTL_POWER_ON	\
	_IO(VFSSPI_IOCTL_MAGIC,   13)

/* Turn off the power to the sensor */
#define VFSSPI_IOCTL_POWER_OFF	\
	_IO(VFSSPI_IOCTL_MAGIC,   14)

/* To disable spi core clock */
#define VFSSPI_IOCTL_DISABLE_SPI_CLOCK	\
	_IO(VFSSPI_IOCTL_MAGIC, 15)

/* Initialize and enable the SPI core (configure with last set (or
 * defaults, if not set, gpios, clks, etc.) */
#define VFSSPI_IOCTL_SET_SPI_CONFIGURATION	\
	_IO(VFSSPI_IOCTL_MAGIC, 16)

/* Uninitialize and disable the SPI core */
#define VFSSPI_IOCTL_RESET_SPI_CONFIGURATION	\
	_IO(VFSSPI_IOCTL_MAGIC, 17)

/* Retrieve sensor mount orientation:
 * 0 - right side up (swipe primary first);
 * 1 - upside down (swipe secondary first) */
#define VFSSPI_IOCTL_GET_SENSOR_ORIENTATION	\
	_IOR(VFSSPI_IOCTL_MAGIC, 18, unsigned int)

/* Register SUSPENDRESUME signal. It is used by SPI driver for indicating host that
 * SUSPEND/RESUME signal is asserted. */
#define VFSSPI_IOCTL_REGISTER_SUSPENDRESUME_SIGNAL	\
    _IOW(VFSSPI_IOCTL_MAGIC, 19, unsigned int)

/* Retrieive System status:
 * 0 - Awake;
 * 1 - Suspend
*/
#define VFSSPI_IOCTL_GET_SYSTEM_STATUS	\
	_IOR(VFSSPI_IOCTL_MAGIC, 20, unsigned int)

#endif /* SHFPSENSOR_IOCTL_H_ */
