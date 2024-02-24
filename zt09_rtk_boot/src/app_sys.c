/*
 * app_sys.c
 *
 *  Created on: Aug 2, 2022
 *      Author: idea
 */
#include "app_sys.h"
#include "app_port.h"
#include <stdarg.h>

sysinfo_s sysinfo;

static const unsigned short ztvm_crctab16[] =
{
    0X0000, 0X1189, 0X2312, 0X329B, 0X4624, 0X57AD, 0X6536, 0X74BF,
    0X8C48, 0X9DC1, 0XAF5A, 0XBED3, 0XCA6C, 0XDBE5, 0XE97E, 0XF8F7,
    0X1081, 0X0108, 0X3393, 0X221A, 0X56A5, 0X472C, 0X75B7, 0X643E,
    0X9CC9, 0X8D40, 0XBFDB, 0XAE52, 0XDAED, 0XCB64, 0XF9FF, 0XE876,
    0X2102, 0X308B, 0X0210, 0X1399, 0X6726, 0X76AF, 0X4434, 0X55BD,
    0XAD4A, 0XBCC3, 0X8E58, 0X9FD1, 0XEB6E, 0XFAE7, 0XC87C, 0XD9F5,
    0X3183, 0X200A, 0X1291, 0X0318, 0X77A7, 0X662E, 0X54B5, 0X453C,
    0XBDCB, 0XAC42, 0X9ED9, 0X8F50, 0XFBEF, 0XEA66, 0XD8FD, 0XC974,
    0X4204, 0X538D, 0X6116, 0X709F, 0X0420, 0X15A9, 0X2732, 0X36BB,
    0XCE4C, 0XDFC5, 0XED5E, 0XFCD7, 0X8868, 0X99E1, 0XAB7A, 0XBAF3,
    0X5285, 0X430C, 0X7197, 0X601E, 0X14A1, 0X0528, 0X37B3, 0X263A,
    0XDECD, 0XCF44, 0XFDDF, 0XEC56, 0X98E9, 0X8960, 0XBBFB, 0XAA72,
    0X6306, 0X728F, 0X4014, 0X519D, 0X2522, 0X34AB, 0X0630, 0X17B9,
    0XEF4E, 0XFEC7, 0XCC5C, 0XDDD5, 0XA96A, 0XB8E3, 0X8A78, 0X9BF1,
    0X7387, 0X620E, 0X5095, 0X411C, 0X35A3, 0X242A, 0X16B1, 0X0738,
    0XFFCF, 0XEE46, 0XDCDD, 0XCD54, 0XB9EB, 0XA862, 0X9AF9, 0X8B70,
    0X8408, 0X9581, 0XA71A, 0XB693, 0XC22C, 0XD3A5, 0XE13E, 0XF0B7,
    0X0840, 0X19C9, 0X2B52, 0X3ADB, 0X4E64, 0X5FED, 0X6D76, 0X7CFF,
    0X9489, 0X8500, 0XB79B, 0XA612, 0XD2AD, 0XC324, 0XF1BF, 0XE036,
    0X18C1, 0X0948, 0X3BD3, 0X2A5A, 0X5EE5, 0X4F6C, 0X7DF7, 0X6C7E,
    0XA50A, 0XB483, 0X8618, 0X9791, 0XE32E, 0XF2A7, 0XC03C, 0XD1B5,
    0X2942, 0X38CB, 0X0A50, 0X1BD9, 0X6F66, 0X7EEF, 0X4C74, 0X5DFD,
    0XB58B, 0XA402, 0X9699, 0X8710, 0XF3AF, 0XE226, 0XD0BD, 0XC134,
    0X39C3, 0X284A, 0X1AD1, 0X0B58, 0X7FE7, 0X6E6E, 0X5CF5, 0X4D7C,
    0XC60C, 0XD785, 0XE51E, 0XF497, 0X8028, 0X91A1, 0XA33A, 0XB2B3,
    0X4A44, 0X5BCD, 0X6956, 0X78DF, 0X0C60, 0X1DE9, 0X2F72, 0X3EFB,
    0XD68D, 0XC704, 0XF59F, 0XE416, 0X90A9, 0X8120, 0XB3BB, 0XA232,
    0X5AC5, 0X4B4C, 0X79D7, 0X685E, 0X1CE1, 0X0D68, 0X3FF3, 0X2E7A,
    0XE70E, 0XF687, 0XC41C, 0XD595, 0XA12A, 0XB0A3, 0X8238, 0X93B1,
    0X6B46, 0X7ACF, 0X4854, 0X59DD, 0X2D62, 0X3CEB, 0X0E70, 0X1FF9,
    0XF78F, 0XE606, 0XD49D, 0XC514, 0XB1AB, 0XA022, 0X92B9, 0X8330,
    0X7BC7, 0X6A4E, 0X58D5, 0X495C, 0X3DE3, 0X2C6A, 0X1EF1, 0X0F78,
};

unsigned short GetCrc16(const char *pData, int nLength)
{
    uint16_t fcs = 0xffff;
    while (nLength > 0)
    {
        fcs = (fcs >> 8) ^ ztvm_crctab16[(fcs ^ (*pData)) & 0xff];
        nLength--;
        pData++;
    }
    return ~fcs;

}
/**************************************************
@bref       ��ȡRTCʱ��
@param
    year
    month
    date
    hour
    minute
    second
@return
@note
**************************************************/
void portGetRtcDateTime(uint16_t *year, uint8_t *month, uint8_t *date, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    uint16_t a, b, c, d, e, f;
    RTC_GetTime(&a, &b, &c, &d, &e, &f);
    *year = a;
    *month = b;
    *date = c;
    *hour = d;
    *minute = e;
    *second = f;
}

/**************************************************
@bref       ��ӡlog��Ϣ
@param
    level   �Ǽ�
    debug   ��Ϣ
@return
@note
**************************************************/
void LogMessage(uint8_t level, char *debug)
{
    uint16_t year = 0;
    uint8_t  month = 0, date = 0, hour = 0, minute = 0, second = 0;
    char timedebug[20];

    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    sprintf(timedebug, "[%02d:%02d:%02d] ", hour, minute, second);
    portUartSend(&usart2_ctl, (uint8_t *)timedebug, strlen(timedebug));
    portUartSend(&usart2_ctl, (uint8_t *)debug, strlen(debug));
    portUartSend(&usart2_ctl, (uint8_t *)"\r\n", 2);
}

/**************************************************
@bref       ��ӡlog��Ϣ
@param
    level
    buf
    len
@return
@note
**************************************************/
void LogMessageWL(uint8_t level, char *buf, uint16_t len)
{
    uint16_t year = 0;
    uint8_t  month = 0, date = 0, hour = 0, minute = 0, second = 0;
    char timedebug[20];

    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    sprintf(timedebug, "[%02d:%02d:%02d] ", hour, minute, second);
    portUartSend(&usart2_ctl, (uint8_t *)timedebug, strlen(timedebug));
    portUartSend(&usart2_ctl, (uint8_t *)buf, len);
    portUartSend(&usart2_ctl, (uint8_t *)"\r\n", 2);
}

/**************************************************
@bref       ��ӡlog��Ϣ
@param
    level
    debug
    ...
@return
@note
**************************************************/
void LogPrintf(uint8_t level, const char *debug, ...)
{
    static char LOGDEBUG[256];
    va_list args;

    va_start(args, debug);
    vsnprintf(LOGDEBUG, 256, debug, args);
    va_end(args);
    LOGDEBUG[255] = 0;
    LogMessageWL(level, LOGDEBUG, strlen(LOGDEBUG));
}

/**************************************************
@bref       cmd1�Ƿ���ȫ����cmd2
@param
@return
@note
**************************************************/
uint8_t mycmdPatch(uint8_t *cmd1, uint8_t *cmd2)
{
    uint8_t ilen1, ilen2;
    if (cmd1 == NULL || cmd2 == NULL)
        return 0;
    ilen1 = strlen((char *)cmd1);
    ilen2 = strlen((char *)cmd2);
    if (ilen1 != ilen2)
        return 0;
    for (ilen1 = 0; ilen1 < ilen2; ilen1++)
    {
        if (cmd1[ilen1] != cmd2[ilen1])
            return 0;
    }
    return 1;
}
/**************************************************
@bref       �����ַ���Դ�ַ����е�λ��
@param
@return
@note
**************************************************/
int getCharIndex(uint8_t *src, int src_len, char ch)
{
    int i;
    for (i = 0; i < src_len; i++)
    {
        if (src[i] == ch)
            return i;
    }
    return -1;
}
/**************************************************
@bref       �ж�str2�Ƿ���ȫ����str1��ͷ��
@param
@return
@note
**************************************************/
int my_strpach(char *str1, const char *str2)
{
    int i = 0, len;
    if (str1 == NULL || str2 == NULL)
        return 0;
    len = strlen(str2);
    for (i = 0; i < len; i++)
    {
        if (str1[i] != str2[i])
            return 0;
    }
    return 1;
}
/**************************************************
@bref       ����str2��str1�е�λ��
@param
@return
@note
**************************************************/
int my_getstrindex(char *str1, const char *str2, int len)
{
    uint16_t strsize;
    uint16_t i = 0;
    if (str1 == NULL || str2 == NULL || len <= 0)
        return -1;
    strsize = strlen(str2);
    if (len < strsize)
        return -2;
    for (i = 0; i <= (len - strsize); i++)
    {
        if (str1[i] == str2[0])
        {
            if (my_strpach(&str1[i], str2))
            {
                return i;
            }
        }
    }
    return -3;
}
/**************************************************
@bref       �ж�str2�Ƿ�Ϊstr1���Ӵ�
@param
@return
@note
**************************************************/
int my_strstr(char *str1, const char *str2, int len)
{
    int strsize;
    int i = 0;
    strsize = strlen(str2);
    for (i = 0; i <= (len - strsize); i++)
    {
        if (str1[i] == str2[0])
        {
            if (my_strpach(&str1[i], str2))
            {
                return 1;
            }
        }
    }
    return 0;
}
/**************************************************
@bref       ʶ��OK
@param
@return
@note
**************************************************/
int distinguishOK(char *buf)
{
    if (strstr(buf, "OK") != NULL)
        return 1;
    return 0;
}

/**************************************************
@bref       �ֽ�����ת�����ַ���
@param
    src     �ֽ�����
    dest    ������
    srclen  �ֽ����鳤��
@return
@note
**************************************************/
void byteToHexString(uint8_t *src, uint8_t *dest, uint16_t srclen)
{
    uint16_t i;
    uint8_t a, b;
    for (i = 0; i < srclen; i++)
    {
        a = (src[i] >> 4) & 0x0F;
        b = (src[i]) & 0x0F;
        if (a < 10)
        {
            dest[i * 2] = a + '0';
        }
        else
        {
            dest[i * 2] = a - 10 + 'A';
        }

        if (b < 10)
        {
            dest[i * 2 + 1] = b + '0';

        }
        else
        {
            dest[i * 2 + 1] = b - 10 + 'A';
        }
    }

}

/*------------------------------------------------------*/

void stringToItem(ITEM *item, uint8_t *str, uint16_t len)
{
    uint16_t i, data_len;
    char debug[50];
    item->item_cnt = 0;
    data_len = 0;
    //���ŷָ�
    memset(item, 0, sizeof(ITEM));
    for (i = 0; i < ITEMCNTMAX; i++)
    {
        item->item_data[i][0] = 0;
    }
    for (i = 0; i < len; i++)
    {
        if (str[i] == ',' || str[i] == '#' || str[i] == '\r' || str[i] == '\n' || str[i] == '=')
        {
            if (item->item_data[item->item_cnt][0] != 0)
            {
                item->item_cnt++;
                data_len = 0;
                if (item->item_cnt >= ITEMCNTMAX)
                {
                    break; ;
                }
            }
        }
        else
        {
            item->item_data[item->item_cnt][data_len] = str[i];
            data_len++;
            if (i + 1 == len)
            {
                item->item_cnt++;
            }
            if (data_len >= ITEMSIZEMAX)
            {
                return ;
            }
            item->item_data[item->item_cnt][data_len] = 0;
        }
    }
    snprintf(debug, 50, "Item cnt=%d", item->item_cnt);
    LogMessage(DEBUG_ALL, debug);

}

/*------------------------------------------------------*/
//��ȡ�ַ�ch������str�е�n��λ�õ�����
int getCharIndexWithNum(uint8_t *src, uint16_t src_len, uint8_t ch, uint8_t num)
{
    int i, count = 0;
    if (src == NULL)
        return -1;
    for (i = 0; i < src_len; i++)
    {
        if (src[i] == ch)
        {
            ++count;
            if (count == num)
            {
                return i;
            }
        }
    }
    return -1;
}


