/*
 * flash.c
 *
 *  Created on: 19/feb/2015
 *      Author: Luca
 */

#include "flash.h"
#include "spi.h"
#include "flash_opcodes.h"
#include <CAENComm.h>

static uint8_t DeviceID;
static uint16_t PAGE_SIZE;
static int is_initialized = 0;

/*
 * Static (local) functions
 *
 */
static void _read_device_id(int handle) {
	uint8_t data;

	spi_select(handle);

	spi_write(handle,READ_MANUFACTURER_DEVICE_ID);
	spi_read(handle, &data); /* Skip manufacturer */

	spi_read(handle, &data);
	DeviceID = data & 0xFF;

	spi_unselect(handle);

}

static uint8_t read_status(int handle) {
	uint8_t data;

	spi_select(handle);

	spi_write(handle,STATUS_READ_CMD);
	spi_read(handle, &data);

	spi_unselect(handle);

	return data;
}

static uint32_t get_spi_address(uint16_t page_addr, uint16_t byte_addr) {
	uint32_t spi_address=0;

	switch (DeviceID) {
	case SPIFLASH_DEVICEID_64MBIT:
		spi_address = ((page_addr & 0x1FFF) << 11) | (byte_addr & 0x7FF); // PA[12:0] & BA[10:0]
		break;
	case SPIFLASH_DEVICEID_32MBIT:
		spi_address = ((page_addr & 0x1FFF) << 10) | (byte_addr & 0x3FF); // PA[12:0] & BA[9:0]
		break;
	case SPIFLASH_DEVICEID_8MBIT:
		spi_address = ((page_addr & 0x0FFF) << 9) | (byte_addr & 0x1FF); // PA[11:0] & BA[8:0]
		break;
	default:
		break;
	}

	return spi_address;

}

static uint32_t get_spi_standard_address(uint16_t page_addr, uint16_t byte_addr) {
	uint32_t spi_address = 0;

	spi_address = ((page_addr & 0x0FFF) << 9) | (byte_addr & 0x1FF); // get address using smallest page size (264)

	return spi_address;
}

static uint16_t get_page_addr_from_sector(uint16_t sector) {
	uint16_t page_addr=0;

	switch (DeviceID) {
	case SPIFLASH_DEVICEID_64MBIT:
		page_addr = (sector & 0x1F) << 8; // 32 sectors
		break;
	case SPIFLASH_DEVICEID_32MBIT:
		page_addr = (sector & 0x3F) << 7; // 64 sectors
		break;
	case SPIFLASH_DEVICEID_8MBIT:
		page_addr = (sector & 0x0F) << 8; // 16 sectors
		break;
	default:
		break;
	}

	return page_addr;

}

static uint16_t get_page_addr_from_block(uint16_t block) {
	uint16_t page_addr=0;

	switch (DeviceID) {
	case SPIFLASH_DEVICEID_64MBIT:
		page_addr = block << 10; // 1024 blocks
		break;
	case SPIFLASH_DEVICEID_32MBIT:
		page_addr = block << 10; // 1024 blocks
		break;
	case SPIFLASH_DEVICEID_8MBIT:
		page_addr = block << 10; //  512 blocks
		break;
	default:
		break;
	}

	return page_addr;

}

/*
 * Public functions
 *
 */

FLASH_API SPIFlash_init(int handle) {

	_read_device_id(handle);

	switch (DeviceID) {
	case SPIFLASH_DEVICEID_64MBIT:
		PAGE_SIZE = 264;
		is_initialized = 1;
		break;
	case SPIFLASH_DEVICEID_32MBIT:
		PAGE_SIZE = 528;
		is_initialized = 1;
		break;
	case SPIFLASH_DEVICEID_8MBIT:
		PAGE_SIZE = 264;
		is_initialized = 1;
		break;
	default:
		is_initialized = 0;
		return FLASH_API_UNSUPPORTED_FLASH_DEVICE;
		break;
	}

	return FLASH_API_SUCCESS;
}

FLASH_API SPIFlash_get_page_size(int *size) {

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	*size = PAGE_SIZE;
	return FLASH_API_SUCCESS;
}

FLASH_API SPIFlash_read_manufacturer_id(int handle, uint8_t *manufacturerId) {
	uint8_t data;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	spi_select(handle);

	spi_write(handle,READ_MANUFACTURER_DEVICE_ID);
	spi_read(handle, &data);
	*manufacturerId = data;

	spi_unselect(handle);

	return FLASH_API_SUCCESS;

}

FLASH_API SPIFlash_read_device_id(int handle, uint8_t *deviceId) {
	uint8_t data;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	spi_select(handle);

	spi_write(handle, READ_MANUFACTURER_DEVICE_ID);
	spi_read(handle, &data); /* Skip manufacturer */

	spi_read(handle, &data);
	*deviceId = data & 0xFF;

	spi_unselect(handle);

	return FLASH_API_SUCCESS;
}

FLASH_API SPIFlash_read_id(int handle, uint8_t *manufacturerId, uint8_t *deviceId) {
	uint8_t data;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	spi_select(handle);

	spi_write(handle, READ_MANUFACTURER_DEVICE_ID);
	spi_read(handle,&data);
	*manufacturerId = data;

	spi_read(handle,&data);
	*deviceId = data & 0xFF;

	spi_unselect(handle);

	return FLASH_API_SUCCESS;

}

/*
 * UniqueId must have been preallocated for 128 byets
 *
 */
FLASH_API SPIFlash_read_unique_id(int handle, uint8_t *UniqueId) {
	uint8_t data;

	int i;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	spi_select(handle);

	spi_write(handle,READ_SECURITY_REGISTER_CMD);

	/* Three dummy writes */
	spi_write(handle,0);
	spi_write(handle,0);
	spi_write(handle,0);

	for (i = 0; i < 128; ++i) {
		spi_read(handle,&data);
		UniqueId[i] = data;
	}

	spi_unselect(handle);

	return FLASH_API_SUCCESS;

}

/*
 * Inputs:
 * 	page  : memory page to load
 */
FLASH_API SPIFlash_read_page_into_buffer1(int handle,uint16_t page) {

	uint32_t page_addr;
	uint32_t byte_addr;
	uint32_t spi_address;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = page;
	byte_addr = 0; /* don't care */

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);

	spi_select(handle);

	spi_write(handle, MAIN_MEMORY_PAGE_TO_BUFFER1_XFER);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address >> 0) & 0xFF));

	spi_unselect(handle);

	/* Wait flash is ready before every write operation */
	SPIFlash_wait(handle);

	return FLASH_API_SUCCESS;

}

FLASH_API SPIFlash_read_page_into_buffer2(int handle,uint16_t page) {

	uint32_t page_addr;
	uint32_t byte_addr;
	uint32_t spi_address;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = page;
	byte_addr = 0; /* don't care */

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);

	spi_select(handle);

	spi_write(handle, MAIN_MEMORY_PAGE_TO_BUFFER2_XFER);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address >> 0) & 0xFF));

	spi_unselect(handle);

	/* Wait flash is ready before every write operation */
	SPIFlash_wait(handle);

	return FLASH_API_SUCCESS;

}

FLASH_API SPIFlash_read_byte(int handle,uint32_t addr, uint8_t *data) {

	uint32_t page_addr;
	uint32_t byte_addr;
	uint32_t spi_address;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = addr / PAGE_SIZE;
	byte_addr = addr % PAGE_SIZE;

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);

	spi_select(handle);

	spi_write(handle, CONTINOUS_ARRAY_READ_LOW_FREQ);

	/* address */
	spi_write(handle, (spi_address >> 16) & 0xFF);
	spi_write(handle, (spi_address >> 8) & 0xFF);
	spi_write(handle, (spi_address >> 0) & 0xFF);

	spi_read(handle, data);

	spi_unselect(handle);

	return FLASH_API_SUCCESS;

}

FLASH_API SPIFlash_read_bytes(int handle, uint32_t addr, uint8_t *buf, uint16_t len) {

	uint32_t page_addr;
	uint32_t byte_addr;
	uint32_t spi_address;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = addr / PAGE_SIZE;
	byte_addr = addr % PAGE_SIZE;

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);

	spi_select(handle);

	spi_write(handle, CONTINOUS_ARRAY_READ_LOW_FREQ);

	/* address */
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address >> 0) & 0xFF));

	spi_read_block(handle, (uint8_t *) buf, len);
//	for (i = 0; i < len; ++i) {
//		spi_read(&((uint8_t *) buf)[i]);
//	}

	spi_unselect(handle);

	return FLASH_API_SUCCESS;

}

FLASH_API SPIFlash_read_buffer1(int handle, int buf_addr, uint16_t len, uint8_t *buf) {
	uint32_t spi_address;
	uint32_t page_addr;
	uint32_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = 0;
	byte_addr = buf_addr;
	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);

	spi_select(handle);

	spi_write(handle, BUFFER_1_READ);

	/* address */
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address >> 0) & 0xFF));

	/* Dummy cycle */
	spi_write(handle, 0);

	spi_read_block(handle, buf, len);

	spi_unselect(handle);

	return FLASH_API_SUCCESS;

}

FLASH_API SPIFlash_read_buffer2(int handle, int buf_addr, uint16_t len, uint8_t *buf) {
	uint32_t spi_address;
	uint32_t page_addr;
	uint32_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = 0;
	byte_addr = buf_addr;
	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);

	spi_select(handle);

	spi_write(handle, BUFFER_2_READ);

	/* address */
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address >> 0) & 0xFF));

	/* Dummy cycle */
	spi_write(handle, 0);

	spi_read_block(handle, (uint8_t *) buf, len);

	spi_unselect(handle);

	return FLASH_API_SUCCESS;

}

FLASH_API SPIFlash_write_byte(int handle, uint32_t addr, uint8_t data) {

	uint32_t page_addr;

	uint32_t byte_addr = addr % PAGE_SIZE;
	uint16_t offset = (uint16_t)byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = addr / PAGE_SIZE;

	SPIFlash_read_page_into_buffer1(handle,(uint16_t)page_addr);
	SPIFlash_write_buffer1(handle,offset, &data, 1);
	SPIFlash_write_buffer1_to_memory(handle,page_addr);

	/* Wait flash is ready before every write operation */
	SPIFlash_wait(handle);

	return FLASH_API_SUCCESS;

}

FLASH_API SPIFlash_write_bytes(int handle,uint32_t addr, uint8_t *buf, uint16_t len) {

	uint16_t n;
	uint32_t page_addr;

	uint32_t byte_addr = addr % PAGE_SIZE;

	uint16_t maxBytes = PAGE_SIZE - (uint16_t)byte_addr; // force the first set of bytes to stay within the first page
	uint16_t offset = 0;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	while (len > 0) {

		page_addr = addr / PAGE_SIZE;
		byte_addr = addr % PAGE_SIZE;

		n = (len <= maxBytes) ? len : maxBytes;

		/* Directly write a full flash page if possible to
		 * speed-up firmware programmming
		 */
		if ((n == PAGE_SIZE) && (byte_addr == 0)) {
			SPIFlash_write_page(handle, (uint16_t)page_addr, &buf[offset]);
		} else {
			SPIFlash_read_page_into_buffer1(handle, (uint16_t)page_addr);
			SPIFlash_write_buffer1(handle,byte_addr, (uint8_t *) &buf[offset],n);
			SPIFlash_write_buffer1_to_memory(handle,page_addr);
		}

		/* Wait flash is ready before every write operation */
		SPIFlash_wait(handle);

		addr    += n; // adjust the addresses and remaining bytes by what we've just transferred.
		offset  += n;
		len     -= n;
		maxBytes = PAGE_SIZE;   // now we can do up to 256 bytes per loop
	}

	return FLASH_API_SUCCESS;

}

int SPIFlash_is_busy(int handle) {

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	return (read_status(handle) & 0x80) >> 7;

}

FLASH_API SPIFlash_wait(int handle) {
	uint8_t data;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	spi_select(handle);

	spi_write(handle, STATUS_READ_CMD);

	do
		spi_read(handle,&data);
	while (!(data & 0x80));

	spi_unselect(handle);

	return FLASH_API_SUCCESS;

}

FLASH_API SPIFlash_write_buffer1(int handle, uint32_t buf_addr, uint8_t *buf, uint16_t len) {
	CAENComm_ErrorCode ret;
	uint32_t spi_address;
	uint32_t page_addr;
	uint32_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = 0;
	byte_addr = buf_addr;

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);
	ret = spi_select(handle);

	ret = spi_write(handle, BUFFER_1_WRITE);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	ret = spi_write_block(handle, buf, len);

	ret = spi_unselect(handle);

	return (ret ? FLASH_API_ACCESS_FAILED : FLASH_API_SUCCESS);
}

FLASH_API SPIFlash_write_buffer2(int handle, uint32_t buf_addr, uint8_t *buf, uint16_t len) {
	int i;
	uint32_t spi_address;
	uint32_t page_addr;
	uint32_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = 0;
	byte_addr = buf_addr;

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);
	spi_select(handle);

	spi_write(handle, BUFFER_2_WRITE);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	for (i = 0; i < len; i++)
		spi_write(handle, buf[i]);

	spi_unselect(handle);

	return FLASH_API_SUCCESS;
}

FLASH_API SPIFlash_write_buffer1_to_memory(int handle, uint32_t page) {
	uint32_t spi_address;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	spi_address = get_spi_address((uint16_t)page, 0);

	spi_select(handle);

	spi_write(handle, BUFFER_1_MAIN_MEM_PAGE_PROG_BE);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	spi_unselect(handle);

	SPIFlash_wait(handle);

	return FLASH_API_SUCCESS;
}

FLASH_API SPIFlash_write_buffer2_to_memory(int handle, uint32_t page) {
	uint32_t spi_address;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	spi_address = get_spi_address((uint16_t)page, 0);

	spi_select(handle);

	spi_write(handle, BUFFER_2_MAIN_MEM_PAGE_PROG_BE);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	spi_unselect(handle);

	SPIFlash_wait(handle);

	return FLASH_API_SUCCESS;
}

FLASH_API SPIFlash_write_buffer1_to_memory_no_erase(int handle, uint32_t page) {
	uint32_t spi_address;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	spi_address = get_spi_address((uint16_t)page, 0);

	spi_select(handle);

	spi_write(handle, BUFFER_1_MAIN_MEM_PAGE_PROG);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	spi_unselect(handle);

	SPIFlash_wait(handle);

	return FLASH_API_SUCCESS;
}

FLASH_API SPIFlash_write_buffer2_to_memory_no_erase(int handle, uint32_t page) {
	uint32_t spi_address;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	spi_address = get_spi_address((uint16_t)page, 0);

	spi_select(handle);

	spi_write(handle, BUFFER_2_MAIN_MEM_PAGE_PROG);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	spi_unselect(handle);

	SPIFlash_wait(handle);

	return FLASH_API_SUCCESS;
}

FLASH_API SPIFlash_erase_page(int handle, uint16_t page) {
	uint32_t spi_address;
	uint32_t page_addr;
	uint32_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = page;
	byte_addr = 0; /* don't care */

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);
	spi_select(handle);

	spi_write(handle, PAGE_ERASE);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	spi_unselect(handle);

	/* Wait flash is ready before every write operation */
	SPIFlash_wait(handle);

	return FLASH_API_SUCCESS;
}

FLASH_API SPIFlash_block_sector(int handle, uint16_t block) {
	uint32_t spi_address;
	uint32_t page_addr;
	uint32_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = get_page_addr_from_block(block);
	byte_addr = 0; /* don't care */

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);
	spi_select(handle);

	spi_write(handle, BLOCK_ERASE);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	spi_unselect(handle);

	/* Wait flash is ready before every write operation */
	SPIFlash_wait(handle);

	return FLASH_API_SUCCESS;
}

/*
 * @note Do not support sector 0a/0b erase (yet)
 * @todo sector 0a/0b
 */
FLASH_API SPIFlash_erase_sector(int handle, uint16_t sector) {
	uint32_t spi_address;
	uint32_t page_addr;
	uint32_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = get_page_addr_from_sector(sector);
	byte_addr = 0; /* don't care */

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);
	spi_select(handle);

	spi_write(handle, SECTOR_ERASE);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	spi_unselect(handle);

	/* Wait flash is ready before every write operation */
	SPIFlash_wait(handle);

	return FLASH_API_SUCCESS;
}

/*! \brief   read a full flash page
*
*   \param   handle: device handle
*   \param   page: page number
*   \param   buf: pointer to the data buffer
*/
FLASH_API SPIFlash_read_page(int handle, uint16_t page, uint8_t *buf) {
	uint32_t spi_address;
	uint32_t page_addr;
	uint32_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = page;
	byte_addr = 0;

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);

	spi_select(handle);
	spi_write(handle, MAIN_MEM_PAGE_READ_CMD);
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	/* 4 dummy cycles */
	spi_write(handle, 0);
	spi_write(handle, 0);
	spi_write(handle, 0);
	spi_write(handle, 0);

	spi_read_block(handle, buf, PAGE_SIZE);

	spi_unselect(handle);

	return FLASH_API_SUCCESS;
}

/*! \brief   read a full page of standard size (264 bytes)
*
*   \param   handle: device handle
*   \param   page: standard page number
*   \param   buf: pointer to the data buffer
*/
FLASH_API SPIFlash_read_virtual_page(int handle, uint16_t page, uint8_t *buf) {
	uint16_t standard_page_addr;
	uint16_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	standard_page_addr = page * VIRTUAL_PAGE_SIZE;
	byte_addr = 0;

	return SPIFlash_read_bytes(handle,(uint32_t)(standard_page_addr + byte_addr), buf, VIRTUAL_PAGE_SIZE);
}

/*! \brief   read some bytes from a page of standard size (264 bytes) starting from a given point
*
*   \param   handle: device handle
*   \param   page: standard page number
*   \param   offset: number of bytes to skip from the beginning of the page before starting to read
*	\param	 size: total number of bytes to read
*   \param   buf: pointer to the data buffer
*/
FLASH_API SPIFlash_read_virtual_page_ex(int handle, uint16_t page, uint16_t offset, uint32_t size, uint8_t *buf) {
	uint16_t standard_page_addr;
	uint16_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	standard_page_addr = page * VIRTUAL_PAGE_SIZE;
	byte_addr = offset;

	return SPIFlash_read_bytes(handle, (uint32_t)(standard_page_addr + byte_addr), buf, (uint16_t)size);
}

/*! \brief   write a full flash page 
*
*   \param   handle: device handle
*   \param   page: page number
*   \param   buf: pointer to the data to write
*/
FLASH_API SPIFlash_write_page(int handle, uint16_t page, uint8_t *buf) {
	uint32_t spi_address;
	uint16_t page_addr;
	uint16_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	page_addr = page;
	byte_addr = 0;

	spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);

	spi_select(handle);
	spi_write(handle, MAIN_MEM_PAGE_PROG_TH_BUF_1_CMD);  // Byte/Page Program
	spi_write(handle, ((spi_address >> 16) & 0xFF));
	spi_write(handle, ((spi_address >> 8) & 0xFF));
	spi_write(handle, ((spi_address) & 0xFF));

	spi_write_block(handle, buf, PAGE_SIZE);

	spi_unselect(handle);

	return FLASH_API_SUCCESS;
}

/*! \brief   write a full flash page of standard size (264 bytes) 
*
*   \param   handle: device handle
*   \param   page:  page number
*   \param   buf: pointer to the data to write
*/
FLASH_API SPIFlash_write_virtual_page(int handle, uint16_t page, uint8_t *buf) {
	uint16_t standard_page_addr;
	uint16_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	standard_page_addr = page * VIRTUAL_PAGE_SIZE;
	byte_addr = 0;

	return SPIFlash_write_bytes(handle, (standard_page_addr + byte_addr), buf, VIRTUAL_PAGE_SIZE);
}

/*! \brief   write some bytes into a page of standard size (264 bytes) starting from a given point
*
*   \param   handle: device handle
*   \param   page: standard page number
*   \param   offset: number of bytes to skip from the beginning of the page before starting to write
*	\param	 size: total number of bytes to write
*   \param   buf: pointer to the data to write
*/
FLASH_API SPIFlash_write_virtual_page_ex(int handle, uint16_t page, uint16_t offset, uint32_t size, uint8_t *buf) {
	uint16_t standard_page_addr;
	uint16_t byte_addr;

	if (!is_initialized)
		return FLASH_API_UNINITIALIZED;

	standard_page_addr = page * VIRTUAL_PAGE_SIZE;
	byte_addr = offset;

	return SPIFlash_write_bytes(handle,(uint32_t)(standard_page_addr + byte_addr), buf, (uint16_t)size);
}

/* TEMPLATE
 FLASH_API SPIFlash_template() {
 uint8_t data;
 uint32_t spi_address;
 uint32_t page_addr;
 uint32_t byte_addr;

 if (!is_initialized)
 return FLASH_API_UNINITIALIZED;

 page_addr = 0;
 byte_addr = 0;

 spi_address = get_spi_address((uint16_t)page_addr, (uint16_t)byte_addr);
 spi_select();
 // Add your spi flash commands implementation here
 spi_unselect();

 return FLASH_API_SUCCESS;

 }

 */
