/*
 * flash.h
 *
 *  Created on: 19/feb/2015
 *      Author: Luca
 */

#ifndef FLASH_H_
#define FLASH_H_

#include <stdint.h>


/** @def FLASH_API_ERROR_CODES
 Library Error codes
 */
typedef enum FLASH_API_ERROR_CODES {
	FLASH_API_SUCCESS = 0,
	FLASH_API_ACCESS_FAILED = -1,
	FLASH_API_PARAMETER_NOT_ALLOWED = -2,
	FLASH_API_FILE_OPEN_ERROR = -3,
	FLASH_API_MALLOC_ERROR = -4,
	FLASH_API_UNINITIALIZED = -5,
	FLASH_API_NOT_IMPLEMENTED = -6,
	FLASH_API_UNSUPPORTED_FLASH_DEVICE = -7
} FLASH_API_ERROR_CODES;

#ifdef WIN32
#define FLASH_API        FLASH_API_ERROR_CODES __stdcall
#else 
#define FLASH_API        FLASH_API_ERROR_CODES
#endif

#define VIRTUAL_PAGE_SIZE 264
#define MAX_SUPPORTED_PAGE_SIZE 1056

/* Standardized page assignment 
** Constants to be used for *_virtual_page functions page parameter
*/
#define CROM_VIRTUAL_PAGE                               0
#define PLL_VIRTUAL_PAGE                                1
#define LICENSE_VIRTUAL_PAGE                            2
#define VCXO_VIRTUAL_PAGE                               3
#define ADC_CALIBRATION_0_VIRTUAL_PAGE                  4 /* Currently used in x730 boards */
#define ADC_CALIBRATION_1_VIRTUAL_PAGE                  5 /* Currently used in x761 boards */
#define OFFSET_CALIBRATION_VIRTUAL_PAGE                 6 

#define PEAK_SENSING_OFFSETS_VIRTUAL_PAGE           (7935*2)
#define PEAK_SENSING_CALIB_START_VIRTUAL_PAGE       (7936*2)
#define PEAK_SENSING_CALIB_STOP_VIRTUAL_PAGE        (8191*2+1)



/* Main functions to access flash with virtual page size (VIRTUAL_PAGE_SIZE) */
/* handle is a CAENComm library handle */
FLASH_API SPIFlash_init(int handle);
FLASH_API SPIFlash_read_virtual_page(int handle, uint16_t page, uint8_t *buf);
FLASH_API SPIFlash_read_virtual_page_ex(int handle, uint16_t page, uint16_t offset, uint32_t size, uint8_t *buf);
FLASH_API SPIFlash_write_virtual_page(int handle, uint16_t page, uint8_t *buf);
FLASH_API SPIFlash_write_virtual_page_ex(int handle, uint16_t page, uint16_t offset, uint32_t size, uint8_t *buf);

/* Flash identification functions */
FLASH_API SPIFlash_get_page_size(int *size);
FLASH_API SPIFlash_read_id(int handle,uint8_t *manufacturerId, uint8_t *deviceId);
FLASH_API SPIFlash_read_manufacturer_id(int handle, uint8_t *manufacturerId);
FLASH_API SPIFlash_read_device_id(int handle, uint8_t *deviceId);
FLASH_API SPIFlash_read_unique_id(int handle, uint8_t *UniqueId);

/* content access */
FLASH_API SPIFlash_read_byte(int handle, uint32_t addr, uint8_t *data);
FLASH_API SPIFlash_read_bytes(int handle, uint32_t addr, uint8_t *buf, uint16_t len);
FLASH_API SPIFlash_write_byte(int handle, uint32_t addr, uint8_t data);
FLASH_API SPIFlash_write_bytes(int handle, uint32_t addr, uint8_t *buf, uint16_t len);

/* Buffer oriented operations */
FLASH_API SPIFlash_read_buffer1(int handle, int buf_addr, uint16_t len, uint8_t *buf);
FLASH_API SPIFlash_read_buffer2(int handle, int buf_addr, uint16_t len, uint8_t *buf);
FLASH_API SPIFlash_write_buffer1(int handle, uint32_t buf_addr, uint8_t *buf, uint16_t len);
FLASH_API SPIFlash_write_buffer2(int handle, uint32_t buf_addr, uint8_t *buf, uint16_t len);

/* Page oriented operations */
FLASH_API SPIFlash_read_page(int handle, uint16_t page, uint8_t *buf);
FLASH_API SPIFlash_read_page_into_buffer1(int handle, uint16_t page);
FLASH_API SPIFlash_read_page_into_buffer2(int handle, uint16_t page);
FLASH_API SPIFlash_write_page(int handle, uint16_t page, uint8_t *buf);
FLASH_API SPIFlash_write_buffer1_to_memory(int handle, uint32_t page);
FLASH_API SPIFlash_write_buffer2_to_memory(int handle, uint32_t page);
FLASH_API SPIFlash_write_buffer1_to_memory_no_erase(int handle, uint32_t page);
FLASH_API SPIFlash_write_buffer2_to_memory_no_erase(int handle, uint32_t page);
FLASH_API SPIFlash_erase_page(int handle, uint16_t page);

/* wait */
int SPIFlash_is_busy(int handle);
FLASH_API SPIFlash_wait(int handle);

#endif /* FLASH_H_ */
