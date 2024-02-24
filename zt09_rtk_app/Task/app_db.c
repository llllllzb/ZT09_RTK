/*
 * app_db.c
 *
 *  Created on: Apr 28, 2022
 *      Author: idea
 */
#include "app_db.h"

#include "app_param.h"
#include "app_sys.h"
#include "app_protocol.h"
#include "app_net.h"
#include "app_jt808.h"

static dbinfo_t dbinfo;
static gpsDataBase_s gpsdb = {0};


/**************************************************
@bref		����������Ϣ
@param
@return
@note
**************************************************/
static void dbInfoWrite(void)
{
    EEPROM_ERASE(DBINFO_ADDRESS, sizeof(dbinfo));
    EEPROM_WRITE(DBINFO_ADDRESS, &dbinfo, sizeof(dbinfo));
}

/**************************************************
@bref		��ʼ������
@param
@return
@note
**************************************************/
void dbInfoClear(void)
{
    tmos_memset(&dbinfo, 0, sizeof(dbinfo));
    dbinfo.dbFlag = DB_FLAG;
    dbInfoWrite();
    LogMessage(DEBUG_ALL, "dbInfoClear");
}

/**************************************************
@bref		��ȡ������Ϣ
@param
@return
@note
**************************************************/

void dbInfoRead(void)
{
    EEPROM_READ(DBINFO_ADDRESS, &dbinfo, sizeof(dbinfo));
    if (dbinfo.dbFlag != DB_FLAG)
    {
        dbInfoClear();
    }
    LogPrintf(DEBUG_ALL, "dbInfoRead==>Begin: %d,End: %d", dbinfo.dbBegin, dbinfo.dbEnd);
}

/**************************************************
@bref		д��������Ϣ
@param
@return
@note
**************************************************/
uint8 dbWrite(uint8 *data, uint16 len)
{
    int ret;
    uint16 relen;
    uint16 resize;
    uint16 earseCnt;
    uint16 earseAddr;
    if (dbinfo.dbEnd == 0)
    {
        LogPrintf(DEBUG_ALL, "doWrite==>Earse %d bytes from 0x%X", EEPROM_MIN_ER_SIZE, DB_ADDRESS);
        ret = EEPROM_ERASE(DB_ADDRESS, EEPROM_MIN_ER_SIZE);
        if (ret != 0)
        {
            LogMessage(DEBUG_ALL, "doWrite==>Earse fail");
            return 0;
        }
    }

    resize = EEPROM_MIN_ER_SIZE - (dbinfo.dbEnd % EEPROM_MIN_ER_SIZE);
    if (len > resize)
    {
        relen = len - resize;
        earseCnt = relen / EEPROM_MIN_ER_SIZE;
        if (relen % EEPROM_MIN_ER_SIZE != 0)
        {
            earseCnt++;
        }

        earseAddr = DB_ADDRESS + dbinfo.dbEnd + resize;

        LogPrintf(DEBUG_ALL, "doWrite==>Earse %d bytes from 0x%X", EEPROM_MIN_ER_SIZE * earseCnt, earseAddr);
        ret = EEPROM_ERASE(earseAddr, EEPROM_MIN_ER_SIZE * earseCnt);
        if (ret != 0)
        {
            LogMessage(DEBUG_ALL, "doWrite==>Earse fail");
            return 0;
        }

    }

    LogPrintf(DEBUG_ALL, "doWrite==>Begin 0x%X ,%d bytes", DB_ADDRESS + dbinfo.dbEnd, len);
    ret = EEPROM_WRITE(DB_ADDRESS + dbinfo.dbEnd, data, len);
    if (ret != 0)
    {
        LogMessage(DEBUG_ALL, "doWrite==>Fail");
        return 0;
    }
    dbinfo.dbEnd += len;
    LogMessage(DEBUG_ALL, "doWrite==>Success");

    return 1;
}


/**************************************************
@bref		��ȡdb����
@param
@return
@note
**************************************************/

uint16 dbRead(uint8 *data, uint16 len)
{
    uint16 relen;
    uint16 readLen;
    int ret;
    if (dbinfo.dbBegin >= dbinfo.dbEnd)
    {
        dbinfo.dbBegin = dbinfo.dbEnd = 0;
        return 0;
    }

    relen = dbinfo.dbEnd - dbinfo.dbBegin;

    readLen = len > relen ? relen : len;
    LogPrintf(DEBUG_ALL, "dbRead==>Read %d bytes from 0x%X", readLen, DB_ADDRESS + dbinfo.dbBegin);
    ret = EEPROM_READ(DB_ADDRESS + dbinfo.dbBegin, data, readLen);
    if (ret != 0)
    {
        LogMessage(DEBUG_ALL, "dbRead==>Fail");
        return 0;
    }
    LogMessage(DEBUG_ALL, "dbRead==>Success");
    dbinfo.dbBegin += readLen;
    return readLen;
}


/**************************************************
@bref		����GPS���ݵ���ʱ������
@param
@return
@note
**************************************************/

void dbSave(uint8_t *data, uint16_t size)
{
    if (gpsdb.size + size > GPS_DB_SIZE)
    {
        dbSaveRelease();
        LogMessage(DEBUG_ALL, "dbSave==>Save to flash");
    }
    memcpy(gpsdb.buffer + gpsdb.size, data, size);
    gpsdb.size += size;
    LogPrintf(DEBUG_ALL, "dbEnd:%d", gpsdb.size);

}

/**************************************************
@bref		���滺�����е����ݵ�flash
@param
@return
@note
**************************************************/

void dbSaveRelease(void)
{
    if (gpsdb.size == 0)
    {
        LogMessage(DEBUG_ALL, "dbsave nothing");
        return;
    }
    LogPrintf(DEBUG_ALL, "dbSaveRelease==>%d,%d", gpsdb.upSize, gpsdb.size - gpsdb.upSize);
    dbCircularWrite(gpsdb.buffer + gpsdb.upSize, gpsdb.size - gpsdb.upSize);
    gpsdb.upSize = gpsdb.size = 0;
}

/**************************************************
@bref		�ϴ��������е�����
@param
@return
@note
**************************************************/
uint8_t dbUpload(void)
{
    uint8_t gpscount, i;
    uint16_t destlen, datalen;
    uint8_t dest[1024];
    gpsRestore_s *gpsinfo;
    if (gpsdb.size == 0)
    {
        return 0;
    }
    if (gpsdb.upSize >= gpsdb.size)
    {
        gpsdb.upSize = gpsdb.size = 0;
        return 0;
    }

    //һ�����������20��
    gpscount = (gpsdb.size - gpsdb.upSize) / sizeof(gpsRestore_s);
    gpscount = gpscount > 5 ? 5 : gpscount;

    if (gpscount == 0)
    {
        gpsdb.size = 0;
        return 0;
    }

    LogPrintf(DEBUG_ALL, "dbUpload==>%d item", gpscount);
    destlen = 0;
    for (i = 0; i < gpscount; i++)
    {
        gpsinfo = (gpsRestore_s *)(gpsdb.buffer + gpsdb.upSize + (sizeof(gpsRestore_s) * i));
        if (sysparam.protocol == JT808_PROTOCOL_TYPE)
        {
            datalen = jt808gpsRestoreDataSend((uint8_t *)dest + destlen, gpsinfo);
        }
        else
        {
            gpsRestoreDataSend(gpsinfo, (char *)dest + destlen, &datalen);
        }
        destlen += datalen;
    }
    gpsdb.upSize += (gpscount * sizeof(gpsRestore_s));
    if (sysparam.protocol == JT808_PROTOCOL_TYPE)
    {
        jt808TcpSend((uint8_t *)dest, destlen);
    }
    else
    {
        socketSendData(NORMAL_LINK, (uint8_t *)dest, destlen);
    }
    if (gpsdb.upSize >= gpsdb.size)
    {
        gpsdb.upSize = gpsdb.size = 0;
    }
    return 1;
}

static uint8 dbSectionWrite(uint8 *data, uint16 len)
{
    bStatus_t status;
    uint8_t blockNum, u8Vlaue, earseBlockNum;
    uint16_t u16Value, earseAddr, earseSize;
    if (len == 0 || data == NULL)
    {
        return 0;
    }
    //�������ڿ���
    blockNum = dbinfo.dbEnd / BLOCK_SIZE;
    //�����ڸÿ���ʣ�¶��ٿռ�ɴ��
    u16Value = (blockNum + 1) * BLOCK_SIZE - dbinfo.dbEnd;
    //LogPrintf(0, "��ǰ��ַ��%#X�����ڿ�����%d��ʣ�ࣺ%d Bytes", dbinfo.dbEnd, blockNum, u16Value);
    //LogPrintf(0, "[s]дָ�룺%#X����ǰ����ʣ�ࣺ%#X Bytes", dbinfo.dbEnd, u16Value);
    LogPrintf(DEBUG_ALL, "[S]׼��д�룺%d Bytes", len);
    if (len <= u16Value)
    {
        if (dbinfo.dbEnd % BLOCK_SIZE == 0)
        {
            //LogPrintf(0, "[1]������%#X~%#X,����%#X Bytes", dbinfo.dbEnd, dbinfo.dbEnd +  BLOCK_SIZE - 1, BLOCK_SIZE);
            status = EEPROM_ERASE(DB_ADDRESS + dbinfo.dbEnd, BLOCK_SIZE);
            if (status != SUCCESS)
            {
                return 0;
            }
        }
        status = EEPROM_WRITE(DB_ADDRESS + dbinfo.dbEnd, data, len);
        if (status != SUCCESS)
        {
            return 0;
        }
        //ʣ��ռ��㹻��ֱ�Ӵ��
        dbinfo.dbEnd += len;
    }
    else
    {
        //ʣ��ռ䲻�㣬��Ҫ����
        //д��ʣ��ռ�
        //������Ҫ�������ٿ�
        u16Value = len - u16Value;
        earseBlockNum = u16Value / BLOCK_SIZE;
        earseBlockNum++;

        if (dbinfo.dbEnd % BLOCK_SIZE == 0)
        {
            earseAddr = blockNum * BLOCK_SIZE;
            earseSize = (earseBlockNum + 1)  * BLOCK_SIZE;
        }
        else
        {
            earseAddr = (blockNum + 1) * BLOCK_SIZE;
            earseSize = earseBlockNum * BLOCK_SIZE;
        }



        //LogPrintf(0, "[2]������%#X~%#X,����%#X Bytes", earseAddr, earseAddr + earseSize - 1, earseSize);

        if (dbinfo.dbBegin > earseAddr && dbinfo.dbBegin <= (earseAddr + earseSize - 1))
        {
            //LogMessage(0, "[2]�������������ָ�룬ֹͣ����");
            return 0;
        }
        status = EEPROM_ERASE(DB_ADDRESS + earseAddr, earseSize);
        if (status != SUCCESS)
        {
            return 0;
        }
        status = EEPROM_WRITE(DB_ADDRESS + dbinfo.dbEnd, data, len);
        if (status != SUCCESS)
        {
            return 0;
        }
        dbinfo.dbEnd += len;
    }


    if (dbinfo.dbEnd == dbinfo.dbBegin)
    {
        u16Value = DB_SIZE;
    }
    else if (dbinfo.dbBegin < dbinfo.dbEnd)
    {
        u16Value = DB_SIZE - (dbinfo.dbEnd - dbinfo.dbBegin);
    }
    else
    {
        u16Value = dbinfo.dbBegin - dbinfo.dbEnd;
    }
    LogPrintf(DEBUG_ALL, "[E]д����ɣ����� %d Bytes", u16Value);
    return len;
}

uint16 dbCircularWrite(uint8 *data, uint16 len)
{
    uint16_t u16Value;
    uint8 ret;
    if (len == 0 || data == NULL)
    {
        return 0;
    }
    if (dbinfo.dbEnd == dbinfo.dbBegin)
    {
        u16Value = DB_SIZE;
    }
    else if (dbinfo.dbBegin < dbinfo.dbEnd)
    {
        u16Value = DB_SIZE - (dbinfo.dbEnd - dbinfo.dbBegin);
    }
    else
    {
        u16Value = dbinfo.dbBegin - dbinfo.dbEnd;
    }
    LogPrintf(DEBUG_ALL, "[A]�洢ʣ�ࣺ%d Bytes", u16Value);
    if (len > (u16Value - BLOCK_SIZE) || (u16Value < BLOCK_SIZE))
    {
        LogPrintf(DEBUG_ALL, "[A]�ڴ治�㣬ֹͣд��");
        return 0;
    }

    if (dbinfo.dbEnd + len <= DB_SIZE)
    {
        dbSectionWrite(data, len);
    }
    else
    {
        u16Value = DB_SIZE - dbinfo.dbEnd;
        ret = dbSectionWrite(data, u16Value);
        if (ret == 0)
        {
            return 0;
        }
        dbinfo.dbEnd = 0;
        dbSectionWrite(data + u16Value, len - u16Value);
    }
    dbInfoWrite();
    return len;
}
uint16 dbCircularRead(uint8 *data, uint16 len)
{
    uint16_t u16Value, offset, ret;
    ret = 0;
    if (data == NULL || len == 0)
    {
        return ret;
    }
    if (dbinfo.dbBegin == dbinfo.dbEnd)
    {
        //LogMessage(DEBUG_ALL, "������");
        return ret;
    }
    if (dbinfo.dbBegin < dbinfo.dbEnd)
    {
        u16Value = dbinfo.dbEnd - dbinfo.dbBegin;
    }
    else
    {
        u16Value = DB_SIZE - (dbinfo.dbBegin - dbinfo.dbEnd);
    }
    //LogPrintf(0, "[S]Begin:%#X ~ End:%#X", dbinfo.dbBegin, dbinfo.dbEnd);
    LogPrintf(0, "[S]�洢��%d Bytes", u16Value);

    //������ö�������
    u16Value = u16Value > len ? len : u16Value;
    ret = u16Value;
    //LogPrintf(0, "[S]׼����ȡ��%d Bytes", u16Value);

    if (dbinfo.dbBegin < dbinfo.dbEnd)
    {
        EEPROM_READ(DB_ADDRESS + dbinfo.dbBegin, data, u16Value);
        dbinfo.dbBegin += u16Value;
        //LogPrintf(0, "[1]��ȡ��%d Bytes����ָ���ƶ���==>%#X", u16Value, dbinfo.dbBegin);
    }
    else// dbinfo.dbBegin > dbinfo.dbEnd
    {
        if (DB_SIZE - dbinfo.dbBegin > u16Value)
        {
            EEPROM_READ(DB_ADDRESS + dbinfo.dbBegin, data, u16Value);
            dbinfo.dbBegin += u16Value;
            //LogPrintf(0, "[2]��ȡ��%d Bytes����ָ���ƶ���==>%#X", u16Value, dbinfo.dbBegin);
        }
        else
        {
            offset = DB_SIZE - dbinfo.dbBegin;
            //LogPrintf(0, "[3]��ȡ��%d Bytes����ָ���ƶ���==>%#X", offset, 0);
            u16Value = u16Value - offset;

            EEPROM_READ(DB_ADDRESS + dbinfo.dbBegin, data, offset);
            dbinfo.dbBegin = 0;
            //��ȡ����
            EEPROM_READ(DB_ADDRESS + dbinfo.dbBegin, data + offset, u16Value);
            dbinfo.dbBegin += u16Value;
            //LogPrintf(0, "[4]��ȡ��%d Bytes����ָ���ƶ���==>%#X", u16Value, dbinfo.dbBegin);
        }
    }
    dbInfoWrite();
    LogPrintf(DEBUG_ALL, "[E]��ȡ���� Begin: %#X ~ End: %#X", dbinfo.dbBegin, dbinfo.dbEnd);
    return ret;
}
