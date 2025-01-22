/*
 * The Clear BSD License
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "lcd.h"

#include "pin_mux.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
int light = 0;
unsigned int state = 2;
float stateMachine[4] = {0, 0.5, 1, 2};


void delay(void)
{
  volatile int i;

  for (i = 0; i < 100000; i++);
}

void irclk_ini()
{
  MCG->C1 = MCG_C1_IRCLKEN(1) | MCG_C1_IREFSTEN(1);
  MCG->C2 = MCG_C2_IRCS(0); //0 32KHZ internal reference clock; 1= 4MHz irc
}

void init_buttons(){
    SIM->SCGC5 |= (1 << 11);
    
    PORTC->PCR[3] = (1 << 8) //configura el pin como GPIO
                  | (1 << 1) //habilita pull-up
                  | (1 << 0); 
    
    PORTC->PCR[12] = (1 << 8) //configura el pin comoGPIO
                   | (1 << 1) //habilita pull-up
                   | (1 << 0);
                   
    GPIOC->PDDR &= ~(1 << 3);
    GPIOC->PDDR &= ~(1 << 12); //configura ambos como puertos de entrada
    
    NVIC_EnableIRQ(PORTC_PORTD_IRQn); //interrupciones del puerto C
    
    PORTC->PCR[3] |= PORT_PCR_IRQC(0xA);
    PORTC->PCR[12] |= PORT_PCR_IRQC(0xA);
}

void led_init()
{
    SIM->SCGC5 |= (1 << 12);      // Habilitar reloxo para o porto D
    SIM->SCGC5 |= (1 << 13);      // Habilitar reloxo para o porto E
    
    PORTD->PCR[5] = 1 << 8;       // Configurar PTD5 como GPIO
    PORTE->PCR[29] = 1 << 8;      // Configurar PTE29 como GPIO
    
    GPIOD->PDDR |= (1 << 5);      // Configurar PTD5 como saída
    GPIOE->PDDR |= (1 << 29);     // Configurar PTE29 como saída
    
    GPIOD->PSOR |= (1 << 5);      // Apagar o LED (pón o PTD5 en alto)
    GPIOE->PSOR |= (1 << 29);     // Apagar o LED (pón o PTE29 en alto)
}

void LPTMR_Init() {
    SIM->SCGC5 |= SIM_SCGC5_LPTMR_MASK;  // Habilitar reloj para LPTMR

    LPTMR0->CSR = 0;  // Desactivar temporizador antes de configurarlo
    
    LPTMR0->PSR = LPTMR_PSR_PCS(1) | LPTMR_PSR_PBYP_MASK;  // Usar reloj LPO (1 kHz), sin divisor
    LPTMR0->CMR = 1000;  // 1000 ticks = 1 segundo

    NVIC_EnableIRQ(LPTMR0_IRQn);  // Activar interrupción para LPTMR
    NVIC_SetPriority(LPTMR0_IRQn, 2);  // Prioridad baja
    
    LPTMR0->CSR = LPTMR_CSR_TIE_MASK | LPTMR_CSR_TEN_MASK;  // Activar con interrupción
}

void config_clock(){
  LPTMR0->CSR = 0;
  if(state == 0){
    LPTMR0->CMR = 0;
  }else{
    LPTMR0->CMR = 1000/stateMachine[state];
  }
  LPTMR0->CSR = LPTMR_CSR_TIE_MASK | LPTMR_CSR_TEN_MASK;  // Activar con interrupción
}

//para las interrupciones
void PORTC_PORTD_IRQHandler(void){
    if(PORTC->ISFR & (1<<12)){ //si botón izquierdo abrimos puerta 2
      state = (state+1)%4;
      
    }else if(PORTC->ISFR & (1<<3)){ //si botón derecho abrimos puerta 2
      
      if(state==0){
        state = 3;
      }else{
        state = (state-1);
      }
    }

    if(state == 0){
      GPIOD->PSOR |= (1 << 5);
    }
    
    config_clock();
    lcd_display_dec(stateMachine[state]*10);

    //limpiamos los flag para que el interrupt deje de producirse
    PORTC->ISFR |= (1 << 12);
    PORTC->ISFR |= (1 << 3);
}

void LPTMR0_IRQHandler(void) {
    if (LPTMR0->CSR & LPTMR_CSR_TCF_MASK) {

      if(light){
         GPIOD->PSOR |= (1 << 5);      // Apagar LED verde
      }else{
        GPIOD->PCOR |= (1 << 5);      // Encender LED verde
      }

      light = (light+1)%2;
      LPTMR0->CSR |= LPTMR_CSR_TCF_MASK;
    }
}

int main(void)
{

  /* Init board hardware. */
  BOARD_InitPins();
  BOARD_BootClockRUN();
  BOARD_InitDebugConsole();

  irclk_ini();
  lcd_ini();
  LPTMR_Init();

  init_buttons();
  led_init();
  
  SIM->COPC = 0;               // Desactivar Watchdog Timer

  PRINTF("Plantilla exame Sistemas Embebidos: 1a oportunidade 24/25 Q1\r\n");
  lcd_display_dec(stateMachine[state]*10);

  while (1)
    {      
      lcd_display_dec(stateMachine[state]*10);
      delay();
    }
}
