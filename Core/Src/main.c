/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "dma.h"
#include "fatfs.h"
#include "iwdg.h"
#include "rtc.h"
#include "sdio.h"
#include "tim.h"
#include "usart.h"
#include "usb_host.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bootloader.h"
#include "bootloader_user.h"
#include "retarget.h"
#include "fonts/font_tahoma.h"
#include "graphics.h"
#include "st7565.h"
#include "libbmp.h"
#include "eeprom.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t __attribute__((section(".newsection"))) sharedmem;
uint16_t VirtAddVarTab[NB_OF_VAR] = { 10, 0, 0 };
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */
	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */
	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_TIM4_Init();
	MX_RTC_Init();
	MX_SDIO_SD_Init();
	MX_FATFS_Init();
	MX_USART2_UART_Init();
	MX_USART3_UART_Init();
	MX_CRC_Init();
	MX_USB_HOST_Init();
#if __WATCHDOG_ENABLE__
  MX_IWDG_Init();
#endif
	/* USER CODE BEGIN 2 */
  uint16_t r;
	RetargetInit(&huart3);
	////////////////////////////////////////////////////////////////////////////
	printf("starting bootloader,shared mem= %lu.\n\r", sharedmem);
	HAL_FLASH_Unlock();
	if ((r=EE_Init()) != EE_OK) {
		printf("EEprom Init problem:%d\n\r",r);
	}
	HAL_Delay(500);
	uint16_t current_version;
	VirtAddVarTab[0] = EE_ADDR_VERSION;
	if((r=EE_ReadVariable(VirtAddVarTab[0], &current_version))!=EE_OK)
		printf("EEprom Read error:%d\n\r",r);

	HAL_FLASH_Lock();
	printf("version of firmware:%d.\n\r", current_version);

	HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port, USB_PWR_EN_Pin, GPIO_PIN_SET);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(USB_PWR_EN_GPIO_Port, USB_PWR_EN_Pin, GPIO_PIN_RESET);
	////////////////////////////////////////////////////
//    CRC_HandleTypeDef CrcHandle;
//    volatile uint32_t calculatedCrc = 0;
//    calculatedCrc =
//        HAL_CRC_Calculate(&hcrc, (uint32_t*)APP_ADDRESS, (uint32_t)9080);
//    printf("crc=%lu",calculatedCrc);
//    while(1);
	////////////////////////////////////////////////
	glcd_init(128, 64);
	glcd_flip_screen(XLR_YTB);
	FIL myfile;
	FRESULT fr;
	if ((fr = f_mount(&SDFatFS, (TCHAR const*) SDPath, 1)) != FR_OK) {
		printf("error mount SD\n\r");
	} else {
		bmp_img img;
		if (bmp_img_read(&img, "logo.bmp") == BMP_OK)
			draw_bmp_h(0, 0, img.img_header.biWidth, img.img_header.biHeight,
					img.img_pixels, 1);
		else
			printf("bmp file error\n\r");
		f_mount(&SDFatFS, "", 1);
		bmp_img_free(&img);
		draw_rectangle(0, 0, 127 , 63,1);
		glcd_refresh();
		HAL_Delay(1000);
	}
	glcd_blank();

	draw_text("Bootloader...", 0, 0, Tahoma8, 1, 0);
	glcd_refresh();
	////////////////////////////////////////////////////////////////////////////
	uint8_t BTNcounter = 0;
	uint8_t result_usb = MEM_NOT_PRESENT, result_sd = MEM_NOT_PRESENT;
	char firmware_path[100];
	uint16_t firmware_version;
	uint32_t firmware_checksum;
	while (IS_BTN_PRESSED() && BTNcounter < 200) {
		if (BTNcounter == 20) {
			printf("Release button to enter Bootloader.\n\r");
			draw_text("Release to Enter Loader", 0, 10, Tahoma8, 1, 0);
			glcd_refresh();
		}
		BTNcounter++;
		MX_USB_HOST_Process();
		HAL_Delay(100);
#if __WATCHDOG_ENABLE__
			HAL_IWDG_Refresh(&hiwdg);
#endif
	}
	if ((BTNcounter < 100 && BTNcounter > 20)
			|| (sharedmem < WRITE_FROM_SD || sharedmem > FORCE_WRITE_FROM_USB)) {
		if (!(BTNcounter < 100 && BTNcounter > 20)) {
			printf("try to go to application\n\r");
			sharedmem = POWER_UP;
			glcd_blank();
			draw_text("Goto Application", 0, 0, Tahoma8, 1, 0);
			glcd_refresh();
			goto_application();
			printf(
					"Application was not present, try flashed it from SD & USB\n\r");
		}
		glcd_blank();
		draw_text("try SD:", 0, 0, Tahoma8, 1, 0);
		glcd_refresh();
		if ((result_sd = Check_SDCard(firmware_path, &firmware_version,
				&firmware_checksum)) == MEM_CHECK_OK) {
			printf("Firmware found in SD\n\r");
			printf("Firmware version:%lu\n\r", firmware_version);
			printf("Firmware checksum:%lu\n\r", firmware_checksum);
			printf("Firmware path:%s\n\r", firmware_path);
			if ((result_sd = Write_Flash(SD_ID, firmware_path, firmware_version,
					firmware_checksum)) == FLASH_WRITE_OK) {
				printf("Program Flashed from SD\n\r");
				draw_text("Flashed from SD", 0, 10, Tahoma8, 1, 0);
				glcd_refresh();
				HAL_Delay(1000);

			}
		} else {
			printf("SD CARD Have problem(%d)\n\r", result_sd);
			draw_text("Failed from SD", 0, 10, Tahoma8, 1, 0);
			glcd_refresh();
			HAL_Delay(1000);

		}

		sharedmem = result_sd;
		glcd_blank();
		draw_text("try USB:", 0, 0, Tahoma8, 1, 0);
		glcd_refresh();
		if (result_sd != FLASH_WRITE_OK) {
			if ((result_usb = Check_USBMEM(firmware_path, &firmware_version,
					&firmware_checksum)) == MEM_CHECK_OK) {
				printf("Firmware found in USB\n\r");
				printf("Firmware version:%lu\n\r", firmware_version);
				printf("Firmware checksum:%lu\n\r", firmware_checksum);
				printf("Firmware path:%s\n\r", firmware_path);

				if ((result_usb = Write_Flash(USB_ID, firmware_path,
						firmware_version, firmware_checksum)) == FLASH_WRITE_OK) {
					printf("Program Flashed from USB\n\r");
					draw_text("Flashed from USB", 0, 10, Tahoma8, 1, 0);
					glcd_refresh();
					HAL_Delay(1000);

				}
			} else {
				printf("USB MEMORY Have problem(%d)\n\r", result_usb);
				draw_text("Failed from USB", 0, 10, Tahoma8, 1, 0);
				glcd_refresh();
				HAL_Delay(1000);

			}

			sharedmem = result_usb;
		}
	} else if (sharedmem == WRITE_FROM_SD || sharedmem == FORCE_WRITE_FROM_SD) {
		glcd_blank();
		draw_text("try SD:", 0, 0, Tahoma8, 1, 0);
		glcd_refresh();
		if ((result_sd = Check_SDCard(firmware_path, &firmware_version,
				&firmware_checksum)) == MEM_CHECK_OK) {
			printf("Firmware found in SD\n\r");
			printf("Firmware version:%lu\n\r", firmware_version);
			printf("Firmware checksum:%lu\n\r", firmware_checksum);
			printf("Firmware path:%s\n\r", firmware_path);

			if ((result_sd = Write_Flash(SD_ID, firmware_path, firmware_version,
					firmware_checksum)) == FLASH_WRITE_OK) {
				printf("Program Flashed from SD\n\r");
				draw_text("Flashed from SD", 0, 10, Tahoma8, 1, 0);
				glcd_refresh();
				HAL_Delay(1000);

			}
		} else {
			printf("SD CARD Have problem(%d)\n\r", result_sd);
			draw_text("Failed from SD", 0, 10, Tahoma8, 1, 0);
			glcd_refresh();
			HAL_Delay(1000);

		}

		sharedmem = result_sd;
	} else if (sharedmem == WRITE_FROM_USB || sharedmem == FORCE_WRITE_FROM_USB) {
		glcd_blank();
		draw_text("try USB", 0, 0, Tahoma8, 1, 0);
		glcd_refresh();
		if ((result_usb = Check_USBMEM(firmware_path, &firmware_version,
				&firmware_checksum)) == MEM_CHECK_OK) {
			printf("Firmware found in USB\n\r");
			printf("Firmware version:%lu\n\r", firmware_version);
			printf("Firmware checksum:%lu\n\r", firmware_checksum);
			printf("Firmware path:%s\n\r", firmware_path);
			if ((result_usb = Write_Flash(USB_ID, firmware_path,
					firmware_version, firmware_checksum)) == FLASH_WRITE_OK) {
				printf("Program Flashed from USB\n\r");
				draw_text("Flashed from USB", 0, 10, Tahoma8, 1, 0);
				glcd_refresh();
				HAL_Delay(1000);

			}
		} else {
			printf("USB MEMORY Have problem(%d)\n\r", result_usb);
			draw_text("Failed from USB", 0, 10, Tahoma8, 1, 0);
			glcd_refresh();
			HAL_Delay(1000);
		}

		sharedmem = result_usb;
	}
	glcd_blank();
	draw_text("goto app", 0, 0, Tahoma8, 1, 0);
	glcd_refresh();
	if (!goto_application()) {
		printf("bootloader could not go to application\n\r");
		draw_text("Failed Application", 0, 10, Tahoma8, 1, 0);
		glcd_refresh();
	}
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */
		MX_USB_HOST_Process();

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI
			| RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	/* USER CODE BEGIN Callback 0 */
	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM1) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */
	/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
