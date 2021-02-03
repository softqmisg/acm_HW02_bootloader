/**
 *******************************************************************************
 * STM32 Bootloader Source
 *******************************************************************************
 * @author Akos Pasztor
 * @file   bootloader.c
 * @brief  This file contains the functions of the bootloader. The bootloader
 *	       implementation uses the official HAL library of ST.
 *
 * @see    Please refer to README for detailed information.
 *******************************************************************************
 * @copyright (c) 2020 Akos Pasztor.                    https://akospasztor.com
 *******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "bootloader.h"

/* Private defines -----------------------------------------------------------*/
#define BOOTLOADER_VERSION_MAJOR 1 /*!< Major version */
#define BOOTLOADER_VERSION_MINOR 1 /*!< Minor version */
#define BOOTLOADER_VERSION_PATCH 2 /*!< Patch version */
#define BOOTLOADER_VERSION_RC    0 /*!< Release candidate version */

/* Private typedef -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/** Private variable for tracking flashing progress */
static uint32_t flash_ptr = APP_ADDRESS;

/**
 * @brief  This function initializes bootloader and flash.
 * @return Bootloader error code ::eBootloaderErrorCodes
 * @retval BL_OK is returned in every case
 */
uint8_t Bootloader_Init(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
//    __HAL_RCC_FLASH_CLK_ENABLE();

    /* Clear flash flags */
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_BSY |\
    		FLASH_FLAG_EOP | \
			FLASH_FLAG_OPERR |\
			FLASH_FLAG_PGAERR | \
			FLASH_FLAG_PGPERR | \
			FLASH_FLAG_PGSERR | \
			FLASH_FLAG_WRPERR\
			);
    HAL_FLASH_Lock();

    return BL_OK;
}

/**
 * @brief  This function erases the user application area in flash
 * @return Bootloader error code ::eBootloaderErrorCodes
 * @retval BL_OK: upon success
 * @retval BL_ERR: upon failure
 */
uint8_t Bootloader_Erase(void)
{
    //uint32_t               NbrOfSectors = 0;
    uint32_t               SectorError  = 0;
    FLASH_EraseInitTypeDef pEraseInit;
    HAL_StatusTypeDef      status = HAL_OK;

    HAL_FLASH_Unlock();



    /* Get the number of pages to erase */
    ///Mehdi Code
    pEraseInit.TypeErase	=FLASH_TYPEERASE_SECTORS;
    pEraseInit.Banks		=FLASH_BANK_1;
    pEraseInit.Sector		=APP_SECTOR_START;//sector5 (after 128K)
    pEraseInit.NbSectors	=APP_SECTOR_NBR;// 7 sectors
    pEraseInit.VoltageRange= VOLTAGE_RANGE_3; //x32bit
    status=HAL_FLASHEx_Erase(&pEraseInit,&SectorError);
//	//////////////////////////

//    /////////////////////////////
    HAL_FLASH_Lock();

    return (status == HAL_OK) ? BL_OK : BL_ERASE_ERROR;
}

/**
 * @brief  Begin flash programming: this function unlocks the flash and sets
 *         the data pointer to the start of application flash area.
 * @see    README for futher information
 * @return Bootloader error code ::eBootloaderErrorCodes
 * @retval BL_OK is returned in every case
 */
uint8_t Bootloader_FlashBegin(void)
{
    /* Reset flash destination address */
    flash_ptr = APP_ADDRESS;

    /* Unlock flash */
    HAL_FLASH_Unlock();

    return BL_OK;
}

/**
 * @brief  Program 32bit data into flash: this function writes an 4byte (32bit)
 *         data chunk into the flash and increments the data pointer.
 * @see    README for futher information
 * @param  data: 32bit data chunk to be written into flash
 * @return Bootloader error code ::eBootloaderErrorCodes
 * @retval BL_OK: upon success
 * @retval BL_WRITE_ERROR: upon failure
 */
uint8_t Bootloader_FlashNext(uint32_t data)
{
    if(!(flash_ptr <= (FLASH_BASE + FLASH_SIZE - 4)) ||
       (flash_ptr < APP_ADDRESS))
    {
        HAL_FLASH_Lock();
        return BL_WRITE_ERROR;
    }

    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_ptr, data) ==  HAL_OK)
    {
        /* Check the written value */
        if(*(uint32_t*)flash_ptr != data)
        {
            /* Flash content doesn't match source content */
            HAL_FLASH_Lock();
            return BL_WRITE_ERROR;
        }
        /* Increment Flash destination address */
        flash_ptr += 4;
    }
    else
    {
        /* Error occurred while writing data into Flash */
        HAL_FLASH_Lock();
        return BL_WRITE_ERROR;
    }

    return BL_OK;
}

/**
 * @brief  Finish flash programming: this function finalizes the flash
 *         programming by locking the flash.
 * @see    README for futher information
 * @return Bootloader error code ::eBootloaderErrorCodes
 * @retval BL_OK is returned in every case
 */
uint8_t Bootloader_FlashEnd(void)
{
    /* Lock flash */
    HAL_FLASH_Lock();

    return BL_OK;
}

/**
 * @brief  This function returns the protection status of flash.
 * @return Flash protection status ::eFlashProtectionTypes
 */
uint8_t Bootloader_GetProtectionStatus(void)
{
    FLASH_OBProgramInitTypeDef OBStruct   = {0};
    uint8_t                    protection = BL_PROTECTION_NONE;

    HAL_FLASH_Unlock();

    //////////////////////////////////////////
    //Mehdi Code
    uint32_t wrpsectors=0;
    for(uint8_t sectors=APP_SECTOR_START;sectors<=FLASH_SECTOR_NBPERBANK;sectors++)
    	wrpsectors|=(uint32_t)(1<<sectors);

    HAL_FLASHEx_OBGetConfig(&OBStruct);
    /* WRP */
    if(((uint32_t)OBStruct.WRPSector&wrpsectors)!=wrpsectors) //3.9.9 reference manual: one sector is wrp
    	protection |= BL_PROTECTION_WRP;
    /* RDP */
    if(OBStruct.RDPLevel != OB_RDP_LEVEL_0)
    {
        protection |= BL_PROTECTION_RDP;
    }
//    ////////////////////////////////////////////////
//    /* Bank 1 */
//    OBStruct.PCROPConfig = FLASH_BANK_1;
//    OBStruct.WRPArea     = OB_WRPAREA_BANK1_AREAA;
//    HAL_FLASHEx_OBGetConfig(&OBStruct);
//    /* PCROP */
//    if(OBStruct.PCROPEndAddr > OBStruct.PCROPStartAddr)
//    {
//        if(OBStruct.PCROPStartAddr >= APP_ADDRESS)
//        {
//            protection |= BL_PROTECTION_PCROP;
//        }
//    }
//    /* WRP Area_A */
//    if(OBStruct.WRPEndOffset > OBStruct.WRPStartOffset)
//    {
//        if((OBStruct.WRPStartOffset * FLASH_PAGE_SIZE + FLASH_BASE) >=
//           APP_ADDRESS)
//        {
//            protection |= BL_PROTECTION_WRP;
//        }
//    }
//
//    OBStruct.WRPArea = OB_WRPAREA_BANK1_AREAB;
//    HAL_FLASHEx_OBGetConfig(&OBStruct);
//    /* WRP Area_B */
//    if(OBStruct.WRPEndOffset > OBStruct.WRPStartOffset)
//    {
//        if((OBStruct.WRPStartOffset * FLASH_PAGE_SIZE + FLASH_BASE) >=
//           APP_ADDRESS)
//        {
//            protection |= BL_PROTECTION_WRP;
//        }
//    }
//
//    /* Bank 2 */
//    OBStruct.PCROPConfig = FLASH_BANK_2;
//    OBStruct.WRPArea     = OB_WRPAREA_BANK2_AREAA;
//    HAL_FLASHEx_OBGetConfig(&OBStruct);
//    /* PCROP */
//    if(OBStruct.PCROPEndAddr > OBStruct.PCROPStartAddr)
//    {
//        if(OBStruct.PCROPStartAddr >= APP_ADDRESS)
//        {
//            protection |= BL_PROTECTION_PCROP;
//        }
//    }
//    /* WRP Area_A */
//    if(OBStruct.WRPEndOffset > OBStruct.WRPStartOffset)
//    {
//        if((OBStruct.WRPStartOffset * FLASH_PAGE_SIZE + FLASH_BASE +
//            FLASH_PAGE_SIZE * FLASH_PAGE_NBPERBANK) >= APP_ADDRESS)
//        {
//            protection |= BL_PROTECTION_WRP;
//        }
//    }
//
//    OBStruct.WRPArea = OB_WRPAREA_BANK2_AREAB;
//    HAL_FLASHEx_OBGetConfig(&OBStruct);
//    /* WRP Area_B */
//    if(OBStruct.WRPEndOffset > OBStruct.WRPStartOffset)
//    {
//        if((OBStruct.WRPStartOffset * FLASH_PAGE_SIZE + FLASH_BASE +
//            FLASH_PAGE_SIZE * FLASH_PAGE_NBPERBANK) >= APP_ADDRESS)
//        {
//            protection |= BL_PROTECTION_WRP;
//        }
//    }
//    /* RDP */
//    if(OBStruct.RDPLevel != OB_RDP_LEVEL_0)
//    {
//        protection |= BL_PROTECTION_RDP;
//    }
    //////////////////////////////////////////////////////////////////////////////////////////////////

    HAL_FLASH_Lock();
    return protection;
}

/**
 * @brief  This function configures the wirte protection of flash.
 * @param  protection: protection type ::eFlashProtectionTypes
 * @return Bootloader error code ::eBootloaderErrorCodes
 * @retval BL_OK: upon success
 * @retval BL_OBP_ERROR: upon failure
 */
uint8_t Bootloader_ConfigProtection(uint32_t protection)
{
    FLASH_OBProgramInitTypeDef OBStruct = {0};
    HAL_StatusTypeDef          status   = HAL_ERROR;

    status = HAL_FLASH_Unlock();
    status |= HAL_FLASH_OB_Unlock();
    ////////////////////////////////////////
    ////Mehdi Code
    uint32_t wrpsectors=0;
    for(uint8_t sectors=APP_SECTOR_START;sectors<=FLASH_SECTOR_NBPERBANK;sectors++)
    	wrpsectors|=(uint32_t)(1<<sectors);

    OBStruct.Banks=FLASH_BANK_1;
	OBStruct.OptionType |=OPTIONBYTE_WRP;
	OBStruct.WRPSector=wrpsectors;

    if(protection & BL_PROTECTION_WRP)
    {

    	OBStruct.WRPState=OB_WRPSTATE_ENABLE;
    }
    else
    {
    	OBStruct.WRPState=OB_WRPSTATE_DISABLE;
    }
    status |= HAL_FLASHEx_OBProgram(&OBStruct);


    if(status == HAL_OK)
    {
        /* Loading Flash Option Bytes - this generates a system reset. */
        status |= HAL_FLASH_OB_Launch();
    }

    status |= HAL_FLASH_OB_Lock();
    status |= HAL_FLASH_Lock();

    return (status == HAL_OK) ? BL_OK : BL_OBP_ERROR;
}

/**
 * @brief  This function checks whether the new application fits into flash.
 * @param  appsize: size of application
 * @return Bootloader error code ::eBootloaderErrorCodes
 * @retval BL_OK: if application fits into flash
 * @retval BL_SIZE_ERROR: if application does not fit into flash
 */
uint8_t Bootloader_CheckSize(uint32_t appsize)
{
    return ((FLASH_BASE + FLASH_SIZE - APP_ADDRESS) >= appsize) ? BL_OK
                                                                : BL_SIZE_ERROR;
}

/**
 * @brief  This function verifies the checksum of application located in flash.
 *         If ::USE_CHECKSUM configuration parameter is disabled then the
 *         function always returns an error code.
 * @return Bootloader error code ::eBootloaderErrorCodes
 * @retval BL_OK: if calculated checksum matches the application checksum
 * @retval BL_CHKS_ERROR: upon checksum mismatch or when ::USE_CHECKSUM is
 *         disabled
 */
uint8_t Bootloader_VerifyChecksum(uint32_t checksum,uint32_t size_file)
{
#if(USE_CHECKSUM)
    CRC_HandleTypeDef CrcHandle;
    volatile uint32_t calculatedCrc = 0;
    __HAL_RCC_CRC_FORCE_RESET();
    __HAL_RCC_CRC_RELEASE_RESET();

//    __HAL_RCC_CRC_CLK_ENABLE();
//    CrcHandle.Instance                     = CRC;
//    /*
//    CrcHandle.Init.DefaultPolynomialUse    = DEFAULT_POLYNOMIAL_ENABLE;
//    CrcHandle.Init.DefaultInitValueUse     = DEFAULT_INIT_VALUE_ENABLE;
//    CrcHandle.Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_NONE;
//    CrcHandle.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
//    CrcHandle.InputDataFormat              = CRC_INPUTDATA_FORMAT_WORDS;
//    */
//
//    if(HAL_CRC_Init(&CrcHandle) != HAL_OK)
//    {
//        return BL_CHKS_ERROR;
//    }

    calculatedCrc =
        HAL_CRC_Calculate(&CrcHandle, (uint32_t*)APP_ADDRESS, size_file);

    __HAL_RCC_CRC_FORCE_RESET();
    __HAL_RCC_CRC_RELEASE_RESET();

    if((uint32_t)calculatedCrc == (uint32_t)checksum)
    {
        return BL_OK;
    }
	printf("Checksum error calculate=%lu,crc in file=%lu\n\r",calculatedCrc,checksum);

#endif
    return BL_CHKS_ERROR;
}

/**
 * @brief  This function checks whether a valid application exists in flash.
 *         The check is performed by checking the very first DWORD (4 bytes) of
 *         the application firmware. In case of a valid application, this DWORD
 *         must represent the initialization location of stack pointer - which
 *         must be within the boundaries of RAM.
 * @return Bootloader error code ::eBootloaderErrorCodes
 * @retval BL_OK: if first DWORD represents a valid stack pointer location
 * @retval BL_NO_APP: first DWORD value is out of RAM boundaries
 */
uint8_t Bootloader_CheckForApplication(void)
{
    return (((*(uint32_t*)APP_ADDRESS) - RAM_BASE) <= RAM_SIZE) ? BL_OK
                                                               : BL_NO_APP;
}

/**
 * @brief  This function performs the jump to the user application in flash.
 * @details The function carries out the following operations:
 *  - De-initialize the clock and peripheral configuration
 *  - Stop the systick
 *  - Set the vector table location (if ::SET_VECTOR_TABLE is enabled)
 *  - Sets the stack pointer location
 *  - Perform the jump
 */
void Bootloader_JumpToApplication(void)
{
    uint32_t  JumpAddress = *(__IO uint32_t*)(APP_ADDRESS + 4);
    pFunction Jump        = (pFunction)JumpAddress;

    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

#if(SET_VECTOR_TABLE)
    SCB->VTOR = APP_ADDRESS;
#endif

    __set_MSP(*(__IO uint32_t*)APP_ADDRESS);
    Jump();
    while(1);
}

/**
 * @brief  This function performs the jump to the MCU System Memory (ST
 *         Bootloader).
 * @details The function carries out the following operations:
 *  - De-initialize the clock and peripheral configuration
 *  - Stop the systick
 *  - Remap the system flash memory
 *  - Perform the jump
 */
void Bootloader_JumpToSysMem(void)
{
    uint32_t  JumpAddress = *(__IO uint32_t*)(SYSMEM_ADDRESS + 4);
    pFunction Jump        = (pFunction)JumpAddress;

    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

    __set_MSP(*(__IO uint32_t*)SYSMEM_ADDRESS);
    Jump();

    while(1)
        ;
}

/**
 * @brief  This function returns the version number of the bootloader library.
 *         Semantic versioning is used for numbering.
 * @see    Semantic versioning: https://semver.org
 * @return Bootloader version number combined into an uint32_t:
 *          - [31:24] Major version
 *          - [23:16] Minor version
 *          - [15:8]  Patch version
 *          - [7:0]   Release candidate version
 */
uint32_t Bootloader_GetVersion(void)
{
    return ((BOOTLOADER_VERSION_MAJOR << 24) |
            (BOOTLOADER_VERSION_MINOR << 16) | (BOOTLOADER_VERSION_PATCH << 8) |
            (BOOTLOADER_VERSION_RC));
}
