#include <app_protocol.h>
#include "app_gps.h"

#include <math.h>
#include "app_param.h"
#include "app_port.h"
#include "app_sys.h"
#include "app_task.h"
#include "app_jt808.h"
#include "app_peripheral.h"


#define PIA 3.1415926
static gpsinfo_s gpsinfonow;
static GPSFIFO gpsfifo;
static LASTUPLOADGPSINFO lastuploadgpsinfo;
static uint8_t ggaData[100];
static uint8_t gpgsvFlag = 0;
static uint8_t bdgsvFlag = 0;

static void addGpsInfo(gpsinfo_s *gpsinfo);


void initGpsNow(void)
{
	tmos_memset(&gpsinfonow, 0, sizeof(gpsinfo_s));
}

static unsigned char nemaCalcuateCrc(char *str, int len)
{
    int i, index, size;
    unsigned char crc;
    crc = str[1];
    index = getCharIndex((uint8_t *)str, len, '*');
    size = len - index;
    for (i = 2; i < len - size; i++)
    {
        crc ^= str[i];
    }
    return crc;
}
static unsigned char chartohexcharvalue(char value)
{
    if (value >= '0' && value <= '9')
        return value - '0';
    if (value >= 'a' && value <= 'z')
        return value - 'a' + 10;
    if (value >= 'A' && value <= 'Z')
        return value - 'A' + 10;
    return 0;
}

/*���ַ�����16����ת��Ϊ��ֵ*/
unsigned int charstrToHexValue(char *value)
{
    unsigned int calvalue = 0;
    unsigned char i, j, len = strlen(value);
    j = 0;
    j = len;
    for (i = 0; i < len; i++)
    {
        value[i] = chartohexcharvalue(value[i]);
        calvalue += value[i] * pow(16, j - 1);
        j--;
    }
    return calvalue;
}

/*
$GPRMC,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>*hh<CR><LF>
<1> UTCʱ�䣬hhmmss��ʱ���룩��ʽ
<2> ��λ״̬��A=��Ч��λ��V=��Ч��λ
<3> γ��ddmm.mmmm���ȷ֣���ʽ��ǰ���0Ҳ�������䣩
<4> γ�Ȱ���N�������򣩻�S���ϰ���
<5> ����dddmm.mmmm���ȷ֣���ʽ��ǰ���0Ҳ�������䣩
<6> ���Ȱ���E����������W��������
<7> �������ʣ�000.0~999.9�ڣ�ǰ���0Ҳ�������䣩
<8> ���溽��000.0~359.9�ȣ����汱Ϊ�ο���׼��ǰ���0Ҳ�������䣩
<9> UTC���ڣ�ddmmyy�������꣩��ʽ
<10> ��ƫ�ǣ�000.0~180.0�ȣ�ǰ���0Ҳ�������䣩
<11> ��ƫ�Ƿ���E��������W������
<12> ģʽָʾ����NMEA0183 3.00�汾�����A=������λ��D=��֣�E=���㣬N=������Ч��
*/
void parse_RMC(GPSITEM *item)
{
    gpsinfo_s *gpsinfo;
    gpsinfo = &gpsinfonow;
    if (item->item_data[1][0] != 0)
    {
        gpsinfo->datetime.hour = (item->item_data[1][0] - '0') * 10 + (item->item_data[1][1] - '0');
        gpsinfo->datetime.minute = (item->item_data[1][2] - '0') * 10 + (item->item_data[1][3] - '0');
        gpsinfo->datetime.second = (item->item_data[1][4] - '0') * 10 + (item->item_data[1][5] - '0');
    }
    if (item->item_data[2][0] != 0)
    {
        if (item->item_data[2][0] == 'A')
        {
            gpsinfo->fixstatus = GPSFIXED;
        }
        else
        {
            gpsinfo->fixstatus = GPSINVALID;
        }
    }
    if (item->item_data[3][0] != 0)
    {
        gpsinfo->latitude = atof(item->item_data[3]);
    }
    if (item->item_data[4][0] != 0)
    {
        gpsinfo->NS = item->item_data[4][0];
        gpsinfo->latitude = latitude_to_double(gpsinfo->latitude, gpsinfo->NS);
    }
    if (item->item_data[5][0] != 0)
    {
        gpsinfo->longtitude = atof(item->item_data[5]);
    }
    if (item->item_data[6][0] != 0)
    {
        gpsinfo->EW = item->item_data[6][0];
        gpsinfo->longtitude = longitude_to_double(gpsinfo->longtitude, gpsinfo->EW);
    }
    if (item->item_data[7][0] != 0)
    {
        gpsinfo->speed = atof(item->item_data[7]) * 1.852;
    }
    if (item->item_data[8][0] != 0)
    {
        gpsinfo->course = (uint16_t)atoi(item->item_data[8]);
    }
    if (item->item_data[9][0] != 0)
    {
        gpsinfo->datetime.day = (item->item_data[9][0] - '0') * 10 + (item->item_data[9][1] - '0');
        gpsinfo->datetime.month = (item->item_data[9][2] - '0') * 10 + (item->item_data[9][3] - '0');
        gpsinfo->datetime.year = (item->item_data[9][4] - '0') * 10 + (item->item_data[9][5] - '0');

    }
    gpsinfo->gpsticksec = gpsinfo->datetime.hour * 3600 + gpsinfo->datetime.minute * 60 + gpsinfo->datetime.second;
    addGpsInfo(gpsinfo);
    memset(gpsinfo, 0, sizeof(gpsinfo_s));
    gpgsvFlag = 0;
    bdgsvFlag = 0;
}

/*
$GPGSA
����$GPGSA,A,3,01,20,19,13,,,,,,,,,40.4,24.4,32.2*0A
�ֶ�0��$GPGSA�����ID�����������ΪGPS DOP and Active Satellites��GSA����ǰ������Ϣ
�ֶ�1����λģʽ��A=�Զ��ֶ�2D/3D��M=�ֶ�2D/3D
�ֶ�2����λ���ͣ�1=δ��λ��2=2D��λ��3=3D��λ
�ֶ�3��PRN�루α��������룩����1�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�4��PRN�루α��������룩����2�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�5��PRN�루α��������룩����3�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�6��PRN�루α��������룩����4�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�7��PRN�루α��������룩����5�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�8��PRN�루α��������룩����6�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�9��PRN�루α��������룩����7�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�10��PRN�루α��������룩����8�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�11��PRN�루α��������룩����9�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�12��PRN�루α��������룩����10�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�13��PRN�루α��������룩����11�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�14��PRN�루α��������룩����12�ŵ�����ʹ�õ�����PRN���ţ�00����ǰ��λ��������0��
�ֶ�15��PDOP�ۺ�λ�þ������ӣ�0.5 - 99.9��
�ֶ�16��HDOPˮƽ�������ӣ�0.5 - 99.9��
�ֶ�17��VDOP��ֱ�������ӣ�0.5 - 99.9��
�ֶ�18��У��ֵ
*/
void parse_GSA(GPSITEM *item)
{
    gpsinfo_s *gpsinfo;
    gpsinfo = &gpsinfonow;

    if (item->item_data[2][0] != 0)
    {
        gpsinfo->fixmode = atoi(item->item_data[2]);
    }
    if (item->item_data[15][0] != 0)
    {
        gpsinfo->pdop = atof(item->item_data[15]);
    }
}
/*
$GPGSV
����$GPGSV,3,1,10,20,78,331,45,01,59,235,47,22,41,069,,13,32,252,45*70
�ֶ�0��$GPGSV�����ID�����������ΪGPS Satellites in View��GSV���ɼ�������Ϣ
�ֶ�1������GSV��������Ŀ��1 - 3��
�ֶ�2������GSV����Ǳ���GSV���ĵڼ�����1 - 3��
�ֶ�3����ǰ�ɼ�����������00 - 12����ǰ��λ��������0��
�ֶ�4��PRN �루α��������룩��01 - 32����ǰ��λ��������0��
�ֶ�5���������ǣ�00 - 90���ȣ�ǰ��λ��������0��
�ֶ�6�����Ƿ�λ�ǣ�00 - 359���ȣ�ǰ��λ��������0��
�ֶ�7������ȣ�00��99��dbHz
�ֶ�8��PRN �루α��������룩��01 - 32����ǰ��λ��������0��
�ֶ�9���������ǣ�00 - 90���ȣ�ǰ��λ��������0��
�ֶ�10�����Ƿ�λ�ǣ�00 - 359���ȣ�ǰ��λ��������0��
�ֶ�11������ȣ�00��99��dbHz
�ֶ�12��PRN �루α��������룩��01 - 32����ǰ��λ��������0��
�ֶ�13���������ǣ�00 - 90���ȣ�ǰ��λ��������0��
�ֶ�14�����Ƿ�λ�ǣ�00 - 359���ȣ�ǰ��λ��������0��
�ֶ�15������ȣ�00��99��dbHz
�ֶ�16��У��ֵ
*/

void parse_GSV(GPSITEM *item)
{
    gpsinfo_s *gpsinfo;
    uint8_t gpskind = 0, currentpage;
    static uint8_t gpscount = 0;
    static uint8_t bdcount = 0;
    gpsinfo = &gpsinfonow;

    if (my_strpach(item->item_data[0], (const char *)"$GP"))
    {
        gpskind = 1;
    }
    else if (my_strpach(item->item_data[0], (const char *)"$GB"))
    {
        gpskind = 2;
    }
    else if (my_strpach(item->item_data[0], (const char *)"$BD"))
    {
        gpskind = 2;
    }
    else if (my_strpach(item->item_data[0], (const char *)"$GL"))
    {
        gpskind = 3;
    }

    if (item->item_data[3][0] != 0)
    {
        currentpage = atoi(item->item_data[2]);
        switch (gpskind)
        {
            case 1:
                gpsinfo->gpsviewstart = atoi(item->item_data[3]);
				
                if (/*currentpage == 1 && */gpgsvFlag == 0)
                {
                	gpgsvFlag = 1;
                    gpscount = 0;
                    memset(gpsinfo->gpsCn, 0, sizeof(gpsinfo->gpsCn));
                }
                if (item->item_data[7][0] != '0')
                {
                    gpsinfo->gpsCn[gpscount++] = atoi(item->item_data[7]);
                }
                if (item->item_data[11][0] != '0')
                {
                    gpsinfo->gpsCn[gpscount++] = atoi(item->item_data[11]);
                }
                if (item->item_data[15][0] != '0')
                {
                    gpsinfo->gpsCn[gpscount++] = atoi(item->item_data[15]);
                }
                if (item->item_data[19][0] != '0')
                {
                    gpsinfo->gpsCn[gpscount++] = atoi(item->item_data[19]);
                }
                break;
            case 2:
                gpsinfo->beidouviewstart = atoi(item->item_data[3]);
                if (/*currentpage == 1 &&*/ bdgsvFlag == 0)
                {
                	bdgsvFlag = 1;
                    bdcount = 0;
                    memset(gpsinfo->beidouCn, 0, sizeof(gpsinfo->beidouCn));
                }
                if (item->item_data[7][0] != '0')
                {
                    gpsinfo->beidouCn[bdcount++] = atoi(item->item_data[7]);
                }
                if (item->item_data[11][0] != '0')
                {
                    gpsinfo->beidouCn[bdcount++] = atoi(item->item_data[11]);
                }
                if (item->item_data[15][0] != '0')
                {
                    gpsinfo->beidouCn[bdcount++] = atoi(item->item_data[15]);
                }
                if (item->item_data[19][0] != '0')
                {
                    gpsinfo->beidouCn[bdcount++] = atoi(item->item_data[19]);
                }
                break;
            case 3:
                gpsinfo->glonassviewstart = atoi(item->item_data[3]);
                break;
        }
    }
    //LogPrintf(DEBUG_ALL, "gpgsvFlag:%d bdgsvFlag:%d gpcn:%d %d %d %dbdcn:%d %d %d %d", gpgsvFlag, bdgsvFlag, gpsinfo->gpsCn[0], gpsinfo->gpsCn[7], gpsinfo->gpsCn[9], gpsinfo->gpsCn[10], gpsinfo->beidouCn[0],gpsinfo->beidouCn[1], gpsinfo->beidouCn[3], gpsinfo->beidouCn[4]);
}
/*
$GPGGA
����$GPGGA,092204.999,4250.5589,S,14718.5084,E,1,04,24.4,19.7,M,,,,0000*1F
�ֶ�0��$GPGGA�����ID�����������ΪGlobal Positioning System Fix Data��GGA��GPS��λ��Ϣ
�ֶ�1��UTC ʱ�䣬hhmmss.sss��ʱ�����ʽ
�ֶ�2��γ��ddmm.mmmm���ȷָ�ʽ��ǰ��λ��������0��
�ֶ�3��γ��N����γ����S����γ��
�ֶ�4������dddmm.mmmm���ȷָ�ʽ��ǰ��λ��������0��
�ֶ�5������E����������W��������
�ֶ�6��GPS״̬��0=δ��λ��1=�ǲ�ֶ�λ��2=��ֶ�λ��3=��ЧPPS��6=���ڹ���
�ֶ�7������ʹ�õ�����������00 - 12����ǰ��λ��������0��
�ֶ�8��HDOPˮƽ�������ӣ�0.5 - 99.9��
�ֶ�9�����θ߶ȣ�-9999.9 - 99999.9��
�ֶ�10��������������Դ��ˮ׼��ĸ߶�
�ֶ�11�����ʱ�䣨�����һ�ν��յ�����źſ�ʼ��������������ǲ�ֶ�λ��Ϊ�գ�
�ֶ�12�����վID��0000 - 1023��ǰ��λ��������0��������ǲ�ֶ�λ��Ϊ�գ�
�ֶ�13��У��ֵ
*/
void parse_GGA(GPSITEM *item)
{
    gpsinfo_s *gpsinfo;
    gpsinfo = &gpsinfonow;
    if (item->item_data[6][0] != 0)
    {
        gpsinfo->fixAccuracy = atoi(item->item_data[6]);
    }
    if (item->item_data[7][0] != 0)
    {
        gpsinfo->used_star = atoi(item->item_data[7]);
    }

    if (item->item_data[9][0] != 0)
    {
        gpsinfo->hight = atof(item->item_data[9]);
    }
}
static unsigned char parseGetNmeaType(char *str)
{
    if (my_strstr(str, "RMC", 6))
    {
        return NMEA_RMC;
    }

    if (my_strstr(str, "GSA", 6))
    {
        return NMEA_GSA;
    }

    if (my_strstr(str, "GGA", 6))
    {
        return NMEA_GGA;
    }

    if (my_strstr(str, "GSV", 6))
    {
        return NMEA_GSV;
    }

    return 0;
}

/*������һ�����ݷֽ��һ����ITEM��*/
void parseGPS(uint8_t *str, uint16_t len)
{
    GPSITEM item;
    unsigned char nmeacrc, nmeatype;
    int i = 0, data_len = 0, index;
    memset(&item, 0, sizeof(GPSITEM));
    for (i = 0; i < len; i++)
    {
        if (str[i] == ',' || str[i] == '*')
        {
            item.item_cnt++;
            data_len = 0;
            if (item.item_cnt >= GPSITEMCNTMAX)
            {
                return ;
            }

        }
        else
        {
            item.item_data[item.item_cnt][data_len] = str[i];
            data_len++;
            if (data_len >= GPSITEMSIZEMAX)
            {
                return ;
            }
        }
    }
    index = getCharIndex((uint8_t *)str, len, '*');
    memcpy(item.item_data[item.item_cnt], str + index + 1, len - index - 1);
    nmeacrc = nemaCalcuateCrc((char *)str, len);
    if (nmeacrc != charstrToHexValue(item.item_data[item.item_cnt]))
    {
        return ;
    }
    nmeatype = parseGetNmeaType(item.item_data[0]);
    switch (nmeatype)
    {
        case NMEA_RMC:
            parse_RMC(&item);
            break;
        case NMEA_GSA:
            parse_GSA(&item);
            break;
        case NMEA_GGA:
        	strncpy(ggaData, str, sizeof(ggaData) - 1);
            parse_GGA(&item);
            break;
        case NMEA_GSV:
            parse_GSV(&item);
            break;
    }


}
/*
$GNRMC,091602.00,A,2303.49865,N,11322.83066,E,0.026,,201219,,,A,V*19
$GNVTG,,T,,M,0.026,N,0.048,K,A*35
$GNGGA,091602.00,2303.49865,N,11322.83066,E,1,12,0.94,3.7,M,-5.2,M,,*52
$GNGSA,M,3,22,14,03,32,16,27,29,,,,,,1.70,0.94,1.41,1*0C
$GNGSA,M,3,05,24,25,09,,,,,,,,,1.70,0.94,1.41,3*0D
$GNGSA,M,3,16,06,07,10,01,02,21,24,29,03,,,1.70,0.94,1.41,4*0E
$GPGSV,3,1,11,03,26,287,34,04,16,321,,14,63,117,43,16,65,257,41,0*68
$GPGSV,3,2,11,22,29,263,41,23,11,320,19,26,70,344,,27,24,181,19,0*66
$GPGSV,3,3,11,29,16,048,18,31,44,039,,32,34,135,40,0*51
$GAGSV,3,1,10,02,07,247,,03,26,319,,05,76,276,38,09,43,157,40,0*7A
$GAGSV,3,2,10,11,09,148,,12,10,098,,18,44,111,44,24,50,013,14,0*7F
$GAGSV,3,3,10,25,49,281,40,31,09,048,,0*77
$GBGSV,4,1,16,01,50,128,36,02,47,234,35,03,62,187,39,04,32,111,33,0*7A
$GBGSV,4,2,16,06,75,120,37,07,74,329,16,08,00,171,,09,74,343,,0*7F
$GBGSV,4,3,16,10,58,260,33,14,33,301,,16,79,114,34,21,41,191,40,0*79
*/
void nmeaParser(uint8_t *buf, uint16_t len)
{
    uint16_t i;
    char onenmeadata[100];
    uint8_t foundnmeahead, restoreindex;
    foundnmeahead = 0;
    restoreindex = 0;

	
    for (i = 0; i < len; i++)
    {
        if (buf[i] == '$')
        {
            foundnmeahead = 1;
            restoreindex = 0;
        }
        if (foundnmeahead == 1)
        {
            if (buf[i] == '\r')
            {
                foundnmeahead = 0;
                onenmeadata[restoreindex] = 0;
                parseGPS((uint8_t *)onenmeadata, restoreindex);
            }
            else
            {
                onenmeadata[restoreindex] = buf[i];
                restoreindex++;
                if (restoreindex >= 99)
                {
                    foundnmeahead = 0;
                }
            }
        }
    }
}

/*
ά��ת��
*/
double latitude_to_double(double latitude, char Direction)
{
    double f_lat;
    unsigned long degree, fen, tmpval;

    if (latitude == 0 || (Direction != 'N' && Direction != 'S'))
    {
        return 0.0;
    }
    //����10��,�������һ��С����,�������
    tmpval = (unsigned long)(latitude * 100000.0);
    //��ȡ��ά��--��
    degree = tmpval / (100 * 100000);
    //��ȡ��ά��--��
    fen = tmpval % (100 * 100000);

    if (Direction == 'S')
    {
        f_lat = ((double)degree + ((double)fen) / 100000.0 / 60.0) * (-1);
    }
    else
    {
        f_lat = (double)degree + ((double)fen) / 100000.0 / 60.0;
    }

    return f_lat;
}

/*
����ת��
*/
double longitude_to_double(double longitude, char Direction)
{
    double  f_lon;
    unsigned long degree, fen, tmpval;


    if (longitude == 0 || (Direction != 'E' && Direction != 'W'))
    {
        return 0.0;
    }

    tmpval = (unsigned long)(longitude * 100000.0);
    degree = tmpval / (100 * 100000);
    fen = tmpval % (100 * 100000);

    if (Direction == 'W')
    {
        f_lon = ((double)degree + ((double)fen) / 100000.0 / 60.0) * (-1);
    }
    else
    {
        f_lon = (double)degree + ((double)fen) / 100000.0 / 60.0;
    }

    return f_lon;

}


/*------------------------------------------------------*/

//����ǰʱ�����ʱ����ת���ɶ�Ӧʱ��
datetime_s changeUTCTimeToLocalTime(datetime_s utctime, int8_t localtimezone)
{
    datetime_s localtime;
    if (utctime.year == 0 || utctime.month == 0 || utctime.day == 0)
    {
        return utctime;
    }
    localtime.second = utctime.second;
    localtime.minute = utctime.minute;
    localtime.hour = utctime.hour + localtimezone;
    localtime.day = utctime.day;
    localtime.month = utctime.month;
    localtime.year = utctime.year;

    if (localtime.hour >= 24) //���˵ڶ����ʱ��
    {
        localtime.hour = localtime.hour % 24;
        localtime.day += 1; //���ڼ�1
        if (localtime.month == 4 || localtime.month == 6 || localtime.month == 9 || localtime.month == 11)
        {
            //30��
            if (localtime.day > 30)
            {
                localtime.day = 1;
                localtime.month += 1;
            }
        }
        else if (localtime.month == 2)//2�·�
        {
            //28���29��
            if ((((localtime.year + 2000) % 100 != 0) && ((localtime.year + 2000) % 4 == 0)) ||
                    ((localtime.year + 2000) % 400 == 0))//����
            {
                if (localtime.day > 29)
                {
                    localtime.day = 1;
                    localtime.month += 1;
                }
            }
            else
            {
                if (localtime.day > 28)
                {
                    localtime.day = 1;
                    localtime.month += 1;
                }
            }
        }
        else
        {
            //31��
            if (localtime.day > 31)
            {
                localtime.day = 1;
                if (localtime.month == 12)
                {
                    //��Ҫ����
                    localtime.month = 1;
                    localtime.year += 1;

                }
                else
                {
                    localtime.month += 1;
                }
            }

        }
    }
    else if (localtime.hour < 0) //ǰһ���ʱ��
    {
        localtime.hour = localtime.hour + 24;
        localtime.day -= 1;
        if (localtime.day == 0)
        {
            localtime.month -= 1;
            if (localtime.month == 4 || localtime.month == 6 || localtime.month == 9 || localtime.month == 11)
            {
                //30��
                localtime.day = 30;

            }
            else if (localtime.month == 2)//2�·�
            {
                //28���29��
                if ((((localtime.year + 2000) % 100 != 0) && ((localtime.year + 2000) % 4 == 0)) ||
                        ((localtime.year + 2000) % 400 == 0))//����
                {
                    localtime.day = 29;
                }
                else
                {
                    localtime.day = 28;
                }
            }
            else
            {
                //31��
                localtime.day = 31;
                if (localtime.month == 0)
                {
                    localtime.month = 12;
                    localtime.year -= 1;
                }
            }
        }
    }
    return localtime;

}
void updateLocalRTCTime(datetime_s *datetime)
{
    datetime_s localtime;
    localtime = changeUTCTimeToLocalTime(*datetime, sysparam.utc);
    portUpdateRtcOffset(localtime.year, localtime.month, localtime.day, localtime.hour, localtime.minute,
                          localtime.second);

    if (sysparam.MODE == MODE1 || sysparam.MODE == MODE21)
    {
        portSetNextAlarmTime();
    }
}

/*------------------------------------------------------*/
/*------------------------------------------------------*/
//����0�����
static uint8_t  gpsFilter(gpsinfo_s *gpsinfo)
{
    if (gpsinfo->fixstatus == 0)
    {
        return 0;
    }
    if (gpsinfo->latitude == 0 || gpsinfo->longtitude == 0)
    {
        return 0;
    }
    if (gpsinfo->datetime.day == 0)
    {
        return 0;
    }
    return 1;
}

void showgpsinfo(void)
{
    char debug[300] = {0};
    uint8_t i, total;
    gpsinfo_s *gpsinfo;
    gpsinfo = &gpsinfonow;
    if (gpsinfo->fixstatus)
    {
    	if (gpsinfo->fixAccuracy == GPS_FIXTYPE_NORMAL)
			sprintf(debug, "[Normal]");
		else if (gpsinfo->fixAccuracy == GPS_FIXTYPE_DIFF)
			sprintf(debug, "[Difference]");
		else if (gpsinfo->fixAccuracy == GPS_FIXTYPE_FIXHOLD)
			sprintf(debug, "[Fixhold]");
		else if (gpsinfo->fixAccuracy == GPS_FIXTYPE_FLOAT)
			sprintf(debug, "[Float]");
		else 
			sprintf(debug, "[Unknow High precision]");
    }
    else
    {
		sprintf(debug, "[No fix]");
    }
    sprintf(debug + strlen(debug), "NMEA:%d/%d/%d  %02d:%02d:%02d;", gpsinfo->datetime.year, gpsinfo->datetime.month, gpsinfo->datetime.day,
            gpsinfo->datetime.hour, gpsinfo->datetime.minute, gpsinfo->datetime.second);
    sprintf(debug + strlen(debug), "%c %f %c %f;\n", gpsinfo->NS, gpsinfo->latitude, gpsinfo->EW, gpsinfo->longtitude);
    sprintf(debug + strlen(debug), "%s;", gpsinfo->fixstatus ? "FIXED" : "Invalid");
    sprintf(debug + strlen(debug), "Speed=%.2f;", gpsinfo->speed);
    sprintf(debug + strlen(debug), "Accuracy=%d;", gpsinfo->fixAccuracy);
    sprintf(debug + strlen(debug), "PDOP=%.2f;Fixmode=%d;", gpsinfo->pdop, gpsinfo->fixmode);
    sprintf(debug + strlen(debug), "Use Star=%d;", gpsinfo->used_star);
    sprintf(debug + strlen(debug), "GPS CN:");
    total = sizeof(gpsinfo->gpsCn);
    for (i = 0; i < total; i++)
    {
        if (gpsinfo->gpsCn[i] != 0)
        {
            sprintf(debug + strlen(debug), "%d;", gpsinfo->gpsCn[i]);
        }
    }
    sprintf(debug + strlen(debug), "BeiDou CN:");
    total = sizeof(gpsinfo->beidouCn);
    for (i = 0; i < total; i++)
    {
        if (gpsinfo->beidouCn[i] != 0)
        {
            sprintf(debug + strlen(debug), "%d;", gpsinfo->beidouCn[i]);
        }
    }
    //debug[strlen(debug)] = 0;
    LogMessage(DEBUG_ALL, debug);

}

static void addGpsInfo(gpsinfo_s *gpsinfo)
{
    static uint8_t tick = 0;
    if (sysinfo.rtcUpdate == 0 && gpsinfo->fixstatus == 1 && gpsinfo->fixAccuracy)
    {
        tick++;
        if (gpsinfo->datetime.day != 0 && gpsinfo->datetime.hour != 0 && gpsinfo->datetime.minute != 0 &&
                gpsinfo->datetime.second != 0 && tick >= 5)
        {

            if (gpsinfo->datetime.day != 0 && gpsinfo->datetime.second != 0)
            {
                sysinfo.rtcUpdate = 1;
                updateLocalRTCTime(&gpsinfo->datetime);
            }
            tick= 0;
        }
    }
    else
    {
        tick = 0;
    }


    //����ӽ��������ݸ��Ƶ�������
    if (gpsFilter(gpsinfo) == 0)
    {
        gpsinfo->fixstatus = 0;
    }
    else
    {
        memcpy(&gpsfifo.lastfixgpsinfo, gpsinfo, sizeof(gpsinfo_s));
    }

    //ʱ�䲻ͬ����������
    gpsfifo.currentindex = (gpsfifo.currentindex + 1) % GPSFIFOSIZE;
    //���������������
    memcpy(&gpsfifo.gpsinfohistory[gpsfifo.currentindex], gpsinfo, sizeof(gpsinfo_s));
    showgpsinfo();
    sysinfo.gpsUpdatetick = sysinfo.sysTick;
}

void gpsClearCurrentGPSInfo(void)
{
    memset(&gpsfifo.gpsinfohistory[gpsfifo.currentindex], 0, sizeof(gpsinfo_s));
    ledStatusUpdate(SYSTEM_LED_GPSOK, 0);
}
gpsinfo_s *getCurrentGPSInfo(void)
{
    return &gpsfifo.gpsinfohistory[gpsfifo.currentindex];
}

gpsinfo_s *getLastFixedGPSInfo(void)
{
    return &gpsfifo.lastfixgpsinfo;
}

GPSFIFO *getGSPfifo(void)
{
    return &gpsfifo;
}

uint8_t *getGga(void)
{
	return ggaData;
}
/*------------------------------------------------------*/
/*------------------------------------------------------*/
/*------------------------------------------------------*/
static double calculateTheDistanceBetweenTwoPonits(double lat1, double lng1, double lat2, double lng2)
{
    double radLng1, radLng2;
    double a, b, s;
    radLng1 = lng1 * PIA / 180.0;
    radLng2 = lng2 * PIA / 180.0;
    a = (radLng1 - radLng2);
    b = (lat1 - lat2) * PIA / 180.0;
    s = 2 * asin(sqrt(sin(a / 2) * sin(a / 2) + cos(radLng1) * cos(radLng2) * sin(b / 2) * sin(b / 2))) * 6378.137;
    return s;
}

static void initLastPoint(gpsinfo_s *gpsinfo)
{
    lastuploadgpsinfo.datetime = gpsinfo->datetime;
    lastuploadgpsinfo.latitude = gpsinfo->latitude;
    lastuploadgpsinfo.longtitude = gpsinfo->longtitude;
    lastuploadgpsinfo.gpsticksec = gpsinfo->gpsticksec;
    lastuploadgpsinfo.init = 1;
    LogPrintf(DEBUG_ALL, "Update last position, lat:%.2f lon:%.2f", lastuploadgpsinfo.latitude, lastuploadgpsinfo.longtitude);
}


static int8_t calculateDistanceOfPoint(void)
{
    gpsinfo_s *gpsinfo;
    double distance;
    char debug[100];
    gpsinfo = getCurrentGPSInfo();
    if (gpsinfo->fixstatus == 0)
    {
        return -1;
    }
    if (lastuploadgpsinfo.init == 0)
    {
        initLastPoint(gpsinfo);
        return -2;
    }

    distance = calculateTheDistanceBetweenTwoPonits(gpsinfo->latitude, gpsinfo->longtitude, lastuploadgpsinfo.latitude,
               lastuploadgpsinfo.longtitude) * 1000;
    sprintf(debug, "lastlat:%.2f, lastlon:%.2f lastgpsticksec:%d nowlat:%.2f, nowlon:%.2f, nowgpstick:%d distance of point =%.2fm", lastuploadgpsinfo.latitude, lastuploadgpsinfo.longtitude, lastuploadgpsinfo.gpsticksec, gpsinfo->latitude,  gpsinfo->longtitude, gpsinfo->gpsticksec, distance);
    LogMessage(DEBUG_ALL, debug);
    if (distance > sysparam.fence)
    {
        return 1;
    }
    return 0;
}

static int8_t calculateTheGPSCornerPoint(void)
{
    GPSFIFO *gpsfifo;
    gpsinfo_s *gpscurrent;
    char debug[30];
    uint16_t course[5];
    uint8_t positionidnex[5];
    uint16_t coursechange;
    uint8_t cur_index, i;
    gpsfifo = getGSPfifo();
    cur_index = gpsfifo->currentindex;
    for (i = 0; i < 5; i++)
    {
        //���˲���
        if (gpsfifo->gpsinfohistory[cur_index].fixstatus == 0 || gpsfifo->gpsinfohistory[cur_index].speed < 6.5)
        {
            return -1;
        }
        if (gpsfifo->gpsinfohistory[cur_index].hadupload == 1)
        {
            return -2;
        }
        //����5����ķ���
        course[i] = gpsfifo->gpsinfohistory[cur_index].course;
        positionidnex[i] = cur_index;
        //��ȡ��һ������
        cur_index = (cur_index + GPSFIFOSIZE - 1) % GPSFIFOSIZE;
    }
    //����ת��Ƕ�
    coursechange = abs(course[0] - course[4]);
    if (coursechange > 180)
    {
        coursechange = 360 - coursechange;
    }
    sprintf(debug, "Total course=%d", coursechange);
    LogMessage(DEBUG_ALL, debug);
    if (coursechange >= 15 && coursechange <= 45)
    {
        protocolSend(NORMAL_LINK, PROTOCOL_12, &gpsfifo->gpsinfohistory[positionidnex[1]]);
        protocolSend(NORMAL_LINK, PROTOCOL_12, &gpsfifo->gpsinfohistory[positionidnex[3]]);
        jt808SendToServer(TERMINAL_POSITION, &gpsfifo->gpsinfohistory[positionidnex[1]]);
        jt808SendToServer(TERMINAL_POSITION, &gpsfifo->gpsinfohistory[positionidnex[3]]);
    }
    else if (coursechange > 45)
    {
        protocolSend(NORMAL_LINK, PROTOCOL_12, &gpsfifo->gpsinfohistory[positionidnex[0]]);
        protocolSend(NORMAL_LINK, PROTOCOL_12, &gpsfifo->gpsinfohistory[positionidnex[1]]);
        protocolSend(NORMAL_LINK, PROTOCOL_12, &gpsfifo->gpsinfohistory[positionidnex[2]]);
        protocolSend(NORMAL_LINK, PROTOCOL_12, &gpsfifo->gpsinfohistory[positionidnex[3]]);
        protocolSend(NORMAL_LINK, PROTOCOL_12, &gpsfifo->gpsinfohistory[positionidnex[4]]);


        jt808SendToServer(TERMINAL_POSITION, &gpsfifo->gpsinfohistory[positionidnex[0]]);
        jt808SendToServer(TERMINAL_POSITION, &gpsfifo->gpsinfohistory[positionidnex[1]]);
        jt808SendToServer(TERMINAL_POSITION, &gpsfifo->gpsinfohistory[positionidnex[2]]);
        jt808SendToServer(TERMINAL_POSITION, &gpsfifo->gpsinfohistory[positionidnex[3]]);
        jt808SendToServer(TERMINAL_POSITION, &gpsfifo->gpsinfohistory[positionidnex[4]]);

        gpscurrent = getCurrentGPSInfo();
        if (gpsfifo->gpsinfohistory[positionidnex[0]].gpsticksec == gpscurrent->gpsticksec)
        {
            gpscurrent->hadupload = 1;
        }
    }
    return 1;
}

/* 
	GPS_REQUEST_ACC_CTL����ŵ��ô˺���	
*/
void gpsUploadPointToServer(void)
{
    static uint32_t tick = 0;
	/* �˶�����С��2����,GPS�������� */
    if (sysinfo.gpsRequest != GPS_REQUEST_ACC_CTL)
    {
        tick = 0;
        return;
    }
    tick++;
    //ȡ���յ�
    //calculateTheGPSCornerPoint();
    if (tick >= sysparam.gpsuploadgap)
    {
        if (calculateDistanceOfPoint() == 1)
        {
//            protocolSend(NORMAL_LINK, PROTOCOL_12, getCurrentGPSInfo());
//            jt808SendToServer(TERMINAL_POSITION, getCurrentGPSInfo());
//            initLastPoint(getCurrentGPSInfo());
			gpsRequestSet(GPS_REQUEST_UPLOAD_ONE);
            tick = 0;
        }
    }
}




