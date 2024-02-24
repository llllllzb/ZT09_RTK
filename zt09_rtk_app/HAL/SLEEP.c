/********************************** (C) COPYRIGHT *******************************
 * File Name          : SLEEP.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : ˯�����ü����ʼ��
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
/* ͷ�ļ����� */
#include "HAL.h"
#include "app_task.h"
#include "app_port.h"

/*
 * 0:��ֹ˯��
 * 1:����˯��
 */
static uint8_t sleepCtl=0;

void sleepSetState(uint8_t s)
{
    sleepCtl=s;
}

uint8_t getSleepState(void)
{
	return sleepCtl;
}

/*��GPIO���ѵ�˯��*/
uint8_t mac_addr_for_sleep[6];

void sleep_ext_init(void) {
    GetMACAddress(mac_addr_for_sleep);
}
__HIGH_CODE
void LowPower_Sleep_ext(uint8_t rm)
{
    uint8_t x32Kpw, x32Mpw;
    uint16_t power_plan;
    do{
    x32Kpw = R8_XT32K_TUNE;
    x32Mpw = R8_XT32M_TUNE;
    x32Mpw = (x32Mpw & 0xfc) | 0x03; // 150%�����
    if(R16_RTC_CNT_32K > 0x3fff)
    {                                    // ����500ms
        x32Kpw = (x32Kpw & 0xfc) | 0x01; // LSE�����������͵������
    }

    sys_safe_access_enable();
    R8_BAT_DET_CTRL = 0; // �رյ�ѹ���
    sys_safe_access_enable();
    R8_XT32K_TUNE = x32Kpw;
    R8_XT32M_TUNE = x32Mpw;
    sys_safe_access_disable();

    PFIC->SCTLR |= (1 << 2); //deep sleep

    power_plan = R16_POWER_PLAN & (RB_PWR_DCDC_EN | RB_PWR_DCDC_PRE);
    power_plan |= RB_PWR_PLAN_EN | RB_PWR_MUST_0010 | RB_PWR_CORE | rm;
    __nop();
    sys_safe_access_enable();
    R8_SLP_POWER_CTRL |= RB_RAM_RET_LV;
    R8_PLL_CONFIG |= (1 << 5);
    R16_POWER_PLAN = power_plan;
#if 0
    do{
#endif
        __WFI();
        __nop();
        __nop();
        DelayUs(70);

        uint8_t mac[6] = {0};

        GetMACAddress(mac);
#if 0
        if(mac[5] != 0xff)
            break;
#else
        if(tmos_memcmp(mac,mac_addr_for_sleep,4)) {
            break;
        }
#endif
    }while(1);

    sys_safe_access_enable();
    R8_PLL_CONFIG &= ~(1 << 5);
    sys_safe_access_disable();
}

/*******************************************************************************
 * @fn          CH58X_LowPower
 *
 * @brief       ����˯��
 *
 * @param   time    - ���ѵ�ʱ��㣨RTC����ֵ��
 *
 * @return      state.
 */
uint32_t CH58X_LowPower(uint32_t time)
{
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    uint32_t tmp, irq_status;
    if(sleepCtl==0)
        return 0;
    SYS_DisableAllIrq(&irq_status);
    tmp = RTC_GetCycle32k();
    if((time < tmp) || ((time - tmp) < 30))
    { // ���˯�ߵ����ʱ��
        SYS_RecoverIrq(irq_status);
        return 2;
    }
    RTC_SetTignTime(time);
    SYS_RecoverIrq(irq_status);
    portWdtFeed();
    // LOW POWER-sleepģʽ
    if(!RTCTigFlag)
    {
        LowPower_Sleep_ext(RB_PWR_RAM2K | RB_PWR_RAM30K | RB_PWR_EXTEND);
        if (RTCTigFlag) // ע�����ʹ����RTC����Ļ��ѷ�ʽ����Ҫע���ʱ32M����δ�ȶ�
        {
            time += WAKE_UP_RTC_MAX_TIME;
            if(time > 0xA8C00000)
            {
                time -= 0xA8C00000;
            }
            RTC_SetTignTime(time);
            LowPower_Idle();
        }
        
        HSECFG_Current(HSE_RCur_100); // ��Ϊ�����(�͹��ĺ�����������HSEƫ�õ���)
    }
    else
    {
        return 3;
    }
#endif
    return 0;
}

/*******************************************************************************
 * @fn      HAL_SleepInit
 *
 * @brief   ����˯�߻��ѵķ�ʽ   - RTC���ѣ�����ģʽ
 *
 * @param   None.
 *
 * @return  None.
 */
void HAL_SleepInit(void)
{
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
    SAFEOPERATE;
    R8_SLP_WAKE_CTRL |= RB_SLP_RTC_WAKE; // RTC����
    R8_RTC_MODE_CTRL |= RB_RTC_TRIG_EN;  // ����ģʽ
    R8_SAFE_ACCESS_SIG = 0;              //
    PFIC_EnableIRQ(RTC_IRQn);
#endif
}
