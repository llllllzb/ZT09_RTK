#ifndef APP_JT808
#define APP_JT808

#include "CH58x_common.h"
#include "app_protocol.h"
#include "app_gps.h"

#define JT808_LOWVOLTAE_ALARM		(1<<7)//�͵籨��
#define JT808_LOSTVOLTAGE_ALARM		(1<<8)//�ϵ籨��
#define JT808_LIGHT_ALARM			(1<<15)//�йⱨ��
#define JT808_TILT_ALARM			(1<<16)//��б����


#define JT808_STATUS_ACC			(1<<0)//acc on
#define JT808_STATUS_FIX			(1<<1)//gps ��λ
#define JT808_STATUS_LATITUDE		(1<<2)//��γ
#define JT808_STATUS_LONGTITUDE		(1<<3)//����
#define JT808_STATUS_MOVE			(1<<4)//��Ӫ״̬
#define JT808_STATUS_OIL			(1<<10)//��·�Ͽ�
#define JT808_STATUS_ELECTRICITY	(1<<11)//��·�Ͽ�
#define JT808_STATUS_USE_GPS		(1<<18)//gps��λ
#define JT808_STATUS_USE_BEIDOU		(1<<19)//������λ
#define JT808_STATUS_USE_GLONASS	(1<<20)//������˹��λ
#define JT808_STATUS_USE_GALILEO	(1<<21)//٤���Զ�λ


#define TERMINAL_REGISTER_MSGID				0x0100  //�ն�ע��
#define TERMINAL_AUTHENTICATION_MSGID		0x0102  //�ն˼�Ȩ
#define TERMINAL_HEARTBEAT_MSGID			0x0002  //�ն�����
#define TERMINAL_POSITION_MSGID				0x0200  //λ����Ϣ
#define TERMINAL_ATTRIBUTE_MSGID			0x0107  //�ն�����
#define TERMINAL_GENERIC_RESPON_MSGID		0x0001  //ͨ�ûش�
#define TERMINAL_DATAUPLOAD_MSGID			0x0900  //��������
#define TERMINAL_BATCHUPLOAD_MSG			0x0704	//�����ϴ�

#define PLATFORM_GENERIC_RESPON_MSGID		0x8001
#define PLATFORM_REGISTER_RESPON_MSGID		0x8100
#define PLATFORM_TERMINAL_CTL_MSGID			0x8105
#define PLATFORM_DOWNLINK_MSGID				0x8900
#define PLATFORM_TERMINAL_SET_PARAM_MSGID	0x8103
#define PLATFORM_QUERY_ATTR_MSGID			0x8107
#define PLATFORM_TEXT_MSGID					0x8300

#define TRANSMISSION_TYPE_F8	0xF8

#define RESTORE_MAX_SZIE		24




typedef enum
{
    TERMINAL_REGISTER,
    TERMINAL_AUTH,
    TERMINAL_HEARTBEAT,
    TERMINAL_POSITION,
    TERMINAL_ATTRIBUTE,
    TERMINAL_GENERICRESPON,
    TERMINAL_DATAUPLOAD,
    TERMINAL_BATCHUPLOAD,
} JT808_PROTOCOL;

typedef struct
{
    //λ�û�����Ϣ
    uint32_t alarm;  		//������־
    uint32_t status; 		//״̬
    uint32_t latitude;		//ά��
    uint32_t longtitude;	//����
    uint16_t hight;			//�߶�
    uint16_t speed;			//�ٶ�
    uint16_t course;		//����
    uint8_t year;	//BCDʱ�� GMT+8
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t mintues;
    uint8_t seconds;
    uint8_t statelliteUsed;
} jt808Position_s;



typedef struct
{
    uint8_t jt808isRegister;
    uint8_t jt808sn[6];
    uint8_t jt808manufacturerID[5];
    uint8_t jt808terminalType[20];
    uint8_t jt808terminalID[7];
    uint8_t jt808AuthCode[50];
    uint8_t jt808AuthLen;
    uint16_t serial;

    int (*jt808Send)(uint8_t, uint8_t *, uint16_t);
} jt808Info_s;

typedef struct
{
    uint16_t platform_serial;
    uint16_t platform_id;
    uint8_t  result;
} jt808TerminalRsp;

typedef struct
{
    uint8_t id;
    void *param;
} trammission_s;

typedef struct{
	gpsRestore_s *grs;
	uint16_t num;
	uint8_t type;
}batch_upload_s;


void jt808InfoInit(void);
void jt808CreateSn(uint8_t *jt808sn, uint8_t *imei, uint8_t snlen);
void jt808UpdateStatus(uint32_t bit, uint8_t onoff);
void jt808UpdateAlarm(uint32_t bit, uint8_t onoff);
void jt808RegisterTcpSend(int (*tcpSend)(uint8_t, uint8_t *, uint16_t));
void jt808RegisterLoginInfo(uint8_t *sn, uint8_t reg, uint8_t *authCode, uint8_t authLen);
void jt808RegisterManufactureId(uint8_t *id);
void jt808RegisterTerminalType(uint8_t *type);
void jt808RegisterTerminalId(uint8_t *id);

int jt808TcpSend(uint8_t *data, uint16_t len);



uint16_t jt808gpsRestoreDataSend(uint8_t *dest, gpsRestore_s *grs);
void jt808SendToServer(JT808_PROTOCOL protocol, void *param);
void jt808MessageSend(uint8_t *buf, uint16_t len);
void jt808ReceiveParser(uint8_t *src, uint16_t len);

void jt808BatchInit(void);
void jt808BatchRestorePushIn(gpsRestore_s *grs , uint8_t type);
void jt808BatchPushIn(gpsinfo_s *gpsinfo);
void jt808BatchPushOut(void);



#endif

