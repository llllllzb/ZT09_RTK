#include "app_net.h"
#include "stdlib.h"
#include "app_kernal.h"
#include "app_param.h"
#include "app_sys.h"
#include "app_socket.h"
#include "app_update.h"
//������ؽṹ��

static moduleState_s  moduleState;
static moduleCtrl_s moduleCtrl;
static cmdNode_s *headNode = NULL;


//ָ���
const atCmd_s cmdtable[] =
{
    {AT_CMD, "ATE0"},
    {CPIN_CMD, "AT+CPIN"},
    {CSQ_CMD, "AT+CSQ"},
    {CGREG_CMD, "AT+CGREG"},
    {CEREG_CMD, "AT+CEREG"},
    {QICSGP_CMD, "AT+QICSGP"},
    {QIACT_CMD, "AT+QIACT"},
    {QIOPEN_CMD, "AT+QIOPEN"},
    {QISEND_CMD, "AT+QISEND"},
    {QSCLK_CMD, "AT+QSCLK"},
    {CFUN_CMD, "AT+CFUN"},
    {QICLOSE_CMD, "AT+QICLOSE"},
    {CGDCONT_CMD, "AT+CGDCONT"},
    //����
    {CGATT_CMD, "AT+CGATT"},
	{CGACT_CMD, "AT+CGACT"},
	{MIPOPEN_CMD, "AT+MIPOPEN"},
	{MIPCLOSE_CMD, "AT+MIPCLOSE"},
	{MIPSEND_CMD, "AT+MIPSEND"},
	{MIPRD_CMD, "AT+MIPRD"},
	{MIPSACK_CMD, "AT+MIPSACK"},
	{MADC_CMD, "AT+MADC"},
	{MLPMCFG_CMD, "AT+MLPMCFG"},
	{MLBSCFG_CMD, "AT+MLBSCFG"},
	{MLBSLOC_CMD, "AT+MLBSLOC"},
	{CMGL_CMD, "AT+CMGL"},
	{MWIFISCANSTART_CMD, "AT+MWIFISCANSTART"},
	{MWIFISCANSTOP_CMD, "AT+MWIFISCANSTOP"},
	{MCHIPINFO_CMD, "AT+MCHIPINFO"},
	{MCFG_CMD, "AT+MCFG"},
	{AUTHREQ_CMD, "AT*AUTHReq"},
	{MIPCALL_CMD, "AT+MIPCALL"},

};

/**************************************************
@bref		����ָ��������У�����˳�����
@param
@return
@note
**************************************************/

static uint8_t createNode(char *data, uint16_t datalen, uint8_t currentcmd)
{
    cmdNode_s *nextnode;
    cmdNode_s *currentnode;
    //�������ͷδ�������򴴽�����ͷ��
    WAKEMODULE;
    if (headNode == NULL)
    {
        headNode = malloc(sizeof(cmdNode_s));
        if (headNode != NULL)
        {
            headNode->currentcmd = currentcmd;
            headNode->data = NULL;
            headNode->data = malloc(datalen);
            if (headNode->data != NULL)
            {
                memcpy(headNode->data, data, datalen);
                headNode->datalen = datalen;
                headNode->nextnode = NULL;
                return 1;
            }
            else
            {
                free(headNode);
                headNode = NULL;
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    currentnode = headNode;
    do
    {
        nextnode = currentnode->nextnode;
        if (nextnode == NULL)
        {
            nextnode = malloc(sizeof(cmdNode_s));
            if (nextnode != NULL)
            {

                nextnode->currentcmd = currentcmd;
                nextnode->data = NULL;
                nextnode->data = malloc(datalen);
                if (nextnode->data != NULL)
                {
                    memcpy(nextnode->data, data, datalen);
                    nextnode->datalen = datalen;
                    nextnode->nextnode = NULL;
                    currentnode->nextnode = nextnode;
                    nextnode = nextnode->nextnode;
                }
                else
                {
                    free(nextnode);
                    nextnode = NULL;
                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }
        currentnode = nextnode;
    }
    while (nextnode != NULL);

    return 1;
}

/**************************************************
@bref		���ݶ������
@param
@return
@note
**************************************************/

void outputNode(void)
{
    cmdNode_s *nextnode;
    cmdNode_s *currentnode;
    if (headNode == NULL)
    {
        return ;
    }

    currentnode = headNode;
    if (currentnode != NULL)
    {
        nextnode = currentnode->nextnode;
        moduleState.cmd = currentnode->currentcmd;
        //���ݷ���
        portUartSend(&usart0_ctl, (uint8_t *)currentnode->data, currentnode->datalen);
        if (currentnode->data[0] != 0X78 && currentnode->data[0] != 0x79 && currentnode->data[0] != 0x7E)
        {
            LogMessageWL(DEBUG_NET, currentnode->data, currentnode->datalen);
        }
        free(currentnode->data);
        free(currentnode);
    }
    headNode = nextnode;
    if (headNode == NULL)
    {
        SLEEPMODULE;
    }
}

/**************************************************
@bref		ģ��ָ���
@param
@return
@note
**************************************************/

uint8_t  sendModuleCmd(uint8_t cmd, char *param)
{
    uint8_t i;
    int16_t cmdtype = -1;
    char sendData[256];
    for (i = 0; i < sizeof(cmdtable) / sizeof(cmdtable[0]); i++)
    {
        if (cmd == cmdtable[i].cmd_type)
        {
            cmdtype = i;
            break;
        }
    }
    if (cmdtype < 0)
    {
        snprintf(sendData, 255, "sendModuleCmd==>No cmd");
        LogMessage(DEBUG_ALL, sendData);
        return 0;
    }
    if (param != NULL && strlen(param) <= 240)
    {
        if (param[0] == '?')
        {
            snprintf(sendData, 255, "%s?\r\n", cmdtable[cmdtype].cmd);

        }
        else
        {
            snprintf(sendData, 255, "%s=%s\r\n", cmdtable[cmdtype].cmd, param);
        }
    }
    else if (param == NULL)
    {
        snprintf(sendData, 255, "%s\r\n", cmdtable[cmdtype].cmd);
    }
    else
    {
        return 0;
    }
    createNode(sendData, strlen(sendData), cmd);
    return 1;
}

/**************************************************
@bref		��ʼ��ģ�����ʹ�ýṹ��
@param
@return
@note
**************************************************/

static void moduleInit(void)
{
    memset(&moduleState, 0, sizeof(moduleState_s));
    netResetCsqSearch();
}

/**************************************************
@bref		�Ƿ񿪻�����
@param
@return
@note
**************************************************/
static void modulePressReleaseKey(void)
{
    PWRKEY_HIGH;
    moduleState.powerState = 1;
    LogPrintf(DEBUG_ALL, "PowerOn Done");
}
/**************************************************
@bref		���¿�������
@param
@return
@note
**************************************************/

static void modulePressPowerKey(void)
{
    PWRKEY_LOW;
    startTimer(2500, modulePressReleaseKey, 0);
}
/**************************************************
@bref		ģ�鿪��
@param
@return
@note
**************************************************/

void modulePowerOn(void)
{
    LogMessage(DEBUG_ALL, "modulePowerOn");
    moduleInit();
    portUartCfg(APPUSART0, 1, 57600, moduleRecvParser);
    POWER_ON;
    PWRKEY_HIGH;
    RSTKEY_HIGH;
    startTimer(600, modulePressPowerKey, 0);
    socketDelAll();
}

/**************************************************
@bref		�ػ����
@param
@return
@note
**************************************************/

static void modulePowerOffDone(void)
{
	LogMessage(DEBUG_ALL, "modulePowerOff Done");
	moduleInit();
	portUartCfg(APPUSART0, 0, 57600, NULL);
	POWER_OFF;
}

/**************************************************
@bref		�ͷŹػ�����
@param
@return
@note
**************************************************/
static void modulePowerOffRelease(void)
{
	LogMessage(DEBUG_ALL, "modulePowerOffRelease");
	PWRKEY_HIGH;
	startTimer(6000, modulePowerOffDone, 0);
}	


/**************************************************
@bref		���¹ػ�����
@param
@return
@note
**************************************************/
static void modulePowerOffProcess(void)
{
    PWRKEY_LOW;
	startTimer(3700, modulePowerOffRelease, 0);
	LogMessage(DEBUG_ALL, "modulePowerOffRelease");
}

/**************************************************
@bref		ģ��ػ�
@param
@return
@note
**************************************************/

void modulePowerOff(void)
{
    LogMessage(DEBUG_ALL, "modulePowerOff");
    
	startTimer(500, modulePowerOffProcess, 0);
    RSTKEY_HIGH;
    PWRKEY_HIGH;
    socketDelAll();
}

/**************************************************
@bref       �ͷŸ�λ����
@param
@return
@note
**************************************************/

 void moduleReleaseRstkey(void)
{
    moduleState.powerState = 1;
    RSTKEY_HIGH;
}
/**************************************************
@bref		ģ�鸴λ
@param
@return
@note
**************************************************/

void moduleReset(void)
{
    LogMessage(DEBUG_ALL, "moduleReset");
    moduleInit();
    PWRKEY_HIGH;
    RSTKEY_HIGH;
    startTimer(500, modulePowerOff, 0);
    startTimer(12000, modulePowerOn, 0);
    socketDelAll();
}

/**************************************************
@bref		�л�����״̬��
@param
@return
@note
**************************************************/
static void changeProcess(uint8_t fsm)
{
    moduleState.fsmState = fsm;
    moduleState.fsmtick = 0;
}

/**************************************************
@bref		����socket
@param
@return
@note
**************************************************/

void openSocket(uint8_t link, char *server, uint16_t port)
{
    char param[100];
    sprintf(param, "%d,\"TCP\",\"%s\",%d,60,0,0", link, server, port);
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(MIPOPEN_CMD, param);

}
/**************************************************
@bref		�ر�socket
@param
@return
@note
**************************************************/

void closeSocket(uint8_t link)
{
    char param[10];
    sprintf(param, "%d", link);
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(MIPCLOSE_CMD, param);
}
/**************************************************
@bref		apn����
@param
@return
@note
**************************************************/

static void netSetCgdcong(char *apn)
{

    char param[100];
    sprintf(param, "1,\"IP\",\"%s\"", apn);
    sendModuleCmd(CGDCONT_CMD, param);
}

/**************************************************
@bref		apn����
@param
@return
@note
**************************************************/

static void netSetApn(char *apn, char *apnname, char *apnpassword)
{
    char param[100];
    sprintf(param, "1,1,%s,%s", apnname, apnpassword);
    sendModuleCmd(AUTHREQ_CMD, param);
}

/**************************************************
@bref		ģ��������ģʽ
@param
@return
@note
**************************************************/

static void moduleEnterFly(void)
{
    sendModuleCmd(CFUN_CMD, "0");
}

/**************************************************
@bref		ģ���������ģʽ
@param
@return
@note
**************************************************/

static void moduleExitFly(void)
{
    sendModuleCmd(CFUN_CMD, "1");
}

/**************************************************
@bref		����׼������
@param
@return
@note
**************************************************/

void netConnectTask(void)
{
    if (moduleState.powerState == 0)
    {
        return;
    }
    moduleState.powerOnTick++;
    switch (moduleState.fsmState)
    {
        case AT_STATUS:
            if (moduleState.atResponOK)
            {
                moduleCtrl.atCount = 0;
                moduleState.atResponOK = 0;
                changeProcess(CPIN_STATUS);
            }
            else
            {
                if (moduleState.fsmtick % 2 == 0)
                {
                    moduleState.powerOnTick = 0;
                    sendModuleCmd(AT_CMD, NULL);
                }
                if (moduleState.fsmtick >= 30)
                {

                    moduleCtrl.atCount++;
                    if (moduleCtrl.atCount >= 2)
                    {
                        moduleCtrl.atCount = 0;
                        modulePowerOn();
                    }
                    else
                    {
                        moduleReset();
                    }
                }
                break;
            }
        case CPIN_STATUS:
            if (moduleState.cpinResponOk)
            {
                moduleState.cpinResponOk = 0;
                sendModuleCmd(AT_CMD, NULL);
                netSetCgdcong((char *)sysparam.apn);
                netSetApn((char *)sysparam.apn, (char *)sysparam.apnuser, (char *)sysparam.apnpassword);
                changeProcess(CSQ_STATUS);

            }
            else
            {
                if (moduleState.fsmtick % 2 == 0)
                {
                    sendModuleCmd(CPIN_CMD, "?");
                }
                if (moduleState.fsmtick >= 30)
                {
                    moduleReset();
                }
                break;
            }
        case CSQ_STATUS:
            if (moduleState.csqOk)
            {
                moduleState.csqOk = 0;
                moduleCtrl.csqCount = 0;
                sendModuleCmd(CEREG_CMD, "2");
                changeProcess(CGREG_STATUS);
                netResetCsqSearch();
            }
            else
            {
                sendModuleCmd(CSQ_CMD, NULL);
                if (moduleState.fsmtick >= moduleCtrl.csqTime)
                {
                    moduleCtrl.csqTime *= 2;
                    moduleCtrl.csqTime=moduleCtrl.csqTime>3600?3600:moduleCtrl.csqTime;
                    moduleCtrl.csqCount++;
                    if (moduleCtrl.csqCount >= 3)
                    {
                        moduleCtrl.csqCount = 0;
                        moduleReset();
                    }
                    else
                    {
                        moduleEnterFly();
                        startTimer(8000, moduleExitFly, 0);
                    }
                }
                break;
            }
        case CGREG_STATUS:
            if (moduleState.cgregOK || moduleState.ceregOK)
            {
                moduleCtrl.cgregCount = 0;
                moduleState.cgregOK = 0;
                changeProcess(CONFIG_STATUS);
            }
            else
            {
                sendModuleCmd(CEREG_CMD, "?");
                if (moduleState.fsmtick >= 180)
                {
                    moduleCtrl.cgregCount++;
                    if (moduleCtrl.cgregCount >= 2)
                    {
                        moduleCtrl.cgregCount = 0;
                        moduleState.cgregOK = 1;
                        LogMessage(DEBUG_ALL, "Register timeout,try to skip\r\n");
                    }
                    else
                    {
                        changeProcess(AT_STATUS);
                    }
                }
                break;
            }
        case CONFIG_STATUS:
            netSetCgdcong((char *)sysparam.apn);
            netSetApn((char *)sysparam.apn, (char *)sysparam.apnuser, (char *)sysparam.apnpassword);
            changeProcess(QIACT_STATUS);

            break;
        case QIACT_STATUS:
            if (moduleState.qipactOk)
            {
                moduleState.qipactOk = 0;
                moduleCtrl.qipactCount = 0;
                changeProcess(NORMAL_STATUS);
            }
            else
            {
				sendModuleCmd(MIPCALL_CMD, "1,1");
				sendModuleCmd(MIPCALL_CMD, "?");

                if (moduleState.fsmtick >= 45)
                {
                    LogMessage(DEBUG_ALL, "try QIPACT again");
                    moduleState.qipactSet = 0;
                    moduleState.fsmtick = 0;
                    moduleCtrl.qipactCount++;
                    if (moduleCtrl.qipactCount >= 3)
                    {
                        moduleCtrl.qipactCount = 0;
                        moduleReset();
                    }
                    else
                    {
                        changeProcess(CPIN_STATUS);
                    }
                }
                break;
            }
        case NORMAL_STATUS:
            socketSchedule();
            break;
        default:
            changeProcess(AT_STATUS);
            break;
    }
    moduleState.fsmtick++;
}

/**************************************************
@bref		AT+CSQ	ָ�����
@param
@return
@note
**************************************************/

static void csqParser(uint8_t *buf, uint16_t len)
{
    int index, indexa, datalen;
    uint8_t *rebuf;
    uint16_t  relen;
    char restore[5];
    index = my_getstrindex((char *)buf, "+CSQ:", len);
    if (index >= 0)
    {
        rebuf = buf + index;
        relen = len - index;
        indexa = getCharIndex(rebuf, relen, ',');
        if (indexa > 6)
        {
            datalen = indexa - 6;
            if (datalen > 5)
                return;
            memset(restore, 0, 5);
            strncpy(restore, (char *)rebuf + 6, datalen);
            moduleState.rssi = atoi(restore);
            if (moduleState.rssi >= 6 && moduleState.rssi <= 31)
                moduleState.csqOk = 1;
        }
    }
}

/**************************************************
@bref		AT+CREG	ָ�����
@param
@return
@note
**************************************************/

static void cgregParser(uint8_t *buf, uint16_t len)
{
    int index, datalen;
    uint8_t *rebuf;
    uint16_t  relen, i;
    char restore[50];
    uint8_t cnt;
    uint8_t type = 0;
    index = my_getstrindex((char *)buf, "+CGREG:", len);
    if (index < 0)
    {
        type = 1;
        index = my_getstrindex((char *)buf, "+CEREG:", len);
    }
    if (index >= 0)
    {
        rebuf = buf + index;
        relen = len - index;
        datalen = 0;
        cnt = 0;
        restore[0] = 0;
        for (i = 0; i < relen; i++)
        {
            if (rebuf[i] == ',' || rebuf[i] == '\r' || rebuf[i] == '\n')
            {
                if (restore[0] != 0)
                {
                    restore[datalen] = 0;
                    cnt++;
                    datalen = 0;
                    switch (cnt)
                    {
                        case 2:
                            if (restore[0] == '1' || restore[0] == '5')
                            {
                                if (type)
                                {
                                    moduleState.ceregOK = 1;
                                }
                                else
                                {
                                    moduleState.cgregOK = 1;
                                }
                            }
                            else
                            {
                                return ;
                            }
                            break;
                        case 3:

                            moduleState.lac = strtoul(restore + 1, NULL, 16);
                            LogPrintf(DEBUG_ALL, "LAC=%s,0x%X", restore, moduleState.lac);
                            break;
                        case 4:
                            moduleState.cid = strtoul(restore + 1, NULL, 16);
                            LogPrintf(DEBUG_ALL, "CID=%s,0x%X", restore, moduleState.cid);
                            break;
                    }
                    restore[0] = 0;
                }
            }
            else
            {
                restore[datalen] = rebuf[i];
                datalen++;
                if (datalen >= 50)
                {
                    return ;
                }

            }
        }
    }
}


/*------------------------------------����----------------------------------------*/
/**************************************************
@bref		+CGATT	ָ�����
@param
@return
@note
	+CGATT: 1

	OK
**************************************************/

static void cgattParser(uint8_t *buf, uint16_t len)
{
    uint8_t ret;
    int index;
    uint8_t *rebuf;
    int16_t relen;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+CGATT:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += index;
    relen -= index;
    ret = rebuf[8] - '0';
    if (ret == 1)
    {
        moduleState.qipactOk = 1;
    }
    else
    {
        moduleState.qipactOk = 0;
    }
}

/**************************************************
@bref		ģ���urc���ݽ��ս�����
@param
@return
@note
+MIPURC: "rtcp",0,2
+MIPURC: "disconn",0,1
+MIPURC: "rtcp",0,10,xx\0D


OK
**************************************************/

uint8_t mipurcParser(uint8_t *buf, uint16_t len)
{
    int index;
    char restore[513];
    uint8_t *rebuf, type, link, numb, ret = 0;
    int16_t relen;
    uint16_t readLen, unreadLen, debugLen;
    ITEM item;
    rebuf = buf;
    relen = len;

    index = my_getstrindex(rebuf, "+MIPURC:", relen);
    if (index < 0)
    	return 0 ;
    while (index >= 0)
    {
		rebuf += index + 10;
		relen -= index - 10;
        if (my_strpach(rebuf, "rtcp"))
        {

        	rebuf += 6;
        	relen -= 6;
        	index = getCharIndexWithNum(rebuf, relen, ',', 2);
        	memcpy(restore, rebuf, index);
			restore[index] = 0;
			stringToItem(&item, restore, index);
			link = atoi(item.item_data[0]);
			readLen = atoi(item.item_data[1]);
			rebuf += index + 1;
			relen -= index - 1;
			index = getCharIndex(rebuf, relen, '\r');
			debugLen = readLen > 256 ? 256 : readLen;                 
            byteToHexString(rebuf, restore, debugLen);
            restore[debugLen * 2] = 0;
			LogPrintf(DEBUG_ALL, "TCP RECV (%d)[%d]:%s", link, readLen, restore);
			socketRecv(link, rebuf, readLen);
			rebuf += index;
			relen -= index;
//			rebuf += 6;
//			relen -= 6;
//			index = getCharIndex(rebuf, relen, '\r');
//			tmos_memcpy(restore, rebuf, index);
//			restore[index] = 0;
//			stringToItem(&item, restore, index);
//			link = atoi(item.item_data[0]);
//			numb = atoi(item.item_data[1]);
//			LogPrintf(DEBUG_ALL, "Socket[%d] recv data", link);
//			switch (link)
//			{
//				case NORMAL_LINK:
//					moduleState.normalLinkQird = 1;
//					break;
//				case BLE_LINK:
//					moduleState.bleLinkQird = 1;
//					break;
//				case JT808_LINK:
//					moduleState.jt808LinkQird = 1;
//					break;
//				case HIDDEN_LINK:
//					moduleState.hideLinkQird = 1;
//					break;
//				case AGPS_LINK:
//					moduleState.agpsLinkQird = 1;
//					break;
//				case NTRIP_LINK:
//					moduleState.ntripLinkQird = 1;
//					break;
//			}

        }
        else if (my_strpach(rebuf, "disconn"))
        {
			rebuf += 9;
			relen -= 9;
			index = getCharIndex(rebuf, relen, '\r');
			memcpy(restore, rebuf, index);
			restore[index] = 0;
			stringToItem(&item, restore, index);
			link = atoi(item.item_data[0]);
			LogPrintf(DEBUG_ALL, "Socket[%d] close", link);
			socketSetConnState(link, SOCKET_CONN_IDLE);
			rebuf += index;
			relen -= index;
        }

        index = my_getstrindex(rebuf, "+MIPURC:", relen);
    }

    return ret;
}


/**************************************************
@bref		MIPOPEN	ָ�����
@param
@return
@note
	+MIPOPEN: 0,0
**************************************************/

void mipopenParser(uint8_t *buf, uint16_t len)
{
	int index;
	int relen;
    uint8_t *rebuf;
    uint8_t result, link;
    ITEM item;
    
    rebuf = buf;
    relen = len;
	index = my_getstrindex(rebuf, "+MIPOPEN:", relen);
	while (index >= 0)
	{
		rebuf += index + 10;
		relen -= index - 10;
		index = getCharIndex(rebuf, relen, '\r');
		stringToItem(&item, rebuf, relen);
		link = atoi(item.item_data[0]);
		result = atoi(item.item_data[1]);
		if (result == 0)
		{
			socketSetConnState(link, SOCKET_CONN_SUCCESS);
			moduleCtrl.qiopenCount = 0;
		}
		else
		{
			socketSetConnState(link, SOCKET_CONN_ERR);
			moduleCtrl.qiopenCount++;
			if (moduleCtrl.qiopenCount >= 4)
			{
				moduleReset();
				moduleCtrl.qiopenCount = 0;
			}
		}
		index = my_getstrindex(rebuf, "+MIPOPEN:", relen);
	}
	
}

/**************************************************
@bref       +MIPCALL  ָ�����
@param
@return
@note
+MIPCALL: 1,1,"10.165.173.87"
+MIPCALL: 1,0

**************************************************/

static void mipcallParser(uint8_t *buf, uint16_t len)
{
    uint8_t ret;
    int index;
    uint8_t *rebuf;
    int16_t relen;
    rebuf = buf;
    relen = len;
    index = my_getstrindex((char *)rebuf, "+MIPCALL:", relen);
    if (index < 0)
    {
        return;
    }
    rebuf += index;
    relen -= index;
    ret = rebuf[12] - '0';
    if (ret == 1)
    {
        moduleState.qipactOk = 1;
    }
    else if (ret == 0)
    {
        moduleState.qipactOk = 0;
        changeProcess(CPIN_STATUS);
    }
    else
    {
		moduleState.qipactOk = 0;
    }

}

/**************************************************
@bref		ģ������ݽ��ս�����
@param
@return
@note
**************************************************/

void moduleRecvParser(uint8_t *buf, uint16_t bufsize)
{
    static uint16_t len = 0;
    static uint8_t dataRestore[MODULE_RX_BUFF_SIZE + 1];

    if (bufsize == 0)
        return;
    if (len + bufsize > MODULE_RX_BUFF_SIZE)
    {
        len = 0;
        bufsize %= MODULE_RX_BUFF_SIZE;
        LogMessage(DEBUG_ALL, "UartRecv Full!!!");
    }
    memcpy(dataRestore + len, buf, bufsize);
    len += bufsize;
    dataRestore[len] = 0;

    if(len <= 2)
    {
        return;
    }
    if(dataRestore[len-2]!='\r' || dataRestore[len-1]!='\n')
    {
        if (dataRestore[2] != '>')
        {
            return;
        }
    }

    LogPrintf(DEBUG_ALL, "--->>>---0x%X [%d]", dataRestore, len);
    LogMessageWL(DEBUG_ALL, (char *)dataRestore, len);
    LogMessage(DEBUG_ALL, "---<<<---");
    /*****************************************/
    moduleRspSuccess();
    mipurcParser(dataRestore, len);
    mipopenParser(dataRestore, len);
	mipcallParser(dataRestore, len);

    /*****************************************/
    switch (moduleState.cmd)
    {
        case AT_CMD:
            if (distinguishOK((char *)dataRestore))
            {
                moduleState.atResponOK = 1;
            }
            break;
        case CPIN_CMD:
            if (my_strstr((char *)dataRestore, "+CPIN: READY", len))
            {
                moduleState.cpinResponOk = 1;
            }
            break;
        case CSQ_CMD:
            csqParser(dataRestore, len);
            break;
        case CGREG_CMD:
        case CEREG_CMD:
            cgregParser(dataRestore, len);
            break;
       	case CGATT_CMD:
            cgattParser(dataRestore, len);
            break;
        default:
            break;
    }

    moduleState.cmd = 0;
    len = 0;
}
/*--------------------------------------------------------------*/

/**************************************************
@bref		�����ź�����ʱ��
@param
@return
@note
**************************************************/

void netResetCsqSearch(void)
{
    moduleCtrl.csqTime = 120;
}

/**************************************************
@bref		socket��������
@param
@return
@note
**************************************************/

int socketSendData(uint8_t link, uint8_t *data, uint16_t len)
{
    int ret = 0;
    char param[10];

    if (socketGetConnStatus(link) == 0)
    {
        //��·δ����
        return 0;
    }
    sprintf(param, "%d,%d", link, len);
    sendModuleCmd(AT_CMD, NULL);
    sendModuleCmd(MIPSEND_CMD, param);
    createNode((char *)data, len, MIPSEND_CMD);
    return len;
}

/**************************************************
@bref		ģ���Ƿ�ﵽ����״̬
@param
@return
@note
**************************************************/

uint8_t isModuleRunNormal(void)
{
    if (moduleState.fsmState == NORMAL_STATUS)
        return 1;
    return 0;
}

