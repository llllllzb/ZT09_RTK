#ifndef APPSERVERPROTOCOL
#define APPSERVERPROTOCOL

#include <stdint.h>


#define WIFIINFOMAX		15
typedef enum{
	PROTOCOL_01,//��¼
	PROTOCOL_12,//��λ
	PROTOCOL_13,//����
	PROTOCOL_16,//����
	PROTOCOL_19,//���վ
	PROTOCOL_21,//����
	PROTOCOL_61,
	PROTOCOL_62,
	PROTOCOL_F1,//ICCID
	PROTOCOL_F3,
	PROTOCOL_8A,
	PROTOCOL_0B,
}PROTOCOLTYPE;



//�ܹ�17���ֽڣ��洢20���ֽ�
typedef struct{
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t latititude[4];
	uint8_t longtitude[4];
	uint8_t speed;
	uint8_t coordinate[2];
	uint8_t temp[3];
}gpsRestore_s;

typedef struct
{
    uint8_t ssid[6];
    int8_t signal;
} WIFIINFOONE;
typedef struct
{
    WIFIINFOONE ap[WIFIINFOMAX];
    uint8_t apcount;
} WIFIINFO;

typedef struct{
	char IMEI[17];
	uint8_t batLevel;
	float Voltage;
	uint16_t startUpcnt;
	uint16_t runCnt;
	uint16_t serialNum;
}protocolInfo_s;


typedef struct
{
    char *dest;
    char *dateTime;
    uint8_t fileType;
    uint8_t *recData;
    uint16_t packNum;
    uint16_t recLen;
    uint32_t totalSize;
    uint16_t packSize;
} recordUploadInfo_s;

typedef struct
{
    uint8_t msgId;
    uint16_t msgLen;
    uint8_t *msg;
} MessageInfo;

typedef struct
{
    MessageInfo *msgList;
    uint8_t size;
} MessageList;

void protocolSend(uint8_t link,PROTOCOLTYPE protocol,void * param);

uint8_t getBatteryLevel(void);
void gpsRestoreDataSend(gpsRestore_s *grs, char *dest	, uint16_t *len);

void save123InstructionId(void);
void reCover123InstructionId(void);

void getInsid(void);
void setInsId(void);

void gpsRestoreUpload(void);


void protocolSnRegister(char *sn);
void protocolInfoResiter(uint8_t batLevel, float vol, uint16_t sCnt, uint16_t runCnt);

void protocolRxParser(uint8_t link, char *protocol, uint16_t size);


#endif
