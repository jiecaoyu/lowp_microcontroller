#pragma once
#include "mbed.h"

int change_frequency(){
    HAL_RCC_DeInit();
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSEState            = RCC_HSE_OFF;
    RCC_OscInitStruct.HSICalibrationValue = 16; 
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 8;             // VCO input clock = 2 MHz (16 MHz / 8)
    RCC_OscInitStruct.PLL.PLLN            = 96;           // VCO output clock = 384 MHz (2 MHz * 192)  
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV8; // PLLCLK = 96 MHz (384 MHz / 4)
    RCC_OscInitStruct.PLL.PLLQ            = 8;             // USB clock = 48 MHz (384 MHz / 8) --> Good for USB

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {   
        return 0; // FAIL
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */    
    RCC_ClkInitStruct.ClockType      = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK; // 96 MHz
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;         // 96 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;           // 48 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;           // 96 MHz
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {   
        return 0; // FAIL
    }

    // JC: configure RCC_OscInitStruct.PLL.PLLN (192 ==> 96)
    // RCC_OscInitStruct.PLL.PLLP (RCC_PLLP_DIV4 ==> RCC_PLLP_DIV8)
    // FLASH_LATENCY_3 ==> FLASH_LATENCY_1


    SystemCoreClockUpdate();
    printf("\n\rCurrent clock: %lu\n\r", SystemCoreClock);
    return 0;
}

