/*
 * app_zhdprotocol.h
 *
 *  Created on: Jul 1, 2024
 *      Author: lvvv
 */

#ifndef TASK_INC_APP_ZHDPROTOCOL_H_
#define TASK_INC_APP_ZHDPROTOCOL_H_

#include "config.h"
#include "app_sys.h"
#include "app_param.h"
#include "app_gps.h"
#include "app_server.h"
#include "app_protocol.h"
#include "app_net.h"


#define ZHD_PROTOCOL_LG		0x4C47	//登录
#define ZHD_PROTOCOL_GP		0x4750	//位置

#define ZHD_LG_SN_LEN	     8
#define ZHD_LG_USER_LEN      8
#define ZHD_LG_PASSWORD_LEN  8

#define ZHD_GP_DATETIME_LEN  14

#define ZHD_MASK		"GkqdSirj"

//登陆协议信息
typedef struct 
{
	uint8_t sn[ZHD_LG_SN_LEN];			    //设备号
	uint8_t user[ZHD_LG_USER_LEN];		    //登录账号
	uint8_t password[ZHD_LG_PASSWORD_LEN];	//登录密码
}zhd_proinfo_lg_s;

//位置协议信息
typedef struct
{
	double  latitude;
	double  longitude;
	float   altitude;
	uint8_t accuracy;
	float   speed;
	uint8_t datetime[ZHD_GP_DATETIME_LEN];
	int     batlevel;
}zhd_proinfo_position_s;


typedef int(*zhd_tcp_send_handle_t)(uint8_t link, uint8_t *buf, uint16_t len);


void zhd_protocol_init(void);
void encryption_by_mask(const uint8_t *mask, uint8_t *str);
void zhd_tcp_send_handle_register(int (* handle)(uint8_t, uint8_t *, uint16_t));
void zhd_lg_info_register(uint8_t *sn, uint8_t *user, uint8_t *password);
void zhd_gp_info_register(gpsinfo_s *gpsinfo, uint8_t level);
int zhd_tcp_send(uint8_t link, uint8_t *data, uint16_t len);
void zhd_protocol_send(uint16_t cmd, void *param);
uint16_t zhd_gp_restore_upload(uint8_t *dest, gpsRestore_s *grs);

void zhd_protocol_rx_parser(uint8_t *protocol, uint16_t len);


#endif /* TASK_INC_APP_ZHDPROTOCOL_H_ */
