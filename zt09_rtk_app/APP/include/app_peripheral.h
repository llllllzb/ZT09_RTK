/*
 * app_peripheral.h
 *
 *  Created on: Feb 25, 2022
 *      Author: idea
 */

#ifndef APP_INCLUDE_APP_PERIPHERAL_H_
#define APP_INCLUDE_APP_PERIPHERAL_H_

#include "config.h"

#define APP_PERIPHERAL_START_EVENT              0x0001
#define APP_PERIPHERAL_PARAM_UPDATE_EVENT       0x0002
#define APP_PERIPHERAL_MTU_CHANGE_EVENT			0x0004
#define APP_UPDATE_MCU_RTC_EVENT				0x0008

typedef struct{
    uint16_t connectionHandle;   //!< Connection Handle from controller used to ref the device
    uint8_t connRole;            //!< Connection formed as Master or Slave
    uint16_t connInterval;       //!< Connection Interval
    uint16_t connLatency;        //!< Connection Latency
    uint16_t connTimeout;        //!< Connection Timeout
}connectionInfoStruct;

void appPeripheralInit(void);
void appSendNotifyData(uint8 *data, uint16 len);
#endif /* APP_INCLUDE_APP_PERIPHERAL_H_ */
