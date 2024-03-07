#ifndef APP_TASK
#define APP_TASK
#include <stdint.h>
#include "app_central.h"
#define SYSTEM_LED_RUN					0X01
#define SYSTEM_LED_NETOK				0X02
#define SYSTEM_LED_GPSOK				0X04	//��ͨGPS
#define SYSTEM_LED_BLE					0x08


#define GPSLED1							0

#define GPS_REQUEST_UPLOAD_ONE			0X00000001
#define GPS_REQUEST_ACC_CTL				0X00000002
#define GPS_REQUEST_GPSKEEPOPEN_CTL		0X00000004
#define GPS_REQUEST_BLE					0X00000008
#define GPS_REQUEST_DEBUG				0X00000010

#define GPS_REQUEST_ALL					0xFFFFFFFF


#define ALARM_LIGHT_REQUEST				0x0001 //�й�
#define ALARM_LOSTV_REQUEST				0x0002 //�ϵ�
#define ALARM_LOWV_REQUEST				0x0004 //�͵�
#define ALARM_SHUTTLE_REQUEST			0x0008 //�𶯱���
#define ALARM_ACCLERATE_REQUEST			0X0010
#define ALARM_DECELERATE_REQUEST		0X0020
#define ALARM_RAPIDRIGHT_REQUEST		0X0040
#define ALARM_RAPIDLEFT_REQUEST			0X0080
#define ALARM_SOS_REQUEST				0X0100
#define ALARM_GUARD_REQUEST				0x0200
#define ALARM_LEAVE_REQUEST				0x0400
#define ALARM_BLEALARM_REQUEST			0x0800



#define ACCURACY_INIT_STATUS			0
#define ACCURACY_INITWAIT_STATUS		1
#define ACCURACY_RUNNING_STATUS			2

#define ACCURACY_INIT_OK				0
#define ACCURACY_INIT_SDK_ERROR			1
#define ACCURACT_INIT_NET_ERROR			2
#define ACCURACT_INIT_NONE				3

#define APP_TASK_KERNAL_EVENT		    0x0001
#define APP_TASK_POLLUART_EVENT			0x0002
#define APP_TASK_RUN_EVENT				0x0004
#define APP_TASK_STOP_EVENT				0x0008
#define APP_TASK_ONEMINUTE_EVENT		0x0010
#define APP_TASK_TENSEC_EVENT			0x0020

#define UART_RECV_BUFF_SIZE 			1024
#define DEBUG_BUFF_SIZE					256

#define MODE_CHOOSE						0
#define MODE_START						1
#define MODE_RUNING						2
#define MODE_STOP						3
#define MODE_DONE						4

//GPS_UPLOAD_GAP_MAX ���£�gps����������(����GPS_UPLOAD_GAP_MAX),���ڿ���
#define GPS_UPLOAD_GAP_MAX				90

#define ACC_READ		0
#define ACC_STATE_ON	1
#define ACC_STATE_OFF	0


#define DEV_EXTEND_OF_MY	0x01
#define DEV_EXTEND_OF_BLE	0x02

typedef struct
{
    uint32_t sys_tick;		//��¼ϵͳ����ʱ��
  	uint8_t sysLedState;
    
    uint8_t	sys_led1_onoff;
	
    uint8_t sys_led1_on_time;
    uint8_t sys_led1_off_time;
	
	
} SystemLEDInfo;

typedef enum
{
    TERMINAL_WARNNING_NORMAL = 0, /*    0 		*/
    TERMINAL_WARNNING_SHUTTLE,    /*    1���𶯱���       */
    TERMINAL_WARNNING_LOSTV,      /*    2���ϵ籨��       */
    TERMINAL_WARNNING_LOWV,       /*    3���͵籨��       */
    TERMINAL_WARNNING_SOS,        /*    4��SOS���      */
    TERMINAL_WARNNING_CARDOOR,        /*    5���������      */
    TERMINAL_WARNNING_SWITCH,        /*    6������      */
    TERMINAL_WARNNING_LIGHT,      /*    7���йⱨ��       */
} TERMINAL_WARNNING_TYPE;

typedef enum{
	GPSCLOSESTATUS,
	GPSWATISTATUS,
	GPSOPENSTATUS,

}GPSREQUESTFSMSTATUS;

typedef struct
{
	uint8_t initok;
	uint8_t fsmStep;
	uint8_t tick;
}AccuracyStruct;

typedef enum
{
    ACC_SRC,
    VOLTAGE_SRC,
    GSENSOR_SRC,
    SYS_SRC,
} motion_src_e;
typedef enum
{
    MOTION_STATIC = 0,
    MOTION_MOVING,
} motionState_e;


typedef struct
{
    uint8_t ind;
    motionState_e motionState;
    uint8_t tapInterrupt;
    uint8_t tapCnt[10];
} motionInfo_s;


typedef enum
{
    BLE_IDLE,
    BLE_SCAN,
    BLE_SCAN_WAIT,
    BLE_CONN,
    BLE_CONN_WAIT,
    BLE_READY,
    BLE_DONE,
} modeChoose_e;

typedef struct
{
    uint8_t runFsm ;
    uint8_t runTick;
    uint8_t scanCnt;
    uint8_t connCnt;
    uint8_t mac[6];
    uint8_t addrType;
} bleScanTry_s;

extern motionInfo_s motionInfo;

void terminalDefense(void);
void terminalDisarm(void);
uint8_t getTerminalAccState(void);
void terminalAccon(void);
void terminalAccoff(void);
void terminalCharge(void);
void terminalunCharge(void);
uint8_t getTerminalChargeState(void);
void terminalGPSFixed(void);
void terminalGPSUnFixed(void);

void hdGpsGsvCtl(uint8_t onoff);

void ledStatusUpdate(uint8_t status, uint8_t onoff);
void lbsRequestClear(void);

void wifiTimeout(void);
void wifiRspSuccess(void);

void wifiRequestClear(void);
void saveGpsHistory(void);

void gpsRequestSet(uint32_t flag);
void gpsRequestClear(uint32_t flag);
uint32_t gpsRequestGet(uint32_t flag);
void gpsTcpSendRequest(void);

void motionClear(void);

uint8_t gpsInWait(void);

void wakeUpByInt(uint8_t     type,uint8_t sec);

void volCheckRequestSet(void);
void volCheckRequestClear(void);

void alarmRequestSet(uint16_t request);
void alarmRequestClear(uint16_t request);

void lbsRequestSet(uint8_t ext);
void wifiRequestSet(uint8_t ext);

void motionOccur(void);

void modeTryToStop(void);
uint8_t isModeDone(void);
void bleScanCallBack(deviceScanList_s *list);
void bleConnCallBack(void);
void sosRequestSet(void);

void doDebugRecvPoll(uint8_t *msg, uint16_t len);
void gpsUartRead(uint8_t *msg, uint16_t len);
void myTaskPreInit(void);
void myTaskInit(void);
#endif
