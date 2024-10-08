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
#include "app_zhdprotocol.h"

#define SYS_LED1_ON       LED1_ON
#define SYS_LED1_OFF      LED1_OFF


static SystemLEDInfo sysledinfo;
motionInfo_s motionInfo;
static bleScanTry_s bleTry;
static int8_t wifiTimeOutId = -1;
static centralPoint_s centralPoi;

/**************************************************
@bref		bit0 置位，布防
@param
@return
@note
**************************************************/
void terminalDefense(void)
{
    sysinfo.terminalStatus |= 0x01;
}

/**************************************************
@bref		bit0 清除，撤防
@param
@return
@note
**************************************************/
void terminalDisarm(void)
{
    sysinfo.terminalStatus &= ~0x01;
}
/**************************************************
@bref		获取运动或静止状态
@param
@return
	>0		运动
	0		静止
@note
**************************************************/

uint8_t getTerminalAccState(void)
{
    return (sysinfo.terminalStatus & 0x02);

}

/**************************************************
@bref		bit1 置位，运动，accon
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
@bref		bit1 清除，静止，accoff
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
@bref		bit2 置位，充电
@param
@return
@note
**************************************************/

void terminalCharge(void)
{
    sysinfo.terminalStatus |= 0x04;
}
/**************************************************
@bref		bit2 清除，未充电
@param
@return
@note
**************************************************/

void terminalunCharge(void)
{
    sysinfo.terminalStatus &= ~0x04;
}

/**************************************************
@bref		获取充电状态
@param
@return
	>0		充电
	0		未充电
@note
**************************************************/

uint8_t getTerminalChargeState(void)
{
    return (sysinfo.terminalStatus & 0x04);
}

/**************************************************
@bref		bit 3~5 报警信息
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
@bref		bit6 置位，已定位
@param
@return
@note
**************************************************/

void terminalGPSFixed(void)
{
    sysinfo.terminalStatus |= 0x40;
}

/**************************************************
@bref		bit6 清除，未定位
@param
@return
@note
**************************************************/

void terminalGPSUnFixed(void)
{
    sysinfo.terminalStatus &= ~0x40;
}

/**************************************************
@bref		LED1 运行任务
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
@bref		设置灯的闪烁频率
@param
@return
@note
**************************************************/

static void ledSetPeriod(uint8_t ledtype, uint8_t on_time, uint8_t off_time)
{
    if (ledtype == GPSLED1)
    {
        //系统信号灯
        sysledinfo.sys_led1_on_time = on_time;
        sysledinfo.sys_led1_off_time = off_time;
    }
}

/**************************************************
@bref		更新系统灯状态
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

        //慢闪
        ledSetPeriod(GPSLED1, 10, 10);
        if ((sysinfo.sysLedState & SYSTEM_LED_NETOK) == SYSTEM_LED_NETOK)
        {
            //常亮
            ledSetPeriod(GPSLED1, 1, 9);
            if ((sysinfo.sysLedState & SYSTEM_LED_GPSOK) == SYSTEM_LED_GPSOK)
            {
                //普通灯常亮
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
@bref		灯控任务
@param
@return
@note
**************************************************/

static void ledTask(void)
{
#define LED_TIME_BASE	10
	static uint16_t tick = 60 * LED_TIME_BASE;
	float x;
	if (++tick >= 60 * LED_TIME_BASE)
	{
		NTC_ON;
		if (tick >= 61 * LED_TIME_BASE)
		{
			x = portGetAdcVol(NTC_CHANNEL);
			sysinfo.temprature = readTemp(x) + sysparam.tempcal;
			tick = 0;
			NTC_OFF;
		}
		LogPrintf(DEBUG_ALL, "temptask==>tick:%d, temp:%f", tick, sysinfo.temprature);
		return;
	}
	if (sysparam.ledctrl == 0)
	{
		if (sysinfo.ledTick == 0)
		{
			SYS_LED1_OFF;
			return;
		}
	}
	else
	{
		if (sysinfo.ledTick == 0)
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
@bref		gps开启请求
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
@bref		gps清除请求
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
@bref		gps任务状态机切换
@param
@return
@note
**************************************************/

static void gpsChangeFsmState(uint8_t state)
{
    sysinfo.gpsFsm = state;
}

/**************************************************
@bref		gps数据接收
@param
@return
@note
Qianxun
BKR

**************************************************/

void gpsUartRead(uint8_t *msg, uint16_t len)
{
    static uint8_t gpsRestore[UART_RECV_BUFF_SIZE + 1];
    static uint16_t size = 0;
    uint16_t i, begin;
    uint16_t relen;
    int index;
    if (len + size > UART_RECV_BUFF_SIZE)
    {
        size = 0;
    }
    relen = len > UART_RECV_BUFF_SIZE ? UART_RECV_BUFF_SIZE : len;
    memcpy(gpsRestore + size, msg, relen);
    size += relen;
    begin = 0;
    if (sysinfo.gpstype == GPS_UNDEFINE)
    {
		index = my_getstrindex(msg, "Qianxun", len);
		if (index < 0)
		{
			index = my_getstrindex(msg, "$POLRS", len);
			if (index >= 0)
			{
				sysinfo.gpstype = GPS_SIMPLE;
			}
		}
		else
		{
			sysinfo.gpstype = GPS_QIANXUN;
		}
    }
	if (sysinfo.gpstype != GPS_UNDEFINE)
		LogPrintf(DEBUG_ALL, "Gps type:%d", sysinfo.gpstype);
    
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
@bref       Simple修改GPS输出信息
@param
@return
@note

开启:
GSV:$POLCFGMSG,0,2,1

关闭:
GSV:$POLCFGMSG,0,2,0
VTG:$POLCFGMSG,0,3,0
GST:$POLCFGMSG,0,12,0
GLL:$POLCFGMSG,0,13,0

保存参数配置
$POLCFGSAVE

**************************************************/
static void simpleChangeGpsMsg(void)
{
	char param[256] = { 0 };
	strcpy(param, "$POLCFGMSG,0,2,0\r\n$POLCFGMSG,0,3,0\r\n$POLCFGMSG,0,12,0\r\n$POLCFGMSG,0,13,0\r\n$POLCFGSAVE\r\n");
    portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
}


/**************************************************
@bref       千寻修改GPS输出信息
@param
@return
@note
开启：
RMC:$QXCFGMSG,1,0,1*0E
GGA:$QXCFGMSG,1,2,1*0C
GSA:$QXCFGMSG,1,3,1*0D
GSV:$QXCFGMSG,1,4,1*0A


关闭：
QXDRS:$QXCFGMSG,0,5,0*0B
RMC:$QXCFGMSG,1,0,0*0F
GGA:$QXCFGMSG,1,2,0*0D
GSA:$QXCFGMSG,1,3,0*0C
GSV:$QXCFGMSG,1,4,0*0B

**************************************************/

static void changeGPSMsg(void)
{
    char param[256] = { 0 };
    /*RMC*/
    strcpy(param, "$QXCFGMSG,1,0,1*0E\r\n$QXCFGMSG,1,2,1*0C\r\n$QXCFGMSG,1,3,1*0D\r\n$QXCFGMSG,1,4,0*0B\r\n$QXCFGMSG,0,5,0*0B\r\n");
    portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
    DelayMs(10);
	/*保存配置参数*/
	strcpy(param, "$QXCFGSAVE*4A\r\n");
    portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
    LogMessage(DEBUG_ALL, "qianxun gps config msg RMC GGA GSA");

}

/**************************************************
@bref       千寻修改GPS定位频度
@param
@return
@note
$QXCFGRATE,1000,1*79

**************************************************/

static void changeGPSRate(void)
{
	char param[50];
	if (sysinfo.gpstype == GPS_QIANXUN)
	{
		strcpy(param, "$QXCFGRATE,1000,1*79\r\n");
	    portUartSend(&usart0_ctl, (uint8_t *)param, strlen(param));
	    LogMessage(DEBUG_ALL, "qianxun gps config rate 1Hz");
	    startTimer(5, changeGPSMsg, 0);
	}
    else if (sysinfo.gpstype == GPS_SIMPLE)
    {
    	startTimer(5, simpleChangeGpsMsg, 0);
    	LogMessage(DEBUG_ALL, "simple gps config");
    }
}

/**************************************************
@bref		开启gps
@param
@return
@note
**************************************************/

static void gpsOpen(void)
{
    GPSPWR_ON;
    GPSLNA_ON;
    portUartCfg(APPUSART0, 1, 115200, gpsUartRead);
    startTimer(25, changeGPSRate, 0);
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
@bref		等待gps稳定
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
@bref		关闭gps
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
@bref		保存上一次gps位置
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

/*把最后一个点放入发送缓冲区*/
void getLastFixLocationFromFlash(void)
{
	gpsinfo_s *gpsinfo;
	gpsinfo = getLastFixedGPSInfo();
    tmos_memcpy(gpsinfo, &dynamicParam.gpsinfo, sizeof(gpsinfo_s));
    LogPrintf(DEBUG_ALL, "Get last fix location, Latitude:%f, Longitude:%f", dynamicParam.gpsinfo.latitude, dynamicParam.gpsinfo.longtitude);
}

void saveLastFixLocationToFlash(void)
{
	gpsinfo_s *gpsinfo;
	gpsinfo = getLastFixedGPSInfo();
	if (gpsinfo->fixstatus != 1)
	{
		return;
	}
	tmos_memcpy(&dynamicParam.gpsinfo, gpsinfo, sizeof(gpsinfo_s));
	LogPrintf(DEBUG_ALL, "Save last fix location, Latitude:%f, Longitude:%f", 
				dynamicParam.gpsinfo.latitude, dynamicParam.gpsinfo.longtitude);
	dynamicParamSaveAll();
}

/**************************************************
@bref		生成最后一次定位点
@param
@return
@note
**************************************************/

void centralPointInit(gpsinfo_s *gpsinfo)
{
	centralPoi.init = 1;
	tmos_memcpy(&centralPoi.gpsinfo, gpsinfo, sizeof(gpsinfo_s));
	LogPrintf(DEBUG_ALL, "%s==>lat:%.2f, lon:%.2f", __FUNCTION__,
				centralPoi.gpsinfo.latitude, centralPoi.gpsinfo.longtitude);
}

/**************************************************
@bref		清除最后一次定位点
@param
@return
@note
**************************************************/

void centralPointClear(void)
{
	centralPoi.init = 0;
	tmos_memset(&centralPoi.gpsinfo, 0, sizeof(gpsinfo_s));
	LogPrintf(DEBUG_ALL, "%s==>OK", __FUNCTION__);
}

/**************************************************
@bref		获取最后一次定位点
@param
@return
@note
**************************************************/

void centralPointGet(gpsinfo_s *dest)
{
	tmos_memcpy(dest, &centralPoi.gpsinfo, sizeof(gpsinfo_s));
	LogPrintf(DEBUG_ALL, "%s==>OK", __FUNCTION__);
}

/**************************************************
@bref		修改定位包的时间
@param
@return
@note
把历史已发送成功的定位包修改成现在的时间的定位包
**************************************************/

void updateHistoryGpsTime(gpsinfo_s *gpsinfo)
{
	uint16_t year;
	uint8_t  month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	datetime_s datetimenew;
	portGetRtcDateTime(&year, &month, &day, &hour, &minute, &second);
	gpsinfo->datetime.year   = year % 100;
	gpsinfo->datetime.month  = month;
	gpsinfo->datetime.day    = day;
	gpsinfo->datetime.hour   = hour;
	gpsinfo->datetime.minute = minute;
	gpsinfo->datetime.second = second;
	gpsinfo->hadupload       = 0;
	datetimenew = changeUTCTimeToLocalTime(gpsinfo->datetime, -sysparam.utc);//减回0时区
	gpsinfo->datetime.year   = datetimenew.year % 100;
	gpsinfo->datetime.month  = datetimenew.month;
	gpsinfo->datetime.day    = datetimenew.day;
	gpsinfo->datetime.hour   = datetimenew.hour;
	gpsinfo->datetime.minute = datetimenew.minute;
	gpsinfo->datetime.second = datetimenew.second;
	LogPrintf(DEBUG_ALL, "gpsRequestSet==>Upload last fixed poi [%02d/%02d/%02d-%02d/%02d/%02d]:lat:%.2f  lon:%.2f", 
														    year % 100, 
														    month,
														    day,
														    hour,
														    minute,
														    second,
														    gpsinfo->latitude,
														    gpsinfo->longtitude);
}


/**************************************************
@bref		gps控制任务
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
            //有设备请求开关
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
					saveLastFixLocationToFlash();
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
    /* 只要ACCON就认为是开启GPS， 所以要忽略GPS的开关与否 */
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
@bref		上送一个gps位置
@param
@return
@note
**************************************************/

static void gpsUplodOnePointTask(void)
{
    gpsinfo_s *gpsinfo;
    static uint16_t runtick = 0;	// 如果未定位最多运行180秒
    static uint16_t fixtick = 0;	// 
    //判断是否有请求该事件
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
    if (gpsinfo->fixstatus == 0)
    {
    	runtick++;
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
    /* 关闭高精度/或者高精度服务器未开启 */
    if (sysparam.ntripEn == 0 || 
		sysinfo.ntripRequest == 0 || 
		sysparam.gpsFilterType == GPS_FILTER_CLOSE || 
		sysparam.ntripServer[0] == 0 || 
		sysparam.ntripServerPort == 0)
    {
    	//如果gps不是常开则10s后上报
		if (gpsRequestGet(GPS_REQUEST_ACC_CTL) == 0)
		{
			if (fixtick >= 10)
				goto UPLOAD;
		}
		//如果gps常开则立即上报
		else
			goto UPLOAD;
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
    if (fixtick >= 60)
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
	zhd_protocol_send(ZHD_PROTOCOL_GP, getCurrentGPSInfo());
	gpsRequestClear(GPS_REQUEST_UPLOAD_ONE);
	if (getTerminalAccState() == 0 && centralPoi.init == 0)
    {
		centralPointInit(getCurrentGPSInfo());
    }

}

/**************************************************
@bref		报警上送请求
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
@bref		清除报警上送
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
@bref		报警任务
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
    //感光报警
    if (sysinfo.alarmRequest & ALARM_LIGHT_REQUEST)
    {
        alarmRequestClear(ALARM_LIGHT_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>Light Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_LIGHT);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //低电报警
    if (sysinfo.alarmRequest & ALARM_LOWV_REQUEST)
    {
        alarmRequestClear(ALARM_LOWV_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>LowVoltage Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_LOWV);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //断电报警
    if (sysinfo.alarmRequest & ALARM_LOSTV_REQUEST)
    {
        alarmRequestClear(ALARM_LOSTV_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>lostVoltage Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_LOSTV);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    
    //震动报警
    if (sysinfo.alarmRequest & ALARM_SHUTTLE_REQUEST)
    {
        alarmRequestClear(ALARM_SHUTTLE_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>shuttle Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_SHUTTLE);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //SOS报警
    if (sysinfo.alarmRequest & ALARM_SOS_REQUEST)
    {
        alarmRequestClear(ALARM_SOS_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>SOS Alarm");
        terminalAlarmSet(TERMINAL_WARNNING_SOS);
        alarm = 0;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //急加速报警
    if (sysinfo.alarmRequest & ALARM_ACCLERATE_REQUEST)
    {
        alarmRequestClear(ALARM_ACCLERATE_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>Rapid Accleration Alarm");
        alarm = 9;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }
    //急减速报警
    if (sysinfo.alarmRequest & ALARM_DECELERATE_REQUEST)
    {
        alarmRequestClear(ALARM_DECELERATE_REQUEST);
        LogMessage(DEBUG_ALL,
                   "alarmRequestTask==>Rapid Deceleration Alarm");
        alarm = 10;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //急左转报警
    if (sysinfo.alarmRequest & ALARM_RAPIDLEFT_REQUEST)
    {
        alarmRequestClear(ALARM_RAPIDLEFT_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>Rapid LEFT Alarm");
        alarm = 11;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //急右转报警
    if (sysinfo.alarmRequest & ALARM_RAPIDRIGHT_REQUEST)
    {
        alarmRequestClear(ALARM_RAPIDRIGHT_REQUEST);
        LogMessage(DEBUG_ALL, "alarmRequestTask==>Rapid RIGHT Alarm");
        alarm = 12;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

    //守卫报警
    if (sysinfo.alarmRequest & ALARM_GUARD_REQUEST)
    {
        alarmRequestClear(ALARM_GUARD_REQUEST);
        LogMessage(DEBUG_ALL, "alarmUploadRequest==>Guard Alarm\n");
        alarm = 1;
        protocolSend(NORMAL_LINK, PROTOCOL_16, &alarm);
    }

}



/**************************************************
@bref		更新运动或静止状态
@param
	src 		检测来源
	newState	新状态
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
        if (sysparam.gpsuploadgap != 0 && isWithinSleepTime() <= 0)
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
        centralPointClear();
    }
    else
    {
        if (sysparam.gpsuploadgap != 0 && isWithinSleepTime() <= 0)
        {
            gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
        }
        gpsRequestClear(GPS_REQUEST_ACC_CTL);
        terminalAccoff();
        updateRTCtimeRequest();
    }
    if (primaryServerIsReady())
    {
        protocolInfoResiter(getBatteryLevel(), sysinfo.outsidevoltage > 5.0 ? sysinfo.outsidevoltage : sysinfo.insidevoltage,
                            dynamicParam.startUpCnt, dynamicParam.runTime);
        protocolSend(NORMAL_LINK, PROTOCOL_13, NULL);
        jt808SendToServer(TERMINAL_POSITION, getLastFixedGPSInfo());
        privateHbtTickReset();
    }
}


/**************************************************
@bref       震动中断
@param
@note
**************************************************/

void motionOccur(void)
{
    motionInfo.tapInterrupt++;
}

/**************************************************
@bref       tapCnt 大小
@param
@note
**************************************************/

uint8_t motionGetSize(void)
{
    return sizeof(motionInfo.tapCnt);
}
/**************************************************
@bref		统计每一秒的中断次数
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
@bref		获取这最近n秒的震动次数
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
@bref       检测单位时间内振动频率，判断是否运动
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
	memset(&motionInfo, 0, sizeof(motionInfo_s));
}

/**************************************************
@bref		获取运动状态
@param
@note
**************************************************/

motionState_e motionGetStatus(void)
{
    return motionInfo.motionState;
}


/**************************************************
@bref		运动和静止的判断
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
    //mode2 sleep功能
	if (sysparam.MODE == MODE2)
	{
		if (isWithinSleepTime() > 0)
		{
			if (gpsRequestGet(GPS_REQUEST_ACC_CTL))
				gpsRequestClear(GPS_REQUEST_ACC_CTL);	
		}
		else
		{
			if (getTerminalAccState() && 
				sysparam.gpsuploadgap != 0 && 
				gpsRequestGet(GPS_REQUEST_ACC_CTL) == 0)
			{
				gpsRequestSet(GPS_REQUEST_ACC_CTL);
			}
		}
	}
    //保持运动状态时，如果gap大于Max，则周期性上报gps
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
//    LogPrintf(DEBUG_ALL, "motionCheckOut=%d,%d,%d,%d,%d,%d", totalCnt, sysparam.gsdettime, sysparam.gsValidCnt,
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
        //线永远是第一优先级
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
        //仅由acc线控制
        if (++accOffTick >= 10)
        {
            accOffTick = 0;
            motionStateUpdate(ACC_SRC, MOTION_STATIC);
        }
        return;
    }

    //剩下的，由acc线+gsensor控制

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
@bref		根据Gsensor步数来判断运动还是静止
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
			/* 清除 */
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
		//限制tick加过头
		if (accOffTick <= 20)
		{
			accOffTick++;
		}
		//检测最近2分钟内所走得步数
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
@bref		运动时GPS周期开启任务
@param
@return
@note
**************************************************/

static void accOnGpsUploadTask(void)
{
	static uint32_t autoTick = 0;
	/* 运动参数大于2分钟,GPS间隔开启 */
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

float readTemp(float adcV)
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
@bref		电压检测任务
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
	/* 设备运行的过程中可能会复位模组，也就是给模组断电，这个时候先暂停电压检测，以免误判电压 */
	if (sysinfo.moduleSupplyStatus == 0)
	{
		LogPrintf(DEBUG_ALL, "Temporarily stop voltage detection..");
		return;
	}

    x = portGetAdcVol(ADC_CHANNEL);
    sysinfo.outsidevoltage = x * sysparam.adccal;
    sysinfo.insidevoltage = sysinfo.outsidevoltage;

   	//LogPrintf(DEBUG_ALL, "x:%.2f, outsidevoltage:%.2f bat:[%d]", x, sysinfo.outsidevoltage, getBatteryLevel());
	
	//电池保护
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
    
    //低电报警
    if (sysinfo.outsidevoltage < sysinfo.lowvoltage)
    {
        lowpowertick++;
        if (lowpowertick >= 30)
        {
            if (lowwflag == 0)
            {
                lowwflag = 1;
                LogPrintf(DEBUG_ALL, "power supply too low %.2fV", sysinfo.outsidevoltage);
                //低电报警
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
@bref		模式状态机切换
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
@bref		快速关闭
@param
@return
@note
**************************************************/

static void modeShutDownQuickly(void)
{
    static uint16_t delaytick = 0;
    //存在一种情况是GPS在一开始就定位了，清除了wifi基站的标志位，而4G上线慢导致运行了30秒就关机了，因此要加上primaryServerIsReady判断
    if (sysinfo.gpsRequest == 0 && sysinfo.alarmRequest == 0 && sysinfo.wifiRequest == 0 && sysinfo.lbsRequest == 0 && primaryServerIsReady())
    {
    	//sysparam.gpsuploadgap>=60时，由于gpsrequest==0会关闭kernal,导致一些以1s为时基的任务不计时，会导致gps上报不及时，acc状态无法切换等
    	if ((sysparam.MODE == MODE21 || sysparam.MODE == MODE23) && getTerminalAccState() && sysparam.gpsuploadgap >= GPS_UPLOAD_GAP_MAX)
    	{
			delaytick = 0;
    	}
        delaytick++;
        if (delaytick >= 20)
        {
            LogMessage(DEBUG_ALL, "modeShutDownQuickly==>shutdown");
            delaytick = 0;
            changeModeFsm(MODE_STOP); //执行完毕，关机
        }
    }
    else
    {
        delaytick = 0;
    }
}

/**************************************************
@bref		设备rtc时间是否处于休眠时间
@param
@return
	-1:未开启mode2 休眠时间功能
	 0:已开启mode2休眠时间功能,但未处于休眠时间
	 1:已开启mode2休眠时间功能,已处于休眠时间
@note
**************************************************/

int isWithinSleepTime(void)
{
	uint16_t rtc_mins;
	//rtcUpdateSuccess这个标志也可以充当是否是第一次上电标志
	if (sysinfo.rtcUpdateSuccess == 0)
		return -1;
	if (sysparam.sleep_start == 0 && sysparam.sleep_end == 0)
		return -1;
	if (sysparam.sleep_start == 0xffff || sysparam.sleep_end == 0xffff)
		return -1;
	uint16_t year;
    uint8_t month, date, hour, minute, second;
    //获取rtc时间
    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    rtc_mins  = (hour & 0x1F) * 60;
    rtc_mins += (minute & 0x3f);
    
    LogPrintf(DEBUG_ALL, "%s==>rtc minutes:%.2d:%.2d sleep minutes:%.2d:%.2d wakeup minutes:%.2d:%.2d", 
    					__FUNCTION__, hour, minute, sysparam.sleep_start / 60, sysparam.sleep_start % 60, 
    					sysparam.sleep_end / 60, sysparam.sleep_end % 60);
    //sysparam.sleep_end > sysparam.sleep_start表示休眠和唤醒时间是同一天
    //________~~~~~~~~~~~~~~~~________
    if (sysparam.sleep_end > sysparam.sleep_start)
    {
		if (rtc_mins >= sysparam.sleep_start && rtc_mins < sysparam.sleep_end)
		{
			LogPrintf(DEBUG_ALL, "Within sleep time");
			return 1;
		}
		else
		{
			LogPrintf(DEBUG_ALL, "No within sleep time");
			return 0;
		}
    }
    //~~~~~~~~~~__________~~~~~~~~~~~
    else if (sysparam.sleep_end < sysparam.sleep_start)
    {
		if (rtc_mins < sysparam.sleep_end || rtc_mins >= sysparam.sleep_start)
		{
			LogPrintf(DEBUG_ALL, "Within sleep time");
			return 1;
		}
		else
		{
			LogPrintf(DEBUG_ALL, "No within sleep time");
			return 0;
		}
    }
    else
    {
    	LogPrintf(DEBUG_ALL, "Unkonw error, No within sleep time");
    	return 0;
    }
}

/**************************************************
@bref		mode2 sleep time快速关闭
@param
@return
@note
**************************************************/

static void mode2SleepQuickly(void)
{
	static uint16_t tick = 0;
	if (sysinfo.rtcUpdateSuccess == 0 ||
		(sysparam.sleep_start == 0 && sysparam.sleep_end == 0) ||
		(sysparam.sleep_start == 0xffff || sysparam.sleep_end == 0xffff))
	{
		if (sysinfo.gsensorOnoff == 0)
			portGsensorCtl(1);
		if (sysinfo.gsensorIntFlag == 0)
			portGsensorIntCfg(1);
		return;
	}
	//联网超时计时
	if (primaryServerIsReady() == 0)
		sysinfo.net_timeout_tick++;
	else sysinfo.net_timeout_tick = 0;
	LogPrintf(DEBUG_ALL, "%s==>net_timeout_tick:%d gps_timeout_tick:%d tick:%d", __FUNCTION__,
						sysinfo.net_timeout_tick, sysinfo.gps_timeout_tick, tick);
	
	//联网超时处理
	if (sysinfo.net_timeout_tick >= 210)
	{
		sysinfo.net_timeout_tick = 0;
		tick = 0;
		changeModeFsm(MODE_STOP); //执行完毕，关机
		portGsensorCtl(0);
		LogPrintf(DEBUG_ALL, "%s==>net_timeout,shutdown", __FUNCTION__);
	}
	//查询是否处在休眠时间段
	
	if (isWithinSleepTime() > 0)
	{
		sysinfo.gps_timeout_tick++;
		portGsensorCtl(0);
		//如果处于单次定位
		if (gpsRequestGet(GPS_REQUEST_UPLOAD_ONE))
			tick = 0;
		if (tick++ >= 10 || sysinfo.gps_timeout_tick >= 180)
		{
			tick = 0;
			sysinfo.gps_timeout_tick = 0;
			changeModeFsm(MODE_STOP); //执行完毕，关机
			LogPrintf(DEBUG_ALL, "%s==>job finish,shutdown", __FUNCTION__);
		}
	}
	else 
	{
		sysinfo.gps_timeout_tick = 0;
		if (sysinfo.gsensorOnoff == 0)
			portGsensorCtl(1);
		if (sysinfo.gsensorIntFlag == 0)
			portGsensorIntCfg(1);
	}
}

/**************************************************
@bref		mode4切回在网模式
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
@bref		运行-》关机
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
@bref		待机
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
@bref		启动扫描
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
@bref		蓝牙状态机切换
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
@bref		扫描完成回调
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
@bref		连接完成调
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
@bref		连接完成调
@param
@return
@note
**************************************************/

void bleTryInit(void)
{
	tmos_memset(&bleTry, 0, sizeof(bleScanTry_s));
}

/**************************************************
@bref		模式启动
@param
@return
@note
**************************************************/

static void modeStart(void)
{
	if (sysparam.pwrOnoff == 0)
	{
		LogPrintf(DEBUG_ALL, "System power key was off");
		modeTryToDone();
		return;
	}
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
			portGsensorCtl(0);
            portGsensorIntCfg(0);
            dynamicParam.startUpCnt++;
            dynamicParamSaveAll();
            portSetNextAlarmTime();
            break;
        case MODE2:
			portGsensorCtl(1);
			portGsensorIntCfg(1);
			if (sysparam.accctlgnss == 0)
			{
				gpsRequestSet(GPS_REQUEST_GPSKEEPOPEN_CTL);
			}
			break;
        
//        	if (isWithinSleepTime())
//        	{
//				portGsensorIntCfg(0);
//				modeTryToStop();
//        	}
//        	else
//        	{
//				portGsensorIntCfg(1);
//				if (sysparam.accctlgnss == 0)
//	            {
//	                gpsRequestSet(GPS_REQUEST_GPSKEEPOPEN_CTL);
//	            }
//	           	LogPrintf(DEBUG_ALL, "modeStart==>%02d/%02d/%02d %02d:%02d:%02d", year, month, date, hour, minute, second);
//			    LogPrintf(DEBUG_ALL, "Mode:%d, startup:%d debug:%d %d", sysparam.MODE, dynamicParam.startUpCnt, sysparam.debug, dynamicParam.debug);
//			    lbsRequestSet(DEV_EXTEND_OF_MY);
//			    //wifiRequestSet(DEV_EXTEND_OF_MY);
//			    gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
//			    modulePowerOn();
//			    netResetCsqSearch();
//			    changeModeFsm(MODE_RUNING);
//        	}
//            return;
        case MODE3:
			portGsensorCtl(0);
			portGsensorIntCfg(0);
            dynamicParam.startUpCnt++;
            dynamicParamSaveAll();
            break;
        case MODE21:
        	portGsensorCtl(1);
 			portGsensorIntCfg(1);
            portSetNextAlarmTime();
            break;
        case MODE23:
        	portGsensorCtl(1);
			portGsensorIntCfg(1);
            break;
        /*离线模式*/
        case MODE4:
        	portGsensorCtl(0);
        	portGsensorIntCfg(0);
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
@bref		模式运行
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
            //该模式下工作3分半钟
            if ((sysinfo.sysTick - sysinfo.runStartTick) >= 210)
            {
                gpsRequestClear(GPS_REQUEST_ALL);
                changeModeFsm(MODE_STOP);
            }
            modeShutDownQuickly();
            break;
        case MODE2:
            //该模式下每隔3分钟记录时长
            sysRunTimeCnt();
            gpsUploadPointToServer();
            break;
        case MODE21:
        case MODE23:
            //该模式下无gps请求时，自动关机
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
@bref		模式接收
@param
@return
@note
**************************************************/

static void modeStop(void)
{
    if (sysparam.MODE == MODE1 || sysparam.MODE == MODE3)
    {
        portGsensorIntCfg(0);
    }
    ledStatusUpdate(SYSTEM_LED_RUN, 0);
    modulePowerOff();
    changeModeFsm(MODE_DONE);
}


/**************************************************
@bref		等待唤醒模式
@param
@return
@note
**************************************************/

static void modeDone(void)
{
	static uint8_t motionTick = 0;
	sysinfo.net_timeout_tick = 0;
	sysinfo.gps_timeout_tick = 0;
	//保证关机时不会有gps请求
	if (sysparam.pwrOnoff == 0)
	{
		sysinfo.gpsRequest = 0;
	}
	//进入到这个模式就把sysinfo.canRunFlag置零，以免别的唤醒源让GPS未经电压检测就起来工作
	sysinfo.canRunFlag = 0;
	bleTryInit();
    if (sysinfo.gpsRequest)
    {
        motionTick = 0;
        volCheckRequestSet();
        changeModeFsm(MODE_START);
        LogMessage(DEBUG_ALL, "modeDone==>Change to mode start");
    }
    else if (sysparam.MODE == MODE1 || sysparam.MODE == MODE3 || sysparam.MODE == MODE4)
    {
    	motionTick = 0;
        if (sysinfo.sleep && getModulePwrState() == 0)
        {
            tmos_set_event(sysinfo.taskId, APP_TASK_STOP_EVENT);
        }
    }
    else if (sysparam.MODE == MODE23 || sysparam.MODE == MODE21 || sysparam.MODE == MODE2)
    {
		/*检测gsensor是否有中断进来*/
		//LogPrintf(DEBUG_ALL, "motioncnt:%d, motionTick:%d ", motionCheckOut(sysparam.gsdettime), motionTick);
		if ((sysinfo.gsensorOnoff && motionCheckOut(sysparam.gsdettime) < 1) || 
			 sysinfo.gsensorOnoff == 0)
		{
			if (sysinfo.sleep && getModulePwrState() == 0)
			{
				tmos_set_event(sysinfo.taskId, APP_TASK_STOP_EVENT);
				motionTick = 0;
			}

		}
    }
}

/**************************************************
@bref		当前是否为运行模式
@param
@return
	1		是
	0		否
@note
**************************************************/

uint8_t isModeRun(void)
{
    if (sysinfo.runFsm == MODE_RUNING || sysinfo.runFsm == MODE_START)
        return 1;
    return 0;
}
/**************************************************
@bref		当前是否为done模式
@param
@return
	1		是
	0		否
@note
**************************************************/

uint8_t isModeDone(void)
{
    if (sysinfo.runFsm == MODE_DONE || sysinfo.runFsm == MODE_STOP)
        return 1;
    return 0;
}


/**************************************************
@bref		系统到时自动唤醒
@param
@return
@note
**************************************************/

static void sysAutoReq(void)
{
    uint16_t year;
    uint8_t month, date, hour, minute, second;
    static uint16_t noNetTick = 0;
    if (sysparam.pwrOnoff == 0)
    {
		sysinfo.mode4NoNetTick = 0;
		sysinfo.sysMinutes = 0;
		LogPrintf(DEBUG_ALL, "sysAutoReq==>system power off");
		return;
    }

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
    else if (sysparam.MODE == MODE23 || sysparam.MODE == MODE2)
    {
		//模式2没网络逻辑/或者处于休眠时间段
		if (isModeDone() && sysparam.MODE == MODE2)
		{
			noNetTick++;
			LogPrintf(DEBUG_ALL, "mode2NoNetTick:%d  check net gap:60", noNetTick);
			if (noNetTick >= 60)
			{
				noNetTick = 0;
				LogMessage(DEBUG_ALL, "mode 2 restoration network");
				if (sysinfo.kernalRun == 0)
				{
					volCheckRequestSet();
					tmos_set_event(sysinfo.taskId, APP_TASK_RUN_EVENT);
				}
				changeModeFsm(MODE_START);
			}
		}
		else
		{
			noNetTick = 0;
		}

		if (sysparam.gapMinutes != 0)
		{
			sysinfo.staticUploadTick++;
			if (sysinfo.staticUploadTick % sysparam.gapMinutes == 0)
			{
				sysinfo.staticUploadTick = 0;
				LogMessage(DEBUG_ALL, "static upload period");
	            if (sysinfo.kernalRun == 0)
	            {
	            	volCheckRequestSet();
	                tmos_set_event(sysinfo.taskId, APP_TASK_RUN_EVENT);
	            }
	            if (getTerminalAccState() == 0 && centralPoi.init)
				{
					netRequestSet();
				}
				else
				{
            		gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            	}
	            if (isModeDone())
	            	changeModeFsm(MODE_START);
            }
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
@bref		电池低电关机检测
@param
@return
@note	0：正在检测				1：检测完成
**************************************************/

uint8_t SysBatDetection(void)
{
	static uint8_t waitTick = 0;
	/*开机检测电压*/
	if (sysinfo.volCheckReq == 0)
    {
    	moduleSupplyOn();
        if (sysinfo.canRunFlag)
        {
			waitTick = 0;
			volCheckRequestClear();
			LogMessage(DEBUG_ALL, "电池电压正常，正常开机");
        }
        else
        {
			if (waitTick++ >= 6)
			{
				waitTick = 0;
				changeModeFsm(MODE_DONE);
				volCheckRequestClear();
				LogMessage(DEBUG_ALL, "电池电压偏低，关机");
			}
        }
        return 0;
    }
	/*工作时检测电压*/
	/*不能工作*/
	if (sysinfo.canRunFlag == 0)
	{
		/*如果正在工作*/
		if (sysinfo.runFsm == MODE_RUNING)
		{
			modeTryToStop();
			if (sysparam.MODE == MODE2 || sysparam.MODE == MODE21 || sysparam.MODE == MODE23)
			{
				portGsensorCtl(0);
				
			}
		}
		else if (sysinfo.runFsm == MODE_START)
		{
			modeTryToDone();
		}
	}
	/*可以工作*/
	else
	{
//		if (sysinfo.canRunFlag == 1)
//		{
//			if (sysparam.MODE == MODE2 || sysparam.MODE == MODE21 || sysparam.MODE == MODE23)
//			{
//				if (sysparam.pwrOnoff && sysinfo.gsensorOnoff == 0)
//				{
//					portGsensorCtl(1);
//					portGsensorIntCfg(1);
//				}
//			}
//		}
	}
	return 1;
}

/**************************************************
@bref		电池检查请求设置
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
@bref		电池检查请求清除
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
@bref		模式运行任务
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
@bref		基站上送请求
@param
@return
@note
**************************************************/

void lbsRequestSet(uint8_t ext)
{
    if (sysparam.protocol == ZT_PROTOCOL_TYPE ||
	    sysparam.protocol == JT808_PROTOCOL_TYPE)
	{
		sysinfo.lbsRequest = 1;
		sysinfo.lbsExtendEvt |= ext;
	    LogPrintf(DEBUG_ALL, "%s==>ext:0x%02x", __FUNCTION__, ext);
	}
	else
	{
		LogPrintf(DEBUG_ALL, "%s==>protocol unsupport", __FUNCTION__);
	}
}

/**************************************************
@bref		清除基站上送请求
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
@bref		基站上送任务
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
    else
    {
		sysinfo.lbsExtendEvt = 0;
    }
}

/**************************************************
@bref		wifi超时处理
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
@bref		wifi应答成功
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
@bref		设置WIFI上送请求
@param
@return
@note
**************************************************/

void wifiRequestSet(uint8_t ext)
{
	if (sysparam.protocol == ZT_PROTOCOL_TYPE ||
	    sysparam.protocol == JT808_PROTOCOL_TYPE)
	{
	    sysinfo.wifiRequest = 1;
	    sysinfo.wifiExtendEvt |= ext;
	    LogPrintf(DEBUG_ALL, "%s==>ext:0x%02x", __FUNCTION__, ext);
	}
	else
	{
		LogPrintf(DEBUG_ALL, "%s==>protocol unsupport", __FUNCTION__);
	}
}

/**************************************************
@bref		清除WIFI上送请求
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
@bref		WIFI上送任务
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
    	//有AGPS先等AGPS数据读取完完再发WIFI请求
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
    else
    {
		sysinfo.wifiExtendEvt = 0;
    }
}

/**************************************************
@bref		唤醒设备
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
@bref		查询是否需要休眠
@param
@return
@note
**************************************************/

static uint8_t getWakeUpState(void)
{
    //打印串口信息时，不休眠
    if (sysinfo.logLevel == DEBUG_FACTORY)
    {
        return 1;
    }
    //未联网，不休眠
    if (primaryServerIsReady() == 0 && isModeRun() && sysparam.MODE != MODE4)
    {
        return 2;
    }
    //开gps时，不休眠
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
    //非0 时强制不休眠
    return 0;
}

/**************************************************
@bref		自动休眠
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
@bref		每天重启
@param
@note
**************************************************/

static void rebootEveryDay(void)
{
    sysinfo.sysTick++;
	if (sysinfo.ledTick > 0)
		sysinfo.ledTick--;

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
@bref		计算设备正常状态时间
@param
@return
@note	
如果设备处于暗状态或者处于平放状态，每一分钟计数+1
**************************************************/

void calculateNormalTime(void)
{
	if (LDR_READ)
	{
		/*暗*/
		if (sysinfo.ldrDarkCnt < 10)
		{
			sysinfo.ldrDarkCnt++;
		}
	}
}

/**************************************************
@bref		感光任务
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
		//亮
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
		//暗
		
	}
}

/**************************************************
@bref       gsensor检查任务
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
    if (sysinfo.gsensorOnoff == 0 || sysinfo.gsensorIntFlag == 0)
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

static void uartAutoCloseTask(void)
{
	static uint8_t tick = 0;
	if (sysinfo.logLevel != 0)
	{
		tick = 0;
		return;
	}
	if (usart2_ctl.init == 0)
	{
		tick = 0;
		return;
	}
	if (tick++ >= 30)
	{
		LogMessage(0, "uartAutoCloseTask==>ok");
		portUartCfg(APPUSART2, 0, 115200, NULL);
		portChargeGpioCfg(1);
	}
}

/**************************************************
@bref		关机
@param
@return
@note
**************************************************/

void systemShutdownHandle(void)
{
	LogPrintf(0, "systemShutdownHandle");
	if (sysinfo.runFsm <= MODE_RUNING)
	{
		modeTryToStop();
	}
	else
	{
		modeTryToDone();
	}
	ledStatusUpdate(SYSTEM_LED_RUN, 0);
	sysinfo.gpsRequest    = 0;
	sysinfo.alarmRequest  = 0;
	sysinfo.wifiRequest   = 0;
	sysinfo.lbsRequest    = 0;
	sysinfo.wifiExtendEvt = 0;
	sysinfo.lbsExtendEvt  = 0;
	portLdrGpioCfg(0);
	portGsensorCtl(0);
	portGsensorIntCfg(0);
	sysparam.pwrOnoff = 0;
	if (sysinfo.logLevel == DEBUG_FACTORY)
			sysinfo.logLevel = 0;
	portUartCfg(APPUSART2, 0, 115200, NULL);
	portChargeGpioCfg(1);
	sysinfo.nmeaOutPutCtl = 0;
	sysinfo.mode4First = 0;
	sysinfo.ledTick = 0;
	paramSaveAll();
}

/**************************************************
@bref		1秒任务
@param
@return
@note
**************************************************/

void taskRunInSecond(void)
{
    rebootEveryDay();
    gpsRequestTask();
    motionCheckTask();
	if (sysparam.pwrOnoff)
	{
	    netConnectTask();
	    gsCheckTask();
	    accOnGpsUploadTask();
	    voltageCheckTask();
	    alarmRequestTask();
	    gpsUplodOnePointTask();
	    lbsRequestTask();
	    wifiRequestTask();
	    lightDetectionTask();
	    serverManageTask();
    }
    autoSleepTask();
    sysModeRunTask();
    uartAutoCloseTask();
}


/**************************************************
@bref		串口调试接收
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


/*调试专用任务*/
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
@bref		系统启动时配置
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
	portLdrGpioCfg(1);
    portWdtCfg();
    portUartCfg(APPUSART2, 1, 115200, doDebugRecvPoll);
    portNtcGpioCfg(1);
    bleTryInit();
    socketListInit();
    //portSleepEn();
	zhd_protocol_init();
	getLastFixLocationFromFlash();
    volCheckRequestSet();
    createSystemTask(ledTask, 1);
    createSystemTask(outputNode, 2);
    
    sysinfo.sysTaskId = createSystemTask(taskRunInSecond, 10);
	LogMessage(DEBUG_ALL, ">>>>>>>>>>>>>>>>>>>>>");
    LogPrintf(DEBUG_ALL, "SYS_GetLastResetSta:%x", SYS_GetLastResetSta());
	if (sysparam.pwrOnoff)
	{
		volCheckRequestSet();
		ledStatusUpdate(SYSTEM_LED_RUN, 1);
		sysinfo.ledTick = 300;
	}
	else
	{
		volCheckRequestClear();
		portUartCfg(APPUSART2, 0, 115200, NULL);
		portChargeGpioCfg(1);
		ledStatusUpdate(SYSTEM_LED_RUN, 0);
		sysinfo.ledTick = 0;
	}
}

/**************************************************
@bref		tmos 任务回调
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
        moduleRequestTask();
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
        /*重新配置IO*/
		portModuleGpioCfg(1);
		portGpsGpioCfg(1);
		portLedGpioCfg(1);
		portAdcCfg(1);
		portNtcGpioCfg(1);
		GPSRTC_ON;
        tmos_start_reload_task(sysinfo.taskId, APP_TASK_KERNAL_EVENT, MS1_TO_SYSTEM_TIME(100));
        return events ^ APP_TASK_RUN_EVENT;
    }
    if (events & APP_TASK_STOP_EVENT)
    {
    	portDebugUartCfg(1);
        LogMessage(DEBUG_ALL, "Task kernal stop");
        sysinfo.kernalRun = 0;
        motionClear();
        /*关闭所有IO*/
		portAdcCfg(0);
		portModuleGpioCfg(0);
		portGpsGpioCfg(0);
		portLedGpioCfg(0);
		portNtcGpioCfg(0);
		portWdtCancel();
		GPSRTC_OFF;
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
//    if (events & APP_TASK_TENSEC_EVENT)
//    {
//		accOnOffByGsensorStep();
//		return events ^ APP_TASK_TENSEC_EVENT;
//    }

    return 0;
}

/**************************************************
@bref		任务初始化
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
}

