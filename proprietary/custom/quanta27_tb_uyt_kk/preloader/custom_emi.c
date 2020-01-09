#include "emi.h"

#define NUM_EMI_RECORD (1)

int num_of_emi_records = NUM_EMI_RECORD;

EMI_SETTINGS emi_settings[] =
{
     
	//COMMON_DDR3_1GB
	{
		0x0,		/* sub_version */
		0x0004,		/* TYPE */
		0,		/* EMMC ID/FW ID checking length */
		0,		/* FW length */
		{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},		/* NAND_EMMC_ID */
		{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},		/* FW_ID */
		0x00003122,		/* EMI_CONA_VAL */
		0xAA00AA00,		/* DRAMC_DRVCTL0_VAL */
		0xAA00AA00,		/* DRAMC_DRVCTL1_VAL */
		0x337A4684,		/* DRAMC_ACTIM_VAL */
		0x01000000,		/* DRAMC_GDDR3CTL1_VAL */
		0xF0748663,		/* DRAMC_CONF1_VAL */
		0xA18E21C1,		/* DRAMC_DDR2CTL_VAL */
		0xBF890401,		/* DRAMC_TEST2_3_VAL */
		0x03046978,		/* DRAMC_CONF2_VAL */
		0xD5972842,		/* DRAMC_PD_CTRL_VAL */
		0x00008888,		/* DRAMC_PADCTL3_VAL */
		0x88888888,		/* DRAMC_DQODLY_VAL */
		0x00000000,		/* DRAMC_ADDR_OUTPUT_DLY */
		0x00000000,		/* DRAMC_CLK_OUTPUT_DLY */
		0x00000650,		/* DRAMC_ACTIM1_VAL*/
		0x07800000,		/* DRAMC_MISCTL0_VAL*/
		0x040014E1,		/* DRAMC_ACTIM05T_VAL*/
		{0x40000000,0,0,0},		/* DRAM RANK SIZE */
		{0,0,0,0,0,0,0,0,0,0},		/* reserved 10 */
		0x00001B51,		/* PCDDR3_MODE_REG0 */
		0x00002000,		/* PCDDR3_MODE_REG1 */
		0x00004010,		/* PCDDR3_MODE_REG2 */
		0x00006000,		/* PCDDR3_MODE_REG3 */
		0x00000024,		/* PCDDR3_MODE_REG4 */
		0x00000000,		/* PCDDR3_MODE_REG5 */
	} ,
};
