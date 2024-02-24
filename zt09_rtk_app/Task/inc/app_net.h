#ifndef APP_NET
#define APP_NET
#include <app_protocol.h>
#include <stdint.h>

#include "app_port.h"

#define POWER_ON			PORT_SUPPLY_ON
#define POWER_OFF			PORT_SUPPLY_OFF

#define PWRKEY_HIGH         PORT_PWRKEY_H
#define PWRKEY_LOW          PORT_PWRKEY_L

#define RSTKEY_HIGH         PORT_RSTKEY_H
#define RSTKEY_LOW          PORT_RSTKEY_L
//DTR
#define WAKEMODULE			DTR_LOW
#define SLEEPMODULE			DTR_HIGH


#define GPSSAVEFILE			"gpssave.bat"


#define FILE_READ_SIZE		400


/* tcp���ݻ���ģʽ */
//#define URC_CACHE_MODE
/* tcp������ʾģʽ */
//#define TCP_DATA_SHOW

#define MODULE_RX_BUFF_SIZE		1800

typedef enum
{
    AT_CMD = 1,
    CPIN_CMD,
    CSQ_CMD,
    CGREG_CMD,
    CEREG_CMD,
    QIACT_CMD,
    QIDEACT_CMD,
    QIOPEN_CMD,
    QISTATE_CMD,
    QISEND_CMD,
    CIMI_CMD,
    CGSN_CMD,
    CCID_CMD,
    ICCID_CMD,
    CMGF_CMD,
    CMGR_CMD,
    CMGD_CMD,
    CMGS_CMD,
    CPMS_CMD,
    CNMI_CMD,
    QSCLK_CMD,

    CFUN_CMD,
    QICLOSE_CMD,
    CGDCONT_CMD,
    QCFG_CMD,
    QICFG_CMD,
    QPOWD_CMD,
    QGPS_CMD,
    QGPSEND_CMD,
    QGPSCFG_CMD,
    QGPSGNMEA_CMD,
    QAGPS_CMD,
    QURCCFG_CMD,

    CGATT_CMD,
    QIDNSIP_CMD,
    QIMUX_CMD,
    QISACK_CMD,
    QINDI_CMD,
    QIRD_CMD,
    ATA_CMD,

	//N
	CIPMUX_CMD,
	CIPSTART_CMD,
	CIPCLOSE_CMD,
	CIPSEND_CMD,
	CIPQSEND_CMD,
	CIPRXGET_CMD,

	CIPSTATUS_CMD,

	CFGRI_CMD,
    WIFISCAN_CMD,
    CFG_CMD,
    CIICR_CMD,


    //����
    CGACT_CMD,
    MIPOPEN_CMD,
    MIPCLOSE_CMD,
    MIPSEND_CMD,
    MIPRD_CMD,
    MIPSACK_CMD,
    MADC_CMD,
    MLPMCFG_CMD,
    MLBSCFG_CMD,
    MLBSLOC_CMD,
    CMGL_CMD,
    MWIFISCANSTART_CMD,
    MWIFISCANSTOP_CMD,
    MCHIPINFO_CMD,
    MCFG_CMD,
    AUTHREQ_CMD,
    MIPCALL_CMD,
    MCCID_CMD,
} atCmdType_e;


/*����ϵͳ����״̬*/
typedef enum
{
    AT_STATUS,	  //0
    CPIN_STATUS,
    CSQ_STATUS,
    CGREG_STATUS,
    CONFIG_STATUS,
    QIACT_STATUS,
    NORMAL_STATUS,
    OFFLINE_STATUS,
} moduleStatus_s;

/*ָ���Ӧ�ṹ��*/
typedef struct
{
    atCmdType_e cmd_type;
    char *cmd;
} atCmd_s;

//���Ͷ��нṹ��
typedef struct cmdNode
{
    char *data;
    uint16_t datalen;
    uint8_t currentcmd;
    struct cmdNode *nextnode;
} cmdNode_s;



typedef struct
{
    uint8_t powerState			: 1;
    uint8_t atResponOK			: 1;
    uint8_t cpinResponOk		: 1;
    uint8_t csqOk				: 1;
    uint8_t cgregOK				: 1;
    uint8_t ceregOK				: 1;
    uint8_t qipactSet			: 1;
    uint8_t qipactOk			: 1;
    uint8_t noGpsFile			: 1;

    uint8_t normalLinkQird		 ;
    uint8_t agpsLinkQird		;
    uint8_t bleLinkQird 		;
    uint8_t jt808LinkQird		;
    uint8_t hideLinkQird;

    uint8_t curQirdId;
    uint8_t rdyQirdId;


    uint8_t fsmState;
    uint8_t cmd;
    uint8_t rssi;

    uint8_t IMEI[21];
    uint8_t IMSI[21];
    uint8_t ICCID[21];
    uint8_t messagePhone[20];

    uint8_t gpsUpFsm;
    uint8_t gpsFileHandle;


    uint8_t mnc;
//    uint8_t qgmr[50];
    uint16_t mcc;
    uint16_t lac;

    uint32_t cid;
    uint32_t powerOnTick;
    uint32_t gpsUpTotalSize;
    uint32_t gpsUpHadRead;
    uint32_t tcpTotal;
    uint32_t tcpAck;
    uint32_t tcpNack;
    uint32_t fsmtick;

} moduleState_s;

typedef struct
{
    uint8_t scanMode;
    uint8_t atCount;
    uint8_t csqCount;
    uint8_t cgregCount;
    uint8_t qipactCount;
    uint8_t qiopenCount;
    uint16_t csqTime;
} moduleCtrl_s;


uint8_t createNode(char *data, uint16_t datalen, uint8_t currentcmd);
void outputNode(void);
uint8_t  sendModuleCmd(uint8_t cmd, char *param);

void modulePowerOn(void);
void modulePowerOff(void);
void moduleReset(void);

void openSocket(uint8_t link, char *server, uint16_t port);
void closeSocket(uint8_t link);
void netRequestSet(void);
void netRequestClear(void);

void netConnectTask(void);
void moduleRecvParser(uint8_t *buf, uint16_t bufsize);

void netResetCsqSearch(void);
int socketSendData(uint8_t link, uint8_t *data, uint16_t len);
void moduleSleepCtl(uint8_t onoff);

void moduleGetCsq(void);
void moduleGetLbs(void);
void moduleGetWifiScan(void);
void moduleStopWifiScan(void);

void sendMessage(uint8_t *buf, uint16_t len, char *telnum);
void deleteAllMessage(void);
void deleteMessage(uint8_t index);
void queryMessageList(void);

void querySendData(uint8_t link);
void queryBatVoltage(void);
void queryTemperture(void);

uint8_t isAgpsDataRecvComplete(void);
void changeMode4Callback(void);
uint8_t isModuleOfflineStatus(void);


uint8_t getModuleRssi(void);
uint8_t *getModuleIMSI(void);
uint8_t *getModuleIMEI(void);
uint8_t *getModuleICCID(void);
uint16_t getMCC(void);
uint8_t getMNC(void);
uint16_t getLac(void);
uint32_t getCid(void);
uint32_t getTcpNack(void);

char *getQgmr(void);

uint8_t isModuleRunNormal(void);
uint8_t isModulePowerOnOk(void);
uint8_t isModulePowerOff(void);

void stopCall(void);
void callPhone(char *tel);


void mwifiscaninfoParser(uint8_t *buf, uint16_t len);



#endif

