/*
 * bootloader_user.h
 *
 *  Created on: Jul 11, 2020
 *      Author: mehdi
 */

#ifndef INC_BOOTLOADER_USER_H_
#define INC_BOOTLOADER_USER_H_

#define SD_ID			10
#define USB_ID			20
#define BOTH_ID			30
#define MEM_CHECK_OK	0
////////////////received code from application
#define WRITE_FROM_SD			1// write from SD with never downgrade
#define WRITE_FROM_USB			2//write from USB with never downgrade
#define FORCE_WRITE_FROM_SD		3//write from SD with forced to flash
#define FORCE_WRITE_FROM_USB	4//write from USB with force to flash
///////////////return  codes to application/////////////////////////////
#define	MEM_NOT_PRESENT			1
#define	MEM_MOUNT_FAIL			2
#define	FILE_OPEN_FAIL			3
#define	FILE_READ_FAIL			4
#define	FLASH_ERASE_FAIL		5
#define	FLASH_WRITE_FAIL		6
#define	FLASH_CHECKVERSION_FAIL	7
#define	FLASH_CHECKSIZE_FAIL	8
#define	FLASH_VERIFY_FAIL		9
#define	FLASH_CHECKSUM_FAIL		10
#define	FLASH_WRITE_OK			11
#define POWER_UP				12

void Enter_Bootloader(char *filename);
uint8_t SD_Init(void);
void SD_DeInit(void);
void SD_Eject(void);

void Peripherials_DeInit(void);
uint8_t Check_SDCard(char *firmware_path, uint16_t *firmware_version,
		uint32_t *firmware_checksum);
uint8_t Check_USBMEM(char *firmware_path, uint16_t *firmware_version,
		uint32_t *firmware_checksum);
uint8_t Write_Flash(uint8_t ID_mem, char *filename, uint16_t version,
		uint32_t checksum);
uint8_t goto_application(void);
#endif /* INC_BOOTLOADER_USER_H_ */
