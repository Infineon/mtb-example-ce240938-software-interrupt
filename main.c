/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the Software interrupt
*              example for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/******************************************************************************
* Include header files
*******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "cy_sysint.h"
#include "cy_tcpwm_counter.h"
#include "mtb_hal.h"

/******************************************************************************
* Macros
*******************************************************************************/
/* Interrupt source to be triggered by software */
#define TCPWM_INTR_SOURCE  CY_TCPWM_INT_ON_TC

/******************************************************************************
* Global Variables
*******************************************************************************/
/* Peripheral interrupt configuration structure */
const cy_stc_sysint_t IRQ_CFG_PERIPHERAL =
{
    .intrSrc  = ((NvicMux5_IRQn << 16) | tcpwm_0_interrupts_1_IRQn),
    .intrPriority = 3UL
};

/* For the Retarget -IO (Debug UART) usage */
static cy_stc_scb_uart_context_t    UART_context;           /** UART context */
static mtb_hal_uart_t               UART_hal_obj;           /** Debug UART HAL object  */

/******************************************************************************
 * Function Name: handle_Counter_Interrupt_0
 * Summary:
 *  This is the interrupt handler for a counter generated interrupt processing.
 * Parameters:
 *  none
 * Return:
 *  void
 ******************************************************************************
 */
void handle_Counter_Interrupt_0(void)
{
    /* Clear TCPWM TC interrupt flag */
    Cy_TCPWM_ClearInterrupt(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM, TCPWM_INTR_SOURCE);
     
    Cy_GPIO_Inv(CYBSP_LED2_PORT, CYBSP_LED2_PIN);
}

/******************************************************************************
 * Function Name: init_Timer_Interrupt
 * Summary:
 *  This is the initializing function for timer setup.
 * Parameters:
 *  none
 * Return:
 *  void
 ******************************************************************************
 */
static void init_Timer_Interrupt(void)
{
    /* Initialize TCPWM0_GRP0_COUNTER 1 as Counter */
    if (Cy_TCPWM_Counter_Init(TCPWM_COUNTER_HW,TCPWM_COUNTER_NUM, &TCPWM_COUNTER_config) != CY_TCPWM_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable TCPWM0_GRP0_COUNTER 1 */
    Cy_TCPWM_Counter_Enable(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM);

    /* Enable desired interrupt for the counter */
    Cy_TCPWM_SetInterruptMask(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM, TCPWM_INTR_SOURCE);

    /*  Setting up the peripheral interrupt */
    if (Cy_SysInt_Init(&IRQ_CFG_PERIPHERAL, handle_Counter_Interrupt_0) != CY_SYSINT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable the peripheral interrupt mapped to NVICMux5 */
    NVIC_EnableIRQ(NvicMux5_IRQn);
}

/******************************************************************************
 * Function Name: main
 * Summary:
 *  This is the main function.
 * Parameters:
 *  none
 * Return:
 *  int
 ******************************************************************************
 */
int main(void)
{
    cy_rslt_t result;

    /* Variable for storing character read from terminal */
    uint8_t uartReadValue;

    /* Initialize the device and board peripherals */
    if (cybsp_init() != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Debug UART init */
    result = (cy_rslt_t)Cy_SCB_UART_Init(UART_HW, &UART_config, &UART_context);

    /* UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_SCB_UART_Enable(UART_HW);

    /* Setup the HAL UART */
    result = mtb_hal_uart_setup(&UART_hal_obj, &UART_hal_config, &UART_context, NULL);

    /* HAL UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    result = cy_retarget_io_init(&UART_hal_obj);

    /* HAL retarget_io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_GPIO_Pin_Init(CYBSP_LED2_PORT, CYBSP_LED2_PIN, &CYBSP_LED2_config);

    /* Initialize TCPWM based software interrupts */
    init_Timer_Interrupt();

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("****************** "
           "Software Interrupt using Peripheral registers example "
           "****************** \r\n\n");

    printf("Press 'p' for interrupts using peripheral registers\r\n\n");

    for (;;)
    {
        /* Check if 'p' key was pressed */
        uartReadValue = Cy_SCB_UART_Get(UART_HW);
        
        if(uartReadValue != 0xff)
        
        {
            if (uartReadValue == 'p')
            {
                /* Set Interrupt for configured timer resource by software */
                Cy_TCPWM_SetInterrupt(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM, TCPWM_INTR_SOURCE);
            }
        }
    }
}
/* [] END OF FILE */
