#include "mbed.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "C12832.h"
//#include "debug_usb.h"


#define DEBUGENABLE_TASKJOYSTICK
//#define DEBUGENABLE_TASKMENU
//#define DEBUGENABLE_TASKBIPPER


DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);
BusIn joy(p15,p12,p13,p16);
DigitalIn fire(p14);
C12832 lcd(p5,p7,p6,p8,p11);
AnalogIn pot1(p19);
AnalogIn pot2(p20);
static uint16_t task4loop=0;


typedef enum Color_t {
  RED,
  GREEN,
  BLUE
} Color_t;

QueueHandle_t colorQueue;

typedef enum Keys_e {
    KEY_NONE,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_PUSH
};

QueueHandle_t xKeysQueue;

void Task_Joystick (void* pvParameters)
{
    int key_read;
    uint8_t key;
    uint8_t key_ant;
    BaseType_t xStatus;

    (void) pvParameters;
    for (;;) {
        led1 = !led1;
        key_read = joy;
        switch(key_read) {
            case 0: key = KEY_NONE; break;   // key_none
            case 1: key = KEY_DOWN; break;
            case 2: key = KEY_UP; break;
            case 4: key = KEY_LEFT; break;
            case 8: key = KEY_RIGHT; break;
        }
        if (fire) key = KEY_PUSH;
        if (key != key_ant) {
            if (key != KEY_NONE) {
                xStatus = xQueueSendToBack(xKeysQueue, &key, 0);
                if (xStatus != pdPASS) {
    #ifdef DEBUGENABLE_TASKJOYSTICK
                    printf("<JoyQueue. Error agregando elemento a la cola.\r\n",key);
    #endif
                } else {
    #ifdef DEBUGENABLE_TASKJOYSTICK
                    printf("<JoyQueue. Key: %d>\r\n",key);
    #endif
                }
            }

            key_ant = key;
        }
        //printf("Task1\n");
        vTaskDelay(500);
    }
}



void Task_Bipper (void* pvParameters)
{
    (void) pvParameters;
    for (;;) {
        led2= !led2;
        #if defined(DEBUGENABLE_TASKBIPPER)
        printf("TaskBipper2\n");
        #endif
        vTaskDelay(1000);
    }
}


void Task_Menu (void* pvParameters)
{
    (void) pvParameters;                    // Just to stop compiler warnings.
    for (;;) {
        led3= !led3;
        #ifdef DEBUGENABLE_TASKMENU
        printf("<Task Menu>");
        #endif
        vTaskDelay(1500);
    }
}


void Task4 (void* pvParameters)
{
    static float pote1, pote2;

    (void) pvParameters;                    // Just to stop compiler warnings.
    for (;;) {
        led4= !led4;
        //printf("Task4\n");
        
        pote1 = pot1.read();
        pote2 = pot2.read();
        lcd.cls();
        lcd.locate(1,1);
        lcd.printf("Ciclo Task #%d",task4loop);
        task4loop++;
        lcd.locate(1,12);
        lcd.printf("  Pote1=%f",pote1);
        lcd.locate(1,22);
        lcd.printf("  Pote2=%f",pote2);
        lcd.copy_to_lcd();
        
        vTaskDelay(1200);
    }
}

int main (void)
{
    xKeysQueue = xQueueCreate (5,sizeof(uint8_t)) ;
    xTaskCreate( Task_Joystick, ( const char * ) "Task Joystick", 256, NULL, 1, ( xTaskHandle * ) NULL );
    xTaskCreate( Task_Bipper, ( const char * ) "TaskBipper", 128, NULL, 1, ( xTaskHandle * ) NULL );
    xTaskCreate( Task_Menu, ( const char * ) "TaskMenu", 128, NULL, 1, ( xTaskHandle * ) NULL );
    xTaskCreate( Task4, ( const char * ) "Task4", 1024, NULL, 2, ( xTaskHandle * ) NULL );

    vTaskStartScheduler();
    //should never get here
    printf("ERORR: vTaskStartScheduler returned!");
    for (;;);
}
