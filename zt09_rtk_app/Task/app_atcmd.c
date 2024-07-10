#include "app_protocol.h"
#include "app_atcmd.h"
#include "app_db.h"
#include "app_gps.h"
#include "app_instructioncmd.h"
#include "app_kernal.h"
#include "app_mir3da.h"
#include "app_net.h"
#include "app_param.h"
#include "app_port.h"
#include "app_sys.h"
#include "app_socket.h"
#include "app_server.h"
#include "aes.h"
#include "app_jt808.h"
#include "app_task.h"
#include "base64.h"
#include "app_zhdprotocol.h"

/*
 * 指令集
 */
const CMDTABLE atcmdtable[] =
{
    {AT_DEBUG_CMD, "DEBUG"},
    {AT_SMS_CMD, "SMS"},
    {AT_NMEA_CMD, "NMEA"},
    {AT_ZTSN_CMD, "ZTSN"},
    {AT_IMEI_CMD, "IMEI"},
    {AT_FMPC_NMEA_CMD, "FMPC_NMEA"},
    {AT_FMPC_BAT_CMD, "FMPC_BAT"},
    {AT_FMPC_GSENSOR_CMD, "FMPC_GSENSOR"},
    {AT_FMPC_ACC_CMD, "FMPC_ACC"},
    {AT_FMPC_GSM_CMD, "FMPC_GSM"},
    {AT_FMPC_RELAY_CMD, "FMPC_RELAY"},
    {AT_FMPC_LDR_CMD, "FMPC_LDR"},
    {AT_FMPC_ADCCAL_CMD, "FMPC_ADCCAL"},
    {AT_FMPC_CSQ_CMD, "FMPC_CSQ"},
    {AT_FMPC_IMSI_CMD, "FMPC_IMSI"},
    {AT_FMPC_CHKP_CMD, "FMPC_CHKP"},
    {AT_FMPC_CM_CMD, "FMPC_CM"},
    {AT_FMPC_CMGET_CMD, "FMPC_CMGET"},
    {AT_FMPC_EXTVOL_CMD, "FMPC_EXTVOL"},
    {AT_FMPC_TEMP_CMD, "FMPC_TEMP"},
};
/**************************************************
@bref		查找指令
@param
@return
@note
**************************************************/

static int16_t getatcmdid(uint8_t *cmdstr)
{
    uint16_t i = 0;
    for (i = 0; i < sizeof(atcmdtable) / sizeof(atcmdtable[0]); i++)
    {
        if (mycmdPatch(cmdstr, (uint8_t *)atcmdtable[i].cmdstr) != 0)
            return atcmdtable[i].cmdid;
    }
    return -1;
}

/**************************************************
@bref		AT^DEBUG 指令解析
@param
@return
@note
**************************************************/

static void doAtdebugCmd(uint8_t *buf, uint16_t len)
{
    int8_t ret;
    ITEM item;
   
    stringToItem(&item, buf, len);
    strToUppper(item.item_data[0], strlen(item.item_data[0]));

    if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"SUSPEND"))
    {
        LogMessage(DEBUG_LOW, "Suspend all task");
        systemTaskSuspend(sysinfo.sysTaskId);
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"RESUME"))
    {
        LogMessage(DEBUG_LOW, "Resume all task");
        systemTaskResume(sysinfo.sysTaskId);
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"DTRH"))
    {
        WAKEMODULE;
        LogMessage(DEBUG_ALL, "DTR high");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"DTRL"))
    {
        SLEEPMODULE;
        LogMessage(DEBUG_ALL, "DTR low");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"POWERKEYH"))
    {
        PORT_PWRKEY_H;
        LogMessage(DEBUG_ALL, "Power key hight");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"POWERKEYL"))
    {
        PORT_PWRKEY_L;
        LogMessage(DEBUG_ALL, "Power key low");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"SUPPLYON"))
    {
        PORT_SUPPLY_ON;
        LogMessage(DEBUG_ALL, "supply on");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"SUPPLYOFF"))
    {
        PORT_SUPPLY_OFF;
        LogMessage(DEBUG_ALL, "supply off");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"NTCON"))
    {
		NTC_ON;
		LogMessage(DEBUG_ALL, "ntc on");
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GPSCLOSE"))
    {
        sysinfo.gpsRequest = 0;
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"MODULEOFF"))
    {
        modulePowerOff();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"MODULERESET"))
    {
        moduleReset();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"MODULEON"))
    {
        modulePowerOn();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"SLEEP"))
    {
        portSleepEn();
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GSOPEN"))
    {
		portGsensorCtl(1);
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GSCLOSE"))
    {
		portGsensorCtl(0);
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GPSHIGH"))
    {
		GPIOB_ModeCfg(GPSPWR_PIN, GPIO_ModeOut_PP_5mA);
	    GPSPWR_ON;
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GPSLOW"))
    {
		GPIOB_ModeCfg(GPSPWR_PIN, GPIO_ModeOut_PP_5mA);
	    GPSPWR_OFF;
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GPSRXH"))
    {
		GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);
		GPIOB_SetBits(GPIO_Pin_7);
		LogPrintf(DEBUG_ALL, "GPSRXH");
    }
   	else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GPSRXL"))
    {
		GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);
		GPIOB_ResetBits(GPIO_Pin_7);
		LogPrintf(DEBUG_ALL, "GPSRXL");
    }
   	else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GPSUTON"))
    {
		portUartCfg(APPUSART0, 1, 115200, gpsUartRead);
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GPSUTOFF"))
    {
		portUartCfg(APPUSART0, 0, 115200, gpsUartRead);
    }
    else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"TEMP"))
    {
		LogPrintf(DEBUG_ALL, "temp:%.2f", getTemp());
    }
	else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"BASE"))
	{
		char src[100] = { 0 };
		char en[100] = { 0 };
		sprintf(src, "%s:%s", item.item_data[1], item.item_data[2]);
		base64_encode(src, strlen(src), en);
		LogPrintf(DEBUG_ALL, "de:%s", en);
	}
	else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"LG"))
	{
		LogPrintf(DEBUG_ALL, "sn:%s", sysparam.zhdsn);
		zhd_lg_info_register(sysparam.zhdsn, sysparam.zhdUser, sysparam.zhdPassword);
		zhd_protocol_send(ZHD_PROTOCOL_LG, NULL);
	}
	else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"GP"))
	{
		gpsinfo_s gpsinfo;
		gpsinfo.longtitude = 113.361511;
		gpsinfo.latitude   = 22.981545;
		gpsinfo.hight      = 40.716999;
		gpsinfo.fixAccuracy = 1;
		gpsinfo.speed      = 0.029000;
		gpsinfo.datetime.year = 21;
		gpsinfo.datetime.month = 9;
		gpsinfo.datetime.day = 22;
		gpsinfo.datetime.hour = 9;
		gpsinfo.datetime.minute = 10;
		gpsinfo.datetime.second = 52;
		zhd_gp_info_register(&gpsinfo, 100);
		zhd_protocol_send(ZHD_PROTOCOL_GP, NULL);
	}
	else if (mycmdPatch((uint8_t *)item.item_data[0], (uint8_t *)"AAA"))
	{
		uint8_t aaa[] = {0x3b, 0x89, 0x08, 0xFF, 0x22, 0x57, 0x5C, 0x40};
		uint8_t bbb[] = {0x31, 0x04, 0x84, 0xFF, 0x22, 0x57, 0x5C, 0x40};
		double la, lo;
		int i;
		memcpy(&la, aaa, sizeof(aaa));
		memcpy(&lo, bbb, sizeof(bbb));
		uint8_t *pla = (void *)&la;
		for(i = 0;i<8;i++)
			LogPrintf(DEBUG_ALL, "%#x", pla[i]);
		pla = (void *)&lo;
        for(i = 0;i<8;i++)
            LogPrintf(DEBUG_ALL, "%#x", pla[i]);
		LogPrintf(DEBUG_ALL, "sss:%.12lf  %.12lf", la, lo);
	}
    else
    {
        if (item.item_data[0][0] >= '0' && item.item_data[0][0] <= '9')
        {
            sysinfo.logLevel = item.item_data[0][0] - '0';
            LogPrintf(DEBUG_NONE, "Debug LEVEL:%d OK", sysinfo.logLevel);
        }
    }
}
/**************************************************
@bref		AT^FMPC_ADCCAL 指令解析
@param
@return
@note
**************************************************/

static void atCmdFmpcAdccalParser(void)
{
    float x;
    x = portGetAdcVol(ADC_CHANNEL);
    LogPrintf(DEBUG_ALL, "vCar:%.2f", x);
    sysparam.adccal = 4.0 / x;
    paramSaveAll();
    LogPrintf(DEBUG_FACTORY, "Update the ADC calibration parameter to %f", sysparam.adccal);
}

/**************************************************
@bref		AT^NMEA 指令解析
@param
@return
@note
**************************************************/

static void atCmdNmeaParser(uint8_t *buf, uint16_t len)
{
    char buff[80];
    if (my_strstr((char *)buf, "ON", len))
    {
        strcpy(buff, "$QXCFGMSG,1,4,1*0A\r\n");
        portUartSend(&usart0_ctl, buff, strlen(buff));
        LogMessage(DEBUG_FACTORY, "NMEA ON OK");
        sysinfo.nmeaOutPutCtl = 1;
        gpsRequestSet(GPS_REQUEST_DEBUG);
    }
    else
    {
        LogMessage(DEBUG_FACTORY, "NMEA OFF OK");
        gpsRequestClear(GPS_REQUEST_ALL);
        sysinfo.nmeaOutPutCtl = 0;
    }
}

/**************************************************
@bref		AT^FMPC_GSENSOR 指令解析
@param
@return
@note
**************************************************/

static void atCmdFMPCgsensorParser(void)
{
    read_gsensor_id();
}

/**************************************************
@bref		ZTSN 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdZTSNParser(uint8_t *buf, uint16_t len)
{

}


/**************************************************
@bref		IMEI 指令
@param
	buf
	len
@return
@note
**************************************************/

void atCmdIMEIParser(void)
{
    char imei[20];
    LogPrintf(DEBUG_FACTORY, "ZTINFO:%s:%s:%s", dynamicParam.SN, getModuleIMEI(), EEPROM_VERSION);
}

//unsigned char nmeaCrc(char *str, int len)
//{
//    int i, index, size;
//    unsigned char crc;
//    crc = str[1];
//    index = getCharIndex((uint8_t *)str, len, '*');
//    size = len - index;
//    for (i = 2; i < len - size; i++)
//    {
//        crc ^= str[i];
//    }
//    return crc;
//}

/**************************************************
@bref		FMPC_NMEA 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcNmeaParser(uint8_t *buf, uint16_t len)
{
	char buff[80];
	if (my_strstr((char *)buf, "ON", len))
	{
		sysinfo.nmeaOutPutCtl = 1;
		//开启AGPS有效性检测
//		sprintf(buff, "$PCAS03,,,,,,,,,,,1*1F\r\n");
//		portUartSend(&usart3_ctl, (uint8_t *)buff, strlen(buff));
		LogMessage(DEBUG_FACTORY, "NMEA OPEN");
	}
	else
	{
//		//关闭AGPS有效性检测
//		sprintf(buff, "$PCAS03,,,,,,,,,,,0*1E\r\n");
//		portUartSend(&usart3_ctl, (uint8_t *)buff, strlen(buff));
		sysinfo.nmeaOutPutCtl = 0;
		LogMessage(DEBUG_FACTORY, "NMEA CLOSE");
	}

}
/**************************************************
@bref		FMPC_BAT 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcBatParser(void)
{
    queryBatVoltage();
    LogPrintf(DEBUG_FACTORY, "Vbat: %.3fv", sysinfo.insidevoltage);
}
/**************************************************
@bref		FMPC_ACC 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcAccParser(void)
{
    LogPrintf(DEBUG_FACTORY, "ACC is %s", ACC_READ == ACC_STATE_ON ? "ON" : "OFF");
}

/**************************************************
@bref		FMPC_GSM  指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcGsmParser(void)
{
    if (isModuleRunNormal())
    {
        LogMessage(DEBUG_FACTORY, "GSM SERVICE OK");
    }
    else
    {
        LogMessage(DEBUG_FACTORY, "GSM SERVICE FAIL");
    }
}

/**************************************************
@bref		FMPC_CSQ 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcCsqParser(void)
{
    sendModuleCmd(CSQ_CMD, NULL);
    LogPrintf(DEBUG_FACTORY, "+CSQ: %d,99\r\nOK", getModuleRssi());
}


/**************************************************
@bref		FMPC_IMSI 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcIMSIParser(void)
{
    sendModuleCmd(CIMI_CMD, NULL);
    sendModuleCmd(MCCID_CMD, NULL);
    sendModuleCmd(CGSN_CMD, "1");
    LogPrintf(DEBUG_FACTORY, "FMPC_IMSI_RSP OK, IMSI=%s&&%s&&%s", dynamicParam.SN, getModuleIMSI(), getModuleICCID());
}

/**************************************************
@bref		FMPC_CHKP 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcChkpParser(void)
{
    LogPrintf(DEBUG_FACTORY, "+FMPC_CHKP:%s,%s:%d", dynamicParam.SN, sysparam.Server, sysparam.ServerPort);
}

/**************************************************
@bref		FMPC_CM 指令
@param
	buf
	len
@return
@note
**************************************************/

static void atCmdFmpcCmParser(void)
{
    sysparam.cm = 1;
    paramSaveAll();
    LogMessage(DEBUG_FACTORY, "CM OK");

}
/**************************************************
@bref		FMPC_CMGET 指令
@param
@return
@note
**************************************************/

static void atCmdCmGetParser(void)
{
    if (sysparam.cm == 1)
    {
        LogMessage(DEBUG_FACTORY, "CM OK");
    }
    else
    {
        LogMessage(DEBUG_FACTORY, "CM FAIL");
    }
}
/**************************************************
@bref		FMPC_EXTVOL 指令
@param
@return
@note
**************************************************/

static void atCmdFmpcExtvolParser(void)
{
    LogPrintf(DEBUG_FACTORY, "+FMPC_EXTVOL: %.2f", sysinfo.outsidevoltage);
}

/**************************************************
@bref		FMPC_EXTVOL 指令
@param
@return
@note
**************************************************/

static void atCmdFMPCLdrParase(void)
{
    if (LDR_READ)
    {
        LogMessage(DEBUG_FACTORY, "Light sensor detects darkness\n");
    }
    else
    {
        LogMessage(DEBUG_FACTORY, "Light sensor detects brightness\n");

    }
}

/**************************************************
@bref		FMPC_TEMP 指令
@param
@return
@note
**************************************************/

static void atCmdFMPCTempParase(uint8_t *buf, uint16_t len)
{
	float temp;
	temp = atof(buf);
	sysinfo.temprature = getTemp();
	sysparam.tempcal = temp - getTemp();
	LogPrintf(DEBUG_FACTORY, "+FMPC_TEMP: %.2f cal:%.2f", sysinfo.temprature + sysparam.tempcal, sysparam.tempcal);
}


/**************************************************
@bref		AT 指令解析
@param
@return
@note
**************************************************/

void atCmdParserFunction(uint8_t *buf, uint16_t len)
{
    int ret, cmdlen, cmdid;
    char cmdbuf[51];
    //识别AT^
    if (buf[0] == 'A' && buf[1] == 'T' && buf[2] == '^')
    {
        LogMessageWL(DEBUG_ALL, (char *)buf, len);
        ret = getCharIndex(buf, len, '=');
        if (ret < 0)
        {
            ret = getCharIndex(buf, len, '\r');

        }
        if (ret >= 0)
        {
            cmdlen = ret - 3;
            if (cmdlen < 50)
            {
                strncpy(cmdbuf, (const char *)buf + 3, cmdlen);
                cmdbuf[cmdlen] = 0;
                cmdid = getatcmdid((uint8_t *)cmdbuf);
                switch (cmdid)
                {
                    case AT_SMS_CMD:
                        instructionParser(buf + ret + 1, len - ret - 1, DEBUG_MODE, NULL);
                        break;
                    case AT_DEBUG_CMD:
                        doAtdebugCmd(buf + ret + 1, len - ret - 1);
                        break;
                    case AT_NMEA_CMD:
                        atCmdNmeaParser(buf + ret + 1, len - ret - 1);
                        break;
                    case AT_ZTSN_CMD:
                        atCmdZTSNParser((uint8_t *)buf + ret + 1, len - ret - 1);
                        break;
                    case AT_IMEI_CMD:
                        atCmdIMEIParser();
                        break;
                    case AT_FMPC_NMEA_CMD :
                        atCmdFmpcNmeaParser((uint8_t *)buf + ret + 1, len - ret - 1);
                        break;
                    case AT_FMPC_BAT_CMD :
                        atCmdFmpcBatParser();
                        break;
                    case AT_FMPC_GSENSOR_CMD :
                        atCmdFMPCgsensorParser();
                        break;
                    case AT_FMPC_ACC_CMD :
                        atCmdFmpcAccParser();
                        break;
                    case AT_FMPC_GSM_CMD :
                        atCmdFmpcGsmParser();
                        break;
                    case AT_FMPC_ADCCAL_CMD:
                        atCmdFmpcAdccalParser();
                        break;
                    case AT_FMPC_CSQ_CMD:
                        atCmdFmpcCsqParser();
                        break;
                    case AT_FMPC_IMSI_CMD:
                        atCmdFmpcIMSIParser();
                        break;
                    case AT_FMPC_CHKP_CMD:
                        atCmdFmpcChkpParser();
                        break;
                    case AT_FMPC_CM_CMD:
                        atCmdFmpcCmParser();
                        break;
                    case AT_FMPC_CMGET_CMD:
                        atCmdCmGetParser();
                        break;
                    case AT_FMPC_EXTVOL_CMD:
                        atCmdFmpcExtvolParser();
                        break;
                    case AT_FMPC_LDR_CMD:
						atCmdFMPCLdrParase();
                    	break;
                    case AT_FMPC_TEMP_CMD:
						atCmdFMPCTempParase((uint8_t *)buf + ret + 1, len - ret - 1);
                    	break;
                    default:
                        LogMessage(DEBUG_ALL, "Unknown Cmd");
                        break;
                }
            }
        }
    }
    else
    {
        //createNode(buf, len, 0);
        portUartSend(&usart0_ctl, (uint8_t *)buf, len);
    }
}
