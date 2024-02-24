#include "app_jt808.h"
#include "app_param.h"
#include "app_gps.h"
#include "app_net.h"
#include "app_sys.h"
#include "app_port.h"
#include "app_instructioncmd.h"
#include "app_socket.h"
#include "app_task.h"
#include "app_server.h"
#include "app_db.h"
static jt808Info_s jt808_info;
static jt808Position_s jt808_position;
static jt808TerminalRsp jt808_terminalRespon;
static gpsRestore_s jt808_gpsres;

extern void gpsRestoreSave(gpsRestore_s *gpsres);
/**************************************************
@bref		jt808��س�ʼ��
@param
@return
@note
**************************************************/

void jt808InfoInit(void)
{
    memset(&jt808_info, 0, sizeof(jt808Info_s));
    memset(&jt808_position, 0, sizeof(jt808Position_s));
    memset(&jt808_terminalRespon, 0, sizeof(jt808TerminalRsp));
    memset(&jt808_gpsres, 0, sizeof(gpsRestore_s));
}


/**************************************************
@bref		ʹ��IMEI����JT808�����к�
@param
	jt808sn		���ɵ�jt808sn
	imei		IMEI
	snlen		IMEILEN
@return
@note
**************************************************/

void jt808CreateSn(uint8_t *jt808sn, uint8_t *imei, uint8_t snlen)
{
    uint8_t i;
    uint8_t newsn[12];
    memset(newsn, 0, 12);
    if (snlen > 12)
    {
        snlen = 12;
    }
    for (i = 0; i < snlen; i++)
    {
        newsn[11 - i] = imei[snlen - 1 - i] - '0';
    }
    for (i = 0; i < 6; i++)
    {
        jt808sn[i] = newsn[i * 2];
        jt808sn[i] <<= 4;
        jt808sn[i] |= newsn[i * 2 + 1];
    }
}

/**************************************************
@bref		����jt808��״̬λ
@param
	bit
	onoff	1����λ	0����λ
@return
@note
**************************************************/

void jt808UpdateStatus(uint32_t bit, uint8_t onoff)
{
    if (onoff)
    {
        jt808_position.status |= bit;
    }
    else
    {
        jt808_position.status &= ~bit;
    }
}
/**************************************************
@bref		����jt808�ı���λ
@param
	bit
	onoff	1����λ	0����λ
@return
@note
**************************************************/

void jt808UpdateAlarm(uint32_t bit, uint8_t onoff)
{
    if (onoff)
    {
        jt808_position.alarm |= bit;
    }
    else
    {
        jt808_position.alarm &= ~bit;
    }
}

/**************************************************
@bref		ע��TCP�������ݺ���
@param
	tcpend	���ͻص�����
@return
@note
**************************************************/

void jt808RegisterTcpSend(int (*tcpSend)(uint8_t, uint8_t *, uint16_t))
{
    jt808_info.jt808Send = tcpSend;
}

/**************************************************
@bref		ע��JT808SN
@param
	sn			JT808�ն˺�
	reg			�Ƿ���ע��
	authCode 	��Ȩ��
	authLen	 	��Ȩ�볤��
@return
@note
**************************************************/

void jt808RegisterLoginInfo(uint8_t *sn, uint8_t reg, uint8_t *authCode, uint8_t authLen)
{
    char debugData[40];
    if (sn == NULL)
    {
        return;
    }

    memcpy((char *)jt808_info.jt808sn, (char *)sn, 6);
    byteToHexString(jt808_info.jt808sn, (uint8_t *)debugData, 6);
    debugData[12] = 0;
    LogPrintf(DEBUG_ALL, "JT808 Sn:%s", debugData);
    if (authCode != NULL)
    {
        memcpy(jt808_info.jt808AuthCode, authCode, 50);
    }
    jt808_info.jt808isRegister = reg;
    jt808_info.jt808AuthLen = authLen;
}

/**************************************************
@bref		ע��JT808 ����ID
@param
	id
@return
@note
**************************************************/

void jt808RegisterManufactureId(uint8_t *id)
{
    memcpy(jt808_info.jt808manufacturerID, id, 5);
}
/**************************************************
@bref		ע��JT808 �ն�����
@param
	id
@return
@note
**************************************************/

void jt808RegisterTerminalType(uint8_t *type)
{
    memcpy(jt808_info.jt808terminalType, type, 20);
}

/**************************************************
@bref		ע��JT808 �ն�ID
@param
	id
@return
@note
**************************************************/

void jt808RegisterTerminalId(uint8_t *id)
{
    memcpy(jt808_info.jt808terminalID, id, 7);
}



/**************************************************
@bref		tcp���ݷ���
@param
	link	��·ID
	data	����
	len		���ݳ���
@return
	1		���ͳɹ�
	0		����ʧ��
@note
**************************************************/

int jt808TcpSend(uint8_t *data, uint16_t len)
{
    int ret;
    if (jt808_info.jt808Send == NULL)
    {
        return 0;
    }
    ret = jt808_info.jt808Send(JT808_LINK, data, len);
    ret = ret > 0 ? 1 : 0;
    return ret;
}


/**************************************************
@bref		��ȡ��Ϣ��ˮ
@param
@return
	��Ϣ��ˮ
@note
**************************************************/
static uint16_t jt808GetSerial(void)
{
    return jt808_info.serial++;
}
/**************************************************
@bref		�����Ϣ������
@param
	subpackage	�Ƿ��зְ�
	encrypt		�Ƿ����
	msgLen		��Ϣ�峤��
@return
@note
**************************************************/

static uint16_t jt808PackMsgAttribute(uint8_t subpackage, uint8_t encrypt, uint16_t msgLen)
{
    uint16_t attr = 0;
    attr = msgLen & 0x3FF; // 0~9 bit Ϊ��Ϣ�峤��
    attr |= (encrypt & 0x07) << 10;
    attr |= (subpackage & 0x01) << 13;
    return attr;
}

/**************************************************
@bref		��װ��Ϣͷ
@param
	dest			���ݴ�ŵ�ַ
	msgId			��ϢID
	sn				�ն˺�
	serial			��ˮ��
	msgAttribute	��Ϣ������
@return
	���ݳ���
@note
**************************************************/

static uint16_t jt808PackMessageHead(uint8_t *dest, uint16_t msgId, uint8_t *sn, uint16_t serial, uint16_t msgAttribute)
{
    uint16_t len = 0;
    uint8_t i;
    //��־λ
    dest[len++] = 0x7E;
    /********��Ϣͷ*********/
    //��ϢID
    dest[len++] = (msgId >> 8) & 0xff;
    dest[len++] = msgId & 0xff;
    //��Ϣ������
    dest[len++] = (msgAttribute >> 8) & 0xff;
    dest[len++] = msgAttribute & 0xff;
    //�ն��ֻ���/�豸��
    for (i = 0; i < 6; i++)
    {
        dest[len++] = sn[i];
    }
    //��Ϣ��ˮ
    dest[len++] = (serial >> 8) & 0xff;
    dest[len++] = serial & 0xff;
    //��Ϣ����װ
    //��ʱ��
    /********��Ϣͷ*********/
    return len;
}

/**************************************************
@bref		����У��
@param
	dest			���ݻ�����
	len				���ݳ���
@return
	�����ܳ���
@note
**************************************************/

static uint16_t jt808PackMessageTail(uint8_t *dest, uint16_t len)
{
    uint8_t crc = 0;
    uint16_t i, attribute;

    attribute = jt808PackMsgAttribute(0, 0, len - 13);

    dest[3] = (attribute >> 8) & 0xff;
    dest[4] = attribute & 0xff;

    for (i = 1; i < len; i++)
    {
        crc ^= dest[i];
    }
    dest[len++] = crc;
    dest[len++] = 0x7E;
    return len;
}
/**************************************************
@bref		ת��
@param
	dest			���ݻ�����
	len				���ݳ���
@return
	�����ܳ���
@note
**************************************************/

static uint16_t jt808Escape(uint8_t *dest, uint16_t len)
{
    uint16_t i, j, k;
    uint16_t msgLen, debugLen;
    uint8_t senddata[301];
    msgLen = len;
    for (i = 1; i < msgLen - 1; i++)
    {
        if (dest[i] == 0x7E)
        {
            //ʣ������ݳ���j
            j = msgLen - 1 - i;
            //ʣ�������������1���ֽ�
            for (k = 0; k < j; k++)
            {
                dest[msgLen - k] = dest[msgLen - k - 1];
            }
            msgLen += 1;
            dest[i] = 0x7d;
            dest[i + 1] = 0x02;
        }
        else if (dest[i] == 0x7D)
        {
            //ʣ������ݳ���j
            j = msgLen - 1 - i;
            //ʣ�������������1���ֽ�
            for (k = 0; k < j; k++)
            {
                dest[msgLen - k] = dest[msgLen - k - 1];
            }
            msgLen += 1;
            dest[i] = 0x7d;
            dest[i + 1] = 0x01;
        }
    }

    debugLen = msgLen > 150 ? 150 : msgLen;
    byteToHexString(dest, senddata, debugLen);
    senddata[debugLen * 2] = 0;
    LogPrintf(DEBUG_ALL, "jt808Escape==> %s", senddata);
    return msgLen;
}

/**************************************************
@bref		��ת��
@param
	dest			���ݻ�����
	len				���ݳ���
@return
	�����ܳ���
@note
**************************************************/
static uint16_t jt808Unescape(uint8_t *src, uint16_t len)
{
    uint16_t i, j, k;
    for (i = 0; i < (len - 1); i++)
    {
        if (src[i] == 0x7d)
        {
            if (src[i + 1] == 0x02)
            {
                src[i] = 0x7e;
                j = len - (i + 2);
                for (k = 0; k < j; k++)
                {
                    src[i + 1 + k] = src[i + 2 + k];
                }
                len -= 1;
            }
            else if (src[i + 1] == 0x01)
            {
                src[i] = 0x7d;
                j = len - (i + 2);
                for (k = 0; k < j; k++)
                {
                    src[i + 1 + k] = src[i + 2 + k];
                }
                len -= 1;
            }
        }
    }
    return len;
}


/**************************************************
@bref		�ն�ע��
@param
	sn				�ն˺�
	manufacturerID
	terminalType
	terminalID
	color
@return
@note
**************************************************/

static void jt808TerminalRegister(uint8_t *sn, uint8_t *manufacturerID, uint8_t *terminalType, uint8_t *terminalID,
                                  uint8_t color)
{
    uint8_t dest[128];
    uint16_t len;
    uint8_t i;
    len = jt808PackMessageHead(dest, TERMINAL_REGISTER_MSGID, sn, jt808GetSerial(), 0);
    //ʡ��ID
    dest[len++] = 0x00;
    dest[len++] = 0x00;
    //����ID
    dest[len++] = 0x00;
    dest[len++] = 0x00;
    //������ID
    for (i = 0; i < 5; i++)
    {
        dest[len++] = manufacturerID[i];
    }
    //�ն��ͺ�
    for (i = 0; i < 20; i++)
    {
        dest[len++] = terminalType[i];
    }
    //�ն�ID
    for (i = 0; i < 7; i++)
    {
        dest[len++] = terminalID[i];
    }
    //������ɫ
    dest[len++] = color;
    len = jt808PackMessageTail(dest, len);
    len = jt808Escape(dest, len);
    jt808TcpSend(dest, len);
}

/**************************************************
@bref		�ն˼�Ȩ
@param
	sn				�ն˺�
	auth			��Ȩ��
	authlen			��Ȩ�볤��
@return
@note
**************************************************/
static void jt808TerminalAuthentication(uint8_t *sn, uint8_t *auth, uint8_t authlen)
{
    uint8_t dest[128];
    uint16_t len;
    uint8_t i;
    len = jt808PackMessageHead(dest, TERMINAL_AUTHENTICATION_MSGID, sn, jt808GetSerial(), 0);
    for (i = 0; i < authlen; i++)
    {
        dest[len++] = auth[i];
    }
    len = jt808PackMessageTail(dest, len);
    len = jt808Escape(dest, len);
    jt808TcpSend(dest, len);
}

/**************************************************
@bref		�ն�����
@param
	sn				�ն˺�
@return
@note
**************************************************/

static void jt808TerminalHeartbeat(uint8_t *sn)
{
    uint8_t dest[128];
    uint16_t len;
    len = jt808PackMessageHead(dest, TERMINAL_HEARTBEAT_MSGID, sn, jt808GetSerial(), 0);
    len = jt808PackMessageTail(dest, len);
    len = jt808Escape(dest, len);
    jt808TcpSend(dest, len);
}

/**************************************************
@bref		�������Լ�תΪbcd��
@param
	value	��ֵ
@return
	bcd����ֵ
@note
**************************************************/

static uint8_t byteToBcd(uint8_t Value)
{
    uint32_t bcdhigh = 0U;
    uint8_t Param = Value;

    while (Param >= 10U)
    {
        bcdhigh++;
        Param -= 10U;
    }

    return ((uint8_t)(bcdhigh << 4U) | Param);
}

/**************************************************
@bref		���λ����Ϣ
@param
	gpsinfo	gps��Ϣ
@return
@note
**************************************************/

static void jt808PackGpsinfo(gpsinfo_s *gpsinfo)
{
    double value;
    datetime_s datetimenew;
    /*--------------γ��-----------------*/
    value = gpsinfo->latitude;
    value *= 1000000;
    if (value < 0)
    {
        value *= -1;
    }
    jt808_position.latitude = value;
    /*--------------����-----------------*/
    value = gpsinfo->longtitude;
    value *= 1000000;
    if (value < 0)
    {
        value *= -1;
    }
    jt808_position.longtitude = value;
    /*--------------����-----------------*/
    jt808_position.course = gpsinfo->course;
    jt808_position.speed = (uint16_t)(gpsinfo->speed * 10);
    jt808_position.hight = gpsinfo->hight;
    jt808_position.statelliteUsed = gpsinfo->used_star;
    /*--------------����ʱ��-----------------*/
    datetimenew = changeUTCTimeToLocalTime(gpsinfo->datetime, 8);
    jt808_position.year = byteToBcd(datetimenew.year % 100);
    jt808_position.month = byteToBcd(datetimenew.month);
    jt808_position.day = byteToBcd(datetimenew.day);
    jt808_position.hour = byteToBcd(datetimenew.hour);
    jt808_position.mintues = byteToBcd(datetimenew.minute);
    jt808_position.seconds = byteToBcd(datetimenew.second);
    /*--------------״̬λ-----------------*/
    if (gpsinfo->fixstatus)
    {
        jt808UpdateStatus(JT808_STATUS_FIX, 1);
    }
    else
    {
        jt808UpdateStatus(JT808_STATUS_FIX, 0);
    }

    if (gpsinfo->NS == 'S')
    {
        jt808UpdateStatus(JT808_STATUS_LATITUDE, 1);
    }
    else
    {
        jt808UpdateStatus(JT808_STATUS_LATITUDE, 0);
    }

    if (gpsinfo->EW == 'W')
    {
        jt808UpdateStatus(JT808_STATUS_LONGTITUDE, 1);
    }
    else
    {
        jt808UpdateStatus(JT808_STATUS_LONGTITUDE, 0);
    }
    if (gpsinfo->gpsviewstart > 0)
    {
        jt808UpdateStatus(JT808_STATUS_USE_GPS, 1);
    }
    else
    {
        jt808UpdateStatus(JT808_STATUS_USE_GPS, 0);
    }
    if (gpsinfo->beidouviewstart > 0)
    {
        jt808UpdateStatus(JT808_STATUS_USE_BEIDOU, 1);
    }
    else
    {
        jt808UpdateStatus(JT808_STATUS_USE_BEIDOU, 0);
    }
    if (gpsinfo->glonassviewstart > 0)
    {
        jt808UpdateStatus(JT808_STATUS_USE_GLONASS, 1);
    }
    else
    {
        jt808UpdateStatus(JT808_STATUS_USE_GLONASS, 0);
    }
    /*-------------------------------*/
}

static uint16_t jt808PositionInfo(uint8_t *dest, uint16_t len, jt808Position_s *positionInfo)
{
    //����
    dest[len++] = (positionInfo->alarm >> 24) & 0xFF;
    dest[len++] = (positionInfo->alarm >> 16) & 0xFF;
    dest[len++] = (positionInfo->alarm >> 8) & 0xFF;
    dest[len++] = (positionInfo->alarm) & 0xFF;
    //LogPrintf(DEBUG_ALL,"Alarm 0x%08X\r\n",positionInfo->alarm);
    //״̬
    dest[len++] = (positionInfo->status >> 24) & 0xFF;
    dest[len++] = (positionInfo->status >> 16) & 0xFF;
    dest[len++] = (positionInfo->status >> 8) & 0xFF;
    dest[len++] = (positionInfo->status) & 0xFF;
    //LogPrintf(DEBUG_ALL,"Status 0x%08X\r\n",positionInfo->status);
    //ά��
    dest[len++] = (positionInfo->latitude >> 24) & 0xFF;
    dest[len++] = (positionInfo->latitude >> 16) & 0xFF;
    dest[len++] = (positionInfo->latitude >> 8) & 0xFF;
    dest[len++] = (positionInfo->latitude) & 0xFF;
    //����
    dest[len++] = (positionInfo->longtitude >> 24) & 0xFF;
    dest[len++] = (positionInfo->longtitude >> 16) & 0xFF;
    dest[len++] = (positionInfo->longtitude >> 8) & 0xFF;
    dest[len++] = (positionInfo->longtitude) & 0xFF;
    //�߶�
    dest[len++] = (positionInfo->hight >> 8) & 0xFF;
    dest[len++] = (positionInfo->hight) & 0xFF;
    //�ٶ�
    dest[len++] = (positionInfo->speed >> 8) & 0xFF;
    dest[len++] = (positionInfo->speed) & 0xFF;
    //����
    dest[len++] = (positionInfo->course >> 8) & 0xFF;
    dest[len++] = (positionInfo->course) & 0xFF;
    //ʱ��
    dest[len++] = positionInfo->year;
    dest[len++] = positionInfo->month;
    dest[len++] = positionInfo->day;
    dest[len++] = positionInfo->hour;
    dest[len++] = positionInfo->mintues;
    dest[len++] = positionInfo->seconds;
    return len;
}


/**************************************************
@bref		λ����Ϣ�ϱ�
@param
	dest			���ݴ�ŵ�ַ
	sn				�ն˺�
	positionInfo	λ������
	type			ʹ������
		1������
		0������
@return
@note
**************************************************/

static uint16_t jt808TerminalPosition(uint8_t *dest, uint8_t *sn, jt808Position_s *positionInfo, uint8_t type)
{
    uint16_t len;

    len = jt808PackMessageHead(dest, TERMINAL_POSITION_MSGID, sn, jt808GetSerial(), 0);
    len = jt808PositionInfo(dest, len, positionInfo);
    if (type == 1)
    {
        /*------------------������Ϣ--------------------*/
        //�����ź�ǿ��
        dest[len++] = 0x30;
        dest[len++] = 0x01;
        dest[len++] = getModuleRssi();
        //GNSS��λ������
        dest[len++] = 0x31;
        dest[len++] = 0x01;
        dest[len++] = positionInfo->statelliteUsed;
    }

    len = jt808PackMessageTail(dest, len);
    len = jt808Escape(dest, len);
    return len;
}
/**************************************************
@bref		��gpsRestore_s����תΪgpsinfo_s����
@param
	gpsinfo
	grs
@return
@note
**************************************************/

static void jt808gpsRestoreChange(gpsinfo_s *gpsinfo, gpsRestore_s *grs)
{
    uint32_t value;
    memset(gpsinfo, 0, sizeof(gpsinfo_s));
    gpsinfo->datetime.year = grs->year;
    gpsinfo->datetime.month = grs->month;
    gpsinfo->datetime.day = grs->day;
    gpsinfo->datetime.hour = grs->hour;
    gpsinfo->datetime.minute = grs->minute;
    gpsinfo->datetime.second = grs->second;

    value = (grs->latititude[0] << 24) | (grs->latititude[1] << 16) | (grs->latititude[2] << 8) | (grs->latititude[3]);
    gpsinfo->latitude = (double)value / 1000000.0;
    value = (grs->longtitude[0] << 24) | (grs->longtitude[1] << 16) | (grs->longtitude[2] << 8) | (grs->longtitude[3]);
    gpsinfo->longtitude = (double)value / 1000000.0;

    gpsinfo->speed = grs->speed;
    gpsinfo->course = (grs->coordinate[0] << 8) | grs->coordinate[1];
    gpsinfo->NS = grs->temp[0];
    gpsinfo->EW = grs->temp[1];
    gpsinfo->fixstatus = 1;
}
/**************************************************
@bref		�켣����
@param
	dest	���ݴ����
	grs		����gps����
@return
@note
**************************************************/

uint16_t jt808gpsRestoreDataSend(uint8_t *dest, gpsRestore_s *grs)
{
    gpsinfo_s gpsinfo;
    uint16_t len = 0;
    jt808gpsRestoreChange(&gpsinfo, grs);
    jt808PackGpsinfo(&gpsinfo);
    len = jt808TerminalPosition(dest, jt808_info.jt808sn, &jt808_position, 0);
    return len;
}

/**************************************************
@bref		gps��ϢתΪ�����ʽ
@param
@return
@note
**************************************************/

static void jt808gpsRestoreSave(gpsinfo_s *gpsinfo)
{
    jt808_gpsres.year = gpsinfo->datetime.year;
    jt808_gpsres.month = gpsinfo->datetime.month;
    jt808_gpsres.day = gpsinfo->datetime.day;
    jt808_gpsres.hour = gpsinfo->datetime.hour;
    jt808_gpsres.minute = gpsinfo->datetime.minute;
    jt808_gpsres.second = gpsinfo->datetime.second;
    jt808_gpsres.speed = gpsinfo->speed;
    jt808_gpsres.coordinate[0] = (gpsinfo->course >> 8) & 0xff;
    jt808_gpsres.coordinate[1] = (gpsinfo->course) & 0xff;
    jt808_gpsres.temp[0] = gpsinfo->NS;
    jt808_gpsres.temp[1] = gpsinfo->EW;
    jt808_gpsres.latititude[0] = (jt808_position.latitude >> 24) & 0xff;
    jt808_gpsres.latititude[1] = (jt808_position.latitude >> 16) & 0xff;
    jt808_gpsres.latititude[2] = (jt808_position.latitude >> 8) & 0xff;
    jt808_gpsres.latititude[3] = (jt808_position.latitude) & 0xff;
    jt808_gpsres.longtitude[0] = (jt808_position.longtitude >> 24) & 0xff;
    jt808_gpsres.longtitude[1] = (jt808_position.longtitude >> 16) & 0xff;
    jt808_gpsres.longtitude[2] = (jt808_position.longtitude >> 8) & 0xff;
    jt808_gpsres.longtitude[3] = (jt808_position.longtitude) & 0xff;
}

/**************************************************
@bref		λ����Ϣ�ϱ�
@param
	gpsinfo			λ����Ϣ
@return
@note
**************************************************/

static void jt808SendPosition(uint8_t *sn, gpsinfo_s *gpsinfo)
{
    int ret = 0;
    uint8_t dest[200];
    uint8_t len;
    if (gpsinfo == NULL)
        return;
    jt808PackGpsinfo(gpsinfo);
    jt808gpsRestoreSave(gpsinfo);

    len = jt808TerminalPosition(dest, sn, &jt808_position, 1);

    if (primaryServerIsReady() && getTcpNack() == 0)
    {
        ret = jt808TcpSend(dest, len);
    }

    if (ret == 0 && gpsinfo->fixstatus && gpsinfo->hadupload == 0)
    {
        gpsRestoreSave(&jt808_gpsres);
    }
    gpsinfo->hadupload = 1;
}

/**************************************************
@bref		��λ���������ϴ�
@param
	sn				�ն����
	positionInfo	λ����Ϣ
	num				λ����Ϣ����
	type			����
		0������λ�������ϴ�
		1��ä������
@return
@note
**************************************************/

static void jt808BatchUpload(uint8_t *sn, batch_upload_s *bus)
{
    gpsinfo_s gpsinfo;
    uint8_t dest[1024], i;
    uint16_t len;
    if (bus == NULL || bus->grs == NULL || bus->num == 0)
    {
        return;
    }
    len = jt808PackMessageHead(dest, TERMINAL_BATCHUPLOAD_MSG, sn, jt808GetSerial(), 0);
    dest[len++] = (bus->num >> 8) & 0xFF;
    dest[len++] = (bus->num) & 0xFF;
    dest[len++] = bus->type;
    for (i = 0; i < bus->num; i++)
    {
        dest[len++] = 0;
        dest[len++] = 28;
        jt808gpsRestoreChange(&gpsinfo, &bus->grs[i]);
        jt808PackGpsinfo(&gpsinfo);
        len = jt808PositionInfo(dest, len, &jt808_position);
    }
    len = jt808PackMessageTail(dest, len);
    len = jt808Escape(dest, len);
    jt808TcpSend(dest, len);
}


/**************************************************
@bref		��ICCIDתΪBCD�����
@param
@return
@note
**************************************************/

static void jt808ICCIDChangeHexToBcd(uint8_t *bcdiccd, uint8_t *hexiccid)
{
    uint8_t i;
    uint8_t iccidbuff[20];
    for (i = 0; i < 20; i++)
    {
        if (hexiccid[i] >= 'A' && hexiccid[i] <= 'F')
        {
            iccidbuff[i] = hexiccid[i] - 'A';

        }
        else if (hexiccid[i] >= 'a' && hexiccid[i] <= 'f')
        {
            iccidbuff[i] = hexiccid[i] - 'a';

        }
        else if (hexiccid[i] >= '0' && hexiccid[i] <= '9')
        {
            iccidbuff[i] = hexiccid[i] - '0';
        }

    }
    for (i = 0; i < 10; i++)
    {
        bcdiccd[i] = iccidbuff[i * 2];
        bcdiccd[i] <<= 4;
        bcdiccd[i] |= iccidbuff[i * 2 + 1];
    }
}

/**************************************************
@bref		�ն�����
@param
	sn				�ն˺�
	manufacturerID
	terminalType
	terminalID
	hexiccid
@return
@note
**************************************************/


static void jt808TerminalAttribute(uint8_t *sn, uint8_t *manufacturerID, uint8_t *terminalType, uint8_t *terminalID,
                                   uint8_t *hexiccid)
{
    uint8_t dest[128], i;
    uint16_t len;
    len = jt808PackMessageHead(dest, TERMINAL_ATTRIBUTE_MSGID, sn, jt808GetSerial(), 0);
    //�ն�����
    dest[len++] = 0x05;
    dest[len++] = 0x00;
    //������ID
    for (i = 0; i < 5; i++)
    {
        dest[len++] = manufacturerID[i];
    }
    //�ն��ͺ�
    for (i = 0; i < 20; i++)
    {
        dest[len++] = terminalType[i];
    }
    //�ն�ID
    for (i = 0; i < 7; i++)
    {
        dest[len++] = terminalID[i];
    }
    //ICCID
    jt808ICCIDChangeHexToBcd(dest + len, hexiccid);
    len += 10;
    //�ն�Ӳ���汾����
    dest[len++] = 2;
    //Ӳ���汾��
    dest[len++] = 0x30;
    dest[len++] = 0x31;
    //�ն˹̼��汾����
    dest[len++] = 2;
    //�̼��汾��
    dest[len++] = 0x32;
    dest[len++] = 0x33;
    //GNSSģ������
    dest[len++] = 0x0f;
    //ͨ��ģ������
    dest[len++] = 0xff;

    len = jt808PackMessageTail(dest, len);
    len = jt808Escape(dest, len);
    jt808TcpSend(dest, len);
}


/**************************************************
@bref		�ն�ͨ�ûظ�
@param
	sn				�ն˺�
	param			�ظ���Ϣ
@return
@note
**************************************************/

static void jt808GenericRespon(uint8_t *sn, jt808TerminalRsp *param)
{
    uint8_t dest[128];
    uint16_t len;
    len = jt808PackMessageHead(dest, TERMINAL_GENERIC_RESPON_MSGID, sn, jt808GetSerial(), 0);
    dest[len++] = (param->platform_serial >> 8) & 0xff;
    dest[len++] = param->platform_serial & 0xff;
    dest[len++] = (param->platform_id >> 8) & 0xff;
    dest[len++] = param->platform_id & 0xff;
    dest[len++] = param->result;
    len = jt808PackMessageTail(dest, len);
    len = jt808Escape(dest, len);
    jt808TcpSend(dest, len);
}

/**************************************************
@bref		F8�ظ�
@param
@return
@note
**************************************************/

static uint16_t transmissionF8(uint8_t *dest, uint16_t len, void *param, jt808TerminalRsp *jtgr)
{
    uint8_t *msg;
    uint16_t i, msglen;
    msg = (uint8_t *)param;
    msglen = strlen((char *)msg);
    dest[len++] = TRANSMISSION_TYPE_F8;
    dest[len++] = (jtgr->platform_serial >> 8) & 0xff;
    dest[len++] = jtgr->platform_serial & 0xff;
    for (i = 0; i < msglen; i++)
    {
        dest[len++] = msg[i];
    }
    return len;
}

/**************************************************
@bref		͸���ظ�
@param
	sn				�ն˺�
	param			�ظ���Ϣ
@return
@note
**************************************************/


static void jt808TranmissionData(uint8_t *sn, void *param)
{
    uint8_t dest[400];
    uint16_t len;
    trammission_s *transmission;
    transmission = (trammission_s *)param;
    len = jt808PackMessageHead(dest, TERMINAL_DATAUPLOAD_MSGID, sn, jt808GetSerial(), 0);
    switch (transmission->id)
    {
        case TRANSMISSION_TYPE_F8:
            len = transmissionF8(dest, len, transmission->param, &jt808_terminalRespon);
            break;
    }

    len = jt808PackMessageTail(dest, len);
    len = jt808Escape(dest, len);
    jt808TcpSend(dest, len);
}

/**************************************************
@bref		jt808Э�鷢��
@param
	sn				�ն˺�
	param			�ظ���Ϣ
@return
@note
**************************************************/

void jt808SendToServer(JT808_PROTOCOL protocol, void *param)
{
    char *iccid;

    if (sysparam.protocol != JT808_PROTOCOL_TYPE)
    {
        return;
    }

    switch (protocol)
    {
        case TERMINAL_REGISTER:
            jt808TerminalRegister(jt808_info.jt808sn, jt808_info.jt808manufacturerID, jt808_info.jt808terminalType,
                                  jt808_info.jt808terminalID, 0);
            break;
        case TERMINAL_AUTH:
            jt808TerminalAuthentication(jt808_info.jt808sn, jt808_info.jt808AuthCode, jt808_info.jt808AuthLen);
            break;
        case TERMINAL_HEARTBEAT:
            jt808TerminalHeartbeat(jt808_info.jt808sn);
            break;
        case TERMINAL_POSITION:
            jt808SendPosition(jt808_info.jt808sn, (gpsinfo_s *)param);
            break;
        case TERMINAL_ATTRIBUTE:
            iccid = getModuleICCID();
            jt808TerminalAttribute(jt808_info.jt808sn, jt808_info.jt808manufacturerID, jt808_info.jt808terminalType,
                                   jt808_info.jt808terminalID, (uint8_t *)iccid);
            break;
        case TERMINAL_GENERICRESPON:
            jt808GenericRespon(jt808_info.jt808sn, (jt808TerminalRsp *)param);
            break;
        case TERMINAL_DATAUPLOAD:
            jt808TranmissionData(jt808_info.jt808sn, param);
            break;
        case TERMINAL_BATCHUPLOAD:
            jt808BatchUpload(jt808_info.jt808sn, (batch_upload_s *)param);
            break;
    }


}

/**************************************************
@bref		jt808ָ��ظ�
@param
	buf		ָ��ظ�����
	len		���ݳ���
@return
@note
**************************************************/

void jt808MessageSend(uint8_t *buf, uint16_t len)
{
    trammission_s transmission;
    transmission.id = TRANSMISSION_TYPE_F8;
    transmission.param = buf;
    jt808SendToServer(TERMINAL_DATAUPLOAD, &transmission);
}

/**************************************************
@bref		jt808�ն�ע��ظ�
@param
	msg		��Ϣ������
	len		���ݳ���
@return
@note
**************************************************/
static void jt808RegisterResponParser(uint8_t *msg, uint16_t len)
{
    uint8_t  result, j;
    result = msg[2];
    if (result == 0)
    {
        for (j = 0; j < (len - 3); j++)
        {
            jt808_info.jt808AuthCode[j] = msg[j + 3];
        }
        jt808_info.jt808AuthLen = len - 3;
        jt808_info.jt808isRegister = 1;
        //����ע����Ϣ
        dynamicParam.jt808isRegister = 1;
        dynamicParam.jt808AuthLen = jt808_info.jt808AuthLen;
        memcpy(dynamicParam.jt808AuthCode, jt808_info.jt808AuthCode, jt808_info.jt808AuthLen);
        dynamicParamSaveAll();
        LogMessage(DEBUG_ALL, "Register OK");
    }
}

/**************************************************
@bref		jt808�ն˿���s
@param
	msg		��������
	len		���ݳ���
@return
@note
**************************************************/

static void jt808TerminalCtrlParser(uint8_t *msg, uint16_t len)
{
    LogPrintf(DEBUG_ALL, "8105 CmdType:0x%02X", msg[0]);
    jt808_terminalRespon.result = 0;
    jt808SendToServer(TERMINAL_GENERICRESPON, &jt808_terminalRespon);
}

/**************************************************
@bref		jt808�ն�͸��F8������Ϣ
@param
	msg		��������
	len		���ݳ���
@return
@note
**************************************************/

static void jt808ReceiveF8(uint8_t *msg, uint16_t len)
{
    insParam_s insParam;
    memset(&insParam, 0, sizeof(insParam_s));
    insParam.link = JT808_LINK;
    instructionParser(msg, len, JT808_MODE, &insParam);

}

/**************************************************
@bref		jt808�ն�͸������
@param
	msg		��������
	len		���ݳ���
@return
@note
**************************************************/

static void jt808DownlinkParser(uint8_t *msg, uint16_t len)
{
    LogPrintf(DEBUG_ALL, "8900 MsgType:0x%02X", msg[0]);
    switch (msg[0])
    {
        case TRANSMISSION_TYPE_F8:
            jt808ReceiveF8(msg + 1, len - 1);
            break;
    }
}

/**************************************************
@bref		jt808ƽ̨ͨ�ûظ�
@param
	msg		��������
	len		���ݳ���
@return
@note
**************************************************/

static void jt808GenericResponParser(uint8_t *msg, uint16_t len)
{
    uint16_t protocol;
    uint8_t result;
    protocol = msg[2] << 8 | msg[3];
    result = msg[4];
    LogMessage(DEBUG_ALL, "Generic Respon");
	hbtRspSuccess();
    switch (protocol)
    {
        case TERMINAL_AUTHENTICATION_MSGID:
            if (result == 0)
            {
                jt808ServerAuthSuccess();
                LogMessage(DEBUG_ALL, "Authtication success");
            }
            else
            {
                LogPrintf(DEBUG_ALL, "Authtication Fail,result:%d", result);
            }
            break;
    }
}


/**************************************************
@bref		jt808�ն˲�������
@param
	msg		��������
	len		���ݳ���
@return
@note 7E 8103 000A 784086963657 000B 01 00000001 04 000000B4 7A7E
**************************************************/

static void jt808SetTerminalParamParser(uint8_t *msg, uint16_t len)
{
    uint8_t paramCount, paramLen, i;
    uint16_t j;
    uint32_t paramId, value;
    j = 0;
    paramCount = msg[j++];
    for (i = 0; i < paramCount; i++)
    {

        paramId = msg[j++];
        paramId = (paramId << 8) | msg[j++];
        paramId = (paramId << 8) | msg[j++];
        paramId = (paramId << 8) | msg[j++];
        paramLen = msg[j++];


        LogPrintf(DEBUG_ALL, "ParamId 0x%04X,ParamLen=%d", paramId, paramLen);
        switch (paramId)
        {
            //�����������
            case 0x0001:
                value = msg[j++];
                value = (value << 8) | msg[j++];
                value = (value << 8) | msg[j++];
                value = (value << 8) | msg[j++];
                sysparam.heartbeatgap = value;
                paramSaveAll();
                LogPrintf(DEBUG_ALL, "Update heartbeat to %d", sysparam.heartbeatgap);
                j += paramLen;
                break;
            default:
                j += paramLen;
                break;
        }
    }
    jt808_terminalRespon.result = 0;
    jt808SendToServer(TERMINAL_GENERICRESPON, &jt808_terminalRespon);
}


/**************************************************
@bref		jt808�ı���Ϣ�·�
@param
	msg		��������
	len		���ݳ���
@return
@note
**************************************************/

static void jt808getTextMessageParser(uint8_t *msg, uint16_t len)
{
    //    uint8_t content[128];
    //    LogPrintf(DEBUG_ALL, "Msg type %d", msg[0]);
    //    strncpy((char *)content, (char *)(msg + 1), len - 1);
    //    content[len - 1] = 0;
    //    LogPrintf(DEBUG_ALL, "Get Msg:%s", content);
}

/**************************************************
@bref		jt808Э���������
@param
	msg		��Ϣ������
	len		��Ϣ�峤��
@return
@note
**************************************************/

static void jt808ProtocolParser(uint16_t protocol, uint8_t *msg, uint16_t len)
{
    switch (protocol)
    {
        case PLATFORM_GENERIC_RESPON_MSGID:
            jt808GenericResponParser(msg, len);
            break;
        case PLATFORM_REGISTER_RESPON_MSGID:
            jt808RegisterResponParser(msg, len);
            break;
        case PLATFORM_TERMINAL_CTL_MSGID:
            jt808TerminalCtrlParser(msg, len);
            break;
        case PLATFORM_DOWNLINK_MSGID:
            jt808DownlinkParser(msg, len);
            break;
        case PLATFORM_TERMINAL_SET_PARAM_MSGID:
            jt808SetTerminalParamParser(msg, len);
            break;
        case PLATFORM_QUERY_ATTR_MSGID:
            jt808SendToServer(TERMINAL_ATTRIBUTE, NULL);
            break;
        case PLATFORM_TEXT_MSGID:
            jt808getTextMessageParser(msg, len);
            break;
    }
}


/**************************************************
@bref		Э�������
@param
	src		�յ�����������
	len		�յ����������ݳ���
@return
@note
**************************************************/

void jt808ReceiveParser(uint8_t *src, uint16_t len)
{
    uint16_t i, j, k;
    uint16_t msglen, crc = 0, protocol;
    len = jt808Unescape(src, len);
    for (i = 0; i < len; i++)
    {
        if (i + 15 > len)
        {
            return ;
        }
        if (src[i] == 0x7E)
        {
            msglen = src[i + 3] << 8 | src[i + 4];
            msglen &= 0x3FF;
            k = i + 13 + msglen;
            if ((k + 1) > len)
            {
                return ;
            }
            if (src[k + 1] != 0x7E)
            {
                LogPrintf(DEBUG_ALL, "JT808 not find tail,it was 0x%02X,k=%d", src[k + 1], k);
                continue ;
            }
            crc = 0;
            for (j = 1; j < (k - i); j++)
            {
                crc ^= src[i + j];
            }
            if (crc == src[k])
            {
                protocol = src[i + 1] << 8 | src[i + 2];
                jt808_terminalRespon.platform_id = protocol;
                jt808_terminalRespon.platform_serial = src[i + 11] << 8 | src[i + 12];
                jt808ProtocolParser(protocol, src + i + 13, msglen);
                i = k + 1;
            }
        }
    }
}
