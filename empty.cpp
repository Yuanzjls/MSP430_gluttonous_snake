/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== empty.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Clock.h>
/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
// #include <ti/drivers/I2C.h>
// #include <ti/drivers/SDSPI.h>
// #include <ti/drivers/SPI.h>
// #include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>
// #include <ti/drivers/WiFi.h>

/* Board Header file */
#include "Board.h"
/* Hardware Header file */
#include "LCD5110.h"
#include "Key.h"
#include "snake.h"
#include "stdlib.h"
#include "Temp.h"

#define TASKSTACKSIZE     128
#define LCD_TASKSTACKSIZE 256
// Desired MCLK frequency


Task_Struct task0Struct;
Task_Struct task1Struct;
Task_Struct task2Struct;

Char task0Stack[TASKSTACKSIZE];
Char task1Stack[TASKSTACKSIZE];
Char task2Stack[LCD_TASKSTACKSIZE];
/*
 *  ======== heartBeatFxn ========
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the heartBeat Task instance.
 */
Void heartBeatFxn(UArg arg0, UArg arg1)
{
    unsigned i=0;
    while (1) {
        Task_sleep((unsigned int)arg0);
        GPIO_toggle(Board_LED0);
        if (i==2)
        {
          ADC_Temp_Display();
          i=0;
        }
        else
        {
          i++;
        }
    }
}
Snake_Class Snake;
extern Key_Class Key;
Void Key_Scan(UArg arg0, UArg arg1)
{
  Key_Class Key;
  unsigned char Key_flag;
  
  while (1) {
      Task_sleep((unsigned int)arg0);
      GPIO_toggle(Board_LED1);
      Key_flag = Key.Key_Scan2();
      if (Key_flag)
      {      
        Snake.Direction_ChangeSignal = Key_flag;
      }
  }
}

Void LCD_Update(UArg arg0, UArg arg1)
{
  unsigned char signal = 0;
  
  LCD5110_str(0,3, (unsigned char *)"Welcome to Snake Game");
  while(!Snake.Direction_ChangeSignal)
  {
    signal++;
    Task_sleep((unsigned int)arg0);
  }
  Snake.Direction_ChangeSignal = 0;
  Snake.SetSrand(signal);
  while(1)
  {
    Snake.Snake_Transform();
    Task_sleep((unsigned int)arg0);
    if (Snake.Direction_ChangeSignal)
    {
      if (!Snake.Snake_ChangeDirection(Snake.Direction_ChangeSignal))
      {
        Task_sleep((unsigned int)200);
      }
    }
    signal = Snake.Snake_Update();
    if (signal != Success)
    {
      if (signal == Game_Over)
      {
        LCD5110_clear();
        LCD5110_str(0, 3, (unsigned char*)"Game Over");
        return;
      }
    }
  }
}

/*
 *  ======== main ========
 */
int main(void)
{
    Task_Params taskParams, Key_TaskParams, LCD_TaskParams;

    /* Call board init functions */
    
    LCD5110_init(); 
    ADC_Temp_Init();
    
    Board_initGeneral();
    Board_initGPIO();
 
    /* Construct heartBeat Task  thread */
    Task_Params_init(&taskParams);
    taskParams.arg0 = 500;
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr)heartBeatFxn, &taskParams, NULL);
    
    Task_Params_init(&Key_TaskParams);
    Key_TaskParams.arg0 = 130;
    Key_TaskParams.stackSize = TASKSTACKSIZE;
    Key_TaskParams.stack = &task1Stack;
    Task_construct(&task1Struct, (Task_FuncPtr)Key_Scan, &Key_TaskParams, NULL);

    Task_Params_init(&LCD_TaskParams);
    LCD_TaskParams.arg0 = 30;
    LCD_TaskParams.stackSize = LCD_TASKSTACKSIZE;
    LCD_TaskParams.stack = &task2Stack;
    Task_construct(&task2Struct, (Task_FuncPtr)LCD_Update, &LCD_TaskParams, NULL);  
    
    System_printf("Starting the example\nSystem provider is set to SysMin. "
                  "Halt the target to view any SysMin contents in ROV.\n");
    /* SysMin will only print to the console when you call flush or exit */
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
