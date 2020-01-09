/*=============================================================================
 *                              Include Files
 *===========================================================================*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>

#include "libfile_op.h"
#include "libnvram.h"
#include "CFG_WIFI_Default.h"
#include "CFG_BT_File.h"
#include "CFG_SNData_File.h"
#include "CFG_SensorData_File.h"
#include "Custom_NvRam_LID.h"
#include <cutils/properties.h>

#define MAX_RETRY_COUNT 20

void show_help(void)
{
    printf("Usage: qcinvram [DEV] [CMD]\n");
    printf("       [DEV]: sn/wifi/bt/sensor/hdcp\n");
    printf("       [CMD]: for wifi/bt, getmac\n");
    printf("              for wifi, getcountrycode\n");
    printf("              for sn, getsmt/getdev/getfa/getsku\n");
    printf("              for sensor, get all\n");///getgsensor/getgyro\n");
    printf("                          get als kadc / get als offset\n");
    printf("                          get gsenosr axisx/ axisy/ axisz\n");
    printf("Command example: qcinvram sn getsmt/getdev/getfa/getsku\n");
    printf("                 qcinvram wifi getmac\n");
    printf("                 qcinvram wifi getcountrycode\n");
    printf("                 qcinvram sensor get als kadc\n");
    printf("                 qcinvram hdcp getkey\n");
    printf("Version: 2015.01.13.2\n");

    return;
}

bool checkNvramReady(void)
{
    int read_nvram_ready_retry = 0;
    int ret = 0;
    char nvram_init_val[PROPERTY_VALUE_MAX];
    while (read_nvram_ready_retry < MAX_RETRY_COUNT) {
        read_nvram_ready_retry++;
        property_get("nvram_init", nvram_init_val, NULL);
        if (strcmp(nvram_init_val, "Ready") == 0) {
            ret = true;
            break;
        }
        else {
            usleep(500 * 1000);
        }
    }
    //printf("Get nvram restore ready retry cc=%d\n", read_nvram_ready_retry);
    if (read_nvram_ready_retry >= MAX_RETRY_COUNT) {
        printf("Get nvram restore ready faild !!!\n");
        ret = false;
    }
    return ret;
}

bool getWiFiMacFromNvRam(WIFI_CFG_PARAM_STRUCT *pPara)
{
    F_ID wifi_nvram_fd;
    int file_lid = AP_CFG_RDEB_FILE_WIFI_LID;
    bool isRead = true;
    int i = 0, rec_sizem, rec_size, rec_num = 0;

    //printf("Waiting NvRam ready...\n");

    if (!checkNvramReady()) {
        printf("checkNvramReady FAIL!!!\n");
        return false;
    }

    wifi_nvram_fd = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);

    if (wifi_nvram_fd.iFileDesc < 0)
        return false;

    if (rec_size != read(wifi_nvram_fd.iFileDesc, pPara, rec_size))
        return false;


    if(!NVM_CloseFileDesc(wifi_nvram_fd))
        return false;

    return true;
}

bool getBTMacFromNvRam(ap_nvram_btradio_mt6610_struct *pPara)
{
    F_ID bt_nvram_fd;
    int file_lid = AP_CFG_RDEB_FILE_BT_ADDR_LID;
    bool isRead = true;
    int i = 0, rec_sizem, rec_size, rec_num = 0;

    //printf("Waiting NvRam ready...\n");

    if (!checkNvramReady()) {
        printf("checkNvramReady FAIL!!!\n");
        return false;
    }

    bt_nvram_fd = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);

    if (bt_nvram_fd.iFileDesc < 0)
        return false;

    if (rec_size != read(bt_nvram_fd.iFileDesc, pPara, rec_size))
        return false;


    if(!NVM_CloseFileDesc(bt_nvram_fd))
        return false;

    return true;
}

bool getSNFromNvRam(SN_CFG_Struct *pPara)
{
    F_ID sn_nvram_fd;
    int file_lid = AP_CFG_RDEB_FILE_SN_DATA_LID;
    bool isRead = true;
    int i = 0, rec_sizem, rec_size, rec_num = 0;

    //printf("Waiting NvRam ready...\n");

    if (!checkNvramReady()) {
        printf("checkNvramReady FAIL!!!\n");
        return false;
    }

    sn_nvram_fd = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);

    if (sn_nvram_fd.iFileDesc < 0)
        return false;

    if (rec_size != read(sn_nvram_fd.iFileDesc, pPara, rec_size))
        return false;


    if(!NVM_CloseFileDesc(sn_nvram_fd))
        return false;

    return true;
}

bool getSensorDataFromNvRam(SENSOR_DATA_CFG_Struct *pPara)
{
    F_ID sensor_nvram_fd;
    int file_lid = AP_CFG_RDEB_FILE_SENSOR_DATA_LID;
    bool isRead = true;
    int i = 0, rec_sizem, rec_size, rec_num = 0;

    //printf("Waiting NvRam ready...\n");

    if (!checkNvramReady()) {
        printf("checkNvramReady FAIL!!!\n");
        return false;
    }

    sensor_nvram_fd = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);

    if (sensor_nvram_fd.iFileDesc < 0)
        return false;

    if (rec_size != read(sensor_nvram_fd.iFileDesc, pPara, rec_size))
        return false;


    if(!NVM_CloseFileDesc(sensor_nvram_fd))
        return false;

    return true;
}

#if 0
bool getHdcpKeyFromNvRam(FILE_CUSTOM_HDCP_KEY_STRUCT *pPara)
{
    F_ID hdcpkey_nvram_fd;
    int file_lid = AP_CFG_RDCL_FILE_HDCP_KEY_LID;
    bool isRead = true;
    int i = 0, rec_sizem, rec_size, rec_num = 0;

    //printf("Waiting NvRam ready...\n");

    if (!checkNvramReady()) {
        printf("checkNvramReady FAIL!!!\n");
        return false;
    }

    hdcpkey_nvram_fd = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);

    if (hdcpkey_nvram_fd.iFileDesc < 0)
        return false;

    if (rec_size != read(hdcpkey_nvram_fd.iFileDesc, pPara, rec_size))
        return false;


    if(!NVM_CloseFileDesc(hdcpkey_nvram_fd))
        return false;

    return true;
}
#endif
int main(int argc, char *argv[])
{
	int num;
	char file_str[2528] = {'\0'};
	FILE *tmp_fd;
	UINT_32 version;
	//    EN_HDCP_RET (*encryption)(unsigned char *, unsigned char *);
	//    EN_HDCP_RET hdcp_result = EN_HDCP_R_INV_ARG;
	bool ret = false;
	WIFI_CFG_PARAM_STRUCT *pWiFiCfg_Value;
	ap_nvram_btradio_mt6610_struct *pBTCfg_Value;
	SN_CFG_Struct *pSNCfg_Value;
	SENSOR_DATA_CFG_Struct *pSensorCfg_Value;
	//    FILE_CUSTOM_HDCP_KEY_STRUCT *pHdcpKeyCfg_Value;

	if (argc > 1) {
		if (!strncmp(argv[1], "update2bin", 10)) {
			ret = FileOp_BackupToBinRegion_All();
			num = 0;
			while ((!ret) && (num < MAX_RETRY_COUNT)) {
				ret = FileOp_BackupToBinRegion_All();
				num++;
			}
			if (num >= MAX_RETRY_COUNT)
				printf("FAIL\n");
			else
				printf("SUCCESS\n");
		} else if (!strncmp(argv[1], "wifi", 4)) {
			if (argv[2] == NULL) {
				show_help();
				return 0;
			}

			pWiFiCfg_Value = (WIFI_CFG_PARAM_STRUCT *) malloc(sizeof(WIFI_CFG_PARAM_STRUCT));
			if (pWiFiCfg_Value == NULL)
				return 0;

			if (!strncmp(argv[2], "getmac", 6)) {
				ret = getWiFiMacFromNvRam(pWiFiCfg_Value);
				if (ret) {
					//printf("SUCCESS\n");
					printf("%02x:%02x:%02x:%02x:%02x:%02x",
							pWiFiCfg_Value->aucMacAddress[0], pWiFiCfg_Value->aucMacAddress[1], pWiFiCfg_Value->aucMacAddress[2],
							pWiFiCfg_Value->aucMacAddress[3], pWiFiCfg_Value->aucMacAddress[4], pWiFiCfg_Value->aucMacAddress[5]);
				} else {
					printf("FAIL\n");
				}
			} else if (!strncmp(argv[2], "getcountrycode", 14)) {
				ret = getWiFiMacFromNvRam(pWiFiCfg_Value);
				if (ret) {
					//printf("SUCCESS\n");
					printf("%02x,%02x",pWiFiCfg_Value->aucCountryCode[0],pWiFiCfg_Value->aucCountryCode[1]);
				} else {
					printf("Fail\n");
				}
			} else {
				printf("Wrong command for wifi!!!\n");
				show_help();
			}
			free(pWiFiCfg_Value);

		} else if (!strncmp(argv[1], "bt", 2)) {
			if (argv[2] == NULL) {
				show_help();
				return 0;
			}
			pBTCfg_Value = (ap_nvram_btradio_mt6610_struct *) malloc(sizeof(ap_nvram_btradio_mt6610_struct));
			if (pBTCfg_Value == NULL)
				return 0;

			if (!strncmp(argv[2], "getmac", 6)) {
				ret = getBTMacFromNvRam(pBTCfg_Value);
				if (ret) {
					//printf("SUCCESS\n");
					printf("%02x:%02x:%02x:%02x:%02x:%02x",
							pBTCfg_Value->addr[0], pBTCfg_Value->addr[1], pBTCfg_Value->addr[2],
							pBTCfg_Value->addr[3], pBTCfg_Value->addr[4], pBTCfg_Value->addr[5]);
				} else {
					printf("FAIL\n");
				}
			} else {
				printf("Wrong command for BT!!!\n");
				show_help();
			}
			free(pBTCfg_Value);

		// sensor begin
		} else if (!strncmp(argv[1], "sensor", 6)) {
			if (argv[2] == NULL) {
				show_help();
				return 0;
			}

			pSensorCfg_Value = (SENSOR_DATA_CFG_Struct *) malloc(sizeof(SENSOR_DATA_CFG_Struct));
			if (pSensorCfg_Value == NULL)
				return 0;

			if (!strncmp(argv[2], "get", 3)) {
				ret = getSensorDataFromNvRam(pSensorCfg_Value);
				if (ret) {
					if (!strncmp(argv[3], "als", 3)) {
						if (!strncmp(argv[4], "kadc", 4))
							printf("%d", pSensorCfg_Value->als_kadc);
						else if (!strncmp(argv[4], "offset", 6))
							printf("%d", pSensorCfg_Value->als_offset);
						else {
							printf("Wrong command for get als data!!!\n");
							show_help();
						}
					} else if (!strncmp(argv[3], "gsensor", 7)) {
						if (!strncmp(argv[4], "axisx", 5))
							printf("%x", pSensorCfg_Value->acc_x);
						else if (!strncmp(argv[4], "axisy", 5))
							printf("%x", pSensorCfg_Value->acc_y);
						else if (!strncmp(argv[4], "axisz", 5))
							printf("%x", pSensorCfg_Value->acc_z);
						else {
							printf("Wrong command for get psensor data!!!\n");
							show_help();
						}
					} else if (!strncmp(argv[3], "all", 3)) {
						//printf("SUCCESS\n");
						printf("%s:\n kadc = %d\n offset = %d\n", "ALS_CaliData", pSensorCfg_Value->als_kadc, pSensorCfg_Value->als_offset);
						printf("%s:\n axisx = %x \n axisy = %x\n axisz = %x\n", "GSensor_CaliData", pSensorCfg_Value->acc_x, pSensorCfg_Value->acc_y,pSensorCfg_Value->acc_z);
					} else {
						printf("Wrong command for get sensor data!!!\n");
						show_help();
					}
				} else
					printf("FAIL\n");
			} else {
				printf("Wrong command for sensor calibration data!!!\n");
				show_help();
			}
			free(pSensorCfg_Value);
			// sensor end
		//SN begin
		} else if (strcmp(argv[1], "sn") == 0) {
			if (argc < 3 || argv[2] == NULL) {
				show_help();
				return 0;
			}

			pSNCfg_Value = (SN_CFG_Struct *) malloc(sizeof(SN_CFG_Struct));
			if (pSNCfg_Value == NULL)
				return 0;

			if (!strncmp(argv[2], "get", 3)) {
				char* cptr = argv[2]+3;
				ret = getSNFromNvRam(pSNCfg_Value);
				if (ret) {
					if (strcmp(cptr, "smt") == 0)
						printf("%s", pSNCfg_Value->smt_sn);
					else if (strcmp(cptr, "dev") == 0)
						printf("%s", pSNCfg_Value->dev_sn);
					else if (strcmp(cptr, "fa") == 0)
						printf("%s", pSNCfg_Value->fa_sn);
					else if (strcmp(cptr, "sku") == 0)
						printf("%s", pSNCfg_Value->sku_id);
					else {
						printf("Wrong command for SN!!!\n");
						show_help();
					}
				} else
					printf("FAIL\n");
			} else {
				printf("Wrong command for SN!!!\n");
				show_help();
			}
			free(pSNCfg_Value);

		} else {
			show_help();
		}
	} else {
		show_help();
	}

	return 0;
}
