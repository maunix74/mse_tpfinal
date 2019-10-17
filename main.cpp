#include "mbed.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "C12832.h"
//#include "debug_usb.h"


#define DEBUGENABLE_TASKJOYSTICK
//#define DEBUGENABLE_TASKMENU
#define DEBUGENABLE_TASKBEEPER


DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);
BusIn joy(p15,p12,p13,p16);
DigitalIn fire(p14);
C12832 lcd(p5,p7,p6,p8,p11);
AnalogIn pot1(p19);
AnalogIn pot2(p20);
PwmOut spkr(p26);

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

typedef enum Beeps {
    KEY_BIPNONE,
    KEY_BIP1,
    KEY_BIP2,
    KEY_BIP3
};

QueueHandle_t xKeysQueue;
QueueHandle_t xBeeperQueue;

typedef enum MenuList {
    MENU_TEMPERATURA_PPAL,
    MENU_TEMPERATURA_SELTEXTO,
    MENU_TEMPERATURA_SELGRAFICO,
    MENU_TEMPERATURA_PRINTTEXTO,
    MENU_TEMPERATURA_PRINTGRAFICO,
    MENU_ACELEROMETRO_PPAL,
    MENU_ACELEROMETRO_SELTEXTO,
    MENU_ACELEROMETRO_SELGRAFICO,
    MENU_ACELEROMETRO_PRINTTEXTO,
    MENU_ACELEROMETRO_PRINTGRAFICO,
    MENU_RGB_PPAL,
    MENU_RGB_SELR,
    MENU_RGB_SELG,
    MENU_RGB_SELB,
    MENU_SPEAKER_PPAL,
    MENU_SPEAKER_SELBIP1,
    MENU_SPEAKER_SELBIP2,
    MENU_SPEAKER_SELBIP3,
    MENU_SPEAKER_SELBIPNONE,
    MENU_SIGNALS_PPAL,
    MENU_SIGNALS_SELSQUAREWAVE,
    MENU_SIGNALS_SELSINEWAVE,
    MENU_SIGNALS_PRINTSQUAREWAVE,
    MENU_SIGNALS_PRINTSINEWAVE,
    MENU_RGB_RMENOS,
    MENU_RGB_RMAS,
    MENU_RGB_GMENOS,
    MENU_RGB_GMAS,
    MENU_RGB_BMENOS,
    MENU_RGB_BMAS,
    MENU_NONE = 0xFF

};

enum Cmd_e {
    CMD_INITIALIZE,    // Indica que la funcion debe inicializar el menu
    CMD_NONE           // indica que la funcion esta ejecutandose en ciclo normal
};

typedef struct  {
  uint8_t keyup;
  uint8_t keydown;
  uint8_t keyleft;
  uint8_t keyright;
  uint8_t keyfire;
  void (*func_ptr)(enum Cmd_e);
} Tmenuitem;



// MENUS
void fcn_temperaturamenu1_0(enum Cmd_e cmd);
void fcn_temperaturamenu1_1(enum Cmd_e cmd);
void fcn_temperaturamenu2_0(enum Cmd_e cmd);
void fcn_temperaturamenu2_1(enum Cmd_e cmd);
void fcn_temperaturamenu3_0(enum Cmd_e cmd);
void fcn_temperaturamenu3_1(enum Cmd_e cmd);
void fcn_temperaturamenu4_0(enum Cmd_e cmd);
void fcn_temperaturamenu4_1(enum Cmd_e cmd);

// <-----  FIN MENUS ------


void Task_Menu (void* pvParameters)
{
    BaseType_t xQueueStatus;
    uint8_t key, key_ant;
    UBaseType_t queueelements;

    Tmenuitem menuitems[] = {
        {1, 1, 2, 3, 5, fcn_temperaturamenu1_0},
        {1, 1, 2, 3, 5, fcn_temperaturamenu1_1},
        {1, 1, 2, 3, 5, fcn_temperaturamenu2_0},
        {1, 1, 2, 3, 5, fcn_temperaturamenu2_1},
        {1, 1, 2, 3, 5, fcn_temperaturamenu3_0},
        {1, 1, 2, 3, 5, fcn_temperaturamenu3_1},
        {1, 1, 2, 3, 5, fcn_temperaturamenu4_0},
        {1, 1, 2, 3, 5, fcn_temperaturamenu4_1}
            
    };
            
    (void) pvParameters;                    // Just to stop compiler warnings.


    for (;;) {
        led3= !led3;
        #ifdef DEBUGENABLE_TASKMENU
        printf("<Task Menu: Queue elements=");
        #endif
        queueelements = uxQueueMessagesWaiting(xKeysQueue);
        #ifdef DEBUGENABLE_TASKMENU
        printf("%d",queueelements);
        #endif
        key = KEY_NONE;
        if (0 < queueelements) {
            // La cola tiene mensaje(s)
            xQueueStatus = xQueueReceive(xKeysQueue, &key, 5);
            if (xQueueStatus == pdPASS) {
                #ifdef DEBUGENABLE_TASKMENU
                printf(" <KeyRead= %d>",key);
                #endif
            } else {
                #ifdef DEBUGENABLE_TASKMENU
                printf(" QueKey Read Error");
                #endif
            }
        }
        #ifdef DEBUGENABLE_TASKMENU
        printf("\r\n");
        #endif

        if (key != key_ant) {
            // evaluar nueva opcion de menu

        }

        vTaskDelay(2500);
    }
}



void Task_Joystick (void* pvParameters)
{
    int key_read;
    uint8_t key;
    uint8_t key_ant;
    BaseType_t xStatus;
    uint8_t bip;

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
                bip++;
                if (bip>3) bip=1;
                xStatus = xQueueSendToBack(xBeeperQueue, &bip, 0);
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
        vTaskDelay(250);
    }
}



void Task_Beeper (void* pvParameters)
{
    BaseType_t xQueueStatus;
    UBaseType_t queueelements;
    uint8_t beeper; 
/*    struct {

    } */

    (void) pvParameters;
    for (;;) {
        led2= !led2;
        queueelements = uxQueueMessagesWaiting(xBeeperQueue);
        //#if defined(DEBUGENABLE_TASKBEEPER)
        //printf("Beeper qe: %d\r\n",queueelements);
        //#endif
        if (0 < queueelements) {
            #if defined(DEBUGENABLE_TASKBEEPER)
            printf("%d",queueelements);
            #endif
            xQueueStatus = xQueueReceive(xBeeperQueue, &beeper, 5);
            if (xQueueStatus == pdPASS) {
            switch(beeper) {
                case KEY_BIP1: { spkr.period(1.0/2000); spkr=0.5;  break;}
                case KEY_BIP2: { spkr.period(2.0/2000); spkr=0.5;  break;}
                case KEY_BIP3: { spkr.period(4.0/2000); spkr=0.5;  break;}
            }
            }

        }

        vTaskDelay(100);
        spkr=0.0;
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
    xBeeperQueue = xQueueCreate (1,sizeof(uint8_t)) ;

    xTaskCreate( Task_Joystick, ( const char * ) "Task Joystick", 256, NULL, 1, ( xTaskHandle * ) NULL );
    xTaskCreate( Task_Beeper, ( const char * ) "TaskBepper", 256, NULL, 1, ( xTaskHandle * ) NULL );
    xTaskCreate( Task_Menu, ( const char * ) "TaskMenu", 128, NULL, 1, ( xTaskHandle * ) NULL );
    xTaskCreate( Task4, ( const char * ) "Task4", 1024, NULL, 2, ( xTaskHandle * ) NULL );
    printf("\r\nTrabajo Final Arquitecturas Embebidas y Procesamiento en Tiempo Real\r\n");

    vTaskStartScheduler();
    //should never get here
    printf("ERORR: vTaskStartScheduler returned!");
    for (;;);
}
