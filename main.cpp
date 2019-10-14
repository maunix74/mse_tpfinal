#include "mbed.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

void Task1 (void* pvParameters)
{
    (void) pvParameters;
    for (;;) {
        led1 = !led1;
        printf("Task1\n");
        vTaskDelay(500);
    }
}

void Task2 (void* pvParameters)
{
    (void) pvParameters;
    for (;;) {
        led2= !led2;
        printf("Task2\n");
        vTaskDelay(1000);
    }
}
void Task3 (void* pvParameters)
{
    (void) pvParameters;                    // Just to stop compiler warnings.
    for (;;) {
        led3= !led3;
        printf("Task3\n");
        vTaskDelay(1500);
    }
}
void Task4 (void* pvParameters)
{
    (void) pvParameters;                    // Just to stop compiler warnings.
    for (;;) {
        led4= !led4;
        printf("Task4\n");
        vTaskDelay(2000);
    }
}

typedef enum Color_t {
RED,
GREEN,
BLUE
} Color_t;

QueueHandle_t colorQueue;

int main (void)
{
    xTaskCreate( Task1, ( const char * ) "Task1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, ( xTaskHandle * ) NULL );
    xTaskCreate( Task2, ( const char * ) "Task2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, ( xTaskHandle * ) NULL );
    xTaskCreate( Task3, ( const char * ) "Task3", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, ( xTaskHandle * ) NULL );
    xTaskCreate( Task4, ( const char * ) "Task4", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, ( xTaskHandle * ) NULL );
    vTaskStartScheduler();
   //should never get here
   printf("ERORR: vTaskStartScheduler returned!");
   for (;;);
}
