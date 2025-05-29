/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>      // add for printf
#include <string.h>
#include "liquidcrystal_i2c.h"
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
uint8_t uart_buf[1];
uint8_t ticks;  // timer2 ticks
uint8_t SM_State; // State for the State Machine

GPIO_TypeDef *RGB_GPIO_Banks[] = {GPIOB, GPIOB, GPIOA};
uint16_t RGB_GPIO_Pins[] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_8};

uint8_t colors_Mode_A[4][3] = {
  {0, 100, 0},
  {75, 70, 40},
  {30, 60, 90},
  {0, 0, 0}
};

uint8_t colors_Mode_B[4][3] = {
  {100, 0, 0},
  {90, 30, 30},
  {0, 70, 70},
  {100, 100, 100}
};

uint8_t colors_Mode_C[4][3] = {
  {0, 0, 100},
  {80, 50, 0},
  {50, 0, 50},
  {0, 0, 0}
};

uint8_t (*color_PWM_Set[3])[3] = { colors_Mode_A, colors_Mode_B, colors_Mode_C };
uint8_t color_PWM[4][3];



char* morse_Messages[] = {
  "..--- ----. / ... - .- .-. -",      
  ". -.-. .... .. .--. .- / ..--- ----.",              
  ".... . .-.. .-.. --- / ..--- ----.",     
};

char morse_Msg[64] = "..--- ----. / ... - .- .-. -";

uint8_t current_Mode = 0; //0 - A, 1 - B, 2 - C
uint8_t T = 100 ;


uint8_t test_mode = 0; // 0=RGB mode, 1=Cable test
uint8_t last_button_state = 1;
/* USER CODE END PV */

typedef enum {
  CABLE_STRAIGHT,
  CABLE_CROSSOVER,
  CABLE_FAULTY
} CableType;

const char* cable_type_names[] = {
  "Straight",
  "Crossover",
  "Faulty"
};
/* USER CODE END PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

/* also redefine _write for printf*/
int _write(int fd, char * ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t *) ptr, len, HAL_MAX_DELAY);
  return len;
}
uint8_t dot_Color[] = {1, 0, 1};
uint8_t dash_Color[] = {0, 1, 0};
void dot() {
  uint8_t dot_Color[] = {1, 0, 1};

  for(unsigned int k=0; k<3; k++)
    HAL_GPIO_WritePin(RGB_GPIO_Banks[k], RGB_GPIO_Pins[k], dot_Color[k]);
  HAL_Delay(T);

  for(unsigned int k=0; k<3; k++)
    HAL_GPIO_WritePin(RGB_GPIO_Banks[k], RGB_GPIO_Pins[k], 0);
  HAL_Delay(T);
}

void dash() {
  uint8_t dash_Color[] = {0, 1, 0};

  for(unsigned int k=0; k<3; k++)
    HAL_GPIO_WritePin(RGB_GPIO_Banks[k], RGB_GPIO_Pins[k], dash_Color[k]);
  HAL_Delay(3*T);

  for(unsigned int k=0; k<3; k++)
    HAL_GPIO_WritePin(RGB_GPIO_Banks[k], RGB_GPIO_Pins[k], 0);
  HAL_Delay(T);
}

void morse_Send(char* msg) {
  printf("\r\n\r\n[ SENDING MESSAGE ]\r\n");
  for(unsigned int j=0; j<strlen(msg); j++) {
    switch(morse_Msg[j]) {
      case '.':
        dot();
        break;
      case '-':
        dash();
        break;
      default:
        HAL_Delay(3*T);
    }
    printf("%c", morse_Msg[j]);
  }
  printf("\r\n");
}

void morse_Trigger(char c) {
  char s[] = "morse";
  static unsigned int i = 0;

  if(c == 'A' || c == 'B' || c == 'C') {
    current_Mode = c - 'A';  // Set mode 0, 1 or 2
    memcpy(color_PWM, color_PWM_Set[current_Mode], sizeof(color_PWM));
    strcpy(morse_Msg, morse_Messages[current_Mode]);
    printf("Switched to mode %c\r\n", c);
    return;
  }

  if(c == s[i])
    i++;
  else
    i = 0;
  
  if(!s[i]) {
    i = 0;
    morse_Send(morse_Msg);
  }
}


/* Callback function for UART RX Complete: process incoming character */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(uart_buf[0] == 'T') { // Toggle test mode
    test_mode = !test_mode;
    HD44780_Clear();
    HD44780_PrintStr(test_mode ? "Cable Test Mode" : "RGB Mode");
  }
  else
    printf("Code Version %d.%d\r\n",SW_VERSION/10, SW_VERSION%10);
  
  HAL_UART_Receive_IT(&huart1, uart_buf, 1); // reactivate interrupt for next char
}

/* Callback function for TIM2 period elapsed */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim2)
{
  if(ticks++ == 100) {    // 100 timer2 ticks = one hundredth of a second! This is fitting for 100Hz PWM with 1% resolution
    ticks=0;   // reset
    for(unsigned int i=0; i<3; i++)
      if(color_PWM[SM_State][i])
        HAL_GPIO_WritePin(RGB_GPIO_Banks[i], RGB_GPIO_Pins[i], 1);
  }

  for(unsigned int i=0; i<3; i++) {
    if(ticks == color_PWM[SM_State][i])
      HAL_GPIO_WritePin(RGB_GPIO_Banks[i], RGB_GPIO_Pins[i], 0);
  }
}

/* USER CODE BEGIN 0 */

void run_cable_test() {
    uint8_t wire_connections[8][8] = {0}; // Connection matrix
    uint16_t pa_pins[8] = {GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4,
                          GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7, GPIO_PIN_8};
    uint16_t pb_pins[8] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11,
                          GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};
    
    // Test all connections
    for (uint8_t pa_idx = 0; pa_idx < 8; pa_idx++) {
        HAL_GPIO_WritePin(GPIOA, pa_pins[pa_idx], GPIO_PIN_SET);
        HAL_Delay(5);
        for (uint8_t pb_idx = 0; pb_idx < 8; pb_idx++) {
            wire_connections[pa_idx][pb_idx] = 
                HAL_GPIO_ReadPin(GPIOB, pb_pins[pb_idx]) ? 1 : 0;
        }
        HAL_GPIO_WritePin(GPIOA, pa_pins[pa_idx], GPIO_PIN_RESET);
    }
    
    // Count total connections
    uint8_t total_connections = 0;
    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            total_connections += wire_connections[i][j];
        }
    }
    
    HD44780_Clear();
    HD44780_SetCursor(0, 0);
    
    if (total_connections == 0) {
        HD44780_PrintStr("NO CABLE");
        return;
    }
    
    // Check for YOUR crossover pattern: 2<->4 and 7<->8
    if ((wire_connections[1][3] && wire_connections[3][1]) && 
        (wire_connections[6][7] && wire_connections[7][6])) {
        HD44780_PrintStr("CROSSOVER CABLE");
        return;
    }
    
    // Check for straight cable
    uint8_t straight_connections = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (wire_connections[i][i] == 1) {
            straight_connections++;
        }
    }
    
    if (straight_connections >= 6) {
        HD44780_PrintStr("STRAIGHT CABLE");
        return;
    }
    
    HD44780_PrintStr("FAULTY CABLE");
    show_faulty_pins(wire_connections);
}

void show_faulty_pins(uint8_t wire_connections[8][8]) {
    HD44780_SetCursor(0, 1);
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t connected = 0;
        for (uint8_t j = 0; j < 8; j++) {
            connected |= wire_connections[i][j];
        }
        if (!connected) {
            char buf[4];
            sprintf(buf, "%d ", i+1);
            HD44780_PrintStr(buf);
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /****************************************************************************/
  /********************************      MAIN        **************************/
  /****************************************************************************/
  
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
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, uart_buf, 1); // enable UART Rx IT=interrupt
  HAL_TIM_Base_Start_IT(&htim2);  //enable Timer 2 interrupt
  memcpy(color_PWM, color_PWM_Set[current_Mode], sizeof(color_PWM));
  // Initialize LCD (16 columns, 2 rows)
HD44780_Init(2);
HD44780_Backlight();
HD44780_Clear();
HD44780_SetCursor(0, 0);
HD44780_PrintStr("hello world from");
HD44780_SetCursor(0, 1);
HD44780_PrintStr("haidamacu");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  
  setbuf(stdout, NULL);
  printf("Code version %d.%d starting...\r\n", SW_VERSION/10, SW_VERSION%10);
  /*********************************** main loop *************************************************/
  while (1)
  {
    /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
static uint32_t button_press_time = 0;
static uint8_t last_button_state = 1; // Initialize with button released state

// Read current button state properly
uint8_t current_button_state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

// Detect button press (falling edge)
if(last_button_state && !current_button_state) { 
    button_press_time = HAL_GetTick();
} 
// Detect button release (rising edge)
else if(!last_button_state && current_button_state) {
    if(button_press_time > 0) {
        // Long press detection (>1000ms)
        if(HAL_GetTick() - button_press_time > 1000) {
            test_mode = !test_mode;
            HD44780_Clear();
            HD44780_PrintStr(test_mode ? "Cable Test Mode" : "RGB MODE");
        }
        // Short press action
        else {
            if(test_mode) {
                run_cable_test();
            } else {
                SM_State = (SM_State+1)%4;
                printf("R:%d G:%d B:%d\r\n", 
                    color_PWM[SM_State][0],
                    color_PWM[SM_State][1],
                    color_PWM[SM_State][2]);
            }
        }
        button_press_time = 0;
    }
}

// Update last state at the END of the loop iteration
last_button_state = current_button_state;


HAL_Delay(10); // Small delay to reduce CPU usage
/* USER CODE END 3 */

  
}

}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
