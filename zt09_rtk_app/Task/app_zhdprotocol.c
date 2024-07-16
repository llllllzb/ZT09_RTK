/*
 * app_zhdprotocol.c
 *
 *  Created on: Jul 1, 2024
 *      Author: lvvv
 *
 * �к����tcpЭ��
 * С��ģʽ
 *
 * 1.LG:��½Э��
 * 2.GP:��λЭ��
 */
#include "app_zhdprotocol.h"

static zhd_proinfo_lg_s zhd_lg;
static zhd_proinfo_position_s zhd_gp;
static zhd_tcp_send_handle_t zhd_tcp_send_handle = NULL;
static gpsRestore_s zhd_gpsres;


/**************************************************
@bref		�к���Э���ʼ��
@param
@return
@note
**************************************************/
void zhd_protocol_init(void)
{
	tmos_memset(&zhd_lg, 0, sizeof(zhd_proinfo_lg_s));
	tmos_memset(&zhd_gp, 0, sizeof(zhd_proinfo_position_s));
	tmos_memset(&zhd_gpsres, 0, sizeof(gpsRestore_s));
}

/**************************************************
@bref		�к���Э������debug
@param
@return
@note
**************************************************/
static void zhd_data_debug(uint8_t *mark, char *txdata, int txlen)
{
	int debuglen;
    char srclen;
    char senddata[300] = { 0 };
    if (strlen(mark) > 43)
    {
		LogPrintf(DEBUG_ALL, "%s==>mark error", __FUNCTION__);
		return;
    }
    debuglen = txlen > 128 ? 128 : txlen;
    sprintf(senddata, "[%s]", mark);
    srclen = tmos_strlen(senddata);
    byteToHexString((uint8_t *)txdata, (uint8_t *)senddata + srclen, (uint16_t) debuglen);
    senddata[srclen + debuglen * 2] = 0;
    LogMessage(DEBUG_ALL, senddata);
}

/**************************************************
@bref		�к���Э��tcp���ͺ���ע��
@param
@return
@note
**************************************************/
void zhd_tcp_send_handle_register(int (* handle)(uint8_t, uint8_t *, uint16_t))
{	
	zhd_tcp_send_handle = handle;
}

/**************************************************
@bref		�к���tcp���ݷ���
@param
	link	��·ID
	data	����
	len		���ݳ���
@return
	1		���ͳɹ�
	0		����ʧ��
@note
**************************************************/
int zhd_tcp_send(uint8_t link, uint8_t *data, uint16_t len)
{
	int ret;
	if (zhd_tcp_send_handle == NULL)
		return 0;
	ret = zhd_tcp_send_handle(link, data, len);
	return ret;
}

/**************************************************
@bref		�������
@param
@return
@note
��������ΪGkqdSirj
**************************************************/
void encryption_by_mask(const uint8_t *mask, uint8_t *str)
{
	uint8_t i, mask_len;
	if (NULL == mask)
	{
		LogPrintf(DEBUG_ALL, "%s==>no mask", __FUNCTION__);
		return;
	}
	if (strlen(mask) != strlen(str))
	{
		LogPrintf(DEBUG_ALL, "%s==>length mismatch,mask:%d,str:%d", __FUNCTION__, strlen(mask), strlen(str));
		return;
	}
	mask_len = strlen(mask);
	for (i = 0; i < mask_len; i++)
	{
		str[i] ^= mask[i];
	}
	zhd_data_debug("enc", str, mask_len);
}

/**************************************************
@bref		�к����¼Э����Ϣע��
@param
@return
@note
**************************************************/
void zhd_lg_info_register(uint8_t *sn, uint8_t *user, uint8_t *password)
{
	uint8_t i, info_len;
	uint8_t pdu_pwsd[ZHD_LG_PASSWORD_LEN] = { 0 };
	uint8_t pdu_sn[10] = {0}, pdu_user[10] = {0}, pdu_pswd[10] = {0};
	//sn
	info_len = strlen(sn);
	for (i = 0; i < ZHD_LG_SN_LEN; i++)
	{
		zhd_lg.sn[i] = (i < info_len) ? sn[i] : 0x20;
	}
	//user
	info_len = strlen(user);
	for (i = 0; i < ZHD_LG_USER_LEN; i++)
	{
		zhd_lg.user[i] = (i < info_len) ? user[i] : 0x20;
	}
	//user
	info_len = strlen(password);
	for (i = 0; i < ZHD_LG_PASSWORD_LEN; i++)
	{
		pdu_pwsd[i] = (i < info_len) ? password[i] : 0x20;
	}
	encryption_by_mask(ZHD_MASK, pdu_pwsd);
	strncpy(zhd_lg.password, pdu_pwsd, ZHD_LG_PASSWORD_LEN);
	//DEBUG
	tmos_memcpy(pdu_sn, zhd_lg.sn, ZHD_LG_SN_LEN);
	pdu_sn[ZHD_LG_SN_LEN] = 0;
	tmos_memcpy(pdu_user, zhd_lg.user, ZHD_LG_USER_LEN);
	pdu_user[ZHD_LG_USER_LEN] = 0;
	tmos_memcpy(pdu_pswd, zhd_lg.password, ZHD_LG_PASSWORD_LEN);
	pdu_pswd[ZHD_LG_PASSWORD_LEN] = 0;
	LogPrintf(DEBUG_ALL, "%s==>sn[%s] user[%s] password[%s]", __FUNCTION__, pdu_sn, pdu_user, pdu_pswd);
//	zhd_data_debug("lg_sn", zhd_lg.sn, ZHD_LG_SN_LEN);
//	zhd_data_debug("lg_user", zhd_lg.user, ZHD_LG_USER_LEN);
//	zhd_data_debug("lg_password", zhd_lg.password, ZHD_LG_PASSWORD_LEN);
}

/**************************************************
@bref		�к����¼Э������
@param
@return
@note
**************************************************/
static uint16_t zhd_protocol_lg_package(uint8_t *dest)
{
	int pdu_len = 0, i, ret;
	uint8_t crc;
	//Э��ͷ$$
	dest[pdu_len++] = 0x24;
	dest[pdu_len++] = 0x24;
	//������
	dest[pdu_len++] = (ZHD_PROTOCOL_LG >> 8) & 0xff;
	dest[pdu_len++] = ZHD_PROTOCOL_LG & 0xff;
	//����
	dest[pdu_len++] = 24;
	dest[pdu_len++] = 0;
	//data
	//sn
	for (i = 0; i < ZHD_LG_SN_LEN; i++)
	{
		dest[pdu_len++] = zhd_lg.sn[i];
	}
	//user
	for (i = 0; i < ZHD_LG_USER_LEN; i++)
	{
		dest[pdu_len++] = zhd_lg.user[i];
	}
	//password
	for (i = 0; i < ZHD_LG_PASSWORD_LEN; i++)
	{
		dest[pdu_len++] = zhd_lg.password[i];
	}
	//#
	dest[pdu_len++] = 0x23;
	//crc
	crc = dest[2];
	XOR_CRC_TOTAL(dest + 3, pdu_len - 4, crc);
	dest[pdu_len++] = crc;
	//Э��β0D0A
	dest[pdu_len++] = 0x0D;
	dest[pdu_len++] = 0x0A;
	LogPrintf(DEBUG_ALL, "ZHD_TCP(%d)send:", ZHD_LINK);
	zhd_data_debug("data", dest, pdu_len);
	return pdu_len;
}

/**************************************************
@bref		�к����¼Э�鷢��
@param
@return
@note
**************************************************/
static void zhd_protocol_lg_send(void)
{
	uint8_t dest[128] = { 0 };
	uint16_t len;
	len = zhd_protocol_lg_package(dest);
	zhd_tcp_send(ZHD_LINK, dest, len);	
}

/**************************************************
@bref		��gps��Ϣ����
@param
@return
@note
�������ڲ���
temp[0] temp[1] temp[2] temp[3]���ڴ洢�߳�
temp[4]���ڴ洢��״̬
temp[5] temp[6] temp[7] temp[8]���ڴ洢�ٶ�
temp[9]���ڴ洢����
**************************************************/
static void zhd_gp_restore(gpsinfo_s *gpsinfo, uint8_t level)
{
	float speed;
	zhd_gpsres.year   = gpsinfo->datetime.year;
	zhd_gpsres.month  = gpsinfo->datetime.month;
	zhd_gpsres.day    = gpsinfo->datetime.day;
	zhd_gpsres.hour   = gpsinfo->datetime.hour;
	zhd_gpsres.minute = gpsinfo->datetime.minute;
	zhd_gpsres.second = gpsinfo->datetime.second;
	tmos_memcpy(zhd_gpsres.latititude, &gpsinfo->latitude, sizeof(double));		//8
	tmos_memcpy(zhd_gpsres.longtitude, &gpsinfo->longtitude, sizeof(double));	//8
	tmos_memcpy(zhd_gpsres.temp, &gpsinfo->hight, sizeof(float));				//4
	zhd_gpsres.temp[4] = gpsinfo->fixAccuracy;									//1
	speed = (float)gpsinfo->speed;
	tmos_memcpy(zhd_gpsres.temp + 5, &speed, sizeof(float));					//4
	zhd_gpsres.temp[9] = level;													//1
}

/**************************************************
@bref		�к���λ����Ϣע��
@param
@return
@note
**************************************************/
void zhd_gp_info_register(gpsinfo_s *gpsinfo, uint8_t level)
{
	double value;
	datetime_s datetimenew;
	uint8_t i = 0;
	//�γɴ洢��ʽ��gps����
	zhd_gp_restore(gpsinfo, level);
	
	value = gpsinfo->latitude;
	if (value < 0)
		value *= -1;
	zhd_gp.latitude = value;
	value = gpsinfo->longtitude;
	if (value < 0)
		value *= -1;
	zhd_gp.longitude = value;
	zhd_gp.altitude  = gpsinfo->hight;
	zhd_gp.accuracy  = gpsinfo->fixAccuracy;
	zhd_gp.speed     = (float)gpsinfo->speed;
	//�õ���utcʱ��,ʱ��д��Ϊ0
	datetimenew = changeUTCTimeToLocalTime(gpsinfo->datetime, 0);
	i = 0;
	zhd_gp.datetime[i++] = '2';
	zhd_gp.datetime[i++] = '0';
	zhd_gp.datetime[i++] = (datetimenew.year / 10)   + '0';
	zhd_gp.datetime[i++] = (datetimenew.year % 10)   + '0';
	zhd_gp.datetime[i++] = (datetimenew.month / 10)  + '0';
	zhd_gp.datetime[i++] = (datetimenew.month % 10)  + '0';
	zhd_gp.datetime[i++] = (datetimenew.day / 10)    + '0';
	zhd_gp.datetime[i++] = (datetimenew.day % 10)    + '0';
	zhd_gp.datetime[i++] = (datetimenew.hour / 10)   + '0';
	zhd_gp.datetime[i++] = (datetimenew.hour % 10)   + '0';
	zhd_gp.datetime[i++] = (datetimenew.minute / 10) + '0';
	zhd_gp.datetime[i++] = (datetimenew.minute % 10) + '0';
	zhd_gp.datetime[i++] = (datetimenew.second / 10) + '0';
	zhd_gp.datetime[i++] = (datetimenew.second % 10) + '0';
	zhd_gp.batlevel = (int)level;
	LogPrintf(DEBUG_ALL, "zhd_gp_info_register==>lat[%f] lon[%f] accuracy[%d] bat[%d] speed[%f]", 
						zhd_gp.latitude, zhd_gp.longitude,
						zhd_gp.accuracy, zhd_gp.batlevel, zhd_gp.speed);
	LogPrintf(DEBUG_ALL, "zhd_gp_info_register==>%d %d %d %d %d %d", 
	datetimenew.year, datetimenew.month, datetimenew.day, 
	datetimenew.hour, datetimenew.minute, datetimenew.second);
}

/**************************************************
@bref		�к���λ��Э������
@param
@return
@note
**************************************************/
static uint16_t zhd_protocol_gp_package(uint8_t *dest)
{
	int pdu_len = 0, i, ret;
	uint8_t crc;
	double la, lo;
	//Э��ͷ$$
	dest[pdu_len++] = 0x24;
	dest[pdu_len++] = 0x24;
	//������
	dest[pdu_len++] = (ZHD_PROTOCOL_GP >> 8) & 0xff;
	dest[pdu_len++] = ZHD_PROTOCOL_GP & 0xff;
	//����
	dest[pdu_len++] = 43;
	dest[pdu_len++] = 0;
	//data
	//����
	lo = zhd_gp.longitude;
	if (lo < 0) 
		lo *= (-1);
	tmos_memcpy(dest + pdu_len, &lo, sizeof(lo));
	pdu_len += sizeof(lo);
	//γ��
	la = zhd_gp.latitude;
	if (la < 0) 
		la *= (-1);
	tmos_memcpy(dest + pdu_len, &la, sizeof(la));
	pdu_len += sizeof(la);
	
	//�߳�
    tmos_memcpy(dest + pdu_len, &zhd_gp.altitude, sizeof(zhd_gp.altitude));
    pdu_len += sizeof(zhd_gp.altitude);
	//��λ��״̬
	dest[pdu_len++] = zhd_gp.accuracy;
	//�ٶ�
    tmos_memcpy(dest + pdu_len, &zhd_gp.speed, sizeof(zhd_gp.speed));
    pdu_len += sizeof(zhd_gp.speed);
	//������ʱ����
	for (i = 0; i < ZHD_GP_DATETIME_LEN; i++)
	{
		dest[pdu_len++] = zhd_gp.datetime[i];
	}
	//����
	dest[pdu_len++] = zhd_gp.batlevel & 0xff;
	dest[pdu_len++] = (zhd_gp.batlevel >> 8)  & 0xff;
	dest[pdu_len++] = (zhd_gp.batlevel >> 16) & 0xff;
	dest[pdu_len++] = (zhd_gp.batlevel >> 24) & 0xff;
	//#
	dest[pdu_len++] = 0x23;
	//crc
	crc = dest[2];
	XOR_CRC_TOTAL(dest + 3, pdu_len - 4, crc);
	dest[pdu_len++] = crc;
	//Э��β0D0A
	dest[pdu_len++] = 0x0D;
	dest[pdu_len++] = 0x0A;
	zhd_data_debug("ZHD_TCP send", dest, pdu_len);
	return pdu_len;	
}

/**************************************************
@bref		�к���λ��Э�鷢��
@param
@return
@note
**************************************************/
static void zhd_protocol_gp_send(gpsinfo_s *gpsinfo)
{
	uint8_t dest[128] = { 0 };
	uint16_t len;
	int ret = 0;
	zhd_gp_info_register(gpsinfo, getBatteryLevel());
	len = zhd_protocol_gp_package(dest);
	
	if (primaryServerIsReady() && getTcpNack() == 0)
	{
		ret = zhd_tcp_send(ZHD_LINK, dest, len);
	}
	if (0 == ret && gpsinfo->fixstatus && gpsinfo->hadupload == 0)
	{
		gpsRestoreSave(&zhd_gpsres);
	}
	gpsinfo->hadupload = 1;
}

/**************************************************
@bref		�к���Э�鷢��
@param
@return
@note
**************************************************/
void zhd_protocol_send(uint16_t cmd, void *param)
{
    if (sysparam.protocol != ZHD_PROTOCOL_TYPE)
    {
        return;
    }
    switch (cmd)
    {
	case ZHD_PROTOCOL_LG:
		zhd_protocol_lg_send();
		break;
	case ZHD_PROTOCOL_GP:
		zhd_protocol_gp_send((gpsinfo_s *)param);
		break;
	default:
		LogPrintf(DEBUG_ALL, "%s==>Unknow zhd protocol[%d]", __FUNCTION__, cmd);
		break;
    }
}

/**************************************************
@bref		�к���Э�黺������ת����Э��
@param
@return
@note
temp[0] temp[1] temp[2] temp[3]���ڴ洢�߳�
temp[4]���ڴ洢��״̬
temp[5] temp[6] temp[7] temp[8]���ڴ洢�ٶ�
temp[9]���ڴ洢����
**************************************************/
static void zhd_gp_info_register_by_restore(gpsRestore_s *grs)
{
	gpsinfo_s gpsinfo;
	float speed;
	gpsinfo.datetime.year   = grs->year;
	gpsinfo.datetime.month  = grs->month;
	gpsinfo.datetime.day    = grs->day;
	gpsinfo.datetime.hour   = grs->hour;
	gpsinfo.datetime.minute = grs->minute;
	gpsinfo.datetime.second = grs->second;
	tmos_memcpy(&gpsinfo.latitude, grs->latititude, sizeof(double));
	tmos_memcpy(&gpsinfo.longtitude, grs->longtitude, sizeof(double));
	tmos_memcpy(&gpsinfo.hight, grs->temp, sizeof(float));
	gpsinfo.fixAccuracy = grs->temp[4];
	tmos_memcpy(&speed, grs->temp + 5, sizeof(float));
	gpsinfo.speed = (double)speed;
	zhd_gp_info_register(&gpsinfo, grs->temp[9]);
}

/**************************************************
@bref		�к���Э�鲹����������
@param
@return
@note
**************************************************/
uint16_t zhd_gp_restore_upload(uint8_t *dest, gpsRestore_s *grs)
{
	uint16_t len;
	zhd_gp_info_register_by_restore(grs);
	len = zhd_protocol_gp_package(dest);
	return len;
}


/**************************************************
@bref		�к���Э�� ��¼�ظ�
@param
@return
@note
**************************************************/
static void zhd_protocol_lg_respon(uint8_t *data, uint16_t len)
{
	if (data[0] == 1)
	{
		zhdServerLoginSuccess();
	}
	LogPrintf(DEBUG_ALL, "-------------ZHD login %s-------------", data[0] ? "Success" : "fail");
	if (data[1] == 1)
	{
		LogPrintf(DEBUG_ALL, "The unit of track upload frequency is %d second", data[2]);
	}
	else
	{
		LogPrintf(DEBUG_ALL, "The unit of track upload frequency is %d hz", data[2]);
	}
	LogPrintf(DEBUG_ALL, "Token>>");
	LogMessageWL(DEBUG_ALL, data + 3, len - 3);
	LogPrintf(DEBUG_ALL, "------------------------------------------", data[0] ? "Success" : "fail");
}

/**************************************************
@bref		�к���Э�� λ�ûظ�
@param
@return
@note
**************************************************/

static void zhd_protocol_gp_respon(void)
{
	LogPrintf(DEBUG_ALL, "zhd gps position respon");
	hbtRspSuccess();
}


/**************************************************
@bref		�к������ݽ��ս���
@param
@return
@note
**************************************************/
static void zhd_protocol_parser(uint16_t cmd, uint8_t *data, uint16_t len)
{
	LogPrintf(DEBUG_ALL, "%s==>cmd:%04X datalen:%d", __FUNCTION__, cmd, len);
	zhd_data_debug("zhd_protocol_respon_data", data, len);
	switch (cmd)
	{
	case ZHD_PROTOCOL_LG:
		zhd_protocol_lg_respon(data, len);
		break;
	case ZHD_PROTOCOL_GP:
		zhd_protocol_gp_respon();
		break;
	default:
		LogPrintf(DEBUG_ALL, "%s==>Unknow zhd protocol[%d]", __FUNCTION__, cmd);
		break;
	}
}

/**************************************************
@bref		�к������ݽ��ս���
@param
@return
@note
**************************************************/
void zhd_protocol_rx_parser(uint8_t *protocol, uint16_t len)
{
	int i;
	uint8_t protocol_crc;
	uint16_t protocol_len, protocol_cmd;
	for (i = 0; i < len; i++)
	{
		//�ҵ�Э��ͷ$$
		if (0x24 == protocol[i] && 0x24 == protocol[i + 1])
		{
			//����Э�����ݳ���
			protocol_len = (protocol[i + 4] & 0xff) | (protocol[i + 5] & 0xff00);
			LogPrintf(DEBUG_ALL, "%s==>protocol_data_len:%d", __FUNCTION__, protocol_len);
			//�����������ܳ��ȴ���ʵ�����ݳ���,����
			if ((i + 10 + protocol_len) > len)
			{
				LogPrintf(DEBUG_ALL, "%s==>protocol_len error:analyis_len:%d  total_len:%d",
										__FUNCTION__, (i + 10 + protocol_len), len);
				continue;
			}
			//�Ҳ���#
			if ('#' != protocol[i + 6 + protocol_len])
			{
				LogPrintf(DEBUG_ALL, "%s==>can not find #", __FUNCTION__);
				//�ǵ����
				hbtRspSuccess();
				continue;
			}
			//����CRCУ��
			protocol_crc = protocol[2];
			XOR_CRC_TOTAL(protocol + 3, protocol_len + 4 - 1, protocol_crc);
			LogPrintf(DEBUG_ALL, "%s==>crc :%d %d", __FUNCTION__, protocol_crc, (protocol[i + 6 + protocol_len + 1]));
			//����������CRC�ͼ��������CRC��ƥ��,����
			if ((protocol[i + 6 + protocol_len + 1]) != protocol_crc)
			{
				LogPrintf(DEBUG_ALL, "%s==>crc error:%d  %d", __FUNCTION__, protocol_crc, (protocol[i + 6 + protocol_len + 1]));
				continue;
			}
			//�ҵ�Э��β0D0A
			if (0x0D == protocol[i + 6 + protocol_len + 2] && 0x0A == protocol[i + 6 + protocol_len + 3])
			{
				protocol_cmd = ((protocol[i + 2] << 8) & 0xff00) | (protocol[i + 3] & 0xff);
				zhd_protocol_parser(protocol_cmd, protocol + i + 6, protocol_len);
				i += 6 + protocol_len + 3;
			}
		}
	}
}


