/********************************** (C) COPYRIGHT *******************************
 * File Name          : SLEEP.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
#ifndef __SLEEP_H
#define __SLEEP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

/**
 * @brief   配置睡眠唤醒的方式   - RTC唤醒，触发模式
 */
extern void HAL_SleepInit(void);

/**
 * @brief   启动睡眠
 *
 * @param   time    - 唤醒的时间点（RTC绝对值）
 *
 * @return  state.
 */
extern uint32_t CH58X_LowPower(uint32_t time);
extern void sleepSetState(uint8_t s);
extern uint8_t getSleepState(void);
extern void sleep_ext_init(void);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
