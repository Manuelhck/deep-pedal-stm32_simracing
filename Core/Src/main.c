/*
 * Pedalera SimRacing - STM32F103C6
 *
 * Desarrollado por: Monster of cookies y su colega Deepseek IA

 *
 * Historial:
 * - Día 1: USB HID funcionando
 * - Día 2: Solución acoplamiento ADC + calibración
 *
 * GitHub: [tu enlace]
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"
#include "usbd_hid.h"

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;

// Estructura optimizada para 3 pedales
static struct {
    uint16_t min;     // Mínimo calibrado
    uint16_t max;     // Máximo calibrado
    uint16_t raw;     // Valor actual filtrado
    uint8_t  out;     // Salida (0-255)
    uint8_t  calib:1; // 1=calibrado
    uint8_t  inv:1;   // 1=invertido
} pedals[3]; // [0]=Gas, [1]=Freno, [2]=Embrague

static uint8_t estado = 0; // 0=calibrando, 1=funcionando

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static uint16_t leer_adc_canal(uint32_t canal);  // Nueva función

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration--------------------------------------------------------*/
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_USB_DEVICE_Init();

    /* USER CODE BEGIN 2 */
    // Inicializar pedales
    for(int i = 0; i < 3; i++) {
        pedals[i].min = 4095;
        pedals[i].max = 0;
        pedals[i].raw = 2048;
        pedals[i].out = 0;
        pedals[i].calib = 0;
        pedals[i].inv = 1; // Invertidos por configuración hardware
    }

    // Comenzar calibración
    estado = 0;
    uint32_t inicio_calib = HAL_GetTick();

    // LED de inicio
    for(int i = 0; i < 6; i++) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(80);
    }
    /* USER CODE END 2 */

    /* Infinite loop */
    while (1)
    {
        static uint32_t tiempo_usb = 0;
        static uint32_t tiempo_led = 0;

        /* 1. LEER LOS 3 ADC UNO POR UNO CON PAUSA (PA1, PA4, PA5) */
        uint16_t lecturas[3];

        // Lecturas separadas con tiempo para estabilización
        lecturas[0] = leer_adc_canal(ADC_CHANNEL_1);   // PA1 - Gas
        HAL_Delay(2);                                   // Pausa crítica

        lecturas[1] = leer_adc_canal(ADC_CHANNEL_4);   // PA4 - Freno
        HAL_Delay(2);                                   // Pausa crítica

        lecturas[2] = leer_adc_canal(ADC_CHANNEL_5);   // PA5 - Embrague

        // Filtro simple (media móvil de 4 muestras)
        for(int i = 0; i < 3; i++) {
            pedals[i].raw = (pedals[i].raw * 3 + lecturas[i]) >> 2; // 3/4 antiguo + 1/4 nuevo
        }

        /* 2. MODO CALIBRACIÓN (10 segundos) */
        if(estado == 0) {
            // Actualizar mínimos y máximos
            for(int i = 0; i < 3; i++) {
                if(pedals[i].raw < pedals[i].min) pedals[i].min = pedals[i].raw;
                if(pedals[i].raw > pedals[i].max) pedals[i].max = pedals[i].raw;
            }

            // Terminar calibración después de 10 segundos
            if(HAL_GetTick() - inicio_calib > 10000) {
                estado = 1; // Cambiar a modo normal
                for(int i = 0; i < 3; i++) {
                    if(pedals[i].max - pedals[i].min > 100) {
                        pedals[i].calib = 1;
                    }
                }

                // Confirmación LED
                for(int i = 0; i < 6; i++) {
                    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                    HAL_Delay(50);
                }
            }
        }

        /* 3. PROCESAR VALORES CALIBRADOS */
        if(estado == 1) {
            for(int i = 0; i < 3; i++) {
                if(pedals[i].calib) {
                    uint16_t rango = pedals[i].max - pedals[i].min;

                    if(rango > 100) {
                        // Mapear de min-max a 0-255
                        uint16_t mapeado;
                        if(pedals[i].raw <= pedals[i].min) {
                            mapeado = 0;
                        } else if(pedals[i].raw >= pedals[i].max) {
                            mapeado = 255;
                        } else {
                            mapeado = (255 * (pedals[i].raw - pedals[i].min)) / rango;
                        }

                        // Zona muerta (2%) - mejorada
                        if(mapeado < 5) {
                            mapeado = 0;
                        } else if(mapeado > 250) {
                            mapeado = 255;
                        } else {
                            // Remapear para eliminar zona muerta del rango útil
                            mapeado = ((mapeado - 5) * 255) / 245;
                        }

                        // Invertir (ya que inv = 1 por defecto)
                        pedals[i].out = pedals[i].inv ? (255 - (uint8_t)mapeado) : (uint8_t)mapeado;
                    }
                }
            }

            /* 4. ENVIAR POR USB (30Hz - suficiente para pedales) */
            if(HAL_GetTick() - tiempo_usb >= 33) {  // ~30Hz
                tiempo_usb = HAL_GetTick();

                uint8_t reporte[3] = {
                    pedals[0].out, // Gas
                    pedals[1].out, // Freno
                    pedals[2].out  // Embrague
                };

                USBD_HID_SendReport(&hUsbDeviceFS, reporte, 3);
            }
        }

        /* 5. CONTROL DEL LED */
        if(HAL_GetTick() - tiempo_led >= (estado == 0 ? 100 : 1000)) {
            tiempo_led = HAL_GetTick();
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }

        /* 6. ESPERA PARA ~20Hz TOTAL */
        HAL_Delay(15);  // Frecuencia total ~20Hz (suficiente para pedales)
    }
}

/**
  * @brief Función para leer un solo canal ADC con estabilización
  * @param canal: Canal ADC a leer (ADC_CHANNEL_1, ADC_CHANNEL_4, ADC_CHANNEL_5)
  * @retval Valor ADC de 12 bits (0-4095)
  */
static uint16_t leer_adc_canal(uint32_t canal)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint16_t valor = 0;

    // 1. Detener ADC si estaba en ejecución
    HAL_ADC_Stop(&hadc1);

    // 2. Configurar el canal específico
    sConfig.Channel = canal;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5; // Tiempo largo para alta impedancia
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    // 3. Esperar estabilización (crítico con potenciómetros de alta impedancia)
    HAL_Delay(1);  // 1ms para cargar condensador de sample&hold

    // 4. Iniciar conversión
    if(HAL_ADC_Start(&hadc1) == HAL_OK) {
        // Esperar conversión con timeout
        if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            valor = HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
    }

    return valor;
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

/**
  * @brief ADC1 Initialization Function
  */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    // CONFIGURACIÓN CRÍTICA: Modo SINGLE, no SCAN
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;      // ← DESHABILITAR SCAN
    hadc1.Init.ContinuousConvMode = DISABLE;         // ← DESHABILITAR CONTINUO
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;                  // ← SOLO 1 CONVERSIÓN
    HAL_ADC_Init(&hadc1);

    // CALIBRACIÓN OBLIGATORIA para STM32F1
    HAL_ADCEx_Calibration_Start(&hadc1);

    // Configuración por defecto para canales (se sobrescribe en cada lectura)
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5; // ← TIEMPO LARGO
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // LED en PC13
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/**
  * @brief  Error Handler
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(50);
    }
}
