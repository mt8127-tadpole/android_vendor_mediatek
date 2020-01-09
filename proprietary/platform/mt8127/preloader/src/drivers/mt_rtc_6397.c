/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include <typedefs.h>
#include <mt_rtc.h>
#include <timer.h>
#include <pmic_wrap_init.h>
#include <upmu_hw_mt6397.h>
#include <platform.h>
#include <cust_rtc.h>
#include <pmic.h>
#include <mt8127.h>
#include <pll.h>

#define RTC_RELPWR_WHEN_XRST	1   /* BBPU = 0 when xreset_rstb goes low */

#define RTC_GPIO_USER_MASK	  (((1U << 13) - 1) & 0xff00)
static int XOSC=0;

static bool recovery_flag = false;
static bool g_rtc_2sec_stat;

static bool rtc_busy_wait(void);
static bool Write_trigger(void);
static U16 eosc_cali(void);
static bool rtc_first_boot_init(void);
static U16 get_frequency_meter(U16 val, U16 measureSrc, U16 window_size);
static bool rtc_frequency_meter_check(void);
static void rtc_recovery_flow(void);
static bool rtc_recovery_mode_check(void);
static bool rtc_init_after_recovery(void);
static bool rtc_get_recovery_mode_stat(void);
static bool rtc_gpio_init(void);
static bool rtc_android_init(void);
static bool rtc_lpd_init(void);
static bool Writeif_unlock(void);
static bool rtc_2sec_stat_clear(void);
void rtc_enable_2sec_reboot(void);

static U16 RTC_Read(U16 addr)
{
	U32 rdata=0;
	pwrap_read((U32)addr, &rdata);
	return (U16)rdata;
}

static void RTC_Write(U16 addr, U16 data)
{
	pwrap_write((U32)addr, (U32)data);
}

static bool rtc_busy_wait(void)
{
	ulong begin = get_timer(0);
	do {												
		while (RTC_Read(RTC_BBPU) & RTC_BBPU_CBUSY)
		{
			/////>    Time > 1sec,  time out and set recovery mode enable.  </////
			if (get_timer(begin) > 1000)
			{
				print("[RTC] rtc cbusy time out!!!!!\n");
				return false;
			}
		}
	} while (0);

	return true;
}	

static void rtc_call_exception(void)
{
	ASSERT(0);
}

static bool rtc_xosc_check_clock(U16 *result)
{

	///// fix me  loose range for frequency meter result////
	if ((result[0] >= 3  &&result[0] <= 7 ) && 
			(result[1] > 1500 && result[1] < 6000) &&
			(result[2] == 0) &&
			(result[3] == 0))
		return true;
	else
		return false;		
}

static bool rtc_eosc_check_clock(U16 *result)
{
	if ((result[0] >= 3  &&result[0] <= 7 )&& 
			(result[1] < 500) &&
			(result[2] > 2 && result[2] < 9) &&
			(result[3] > 300 && result[3] < 10400))
		return true;
	else
		return false;
}

static void rtc_clk_cfg_write(void)
{
    U32 temp;
#ifdef MTK_ALPS_BOX_SUPPORT
    temp = DRV_Reg(CLK_CFG_6);
    temp &= ~0x03;
	temp |= 0x0;
	DRV_WriteReg32(CLK_CFG_6,temp);
#else
    temp = DRV_Reg(CLK_CFG_6);
    temp &= ~0x03;
	temp |= 0x1;
	DRV_WriteReg32(CLK_CFG_6,temp);
	temp = DRV_Reg32(CLK_MISC_CFG_0);
	temp &=~0x4000;
	DRV_WriteReg32(CLK_MISC_CFG_0,temp);
#endif
}

static void rtc_xosc_write(U16 val)
{
	U16 bbpu;

	RTC_Write(RTC_OSC32CON, 0x1a57);
	mdelay(1);
	//rtc_busy_wait();
	RTC_Write(RTC_OSC32CON, 0x2b68);
	mdelay(1);
	//rtc_busy_wait();

	RTC_Write(RTC_OSC32CON, val);
	mdelay(1);
	//rtc_busy_wait();
#if 0
	if (reload) {
		bbpu = RTC_Read(RTC_BBPU) | RTC_BBPU_KEY | RTC_BBPU_RELOAD;
		RTC_Write(RTC_BBPU, bbpu);
		Write_trigger();
	}
#endif
}

static U16 get_frequency_meter(U16 val, U16 measureSrc, U16 window_size)
{
	U16 ret;
	int i;
	ulong begin = get_timer(0);

	if(val!=0)
		rtc_xosc_write(val);

	RTC_Write(TOP_RST_CON, 0x0100);			//FQMTR reset
	while( !(RTC_Read(FQMTR_CON2)==0)  && (0x8&RTC_Read(FQMTR_CON0))==0x8);
	RTC_Write(TOP_RST_CON, 0x0000);			//FQMTR normal

	RTC_Write(FQMTR_CON1, window_size); //set freq. meter window value (0=1X32K(fix clock))
	RTC_Write(FQMTR_CON0, 0x8000 | measureSrc); //enable freq. meter, set measure clock to 26Mhz

	mdelay(1);
	while( (0x8&RTC_Read(FQMTR_CON0))==0x8 )
	{
		if (get_timer(begin) > 1000)
		{
			print("get frequency time out\n");
			break;
		}
	};		// FQMTR read until ready
	
	ret = RTC_Read(FQMTR_CON2);				//read data should be closed to 26M/32k = 812.5		
	print("[RTC] get_frequency_meter: input=0x%x, ouput=%d\n",val, ret);

	return ret;
}

static void rtc_measure_four_clock(U16 *result)
{
	U16 window_size;
	
	RTC_Write(TOP_CKCON2, (RTC_Read(TOP_CKCON2) & ~0x8000) ); //select 26M as fixed clock
	window_size = 4;
	mdelay(1);
	result[0] = get_frequency_meter(0, 0x0004, window_size); 		//select 26M as target clock
	
	//select XOSC_DET as fixed clock
	RTC_Write(TOP_CKTST2, (RTC_Read(TOP_CKTST2) & ~0x00C0));  
	RTC_Write(TOP_CKCON2, (RTC_Read(TOP_CKCON2) | 0x8000));
	window_size = 4;
	mdelay(1);
	result[1] = get_frequency_meter(0, 0x0004, window_size); 		//select 26M as target clock
	
	//select 26M as fixed clock
	//RTC_Write(TOP_CKTST2, (RTC_Read(TOP_CKTST2) & ~0x0080));  
	RTC_Write(TOP_CKCON2, (RTC_Read(TOP_CKCON2) & ~0x8000));
	window_size = 794 * 5;
	mdelay(1);
	//result[2] = get_frequency_meter(0x0007, 0x0000, window_size); 								//select DCXO_32 as target clock
	result[2] = get_frequency_meter(0, 0x0000, window_size); 								//select DCXO_32 as target clock
	result[2] = get_frequency_meter(0, 0x0001, window_size); 								//select DCXO_32 as target clock
	//result[2] = get_frequency_meter(0x0007, 0x0002, window_size); 								//select DCXO_32 as target clock

	//select EOSC_32 as fixed clock
	RTC_Write(TOP_CKTST2, (RTC_Read(TOP_CKTST2) | 0x0080 & ~0x0040));  
	RTC_Write(TOP_CKCON2, (RTC_Read(TOP_CKCON2) | 0x8000));
	window_size = 4;
	mdelay(1);
	result[3] = get_frequency_meter(0, 0x0004, window_size); 			//select 26M as target clock
}

static void rtc_switch_mode(bool XOSC, bool recovery)
{
	if (XOSC)
	{
		if (recovery)
		{
			/* HW bypass switch mode control and set to XOSC */
			RTC_Write(CHRSTATUS, (RTC_Read(CHRSTATUS) | 0x0800 & ~0x0200));
		}
		rtc_xosc_write(0x0003);  /* assume crystal exist mode + XOSCCALI = 0x3 */
		if (recovery)
			mdelay(1000);
	} else 
	{
		if (recovery)
		{
			/* HW bypass switch mode control and set to DCXO */
			RTC_Write(CHRSTATUS, (RTC_Read(CHRSTATUS) | 0x0A00));
		}
		rtc_xosc_write(0x240F); /*crystal not exist + eosc cali = 0xF*/
		mdelay(10);
	}
}

static void rtc_switch_to_xosc_mode()
{
	rtc_switch_mode(true, false);
}

static void rtc_switch_to_dcxo_mode()
{
	rtc_switch_mode(false, false);
}

static void rtc_switch_to_xosc_recv_mode()
{
	rtc_switch_mode(true, true);
}

static void rtc_switch_to_dcxo_recv_mode()
{
	rtc_switch_mode(false, true);
}

static bool rtc_get_xosc_mode(void)
{
	U16 con, xosc_mode;;

	con = RTC_Read(RTC_OSC32CON);
	
	if((con & 0x0020) == 0)
	{
		xosc_mode = 1;
	} 
	else
		xosc_mode = 0;
	return xosc_mode;
}

static bool rtc_frequency_meter_check(void)
{
	U16  result[4];

	if (rtc_get_recovery_mode_stat())
		rtc_switch_to_xosc_recv_mode();
		
	rtc_measure_four_clock(result);
	if (rtc_xosc_check_clock(result))
	{
		rtc_xosc_write(0x0000);	/* crystal exist mode + XOSCCALI = 0 */
		return true;
	}
	else
	{
		if (!rtc_get_recovery_mode_stat())
			rtc_switch_to_dcxo_mode();
		else
			rtc_switch_to_dcxo_recv_mode();
	}

	rtc_measure_four_clock(result);
	
	if (rtc_eosc_check_clock(result))
	{
		U16 val;

		val = eosc_cali();
		print("[RTC] EOSC cali val = 0x%x\n", val);
		//EMB_HW_Mode
		val = (val & 0x001f)|0x2400;
		rtc_xosc_write(val);
		return true;
	}
	else
	{
		return false;
	}
}

static void rtc_set_recovery_mode_stat(bool enable)
{
	recovery_flag = enable;
}

static bool rtc_get_recovery_mode_stat(void)
{
	return recovery_flag;
}

static bool rtc_init_after_recovery(void)
{
	if (!Writeif_unlock())
		return false;
	/* write powerkeys */
	RTC_Write(RTC_POWERKEY1, RTC_POWERKEY1_KEY);
	RTC_Write(RTC_POWERKEY2, RTC_POWERKEY2_KEY);
	if (!Write_trigger())
		return false;

	RTC_Write(CHRSTATUS, (RTC_Read(CHRSTATUS) & ~0x0800));
	
	if (!rtc_gpio_init())
		return false;
	if (!rtc_android_init())
		return false;
	if (!rtc_lpd_init())
		return false;

	return true;
}
static bool rtc_recovery_mode_check(void)
{
	/////// fix me add return ret for recovery mode check fail
	if (!rtc_frequency_meter_check())
	{
		rtc_call_exception();
		return false;
	}
	return true;
}

static void rtc_recovery_flow(void)
{
	U8 count = 0;
	print("rtc_recovery_flow\n");
	rtc_set_recovery_mode_stat(true);
	while (count < 3)
	{
		if(rtc_recovery_mode_check()) 
		{
			if (rtc_init_after_recovery())
				break;
		} 
		count++;
	} 
	rtc_set_recovery_mode_stat(false);
	if (count == 3)
		rtc_call_exception();

}
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
extern kal_bool kpoc_flag ;
#endif

static unsigned long rtc_mktime(int yea, int mth, int dom, int hou, int min, int sec)
{
	unsigned long d1, d2, d3;

	mth -= 2;
	if (mth <= 0) {
		mth += 12;
		yea -= 1;
	}

	d1 = (yea - 1) * 365 + (yea / 4 - yea / 100 + yea / 400);
	d2 = (367 * mth / 12 - 30) + 59;
	d3 = d1 + d2 + (dom - 1) - 719162;
	
	return ((d3 * 24 + hou) * 60 + min) * 60 + sec;
}

static bool Write_trigger(void)
{
	RTC_Write(RTC_WRTGR, 1);
	if (rtc_busy_wait())
		return true;
	else
		return false;
}

static bool Writeif_unlock(void)
{
	RTC_Write(RTC_PROT, 0x586a);
	if (!Write_trigger())
		return false;
	RTC_Write(RTC_PROT, 0x9136);
	if (!Write_trigger())
		return false;
	
	return true;
}

static bool rtc_android_init(void)
{
	U16 irqsta;

	RTC_Write(RTC_IRQ_EN, 0);
	RTC_Write(RTC_CII_EN, 0);
	RTC_Write(RTC_AL_MASK, 0);

	RTC_Write(RTC_AL_YEA, 1970 - RTC_MIN_YEAR);
	RTC_Write(RTC_AL_MTH, 1);
	RTC_Write(RTC_AL_DOM, 1);
	RTC_Write(RTC_AL_DOW, 1);
	RTC_Write(RTC_AL_HOU, 0);
	RTC_Write(RTC_AL_MIN, 0);
	RTC_Write(RTC_AL_SEC, 0);

	RTC_Write(RTC_PDN1, 0x8000);   /* set Debug bit */
	RTC_Write(RTC_PDN2, ((1970 - RTC_MIN_YEAR) << 8) | 1);
	RTC_Write(RTC_SPAR0, 0);
	RTC_Write(RTC_SPAR1, (1 << 11));

	RTC_Write(RTC_DIFF, 0);
	RTC_Write(RTC_CALI, 0);
    if (!rtc_2sec_stat_clear())
        return false;
	if (!Write_trigger())
		return false;

	irqsta = RTC_Read(RTC_IRQ_STA);	/* read clear */

	/* init time counters after resetting RTC_DIFF and RTC_CALI */
	RTC_Write(RTC_TC_YEA, RTC_DEFAULT_YEA - RTC_MIN_YEAR);
	RTC_Write(RTC_TC_MTH, RTC_DEFAULT_MTH);
	RTC_Write(RTC_TC_DOM, RTC_DEFAULT_DOM);
	RTC_Write(RTC_TC_DOW, 1);
	RTC_Write(RTC_TC_HOU, 0);
	RTC_Write(RTC_TC_MIN, 0);
	RTC_Write(RTC_TC_SEC, 0);
	if(!Write_trigger())
		return false;

	return true;
}

static bool rtc_gpio_init(void)
{
	U16 con;

	/* GPI mode and pull enable + pull down */
	con = RTC_Read(RTC_CON) & 0x800c;
	con &= ~(RTC_CON_GOE | RTC_CON_GPU);
	con |= RTC_CON_GPEN | RTC_CON_F32KOB;
	RTC_Write(RTC_CON, con);
	if (Write_trigger())
		return true;
	else
		return false;
}

static U16 eosc_cali(void)
{
	U16 val, diff;
	int middle;
	int i, j;

	print("[RTC] EOSC_Cali: TOP_CKCON1=0x%x\n", RTC_Read(TOP_CKCON1));

	int left = 0x24C0, right=0x24DF;

	RTC_Write(TOP_CKCON2, (RTC_Read(TOP_CKCON2) | 0x8000));
	while( left<=(right) )
	{
		middle = (right + left) / 2;
		if(middle == left)
			break;

		val = get_frequency_meter(middle, 0x0004, 0);
		if ((val>792) && (val<796))
			break;
		if (val > 795)
			//right = middle - 1;
			right = middle;
		else
			//left = middle + 1;
			left = middle;
	}
	
	if ((val>792) && (val<796))
		return middle;
	
	val=get_frequency_meter(left, 0x0004, 0);
	diff=793-val;
	val=get_frequency_meter(right, 0x0004, 0);
	if(diff<(val-793))
		return left;
	else
		return right;	
}

static void rtc_osc_init(void)
{
	/* disable 32K export if there are no RTC_GPIO users */
	if (!(RTC_Read(RTC_PDN1) & RTC_GPIO_USER_MASK))
		rtc_gpio_init();
	
	if(rtc_get_xosc_mode())
	{
		U16 con;
		RTC_Write(TOP_CKTST2, 0x0);
		con = RTC_Read(RTC_OSC32CON);
		if ((con & 0x000f) != 0x0) {	/* check XOSCCALI */
			rtc_xosc_write(0x0003);  /* crystal exist mode + XOSCCALI = 0x3 */
			gpt_busy_wait_us(200);
		}
	
		rtc_xosc_write(0x0000);  /* crystal exist mode + XOSCCALI = 0x0 */
	}
	else
	{
		U16 val;
		RTC_Write(TOP_CKTST2, 0x80);
		val = eosc_cali();
		print("[RTC] EOSC cali val = 0x%x\n", val);
		//EMB_HW_Mode
		val = (val & 0x001f)|0x2400;
		rtc_xosc_write(val);		
	}
}

static bool rtc_lpd_init(void)
{
	U16 con;

	con = RTC_Read(RTC_CON) | RTC_CON_LPEN;
	con &= ~RTC_CON_LPRST;
	RTC_Write(RTC_CON, con);
	if (!Write_trigger())
		return false;

	con |= RTC_CON_LPRST;
	RTC_Write(RTC_CON, con);
	if (!Write_trigger())
		return false;

	con &= ~RTC_CON_LPRST;
	RTC_Write(RTC_CON, con);
	if (!Write_trigger())
		return false;

	return true;
}

static bool rtc_first_boot_init(void)
{
	print("rtc_first_boot_init\n");

	if (!Writeif_unlock())
		return false;

	if (!rtc_gpio_init())	
		return false;
	rtc_switch_to_xosc_mode();
	/* write powerkeys */
	RTC_Write(RTC_POWERKEY1, RTC_POWERKEY1_KEY);
	RTC_Write(RTC_POWERKEY2, RTC_POWERKEY2_KEY);
	if (!Write_trigger())
		return false;
	mdelay(1000);

	if (!rtc_frequency_meter_check())
		return false;
	if (!rtc_android_init())
		return false;
	if (!rtc_lpd_init())
		return false;

	return true;
}

static void rtc_bbpu_power_down(void)
{
	U16 bbpu;

	/* pull PWRBB low */
	bbpu = RTC_BBPU_KEY | RTC_BBPU_AUTO | RTC_BBPU_PWREN;
	Writeif_unlock();
	RTC_Write(RTC_BBPU, bbpu);
	Write_trigger();
}

#define RTC_SPAR0_32K_LESS 				(1U << 6)
bool rtc_crystal_exist_status(void)
{
	/*RTC_SPAR0: 
	bit 6 	 : 32K less bit. True:with 32K, False:Without 32K*/
	u16 spar0;
	
	spar0 = RTC_Read(RTC_SPAR0);
	if(spar0 & RTC_SPAR0_32K_LESS)
		return true;
	else
		return false;
}

/*DCXO setting for correct frequency. No need to disable 26MHz,when power on.
RG_DCXO_CON2 0x83e
RG_DCXO_MANUAL_CON1 0x844

PMIC_RG_DCXO_C2_UNTRIM_MASK 0x1
PMIC_RG_DCXO_MANUAL_SYNC_EN_MASK 0x1
PMIC_RG_DCXO_MANUAL_C1C2_SYNC_EN_MASK 0x1

PMIC_RG_DCXO_C2_UNTRIM_SHIFT 15
PMIC_RG_DCXO_MANUAL_SYNC_EN_SHIFT 8
PMIC_RG_DCXO_MANUAL_C1C2_SYNC_EN_SHIFT 9
*/
static void dcxo_setting_correct_frequency(void)
{
	u16 tmp = 0;
 
	tmp = RTC_Read(RG_DCXO_CON2);
	RTC_Write(RG_DCXO_CON2, tmp&(~(PMIC_RG_DCXO_C2_UNTRIM_MASK<<PMIC_RG_DCXO_C2_UNTRIM_SHIFT)));
	
	tmp = RTC_Read(RG_DCXO_MANUAL_CON1);
	RTC_Write(RG_DCXO_MANUAL_CON1, tmp|(PMIC_RG_DCXO_MANUAL_SYNC_EN_MASK<<PMIC_RG_DCXO_MANUAL_SYNC_EN_SHIFT));

	tmp =RTC_Read(RG_DCXO_MANUAL_CON1);
	RTC_Write(RG_DCXO_MANUAL_CON1, tmp|(PMIC_RG_DCXO_MANUAL_C1C2_SYNC_EN_MASK<<PMIC_RG_DCXO_MANUAL_C1C2_SYNC_EN_SHIFT));

	udelay(1);

	tmp = RTC_Read(RG_DCXO_MANUAL_CON1);
	RTC_Write(RG_DCXO_MANUAL_CON1, tmp&(~(PMIC_RG_DCXO_MANUAL_SYNC_EN_MASK<<PMIC_RG_DCXO_MANUAL_SYNC_EN_SHIFT)));
	
	tmp = RTC_Read(RG_DCXO_MANUAL_CON1);
	RTC_Write(RG_DCXO_MANUAL_CON1, tmp&(~(PMIC_RG_DCXO_MANUAL_C1C2_SYNC_EN_MASK<<PMIC_RG_DCXO_MANUAL_C1C2_SYNC_EN_SHIFT)));	
}

void rtc_bbpu_power_on(void)
{
	U16 bbpu;

	/* pull PWRBB high */
#if RTC_RELPWR_WHEN_XRST
	bbpu = RTC_BBPU_KEY | RTC_BBPU_AUTO | RTC_BBPU_BBPU | RTC_BBPU_PWREN;
#else
	bbpu = RTC_BBPU_KEY | RTC_BBPU_BBPU | RTC_BBPU_PWREN;
#endif
	RTC_Write(RTC_BBPU, bbpu);
	Write_trigger();
#if 1 //dxco setting, reduce DXCO 26M crystal settle time to 2~3ms
    if (rtc_crystal_exist_status())
	{   
		RTC_Write(0x854, RTC_Read(0x854)|0x0807);
		RTC_Write(0x85c, RTC_Read(0x85c)|0x0F07);
		print("[RTC] EXT32k Mode \n");
	}
	else
	{
	    print("[RTC] INT32k Mode \n");
	}

	RTC_Write(0x83e, RTC_Read(0x83e)|0x286A);
	
	print("[RTC] EXT32k Mode, DCXO REG Value 0x854=0x%x, 0x85c=0x%x, 0x83e=0x%x\n", RTC_Read(0x854), RTC_Read(0x85c), RTC_Read(0x83e));
	
#endif
	print("[RTC] rtc_bbpu_power_on done\n");

	dcxo_setting_correct_frequency();
    //detect hw clock done,close RG_RTC_75K_PDN for low power golden setting.
    RTC_Write(TOP_CKPDN2,RTC_Read(TOP_CKPDN2)|0x4000);
#if RTC_2SEC_REBOOT_ENABLE	
    rtc_enable_2sec_reboot();
#else
    RTC_Write(RTC_CALI, RTC_Read(RTC_CALI) & ~RTC_CALI_BBPU_2SEC_EN);
#endif
    rtc_clk_cfg_write();
}

void rtc_mark_bypass_pwrkey(void)
{
	U16 pdn1;

	pdn1 = RTC_Read(RTC_PDN1) | 0x0040;
	RTC_Write(RTC_PDN1, pdn1);
	Write_trigger();
}

static void rtc_clean_mark(void)
{
	U16 pdn1, pdn2;

	pdn1 = RTC_Read(RTC_PDN1) & ~0x8040;   /* also clear Debug bit */
	pdn2 = RTC_Read(RTC_PDN2) & ~0x0010;
	RTC_Write(RTC_PDN1, pdn1);
	RTC_Write(RTC_PDN2, pdn2);
	Write_trigger();
}

U16 rtc_rdwr_uart_bits(U16 *val)
{
	U16 pdn2;

	if (RTC_Read(RTC_CON) & RTC_CON_LPSTA_RAW)
		return 3;   /* UART bits are invalid due to RTC uninit */

	if (val) {
		pdn2 = RTC_Read(RTC_PDN2) & ~0x0060;
		pdn2 |= (*val & 0x0003) << 5;
		RTC_Write(RTC_PDN2, pdn2);
		Write_trigger();
	}

	return (RTC_Read(RTC_PDN2) & 0x0060) >> 5;
}

bool rtc_boot_check(void)
{
	U16 irqsta, pdn1, pdn2, spar0, spar1, Rdata;
	U16 result[4];
	bool check_mode_flag = false;
#ifdef MTK_ALPS_BOX_SUPPORT
	return false;
#endif
	//Disable RTC CLK gating
	RTC_Write(TOP_CKPDN, 0);
	RTC_Write(TOP_CKPDN2, 0);
#if 0
	RTC_Write(TOP_CKCON2, (RTC_Read(TOP_CKCON2)|0x8000) );//how many 26M pulse in one 32K pulse
#if 0
	//reset freq. meter
	RTC_Write(TOP_RST_CON, 0x0100);
	while( !(RTC_Read(FQMTR_CON2)==0) );
	RTC_Write(TOP_RST_CON, 0x0000);

	//Select fix clock source
	RTC_Write(TOP_CKTST2, 0x80); //swtich to EOSC32 
	RTC_Write(TOP_CKTST2, 0x00); //swtich to XOSC32_DET
	RTC_Write(FQMTR_CON1, 0); //set freq. meter window value (0=1X32K(fix clock))
	RTC_Write(FQMTR_CON0, 0x8004); //enable freq. meter, set measure clock to 26Mhz
	
	mdelay(1);

	while( (0x8 & RTC_Read(FQMTR_CON0))==0x8); //read until busy = 0
	Rdata = RTC_Read(FQMTR_CON2); //read data should be closed to 26M/32k = 812.5
	print("[RTC] FQMTR_CON2=0x%x\n", Rdata);
#endif
	
	RTC_Write(TOP_CKTST2, 0x00); //swtich to XOSC32_DET
	
	Rdata = RTC_Read(TOP_CKCON1);
	if(! (Rdata&0x0010) )
	{	
		print("[RTC] TOP_CKCON1=0x%x\n", Rdata);
		RTC_Write(TOP_CKCON1, Rdata|0x0010);
		mdelay(1);
	}
	Rdata = get_frequency_meter(0, 0x0004, 0);
	if(Rdata==0)
	{
		//crystal doesn't exist
		XOSC=0;
		print("[RTC] External crystal doesn't exist\n");
	}
	else
	{
		//crystal exist
		XOSC=1;
		print("[RTC] External crystal exist\n");
	}
#endif
	
	print("[RTC] bbpu = 0x%x, con = 0x%x\n", RTC_Read(RTC_BBPU), RTC_Read(RTC_CON));
	if ((RTC_Read(RTC_CON) & RTC_CON_LPSTA_RAW || check_mode_flag)) 
	{
		if (!rtc_first_boot_init()) {
			rtc_recovery_flow();
		}
	}
	else
	{
		/* normally HW reload is done in BROM but check again here */
		///fix me     rtc_busy_wait();///////
		if (!rtc_busy_wait())
			rtc_recovery_flow();
		///////////////////////////////////
		if (!Writeif_unlock())
			rtc_recovery_flow();
	}

	if (RTC_Read(RTC_POWERKEY1) != RTC_POWERKEY1_KEY ||
		RTC_Read(RTC_POWERKEY2) != RTC_POWERKEY2_KEY) 
	{
		print("[RTC] powerkey1 = 0x%x, powerkey2 = 0x%x\n",
			RTC_Read(RTC_POWERKEY1), RTC_Read(RTC_POWERKEY2));
		if (!rtc_first_boot_init()) {
			rtc_recovery_flow();
		}
	} else
	{
		RTC_Write(RTC_BBPU, RTC_Read(RTC_BBPU) | RTC_BBPU_KEY | RTC_BBPU_RELOAD);
		Write_trigger();
		rtc_osc_init();	
	}
	rtc_clean_mark();
	//set register to let MD know 32k status
	spar0 = RTC_Read(RTC_SPAR0);
	if(rtc_get_xosc_mode())
	{
		RTC_Write(RTC_SPAR0, (spar0 | 0x0040) );
	}
	else
	{
		RTC_Write(RTC_SPAR0, (spar0 & 0xffbf) );
	}
	Write_trigger();
	rtc_save_2sec_stat();

	irqsta = RTC_Read(RTC_IRQ_STA);	/* Read clear */
	pdn1 = RTC_Read(RTC_PDN1);
	pdn2 = RTC_Read(RTC_PDN2);
	spar0 = RTC_Read(RTC_SPAR0);
	spar1 = RTC_Read(RTC_SPAR1);
	print("[RTC] irqsta = 0x%x, pdn1 = 0x%x, pdn2 = 0x%x, spar0 = 0x%x, spar1 = 0x%x\n",
		  irqsta, pdn1, pdn2, spar0, spar1);
	print("[RTC] new_spare0 = 0x%x, new_spare1 = 0x%x, new_spare2 = 0x%x, new_spare3 = 0x%x\n",
		  RTC_Read(RTC_AL_HOU), RTC_Read(RTC_AL_DOM), RTC_Read(RTC_AL_DOW), RTC_Read(RTC_AL_MTH));
	print("[RTC] bbpu = 0x%x, con = 0x%x\n", RTC_Read(RTC_BBPU), RTC_Read(RTC_CON));


	if (irqsta & RTC_IRQ_STA_AL) {
#if RTC_RELPWR_WHEN_XRST
		/* set AUTO bit because AUTO = 0 when PWREN = 1 and alarm occurs */
		U16 bbpu = RTC_Read(RTC_BBPU) | RTC_BBPU_KEY | RTC_BBPU_AUTO;
		RTC_Write(RTC_BBPU, bbpu);
		Write_trigger();
#endif

		if (pdn1 & 0x0080) {	/* power-on time is available */
			U16 now_sec, now_min, now_hou, now_dom, now_mth, now_yea;
			U16 irqen, sec, min, hou, dom, mth, yea;
			unsigned long now_time, time;

			now_sec = RTC_Read(RTC_TC_SEC);
			now_min = RTC_Read(RTC_TC_MIN);
			now_hou = RTC_Read(RTC_TC_HOU);
			now_dom = RTC_Read(RTC_TC_DOM);
			now_mth = RTC_Read(RTC_TC_MTH);
			now_yea = RTC_Read(RTC_TC_YEA) + RTC_MIN_YEAR;
			if (RTC_Read(RTC_TC_SEC) < now_sec) {  /* SEC has carried */
				now_sec = RTC_Read(RTC_TC_SEC);
				now_min = RTC_Read(RTC_TC_MIN);
				now_hou = RTC_Read(RTC_TC_HOU);
				now_dom = RTC_Read(RTC_TC_DOM);
				now_mth = RTC_Read(RTC_TC_MTH);
				now_yea = RTC_Read(RTC_TC_YEA) + RTC_MIN_YEAR;
			}

			sec = spar0 & 0x003f;
			min = spar1 & 0x003f;
			hou = (spar1 & 0x07c0) >> 6;
			dom = (spar1 & 0xf800) >> 11;
			mth = pdn2 & 0x000f;
			yea = ((pdn2 & 0x7f00) >> 8) + RTC_MIN_YEAR;

			now_time = rtc_mktime(now_yea, now_mth, now_dom, now_hou, now_min, now_sec);
			time = rtc_mktime(yea, mth, dom, hou, min, sec);

			print("[RTC] now = %d/%d/%d %d:%d:%d (%u)\n",
				  now_yea, now_mth, now_dom, now_hou, now_min, now_sec, now_time);
			print("[RTC] power-on = %d/%d/%d %d:%d:%d (%u)\n",
				  yea, mth, dom, hou, min, sec, time);

			if (now_time >= time - 1 && now_time <= time + 4) {	 /* power on */
				pdn1 = (pdn1 & ~0x0080) | 0x0040;
				RTC_Write(RTC_PDN1, pdn1);
				RTC_Write(RTC_PDN2, pdn2 | 0x0010);
				Write_trigger();
				if (!(pdn2 & 0x8000))   /* no logo means ALARM_BOOT */
					g_boot_mode = ALARM_BOOT;
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
			if ((pdn1 & 0x4000) == 0x4000) {   
				kpoc_flag = false;
				RTC_Write(RTC_PDN1, pdn1 & ~0x4000);
				Write_trigger();
			}
#endif
				return true;
			} else if (now_time < time) {   /* set power-on alarm */
				RTC_Write(RTC_AL_YEA, yea - RTC_MIN_YEAR);
				RTC_Write(RTC_AL_MTH, (RTC_Read(RTC_AL_MTH)&0xff00)|mth);
				RTC_Write(RTC_AL_DOM, (RTC_Read(RTC_AL_DOM)&0xff00)|dom);
				RTC_Write(RTC_AL_HOU, (RTC_Read(RTC_AL_HOU)&0xff00)|hou);
				RTC_Write(RTC_AL_MIN, min);
				RTC_Write(RTC_AL_SEC, sec);
				RTC_Write(RTC_AL_MASK, 0x0010);	/* mask DOW */
				Write_trigger();
				irqen = RTC_Read(RTC_IRQ_EN) | RTC_IRQ_EN_ONESHOT_AL;
				RTC_Write(RTC_IRQ_EN, irqen);
				Write_trigger();
			}
		}
	}

	if ((pdn1 & 0x0030) == 0x0010) {	/* factory data reset */
		/* keep bit 4 set until rtc_boot_check() in U-Boot */
		return true;
	}
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
	if ((pdn1 & 0x4000) == 0x4000) {   
		kpoc_flag = true;
		return false;
	}
#endif

	return false;
}

void pl_power_off(void)
{
	print("[RTC] pl_power_off\n");

	rtc_bbpu_power_down();

	while (1);
}

static bool rtc_2sec_stat_clear(void)
{
	print("rtc_2sec_stat_clear\n");
	RTC_Write(RTC_CALI, RTC_Read(RTC_CALI) & ~RTC_CALI_BBPU_2SEC_STAT);
	if (!Write_trigger())
		return false;
	RTC_Write(RTC_CALI, RTC_Read(RTC_CALI) | RTC_CALI_BBPU_2SEC_STAT);
	if(!Write_trigger())
		return false;
	RTC_Write(RTC_CALI, RTC_Read(RTC_CALI) & ~RTC_CALI_BBPU_2SEC_STAT);
	if(!Write_trigger())
		return false;
	
	return true;
}

void rtc_save_2sec_stat(void)

{
	U16 cali;
	static bool save_stat=false;
	
	if(save_stat==true)
		return;
	else	
		save_stat = true;
	
	cali = RTC_Read(RTC_CALI);
	print("rtc_2sec_reboot_check: CALI = 0x%x\n",cali);
	if (cali & RTC_CALI_BBPU_2SEC_EN) {
		switch((cali & RTC_CALI_BBPU_2SEC_MODE_MSK) >> RTC_CALI_BBPU_2SEC_MODE_SHIFT) {
			case 0:
			case 1:
			case 2:
				if(cali & RTC_CALI_BBPU_2SEC_STAT) {
					rtc_2sec_stat_clear();
					g_rtc_2sec_stat = true;
				} else {
					rtc_2sec_stat_clear();
					g_rtc_2sec_stat = false;
				}
				break;
			case 3:
				rtc_2sec_stat_clear();
				g_rtc_2sec_stat = true;
			default:
				break;		
		}
	}
}

bool rtc_2sec_reboot_check(void)
{
	return g_rtc_2sec_stat;
}

void rtc_enable_2sec_reboot(void)
{
	U16 cali;
	U16 bbpu;
	
	cali = RTC_Read(RTC_CALI) | RTC_CALI_BBPU_2SEC_EN;
	cali = (cali & ~(RTC_CALI_BBPU_2SEC_MODE_MSK)) | (RTC_2SEC_MODE << RTC_CALI_BBPU_2SEC_MODE_SHIFT);
	RTC_Write(RTC_CALI, cali);
	Write_trigger();
}

