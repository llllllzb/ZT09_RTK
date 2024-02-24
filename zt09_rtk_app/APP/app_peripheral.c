/*
 * app_peripheral.c
 *
 *  Created on: Feb 25, 2022
 *      Author: idea
 */
#include "app_peripheral.h"
#include "aes.h"
#include "app_instructioncmd.h"
#include "app_port.h"
#include "app_sys.h"
#include "app_server.h"
#include "app_protocol.h"
#include "app_param.h"
#include "aes.h"
/*
 * ȫ�ֱ�������
 */
tmosTaskID appPeripheralTaskId = INVALID_TASK_ID;
gapBondCBs_t appPeripheralGapBondCallBack;
gapRolesCBs_t appPeripheralGapRolesCallBack;
connectionInfoStruct appPeripheralConn;
/*
 * ��������
 */
static tmosEvents appPeripheralEventProcess(tmosTaskID taskID, tmosEvents events);
static void appPeripheralGapRolesRssiRead(uint16_t connHandle, int8_t newRSSI);
static void appPeripheralGapRolesStateNotify(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void appPeripheralGapRolesParamUpdate(uint16_t connHandle, uint16_t connInterval, uint16_t connSlaveLatency,
        uint16_t connTimeout);

/********************************************************
 * *����UUID
 ********************************************************/

#define APP_SERVICE_UUID            0xFFE0
#define APP_CHARACTERISTIC1_UUID    0xFFE1

/********************************************************
 * *UUID����С��
 ********************************************************/

static uint8 ServiceUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(APP_SERVICE_UUID), HI_UINT16(APP_SERVICE_UUID)};
static uint8 Char1UUID[ATT_BT_UUID_SIZE] = { LO_UINT16(APP_CHARACTERISTIC1_UUID), HI_UINT16(APP_CHARACTERISTIC1_UUID)};

/********************************************************
 * *����UUID������Ϣ
 ********************************************************/
static gattAttrType_t ServiceProfile =
{
    ATT_BT_UUID_SIZE, ServiceUUID
};

/********************************************************
 * *����������ֵ
 ********************************************************/
static uint8 char1_Properties = GATT_PROP_READ | GATT_PROP_WRITE
                                | GATT_PROP_NOTIFY;

/********************************************************
 * *�����洢��
 ********************************************************/
static uint8 char1ValueStore[4];
static gattCharCfg_t char1ClientConfig[4];

/********************************************************
 * *��������
 ********************************************************/

static uint8 char1Description[] = "appchar1";

/********************************************************
 * *����������
 ********************************************************/
static gattAttribute_t appAttributeTable[] =
{
    //Service
    {   { ATT_BT_UUID_SIZE, primaryServiceUUID }, //type
        GATT_PERMIT_READ, 0,
        (uint8 *)& ServiceProfile
    },
    //��������
    {   { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ, 0, &char1_Properties
    },
    //��������ֵ
    {   { ATT_BT_UUID_SIZE, Char1UUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, char1ValueStore
    },
    //�ͻ�����������NOTIFY
    {   { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, (uint8 *) char1ClientConfig
    },
    //�����������û�����
    {   { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 0, char1Description
    }
};

/********************************************************
 * *��ӷ���ص�
 ********************************************************/
static bStatus_t appReadAttrCB(uint16 connHandle, gattAttribute_t *pAttr,
                               uint8 *pValue, uint16 *pLen, uint16 offset, uint16 maxLen, uint8 method)
{
    bStatus_t ret = SUCCESS;
    uint16 uuid;
    if (gattPermitAuthorRead(pAttr->permissions))
    {
        return ATT_ERR_INSUFFICIENT_AUTHOR;
    }
    if (offset > 0)
    {
        return ATT_ERR_ATTR_NOT_LONG;
    }
    if (pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        LogPrintf(DEBUG_ALL, "UUID[0x%X]==>Read Request", uuid);
        switch (uuid)
        {
            case APP_CHARACTERISTIC1_UUID:
                *pLen = 4;
                tmos_memcpy(pValue, pAttr->pValue, 4);
                break;
        }
    }
    else
    {
        *pLen = 0;
        ret = ATT_ERR_INVALID_HANDLE;
    }

    return ret;
}
//SN:999913436051195,292,3.77,46
void bleRecvParser(char *data, uint16_t len)
{
    ITEM item;
    insParam_s insparam;
    bleInfo_s devInfo;
    int index;
    char *rebuf;
    int16_t relen;
    char dec[256];
    uint8_t decLen, ret;

    ret = dencryptData(dec, &decLen, data, len);
    if (ret == 0)
    {
        return;
    }

    rebuf = dec;
    relen = decLen;

    if ((index = my_getstrindex(rebuf, "RE:", relen)) >= 0)
    {
        setInsId();
        insparam.data = rebuf;
        insparam.len = relen;
        //protocolSend(BLE_LINK, PROTOCOL_21, &insparam);
    }
    else if ((index = my_getstrindex(rebuf, "SN:", relen)) >= 0)
    {
        rebuf += index + 3;
        relen -= index + 3;
        stringToItem(&item, rebuf, relen);
        if (item.item_cnt == 4)
        {
            strncpy(devInfo.imei, item.item_data[0], 15);
            devInfo.imei[15] = 0;
            devInfo.startCnt = atoi(item.item_data[1]);
            devInfo.vol = atof(item.item_data[2]);
            devInfo.batLevel = atoi(item.item_data[3]);

            bleServerAddInfo(devInfo);
            tmos_start_task(appPeripheralTaskId, APP_UPDATE_MCU_RTC_EVENT, MS1_TO_SYSTEM_TIME(200));
        }
    }
}


static bStatus_t appWriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr,
                                uint8 *pValue, uint16 len, uint16 offset, uint8 method)
{
    bStatus_t ret = SUCCESS;
    uint16 uuid;
    uint8_t debugStr[101], debugLen;
    if (gattPermitAuthorWrite(pAttr->permissions))
    {
        return ATT_ERR_INSUFFICIENT_AUTHOR;
    }
    if (pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        LogPrintf(DEBUG_ALL, "UUID[0x%X]==>Write Request,size:%d", uuid, len);
        debugLen = len > 50 ? 50 : len;
        byteToHexString(pValue, debugStr, debugLen);
        debugStr[debugLen * 2] = 0;
        LogPrintf(DEBUG_ALL, "%s", debugStr);
        switch (uuid)
        {
            case APP_CHARACTERISTIC1_UUID:
                bleRecvParser(pValue, len);
                break;
            case GATT_CLIENT_CHAR_CFG_UUID:
                ret = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                     offset, GATT_CLIENT_CFG_NOTIFY);
                break;
        }
    }
    else
    {
        ret = ATT_ERR_INVALID_HANDLE;
    }
    return ret;
}
static gattServiceCBs_t gattServiceCallBack = { appReadAttrCB, appWriteAttrCB,
                                                NULL,
                                              };

static void appHandleConnStatusCB(uint16 connHandle, uint8 changeType)
{
    if (connHandle != LOOPBACK_CONNHANDLE)
    {
        if ((changeType == LINKDB_STATUS_UPDATE_REMOVED)
                || ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS)
                    && (!linkDB_Up(connHandle))))
        {
            GATTServApp_InitCharCfg(connHandle, char1ClientConfig);
        }
    }
}

static void appAddServer(void)
{
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, char1ClientConfig);
    linkDB_Register(appHandleConnStatusCB);
    GATTServApp_RegisterService(appAttributeTable,
                                GATT_NUM_ATTRS(appAttributeTable), GATT_MAX_ENCRYPT_KEY_SIZE,
                                &gattServiceCallBack);
}

/*
 * ���ù㲥����Ϣ
 */
void appPeripheralBroadcastInfoCfg(uint8 *broadcastnmae)
{
    uint8 len, advLen;
    uint8 advertData[31];
    len = tmos_strlen(broadcastnmae);

    advLen = 0;
    advertData[advLen++] = len + 1;
    advertData[advLen++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    tmos_memcpy(advertData + advLen, broadcastnmae, len);
    advLen += len;
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, advLen, advertData);
}
/*
 * ��������ʼ��
 */
void appPeripheralInit(void)
{
    char broadCastNmae[30];
    uint8 u8Value;
    uint16 u16Value;
    //�����ʼ��
    GAPRole_PeripheralInit();
    //ע������
    appPeripheralTaskId = TMOS_ProcessEventRegister(appPeripheralEventProcess);

    appPeripheralGapBondCallBack.pairStateCB = NULL;
    appPeripheralGapBondCallBack.passcodeCB = NULL;
    appPeripheralGapRolesCallBack.pfnParamUpdate = appPeripheralGapRolesParamUpdate;
    appPeripheralGapRolesCallBack.pfnRssiRead = appPeripheralGapRolesRssiRead;
    appPeripheralGapRolesCallBack.pfnStateChange = appPeripheralGapRolesStateNotify;

    tmos_memset(&appPeripheralConn, 0, sizeof(appPeripheralConn));
    appPeripheralConn.connectionHandle = GAP_CONNHANDLE_INIT;
    //��������
    //�����㲥
    u8Value = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &u8Value);
    //���ù㲥��Ϣ
    sprintf(broadCastNmae, "%s-%s", "ZT09", dynamicParam.SN);
    appPeripheralBroadcastInfoCfg(broadCastNmae);
    //����������Ӽ��
    u16Value = 0x0006;    //6*1.25=7.5ms
    GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16), &u16Value);
    //��������Ӽ��
    u16Value = 0x0c80;    //3200*1.25=4000ms
    GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16), &u16Value);
    //������̹㲥���
    //unit:0.625ms*160=100ms
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, 160);
    //������㲥���
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, 160);
    //ʹ��ɨ��Ӧ��֪ͨ
    GAP_SetParamValue(TGAP_ADV_SCAN_REQ_NOTIFY, FALSE);
    //��ӷ���
    appAddServer();
    //GATT_InitClient();
    tmos_set_event(appPeripheralTaskId, APP_PERIPHERAL_START_EVENT);
}

/*
 * *notify data
 */
static bStatus_t appNotify(uint16 connHandle, attHandleValueNoti_t *pNoti)
{
    uint16 value = GATTServApp_ReadCharCfg(connHandle, char1ClientConfig);
    if (value & GATT_CLIENT_CFG_NOTIFY)
    {
        pNoti->handle = appAttributeTable[2].handle;
        return GATT_Notification(connHandle, pNoti, FALSE);
    }
    return bleIncorrectMode;
}

void appSendNotifyData(uint8 *data, uint16 len)
{
    attHandleValueNoti_t notify;
    notify.len = len;
    notify.pValue = GATT_bm_alloc(appPeripheralConn.connectionHandle, ATT_HANDLE_VALUE_NOTI, notify.len, NULL, 0);
    if (notify.pValue == NULL)
    {
        LogPrintf(DEBUG_ALL, "appSendNotifyData==>alloc memory fail");
        return;
    }
    tmos_memcpy(notify.pValue, data, notify.len);
    if (appNotify(appPeripheralConn.connectionHandle, &notify) != SUCCESS)
    {
        GATT_bm_free((gattMsg_t *)&notify, ATT_HANDLE_VALUE_NOTI);
        LogPrintf(DEBUG_ALL, "Notify fail");
    }
    else
    {
        LogPrintf(DEBUG_ALL, "Notify success");
    }
}

static void appGapMsgProcess(gapRoleEvent_t *pMsg)
{
    int8 debug[20];
    LogPrintf(DEBUG_ALL, "appGapMsgProcess==>OptionCode:0x%X", pMsg->gap.opcode);
    switch (pMsg->gap.opcode)
    {
        case GAP_SCAN_REQUEST_EVENT:
            byteToHexString(pMsg->scanReqEvt.scannerAddr, debug, B_ADDR_LEN);
            debug[B_ADDR_LEN * 2] = 0;
            LogPrintf(DEBUG_ALL, "ScannerMac:%s", debug);
            break;
        case GAP_PHY_UPDATE_EVENT:
            LogPrintf(DEBUG_ALL, "-------------------------------------");
            LogPrintf(DEBUG_ALL, "*****LinkPhyUpdate*****");
            LogPrintf(DEBUG_ALL, "connHandle:%d", pMsg->linkPhyUpdate.connectionHandle);
            LogPrintf(DEBUG_ALL, "connRxPHYS:%d", pMsg->linkPhyUpdate.connRxPHYS);
            LogPrintf(DEBUG_ALL, "connTxPHYS:%d", pMsg->linkPhyUpdate.connTxPHYS);
            LogPrintf(DEBUG_ALL, "-------------------------------------");
            break;
    }
}

static void appGattMsgProcess(gattMsgEvent_t *pMsg)
{
    LogPrintf(DEBUG_ALL, "pMsg->connHandle==>%#X", pMsg->connHandle);
    LogPrintf(DEBUG_ALL, "pMsg->method==>%#X", pMsg->method);
    switch (pMsg->method)

    {
        case ATT_EXCHANGE_MTU_RSP:
            LogPrintf(DEBUG_ALL, "pMsg->msg.exchangeMTURsp.serverRxMTU==>%d", pMsg->msg.exchangeMTURsp.serverRxMTU);
            break;
        case ATT_MTU_UPDATED_EVENT:
            LogPrintf(DEBUG_ALL, "pMsg->msg.mtuEvt.MTU==>%d", pMsg->msg.mtuEvt.MTU);
            break;
    }
}
static void appPerihperalSysEventMsg(tmos_event_hdr_t *msg)
{
    LogPrintf(DEBUG_ALL, "SysMsgEvent=0x%X,Status=0x%X", msg->event, msg->status);
    switch (msg->event)
    {
        case GAP_MSG_EVENT:
            appGapMsgProcess((gapRoleEvent_t *) msg);
            break;
        case GATT_MSG_EVENT:
            appGattMsgProcess((gattMsgEvent_t *)msg);
            break;
    }
}
static void appChangeMtu(void)
{
    bStatus_t ret;
    attExchangeMTUReq_t pReq;
    pReq.clientRxMTU = BLE_BUFF_MAX_LEN - 4;
    ret = GATT_ExchangeMTU(appPeripheralConn.connectionHandle, &pReq, appPeripheralTaskId);
    LogPrintf(DEBUG_ALL, "appChangeMtu==>Ret:0x%X", ret);
}

static void sendRtcDateTime(void)
{
    uint16_t year;
    uint8_t  month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    char msg[20];
    char encrypt[128];
    uint8_t encryptLen;
    portGetRtcDateTime(&year, &month, &date, &hour, &minute, &second);
    sprintf(msg, "TIME,%d,%d,%d,%d,%d,%d", year, month, date, hour, minute, second);
    encryptData(encrypt, &encryptLen, msg, strlen(msg));
    LogMessage(DEBUG_ALL, "send datetime");
    appSendNotifyData(encrypt, encryptLen);
}

/*
 * �����¼�����
 */
static tmosEvents appPeripheralEventProcess(tmosTaskID taskID,tmosEvents events)
{
    if (events & SYS_EVENT_MSG)
    {
        uint8 *pMsg;
        if ((pMsg = tmos_msg_receive(appPeripheralTaskId)) != NULL)
        {
            appPerihperalSysEventMsg((tmos_event_hdr_t *) pMsg);
            tmos_msg_deallocate(pMsg);
        }
        return (events ^ SYS_EVENT_MSG);
    }
    if (events & APP_PERIPHERAL_START_EVENT)
    {
        GAPRole_PeripheralStartDevice(appPeripheralTaskId,
                                      &appPeripheralGapBondCallBack, &appPeripheralGapRolesCallBack);
        return events ^ APP_PERIPHERAL_START_EVENT;
    }
    if (events & APP_PERIPHERAL_PARAM_UPDATE_EVENT)
    {
        GAPRole_PeripheralConnParamUpdateReq(appPeripheralConn.connectionHandle, 6, 20, 0, 300, appPeripheralTaskId);
        return events ^ APP_PERIPHERAL_PARAM_UPDATE_EVENT;
    }
    if (events & APP_PERIPHERAL_MTU_CHANGE_EVENT)
    {
        appChangeMtu();
        return events ^ APP_PERIPHERAL_MTU_CHANGE_EVENT;
    }
    if (events & APP_UPDATE_MCU_RTC_EVENT)
    {
        sendRtcDateTime();
        return events ^ APP_UPDATE_MCU_RTC_EVENT;
    }
    return 0;
}
/*
 * �豸��������
 */
static void appEstablished(gapRoleEvent_t *pEvent)
{
    uint8 debug[20];
    if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
    {
        appPeripheralConn.connectionHandle = pEvent->linkCmpl.connectionHandle;
        appPeripheralConn.connInterval = pEvent->linkCmpl.connInterval;
        appPeripheralConn.connLatency = pEvent->linkCmpl.clockAccuracy;
        appPeripheralConn.connRole = pEvent->linkCmpl.connRole;
        appPeripheralConn.connTimeout = pEvent->linkCmpl.connTimeout;
        byteToHexString(pEvent->linkCmpl.devAddr, debug, 6);
        debug[12] = 0;
        LogPrintf(DEBUG_ALL, "-------------------------------------");
        LogPrintf(DEBUG_ALL, "*****Device Connection*****");
        LogPrintf(DEBUG_ALL, "DeviceMac:%s", debug);
        LogPrintf(DEBUG_ALL, "DeviceType:%d", pEvent->linkCmpl.devAddrType);

        LogPrintf(DEBUG_ALL, "connectionHandle:%d", pEvent->linkCmpl.connectionHandle);
        LogPrintf(DEBUG_ALL, "connInterval:%d", pEvent->linkCmpl.connInterval);
        LogPrintf(DEBUG_ALL, "connLatency:%d", pEvent->linkCmpl.connLatency);
        LogPrintf(DEBUG_ALL, "connRole:%d", pEvent->linkCmpl.connRole);
        LogPrintf(DEBUG_ALL, "connTimeout:%d", pEvent->linkCmpl.connTimeout);
        LogPrintf(DEBUG_ALL, "-------------------------------------");
        tmos_set_event(appPeripheralTaskId, APP_PERIPHERAL_MTU_CHANGE_EVENT);
    }
}
/*
 * �豸�ȴ�
 */

static void appGaproleWaitting(gapRoleEvent_t *pEvent)
{
    uint8 u8Value;
    if (pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
    {
        pEvent->linkTerminate.connectionHandle;
        LogPrintf(DEBUG_ALL, "-------------------------------------");
        LogPrintf(DEBUG_ALL, "*****Device Terminate*****");
        LogPrintf(DEBUG_ALL, "TerminateHandle:%d", pEvent->linkTerminate.connectionHandle);
        LogPrintf(DEBUG_ALL, "TerminateRole:%d", pEvent->linkTerminate.connRole);
        LogPrintf(DEBUG_ALL, "TerminateReason:0x%X", pEvent->linkTerminate.reason);
        LogPrintf(DEBUG_ALL, "-------------------------------------");
        u8Value = TRUE;
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &u8Value);
    }
}
/*
 * ��������
 */
static void appPeripheralGapRolesParamUpdate(uint16_t connHandle,
        uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout)
{
    if (appPeripheralConn.connectionHandle == connHandle)
    {
        appPeripheralConn.connInterval = connInterval;
        appPeripheralConn.connLatency = connSlaveLatency;
        appPeripheralConn.connTimeout = connTimeout;
    }
    LogPrintf(DEBUG_ALL, "-------------------------------------");
    LogPrintf(DEBUG_ALL, "*****ParamUpdate*****");
    LogPrintf(DEBUG_ALL, "connectionHandle:%d", connHandle);
    LogPrintf(DEBUG_ALL, "connInterval:%d", connInterval);
    LogPrintf(DEBUG_ALL, "connLatency:%d", connSlaveLatency);
    LogPrintf(DEBUG_ALL, "connTimeout:%d", connTimeout);
    LogPrintf(DEBUG_ALL, "-------------------------------------");
}
/*
 * �ź�ֵ
 */
static void appPeripheralGapRolesRssiRead(uint16_t connHandle, int8_t newRSSI)
{
    LogPrintf(DEBUG_ALL, "ConnHandle %d, RSSI :%d", connHandle, newRSSI);
}
/*
 * ״̬�¼�
 */
static void appPeripheralGapRolesStateNotify(gapRole_States_t newState,        gapRoleEvent_t *pEvent)
{

    LogPrintf(DEBUG_ALL, "OptionCode=%d", pEvent->gap.opcode);
    switch (newState & GAPROLE_STATE_ADV_MASK)
    {
        case GAPROLE_STARTED:
            LogPrintf(DEBUG_ALL, "GAPRole started");
            break;
        case GAPROLE_ADVERTISING:
            LogPrintf(DEBUG_ALL, "GAPRole advertising");
            break;
        case GAPROLE_WAITING:
            LogPrintf(DEBUG_ALL, "GAPRole waitting");
            appGaproleWaitting(pEvent);
            break;
        case GAPROLE_CONNECTED:
            LogPrintf(DEBUG_ALL, "GAPRole connected");
            appEstablished(pEvent);
            break;
        case GAPROLE_CONNECTED_ADV:
            LogPrintf(DEBUG_ALL, "GAPRole connected adv");
            break;
        case GAPROLE_ERROR:
            LogPrintf(DEBUG_ALL, "GAPRole error");
            break;
        default:
            break;
    }
}
