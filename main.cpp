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
void menu_temperatura_ppal(enum Cmd_e cmd);
void menu_temperatura_seltexto(enum Cmd_e cmd);
void menu_temperatura_selgrafico(enum Cmd_e cmd);
void menu_temperatura_printtexto(enum Cmd_e cmd);
void menu_temperatura_printgrafico(enum Cmd_e cmd);
void menu_acelerometro_ppal(enum Cmd_e cmd);
void menu_acelerometro_seltexto(enum Cmd_e cmd);
void menu_acelerometro_selgrafico(enum Cmd_e cmd);
void menu_acelerometro_printtexto(enum Cmd_e cmd);
void menu_acelerometro_printgrafico(enum Cmd_e cmd);
void menu_rgb_ppal(enum Cmd_e cmd);
void menu_rgb_selr(enum Cmd_e cmd);
void menu_rgb_selg(enum Cmd_e cmd);
void menu_rgb_selb(enum Cmd_e cmd);
void menu_speaker_ppal(enum Cmd_e cmd);
void menu_speaker_selbip1(enum Cmd_e cmd);
void menu_speaker_selbip2(enum Cmd_e cmd);
void menu_speaker_selbip3(enum Cmd_e cmd);
void menu_speaker_selbipnone(enum Cmd_e cmd);
void menu_signals_ppal(enum Cmd_e cmd);
void menu_signals_selsquarewave(enum Cmd_e cmd);
void menu_signals_selsinewave(enum Cmd_e cmd);
void menu_signals_printsquarewave(enum Cmd_e cmd);
void menu_signals_printsinewave(enum Cmd_e cmd);
void menu_rgb_rmenos(enum Cmd_e cmd);
void menu_rgb_rmas(enum Cmd_e cmd);
void menu_rgb_gmenos(enum Cmd_e cmd);
void menu_rgb_gmas(enum Cmd_e cmd);
void menu_rgb_bmenos(enum Cmd_e cmd);
void menu_rgb_bmas(enum Cmd_e cmd);

// <-----  FIN MENUS ------


void Task_Menu (void* pvParameters)
{
    BaseType_t xQueueStatus;
    uint8_t key, key_ant;
    UBaseType_t queueelements;
/*
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
*/
    Tmenuitem menuitems[] = {
{MENU_SIGNALS_PPAL,MENU_ACELEROMETRO_PPAL,MENU_NONE,MENU_NONE,MENU_TEMPERATURA_SELTEXTO,menu_temperatura_ppal,},
{MENU_SIGNALS_PPAL,MENU_ACELEROMETRO_PPAL,MENU_TEMPERATURA_SELGRAFICO,MENU_TEMPERATURA_SELGRAFICO,MENU_TEMPERATURA_PRINTTEXTO,menu_temperatura_seltexto,},
{MENU_SIGNALS_PPAL,MENU_ACELEROMETRO_PPAL,MENU_TEMPERATURA_SELTEXTO,MENU_TEMPERATURA_SELTEXTO,MENU_TEMPERATURA_PRINTGRAFICO,menu_temperatura_selgrafico,},
{MENU_SIGNALS_PPAL,MENU_ACELEROMETRO_PPAL,MENU_NONE,MENU_NONE,MENU_NONE,menu_temperatura_printtexto,},
{MENU_SIGNALS_PPAL,MENU_ACELEROMETRO_PPAL,MENU_NONE,MENU_NONE,MENU_NONE,menu_temperatura_printgrafico,},
{MENU_TEMPERATURA_PPAL,MENU_RGB_PPAL,MENU_NONE,MENU_NONE,MENU_ACELEROMETRO_SELTEXTO,menu_acelerometro_ppal,},
{MENU_TEMPERATURA_PPAL,MENU_RGB_PPAL,MENU_ACELEROMETRO_SELGRAFICO,MENU_ACELEROMETRO_SELGRAFICO,MENU_ACELEROMETRO_PRINTTEXTO,menu_acelerometro_seltexto,},
{MENU_TEMPERATURA_PPAL,MENU_RGB_PPAL,MENU_ACELEROMETRO_SELTEXTO,MENU_ACELEROMETRO_SELTEXTO,MENU_ACELEROMETRO_PRINTGRAFICO,menu_acelerometro_selgrafico,},
{MENU_TEMPERATURA_PPAL,MENU_RGB_PPAL,MENU_NONE,MENU_NONE,MENU_NONE,menu_acelerometro_printtexto,},
{MENU_TEMPERATURA_PPAL,MENU_RGB_PPAL,MENU_NONE,MENU_NONE,MENU_NONE,menu_acelerometro_printgrafico,},
{MENU_ACELEROMETRO_PPAL,MENU_SPEAKER_PPAL,MENU_NONE,MENU_NONE,MENU_RGB_SELR,menu_rgb_ppal,},
{MENU_RGB_RMENOS,MENU_RGB_RMAS,MENU_NONE,MENU_NONE,MENU_RGB_SELG,menu_rgb_selr,},
{MENU_RGB_GMENOS,MENU_RGB_GMAS,MENU_NONE,MENU_NONE,MENU_RGB_SELB,menu_rgb_selg,},
{MENU_RGB_BMENOS,MENU_RGB_BMAS,MENU_NONE,MENU_NONE,MENU_SPEAKER_PPAL,menu_rgb_selb,},
{MENU_RGB_PPAL,MENU_SIGNALS_PPAL,MENU_NONE,MENU_NONE,MENU_SPEAKER_SELBIP1,menu_speaker_ppal,},
{MENU_RGB_PPAL,MENU_SIGNALS_PPAL,MENU_SPEAKER_SELBIPNONE,MENU_SPEAKER_SELBIP2,MENU_SIGNALS_PPAL,menu_speaker_selbip1,},
{MENU_RGB_PPAL,MENU_SIGNALS_PPAL,MENU_SPEAKER_SELBIP1,MENU_SPEAKER_SELBIP3,MENU_SIGNALS_PPAL,menu_speaker_selbip2,},
{MENU_RGB_PPAL,MENU_SIGNALS_PPAL,MENU_SPEAKER_SELBIP2,MENU_SPEAKER_SELBIPNONE,MENU_SIGNALS_PPAL,menu_speaker_selbip3,},
{MENU_RGB_PPAL,MENU_SIGNALS_PPAL,MENU_SPEAKER_SELBIP3,MENU_SPEAKER_SELBIP1,MENU_SIGNALS_PPAL,menu_speaker_selbipnone,},
{MENU_SPEAKER_PPAL,MENU_TEMPERATURA_PPAL,MENU_NONE,MENU_NONE,MENU_SIGNALS_SELSQUAREWAVE,menu_signals_ppal,},
{MENU_SPEAKER_PPAL,MENU_TEMPERATURA_PPAL,MENU_SIGNALS_SELSINEWAVE,MENU_SIGNALS_SELSINEWAVE,MENU_SIGNALS_PRINTSQUAREWAVE,menu_signals_selsquarewave,},
{MENU_SPEAKER_PPAL,MENU_TEMPERATURA_PPAL,MENU_SIGNALS_SELSQUAREWAVE,MENU_SIGNALS_SELSQUAREWAVE,MENU_SIGNALS_PRINTSINEWAVE,menu_signals_selsinewave,},
{MENU_SPEAKER_PPAL,MENU_TEMPERATURA_PPAL,MENU_NONE,MENU_NONE,MENU_NONE,menu_signals_printsquarewave,},
{MENU_SPEAKER_PPAL,MENU_TEMPERATURA_PPAL,MENU_NONE,MENU_NONE,MENU_NONE,menu_signals_printsinewave,},
{MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_rmenos,},
{MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_rmas,},
{MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_gmenos,},
{MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_gmas,},
{MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_bmenos,},
{MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_bmas,}
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
