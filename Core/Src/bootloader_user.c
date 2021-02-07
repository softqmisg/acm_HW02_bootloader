/*
 * bootloader_user.c
 *
 *  Created on: Jul 11, 2020
 *      Author: mehdi
 */
#include "usart.h"
#include "fatfs.h"
#include "bootloader.h"
#include "bootloader_user.h"
#include "eeprom.h"
#include "usb_host.h"
#include "gpio.h"
#include "iwdg.h"
#include "fonts/font_tahoma.h"
#include "graphics.h"
#include "st7565.h"
#include "libbmp.h"

extern uint32_t __attribute__((section(".newsection"))) sharedmem;
extern USBH_HandleTypeDef hUsbHostHS;
/**************************************************************/
void Peripherials_DeInit(void) {
//	BSP_SD_DeInit();
//	FATFS_DeInit();
	MX_FATFS_DeInit();
	HAL_UART_DeInit(&huart2);
	HAL_UART_MspDeInit(&huart2);
	HAL_UART_DeInit(&huart3);
	HAL_UART_MspDeInit(&huart3);
	BSP_SD_DeInit();
	MX_USB_HOST_DeInit();
	MX_GPIO_DeInit();
	//MX_IWDG_DeInit();

}
/*** Bootloader ***************************************************************/
/*** SD Card ******************************************************************/
uint8_t Check_SDCard(char *firmware_path, uint16_t *firmware_version,
		uint32_t *firmware_checksum) {
	FRESULT fr;
	char readline[100];
	if (BSP_SD_IsDetected() == SD_NOT_PRESENT) {
		printf("SD card is not present.\n\r");
		return MEM_NOT_PRESENT;
	}
	printf("SD card is present\n\r");
	/* Mount SD card */
	fr = f_mount(&SDFatFS, (TCHAR const*) SDPath, 1);
	if (fr != FR_OK) {
		/* f_mount failed */
		printf("SD card cannot be mounted.\n\r");
		printf("FatFs error code: %u\n\r", fr);
		return MEM_MOUNT_FAIL;
	}
	printf("SD card mountes\n\r");
	/* Open file for programming */
	fr = f_open(&SDFile, "0:/boot.ini", FA_READ);
	if (fr != FR_OK) {
		/* f_open failed */
		printf("File cannot be opened.\n\r");
		printf("FatFs error code: %u\n\r", fr);
		f_mount(NULL, (TCHAR const*) SDPath, 0);
		return FILE_OPEN_FAIL;
	}
	f_gets(readline, 100, &SDFile);
//	if (fr != FR_OK) {
//		printf("File cannot be read.\n\r");
//		printf("FatFs error code: %u\n\r", fr);
//		f_mount(NULL, (TCHAR const*) SDPath, 0);
//		return FILE_READ_FAIL;
//	}
	*firmware_version = atol(readline);
	f_gets(firmware_path, 100, &SDFile);
	uint8_t index = 0;
	while (*(firmware_path + index) != '\n' || *(firmware_path + index) != '\r')
		index++;
	firmware_path[index] = '\0';
//	if (fr != FR_OK) {
//		printf("File cannot be read.\n\r");
//		printf("FatFs error code: %u\n\r", fr);
//		f_mount(NULL, (TCHAR const*) SDPath, 0);
//		return FILE_READ_FAIL;
//	}
	f_gets(readline, 100, &SDFile);
//	if (fr != FR_OK) {
//		printf("File cannot be read.\n\r");
//		printf("FatFs error code: %u\n\r", fr);
//		f_mount(NULL, (TCHAR const*) SDPath, 0);
//		return FILE_READ_FAIL;
//	}
	*firmware_checksum = atol(readline);
	fclose(&SDFile);
	f_mount(NULL, (TCHAR const*) SDPath, 0);
	return MEM_CHECK_OK;
}
/*** USB Memory ******************************************************************/
uint8_t Check_USBMEM(char *firmware_path, uint16_t *firmware_version,
		uint32_t *firmware_checksum) {
	FIL fp;
	FRESULT fr;
	char readline[200];
	uint16_t usb_counter = 0;
	do {
		MX_USB_HOST_Process();
		HAL_Delay(10);
		usb_counter++;
	} while ((usb_counter < 1000) && (!USBH_MSC_IsReady(&hUsbHostHS)));
	if (usb_counter >= 1000) {
		printf("USB is not present.\n\r");
		return MEM_NOT_PRESENT;
	}
	printf("USB is present\n\r");
	/* Mount SD card */
	fr = f_mount(&USBHFatFS, (TCHAR const*) USBHPath, 1);
	if (fr != FR_OK) {
		/* f_mount failed */
		printf("USB cannot be mounted.\n\r");
		printf("FatFs error code: %u\n\r", fr);
		return MEM_MOUNT_FAIL;
	}
	printf("USB was mounted.\n\r");
	HAL_Delay(100);
	/* Open file for programming */
	fr = f_open(&USBHFile, "1:/boot.ini", FA_READ);
	if (fr != FR_OK) {
		/* f_open failed */
		printf("File cannot be opened.\n\r");
		printf("FatFs error code: %u\n\r", fr);
		f_mount(NULL, (TCHAR const*) USBHPath, 0);
		return FILE_OPEN_FAIL;
	}
	f_gets(readline, 100, &USBHFile);
	readline[strlen(readline) - 1] = 0;
	*firmware_version = atol(readline);
//	HAL_Delay(50);
//	if (fr != FR_OK) {
//		printf("File cannot be read.\n\r");
//		printf("FatFs error code: %u\n\r", fr);
//		f_mount(NULL, (TCHAR const*) USBHPath, 0);
//		return FILE_READ_FAIL;
//	}

	f_gets(readline, 100, &USBHFile);
	readline[strlen(readline) - 1] = 0;
	sprintf(firmware_path, "%s", readline);

//	if (fr != FR_OK) {
//		printf("File cannot be read.\n\r");
//		printf("FatFs error code: %u\n\r", fr);
//		f_mount(NULL, (TCHAR const*) USBHPath, 0);
//		return FILE_READ_FAIL;
//	}
	f_gets(readline, 100, &USBHFile);
	readline[strlen(readline) - 1] = 0;
	*firmware_checksum = atol(readline);
	HAL_Delay(50);
//	if (fr != FR_OK) {
//		printf("File cannot be read.\n\r");
//		printf("FatFs error code: %u\n\r", fr);
//		f_mount(NULL, (TCHAR const*) USBHPath, 0);
//		return FILE_READ_FAIL;
//	}

	f_close(&USBHFile);
	f_mount(NULL, (TCHAR const*) USBHPath, 0);
	return MEM_CHECK_OK;
}
/*** Go application******************************************************************/
uint8_t goto_application(void) {
	if (Bootloader_CheckForApplication() == BL_OK) {
		printf("Launching Application.\n\r");
//		draw_text("Launching Application", 0, 50, Tahoma8, 1, 0);
//		glcd_refresh();
//		HAL_Delay(1000);
		/* De-initialize bootloader hardware & peripherals */
		Peripherials_DeInit();
		/* Launch application */
		Bootloader_JumpToApplication();
		return 1;
	}
	return 0;
}
/***Write_Flash******************************************************************/
/*
 * Write program to flash memory & verify
 */
extern uint16_t VirtAddVarTab[NB_OF_VAR];
uint8_t Write_Flash(uint8_t ID_mem, char *name, uint16_t version,
		uint32_t checksum) {
	uint16_t current_version;
	FRESULT fr;
	UINT num;
	uint8_t status;
	uint32_t data;
	uint32_t cntr;
	uint32_t addr;
	FSIZE_t filesize;
	char filename[100];
	char tmp_str[30];
	if (ID_mem == SD_ID)
		sprintf(filename, "0:/%s", name);
	else
		sprintf(filename, "1:/%s", name);
	printf("name of file %s\n\r", filename);
	///////////////////////get version//////////////////////////////////
	HAL_FLASH_Unlock();
	VirtAddVarTab[0] = EE_ADDR_VERSION;
	EE_ReadVariable(VirtAddVarTab[0], &current_version);
	HAL_FLASH_Lock();
	//////////////////////check downgrade?////////////////////////////////////////////
	if (version == current_version) {
		printf(
				"current version is same as new version but programming will be done\n\r");
	} else if (version < current_version) {

		if (sharedmem == WRITE_FROM_SD || sharedmem == WRITE_FROM_USB) {
			printf("Downgrade firmware is not possible.\n\r");
			return FLASH_CHECKVERSION_FAIL;
		}
		printf("Downgrade firmware.\n\r");
	} else {
		printf("Upgrade firmware.\n\r");
	}
	////////////////////////////////////////////////////////////////
	/* Mount MEM */
	if (ID_mem == SD_ID)
		fr = f_mount(&SDFatFS, (TCHAR const*) SDPath, 1);
	else
		fr = f_mount(&USBHFatFS, (TCHAR const*) USBHPath, 1);
	if (fr != FR_OK) {
		/* f_mount failed */
		printf("Mem cannot be mounted.\n\r");
		printf("FatFs error code: %u\n\r", fr);
		return MEM_MOUNT_FAIL;
	}
	/* check file  present on MEM */
	if (ID_mem == SD_ID)
		fr = f_open(&SDFile, filename, FA_READ);
	else
		fr = f_open(&USBHFile, filename, FA_READ);

	if (fr != FR_OK) {
		/* f_open failed */
		printf("File cannot be opened.\n\r");
		printf("FatFs error code: %u\n\r", fr);
		return FILE_OPEN_FAIL;
	}
	/* check file  size:must less than flash size on MEM */
	if (ID_mem == SD_ID)
		filesize = f_size(&SDFile);
	else
		filesize = f_size(&USBHFile);
	if (ID_mem == SD_ID)
		printf("Software found on SD(%lu byte).\n\r", filesize);
	else
		printf("Software found on USBH(%lu byte).\n\r", filesize);
	if (Bootloader_CheckSize((uint32_t) filesize) != BL_OK) {
		printf("Error: app on SD card is too large.\n\r");
		if (ID_mem == SD_ID)
			f_close(&SDFile);
		else
			f_close(&USBHFile);
		return FLASH_CHECKSIZE_FAIL;
	}
	//	/* Step 1: Init Bootloader and Flash */
	Bootloader_Init();
	/* Step 2: Erase Flash */
	printf("Erasing flash...\n\r");
	draw_text("Erase Flash...", 0, 10, Tahoma8, 1, 0);
	glcd_refresh();
	if (Bootloader_Erase() != BL_OK) {
		printf("Flashe erase error.\n\r");
//		sharedmem = FLASH_ERASE_FAIL;
//		goto_application();
		return FLASH_ERASE_FAIL;
	}

	printf("Flash erase finished.\n\r");
	HAL_Delay(1000);
//	/* If BTN is pressed, then skip programming */
//	if (IS_BTN_PRESSED()) {
//		printf("Programming skipped.\n\r");
//		if (ID_mem == SD_ID)
//			f_close(&SDFile);
//		else
//			f_close(&USBHFile);
//		return;
//	}
	/* Step 3: Programming */
	printf("Starting programming...\n\r");
	cntr = 0;
	Bootloader_FlashBegin(); //unlock flash
	do {
#if __WATCHDOG_ENABLE__
		HAL_IWDG_Refresh(&hiwdg);
#endif
		data = 0xFFFFFFFF; // flash write is in 32bit mode write
		if (ID_mem == SD_ID)
			fr = f_read(&SDFile, &data, 4, &num);
		else
			fr = f_read(&USBHFile, &data, 4, &num);

		if (num) {
			status = Bootloader_FlashNext(data);
			if (status == BL_OK) {
				cntr++;
			} else {
				printf("Programming error at: %lu byte\n\r", (cntr * 4));

				if (ID_mem == SD_ID)
					f_close(&SDFile);
				else
					f_close(&USBHFile);
//				sharedmem = FLASH_WRITE_FAIL;
				return FLASH_WRITE_FAIL;
			}
		}
		if (cntr % 256 == 0) {
			/* Toggle green LED during programming every 958byte */
			printf("%d %% was flahsed(%lu/%lu)\n\r",
					(uint8_t) ((uint32_t) (cntr * 4) * 100 / (uint32_t) filesize),
					(cntr * 4), (uint32_t) filesize);
			sprintf(tmp_str,"%03d%%_flashed",(uint8_t) ((uint32_t) (cntr * 4) * 100 / (uint32_t) filesize));
			draw_text(tmp_str, 0, 20, Tahoma8, 1, 0);
			glcd_refresh();
		}
	} while ((fr == FR_OK) && (num > 0));
	/* Step 4: Finalize Programming */
	Bootloader_FlashEnd();
	if (ID_mem == SD_ID)
		f_close(&SDFile);
	else
		f_close(&USBHFile);
	printf("Programming finished.\n\r");
	printf("Flashed: %lu bytes.", (cntr * 4));
	/* Open file for verification */
	if (ID_mem == SD_ID)
		fr = f_open(&SDFile, filename, FA_READ);
	else
		fr = f_open(&USBHFile, filename, FA_READ);

	if (fr != FR_OK) {
		/* f_open failed */
		printf("File cannot be opened.\n\r");
		printf("FatFs error code: %u\n\r", fr);
//		sharedmem = ID_mem + FLASH_READFILE_FAIL;
		return FILE_OPEN_FAIL;
	}
	if (ID_mem == SD_ID)
		printf("Software found on SD.\n\r");
	else
		printf("Software found on USBH.\n\r");

	/* Step 5: Verify Flash Content */
	addr = APP_ADDRESS;
	cntr = 0;
	do {
#if __WATCHDOG_ENABLE__
		HAL_IWDG_Refresh(&hiwdg);
#endif
		data = 0xFFFFFFFF;
		if (ID_mem == SD_ID)
			fr = f_read(&SDFile, &data, 4, &num);
		else
			fr = f_read(&USBHFile, &data, 4, &num);

		if (num) {
			if (*(uint32_t*) addr == (uint32_t) data) {
				addr += 4;
				cntr++;
			} else {
				printf("Verification error at: %lu byte.\n\r", (cntr * 4));
				if (ID_mem == SD_ID)
					f_close(&SDFile);
				else
					f_close(&USBHFile);
//				sharedmem = ID_mem + FLASH_VERIFY_FAIL;
				return FLASH_VERIFY_FAIL;
			}
		}
		if (cntr % 256 == 0) {
			/* Toggle green LED during verification */
			printf("%d %% was verified(%lu/%lu)\n\r",
					(uint8_t) ((uint32_t) (cntr * 4) * 100 / (uint32_t) filesize),
					(cntr * 4), (uint32_t) filesize);
			sprintf(tmp_str,"%03d%% verified",(uint8_t) ((uint32_t) (cntr * 4) * 100 / (uint32_t) filesize));
			draw_text(tmp_str, 0, 30, Tahoma8, 1, 0);
			glcd_refresh();

		}
	} while ((fr == FR_OK) && (num > 0));
	printf("Verification passed.\n\r");
	if (ID_mem == SD_ID)
		f_close(&SDFile);
	else
		f_close(&USBHFile);
#if(USE_CHECKSUM)
	if (Bootloader_VerifyChecksum((uint32_t)checksum, (uint32_t) filesize) != BL_OK) {
		printf("Checksum error \n\r");
//		sharedmem = ID_mem + FLASH_CHECKSUM_FAIL;
		return FLASH_CHECKSUM_FAIL;
	}
	else
	{
		printf("Checksum OK\n\r");
	}

#endif
#if(USE_WRITE_PROTECTION)
		    printf("Enablig flash write protection and generating system reset...\n\r");
		    if(Bootloader_ConfigProtection(BL_PROTECTION_WRP) != BL_OK)
		    {
		        printf("Failed to enable write protection.\n\r");
		        printf("Exiting Bootloader.\n\r");
		    }
#endif
	printf("flashed programmed & verified\n\r");
	if(HAL_FLASH_Unlock()!=HAL_OK)
		printf("flash unlock error\n\r");
	HAL_Delay(500);
	if (EE_Init() != EE_OK) {
		printf("EE  prom iNit problem\n\r");
	}
	HAL_Delay(500);
	VirtAddVarTab[0] = EE_ADDR_VERSION;
	uint16_t r;
	if ((r = EE_WriteVariable(VirtAddVarTab[0], (uint16_t) version))
			!= HAL_OK) {
		printf("EE Write Error %d\n\r", r);
	}
	uint16_t Data;
	if((r=EE_ReadVariable(VirtAddVarTab[0], &Data))!=HAL_OK){
		printf("EE Read Error %d\n\r", r);;
	}
	HAL_Delay(100);
	printf("Version Write in Flash %d\n\r", Data);

	HAL_Delay(500);
	HAL_FLASH_Lock();
	return FLASH_WRITE_OK;
}

