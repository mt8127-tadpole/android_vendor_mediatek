#ifndef _SPM_TWAM_H_
#define _SPM_TWAM_H_

struct sig_desc_t {
	unsigned long sig;
	char name[40];
};

/* mt8127 */
struct sig_desc_t twam_sig[] = {
	{ 0,	"axi_idle_to_scpsys"},
	{ 1,	"faxi_all_axi_idle"},
	{ 2,	"emi_idle"},
	{ 3,	"disp_req"},
	{ 4,	"mfg_req"},
	{ 5,	"core0_wfi"},
	{ 6,	"core1_wfi"},
	{ 7,	"core2_wfi"},
	{ 8,	"core3_wfi"},
	{ 9,	"mcu_i2c_idle"},
	{ 10,	"mcu_scu_idle"},
	{ 11,	"dram_sref"},
	{ 12,	"md_srcclkena"},
	{ 13,	"md_apsrc_req"},
	{ 14,	"conn_srcclkena"},
	{ 15,	"conn_apsrc_req"}
};

#endif // _SPM_TWAM_H_
