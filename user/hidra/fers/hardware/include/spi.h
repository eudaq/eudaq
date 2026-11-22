/*
 * spi.h
 *
 *  Created on: 19/feb/2015
 *      Author: Luca
 */

#ifndef SPI_H_
#define SPI_H_

#include <CAENComm.h>

#define MAX_SUPPORTED_PAGE_SIZE 1056

#define SPI_API CAENComm_ErrorCode

#define SPI_SELECT_REG_ADDR 0xEF2C
#define SPI_DATA_REG_ADDR   0xEF30
#define SPI_FLASH_SIZE_ADDR   0xF050

#define SMALL_FLASH	0
#define BIG_FLASH	1
#define HUGE_FLASH	2


#define __MULTIWRITE_SUPPORT__
#define __MULTIREAD_SUPPORT__

SPI_API spi_select(int handle);
SPI_API spi_unselect(int handle);

SPI_API spi_write(int handle, uint8_t data);
SPI_API spi_read(int handle, uint8_t *data);

SPI_API spi_write_block(int handle, uint8_t *buf, uint32_t len);
SPI_API spi_read_block(int handle, uint8_t *buf, uint32_t len);

#endif /* SPI_H_ */
