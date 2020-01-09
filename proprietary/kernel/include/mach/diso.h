#ifndef DISO_H
#define DISO_H

#include <mach/mt_typedefs.h>


/*****************************************************************************
 *
 ****************************************************************************/
typedef unsigned int	   WORD;


#define DISO_IRQ_DISABLE  0
#define DISO_IRQ_ENABLE  1
#define DISO_IRQ_FALLING 0
#define DISO_IRQ_RISING 1
#define DISO_ONLINE  KAL_TRUE
#define DISO_OFFLINE KAL_FALSE

/*****************************************************************************
 *  structure
 ****************************************************************************/
typedef enum {
		IDLE = 0,
		OTG_ONLY,
		USB_ONLY,
		USB_WITH_OTG, // not support
		DC_ONLY,
		DC_WITH_OTG,
		DC_WITH_USB,
		DC_USB_OTG,//not support
}DISO_STATE;

typedef  struct {
	bool  cur_otg_state :1;
	bool  cur_vusb_state :1;
	bool cur_vdc_state :1;
	bool pre_otg_state :1;
	bool pre_vusb_state :1;
	bool pre_vdc_state :1;
}DISO_state;

typedef  struct {
	INT32		 number;
	INT32		 period;
	INT32		 debounce;
	INT32        falling_threshold;
	INT32        rising_threshold;
}DISO_channel_data;
typedef struct 
{
	DISO_state	 diso_state;
	INT32		 irq;
	DISO_channel_data vdc_measure_channel;
	DISO_channel_data vusb_measure_channel;
	INT32		 switch_rising_timing;
	INT32		 switch_falling_timing;

	UINT32		 vdc_hv_voltage;		 
	UINT32		 vdc_lv_voltage;		 
	UINT32		 vusb_hv_voltage;		 
	UINT32		 vusb_lv_voltage;		 
	UINT32		 irq_line_number;		 
} DISO_ChargerStruct;

/*****************************************************************************
 *  Extern Variable
 ****************************************************************************/
extern DISO_ChargerStruct DISO_data;


/*****************************************************************************
 *  Extern Function
 ****************************************************************************/

extern bool mt_usb_is_host(void);
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
extern void mt_auxadc_enableBackgroundDection(u16 channel, u16 volt, u16 period, u16 debounce, u16  tFlag);
extern void mt_auxadc_disableBackgroundDection(u16 channel);
extern u16 mt_auxadc_getCurrentChannel();
extern u16 mt_auxadc_getCurrentTrigger();
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
extern int IMM_auxadc_GetOneChannelValue_Cali(int Channel, int*voltage);
extern int IMM_IsAdcInitReady(void);
extern void set_vdc_auxadc_irq(bool  enable, bool flag);
extern void set_vusb_auxadc_irq(bool  enable, bool flag);

#endif //#ifndef DISO_H
