#ifndef APP_GPS
#define APP_GPS

#include <stdint.h>

#define GPSFIXED	1
#define GPSINVALID	0

#define GPSITEMCNTMAX	24
#define GPSITEMSIZEMAX	20

#define GPSFIFOSIZE	7



typedef enum
{
	GPS_FILTER_AUTO,	//任何高精度定位
	GPS_FILTER_DIFF,	//只获取差分定位
	GPS_FILTER_FIXHOLD, //只获取固定解定位
	GPS_FILTER_FLOAT,	//只获取浮点解定位

}gpsFilterType_e;

typedef enum
{
	GPS_FIXTYPE_NORMAL  = 1,	 //
	GPS_FIXTYPE_DIFF    = 2,	 //差分定位
	GPS_FIXTYPE_FIXHOLD = 4, //固定解定位
	GPS_FIXTYPE_FLOAT   = 5,	 //浮点解定位

}gpsFixType_e;



typedef struct
{
    unsigned char item_cnt;
    char item_data[GPSITEMCNTMAX][GPSITEMSIZEMAX];
} GPSITEM;


typedef struct
{
    uint8_t year;
    uint8_t month;
    uint8_t day;
    int8_t hour;
    uint8_t minute;
    uint8_t second;
} datetime_s;

typedef struct
{
    datetime_s datetime;//日期时间
    char fixstatus; //定位状态
    double latitude;//纬度
    double longtitude;//经度
    double speed;		//速度
    uint16_t course;//航向角
    float pdop;
	float hight;
    uint32_t gpsticksec;

    char NS; //南北纬
    char EW; //东西经
    char fixmode;
	char fixAccuracy;
    unsigned char used_star;//可用卫星
    unsigned char gpsviewstart;//可见卫星
    unsigned char beidouviewstart;
    unsigned char glonassviewstart;
    uint8_t hadupload;
    uint8_t beidouCn[30];
    uint8_t gpsCn[30];
} gpsinfo_s;

typedef enum
{
    NMEA_RMC = 1,
    NMEA_GSA,
    NMEA_GGA,
    NMEA_GSV,
} NMEATYPE;

/*------------------------------------------------------*/

typedef struct{
	uint8_t currentindex;
	gpsinfo_s gpsinfohistory[GPSFIFOSIZE];
	gpsinfo_s lastfixgpsinfo;
}GPSFIFO;

/*------------------------------------------------------*/

typedef struct{
	datetime_s datetime;//日期时间
	double latitude;//纬度
	double longtitude;//经度
	uint32_t gpsticksec;
	uint8_t init;
}LASTUPLOADGPSINFO;

/*------------------------------------------------------*/

void initGpsNow(void);

void nmeaParser(uint8_t *buf, uint16_t len);
double latitude_to_double(double latitude, char Direction);
double longitude_to_double(double longitude, char Direction);
/*------------------------------------------------------*/
datetime_s changeUTCTimeToLocalTime(datetime_s utctime,int8_t localtimezone);
void updateLocalRTCTime(datetime_s *datetime);

/*------------------------------------------------------*/

void gpsClearCurrentGPSInfo(void);
gpsinfo_s *getCurrentGPSInfo(void);
gpsinfo_s *getLastFixedGPSInfo(void);
GPSFIFO *getGSPfifo(void);
uint8_t *getGga(void);

/*------------------------------------------------------*/
void gpsUploadPointToServer(void);

#endif
