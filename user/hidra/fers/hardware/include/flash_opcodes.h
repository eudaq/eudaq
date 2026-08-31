#ifndef FLASH_OPCODES_H_
#define FLASH_OPCODES_H_

/* Read Commands */
#define MAIN_MEM_PAGE_READ_CMD               0xD2
#define CONTINOUS_ARRAY_READ_LEGACY          0xE8
#define CONTINOUS_ARRAY_READ_LOW_FREQ        0x03
#define CONTINOUS_ARRAY_READ_HIGH_FREQ       0x0B
#define BUFFER_1_READ_LOW_FREQ               0xD1
#define BUFFER_2_READ_LOW_FREQ               0xD3
#define BUFFER_1_READ                        0xD4
#define BUFFER_2_READ                        0xD6

/* Program and Erase Command */
#define BUFFER_1_WRITE                       0x84
#define BUFFER_2_WRITE                       0x87
#define BUFFER_1_MAIN_MEM_PAGE_PROG_BE       0x83 /* w/ Build-in erase */
#define BUFFER_2_MAIN_MEM_PAGE_PROG_BE       0x86 /* w/ Build-in erase */
#define BUFFER_1_MAIN_MEM_PAGE_PROG          0x88 /* w/o Build-in erase */
#define BUFFER_2_MAIN_MEM_PAGE_PROG          0x89 /* w/o Build-in erase */
#define PAGE_ERASE                           0x81
#define BLOCK_ERASE                          0x50
#define SECTOR_ERASE                         0x7C
#define CHIP_ERASE_0                         0xC7
#define CHIP_ERASE_1                         0x94
#define CHIP_ERASE_2                         0x80
#define CHIP_ERASE_3                         0x9A
#define MAIN_MEM_PAGE_PROG_TH_BUF_1_CMD      0x82
#define MAIN_MEM_PAGE_PROG_TH_BUF_2_CMD      0x85

/* Protection and Security Commands */
#define ENABLE_SECTOR_PROTECTION_0           0x3D
#define ENABLE_SECTOR_PROTECTION_1           0x2A
#define ENABLE_SECTOR_PROTECTION_2           0x7F
#define ENABLE_SECTOR_PROTECTION_3           0xA9

#define DISABLE_SECTOR_PROTECTION_0          0x3D
#define DISABLE_SECTOR_PROTECTION_1          0x2A
#define DISABLE_SECTOR_PROTECTION_2          0x7F
#define DISABLE_SECTOR_PROTECTION_3          0xA9

#define ERASE_SECTOR_PROTECTION_REG_0        0x3D
#define ERASE_SECTOR_PROTECTION_REG_1        0x2A
#define ERASE_SECTOR_PROTECTION_REG_2        0x7F
#define ERASE_SECTOR_PROTECTION_REG_3        0xCF

#define PROGRAM_SECTOR_PROTECTION_REG_0      0x3D
#define PROGRAM_SECTOR_PROTECTION_REG_1      0x2A
#define PROGRAM_SECTOR_PROTECTION_REG_2      0x7F
#define PROGRAM_SECTOR_PROTECTION_REG_3      0xFC

#define READ_SECTOR_PROTECTION_REG           0x32

#define SECTOR_LOCKDOWN_0                    0x3D
#define SECTOR_LOCKDOWN_1                    0x2A
#define SECTOR_LOCKDOWN_2                    0x7F
#define SECTOR_LOCKDOWN_3                    0x30

#define READ_SECTOR_LOCKDOWN_REG             0x35

#define PROGRAM_SECUTITY_REG_0               0x9B
#define PROGRAM_SECUTITY_REG_1               0x00
#define PROGRAM_SECUTITY_REG_2               0x00
#define PROGRAM_SECUTITY_REG_3               0x00
#define READ_SECURITY_REGISTER_CMD           0x77

/* Additional commands */
#define MAIN_MEMORY_PAGE_TO_BUFFER1_XFER     0x53
#define MAIN_MEMORY_PAGE_TO_BUFFER2_XFER     0x55
#define MAIN_MEMORY_PAGE_TO_BUFFER_1_COMPARE 0x60
#define MAIN_MEMORY_PAGE_TO_BUFFER_2_COMPARE 0x61
#define AUTO_PAGE_REWRITE_THROUGH_BUFFER_1   0x58
#define AUTO_PAGE_REWRITE THROUGH BUFFER_2   0x59
#define DEEP_PWER_DOWN                       0xB9
#define RESUME_FROM_DEEP_POWER_DOWN          0xAB
#define STATUS_READ_CMD                      0xD7
#define READ_MANUFACTURER_DEVICE_ID          0x9F

/* Legacy commands
 * NOT RECCOMMENDED FOR NEW DESIGNS
 */
#define LEGACY_BUFFER_1_READ                 0x54
#define LEGACY_BUFFER_2_READ                 0x56
#define LEGACY_MAIN_MEMORY_PAGE_READ         0x52
#define LEGACY_CONTINOUS_ARRAY_READ          0x68
#define LEGACY_STATUS_REGISTER_READ          0x57

#define SPIFLASH_DEVICEID_64MBIT             0x28
#define SPIFLASH_DEVICEID_32MBIT             0x27
#define SPIFLASH_DEVICEID_8MBIT              0x25

#endif /* FLASH_OPCODES_H_ */