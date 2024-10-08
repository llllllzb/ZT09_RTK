#include "app_param.h"
#include "app_gps.h"
#include "app_db.h"
#include "app_task.h"
bootParam_s bootparam;
systemParam_s sysparam;
dynamicParam_s dynamicParam;


void bootParamSaveAll(void)
{
    bootparam.VERSION = BOOT_PARAM_FLAG;
    EEPROM_ERASE(BOOT_USER_PARAM_ADDR, sizeof(bootParam_s));
    EEPROM_WRITE(BOOT_USER_PARAM_ADDR, &bootparam, sizeof(bootParam_s));
}
void bootParamGetAll(void)
{
    EEPROM_READ(BOOT_USER_PARAM_ADDR, &bootparam, sizeof(bootParam_s));
}

void paramSaveAll(void)
{
    EEPROM_ERASE(APP_USER_PARAM_ADDR, sizeof(systemParam_s));
    EEPROM_WRITE(APP_USER_PARAM_ADDR, &sysparam, sizeof(systemParam_s));
}
void paramGetAll(void)
{
    EEPROM_READ(APP_USER_PARAM_ADDR, &sysparam, sizeof(systemParam_s));
}

void dynamicParamSaveAll(void)
{
	EEPROM_ERASE(APP_DYNAMIC_PARAM_ADDR, sizeof(dynamicParam_s));
	EEPROM_WRITE(APP_DYNAMIC_PARAM_ADDR, &dynamicParam, sizeof(dynamicParam_s));
}

void dynamicParamGetAll(void)
{
	EEPROM_READ(APP_DYNAMIC_PARAM_ADDR, &dynamicParam, sizeof(dynamicParam_s));
}


void paramDefaultInit(uint8_t level)
{
    LogMessage(DEBUG_ALL, "paramDefaultInit");
    if (level == 0)
    {
        memset(&sysparam, 0, sizeof(systemParam_s));
        memset(&dynamicParam, 0, sizeof(dynamicParam_s));
        sysparam.VERSION = APP_PARAM_FLAG;
        strcpy(dynamicParam.SN, "888888887777777");
        strncpy(dynamicParam.jt808sn, "888777", 6);
		strncpy(sysparam.zhdsn, "88887777", 9);
		strncpy(sysparam.zhdUser, "12345678", 9);
		strncpy(sysparam.zhdPassword, "12345678", 9);
		
        strcpy(sysparam.Server, "jzwz.basegps.com");
        strcpy(sysparam.hiddenServer, "jzwz.basegps.com");
        strcpy(sysparam.jt808Server, "47.106.96.28");

		strcpy(sysparam.zhdServer, "59.42.52.138");

        sysparam.ServerPort = 9998;
        sysparam.hiddenPort = 9998;
        sysparam.jt808Port = 9997;
		sysparam.zhdPort = 65002;
		
		sysparam.protocol = 0;
        sysparam.utc = 8;
        sysparam.AlarmTime[0] = 720;
        sysparam.AlarmTime[1] = 0xFFFF;
        sysparam.AlarmTime[2] = 0xFFFF;
        sysparam.AlarmTime[3] = 0xFFFF;
        sysparam.AlarmTime[4] = 0xFFFF;
        sysparam.MODE = 2;
        sysparam.gpsuploadgap = 10;
        sysparam.gapDay = 1;
        sysparam.poitype = 2;
        sysparam.lowvoltage = 36;
        sysparam.adccal = 3.06;
        strcpy(sysparam.apn, "cmnet");

        strcpy((char *)sysparam.agpsServer, "agps.domilink.com");
        strcpy((char *)sysparam.agpsUser, "123");
        strcpy((char *)sysparam.agpsPswd, "123");
        sysparam.agpsPort = 10189;

    }
    strcpy(sysparam.Server, "jzwz.basegps.com");
    sysparam.ServerPort = 9998;
    sysparam.gpsuploadgap = 60;
    sysparam.gapMinutes = 30;
    dynamicParam.runTime = 0;
    dynamicParam.startUpCnt = 0;
    sysparam.accctlgnss = 1;
    sysparam.accdetmode = 2;
    sysparam.heartbeatgap = 180;
    sysparam.ledctrl = 0;
    sysparam.poitype = 2;
    sysparam.fence = 0;
    sysparam.accOnVoltage = 13.2;
    sysparam.accOffVoltage = 12.8;
    dynamicParam.noNmeaRstCnt = 0;
    sysparam.sosalm = ALARM_TYPE_NONE;
    sysparam.ldrEn = 1;
    sysparam.tiltalm = 0;
    sysparam.gsdettime=15;
    sysparam.gsValidCnt=3;
    sysparam.gsInvalidCnt=0;
    sysparam.hiddenServOnoff = 0;
    sysparam.debug = 0;
    sysparam.agpsen = 1;
    dynamicParam.debug = 0;
    sysparam.mode4Alarm = 1200;
    sysparam.smsreply = 0;
    sysparam.bf = 0;
    sysparam.range = 0x61;
    sysparam.stepFliter = 0xA7;
    sysparam.smThrd = 0x60;
    sysparam.motionstep = 30;
    sysparam.staticStep = 30;
    sysparam.ntripEn = 1;
    sysparam.gpsFilterType = GPS_FILTER_AUTO;
    sysparam.tempcal = -4;
    sysparam.pwrOnoff = 1;
    sysparam.sleep_start = 0;
    sysparam.sleep_end   = 0;	
    dynamicParamSaveAll();
    paramSaveAll();
}

void paramInit(void)
{
    paramGetAll();
    dynamicParamGetAll();
    if (sysparam.VERSION != APP_PARAM_FLAG)
    {
        paramDefaultInit(0);
    }
   	if (sysparam.otaParamFlag != OTA_PARAM_FLAG)
    {
		sysparam.otaParamFlag = OTA_PARAM_FLAG;
		sysparam.sleep_start = 0;
    	sysparam.sleep_end   = 0;
		paramSaveAll();
    }
    sysinfo.lowvoltage = sysparam.lowvoltage / 10.0;
    dbInfoRead();
}
