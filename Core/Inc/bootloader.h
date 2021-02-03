/**
 *******************************************************************************
 * STM32 Bootloader Header
 *******************************************************************************
 * @author Akos Pasztor
 * @file   bootloader.h
 * @brief  This file contains the bootloader configuration parameters,
 *	       function prototypes and other required macros and definitions.
 *
 * @see    Please refer to README for detailed information.
 *******************************************************************************
 * @copyright (c) 2020 Akos Pasztor.                    https://akospasztor.com
 *******************************************************************************
 */

#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

typedef void (*pFunction)(void); /*!< Function pointer definition */

/** Bootloader Configuration
 * @defgroup Bootloader_Configuration Bootloader Configuration
 * @{
 */

/** Select target MCU family: please define the target MCU family type below.
 * Currently supported MCU families:
 *  - STM32L4
 */
#define STM32F4

/** Check application checksum on startup */
#define USE_CHECKSUM 0

/** Enable write protection after performing in-app-programming */
#define USE_WRITE_PROTECTION 0

/** Automatically set vector table location before launching application */
#define SET_VECTOR_TABLE 1

/** Clear reset flags
 *  - If enabled: bootloader clears reset flags. (This occurs only when OBL RST
 * flag is active.)
 *  - If disabled: bootloader does not clear reset flags, not even when OBL RST
 * is active.
 */
#define CLEAR_RESET_FLAGS 1

/** Start address of application space in flash */
#define APP_ADDRESS (uint32_t)0x08020000 //after 128K

/** Start address of application checksum in flash */
#define CRC_ADDRESS (uint32_t)0x080FFFFC
/** Address of auto program;
 * 1 value in this address, auto program, 0 value key check
 * after read this address it will be cleared
 *  */
#define AUTOPROG_ADDRESS	(uint32_t)0x080c0004 //NOT USED
/** Address of the device which be used for programming
 * values:
 * 0=SDCARD
 * 1=
 */
#define DEVICEPROG_ADDRESS	(uint32_t)0x080c0002 //NOT USED
/** Address of System Memory (ST Bootloader) */
#define SYSMEM_ADDRESS (uint32_t)0x1FFF0000  //from datasheet memory mappyng
/** @} */
/* End of configuration ------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
/* Include the appropriate header file */
#if defined(STM32F4)
#include "stm32F4xx.h"
#else
#error "Target MCU header file is not defined or unsupported."
#endif

/* Defines -------------------------------------------------------------------*/
/** Size of application in DWORD (32bits or 4bytes) */
//#define APP_SIZE (uint32_t)(((END_ADDRESS - APP_ADDRESS) + 3) / 4) //NOT USED

/** Number of pages per bank in flash */
//#define FLASH_PAGE_NBPERBANK (256)
#define FLASH_SECTOR_NBPERBANK	FLASH_SECTOR_11
#define APP_SECTOR_START	FLASH_SECTOR_5 //after 128KB of flash
#define APP_SECTOR_NBR		FLASH_SECTOR_NBPERBANK-	APP_SECTOR_START+1

/* MCU RAM information (to check whether flash contains valid application) */
#define RAM_BASE SRAM1_BASE     /*!< Start address of RAM */
#define RAM_SIZE 0x20000//SRAM1_SIZE_MAX /*!< RAM size in bytes */

#define FLASH_SIZE (uint32_t)0x100000 //1MB
/* Enumerations --------------------------------------------------------------*/
/** Bootloader error codes */
enum eBootloaderErrorCodes
{
    BL_OK = 0,      /*!< No error */
    BL_NO_APP,      /*!< No application found in flash */
    BL_SIZE_ERROR,  /*!< New application is too large for flash */
    BL_CHKS_ERROR,  /*!< Application checksum error */
    BL_ERASE_ERROR, /*!< Flash erase error */
    BL_WRITE_ERROR, /*!< Flash write error */
    BL_OBP_ERROR    /*!< Flash option bytes programming error */
};

/** Flash Protection Types */
enum eFlashProtectionTypes
{
    BL_PROTECTION_NONE  = 0,   /*!< No flash protection */
    BL_PROTECTION_WRP   = 0x1, /*!< Flash write protection */
    BL_PROTECTION_RDP   = 0x2, /*!< Flash read protection */
    BL_PROTECTION_PCROP = 0x4, /*!< Flash propietary code readout protection */
};

/* Functions -----------------------------------------------------------------*/
uint8_t Bootloader_Init(void);
uint8_t Bootloader_Erase(void);

uint8_t Bootloader_FlashBegin(void);
uint8_t Bootloader_FlashNext(uint32_t data);
uint8_t Bootloader_FlashEnd(void);

uint8_t Bootloader_GetProtectionStatus(void);
uint8_t Bootloader_ConfigProtection(uint32_t protection);

uint8_t Bootloader_CheckSize(uint32_t appsize);
uint8_t Bootloader_VerifyChecksum(uint32_t checksum,uint32_t size_file);
uint8_t Bootloader_CheckForApplication(void);
void    Bootloader_JumpToApplication(void);
void    Bootloader_JumpToSysMem(void);

uint32_t Bootloader_GetVersion(void);

#endif /* __BOOTLOADER_H */
