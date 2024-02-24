#ifndef APP_EEPROM_H
#define APP_EEPROM_H
#include <stdint.h>
#include "app_sys.h"

//ĿǰԤ��ǰ1K�ֽڸ�bootloader������app��0x400��ʼ
#define APP_USER_PARAM_ADDR   0x00  //ʵ����0x00070000+APP_USER_PARAM_ADDR
#define APP_PARAM_FLAG        0xB0


/*�洢��EEPROM�е�����*/
typedef struct
{
    uint8_t VERSION;        /*��ǰ����汾��*/
    uint8_t updateStatus;
    uint8_t apn[50];
    uint8_t apnuser[50];
    uint8_t apnpassword[50];
    uint8_t SN[20];
    uint8_t updateServer[50];
    uint8_t codeVersion[50];
    uint16_t updatePort;
} SYSTEM_FLASH_DEFINE;


extern SYSTEM_FLASH_DEFINE sysparam;


void paramInit(void);
void paramDefaultInit(uint8_t type);

void paramGetCodeVersion(char *version);
void paramSaveUpdateStatus(uint8_t status);

void paramSaveAll(void);
#endif

