/*
 * app_db.h
 *
 *  Created on: Apr 28, 2022
 *      Author: idea
 */

#ifndef TASK_INC_APP_DB_H_
#define TASK_INC_APP_DB_H_

#include "CH58x_common.h"
#include "config.h"

/*
 * DataFlash �ֲ�
 * [0~1)        Total:1KB   bootloader
 * [1~10)       Total:9KB   app
 * [10~30)      Total:20KB  database
 * [30~31)		Total:1KB	databaseInfo
 * [31~32)      Total:1KB   blestack
 */

#define DB_ADDRESS          0x2800
#define DB_SIZE             0x5000
#define DBINFO_ADDRESS		0x7800
#define DBINFO_SIZE			0x100
#define WIFI_ADDRESS		0X7400

#define DB_FLAG				0xAB
#define GPS_DB_SIZE			500

#define BLOCK_SIZE		256

typedef struct
{
	uint8_t dbFlag;
    uint16 dbBegin;
    uint16 dbEnd;
} dbinfo_t;

typedef struct
{
    uint16_t size;
    uint16_t upSize;
    uint8_t buffer[GPS_DB_SIZE];
} gpsDataBase_s;

void dbInfoClear(void);
void dbInfoRead(void);


void dbSave(uint8_t *data, uint16_t size);
void dbSaveRelease(void);
uint8_t dbUpload(void);


uint16 dbCircularWrite(uint8 *data, uint16 len);
uint16 dbCircularRead(uint8 *data, uint16 len);

#endif /* TASK_INC_APP_DB_H_ */
