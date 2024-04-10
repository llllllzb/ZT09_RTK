#include <app_protocol.h>
#include "app_task.h"
#include "app_mir3da.h"
#include "app_atcmd.h"
#include "app_gps.h"
#include "app_instructioncmd.h"
#include "app_kernal.h"
#include "app_net.h"
#include "app_net.h"
#include "app_param.h"
#include "app_port.h"
#include "app_sys.h"
#include "app_socket.h"
#include "app_server.h"
#include "app_jt808.h"
#include "app_central.h"
#include <math.h>

#define SYS_LED1_ON       LED1_ON
#define SYS_LED1_OFF      LED1_OFF


static SystemLEDInfo sysledinfo;
motionInfo_s motionInfo;
static bleScanTry_s bleTry;
static int8_t wifiTimeOutId = -1;

/**************************************************
@bref		bit0 ��λ������
@param
@return
@note
**************************************************/
void terminalDefense(void)
{
    sysinfo.terminalStatus |= 0x01;
}

/**************************************************
@bref		bit0 ���������
@param
@return
@note
**************************************************/
void terminalDisarm(void)
{
    sysinfo.terminalStatus &= ~0x01;
}
/**************************************************
@bref		��ȡ�˶���ֹ״̬
@param
@return
	>0		�˶�
	0		��ֹ
@note
**************************************************/

uint8_t getTerminalAccState(void)
{
    return (sysinfo.terminalStatus & 0x02);

}

/**************************************************
@bref		bit1 ��λ���˶���accon
@param
@return
@note
**************************************************/

void terminalAccon(void)
{
    sysinfo.terminalStatus |= 0x02;
    jt808UpdateStatus(JT808_STATUS_ACC, 1);
}

/**************************************************
@bref		bit1 �������ֹ��accoff
@param
@return
@note
**************************************************/
void terminalAccoff(void)
{
    sysinfo.terminalStatus &= ~0x02;
    jt808UpdateStatus(JT808_STATUS_ACC, 0);
}

/**************************************************
@bref		bit2 ��λ�����
@param
@return
@note
**************************************************/

void terminalCharge(void)
{
    sysinfo.terminalStatus |= 0x04;
}
/**************************************************
@bref		bit2 �����δ���
@param
@return
@note
**************************************************/

void terminalunCharge(void)
{
    sysinfo.terminalStatus &= ~0x04;
}

/**************************************************
@bref		��ȡ���״̬
@param
@return
	>0		���
	0		δ���
@note
**************************************************/

uint8_t getTerminalChargeState(void)
{
    return (sysinfo.terminalStatus & 0x04);
}

/**************************************************
@bref		bit 3~5 ������Ϣ
@param
@return
@note
**************************************************/

void terminalAlarmSet(TERMINAL_WARNNING_TYPE alarm)
{
    sysinfo.terminalStatus &= ~(0x38);
    sysinfo.terminalStatus |= (alarm << 3);
}

/**************************************************
@bref		bit6 ��λ���Ѷ�λ
@param
@return
@note
**************************************************/

void terminalGPSFixed(void)
{
    sysinfo.terminalStatus |= 0x40;
}

/**************************************************
@bref		bit6 �����δ��λ
@param
@return
@note
**************************************************/

void terminalGPSUnFixed(void)
{
    sysinfo.terminalStatus &= ~0x40;
}

/**************************************************
@bref		LED1 ��������
@param
@return
@note
**************************************************/

static void sysLed1Run(void)
{
    static uint8_t tick = 0;


    if (sysledinfo.sys_led1_on_time == 0)
    {
        SYS_LED1_OFF;
        return;
    }
    else if (sysledinfo.sys_led1_off_time == 0)
    {
        SYS_LED1_ON;
        return;
    }

    tick++;
    if (sysledinfo.sys_led1_onoff == 1) //on status
    {
        SYS_LED1_ON;
        if (tick >= sysledinfo.sys_led1_on_time)
        {
            tick = 0;
            sysledinfo.sys_led1_onoff = 0;
        }
    }
    else   //off status
    {
        SYS_LED1_OFF;
        if (tick >= sysledinfo.sys_led1_off_time)
        {
            tick = 0;
            sysledinfo.sys_led1_onoff = 1;
        }
    }
}

/**************************************************
@bref		���õƵ���˸Ƶ��
@param
@return
@note
**************************************************/

static void ledSetPeriod(uint8_t ledtype, uint8_t on_time, uint8_t off_time)
{
    if (ledtype == GPSLED1)
    {
        //ϵͳ�źŵ�
        sysledinfo.sys_led1_on_time = on_time;
        sysledinfo.sys_led1_off_time = off_time;
    }
}

/**************************************************
@bref		����ϵͳ��״̬
@param
@return
@note
**************************************************/

void ledStatusUpdate(uint8_t status, uint8_t onoff)
{
    if (onoff == 1)
    {
        sysinfo.sysLedState |= status;
    }
    else
    {
        sysinfo.sysLedState &= ~status;
    }
    if ((sysinfo.sysLedState & SYSTEM_LED_RUN) == SYSTEM_LED_RUN)
    {

        //����
        ledSetPeriod(GPSLED1, 10, 10);
        if ((sysinfo.sysLedState & SYSTEM_LED_NETOK) == SYSTEM_LED_NETOK)
        {
            //����
            ledSetPeriod(GPSLED1, 1, 9);
            if ((sysinfo.sysLedState & SYSTEM_LED_GPSOK) == SYSTEM_LED_GPSOK)
            {
                //��ͨ�Ƴ���
                ledSetPeriod(GPSLED1, 1, 0);
            }
        }

    }
    else if ((sysinfo.sysLedState & SYSTEM_LED_BLE) == SYSTEM_LED_BLE)
    {
		ledSetPeriod(GPSLED1, 5, 5);
    }
    else
    {
        SYS_LED1_OFF;
        ledSetPeriod(GPSLED1, 0, 1);
    }
}

/**************************************************
@bref		�ƿ�����
@param
@return
@note
**************************************************/

static void ledTask(void)
{
	if (sysparam.ledctrl == 0)
	{
		if (sysinfo.sysTick >= 300)
		{
			SYS_LED1_OFF;
			return;
		}
	}
	else
	{
		if (sysinfo.sysTick >= 300)
		{
			if (getTerminalAccState() == 0)
			{
				SYS_LED1_OFF;
				return;
			}
		}
	}

    sysLed1Run();
}
/**************************************************
@bref		gps��������
@param
@return
@note
**************************************************/
void gpsRequestSet(uint32_t flag)
{
    LogPrintf(DEBUG_ALL, "gpsRequestSet==>0x%04X", flag);
    sysinfo.gpsRequest |= flag;
}

/**************************************************
@bref		gps�������
@param
@return
@note
**************************************************/

void gpsRequestClear(uint32_t flag)
{
    LogPrintf(DEBUG_ALL, "gpsRequestClear==>0x%04X", flag);
    sysinfo.gpsRequest &= ~flag;
}

uint32_t gpsRequestGet(uint32_t flag)
{
    return sysinfo.gpsRequest & flag;
}

/**************************************************
@bref		gps����״̬���л�
@param
@return
@note
**************************************************/

static void gpsChangeFsmState(uint8_t state)
{
    sysinfo.gpsFsm = state;
}

/**************************************************
@bref		gps���ݽ���
@param
@return
@note
**************************************************/

void gpsUartRead(uint8_t *msg, uint16_t len)
{
    static uint8_t gpsRestore[UART_RECV_BUFF_SIZE + 1];
    static uint16_t size = 0;
    uint16_t i, begin;
    if (len + size > UART_RECV_BUFF_SIZE)
    {
        size = 0;
    }
    memcpy(gpsRestore + size, msg, len);
    size += len;
    begin = 0;
    for (i = 0; i < size; i++)
    {
        if (gpsRestore[i] == '\n')
        {
            if (sysinfo.nmeaOutPutCtl)
            {
                LogWL(DEBUG_GPS, gpsRestore + begin, i - begin);
                LogWL(DEBUG_GPS, "\r\n", 2);
            }
            nmeaParser(gpsRestore + begin, i - begin);
            begin = i + 1;
        }
    }
    if (begin != 0)
    {
        memmove(gpsRestore, gpsRestore + begin, size - begin);
        size -= begin;
    }
}

/**************************************************
@bref       ǧѰ�޸�GPS�����Ϣ
@param
@return
@note
������
RMC:$QXCFGMSG,1,0,1*0E
GGA:$QXCFGMSG,1,2,1*0C
GSA:$QXCFGMSG,1,3,1*0D
GSV:$QXCFGMSG,1,4,1*0A
QXANTSTAT:$QXCFGMSG,0,1,1*0E	//�������ݽ�β


�رգ�
QXDRS:$QXCFGMSG,0,5,0*0B
RMC:$QXCFGMSG,1,0,0*0F
GGA:$QXCFGMSG,1,2,0*0D
GSA:$QXCFGMSG,1,3,0*0C

**************************************************/

static void changeGPSMsg(void)
{
    char param[50];
    /*RMC*/
    strcpy(param, "$QXCFGMSG,1,0,1*0E\r\n");
    portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
    /*GGA*/
    strcpy(param, "$QXCFGMSG,1,2,1*0C\r\n");
    portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
    /*GSA*/
    strcpy(param, "$QXCFGMSG,1,3,1*0D\r\n");
    portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
//	/*GSV*/
//	strcpy(param, "$QXCFGMSG,1,4,1*0A\r\n");
//	portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
    /*QXDRS*/
    strcpy(param, "$QXCFGMSG,0,5,0*0B\r\n");
    portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
    /*QXANTSTAT*/
    strcpy(param, "$QXCFGMSG,0,1,1*0E\r\n");
    portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
    LogMessage(DEBUG_ALL, "gps config msg RMC GGA GSA");

}

/**************************************************
@bref       ǧѰע�븨����λ
@param
@return
@note
$QXCFGRATE,1000,1*79

**************************************************/

/**************************************************
@bref       ǧѰ�޸�GPS��λƵ��
@param
@return
@note
$QXCFGRATE,1000,1*79

**************************************************/

static void changeGPSRate(void)
{
    char param[50];
    strcpy(param, "$QXCFGRATE,1000,1*79\r\n");
    portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
    LogMessage(DEBUG_ALL, "gps config rate 1Hz");
    startTimer(10, changeGPSMsg, 0);
}

/**************************************************
@bref		����gps
@param
@return
@note
**************************************************/

static void gpsOpen(void)
{
    GPSPWR_ON;
    GPSLNA_ON;
    portUartCfg(APPUSART0, 1, 115200, gpsUartRead);
    startTimer(10, changeGPSRate, 0);
    sysinfo.gpsUpdatetick = sysinfo.sysTick;
    sysinfo.gpsOnoff = 1;
    gpsChangeFsmState(GPSWATISTATUS);
    gpsClearCurrentGPSInfo();
    ledStatusUpdate(SYSTEM_LED_GPSOK, 0);
    moduleSleepCtl(0);
    LogMessage(DEBUG_ALL, "gpsOpen");
    initGpsNow();
}
/**************************************************
@bref		�ȴ�gps�ȶ�
@param
@return
@note
**************************************************/

static void gpsWait(void)
{
    static uint8_t runTick = 0;
    static uint8_t first;
    if (++runTick >= 5)
    {
        runTick = 0;
        gpsChangeFsmState(GPSOPENSTATUS);
//		if (gpsRequestOtherGet(GPS_REQUEST_BLE))
//		{
//			if (sysinfo.sysTick - sysinfo.agpsTick >= 7200 || first == 0)
//			{
//				first = 1;
//				sysinfo.agpsTick = sysinfo.sysTick;
//				agpsRequestSet();
//			}
//		}
    }
}

/**************************************************
@bref		�ر�gps
@param
@return
@note
**************************************************/

static void gpsClose(void)
{
    GPSPWR_OFF;
    GPSLNA_OFF;
    portUartCfg(APPUSART0, 0, 115200, NULL);
    sysinfo.rtcUpdate = 0;
    sysinfo.gpsOnoff = 0;
    gpsClearCurrentGPSInfo();
    terminalGPSUnFixed();
    gpsChangeFsmState(GPSCLOSESTATUS);
    ledStatusUpdate(SYSTEM_LED_GPSOK, 0);
    if (primaryServerIsReady())
    {
        moduleSleepCtl(1);
    }
    LogMessage(DEBUG_ALL, "gpsClose");
}

/**************************************************
@bref		������һ��gpsλ��
@param
@return
@note
**************************************************/
void saveGpsHistory(void)
{
	gpsinfo_s *gpsinfo;
    double latitude, longtitude;
    gpsinfo = getLastFixedGPSInfo();
    if (gpsinfo->fixstatus != 0)
    {
        latitude = gpsinfo->latitude;
        longtitude = gpsinfo->longtitude;
        if (gpsinfo->NS == 'S')
        {
            if (latitude > 0)
            {
                latitude *= -1;
            }
        }
        if (gpsinfo->EW == 'W')
        {
            if (longtitude > 0)
            {
                longtitude *= -1;
            }
        }
        dynamicParam.saveLat = latitude;
        dynamicParam.saveLon = longtitude;
        LogPrintf(DEBUG_ALL, "Save Latitude:%lf,Longtitude:%lf", dynamicParam.saveLat, dynamicParam.saveLon);
		dynamicParamSaveAll();
    }
}


/**************************************************
@bref		gps��������
@param
@return
@note
**************************************************/

static void gpsRequestTask(void)
{
    gpsinfo_s *gpsinfo;
    static uint16_t gpsInvalidTick = 0;
	static uint8_t gpsInvalidFlag = 0, gpsInvalidFlagTick = 0;
	uint16_t gpsInvalidparam;

    switch (sysinfo.gpsFsm)
    {
        case GPSCLOSESTATUS:
            //���豸���󿪹�
            if (sysinfo.canRunFlag != 1)
            {
            	break;
            }
            if (sysinfo.gpsRequest != 0)
            {
                gpsOpen();
                ntripRequestSet();
            }
            break;
        case GPSWATISTATUS:
            gpsWait();
            break;
        case GPSOPENSTATUS:
            gpsinfo = getCurrentGPSInfo();
            if (gpsinfo->fixstatus)
            {
                ledStatusUpdate(SYSTEM_LED_GPSOK, 1);
               	lbsRequestClear();
				wifiRequestClear();
            }
            else
            {
                ledStatusUpdate(SYSTEM_LED_GPSOK, 0);    
            }
            if (sysinfo.gpsRequest == 0 || (sysinfo.sysTick - sysinfo.gpsUpdatetick) >= 20)
            {
            	if (sysinfo.gpsRequest == 0)
            	{
					saveGpsHistory();
					agpsRequestClear();
					ntripRequestClear();
            	}
                gpsClose();
            }
            break;
        default:
            gpsChangeFsmState(GPSCLOSESTATUS);
            break;
    }
    /* ֻҪACCON����Ϊ�ǿ���GPS�� ����Ҫ����GPS�Ŀ������ */
//    if (getTerminalAccState() == 0)
//    {
//        gpsInvalidTick = 0;
//        gpsInvalidFlag = 0;
//        gpsInvalidFlagTick = 0;
//        return;
//    }
//    if (sysparam.gpsuploadgap == 0)
//    {
//        gpsInvalidTick = 0;
//        gpsInvalidFlag = 0;
//        gpsInvalidFlagTick = 0;
//        return;
//    }
//    gpsInvalidparam = (sysparam.gpsuploadgap < 60) ? 60 : sysparam.gpsuploadgap;
//    LogPrintf(DEBUG_ALL, "gpsInvalidTick:%d  gpsInvalidparam:%d", gpsInvalidTick, gpsInvalidparam);
//    gpsinfo = getCurrentGPSInfo();
//    if (gpsinfo->fixstatus == 0)
//    {
//        if (++gpsInvalidTick >= gpsInvalidparam)
//        {
//            gpsInvalidTick = 0;
//            gpsInvalidFlag = 1;
//    		wifiRequestSet(DEV_EXTEND_OF_MY);
//        }
//    }
//    else
//    {
//        gpsInvalidTick = 0;
//        gpsInvalidFlag = 0;
//        gpsInvalidFlagTick = 0;
//    }

}


/**************************************************
@bref		����һ��gpsλ��
@param
@return
@note
**************************************************/

static void gpsUplodOnePointTask(void)
{
    gpsinfo_s *gpsinfo;
    static uint16_t runtick = 0;	// ���δ��λ�������180��
    static uint16_t fixtick = 0;	// 
    //�ж��Ƿ���������¼�
    if (gpsRequestGet(GPS_REQUEST_UPLOAD_ONE) == 0)
    {
        runtick = 0;
        fixtick = 0;
        return;
    }
    if (sysinfo.gpsOnoff == 0)
        return;
    gpsinfo = getCurrentGPSInfo();
    LogPrintf(DEBUG_ALL, "gpsUplodOnePointTask==>runtick:%d fixtick:%d", runtick, fixtick);
    runtick++;
    if (gpsinfo->fixstatus == 0)
    {
        //fixtick = 0;
        if (runtick >= 180)
        {
            runtick = 0;
            fixtick = 0;
            LogPrintf(DEBUG_ALL, "gps fix time out");
            gpsRequestClear(GPS_REQUEST_UPLOAD_ONE);
            lbsRequestSet(DEV_EXTEND_OF_MY);

        }
        return;
    }
    //runtick = 0;
    /* �رո߾��� */
    if (sysparam.gpsFilterType == GPS_FILTER_CLOSE)
    {
		if (fixtick >= 10)
		{
			goto UPLOAD;
		}
    }
	/* DIFF */
    else if (sysparam.gpsFilterType == GPS_FILTER_DIFF)
    {
		if (gpsinfo->fixAccuracy == GPS_FIXTYPE_DIFF)
		{
			goto UPLOAD;
		}
    }
    /* FIXHOLD */
    else if (sysparam.gpsFilterType == GPS_FILTER_FIXHOLD)
    {
		if (gpsinfo->fixAccuracy == GPS_FIXTYPE_FIXHOLD)
		{
			goto UPLOAD;
		}
    }
    /* FLOAT */
    else if (sysparam.gpsFilterType == GPS_FILTER_FLOAT)
    {
		if (gpsinfo->fixAccuracy == GPS_FIXTYPE_FLOAT)
		{
			goto UPLOAD;
		}
    }
    /* AUTO */
    else
    {
		if (gpsinfo->fixAccuracy > GPS_FIXTYPE_NORMAL)
		{
			goto UPLOAD;
		}
    }
    ++fixtick;
    if (fixtick >= 90)
	{
		fixtick = 0;
		lbsRequestSet(DEV_EXTEND_OF_MY);
		gpsRequestClear(GPS_REQUEST_UPLOAD_ONE);
		if (sysparam.gpsFilterType == GPS_FILTER_AUTO) goto UPLOAD;
	}
    return;
UPLOAD:
	runtick = 0;
	fixtick = 0;
	if (sysinfo.flag123)
	{
		dorequestSend123();
	}
	protocolSend(NORMAL_LINK, PROTOCOL_12, getCurrentGPSInfo());
	jt808SendToServer(TERMINAL_POSITION, getCurrentGPSInfo());
	gpsRequestClear(GPS_REQUEST_UPLOAD_ONE);

}

/**************************************************
@bref		������������
@param
@return
@note
**************************************************/
void alarmRequestSet(uint16_t request)
{
    LogPrintf(DEBUG_ALL, "alarmRequestSet==>0x%04X", request);
    sysinfo.alarmRequest |= request;
}
/**************************************************
@bref		�����������
@param
@return
@note
**************************************************/

void alarmRequestClear(uint16_t request)
{
    LogPrintf(DEBUG_ALL, "alarmRequestClear==>0x%04X", request);
    sysinfo.alarmRequest &= ~request;
}

/**************************************************
@bref		��������
@param
@return
@note
**************************************************/

void alarmRequestTask(void)
{
    uint8_t alarm;
    if (primaryServerIsReady() == 0 || sysinfo.alarmRequest == 0)
    {
        return;
    }
    if (getTcpNack() != 0)
    {
        return;
    }
    //�йⱨ��
    if (sysinfo.alarmRequest & ALARM_LIGHT_REQUEST)
    {
        alarmRequestClear(ALARM_LIGHT_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>Light Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_LIGHT);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //�͵籨��
    if (sysinfo.alarmRequest & ALARM_LOWV_REQUEST)
    {
        alarmRequestClear(ALARM_LOWV_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>LowVoltage Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_LOWV);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //�ϵ籨��
    if (sysinfo.alarmRequest & ALARM_LOSTV_REQUEST)
    {
        alarmRequestClear(ALARM_LOSTV_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>lostVoltage Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_LOSTV);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    
    //�𶯱���
    if (sysinfo.alarmRequest & ALARM_SHUTTLE_REQUEST)
    {
        alarmRequestClear(ALARM_SHUTTLE_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>shuttle Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_SHUTTLE);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //SOS����
    if (sysinfo.alarmRequest & ALARM_SOS_REQUEST)
    {
        alarmRequestClear(ALARM_SOS_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>SOS Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_SOS);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //�����ٱ���
    if (sysinfo.alarmRequest & ALARM_ACCLERATE_REQUEST)
    {
        alarmRequestClear(ALARM_ACCLERATE_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>Rapid Accleration Alarm");
        alarm = 9;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    //�����ٱ���
    if (sysinfo.alarmRequest & ALARM_DECELERATE_REQUEST)
    {
        alarmRequestClear(ALARM_DECELERATE_REQUEST);
        LogMessage(DEBUG_ALL,
                   "alarmRequestTask==>Rapid Deceleration Alarm");
        alarm = 10;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //����ת����
    if (sysinfo.alarmRequest & ALARM_RAPIDLEFT_REQUEST)
    {
        alarmRequestClear(ALARM_RAPIDLEFT_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>Rapid LEFT Alarm");
        alarm = 11;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //����ת����
    if (sysinfo.alarmRequest & ALARM_RAPIDRIGHT_REQUEST)
    {
        alarmRequestClear(ALARM_RAPIDRIGHT_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>Rapid RIGHT Alarm");
        alarm = 12;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //��������
    if (sysinfo.alarmRequest & ALARM_GUARD_REQUEST)
    {
        alarmRequestClear(ALARM_GUARD_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>Guard Alarm\n");
        alarm = 1;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

}



/**************************************************
@bref		�����˶���ֹ״̬
@param
	src 		�����Դ
	newState	��״̬
@note
**************************************************/

static void motionStateUpdate(motion_src_e src, motionState_e newState)
{
    char type[20];


    if (motionInfo.motionState == newState)
    {
        return;
    }
    motionInfo.motionState = newState;
    switch (src)
    {
        case ACC_SRC:
            strcpy(type, "acc");
            break;
        case VOLTAGE_SRC:
            strcpy(type, "voltage");
            break;
        case GSENSOR_SRC:
            strcpy(type, "gsensor");
            break;
       	case SYS_SRC:
       		strcpy(type, "sys");
            break;
        default:
            return;
            break;
    }
    LogPrintf(DEBUG_ALL, "Device %s , detected by %s", newState == MOTION_MOVING ? "moving" : "static", type);

    if (newState)
    {
        netResetCsqSearch();
        if (sysparam.gpsuploadgap != 0)
        {
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            if (sysparam.gpsuploadgap < GPS_UPLOAD_GAP_MAX)
            {
                gpsRequestSet(GPS_REQUEST_ACC_CTL);
            }
        }
        if (sysparam.bf)
        {
			alarmRequestSet(ALARM_GUARD_REQUEST);
        }
        terminalAccon();
        hiddenServerCloseClear();
    }
    else
    {
        if (sysparam.gpsuploadgap != 0)
        {
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            gpsRequestClear(GPS_REQUEST_ACC_CTL);
        }
        
        terminalAccoff();
        updateRTCtimeRequest();
    }
    if (primaryServerIsReady())
    {
        protocolInfoResiter(getBatteryLevel(), sysinfo.outsidevoltage > 5.0 ? sysinfo.outsidevoltage : sysinfo.insidevoltage,
                            dynamicParam.startUpCnt, dynamicParam.runTime);
        protocolSend(NORMAL_LINK, PROTOCOL_13, NULL);
        jt808SendToServer(TERMINAL_POSITION, getLastFixedGPSInfo());
    }
}


/**************************************************
@bref       ���ж�
@param
@note
**************************************************/

void motionOccur(void)
{
    motionInfo.tapInterrupt++;
}

/**************************************************
@bref       tapCnt ��С
@param
@note
**************************************************/

uint8_t motionGetSize(void)
{
    return sizeof(motionInfo.tapCnt);
}
/**************************************************
@bref		ͳ��ÿһ����жϴ���
@param
@note
**************************************************/

static void motionCalculate(void)
{
    motionInfo.ind = (motionInfo.ind + 1) % sizeof(motionInfo.tapCnt);
    motionInfo.tapCnt[motionInfo.ind] = motionInfo.tapInterrupt;
    motionInfo.tapInterrupt = 0;
}
/**************************************************
@bref		��ȡ�����n����𶯴���
@param
@note
**************************************************/

static uint16_t motionGetTotalCnt(uint8_t n)
{
    uint16_t cnt;
    uint8_t i;
    cnt = 0;
    for (i = 0; i < n; i++)
    {
        cnt += motionInfo.tapCnt[(motionInfo.ind + sizeof(motionInfo.tapCnt) - i) % sizeof(motionInfo.tapCnt)];
    }
    return cnt;
}

/**************************************************
@bref       ��ⵥλʱ������Ƶ�ʣ��ж��Ƿ��˶�
@param
@note
**************************************************/

static uint16_t motionCheckOut(uint8_t sec)
{
    uint8_t i;
    uint16_t validCnt;

    validCnt = 0;
    if (sec == 0 || sec > sizeof(motionInfo.tapCnt))
    {
        return 0;
    }
    for (i = 0; i < sec; i++)
    {
        if (motionInfo.tapCnt[(motionInfo.ind + sizeof(motionInfo.tapCnt) - i) % sizeof(motionInfo.tapCnt)] != 0)
        {
            validCnt++;
        }
    }
    return validCnt;
}

void motionClear(void)
{
	LogMessage(DEBUG_ALL, "motionClear==>OK");
	memset(motionInfo.tapCnt, 0, sizeof(motionInfo.tapCnt));
}

/**************************************************
@bref		��ȡ�˶�״̬
@param
@note
**************************************************/

motionState_e motionGetStatus(void)
{
    return motionInfo.motionState;
}


/**************************************************
@bref		�˶��;�ֹ���ж�
@param
@note
**************************************************/

static void motionCheckTask(void)
{
    static uint16_t gsStaticTick = 0;
    static uint16_t autoTick = 0;
    static uint8_t  accOnTick = 0;
    static uint8_t  accOffTick = 0;
    static uint8_t fixTick = 0;

    static uint8_t  volOnTick = 0;
    static uint8_t  volOffTick = 0;
    static uint8_t bfFlag = 0;
    static uint8_t bfTick = 0;
    static uint8_t lTick = 0, hTick = 0;
    static uint8_t vFlag = 0;
    static uint8_t motionState = 0;
    gpsinfo_s *gpsinfo;

    uint16_t totalCnt, staticTime;

    motionCalculate();

    if (sysparam.MODE == MODE21 || sysparam.MODE == MODE23)
    {
        staticTime = 180;
    }
    else
    {
        staticTime = 180;
    }



    if (sysparam.MODE == MODE1 || sysparam.MODE == MODE3 || sysparam.MODE == MODE4)
    {
        motionStateUpdate(SYS_SRC, MOTION_STATIC);
        gsStaticTick = 0;
        return ;
    }

    //�����˶�״̬ʱ�����gap����Max�����������ϱ�gps
    if (getTerminalAccState() && sysparam.gpsuploadgap >= GPS_UPLOAD_GAP_MAX)
    {
        if (++autoTick >= sysparam.gpsuploadgap)
        {
            autoTick = 0;
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
        }
    }
    else
    {
        autoTick = 0;
    }
    totalCnt = motionCheckOut(sysparam.gsdettime);
//        LogPrintf(DEBUG_ALL, "motionCheckOut=%d,%d,%d,%d,%d,%d", totalCnt, sysparam.gsdettime, sysparam.gsValidCnt,
//                  sysparam.gsInvalidCnt, motionState, autoTick);

    if (totalCnt >= sysparam.gsValidCnt && sysparam.gsValidCnt != 0)
    {
        motionState = 1;
    }
    else if (totalCnt <= sysparam.gsInvalidCnt)
    {
        motionState = 0;
    }

    if (ACC_READ == ACC_STATE_ON)
    {
        //����Զ�ǵ�һ���ȼ�
        if (++accOnTick >= 10)
        {
            accOnTick = 0;
            motionStateUpdate(ACC_SRC, MOTION_MOVING);
        }
        accOffTick = 0;
        return;
    }
    accOnTick = 0;
    if (sysparam.accdetmode == ACCDETMODE0)
    {
        //����acc�߿���
        if (++accOffTick >= 10)
        {
            accOffTick = 0;
            motionStateUpdate(ACC_SRC, MOTION_STATIC);
        }
        return;
    }

    if (sysparam.accdetmode == ACCDETMODE1 || sysparam.accdetmode == ACCDETMODE3)
    {
        //��acc��+��ѹ����
        if (sysinfo.outsidevoltage >= sysparam.accOnVoltage)
        {
            if (++volOnTick >= 5)
            {
                vFlag = 1;
                volOnTick = 0;
                motionStateUpdate(VOLTAGE_SRC, MOTION_MOVING);
            }
        }
        else
        {
            volOnTick = 0;
        }

        if (sysinfo.outsidevoltage < sysparam.accOffVoltage)
        {
            if (++volOffTick >= 15)
            {
                vFlag = 0;
                volOffTick = 0;
                if (sysparam.accdetmode == ACCDETMODE1)
                {
                    motionStateUpdate(MOTION_MOVING, MOTION_STATIC);
                }
            }
        }
        else
        {
            volOffTick = 0;
        }
        if (sysparam.accdetmode == ACCDETMODE1 || vFlag != 0)
        {
            return;
        }
    }
    //ʣ�µģ���acc��+gsensor����

    if (motionState)
    {
        motionStateUpdate(GSENSOR_SRC, MOTION_MOVING);
    }
    if (motionState == 0)
    {
        if (sysinfo.gpsOnoff)
        {
            gpsinfo = getCurrentGPSInfo();
            if (gpsinfo->fixstatus && gpsinfo->speed >= 7)
            {
                if (++fixTick >= 5)
                {
                    gsStaticTick = 0;
                }
            }
            else
            {
                fixTick = 0;
            }
        }
        gsStaticTick++;
        if (gsStaticTick >= staticTime)
        {
            motionStateUpdate(GSENSOR_SRC, MOTION_STATIC);
        }
    }
    else
    {
        gsStaticTick = 0;
    }
}


/**************************************************
@bref		����Gsensor�������ж��˶����Ǿ�ֹ
@param
@return
@note
**************************************************/

void accOnOffByGsensorStep(void)
{
#define ACC_ON_DETECT_TIME			6
#define ACC_OFF_DETECT_TIME			12
	static uint8_t accOnStep[ACC_ON_DETECT_TIME]   = { 0 };
	static uint8_t accOffStep[ACC_OFF_DETECT_TIME] = { 0 };
	static uint8_t acc_on_step_ind  = 0;
	static uint8_t acc_off_step_ind = 0;

	uint16_t stepgap = 0;
	uint8_t i = 0;
	uint16_t totalStep = 0;
	static uint8_t accOffTick = 0;
	if (sysinfo.gsensorOnoff == 0)
	{
		stepgap = 0;
		accOffTick = 0;
		return;
	}
	if (sysparam.MODE != MODE2)
	{
		stepgap = 0;
		accOffTick = 0;
		terminalAccoff();
		if (gpsRequestGet(GPS_REQUEST_ACC_CTL))
        {
            gpsRequestClear(GPS_REQUEST_ACC_CTL);
        }
		return;
	}
	portUpdateStep();
	stepgap = abs(sysinfo.step - sysinfo.accStep);
	if (getTerminalAccState() == 0)
	{	
		LogPrintf(DEBUG_ALL, "Acc state: OFF");
		accOnStep[acc_on_step_ind] = stepgap;
		acc_on_step_ind = (acc_on_step_ind + 1) % ACC_ON_DETECT_TIME;
		for (i = 0; i < ACC_ON_DETECT_TIME; i++)
		{
			totalStep += accOnStep[i];
		}
		if (totalStep >= sysparam.motionstep)
		{
			/* ��� */
			acc_off_step_ind = 0;
			memset(accOffStep, 0, sizeof(accOffStep));
			accOffTick = 0;
			terminalAccon();
			hiddenServerCloseClear();
			LogMessage(DEBUG_ALL, "Acc on detected by step");
			if (sysparam.gpsuploadgap != 0)
			{
				gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
				if (sysparam.gpsuploadgap < GPS_UPLOAD_GAP_MAX)
	            {
	                gpsRequestSet(GPS_REQUEST_ACC_CTL);
	            }
			}
			if (primaryServerIsReady())
		    {
		        protocolInfoResiter(getBatteryLevel(), sysinfo.outsidevoltage > 5.0 ? sysinfo.outsidevoltage : sysinfo.insidevoltage,
		                            dynamicParam.startUpCnt, dynamicParam.runTime);
		        protocolSend(NORMAL_LINK, PROTOCOL_13, NULL);
		        jt808SendToServer(TERMINAL_POSITION, getLastFixedGPSInfo());
		    }
		    if (sysparam.bf)
	        {
				alarmRequestSet(ALARM_GUARD_REQUEST);
	        }
		}
	}
	else
	{
		LogPrintf(DEBUG_ALL, "Acc state: ON");
		accOffStep[acc_off_step_ind] = stepgap;
		acc_off_step_ind = (acc_off_step_ind + 1) % ACC_OFF_DETECT_TIME;
//		for (i = 0; i < ACC_OFF_DETECT_TIME; i++)
//		{
//			totalStep += accOffStep[i];
//			LogPrintf(DEBUG_ALL, "[accOffStep:%d]", accOffStep[i]);
//		}
		//����tick�ӹ�ͷ
		if (accOffTick <= 20)
		{
			accOffTick++;
		}
		//������2���������ߵò���
		if (totalStep < sysparam.staticStep && accOffTick >= ACC_OFF_DETECT_TIME)	//120s
		{
			accOffTick = 0;
			acc_on_step_ind = 0;
			memset(accOnStep, 0, sizeof(accOnStep));
			terminalAccoff();
			LogMessage(DEBUG_ALL, "Acc off detected by step");
			if (sysparam.gpsuploadgap != 0)
	        {
	            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
	            gpsRequestClear(GPS_REQUEST_ACC_CTL);
	        }
			if (primaryServerIsReady())
		    {
		        protocolInfoResiter(getBatteryLevel(), sysinfo.outsidevoltage > 5.0 ? sysinfo.outsidevoltage : sysinfo.insidevoltage,
		                            dynamicParam.startUpCnt, dynamicParam.runTime);
		        protocolSend(NORMAL_LINK, PROTOCOL_13, NULL);
		        jt808SendToServer(TERMINAL_POSITION, getLastFixedGPSInfo());
		    }
		}
	}
	sysinfo.accStep = sysinfo.step;

	LogPrintf(DEBUG_ALL, "%s==>stepgap:%d, totalStep:%d, accOffTick:%d", __FUNCTION__, stepgap, totalStep, accOffTick);
}

/**************************************************
@bref		�˶�ʱGPS���ڿ�������
@param
@return
@note
**************************************************/

static void accOnGpsUploadTask(void)
{
	static uint32_t autoTick = 0;
	/* �˶���������2����,GPS������� */
    if (getTerminalAccState() && sysparam.gpsuploadgap >= GPS_UPLOAD_GAP_MAX)
    {
        if (++autoTick >= sysparam.gpsuploadgap)
        {
            autoTick = 0;
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
        }
    }
    else
    {
        autoTick = 0;
    }
}

const float Rp = 100000.0; //100K
const float T2 = (273.15 + 25.0);
const float B = 3950.0;
const float K = 273.15;

static float readTemp(float adcV)
{
	float v;
	float Rt;
	v = adcV;
	Rt = (100000 * v) / (2.8 - v);
	v = (1 / (log(Rt / Rp) / B + (1 / T2))) - 273.15 + 0.5;
	return v;
}

float getTemp(void)
{
	float x;
	NTC_ON;
	DelayMs(100);
	x = portGetAdcVol(NTC_CHANNEL);
	NTC_OFF;
	return readTemp(x);
}


/**************************************************
@bref		��ѹ�������
@param
@return
@note
**************************************************/

static void voltageCheckTask(void)
{
    static uint16_t lowpowertick = 0;
    static uint8_t  lowwflag = 0;
    float x;
    static uint8_t protectTick = 0;
    uint8_t ret = 0;
    if (sysinfo.adcOnoff == 0)
	{
		return;
	}

    x = portGetAdcVol(ADC_CHANNEL);
    sysinfo.outsidevoltage = x * sysparam.adccal;
    sysinfo.insidevoltage = sysinfo.outsidevoltage;

   	//LogPrintf(DEBUG_ALL, "x:%.2f, outsidevoltage:%.2f bat:[%d]", x, sysinfo.outsidevoltage, getBatteryLevel());
	
	//��ر���
    if (sysinfo.outsidevoltage < 2.9 && sysinfo.canRunFlag == 1)
    {
		protectTick++;
		if (protectTick >= 5)
		{
			protectTick = 0;
			sysinfo.canRunFlag = 0;
			portDebugUartCfg(1);
			LogPrintf(DEBUG_ALL, "Batvoltage is lowwer than %.2f", sysinfo.outsidevoltage);
			portDebugUartCfg(0);
		}
    }
	else if (sysinfo.outsidevoltage >= 3.0 && sysinfo.canRunFlag == 0)
	{
		protectTick++;
		if (protectTick >= 5)
		{
			protectTick = 0;
			sysinfo.canRunFlag = 1;
			portDebugUartCfg(1);
			LogPrintf(DEBUG_ALL, "Batvoltage is more than %.2f", sysinfo.outsidevoltage);
			portDebugUartCfg(0); 
		}
		
	}
	else
	{
		protectTick = 0;
	}
    
    //�͵籨��
    if (sysinfo.outsidevoltage < sysinfo.lowvoltage)
    {
        lowpowertick++;
        if (lowpowertick >= 30)
        {
            if (lowwflag == 0)
            {
                lowwflag = 1;
                LogPrintf(DEBUG_ALL, "power supply too low %.2fV", sysinfo.outsidevoltage);
                //�͵籨��
                jt808UpdateAlarm(JT808_LOWVOLTAE_ALARM, 1);
                alarmRequestSet(ALARM_LOWV_REQUEST);
                lbsRequestSet(DEV_EXTEND_OF_MY);
                gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            }

        }
    }
    else
    {
        lowpowertick = 0;
    }


    if (sysinfo.outsidevoltage >= sysinfo.lowvoltage + 0.5)
    {
        lowwflag = 0;
        jt808UpdateAlarm(JT808_LOWVOLTAE_ALARM, 0);
    }


}

/**************************************************
@bref		ģʽ״̬���л�
@param
@return
@note
**************************************************/

static void changeModeFsm(uint8_t fsm)
{
    sysinfo.runFsm = fsm;
    LogPrintf(DEBUG_ALL, "changeModeFsm==>%d", fsm);
}

/**************************************************
@bref		���ٹر�
@param
@return
@note
**************************************************/

static void modeShutDownQuickly(void)
{
    static uint16_t delaytick = 0;
    //����һ�������GPS��һ��ʼ�Ͷ�λ�ˣ������wifi��վ�ı�־λ����4G����������������30��͹ػ��ˣ����Ҫ����primaryServerIsReady�ж�
    if (sysinfo.gpsRequest == 0 && sysinfo.alarmRequest == 0 && sysinfo.wifiRequest == 0 && sysinfo.lbsRequest == 0 && primaryServerIsReady())
    {
    	//sysparam.gpsuploadgap>=60ʱ������gpsrequest==0��ر�kernal,����һЩ��1sΪʱ�������񲻼�ʱ���ᵼ��gps�ϱ�����ʱ��acc״̬�޷��л���
    	if ((sysparam.MODE == MODE21 || sysparam.MODE == MODE23) && getTerminalAccState() && sysparam.gpsuploadgap >= GPS_UPLOAD_GAP_MAX)
    	{
			delaytick = 0;
    	}
        delaytick++;
        if (delaytick >= 20)
        {
            LogMessage(DEBUG_ALL, "modeShutDownQuickly==>shutdown");
            delaytick = 0;
            changeModeFsm(MODE_STOP); //ִ����ϣ��ػ�
        }
    }
    else
    {
        delaytick = 0;
    }
}

/**************************************************
@bref		mode4�л�����ģʽ
@param
@return
@note
**************************************************/

static void mode4CloseSocketQuickly(void)
{
	static uint16_t tick = 0;
	
	if (isModuleRunNormal())
	{
		if (sysinfo.gpsRequest == 0 && sysinfo.alarmRequest == 0 && sysinfo.wifiExtendEvt == 0 && sysinfo.lbsRequest == 0 && sysinfo.netRequest == 0)
		{
			tick++;
			if (tick >= 15)
			{
				LogMessage(DEBUG_ALL, "mode4CloseSocketQuickly==>On net");
				changeMode4Callback();
				tick = 0;
			}
		}
		else
		{
			tick = 0;
		}
		sysinfo.nonetTick = 0;
	}
	else if (isModuleRunNormal() == 0 && isModuleOfflineStatus() == 0)
	{
		sysinfo.nonetTick++;
		tick = 0;
		LogPrintf(DEBUG_ALL, "sysinfo.nonetTick:%d", sysinfo.nonetTick);
		if (sysinfo.nonetTick >= 270)
		{
			sysinfo.nonetTick = 0;
			LogMessage(DEBUG_ALL, "mode4CloseSocketQuickly==>Shut down");
			modeTryToStop();
		}
	}
	else
	{
		sysinfo.nonetTick = 0;
		tick = 0;
	}
}

/**************************************************
@bref		����-���ػ�
@param
@return
@note
**************************************************/

void modeTryToStop(void)
{
    sysinfo.gpsRequest = 0;
    sysinfo.alarmRequest = 0;
    sysinfo.wifiRequest = 0;
    sysinfo.lbsRequest = 0;
    netRequestClear();
    changeModeFsm(MODE_STOP);
    LogMessage(DEBUG_ALL, "modeTryToStop");
}

/**************************************************
@bref		����
@param
@return
@note
**************************************************/

void modeTryToDone(void)
{
	sysinfo.gpsRequest = 0;
    sysinfo.alarmRequest = 0;
    sysinfo.wifiRequest = 0;
    sysinfo.lbsRequest = 0;
    netRequestClear();
    changeModeFsm(MODE_DONE);
    LogMessage(DEBUG_ALL, "modeTryToDone");
}


/**************************************************
@bref		����ɨ��
@param
@return
@note
**************************************************/

static void modeScan(void)
{
//    static uint8_t runTick = 0;
//    scanList_s  *list;
//    if (sysparam.leavealm == 0 || (sysparam.MODE != MODE1 && sysparam.MODE != MODE3))
//    {
//        runTick = 0;
//        changeModeFsm(MODE_START);
//        return;
//    }
//    if (runTick == 1)
//    {
//        portFsclkChange(1);
//        bleCentralDiscovery();
//    }
//    else if (runTick >= 20)
//    {
//        runTick = 0;
//        list = scanListGet();
//        if (list->cnt == 0)
//        {
//            alarmRequestSet(ALARM_LEAVE_REQUEST);
//        }
//        changeModeFsm(MODE_START);
//    }
//    runTick++;
}


/**************************************************
@bref		����״̬���л�
@param
@return
@note
**************************************************/

static void bleChangeFsm(modeChoose_e fsm)
{
    bleTry.runFsm = fsm;
    bleTry.runTick = 0;
}

/**************************************************
@bref		ɨ����ɻص�
@param
@return
@note
**************************************************/

void bleScanCallBack(deviceScanList_s *list)
{
    uint8_t i;
    for (i = 0; i < list->cnt; i++)
    {
        if (my_strpach(list->list[i].broadcaseName, "AUTO"))
        {
            LogPrintf(DEBUG_ALL, "Find Ble [%s],rssi:%d", list->list[i].broadcaseName, list->list[i].rssi);
            tmos_memcpy(bleTry.mac, list->list[i].addr, 6);
            bleTry.addrType = list->list[i].addrType;
            bleChangeFsm(BLE_CONN);
            return;
        }
    }
    LogMessage(DEBUG_ALL, "no find my ble");
    bleChangeFsm(BLE_SCAN);
}

/**************************************************
@bref		������ɵ�
@param
@return
@note
**************************************************/

void bleConnCallBack(void)
{
    LogMessage(DEBUG_ALL, "connect success");
    bleChangeFsm(BLE_READY);
    dynamicParam.bleLinkCnt = 0;
    dynamicParamSaveAll();
    tmos_set_event(appCentralTaskId, APP_WRITEDATA_EVENT);
}

/**************************************************
@bref		������ɵ�
@param
@return
@note
**************************************************/

void bleTryInit(void)
{
	tmos_memset(&bleTry, 0, sizeof(bleScanTry_s));
}

/**************************************************
@bref		ģʽѡ�񣬽�ģʽһ�����£������������ܲ�������
@param
@return
@note
**************************************************/

static void modeChoose(void)
{

	bleChangeFsm(BLE_IDLE);
    changeModeFsm(MODE_START);
//	static uint8_t flag = 0;
//
//    if (sysparam.MODE != MODE1 && sysparam.MODE != MODE3)
//    {
//        bleChangeFsm(BLE_IDLE);
//        changeModeFsm(MODE_START);
//        return;
//    }
//    if (sysinfo.alarmRequest != 0)
//    {
//        bleChangeFsm(BLE_IDLE);
//        changeModeFsm(MODE_START);
//        return;
//    }
//    if (sysinfo.first == 0)
//    {
//		sysinfo.first = 1;
//		bleChangeFsm(BLE_IDLE);
//        changeModeFsm(MODE_START);
//        return;
//    }
//    if (sysparam.bleLinkFailCnt == 0)
//    {
//		bleChangeFsm(BLE_IDLE);
//        changeModeFsm(MODE_START);
////        dynamicParam.bleLinkCnt = 0;
////        dynamicParamSaveAll();
//        return;
//    }
//    if (flag == 0)
//    {
//    	flag = 1;
//		portFsclkChange(1);
//    }
//    switch (bleTry.runFsm)
//    {
//        case BLE_IDLE:
//            dynamicParam.startUpCnt++;
//            dynamicParam.bleLinkCnt++;
//            dynamicParamSaveAll();
//            wakeUpByInt(2, 30);
//            ledStatusUpdate(SYSTEM_LED_BLE, 1);
//             
//            portSetNextAlarmTime();
//            bleChangeFsm(BLE_SCAN);
//            bleTry.scanCnt = 0;
//            
//            break;
//        case BLE_SCAN:
//            bleTry.connCnt = 0;
//            if (bleTry.scanCnt++ < 3)
//            {
//                //����ɨ��
//                centralStartDisc();
//                bleChangeFsm(BLE_SCAN_WAIT);
//            }
//            else
//            {
//                bleTry.scanCnt = 0;
//                //ɨ��ʧ��
//                if (dynamicParam.bleLinkCnt >= sysparam.bleLinkFailCnt)
//                {
//                    LogPrintf(DEBUG_ALL, "scan fail==>%d", dynamicParam.bleLinkCnt);
//                    alarmRequestSet(ALARM_BLEALARM_REQUEST);
//                    dynamicParam.bleLinkCnt = 0;
//                    dynamicParamSaveAll();
//                    changeModeFsm(MODE_START);
//                    bleChangeFsm(BLE_IDLE);
//                    flag = 0;
//                }
//                else
//                {
//                    LogPrintf(DEBUG_ALL, "scan cnt==>%d", dynamicParam.bleLinkCnt);
//                    bleChangeFsm(BLE_DONE);
//                }
//
//            }
//            break;
//        case BLE_SCAN_WAIT:
//            //�ȴ�ɨ����
//            if (++bleTry.runTick >= 12)
//            {
//                bleChangeFsm(BLE_SCAN);
//            }
//            break;
//        case BLE_CONN:
//            //��ʼ��������
//            if (bleTry.connCnt++ < 3)
//            {
//                centralEstablish(bleTry.mac, bleTry.addrType);
//                bleChangeFsm(BLE_CONN_WAIT);
//            }
//            else
//            {
//                bleTry.connCnt = 0;
//                if (dynamicParam.bleLinkCnt >= sysparam.bleLinkFailCnt)
//                {
//                    LogPrintf(DEBUG_ALL, "conn fail==>%d", dynamicParam.bleLinkCnt);
//                    dynamicParam.bleLinkCnt = 0;
//                    alarmRequestSet(ALARM_BLEALARM_REQUEST);
//                    dynamicParamSaveAll();
//                    changeModeFsm(MODE_START);
//                    bleChangeFsm(BLE_IDLE);
//                    flag = 0;
//                }
//                else
//                {
//                    bleChangeFsm(BLE_DONE);
//                }
//            }
//            break;
//        case BLE_CONN_WAIT:
//            //�ȴ����ӽ��
//            if (++bleTry.runTick >= 12)
//            {
//                centralTerminate();
//                bleChangeFsm(BLE_CONN);
//            }
//            break;
//        case BLE_READY:
//            //�������ӳɹ�
//            if (++bleTry.runTick >= 20)
//            {
//                centralTerminate();
//                bleChangeFsm(BLE_DONE);
//            }
//            break;
//        case BLE_DONE:
//            POWER_OFF;
//            ledStatusUpdate(SYSTEM_LED_BLE, 0);
//            bleChangeFsm(BLE_IDLE);
//            changeModeFsm(MODE_DONE);
//            gpsRequestClear(GPS_REQUEST_ALL);
//            flag = 0;
//            break;
//        default:
//            bleChangeFsm(BLE_IDLE);
//            break;
//    }
}




/**************************************************
@bref		ģʽ����
@param
@return
@note
**************************************************/

static void modeStart(void)
{
    uint16_t year;
    uint8_t month, date, hour, minute, second;
    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    
    sysinfo.runStartTick = sysinfo.sysTick;
    sysinfo.gpsuploadonepositiontime = 180;
    updateRTCtimeRequest();
    portFsclkChange(0);
    if (sysinfo.mode4First == 0)
    {
		sysinfo.mode4First = 1;
		lbsRequestSet(DEV_EXTEND_OF_MY);
		//wifiRequestSet(DEV_EXTEND_OF_MY);
    	gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
    	netRequestSet();
    }
    sysinfo.nonetTick = 0;
    switch (sysparam.MODE)
    {
        case MODE1:
            
            dynamicParam.startUpCnt++;
            dynamicParamSaveAll();
            portSetNextAlarmTime();
            break;
        case MODE2:

            if (sysparam.accctlgnss == 0)
            {
                gpsRequestSet(GPS_REQUEST_GPSKEEPOPEN_CTL);
            }
            break;
        case MODE3:

            dynamicParam.startUpCnt++;
            dynamicParamSaveAll();
            break;
        case MODE21:
 
            portSetNextAlarmTime();
            break;
        case MODE23:

            break;
        /*����ģʽ*/
        case MODE4:
		    modulePowerOn();
		    netResetCsqSearch();
		    portSetNextAlarmTime();
		    portSetNextMode4AlarmTime();
		    changeModeFsm(MODE_RUNING);
        	return;
        default:
            sysparam.MODE = MODE2;
            paramSaveAll();
            break;
    }
    LogPrintf(DEBUG_ALL, "modeStart==>%02d/%02d/%02d %02d:%02d:%02d", year, month, date, hour, minute, second);
    LogPrintf(DEBUG_ALL, "Mode:%d, startup:%d debug:%d %d", sysparam.MODE, dynamicParam.startUpCnt, sysparam.debug, dynamicParam.debug);
    lbsRequestSet(DEV_EXTEND_OF_MY);
    //wifiRequestSet(DEV_EXTEND_OF_MY);
    gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
    modulePowerOn();
    netResetCsqSearch();
    changeModeFsm(MODE_RUNING);
}

static void sysRunTimeCnt(void)
{
    static uint8_t runTick = 0;
    if (++runTick >= 180)
    {
        runTick = 0;
        dynamicParam.runTime++;
        portSaveStep();
    }
}


/**************************************************
@bref		ģʽ����
@param
@return
@note
**************************************************/

static void modeRun(void)
{
    static uint8_t runtick = 0;
    switch (sysparam.MODE)
    {
        case MODE1:
        case MODE3:
            //��ģʽ�¹���3�ְ���
            if ((sysinfo.sysTick - sysinfo.runStartTick) >= 210)
            {
                gpsRequestClear(GPS_REQUEST_ALL);
                changeModeFsm(MODE_STOP);
            }
            modeShutDownQuickly();
            break;
        case MODE2:
            //��ģʽ��ÿ��3���Ӽ�¼ʱ��
            sysRunTimeCnt();
            gpsUploadPointToServer();
            break;
        case MODE21:
        case MODE23:
            //��ģʽ����gps����ʱ���Զ��ػ�
            sysRunTimeCnt();
            modeShutDownQuickly();
            //gpsUploadPointToServer();
            break;
        case MODE4:
			sysRunTimeCnt();
			mode4CloseSocketQuickly();
        	break;
        default:
            LogMessage(DEBUG_ALL, "mode change unknow");
            sysparam.MODE = MODE2;
            break;
    }
}

/**************************************************
@bref		ģʽ����
@param
@return
@note
**************************************************/

static void modeStop(void)
{
//    if (sysparam.MODE == MODE1 || sysparam.MODE == MODE3)
//    {
//        portGsensorCtl(0);
//    }
    ledStatusUpdate(SYSTEM_LED_RUN, 0);
    modulePowerOff();
    changeModeFsm(MODE_DONE);
}


/**************************************************
@bref		�ȴ�����ģʽ
@param
@return
@note
**************************************************/

static void modeDone(void)
{
	static uint8_t motionTick = 0;
	//���뵽���ģʽ�Ͱ�sysinfo.canRunFlag���㣬�����Ļ���Դ��GPSδ����ѹ������������
	sysinfo.canRunFlag = 0;
	bleTryInit();
    if (sysinfo.gpsRequest)
    {
        motionTick = 0;
        volCheckRequestSet();
        if (sysparam.MODE == MODE1 || sysparam.MODE == MODE3)
        {
            changeModeFsm(MODE_CHOOSE);
        }
        else
        {
            changeModeFsm(MODE_START);
        }
        LogMessage(DEBUG_ALL, "modeDone==>Change to mode start");
    }
    else if (sysparam.MODE == MODE1 || sysparam.MODE == MODE3 || sysparam.MODE == MODE4)
    {
    	motionTick = 0;
        if (sysinfo.sleep && isModulePowerOff())
        {
            tmos_set_event(sysinfo.taskId, APP_TASK_STOP_EVENT);
        }
    }
    else if (sysparam.MODE == MODE23 || sysparam.MODE == MODE21 || sysparam.MODE == MODE2)
    {
		/*���gsensor�Ƿ����жϽ���*/
		//LogPrintf(DEBUG_ALL, "motioncnt:%d, motionTick:%d ", motionCheckOut(sysparam.gsdettime), motionTick);
		if (motionCheckOut(sysparam.gsdettime) <= 1)
		{
			if (sysinfo.sleep)
			{
				tmos_set_event(sysinfo.taskId, APP_TASK_STOP_EVENT);
				motionTick = 0;
			}

		}
//		/*�ߵ�ƽ���˶�*/
//		if (GSINT_DET)
//		{
//			motionTick = 0;
//		}
//		/*�͵�ƽ����ֹ*/
//		else
//		{
//			if (motionTick++ >= 5)
//			{
//				if (sysinfo.sleep)
//				{
//					tmos_set_event(sysinfo.taskId, APP_TASK_STOP_EVENT);
//					motionTick = 0;
//				}
//			}
//		}
    }
}

/**************************************************
@bref		��ǰ�Ƿ�Ϊ����ģʽ
@param
@return
	1		��
	0		��
@note
**************************************************/

uint8_t isModeRun(void)
{
    if (sysinfo.runFsm == MODE_RUNING || sysinfo.runFsm == MODE_START)
        return 1;
    return 0;
}
/**************************************************
@bref		��ǰ�Ƿ�Ϊdoneģʽ
@param
@return
	1		��
	0		��
@note
**************************************************/

uint8_t isModeDone(void)
{
    if (sysinfo.runFsm == MODE_DONE || sysinfo.runFsm == MODE_STOP)
        return 1;
    return 0;
}


/**************************************************
@bref		ϵͳ��ʱ�Զ�����
@param
@return
@note
**************************************************/

static void sysAutoReq(void)
{
    uint16_t year;
    uint8_t month, date, hour, minute, second;

    if (sysparam.MODE == MODE1 || sysparam.MODE == MODE21)
    {
        portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
        if (date == sysinfo.alarmDate && hour == sysinfo.alarmHour && minute == sysinfo.alarmMinute)
        {
            LogPrintf(DEBUG_ALL, "sysAutoReq==>%02d/%02d/%02d %02d:%02d:%02d", year, month, date, hour, minute, second);
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            if (sysinfo.kernalRun == 0)
            {
            	volCheckRequestSet();
                tmos_set_event(sysinfo.taskId, APP_TASK_RUN_EVENT);
            }
        }
    }
    else if (sysparam.MODE == MODE4)
    {
    	portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
        if (hour == sysinfo.mode4alarmHour && minute == sysinfo.mode4alarmMinute)
        {
            LogPrintf(DEBUG_ALL, "sysAutoReq==>MODE4:%02d/%02d/%02d %02d:%02d:%02d", year, month, date, hour, minute, second);
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            netRequestSet();
            if (sysinfo.kernalRun == 0)
            {
            	volCheckRequestSet();
                tmos_set_event(sysinfo.taskId, APP_TASK_RUN_EVENT);
            }
        }
		if (isModeDone())
		{
			sysinfo.mode4NoNetTick++;
			LogPrintf(DEBUG_ALL, "mode4NoNetTick:%d", sysinfo.mode4NoNetTick);
			if (sysinfo.mode4NoNetTick >= 5)
			{
				sysinfo.mode4NoNetTick = 0;
                LogMessage(DEBUG_ALL, "mode 4 restoration network");
                if (sysinfo.kernalRun == 0)
                {
                	volCheckRequestSet();
                    tmos_set_event(sysinfo.taskId, APP_TASK_RUN_EVENT);
                    changeModeFsm(MODE_START);
                }
			}
		}
		else
		{
			sysinfo.mode4NoNetTick = 0;
		}
    }
    else
    {
        if (sysparam.gapMinutes != 0)
        {
            sysinfo.sysMinutes++;
            if (sysinfo.sysMinutes % sysparam.gapMinutes == 0)
            {
            	sysinfo.sysMinutes = 0;
				gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
                LogMessage(DEBUG_ALL, "upload period");
                if (sysinfo.kernalRun == 0)
                {
                	volCheckRequestSet();
                    tmos_set_event(sysinfo.taskId, APP_TASK_RUN_EVENT);
                }
            }
        }
    }
}

/**************************************************
@bref		��ص͵�ػ����
@param
@return
@note	0�����ڼ��				1��������
**************************************************/

uint8_t SysBatDetection(void)
{
	static uint8_t waitTick = 0;
	/*��������ѹ*/
	if (sysinfo.volCheckReq == 0)
    {
        if (sysinfo.canRunFlag)
        {
			waitTick = 0;
			volCheckRequestClear();
			LogMessage(DEBUG_ALL, "��ص�ѹ��������������");
        }
        else
        {
			if (waitTick++ >= 6)
			{
				waitTick = 0;
				changeModeFsm(MODE_DONE);
				volCheckRequestClear();
				LogMessage(DEBUG_ALL, "��ص�ѹƫ�ͣ��ػ�");
			}
        }
        return 0;
    }
	/*����ʱ����ѹ*/
	/*���ܹ���*/
	if (sysinfo.canRunFlag == 0)
	{
		/*������ڹ���*/
		if (sysinfo.runFsm == MODE_RUNING)
		{
			modeTryToStop();
			if (sysparam.MODE == MODE2 || sysparam.MODE == MODE21 || sysparam.MODE == MODE23)
			{
				portGsensorCtl(0);
			}
		}
		else if (sysinfo.runFsm == MODE_START || sysinfo.runFsm == MODE_CHOOSE)
		{
			modeTryToDone();
		}
	}
	/*���Թ���*/
	else
	{
		if (sysinfo.canRunFlag == 1)
		{
			if (sysparam.MODE == MODE2 || sysparam.MODE == MODE21 || sysparam.MODE == MODE23)
			{
				portGsensorCtl(1);
			}
		}
	}
	return 1;
}

/**************************************************
@bref		��ؼ����������
@param
@return
@note
**************************************************/

void volCheckRequestSet(void)
{
	sysinfo.volCheckReq = 0;
	sysinfo.canRunFlag = 0;
	LogMessage(DEBUG_ALL, "volCheckRequestSet==>OK");
}

/**************************************************
@bref		��ؼ���������
@param
@return
@note
**************************************************/

void volCheckRequestClear(void)
{
	sysinfo.volCheckReq = 1;
	LogMessage(DEBUG_ALL, "volCheckRequestClear==>OK");
}

/**************************************************
@bref		ģʽ��������
@param
@return
@note
**************************************************/

static void sysModeRunTask(void)
{
	if (SysBatDetection() != 1)
	{
		return;
	}
    switch (sysinfo.runFsm)
    {
        case MODE_CHOOSE:
            modeChoose();
            break;
        case MODE_START:
            modeStart();
            break;
        case MODE_RUNING:
            modeRun();
            break;
        case MODE_STOP:
            modeStop();
            break;
        case MODE_DONE:
            modeDone();
            break;
    }
}

/**************************************************
@bref		��վ��������
@param
@return
@note
**************************************************/

void lbsRequestSet(uint8_t ext)
{
    sysinfo.lbsRequest = 1;
    sysinfo.lbsExtendEvt |= ext;
}

/**************************************************
@bref		�����վ��������
@param
@return
@note
**************************************************/

void lbsRequestClear(void)
{
	sysinfo.lbsRequest = 0;
    sysinfo.lbsExtendEvt = 0;
}

static void sendLbs(void)
{
    if (sysinfo.lbsExtendEvt & DEV_EXTEND_OF_MY)
    {
        protocolSend(NORMAL_LINK, PROTOCOL_19, NULL);
    }

    sysinfo.lbsExtendEvt = 0;
}
/**************************************************
@bref		��վ��������
@param
@return
@note
**************************************************/

static void lbsRequestTask(void)
{
    if (sysinfo.lbsRequest == 0)
    {
        return;
    }
    if (primaryServerIsReady() == 0)
        return;
    sysinfo.lbsRequest = 0;
    if (sysparam.protocol == ZT_PROTOCOL_TYPE)
    {
        moduleGetLbs();
        startTimer(20, sendLbs, 0);
    }
}

/**************************************************
@bref		wifi��ʱ����
@param
@return
@note
**************************************************/

void wifiTimeout(void)
{
	LogMessage(DEBUG_ALL, "wifiTimeout");
	lbsRequestSet(DEV_EXTEND_OF_MY);
	wifiRequestClear();
	wifiTimeOutId = -1;
}

/**************************************************
@bref		wifiӦ��ɹ�
@param
@return
@note
**************************************************/

void wifiRspSuccess(void)
{
	LogMessage(DEBUG_ALL, "wifiRspSuccess");
	sysinfo.lockTick = 125;
	if (wifiTimeOutId != -1)
	{
		stopTimer(wifiTimeOutId);
		wifiTimeOutId = -1;
	}
}

/**************************************************
@bref		����WIFI��������
@param
@return
@note
**************************************************/

void wifiRequestSet(uint8_t ext)
{
    sysinfo.wifiRequest = 1;
    sysinfo.wifiExtendEvt |= ext;
}

/**************************************************
@bref		���WIFI��������
@param
@return
@note
**************************************************/

void wifiRequestClear(void)
{
	sysinfo.wifiRequest = 0;
	sysinfo.wifiExtendEvt = 0;
}


/**************************************************
@bref		WIFI��������
@param
@return
@note
**************************************************/

static void wifiRequestTask(void)
{
    if (sysinfo.wifiRequest == 0)
    {
        return;
    }
    if (primaryServerIsReady() == 0)
        return;

    sysinfo.wifiRequest = 0;
    if (sysparam.protocol == ZT_PROTOCOL_TYPE)
    {
    	//��AGPS�ȵ�AGPS���ݶ�ȡ�����ٷ�WIFI����
    	if (sysinfo.agpsRequest)
    	{
        	startTimer(70, moduleGetWifiScan, 0);
        }
        else
        {
			startTimer(30, moduleGetWifiScan, 0);
        }
        //wifiTimeOutId = startTimer(300, wifiTimeout, 0);
    }
}

/**************************************************
@bref		�����豸
@param
@return
@note
**************************************************/
void wakeUpByInt(uint8_t      type, uint8_t sec)
{
    switch (type)
    {
        case 0:
            sysinfo.ringWakeUpTick = sec;
            break;
        case 1:
            sysinfo.cmdTick = sec;
            break;
        case 2:
        	sysinfo.irqTick = sec;
        	break;
    }

    portSleepDn();
}

/**************************************************
@bref		��ѯ�Ƿ���Ҫ����
@param
@return
@note
**************************************************/

static uint8_t getWakeUpState(void)
{
    //��ӡ������Ϣʱ��������
    if (sysinfo.logLevel == DEBUG_FACTORY)
    {
        return 1;
    }
    //δ������������
    if (primaryServerIsReady() == 0 && isModeRun() && sysparam.MODE != MODE4)
    {
        return 2;
    }
    //��gpsʱ��������
    if (sysinfo.gpsRequest != 0)
    {
        return 3;
    }
    if (sysinfo.ringWakeUpTick != 0)
    {
        return 4;
    }
    if (sysinfo.cmdTick != 0)
    {
        return 5;
    }
    if (sysinfo.irqTick != 0)
    {
		return 6;
    }
    if (sysparam.MODE == MODE4 && isModeRun() && isModuleOfflineStatus() == 0)
    {
    	return 7;
    }
    //��0 ʱǿ�Ʋ�����
    return 0;
}

/**************************************************
@bref		�Զ�����
@param
@return
@note
**************************************************/

void autoSleepTask(void)
{
    static uint8_t flag = 0;
    if (sysinfo.ringWakeUpTick != 0)
    {
        sysinfo.ringWakeUpTick--;
    }
    if (sysinfo.cmdTick != 0)
    {
        sysinfo.cmdTick--;
    }
    if (sysinfo.irqTick != 0)
    {
		sysinfo.irqTick--;
    }
    
    if (getWakeUpState())
    {
        portSleepDn();
        if (flag != 0)
        {
            flag = 0;
            portFsclkChange(0);
            LogMessage(DEBUG_ALL, "disable sleep");
            tmos_start_reload_task(sysinfo.taskId, APP_TASK_POLLUART_EVENT, MS1_TO_SYSTEM_TIME(50));
            sysinfo.sleep = 0;
            //portDebugUartCfg(1);
        }
    }
    else
    {
        portSleepEn();
        if (flag != 1)
        {
            flag = 1;
            portFsclkChange(1);
            LogMessage(DEBUG_ALL, "enable sleep");
            tmos_stop_task(sysinfo.taskId, APP_TASK_POLLUART_EVENT);
            sysinfo.sleep = 1;
            //portDebugUartCfg(0);
        }
    }
}

/**************************************************
@bref		ÿ������
@param
@note
**************************************************/

static void rebootEveryDay(void)
{
    sysinfo.sysTick++;
    //    if (sysinfo.sysTick < 86400)
    //        return ;
    //    if (sysinfo.gpsRequest != 0)
    //        return ;
    //    portSysReset();
    uint16 year = 0;
    uint8  month = 0, date = 0, hour = 0, minute = 0, second = 0;
    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    if (sysinfo.rtcUpdate == 1 && hour == 0 && minute == 0 && second == 0)
    {
		portClearStep();
    }
}



/**************************************************
@bref		�����豸����״̬ʱ��
@param
@return
@note	
����豸���ڰ�״̬���ߴ���ƽ��״̬��ÿһ���Ӽ���+1
**************************************************/

void calculateNormalTime(void)
{
	if (LDR_READ)
	{
		/*��*/
		if (sysinfo.ldrDarkCnt < 10)
		{
			sysinfo.ldrDarkCnt++;
		}
	}
}

/**************************************************
@bref		�й�����
@param
@return
@note
**************************************************/
static void lightDetectionTask(void)
{
	if (sysparam.ldrEn == 0)
		return;
	//LogPrintf(DEBUG_ALL, "ldr:%d", LDR_READ);
	if (sysinfo.ldrIrqFlag)
	{
		//��
		if (sysinfo.ldrDarkCnt >= 2)
		{
			LogMessage(DEBUG_ALL, "Light alarm");
			alarmRequestSet(ALARM_LIGHT_REQUEST);
			gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
			jt808UpdateAlarm(JT808_LIGHT_ALARM, 1);
			lbsRequestSet(DEV_EXTEND_OF_MY);
		}
		else
		{
			LogMessage(DEBUG_ALL, "Light alarm cancel");
		}
		sysinfo.ldrIrqFlag = 0;
		sysinfo.ldrDarkCnt = 0;
	}
	else
	{
		//��
		
	}
}

/**************************************************
@bref       gsensor�������
@param
@note
**************************************************/
static void gsensorRepair(void)
{
    portGsensorCtl(1);
    LogMessage(DEBUG_ALL, "repair gsensor");
}

static void gsCheckTask(void)
{
    static uint8_t tick = 0;
    static uint8_t errorcount = 0;
    if (sysinfo.gsensorOnoff == 0)
    {
        tick = 0;
        return;
    }

    tick++;
    if (tick % 60 == 0)
    {
        tick = 0;
        if (readInterruptConfig() != 0)
        {
            LogMessage(DEBUG_ALL, "gsensor error");
            portGsensorCtl(0);
            startTimer(20, gsensorRepair, 0);

        }
        else
        {
            errorcount = 0;
        }
    }
}


/**************************************************
@bref		1������
@param
@return
@note
**************************************************/

void taskRunInSecond(void)
{
    rebootEveryDay();
    netConnectTask();
    //motionCheckTask();
    accOnGpsUploadTask();
    gsCheckTask();
    gpsRequestTask();
    voltageCheckTask();
    alarmRequestTask();
    gpsUplodOnePointTask();
    lbsRequestTask();
    wifiRequestTask();
    lightDetectionTask();
    autoSleepTask();
    sysModeRunTask();
    serverManageTask();
	
}


/**************************************************
@bref		���ڵ��Խ���
@param
@return
@note
**************************************************/
void doDebugRecvPoll(uint8_t *msg, uint16_t len)
{
    static uint8_t gpsRestore[DEBUG_BUFF_SIZE + 1];
    static uint16_t size = 0;
    uint16_t i, begin;
    if (len + size > DEBUG_BUFF_SIZE)
    {
        size = 0;
    }
    memcpy(gpsRestore + size, msg, len);
    size += len;
    begin = 0;
    for (i = 0; i < size; i++)
    {
        if (gpsRestore[i] == '\n')
        {
            atCmdParserFunction(gpsRestore + begin, i - begin + 1);
            begin = i + 1;
        }
    }
    if (begin != 0)
    {
        memmove(gpsRestore, gpsRestore + begin, size - begin);
        size -= begin;
    }
}


/*����ר������*/
void debugtask(void)
{
    static uint8_t fsm;
    static uint8_t tick;
    switch(fsm)
    {
    case 0:
        modulePowerOn();
        //portGsensorCtl(1);
        fsm = 1;
        break;

    case 1:
        tick++;
        if (tick>=5)
        {
            tick = 0;
        	fsm = 3;
        	modulePowerOff();
//        	moduleSleepCtl(1);
        }
//        else
//        {
//			netConnectTask();
//			serverManageTask();
//        }   
        break;
    case 2:
    	LogMessage(DEBUG_ALL, "SHUT DOWN");
        tmos_stop_task(sysinfo.taskId, APP_TASK_POLLUART_EVENT);
        tmos_set_event(sysinfo.taskId, APP_TASK_STOP_EVENT);
		portDebugUartCfg(0);
        portSleepEn();
        fsm = 3;
        break;
    case 3:
        tick++;
        if (tick>=5)
        {
            modulePowerOff();
            tick = 0;
            fsm = 4;
            tmos_stop_task(sysinfo.taskId, APP_TASK_POLLUART_EVENT);
            tmos_set_event(sysinfo.taskId, APP_TASK_STOP_EVENT);
            portSleepEn();
        }
        break;

    }
}

/**************************************************
@bref		ϵͳ����ʱ����
@param
@return
@note
**************************************************/

void myTaskPreInit(void)
{
    tmos_memset(&sysinfo, 0, sizeof(sysinfo));
    paramInit();
    sysinfo.logLevel = 9;
    SetSysClock(CLK_SOURCE_HSE_16MHz);
    portGpioSetDefCfg();
    portModuleGpioCfg(1);
    portGpsGpioCfg(1);
    portLedGpioCfg(1);
    portAdcCfg(1);
	portMicGpioCfg();
	portLdrGpioCfg(1);
    portWdtCfg();
    portDebugUartCfg(1);
    portNtcGpioCfg(1);
    bleTryInit();
    socketListInit();
    //portSleepEn();
	ledStatusUpdate(SYSTEM_LED_RUN, 1);
	portGsensorCtl(1);
	
    volCheckRequestSet();
    createSystemTask(ledTask, 1);
    createSystemTask(outputNode, 2);
    sysinfo.sysTaskId = createSystemTask(taskRunInSecond, 10);
	LogMessage(DEBUG_ALL, ">>>>>>>>>>>>>>>>>>>>>");
    LogPrintf(DEBUG_ALL, "SYS_GetLastResetSta:%x", SYS_GetLastResetSta());

}

/**************************************************
@bref		tmos ����ص�
@param
@return
@note
**************************************************/

static tmosEvents myTaskEventProcess(tmosTaskID taskID, tmosEvents events)
{
	uint8_t ret;
    uint16_t year = 0;
    uint8_t  month = 0, date = 0, hour = 0, minute = 0, second = 0;
    if (events & SYS_EVENT_MSG)
    {
        uint8 *pMsg;
        if ((pMsg = tmos_msg_receive(sysinfo.taskId)) != NULL)
        {
            tmos_msg_deallocate(pMsg);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & APP_TASK_KERNAL_EVENT)
    {
        kernalRun();
        portWdtFeed();
        return events ^ APP_TASK_KERNAL_EVENT;
    }

    if (events & APP_TASK_POLLUART_EVENT)
    {
        pollUartData();
        portWdtFeed();
        return events ^ APP_TASK_POLLUART_EVENT;
    }

    if (events & APP_TASK_RUN_EVENT)
    {
    	portDebugUartCfg(1);
        LogMessage(DEBUG_ALL, "Task kernal start");
    
        sysinfo.kernalRun = 1;
		portWdtCfg();
        /*��������IO*/
		portModuleGpioCfg(1);
		portGpsGpioCfg(1);
		portLedGpioCfg(1);
		portAdcCfg(1);
		portMicGpioCfg();
		POWER_ON;
		portNtcGpioCfg(1);
        tmos_start_reload_task(sysinfo.taskId, APP_TASK_KERNAL_EVENT, MS1_TO_SYSTEM_TIME(100));
        return events ^ APP_TASK_RUN_EVENT;
    }
    if (events & APP_TASK_STOP_EVENT)
    {
    	portDebugUartCfg(1);
        LogMessage(DEBUG_ALL, "Task kernal stop");
        sysinfo.kernalRun = 0;
        motionClear();
        /*�ر�����IO*/
		portAdcCfg(0);
		portModuleGpioCfg(0);
		portGpsGpioCfg(0);
		portLedGpioCfg(0);
		portMicGpioCfg();
		POWER_OFF;
		portNtcGpioCfg(0);
		portWdtCancel();
       	tmos_stop_task(sysinfo.taskId, APP_TASK_KERNAL_EVENT);
        portDebugUartCfg(0);
        return events ^ APP_TASK_STOP_EVENT;
    }
    if (events & APP_TASK_ONEMINUTE_EVENT)
    {
    	sysinfo.oneMinTick++;
   	 	portDebugUartCfg(1);
    	sysAutoReq();
    	calculateNormalTime();
        LogMessage(DEBUG_ALL, "***************************Task one minutes**********************");
        LogPrintf(DEBUG_ALL,  "*Mode: %d, rungap: %d, System run: %d min, darktime: %d, oneminute: %d*", sysparam.MODE, sysparam.gapMinutes, sysinfo.sysMinutes, sysinfo.ldrDarkCnt, sysinfo.oneMinTick);
        portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
        if (hour == 0 && minute == 0)
        {
            LogPrintf(DEBUG_BLE, "time of zero");
            portClearStep();
        }
        LogMessage(DEBUG_ALL, "*****************************************************************");
        portDebugUartCfg(0);
        return events ^ APP_TASK_ONEMINUTE_EVENT;
    }
    if (events & APP_TASK_TENSEC_EVENT)
    {
		accOnOffByGsensorStep();
		return events ^ APP_TASK_TENSEC_EVENT;
    }

    return 0;
}

/**************************************************
@bref		�����ʼ��
@param
@return
@note
**************************************************/

void myTaskInit(void)
{
    sysinfo.taskId = TMOS_ProcessEventRegister(myTaskEventProcess);
    tmos_set_event(sysinfo.taskId, APP_TASK_RUN_EVENT);
    tmos_start_reload_task(sysinfo.taskId, APP_TASK_POLLUART_EVENT, MS1_TO_SYSTEM_TIME(50));
    tmos_start_reload_task(sysinfo.taskId, APP_TASK_ONEMINUTE_EVENT, MS1_TO_SYSTEM_TIME(60000));
	tmos_start_reload_task(sysinfo.taskId, APP_TASK_TENSEC_EVENT, MS1_TO_SYSTEM_TIME(10000));

}

