#include "mbed.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "C12832.h"
#include "LM75B.h"
#include "MMA7660.h"

//#include "debug_usb.h"


#define DISABLE_USART
//#define DEBUGENABLE_TASKJOYSTICK
#define DEBUGENABLE_TASKMENU
//#define DEBUGENABLE_TASKMENUSM
//#define DEBUGENABLE_TASKBEEPER
//#define DEBUGENABLE_TASKSCALE
//#define DEBUGENABLE_LOGTEMPERATUREPRINT
//#define DEBUGENABLE_LOGTEMPQUEUEERRORS
//#define DEBUGENABLE_LOGTEMPQUEUELOGS
#define DEBUGENABLE_TASKSTARTED


// creo que hay bugs en las librerias al acceder a perifericos se cuelgan y no resumen las tareas
//#define DISABLE_LOGICARTOSACELEROMETRO
//#define DISABLE_LOGICARTOSTEMPERATURA
//#define DISABLE_LOGICARTOSSCALE


DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);
BusIn joy(p15,p12,p13,p16);
DigitalIn fire(p14);
C12832 lcd(p5,p7,p6,p8,p11);
LM75B sensortemperatura(p28,p27);
MMA7660 mma(p28,p27);
AnalogIn pot1(p19);
AnalogIn pot2(p20);
PwmOut spkr(p26);
PwmOut r(p23);
PwmOut g(p24);
PwmOut b(p25);

#define CTE_RGBINCREMENT 0.1
#define CTE_HISTORYSIZE  120
#define CTE_XSTEPMIN 5.0
#define CTE_XSTEPMAX 30.0
#define CTE_YSCALEMIN 0.2
#define CTE_YSCALEMAX 1.0
#define CTE_CANTBIPSENMENUSLECCION  3

#define CTE_XSTEPSQUAREMIN 5.0
#define CTE_XSTEPSQUAREMAX 20.0

#define CTE_TEMPERATURAMAXIMA  33
#define CTE_TEMPERATURAMINIMA  24 
#define CTE_ALTODISPLAY        32


typedef enum  {
    KEY_NONE,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_PUSH
} Keys_e;

typedef enum  {
    KEY_BIPNONE,
    KEY_BIP1,
    KEY_BIP2,
    KEY_BIP3
} Beeps;

typedef enum  {
    MENU_TEMPERATURA_PPAL,
    MENU_TEMPERATURA_SELTEXTO,
    MENU_TEMPERATURA_SELGRAFICO,
    MENU_TEMPERATURA_PRINTTEXTO,
    MENU_TEMPERATURA_PRINTGRAFICO,
    MENU_TEMPERATURA_EXIT,
    MENU_ACELEROMETRO_PPAL,
    MENU_ACELEROMETRO_SELTEXTO,
    MENU_ACELEROMETRO_SELGRAFICO,
    MENU_ACELEROMETRO_PRINTTEXTO,
    MENU_ACELEROMETRO_PRINTGRAFICO,
    MENU_ACELEROMETRO_EXIT,
    MENU_RGB_PPAL,
    MENU_RGB_SELR,
    MENU_RGB_SELG,
    MENU_RGB_SELB,
    MENU_RGB_EXIT,
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
    MENU_SIGNALS_EXIT,
    MENU_RGB_RMENOS,
    MENU_RGB_RMAS,
    MENU_RGB_GMENOS,
    MENU_RGB_GMAS,
    MENU_RGB_BMENOS,
    MENU_RGB_BMAS,
    MENU_NONE = 0xFF

} MenuList_e;

enum Cmd_e {
    CMD_INITIALIZE,    // Indica que la funcion debe inicializar el menu
    CMD_NONE           // indica que la funcion esta ejecutandose en ciclo normal
};

enum MenuSm_e {
    MENUSM_INITIALIZE,
    MENUSM_LOOP
};


typedef struct Tmenuitem {
  uint8_t keyleft;
  uint8_t keyright;
  uint8_t keyup;
  uint8_t keydown;
  uint8_t keyfire;
  void (*func_ptr)(enum Cmd_e);
};


typedef struct {
    enum MenuSm_e sm;
    uint8_t idx;
    uint8_t idx_ant; // para saber si debo inicializar o no o si estoy en el mismo menu
    uint8_t idx_tmp;  // variable temporal para alojar la posible seleccion del menu al pulsar una tecla
    Beeps bip;
    float r;
    float r_ant;  // Usado para mejorar el refresco de pantalla y actualizar solo si es necesario
    float g;
    float g_ant;  // Usado para mejorar el refresco de pantalla y actualizar solo si es necesario
    float b;
    float b_ant;  // Usado para mejorar el refresco de pantalla y actualizar solo si es necesario
    uint8_t updatestatus;  // indica el estado del update de pantalla para saber si debo reimprimir el menu o no

    struct {
       float x_scale;
       float y_scale;
       int8_t  historia[CTE_HISTORYSIZE];
       uint8_t historysize;
       uint8_t idx_puntoactual;
       float x_pos;
       float y_pos;
    } grafico;
} Tmenu;

/*
struct {
    int x_loop;
    int y_result; 
    float y_temp;
} grafico;
*/

typedef struct Tscale {
    float x_scale;
    float y_scale;
};

Tmenu menu;



// MENUS
void menu_temperatura_ppal(enum Cmd_e cmd);
void menu_temperatura_seltexto(enum Cmd_e cmd);
void menu_temperatura_selgrafico(enum Cmd_e cmd);
void menu_temperatura_printtexto(enum Cmd_e cmd);
void menu_temperatura_printgrafico(enum Cmd_e cmd);
void menu_temperatura_exit(enum Cmd_e cmd);
void menu_acelerometro_ppal(enum Cmd_e cmd);
void menu_acelerometro_seltexto(enum Cmd_e cmd);
void menu_acelerometro_selgrafico(enum Cmd_e cmd);
void menu_acelerometro_printtexto(enum Cmd_e cmd);
void menu_acelerometro_printgrafico(enum Cmd_e cmd);
void menu_acelerometro_exit(enum Cmd_e cmd);
void menu_rgb_ppal(enum Cmd_e cmd);
void menu_rgb_selr(enum Cmd_e cmd);
void menu_rgb_selg(enum Cmd_e cmd);
void menu_rgb_selb(enum Cmd_e cmd);
void menu_rgb_exit(enum Cmd_e cmd);
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
void menu_signals_exit(enum Cmd_e cmd);
void menu_rgb_rmenos(enum Cmd_e cmd);
void menu_rgb_rmas(enum Cmd_e cmd);
void menu_rgb_gmenos(enum Cmd_e cmd);
void menu_rgb_gmas(enum Cmd_e cmd);
void menu_rgb_bmenos(enum Cmd_e cmd);
void menu_rgb_bmas(enum Cmd_e cmd);

Tmenuitem menuitems[] = {
    {MENU_SIGNALS_PPAL,MENU_ACELEROMETRO_PPAL,MENU_NONE,MENU_NONE,MENU_TEMPERATURA_SELTEXTO,menu_temperatura_ppal},
    {MENU_SIGNALS_PPAL,MENU_ACELEROMETRO_PPAL,MENU_TEMPERATURA_SELGRAFICO,MENU_TEMPERATURA_SELGRAFICO,MENU_TEMPERATURA_PRINTTEXTO,menu_temperatura_seltexto},
    {MENU_SIGNALS_PPAL,MENU_ACELEROMETRO_PPAL,MENU_TEMPERATURA_SELTEXTO,MENU_TEMPERATURA_SELTEXTO,MENU_TEMPERATURA_PRINTGRAFICO,menu_temperatura_selgrafico},
    {MENU_TEMPERATURA_EXIT,MENU_TEMPERATURA_EXIT,MENU_NONE,MENU_NONE,MENU_NONE,menu_temperatura_printtexto},
    {MENU_TEMPERATURA_EXIT,MENU_TEMPERATURA_EXIT,MENU_NONE,MENU_NONE,MENU_NONE,menu_temperatura_printgrafico},
    {MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,menu_temperatura_exit},
    {MENU_TEMPERATURA_PPAL,MENU_RGB_PPAL,MENU_NONE,MENU_NONE,MENU_ACELEROMETRO_SELTEXTO,menu_acelerometro_ppal},
    {MENU_TEMPERATURA_PPAL,MENU_RGB_PPAL,MENU_ACELEROMETRO_SELGRAFICO,MENU_ACELEROMETRO_SELGRAFICO,MENU_ACELEROMETRO_PRINTTEXTO,menu_acelerometro_seltexto},
    {MENU_TEMPERATURA_PPAL,MENU_RGB_PPAL,MENU_ACELEROMETRO_SELTEXTO,MENU_ACELEROMETRO_SELTEXTO,MENU_ACELEROMETRO_PRINTGRAFICO,menu_acelerometro_selgrafico},
    {MENU_TEMPERATURA_PPAL,MENU_RGB_PPAL,MENU_NONE,MENU_NONE,MENU_NONE,menu_acelerometro_printtexto},
    {MENU_TEMPERATURA_PPAL,MENU_RGB_PPAL,MENU_NONE,MENU_NONE,MENU_NONE,menu_acelerometro_printgrafico},
    {MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,menu_acelerometro_exit},
    {MENU_ACELEROMETRO_PPAL,MENU_SPEAKER_PPAL,MENU_NONE,MENU_NONE,MENU_RGB_SELR,menu_rgb_ppal},
    {MENU_RGB_RMENOS,MENU_RGB_RMAS,MENU_NONE,MENU_NONE,MENU_RGB_SELG,menu_rgb_selr},
    {MENU_RGB_GMENOS,MENU_RGB_GMAS,MENU_NONE,MENU_NONE,MENU_RGB_SELB,menu_rgb_selg},
    {MENU_RGB_BMENOS,MENU_RGB_BMAS,MENU_NONE,MENU_NONE,MENU_RGB_EXIT,menu_rgb_selb},
    {MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_exit},
    {MENU_RGB_PPAL,MENU_SIGNALS_PPAL,MENU_NONE,MENU_NONE,MENU_SPEAKER_SELBIP1,menu_speaker_ppal},
    {MENU_RGB_PPAL,MENU_SIGNALS_PPAL,MENU_SPEAKER_SELBIP2,MENU_SPEAKER_SELBIPNONE,MENU_SIGNALS_PPAL,menu_speaker_selbip1},
    {MENU_RGB_PPAL,MENU_SIGNALS_PPAL,MENU_SPEAKER_SELBIP3,MENU_SPEAKER_SELBIP1,MENU_SIGNALS_PPAL,menu_speaker_selbip2},
    {MENU_RGB_PPAL,MENU_SIGNALS_PPAL,MENU_SPEAKER_SELBIPNONE,MENU_SPEAKER_SELBIP2,MENU_SIGNALS_PPAL,menu_speaker_selbip3},
    {MENU_RGB_PPAL,MENU_SIGNALS_PPAL,MENU_SPEAKER_SELBIP1,MENU_SPEAKER_SELBIP3,MENU_SIGNALS_PPAL,menu_speaker_selbipnone},
    {MENU_SPEAKER_PPAL,MENU_TEMPERATURA_PPAL,MENU_NONE,MENU_NONE,MENU_SIGNALS_SELSQUAREWAVE,menu_signals_ppal},
    {MENU_SPEAKER_PPAL,MENU_TEMPERATURA_PPAL,MENU_SIGNALS_SELSINEWAVE,MENU_SIGNALS_SELSINEWAVE,MENU_SIGNALS_PRINTSQUAREWAVE,menu_signals_selsquarewave},
    {MENU_SPEAKER_PPAL,MENU_TEMPERATURA_PPAL,MENU_SIGNALS_SELSQUAREWAVE,MENU_SIGNALS_SELSQUAREWAVE,MENU_SIGNALS_PRINTSINEWAVE,menu_signals_selsinewave},
    {MENU_SIGNALS_EXIT,MENU_SIGNALS_EXIT,MENU_NONE,MENU_NONE,MENU_NONE,menu_signals_printsquarewave},
    {MENU_SIGNALS_EXIT,MENU_SIGNALS_EXIT,MENU_NONE,MENU_NONE,MENU_NONE,menu_signals_printsinewave},
    {MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,MENU_NONE,menu_signals_exit},
    {MENU_RGB_RMENOS,MENU_RGB_RMAS,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_rmenos},
    {MENU_RGB_RMENOS,MENU_RGB_RMAS,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_rmas},
    {MENU_RGB_GMENOS,MENU_RGB_GMAS,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_gmenos},
    {MENU_RGB_GMENOS,MENU_RGB_GMAS,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_gmas},
    {MENU_RGB_BMENOS,MENU_RGB_BMAS,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_bmenos},
    {MENU_RGB_BMENOS,MENU_RGB_BMAS,MENU_NONE,MENU_NONE,MENU_NONE,menu_rgb_bmas}
};

typedef struct {
    float x, y, z;
} Acelerometro_t;

QueueHandle_t xScaleQueue;
QueueHandle_t xKeysQueue;
QueueHandle_t xBeeperQueue;
QueueHandle_t xTemperatureQueue;
QueueHandle_t xAcelerometroQueue;

TaskHandle_t TaskHandleScale;
TaskHandle_t TaskHandleTemperature;
TaskHandle_t TaskHandleAcelerometro;



//-------------------------------------------
// FONTS
//-------------------------------------------
// Small 7
// 
const unsigned char Small_7[] = {
    19,9,9,2,                                    // Length,horz,vert,byte/vert
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char
    0x02, 0x00, 0x00, 0x9E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char !
    0x04, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char "
    0x06, 0x00, 0x00, 0x50, 0x00, 0xF8, 0x00, 0x50, 0x00, 0xF8, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char #
    0x06, 0x00, 0x00, 0x8C, 0x00, 0x92, 0x00, 0xFE, 0x01, 0xA2, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char $
    0x07, 0x1E, 0x00, 0x92, 0x00, 0x5E, 0x00, 0x20, 0x00, 0xF8, 0x00, 0x94, 0x00, 0xF2, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char %
    0x07, 0x00, 0x00, 0x64, 0x00, 0x9A, 0x00, 0xAA, 0x00, 0xCC, 0x00, 0x60, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char &
    0x02, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char '
    0x03, 0x00, 0x00, 0x7C, 0x00, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char (
    0x03, 0x00, 0x00, 0x83, 0x01, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char )
    0x04, 0x00, 0x00, 0x30, 0x00, 0x78, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char *
    0x05, 0x10, 0x00, 0x10, 0x00, 0x7C, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char +
    0x02, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ,
    0x04, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char -
    0x02, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char .
    0x04, 0x00, 0x01, 0xE0, 0x00, 0x1C, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char /
    0x05, 0x00, 0x00, 0x7C, 0x00, 0x82, 0x00, 0x82, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 0
    0x05, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 1
    0x05, 0x00, 0x00, 0x84, 0x00, 0xC2, 0x00, 0xA2, 0x00, 0x9C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 2
    0x05, 0x00, 0x00, 0x82, 0x00, 0x92, 0x00, 0x92, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 3
    0x05, 0x00, 0x00, 0x38, 0x00, 0x2C, 0x00, 0x22, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 4
    0x05, 0x00, 0x00, 0x9E, 0x00, 0x92, 0x00, 0x92, 0x00, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 5
    0x05, 0x00, 0x00, 0x7C, 0x00, 0x92, 0x00, 0x92, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 6
    0x05, 0x00, 0x00, 0x02, 0x00, 0xC2, 0x00, 0x32, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 7
    0x05, 0x00, 0x00, 0x6C, 0x00, 0x92, 0x00, 0x92, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 8
    0x05, 0x00, 0x00, 0x9C, 0x00, 0x92, 0x00, 0x92, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 9
    0x02, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char :
    0x02, 0x00, 0x01, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ;
    0x05, 0x10, 0x00, 0x10, 0x00, 0x28, 0x00, 0x28, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char <
    0x05, 0x00, 0x00, 0x28, 0x00, 0x28, 0x00, 0x28, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char =
    0x05, 0x00, 0x00, 0x44, 0x00, 0x28, 0x00, 0x28, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char >
    0x05, 0x00, 0x00, 0x02, 0x00, 0xB2, 0x00, 0x12, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ?
    0x09, 0x00, 0x00, 0xF8, 0x00, 0x84, 0x01, 0x72, 0x01, 0x4A, 0x01, 0x4A, 0x01, 0x7A, 0x01, 0x42, 0x00, 0x3C, 0x00,  // Code for char @
    0x06, 0x00, 0x00, 0xF8, 0x00, 0x24, 0x00, 0x22, 0x00, 0x24, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char A
    0x06, 0x00, 0x00, 0xFE, 0x00, 0x92, 0x00, 0x92, 0x00, 0x92, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char B
    0x06, 0x00, 0x00, 0x7C, 0x00, 0x82, 0x00, 0x82, 0x00, 0x82, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char C
    0x06, 0x00, 0x00, 0xFE, 0x00, 0x82, 0x00, 0x82, 0x00, 0xC6, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char D
    0x05, 0x00, 0x00, 0xFE, 0x00, 0x92, 0x00, 0x92, 0x00, 0x92, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char E
    0x05, 0x00, 0x00, 0xFE, 0x00, 0x12, 0x00, 0x12, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char F
    0x06, 0x00, 0x00, 0x7C, 0x00, 0xC6, 0x00, 0x82, 0x00, 0x92, 0x00, 0xF6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char G
    0x06, 0x00, 0x00, 0xFE, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char H
    0x02, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char I
    0x04, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char J
    0x05, 0x00, 0x00, 0xFE, 0x00, 0x10, 0x00, 0x2C, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char K
    0x05, 0x00, 0x00, 0xFE, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char L
    0x08, 0x00, 0x00, 0xFE, 0x00, 0x06, 0x00, 0x18, 0x00, 0xE0, 0x00, 0x18, 0x00, 0x06, 0x00, 0xFE, 0x00, 0x00, 0x00,  // Code for char M
    0x06, 0x00, 0x00, 0xFE, 0x00, 0x06, 0x00, 0x18, 0x00, 0x60, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char N
    0x06, 0x00, 0x00, 0x7C, 0x00, 0x82, 0x00, 0x82, 0x00, 0x82, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char O
    0x05, 0x00, 0x00, 0xFE, 0x00, 0x12, 0x00, 0x12, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char P
    0x07, 0x00, 0x00, 0x7C, 0x00, 0x82, 0x00, 0x82, 0x00, 0xC2, 0x00, 0xFC, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,  // Code for char Q
    0x05, 0x00, 0x00, 0xFE, 0x00, 0x12, 0x00, 0x12, 0x00, 0xEC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char R
    0x05, 0x00, 0x00, 0xCC, 0x00, 0x92, 0x00, 0x92, 0x00, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char S
    0x06, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0xFE, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char T
    0x06, 0x00, 0x00, 0x7E, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char U
    0x07, 0x00, 0x00, 0x06, 0x00, 0x3C, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0x1C, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char V
    0x06, 0x00, 0x00, 0x1E, 0x00, 0xE0, 0x00, 0x3E, 0x00, 0xE0, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char W
    0x06, 0x00, 0x00, 0x82, 0x00, 0x64, 0x00, 0x38, 0x00, 0x6C, 0x00, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char X
    0x06, 0x00, 0x00, 0x02, 0x00, 0x0C, 0x00, 0xF0, 0x00, 0x0C, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char Y
    0x06, 0x00, 0x00, 0x82, 0x00, 0xE2, 0x00, 0x92, 0x00, 0x8E, 0x00, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char Z
    0x03, 0x00, 0x00, 0xFF, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char [
    0x04, 0x01, 0x00, 0x0E, 0x00, 0x70, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char BackSlash
    0x02, 0x01, 0x01, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ]
    0x04, 0x00, 0x00, 0x18, 0x00, 0x0C, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ^
    0x06, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char _
    0x03, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char `
    0x05, 0x00, 0x00, 0xE8, 0x00, 0xA8, 0x00, 0xA8, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char a
    0x05, 0x00, 0x00, 0xFE, 0x00, 0x88, 0x00, 0x88, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char b
    0x05, 0x00, 0x00, 0x70, 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char c
    0x05, 0x00, 0x00, 0x70, 0x00, 0x88, 0x00, 0x88, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char d
    0x05, 0x00, 0x00, 0x70, 0x00, 0xA8, 0x00, 0xA8, 0x00, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char e
    0x04, 0x08, 0x00, 0xFE, 0x00, 0x0A, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char f
    0x05, 0x00, 0x00, 0x30, 0x00, 0x48, 0x01, 0x48, 0x01, 0xF8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char g
    0x05, 0x00, 0x00, 0xFE, 0x00, 0x08, 0x00, 0x08, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char h
    0x02, 0x00, 0x00, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char i
    0x02, 0x00, 0x01, 0xFA, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char j
    0x05, 0x00, 0x00, 0xFE, 0x00, 0x20, 0x00, 0x50, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char k
    0x02, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char l
    0x06, 0x00, 0x00, 0xF8, 0x00, 0x08, 0x00, 0xF8, 0x00, 0x08, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char m
    0x05, 0x00, 0x00, 0xF8, 0x00, 0x08, 0x00, 0x08, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char n
    0x05, 0x00, 0x00, 0x70, 0x00, 0x88, 0x00, 0x88, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char o
    0x05, 0x00, 0x00, 0xF8, 0x01, 0x48, 0x00, 0x48, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char p
    0x05, 0x00, 0x00, 0x30, 0x00, 0x48, 0x00, 0x48, 0x00, 0xF8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char q
    0x04, 0x00, 0x00, 0xF8, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char r
    0x04, 0x00, 0x00, 0x98, 0x00, 0xA8, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char s
    0x04, 0x00, 0x00, 0x08, 0x00, 0xFC, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char t
    0x05, 0x00, 0x00, 0x78, 0x00, 0x80, 0x00, 0x80, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char u
    0x04, 0x00, 0x00, 0x38, 0x00, 0xC0, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char v
    0x06, 0x00, 0x00, 0x78, 0x00, 0xC0, 0x00, 0x38, 0x00, 0xC0, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char w
    0x05, 0x00, 0x00, 0x88, 0x00, 0x70, 0x00, 0x70, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char x
    0x05, 0x00, 0x00, 0x38, 0x00, 0x40, 0x01, 0x40, 0x01, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char y
    0x05, 0x00, 0x00, 0xC8, 0x00, 0xE8, 0x00, 0xB8, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char z
    0x04, 0x10, 0x00, 0x38, 0x00, 0xEF, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char {
    0x03, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char |
    0x04, 0x01, 0x01, 0xC7, 0x01, 0x38, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char }
    0x05, 0x0C, 0x00, 0x04, 0x00, 0x0C, 0x00, 0x08, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ~
    0x03, 0xFE, 0x01, 0x02, 0x01, 0xFE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // Code for char 
    };


const unsigned char Arial12x12[] = {
    25,12,12,2,                                                                           // Length,horz,vert,byte/vert
    0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char
    0x02, 0x00, 0x00, 0x7F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char !
    0x03, 0x07, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char "
    0x07, 0x24, 0x00, 0xA4, 0x01, 0x7C, 0x00, 0xA7, 0x01, 0x7C, 0x00, 0x27, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char #
    0x06, 0x00, 0x00, 0xCE, 0x00, 0x11, 0x01, 0xFF, 0x03, 0x11, 0x01, 0xE2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char $
    0x0A, 0x00, 0x00, 0x0E, 0x00, 0x11, 0x00, 0x11, 0x01, 0xCE, 0x00, 0x38, 0x00, 0xE6, 0x00, 0x11, 0x01, 0x10, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char %
    0x08, 0x00, 0x00, 0xE0, 0x00, 0x1E, 0x01, 0x11, 0x01, 0x29, 0x01, 0xC6, 0x00, 0xA0, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char &
    0x02, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char '
    0x04, 0x00, 0x00, 0xF8, 0x00, 0x06, 0x03, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char (
    0x03, 0x01, 0x04, 0x06, 0x03, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char )
    0x05, 0x02, 0x00, 0x0A, 0x00, 0x07, 0x00, 0x0A, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char *
    0x06, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x7C, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char +
    0x02, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ,
    0x03, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char -
    0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char .
    0x03, 0x80, 0x01, 0x7C, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char /
    0x06, 0x00, 0x00, 0xFE, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 0
    0x06, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 1
    0x06, 0x00, 0x00, 0x02, 0x01, 0x81, 0x01, 0x41, 0x01, 0x31, 0x01, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 2
    0x06, 0x00, 0x00, 0x82, 0x00, 0x01, 0x01, 0x11, 0x01, 0x11, 0x01, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 3
    0x06, 0x00, 0x00, 0x60, 0x00, 0x58, 0x00, 0x46, 0x00, 0xFF, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 4
    0x06, 0x00, 0x00, 0x9C, 0x00, 0x0B, 0x01, 0x09, 0x01, 0x09, 0x01, 0xF1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 5
    0x06, 0x00, 0x00, 0xFE, 0x00, 0x11, 0x01, 0x09, 0x01, 0x09, 0x01, 0xF2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 6
    0x06, 0x00, 0x00, 0x01, 0x00, 0xC1, 0x01, 0x39, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 7
    0x06, 0x00, 0x00, 0xEE, 0x00, 0x11, 0x01, 0x11, 0x01, 0x11, 0x01, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 8
    0x06, 0x00, 0x00, 0x9E, 0x00, 0x21, 0x01, 0x21, 0x01, 0x11, 0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char 9
    0x02, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char :
    0x02, 0x00, 0x00, 0x40, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ;
    0x06, 0x00, 0x00, 0x10, 0x00, 0x28, 0x00, 0x28, 0x00, 0x44, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char <
    0x06, 0x48, 0x00, 0x48, 0x00, 0x48, 0x00, 0x48, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char =
    0x06, 0x00, 0x00, 0x44, 0x00, 0x44, 0x00, 0x28, 0x00, 0x28, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char >
    0x06, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00, 0x61, 0x01, 0x11, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ?
    0x0C, 0x00, 0x00, 0xF0, 0x01, 0x0C, 0x02, 0xE2, 0x04, 0x12, 0x09, 0x09, 0x09, 0x09, 0x09, 0xF1, 0x09, 0x19, 0x09, 0x02, 0x05, 0x86, 0x04, 0x78, 0x02,  // Code for char @
    0x07, 0x80, 0x01, 0x70, 0x00, 0x2E, 0x00, 0x21, 0x00, 0x2E, 0x00, 0x70, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char A
    0x07, 0x00, 0x00, 0xFF, 0x01, 0x11, 0x01, 0x11, 0x01, 0x11, 0x01, 0x11, 0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char B
    0x08, 0x00, 0x00, 0x7C, 0x00, 0x82, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x82, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char C
    0x08, 0x00, 0x00, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x82, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char D
    0x07, 0x00, 0x00, 0xFF, 0x01, 0x11, 0x01, 0x11, 0x01, 0x11, 0x01, 0x11, 0x01, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char E
    0x06, 0x00, 0x00, 0xFF, 0x01, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char F
    0x08, 0x00, 0x00, 0x7C, 0x00, 0x82, 0x00, 0x01, 0x01, 0x01, 0x01, 0x11, 0x01, 0x92, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char G
    0x08, 0x00, 0x00, 0xFF, 0x01, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char H
    0x02, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char I
    0x05, 0xC0, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char J
    0x08, 0x00, 0x00, 0xFF, 0x01, 0x20, 0x00, 0x10, 0x00, 0x28, 0x00, 0x44, 0x00, 0x82, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char K
    0x07, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char L
    0x08, 0x00, 0x00, 0xFF, 0x01, 0x06, 0x00, 0x78, 0x00, 0x80, 0x01, 0x78, 0x00, 0x06, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char M
    0x08, 0x00, 0x00, 0xFF, 0x01, 0x02, 0x00, 0x0C, 0x00, 0x10, 0x00, 0x60, 0x00, 0x80, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char N
    0x08, 0x00, 0x00, 0x7C, 0x00, 0x82, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x82, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char O
    0x07, 0x00, 0x00, 0xFF, 0x01, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char P
    0x08, 0x00, 0x00, 0x7C, 0x00, 0x82, 0x00, 0x01, 0x01, 0x41, 0x01, 0x41, 0x01, 0x82, 0x00, 0x7C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char Q
    0x08, 0x00, 0x00, 0xFF, 0x01, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x31, 0x00, 0xD1, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char R
    0x07, 0x00, 0x00, 0xCE, 0x00, 0x11, 0x01, 0x11, 0x01, 0x11, 0x01, 0x11, 0x01, 0xE6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char S
    0x07, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0xFF, 0x01, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char T
    0x08, 0x00, 0x00, 0x7F, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x80, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char U
    0x07, 0x03, 0x00, 0x1C, 0x00, 0x60, 0x00, 0x80, 0x01, 0x60, 0x00, 0x1C, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char V
    0x0B, 0x07, 0x00, 0x78, 0x00, 0x80, 0x01, 0x70, 0x00, 0x0E, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x70, 0x00, 0x80, 0x01, 0x7C, 0x00, 0x03, 0x00, 0x00, 0x00,  // Code for char W
    0x07, 0x01, 0x01, 0xC6, 0x00, 0x28, 0x00, 0x10, 0x00, 0x28, 0x00, 0xC6, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char X
    0x07, 0x01, 0x00, 0x06, 0x00, 0x08, 0x00, 0xF0, 0x01, 0x08, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char Y
    0x07, 0x00, 0x01, 0x81, 0x01, 0x61, 0x01, 0x11, 0x01, 0x0D, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char Z
    0x03, 0x00, 0x00, 0xFF, 0x07, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char [
    0x03, 0x03, 0x00, 0x7C, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char BackSlash
    0x02, 0x01, 0x04, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ]
    0x05, 0x18, 0x00, 0x06, 0x00, 0x01, 0x00, 0x06, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ^
    0x07, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char _
    0x03, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char `
    0x06, 0x00, 0x00, 0xC8, 0x00, 0x24, 0x01, 0x24, 0x01, 0xA4, 0x00, 0xF8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char a
    0x06, 0x00, 0x00, 0xFF, 0x01, 0x88, 0x00, 0x04, 0x01, 0x04, 0x01, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char b
    0x05, 0x00, 0x00, 0xF8, 0x00, 0x04, 0x01, 0x04, 0x01, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char c
    0x06, 0x00, 0x00, 0xF8, 0x00, 0x04, 0x01, 0x04, 0x01, 0x08, 0x01, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char d
    0x06, 0x00, 0x00, 0xF8, 0x00, 0x24, 0x01, 0x24, 0x01, 0x24, 0x01, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char e
    0x04, 0x04, 0x00, 0xFE, 0x01, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char f
    0x06, 0x00, 0x00, 0xF8, 0x04, 0x04, 0x05, 0x04, 0x05, 0x88, 0x04, 0xFC, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char g
    0x06, 0x00, 0x00, 0xFF, 0x01, 0x08, 0x00, 0x04, 0x00, 0x04, 0x00, 0xF8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char h
    0x02, 0x00, 0x00, 0xFD, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char i
    0x02, 0x00, 0x04, 0xFD, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char j
    0x06, 0x00, 0x00, 0xFF, 0x01, 0x20, 0x00, 0x30, 0x00, 0xC8, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char k
    0x02, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char l
    0x0A, 0x00, 0x00, 0xFC, 0x01, 0x08, 0x00, 0x04, 0x00, 0x04, 0x00, 0xF8, 0x01, 0x08, 0x00, 0x04, 0x00, 0x04, 0x00, 0xF8, 0x01, 0x00, 0x00, 0x00, 0x00,  // Code for char m
    0x06, 0x00, 0x00, 0xFC, 0x01, 0x08, 0x00, 0x04, 0x00, 0x04, 0x00, 0xF8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char n
    0x06, 0x00, 0x00, 0xF8, 0x00, 0x04, 0x01, 0x04, 0x01, 0x04, 0x01, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char o
    0x06, 0x00, 0x00, 0xFC, 0x07, 0x88, 0x00, 0x04, 0x01, 0x04, 0x01, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char p
    0x06, 0x00, 0x00, 0xF8, 0x00, 0x04, 0x01, 0x04, 0x01, 0x88, 0x00, 0xFC, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char q
    0x04, 0x00, 0x00, 0xFC, 0x01, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char r
    0x06, 0x00, 0x00, 0x98, 0x00, 0x24, 0x01, 0x24, 0x01, 0x24, 0x01, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char s
    0x03, 0x04, 0x00, 0xFF, 0x01, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char t
    0x06, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0xFC, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char u
    0x05, 0x0C, 0x00, 0x70, 0x00, 0x80, 0x01, 0x70, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char v
    0x09, 0x0C, 0x00, 0x70, 0x00, 0x80, 0x01, 0x70, 0x00, 0x0C, 0x00, 0x70, 0x00, 0x80, 0x01, 0x70, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char w
    0x05, 0x04, 0x01, 0xD8, 0x00, 0x20, 0x00, 0xD8, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char x
    0x05, 0x0C, 0x00, 0x70, 0x04, 0x80, 0x03, 0x70, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char y
    0x05, 0x04, 0x01, 0xC4, 0x01, 0x24, 0x01, 0x1C, 0x01, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char z
    0x03, 0x20, 0x00, 0xDE, 0x03, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char {
    0x02, 0x00, 0x00, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char |
    0x04, 0x00, 0x00, 0x01, 0x04, 0xDE, 0x03, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char }
    0x07, 0x00, 0x00, 0x20, 0x00, 0x10, 0x00, 0x10, 0x00, 0x20, 0x00, 0x20, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Code for char ~
    0x08, 0x00, 0x00, 0xFE, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0xFE, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // Code for char 
    };





void menu_temperatura_ppal(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Arial12x12);
            lcd.locate(24, 10);
            lcd.printf("Temperatura");
            lcd.copy_to_lcd(); // update lcd
            menu.sm = MENUSM_LOOP;
            #ifdef DEBUGENABLE_TASKMENU
            printf("Menu Temperatura Ppal\r\n");
            #endif
            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};



void menu_temperatura_seltexto(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Small_7);
            lcd.locate(0, 0);
            lcd.printf("Temperatura");
            lcd.locate(30, 10);
            lcd.printf("Texto <");
            lcd.locate(30, 20);
            lcd.printf("Grafico");
            lcd.copy_to_lcd(); // update lcd
            menu.sm = MENUSM_LOOP;
            #ifdef DEBUGENABLE_TASKMENU
            printf("Menu Temperatura Elegir Texto\r\n");
            #endif

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_temperatura_selgrafico(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.locate(0, 0);
            lcd.printf("Temperatura");
            lcd.locate(30, 10);
            lcd.printf("Texto");
            lcd.locate(30, 20);
            lcd.printf("Grafico <");
            lcd.copy_to_lcd(); // update lcd
            #ifdef DEBUGENABLE_TASKMENU
            printf("Menu Temperatura Elegir Grafico\r\n");
            #endif

            menu.sm = MENUSM_LOOP;
            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_temperatura_printtexto(enum Cmd_e cmd)
{
    BaseType_t xQueueStatus;
    float f;

    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }

    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.printf("Temperatura:");
            lcd.copy_to_lcd();
            #ifdef DEBUGENABLE_TASKMENU
            printf("Menu Temperatura TEXTO\r\n");
            #endif

            vTaskResume(TaskHandleTemperature);
            menu.sm = MENUSM_LOOP;
            break;
        }

        case MENUSM_LOOP: {
            #ifdef DEBUGENABLE_LOGTEMPERATUREPRINT
            //printf("(temp print)\r\n");
            #endif
            xQueueStatus = xQueueReceive(xTemperatureQueue, &f, 5);
            if (xQueueStatus == pdPASS) {
                lcd.cls();
                lcd.locate(10,10);
                lcd.printf("Temperatura: %.1f",f);
                #ifdef DEBUGENABLE_LOGTEMPERATUREPRINT
                printf("Temperatura: %.1f\r\n",f);
                #endif
                lcd.copy_to_lcd();
            }


            break;
        }

    }
};


void menu_temperatura_printgrafico(enum Cmd_e cmd)
{
    BaseType_t xQueueStatus;
    float f;
    int y;
    uint8_t loop;
    static float scale_y;

    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.copy_to_lcd();   // update lcd
            menu.grafico.historysize = 0;
            menu.grafico.idx_puntoactual = 0;
            menu.grafico.x_pos = 0.0;
            scale_y = CTE_ALTODISPLAY/(CTE_TEMPERATURAMAXIMA-CTE_TEMPERATURAMINIMA);
            vTaskResume(TaskHandleTemperature);
            #ifdef DEBUGENABLE_TASKMENU
            printf("Menu Temperatura GRAFICO\r\n");
            #endif
            menu.sm = MENUSM_LOOP;
            break;
        }

        case MENUSM_LOOP: {
            xQueueStatus = xQueueReceive(xTemperatureQueue, &f, 5);
            if (xQueueStatus == pdPASS) {

                if (menu.grafico.idx_puntoactual < CTE_HISTORYSIZE) {
                    y = round(CTE_TEMPERATURAMAXIMA - scale_y*(f-CTE_TEMPERATURAMINIMA));
                    menu.grafico.historia[menu.grafico.idx_puntoactual] = y;
                    lcd.pixel(menu.grafico.idx_puntoactual, menu.grafico.historia[menu.grafico.idx_puntoactual],1);
                    menu.grafico.idx_puntoactual++;
                    lcd.copy_to_lcd(); // update lcd
                } else {
                    lcd.cls();
                    for(loop=0;loop<CTE_HISTORYSIZE-1;loop++) {
                        menu.grafico.historia[loop] = menu.grafico.historia[loop+1];
                        lcd.pixel(loop,menu.grafico.historia[loop],1);
                    }
                    y = round(CTE_TEMPERATURAMAXIMA - scale_y*(f-CTE_TEMPERATURAMINIMA));
                    menu.grafico.historia[menu.grafico.idx_puntoactual] = y;
                    lcd.pixel(menu.grafico.idx_puntoactual, menu.grafico.historia[CTE_HISTORYSIZE-1],1);
                    lcd.copy_to_lcd(); // update lcd
                }
            }

            break;
        }

    }
};


void menu_temperatura_exit(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Temperatura Exit\r\n");
            #endif
            vTaskSuspend(TaskHandleTemperature);
            menu.idx_tmp = MENU_TEMPERATURA_PPAL;
            break;
        }

        case MENUSM_LOOP: {
            menu.idx_tmp = MENU_TEMPERATURA_PPAL;
            break;
        }

    }
};


void menu_acelerometro_ppal(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Arial12x12);
            lcd.locate(24, 10);
            lcd.printf("Acelerometro");
            lcd.copy_to_lcd(); // update lcd
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Acelerometro Principal\r\n");
            #endif

            menu.sm = MENUSM_LOOP;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_acelerometro_seltexto(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Small_7);
            lcd.locate(0, 0);
            lcd.printf("Acelerometro");
            lcd.locate(30, 10);
            lcd.printf("Texto <");
            lcd.locate(30, 20);
            lcd.printf("Grafico");
            lcd.copy_to_lcd(); // update lcd

            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Acelerometro Texto\r\n");
            #endif

            menu.sm = MENUSM_LOOP;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_acelerometro_selgrafico(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.locate(0, 0);
            lcd.printf("Acelerometro");
            lcd.locate(30, 10);
            lcd.printf("Texto");
            lcd.locate(30, 20);
            lcd.printf("Grafico <");
            lcd.copy_to_lcd(); // update lcd
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Acelerometro Grafico\r\n");
            #endif

            menu.sm = MENUSM_LOOP;
            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};




void menu_acelerometro_printtexto(enum Cmd_e cmd)
{
    BaseType_t xQueueStatus;
    Acelerometro_t accel_read;

    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Acelerometro Print Texto\r\n");
            #endif
            vTaskResume(TaskHandleAcelerometro);
            menu.sm = MENUSM_LOOP;
            break;
        }

        case MENUSM_LOOP: {
            xQueueStatus = xQueueReceive(xAcelerometroQueue, &accel_read, 5);
            if (xQueueStatus == pdPASS) {
                lcd.cls();
                lcd.locate(0,0);
                lcd.printf("Acelerometro X: %.1f",accel_read.x);
                lcd.locate(0,10);
                lcd.printf("Acelerometro Y: %.1f",accel_read.y);
                lcd.locate(0,20);
                lcd.printf("Acelerometro Z: %.1f",accel_read.z);
                lcd.copy_to_lcd();
            }

            break;
        }

    }
};


void menu_acelerometro_printgrafico(enum Cmd_e cmd)
{
    BaseType_t xQueueStatus;
    Acelerometro_t accel_read;
    uint8_t delta_axis;
    uint8_t loop;

    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Acelerometro Print Grafico\r\n");
            #endif
            vTaskResume(TaskHandleAcelerometro);
            menu.sm = MENUSM_LOOP;
            break;
        }

        case MENUSM_LOOP: {
            xQueueStatus = xQueueReceive(xAcelerometroQueue, &accel_read, 5);
            if (xQueueStatus == pdPASS) {
                lcd.cls();
                lcd.locate(55,0);
                lcd.printf(":X:");
                lcd.locate(55,10);
                lcd.printf(":Y:");
                lcd.locate(55,20);
                lcd.printf(":Z:");
                if (accel_read.x > 0) {
                    delta_axis = accel_read.x / 0.03;
                    for(loop=0;loop<delta_axis;loop++) {
                        lcd.line(70+loop,0,70+loop,10,1);
                    }
                } else {
                    delta_axis = -accel_read.x / 0.03;
                    for(loop=0;loop<delta_axis;loop++) {
                        lcd.line(50-loop,0,50-loop,10,1);
                    }
                }

                if (accel_read.y > 0) {
                    delta_axis = accel_read.y / 0.03;
                    for(loop=0;loop<delta_axis;loop++) {
                        lcd.line(70+loop,11,70+loop,20,1);
                    }
                } else {
                    delta_axis = -accel_read.y / 0.03;
                    for(loop=0;loop<delta_axis;loop++) {
                        lcd.line(50-loop,11,50-loop,20,1);
                    }
                }


                if (accel_read.z > 0) {
                    delta_axis = accel_read.z / 0.03;
                    for(loop=0;loop<delta_axis;loop++) {
                        lcd.line(70+loop,21,70+loop,30,1);
                    }
                } else {
                    delta_axis = -accel_read.z / 0.03;
                    for(loop=0;loop<delta_axis;loop++) {
                        lcd.line(50-loop,21,50-loop,30,1);
                    }
                }


                lcd.copy_to_lcd();
            }

            break;
        }

    }
};


void menu_acelerometro_exit(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }

    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Acelerometro Exit\r\n");
            #endif
            vTaskSuspend(TaskHandleAcelerometro);
            menu.idx_tmp = MENU_ACELEROMETRO_PPAL;
            break;
        }

        case MENUSM_LOOP: {
            
            break;
        }

    }
};


void menu_rgb_ppal(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Arial12x12);
            lcd.locate(44, 10);
            lcd.printf("RGB");
            lcd.copy_to_lcd(); // update lcd
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu RGB Ppal\r\n");
            #endif

            menu.sm = MENUSM_LOOP;
            break;
        }

        case MENUSM_LOOP: {
            
            break;
        }

    }
};


void fcn_graphbarrapje(int x_start, int y_start, float pje)
{
    uint8_t loop=1;
    float x=0.01;
    while(loop<=100) {
        if (x <= pje) {
            lcd.line(x_start+loop,y_start,x_start+loop,y_start+8,1);
        } else {
            lcd.line(x_start+loop,y_start,x_start+loop,y_start+8,0);
        }
        x += 0.01;
        loop+=1;
    }
    
   
}

void menu_rgb_selr(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }

    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            
            if (menu.updatestatus != 1) {
                lcd.cls();
                lcd.set_font((unsigned char*) Small_7);
                lcd.locate(0,0);
                lcd.printf("R*");
                lcd.locate(0,11);
                lcd.printf("G");
                lcd.locate(0,21);
                lcd.printf("B");
                menu.updatestatus = 1;
            }

            if (menu.r != menu.r_ant) {
                fcn_graphbarrapje(15,0,menu.r);
                menu.r_ant = menu.r;
            }

            if (menu.g != menu.g_ant) {
                fcn_graphbarrapje(15,11,menu.g);
                menu.g_ant = menu.g;
            }

            if (menu.b != menu.b_ant) {
                fcn_graphbarrapje(15,21,menu.b);
                menu.b_ant = menu.b;
            }

            lcd.copy_to_lcd(); // update lcd

            r = 1.0 - menu.r;
            g = 1.0 - menu.g;
            b = 1.0 - menu.b;
            menu.sm = MENUSM_LOOP;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};



void menu_rgb_selg(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            if (menu.updatestatus != 2) {
                lcd.rect(6,0,6,10,0);
                lcd.rect(7,0,7,10,0);
                lcd.rect(8,0,8,10,0);
                lcd.rect(9,0,9,10,0);
                lcd.locate(0,11);
                lcd.printf("G*");
                menu.updatestatus = 2;
            }

            if (menu.r != menu.r_ant) {
                fcn_graphbarrapje(15,0,menu.r);
                menu.r_ant = menu.r;
            }   

            if (menu.g != menu.g_ant) {
                fcn_graphbarrapje(15,11,menu.g);
                menu.g_ant = menu.g;
            }   

            if (menu.b != menu.b_ant) {
                fcn_graphbarrapje(15,21,menu.b);
                menu.b_ant = menu.b;
            }   

            lcd.copy_to_lcd(); // update lcd
            r = menu.r;
            g = menu.g;
            b = menu.b;

            menu.sm = MENUSM_LOOP;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_rgb_selb(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            if (menu.updatestatus != 3) {
                lcd.rect(6,11,6,21,0);
                lcd.rect(7,11,7,21,0);
                lcd.rect(8,11,8,21,0);
                lcd.rect(9,11,9,21,0);
                lcd.locate(0,21);
                lcd.printf("B*");
                menu.updatestatus = 3;
            }

            if (menu.r != menu.r_ant) {
                fcn_graphbarrapje(15,0,menu.r);
                menu.r_ant = menu.r;
            }   

            if (menu.g != menu.g_ant) {
                fcn_graphbarrapje(15,11,menu.g);
                menu.g_ant = menu.g;
            }   

            if (menu.b != menu.b_ant) {
                fcn_graphbarrapje(15,21,menu.b);
                menu.b_ant = menu.b;
            }   

            lcd.copy_to_lcd(); // update lcd
            r = menu.r;
            g = menu.g;
            b = menu.b;
            menu.sm = MENUSM_LOOP;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_rgb_exit(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            // inicializamos todo para que al proximo ingreso al menu de seleccion se actualice la pantalla
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu RGB exit\r\n");
            #endif
            menu.r_ant = -menu.r;
            menu.b_ant = -menu.b;
            menu.g_ant = -menu.g;
            menu.idx_tmp = MENU_RGB_PPAL;
            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};

void menu_speaker_ppal(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Arial12x12);
            lcd.locate(50, 10);
            lcd.printf("Speaker");
            lcd.copy_to_lcd(); // update lcd

            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Speaker Principal\r\n");
            #endif

            menu.sm = MENUSM_LOOP;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_speaker_selbip1(enum Cmd_e cmd)
{
    BaseType_t xStatus;
    static uint8_t sound;  // para que no suene todo el tiempo

    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Small_7);
            lcd.locate(24, 10);
            lcd.printf("> bip1 < ");
            lcd.locate(24, 20);
            lcd.printf("  bip2   ");
            lcd.locate(64, 10);
            lcd.printf("  bip3   ");
            lcd.locate(64, 20);
            lcd.printf("  none   ");
            lcd.copy_to_lcd(); // update lcd
            menu.bip = KEY_BIP1;

            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Speaker Bip1\r\n");
            #endif

            menu.sm = MENUSM_LOOP;

            break;
        }

        case MENUSM_LOOP: {
            if (sound >= CTE_CANTBIPSENMENUSLECCION) {
                xStatus = xQueueSendToBack(xBeeperQueue, &menu.bip, 0);
                sound=0;
            } else {
                sound++;
            }
            break;
        }

    }
};


void menu_speaker_selbip2(enum Cmd_e cmd)
{
    BaseType_t xStatus;
    static uint8_t sound;  // para que no suene todo el tiempo

    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Small_7);
            lcd.locate(24, 10);
            lcd.printf("  bip1  ");
            lcd.locate(24, 20);
            lcd.printf("> bip2 <");
            lcd.locate(64, 10);
            lcd.printf("  bip3  ");
            lcd.locate(64, 20);
            lcd.printf("  none  ");
            lcd.copy_to_lcd(); // update lcd
            menu.bip = KEY_BIP2;
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Speaker Bip2\r\n");
            #endif

            menu.sm = MENUSM_LOOP;

            break;
        }

        case MENUSM_LOOP: {
            if (sound >= CTE_CANTBIPSENMENUSLECCION) {
                xStatus = xQueueSendToBack(xBeeperQueue, &menu.bip, 0);
                sound=0;
            } else {
                sound++;
            }
            break;
        }

    }
};


void menu_speaker_selbip3(enum Cmd_e cmd)
{
    BaseType_t xStatus;
    static uint8_t sound;  // para que no suene todo el tiempo

    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Small_7);
            lcd.locate(24, 10);
            lcd.printf("  bip1   ");
            lcd.locate(24, 20);
            lcd.printf("  bip2   ");
            lcd.locate(64, 10);
            lcd.printf("> bip3 <");
            lcd.locate(64, 20);
            lcd.printf("  none  ");
            lcd.copy_to_lcd(); // update lcd
            menu.bip = KEY_BIP3;
            menu.sm = MENUSM_LOOP;
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Speaker Bip3\r\n");
            #endif
            sound=0;

            break;
        }

        case MENUSM_LOOP: {
            if (sound >= CTE_CANTBIPSENMENUSLECCION) {
                xStatus = xQueueSendToBack(xBeeperQueue, &menu.bip, 0);
                sound=0;
            } else {
                sound++;
            }
            break;
        }

    }
};


void menu_speaker_selbipnone(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Small_7);
            lcd.locate(34, 10);
            lcd.printf("  bip1  ");
            lcd.locate(34, 20);
            lcd.printf("  bip2  ");
            lcd.locate(74, 10);
            lcd.printf("  bip3  ");
            lcd.locate(74, 20);
            lcd.printf("> none <");
            lcd.copy_to_lcd(); // update lcd
            menu.bip = KEY_BIPNONE;
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Speaker BipNone\r\n");
            #endif

            menu.sm = MENUSM_LOOP;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_signals_ppal(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Arial12x12);
            lcd.locate(34, 10);
            lcd.printf("Signals");
            lcd.copy_to_lcd(); // update lcd
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Signals Principal\r\n");
            #endif
            menu.sm = MENUSM_LOOP;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_signals_selsquarewave(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Small_7);
            lcd.locate(0, 0);
            lcd.printf("Signals");
            lcd.locate(30, 10);
            lcd.printf("Cuadrada <");
            lcd.locate(30, 20);
            lcd.printf("Sinusoidal");
            lcd.copy_to_lcd(); // update lcd

            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Signals Seleccionado Square\r\n");
            #endif

            menu.sm = MENUSM_LOOP;
            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};




void menu_signals_selsinewave(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            lcd.set_font((unsigned char*) Small_7);
            lcd.locate(0, 0);
            lcd.printf("Signals");
            lcd.locate(30, 10);
            lcd.printf("Cuadrada");
            lcd.locate(30, 20);
            lcd.printf("Sinusoidal <");
            lcd.copy_to_lcd(); // update lcd
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Signals Seleccionado Seno\r\n");
            #endif

            menu.sm = MENUSM_LOOP;
            break;
        }

        case MENUSM_LOOP: {

            break;
        }

    }
};


void menu_signals_printsquarewave(enum Cmd_e cmd)
{
    Tscale scale;
    BaseType_t  xQueueStatus;
    static float y;
    static float y_ant;
    //static int y_result_ant;
    uint8_t loop;

    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            menu.grafico.historysize = 0;
            menu.grafico.idx_puntoactual = 0;
            menu.grafico.x_pos = 0.0;
            vTaskResume(TaskHandleScale);
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Signals Print Square\r\n");
            #endif
            menu.sm = MENUSM_LOOP;

            break;
        }


        case MENUSM_LOOP: {
            xQueueStatus = xQueueReceive(xScaleQueue, &scale, 5);
            if (xQueueStatus == pdPASS) {
                // Update del scale x e y
                menu.grafico.x_scale = CTE_XSTEPSQUAREMIN + scale.x_scale*(CTE_XSTEPSQUAREMAX-CTE_XSTEPSQUAREMIN);
                menu.grafico.y_scale = CTE_YSCALEMIN + scale.y_scale*(CTE_YSCALEMAX - CTE_YSCALEMIN);
            }


            if (menu.grafico.idx_puntoactual < CTE_HISTORYSIZE) {
                menu.grafico.y_pos = sin(menu.grafico.x_pos*3.14/180);
                if (menu.grafico.y_pos >= 0) menu.grafico.y_pos = 1.0; else menu.grafico.y_pos = -1.0;
                y = round(16 + menu.grafico.y_pos * menu.grafico.y_scale * 14);
                menu.grafico.historia[menu.grafico.idx_puntoactual] = y;
                menu.grafico.x_pos += menu.grafico.x_scale;
                lcd.pixel(menu.grafico.idx_puntoactual, menu.grafico.historia[menu.grafico.idx_puntoactual],1);

                if (((menu.grafico.y_pos > 0) && (y_ant < 0)) ||
                    ((menu.grafico.y_pos < 0) && (y_ant > 0))) {
                    lcd.rect(menu.grafico.x_pos,menu.grafico.historia[menu.grafico.idx_puntoactual],menu.grafico.x_pos,menu.grafico.historia[menu.grafico.idx_puntoactual-1],1);
                }
                y_ant = menu.grafico.y_pos;
                menu.grafico.idx_puntoactual++;
                lcd.copy_to_lcd(); // update lcd
            } else {
                lcd.cls();
                for(loop=0;loop<CTE_HISTORYSIZE-1;loop++) {
                    menu.grafico.historia[loop] = menu.grafico.historia[loop+1];
                    lcd.pixel(loop,menu.grafico.historia[loop],1);
                    if (((menu.grafico.historia[loop+1] > 16) && (menu.grafico.historia[loop] < 16)) ||
                        ((menu.grafico.historia[loop+1] < 16) && (menu.grafico.historia[loop] > 16))) {
                         lcd.rect(loop,menu.grafico.historia[menu.grafico.idx_puntoactual],loop,menu.grafico.historia[menu.grafico.idx_puntoactual+1],1);                            
                    }
                }
                menu.grafico.y_pos = sin(menu.grafico.x_pos*3.14/180);
                if (menu.grafico.y_pos >= 0) menu.grafico.y_pos = 1.0; else menu.grafico.y_pos = -1.0;
                y = round(16 + menu.grafico.y_pos * menu.grafico.y_scale * 14);
                menu.grafico.historia[CTE_HISTORYSIZE-1] = y;
                menu.grafico.x_pos += menu.grafico.x_scale;
                lcd.pixel(menu.grafico.idx_puntoactual, menu.grafico.historia[CTE_HISTORYSIZE-1],1);
                lcd.copy_to_lcd(); // update lcd
            }

            break;
        }

    }
};

void menu_signals_printsinewave(enum Cmd_e cmd)
{
    int y;
    uint8_t loop;
    BaseType_t xQueueStatus;
    Tscale scale;

    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            lcd.cls();
            menu.grafico.historysize = 0;
            menu.grafico.idx_puntoactual = 0;
            menu.grafico.x_pos = 0.0;
            vTaskResume(TaskHandleScale);

            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Signals Print Sinusoidal\r\n");
            #endif

            menu.sm = MENUSM_LOOP;
            break;
        }

        case MENUSM_LOOP: {
            xQueueStatus = xQueueReceive(xScaleQueue, &scale, 5);
            if (xQueueStatus == pdPASS) {
                // Update del scale x e y
                menu.grafico.x_scale = CTE_XSTEPMIN + scale.x_scale*(CTE_XSTEPMAX-CTE_XSTEPMIN);
                menu.grafico.y_scale = CTE_YSCALEMIN + scale.y_scale*(CTE_YSCALEMAX - CTE_YSCALEMIN);
            }
//            if (menu.grafico.historysize < CTE_HISTORYSIZE) {
            if (menu.grafico.idx_puntoactual < CTE_HISTORYSIZE) {
                menu.grafico.y_pos = sin(menu.grafico.x_pos*3.14/180);
                y = round(16 + menu.grafico.y_pos * menu.grafico.y_scale * 14);
                menu.grafico.historia[menu.grafico.idx_puntoactual] = y;
                menu.grafico.x_pos += menu.grafico.x_scale;
                lcd.pixel(menu.grafico.idx_puntoactual, menu.grafico.historia[menu.grafico.idx_puntoactual],1);
                menu.grafico.idx_puntoactual++;
                lcd.copy_to_lcd(); // update lcd
            } else {
                lcd.cls();
                for(loop=0;loop<CTE_HISTORYSIZE-1;loop++) {
                    menu.grafico.historia[loop] = menu.grafico.historia[loop+1];
                    lcd.pixel(loop,menu.grafico.historia[loop],1);
                }
                menu.grafico.y_pos = sin(menu.grafico.x_pos*3.14/180);
                y = round(16 + menu.grafico.y_pos * menu.grafico.y_scale * 14);
                menu.grafico.historia[CTE_HISTORYSIZE-1] = y;
                menu.grafico.x_pos += menu.grafico.x_scale;
                lcd.pixel(menu.grafico.idx_puntoactual, menu.grafico.historia[CTE_HISTORYSIZE-1],1);
                lcd.copy_to_lcd(); // update lcd
            }
            break;
        }

    }
};


void menu_signals_exit(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            #ifdef DEBUGENABLE_TASKMENU            
            printf("Menu Signals Exit\r\n");
            #endif

            vTaskSuspend(TaskHandleScale);
            menu.idx_tmp = MENU_SIGNALS_PPAL;
            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_rgb_rmenos(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            printf("R- %f\r\n",menu.r);
            if (menu.r > CTE_RGBINCREMENT) {
                menu.r -= CTE_RGBINCREMENT;
            }
            menu.idx_tmp = MENU_RGB_SELR;
            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_rgb_rmas(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            printf("R+ %f\r\n",menu.r);
            if (menu.r < 1.0) {
                menu.r += CTE_RGBINCREMENT;
            }
            menu.idx_tmp = MENU_RGB_SELR;
            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_rgb_gmenos(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            printf("G- %f\r\n",menu.g);
            if (menu.g > CTE_RGBINCREMENT) {
                menu.g -= CTE_RGBINCREMENT;
            }
            menu.idx_tmp = MENU_RGB_SELG;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_rgb_gmas(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            printf("g+ %f\r\n",menu.g);
            if (menu.g < 1.0) {
                menu.g += CTE_RGBINCREMENT;
            }
            menu.idx_tmp = MENU_RGB_SELG;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_rgb_bmenos(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            printf("B- %f\r\n",menu.b);
            if (menu.b > CTE_RGBINCREMENT) {
                menu.b -= CTE_RGBINCREMENT;
            }
            menu.idx_tmp = MENU_RGB_SELB;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};


void menu_rgb_bmas(enum Cmd_e cmd)
{
    if (cmd == CMD_INITIALIZE) {
        menu.sm = MENUSM_INITIALIZE;
    }
    switch(menu.sm) {
        case MENUSM_INITIALIZE: {
            printf("B+ %f\r\n",menu.b);
            if (menu.b < 1.0) {
                menu.b += CTE_RGBINCREMENT;
            }
            menu.idx_tmp = MENU_RGB_SELB;

            break;
        }

        case MENUSM_LOOP: {
            break;
        }

    }
};



// <-----  FIN MENUS ------


void Task_Menu (void* pvParameters)
{
    BaseType_t xQueueStatus;
    uint8_t key, key_ant;
    UBaseType_t queueelements;

    Tmenuitem * menuitem;
    
#ifndef DISABLE_USART
    uint8_t char_input;
#endif

    menu.sm  = MENUSM_INITIALIZE;
    menu.idx = MENU_TEMPERATURA_PPAL;
    menu.idx_tmp = MENU_TEMPERATURA_PPAL;
    menu.r = 0.50;
    menu.g = 0.50;
    menu.b = 0.50;
    // para forzar un primer update 
    menu.r_ant = 0.1;
    menu.g_ant = 0.1;
    menu.b_ant = 0.1;
    menu.updatestatus = 0;
    r.period(0.001) ;
    g.period(0.001) ;
    b.period(0.001) ;
    menu.grafico.historysize=0;

    menu.grafico.x_scale = CTE_XSTEPMAX;
    menu.grafico.y_scale = 1.0;
    menu.bip = KEY_BIPNONE;
    menuitem = &menuitems[0];
    menuitem += menu.idx;
    (*menuitem->func_ptr)(CMD_INITIALIZE);
    key_ant = KEY_NONE;
   
    (void) pvParameters;                    // Just to stop compiler warnings.
    
    // Tomar semaforos para que las tareas no se bloqueen



    for (;;) {
        led1= !led1;
        #ifdef DEBUGENABLE_TASKMENUDETAIL
        printf("<Task Menu: Queue elements=");
        #endif
        queueelements = uxQueueMessagesWaiting(xKeysQueue);
        #ifdef DEBUGENABLE_TASKMENUDETAIL
        printf("%d\r\n",queueelements);
        #endif
        key = KEY_NONE;
        if (0 < queueelements) {
            // La cola tiene mensaje(s)
            xQueueStatus = xQueueReceive(xKeysQueue, &key, 5);
            if (xQueueStatus == pdPASS) {
                #ifdef DEBUGENABLE_TASKMENUDETAIL
                printf(" <KeyRead= %d>\r\n",key);
                #endif
            } else {
                #ifdef DEBUGENABLE_TASKMENUDETAIL
                printf(" QueKey Read Error\r\n");
                #endif
            }
        }

#ifndef DISABLE_USART
        scanf(" %c", &char_input);
        //if (char_input = getchar()); {
        //    led2 != led2;
        //}
//        /*
        if (char_input != 0) {
            switch(char_input) {
            case '1': {vTaskResume(TaskHandleTemperature); break;}
            case '2': {vTaskResume(TaskHandleScale); break;}
            case '3': {vTaskResume(TaskHandleAcelerometro); break;}
            case '4': {vTaskSuspend(TaskHandleTemperature); break;}
            case '5': {vTaskSuspend(TaskHandleScale); break;}
            case '6': {vTaskSuspend(TaskHandleAcelerometro); break;}
            }
        }
//        */
#endif

        if (key != key_ant) {
            switch(key) {
                case KEY_UP: {menu.idx_tmp = menuitem->keyup; break;}
                case KEY_DOWN: {menu.idx_tmp = menuitem->keydown; break;}
                case KEY_LEFT: {menu.idx_tmp = menuitem->keyleft; break;}
                case KEY_RIGHT: {menu.idx_tmp = menuitem->keyright; break;}
                case KEY_PUSH: {menu.idx_tmp = menuitem->keyfire; break;}
            }
        }
        
        #ifdef DEBUGENABLE_TASKMENUDETAIL
        printf("MenuTMP = %d\r\n",menu.idx_tmp);
        #endif

        menuitem = &menuitems[0];
        menuitem += menu.idx;
        if (menu.idx_tmp != MENU_NONE) {
            if (menu.idx_tmp != menu.idx) {
                // Se debe pasar de menu
        #ifdef DEBUGENABLE_TASKMENUDETAIL
        printf("Cambio de Menu\r\n",menu.idx_tmp);
        #endif
                menu.idx = menu.idx_tmp;
                menuitem = &menuitems[0];
                menuitem += menu.idx;
                (*menuitem->func_ptr)(CMD_INITIALIZE);

            } else {
        #ifdef DEBUGENABLE_TASKMENUDETAIL
        printf("Mismo Menu. ");
        #endif
                (*menuitem->func_ptr)(CMD_NONE);
            }
        }
    
        #ifdef DEBUGENABLE_TASKMENUDETAIL
        printf("Menu IDX=%d SM=%d idx_tmp=%d\r\n",menu.idx, menu.sm, menu.idx_tmp);
        #endif

        vTaskDelay(150);
    }
}



void Task_Joystick (void* pvParameters)
{
    int key_read;
    uint8_t key;
    //uint8_t key_ant;
    BaseType_t xStatus;
    //uint8_t bip;

    (void) pvParameters;
    for (;;) {
        key_read = joy;
        switch(key_read) {
            case 0: key = KEY_NONE; break;   // key_none
            case 1: key = KEY_DOWN; break;
            case 2: key = KEY_UP; break;
            case 4: key = KEY_LEFT; break;
            case 8: key = KEY_RIGHT; break;
        }
        if (fire) key = KEY_PUSH;
//        if (key != key_ant) {
            if (key != KEY_NONE) {
                xStatus = xQueueSendToBack(xKeysQueue, &key, 0);
                xStatus = xQueueSendToBack(xBeeperQueue, &menu.bip, 0);
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

//            key_ant = key;
//        }
        //printf("Task1\n");
        vTaskDelay(250);
    }
}



void Task_Beeper (void* pvParameters)
{
    BaseType_t xQueueStatus;
    UBaseType_t queueelements;
    uint8_t beeper; 
    (void) pvParameters;
    for (;;) {
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



void Task_Scale (void* pvParameters)
{
    BaseType_t xStatus;
    Tscale scale;

    (void) pvParameters;                    // Just to stop compiler warnings.
    for (;;) {
        led2= !led2;

        scale.x_scale = pot1.read();
        scale.y_scale = pot2.read();
        xStatus = xQueueSendToBack(xScaleQueue, &scale, 0);
        if (xStatus != pdPASS) {
    #ifdef DEBUGENABLE_TASKSCALE
            printf("<xScaleQueue. Error agregando elemento a la cola.\r\n");
    #endif
        } else {
    #ifdef DEBUGENABLE_TASKSCALE
            printf("<xScaleQueue. Scale X,Y: %f,%f>\r\n",scale.x_scale, scale.y_scale);
    #endif
        }
        vTaskDelay(500);
    }
}






void Task_Temperatura (void* pvParameters)
{
    BaseType_t xStatus;
    float f;

    led4 = 0; 
    (void) pvParameters;                    // Just to stop compiler warnings.
    if (!sensortemperatura.open()) {
        printf("Sensor Temperatura no presente\r\n");
//        vTaskSuspend(NULL);
    }
    for (;;) {
        led3 = !led3;
        f = sensortemperatura.temp();
        xStatus = xQueueSendToBack(xTemperatureQueue, &f,0);
        if (xStatus != pdPASS) {
    #ifdef DEBUGENABLE_LOGTEMPQUEUEERRORS
            printf("<xScaleQueue. Error agregando elemento a la cola.\r\n");
    #endif
        } else {
    #ifdef DEBUGENABLE_LOGTEMPQUEUELOGS
                printf("<Temp=%.1f grados leidos del sensor. Agregado a la cola.\r\n",f);
    #endif
        }
        vTaskDelay(500);
    }
}


void Task_Acelerometro (void* pvParameters)
{
    BaseType_t xStatus;
    Acelerometro_t accel;

    led4 = 0; 
    if(!mma.testConnection()) {
        printf("MMA7660 no detectado.\r\n");
        //vTaskSuspend(NULL);
    }
  
    (void) pvParameters;                    // Just to stop compiler warnings.

    for (;;) {
        led4 = !led4;
        printf("Leer Acelerometro\r\n");
        accel.z = mma.z();
        accel.x = mma.x();
        accel.y = mma.y();
        
        xStatus = xQueueSendToBack(xAcelerometroQueue, &accel,0);
        if (xStatus != pdPASS) {
    #ifdef DEBUGENABLE_LOGTEMPQUEUEERRORS
            printf("<xScaleQueue. Error agregando elemento a la cola.\r\n");
    #endif
        } else {
    #ifdef DEBUGENABLE_LOGTEMPQUEUELOGS
                printf("<Temp=%.1f grados leidos del sensor. Agregado a la cola.\r\n",f);
    #endif
        }


        vTaskDelay(500);
    }
}



int main (void)
{
    xKeysQueue = xQueueCreate (5,sizeof(uint8_t));
    xBeeperQueue = xQueueCreate (1,sizeof(uint8_t));
    xScaleQueue = xQueueCreate(5, sizeof(Tscale));
    xTemperatureQueue = xQueueCreate(1,sizeof(float));
    xAcelerometroQueue = xQueueCreate(3,sizeof(Acelerometro_t));

    xTaskCreate( Task_Joystick, ( const char * ) "Task Joystick", 192, NULL, 1, ( xTaskHandle * ) NULL );
    xTaskCreate( Task_Beeper, ( const char * ) "TaskBepper", 256, NULL, 1, ( xTaskHandle * ) NULL );
    xTaskCreate( Task_Menu, ( const char * ) "TaskMenu", 1532, NULL, 3, ( xTaskHandle * ) NULL);
    #ifndef DISABLE_LOGICARTOSSCALE
    xTaskCreate( Task_Scale, ( const char * ) "Task Scale", 128, NULL, 2, &TaskHandleScale);
    #endif
    #ifndef DISABLE_LOGICARTOSTEMPERATURA
    xTaskCreate( Task_Temperatura, ( const char * ) "Task Temperatura", 1024, NULL, 2, &TaskHandleTemperature);
    #endif
    #ifndef DISABLE_LOGICARTOSACELEROMETRO
    xTaskCreate( Task_Acelerometro, ( const char * ) "Task Acelerometro", 512, NULL, 2, &TaskHandleAcelerometro);
    #endif
    vTaskSuspend(TaskHandleScale);
    vTaskSuspend(TaskHandleTemperature);
    vTaskSuspend(TaskHandleAcelerometro);
    printf("\r\n\r\n\r\n\r\n\r\nTrabajo Final Arquitecturas Embebidas y Procesamiento en Tiempo Real\r\n");
    printf("--------------------------------------------------------------------\r\n\r\n\r\n");

    vTaskStartScheduler();
    //should never get here
    printf("ERORR: vTaskStartScheduler returned!");
    for (;;);
}
