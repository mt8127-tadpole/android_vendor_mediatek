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

//return value
typedef enum {
    EN_HDCP_R_OK = 0,                       //successfull
    EN_HDCP_R_INV_ARG,                      //invalid arguments
    EN_HDCP_R_NOT_DEFINE_CONTROL_TYPE,      //not defined control type
    EN_HDCP_R_NOT_ENCRYPTED_BIN,            //not encrypted HDCP binary
    EN_HDCP_R_SET_DECRYPT_KEY_ERR,          //set decrypted AES key error
    EN_HDCP_R_ENCRYPT_IC_ERR                //encrypted IC error
} EN_HDCP_RET;

void show_help(void)
{
    printf("Usage: qcinvram [DEV] [CMD] <PARAMETER>\n");
    printf("       [DEV]: sn/wifi/bt/sensor/hdcp\n");
    printf("       [CMD]: for wifi/bt, getmac/setmac\n");
    printf("              for wifi, getcountrycode/setcountrycode\n");
    printf("              for wifi, countrycode rule    : EU(ch1-13) = 45,55\n");
    printf("                                            : US(ch1-11) = 55,53\n");
    printf("                                            : JP(ch1-14) = 4a,50\n");
    printf("              for sn, getsmt/getdev/getfa/getsku/setsmt/setdev/setfa/setsku\n");
    printf("              for sensor, get all\n");///getgsensor/getgyro\n");
    printf("                          get als kadc / get als offset\n");
    printf("                          set als kadc / set als offset\n");
    printf("                          get gsenosr axisx/ axisy/ axisz\n");
    printf("                          set gsensor axisx/ axisy/ axisz\n");
    printf("                 \n");
    printf("Command example: qcinvram sn setsmt/setdev/setsku/setfa FEDCBA9876543210\n");
    printf("                 qcinvram wifi setmac 01:23:45:67:89:ab\n");
    printf("                 qcinvram wifi getmac\n");
    printf("                 qcinvram wifi setcountrycode 00,00\n");
    printf("                 qcinvram wifi getcountrycode\n");
    printf("                 qcinvram sensor set als kadc 56789\n");
    printf("                 qcinvram sensor get als kadc\n");
    printf("                 qcinvram hdcp setkey HDCPKEY.bin\n");
    printf("                 qcinvram hdcp getkey\n");
    printf("                 qcinvram update2bin\n");
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

bool setWiFiMacFromNvRam(WIFI_CFG_PARAM_STRUCT *pPara)
{
    F_ID wifi_nvram_fd;
    int file_lid = AP_CFG_RDEB_FILE_WIFI_LID;
    bool isRead = false;
    int i = 0, rec_sizem, rec_size, rec_num = 0;

    //printf("Waiting NvRam ready...\n");
    if (!checkNvramReady()) {
        printf("checkNvramReady FAIL!!!\n");
        return false;
    }

    wifi_nvram_fd = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);
    if (wifi_nvram_fd.iFileDesc < 0) {
        printf("Failed to open nvram file!!!\n");
        return false;
    }

    /* Change wifi mac address */
    if (rec_size != write(wifi_nvram_fd.iFileDesc, pPara, rec_size)) {
        printf("Failed to write nvram file!!!\n");
        return false;
    }

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

bool setBTMacFromNvRam(ap_nvram_btradio_mt6610_struct *pPara)
{
    F_ID bt_nvram_fd;
    int file_lid = AP_CFG_RDEB_FILE_BT_ADDR_LID;
    bool isRead = false;
    int i = 0, rec_sizem, rec_size, rec_num = 0;

    //printf("Waiting NvRam ready...\n");
    if (!checkNvramReady()) {
        printf("checkNvramReady FAIL!!!\n");
        return false;
    }

    bt_nvram_fd = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);
    if (bt_nvram_fd.iFileDesc < 0) {
        printf("Failed to open nvram file!!!\n");
        return false;
    }

    /* Change BT mac address */
    if (rec_size != write(bt_nvram_fd.iFileDesc, pPara, rec_size)) {
        printf("Failed to write nvram file!!!\n");
        return false;
    }

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


bool setSNFromNvRam(SN_CFG_Struct *pPara)
{
    F_ID sn_nvram_fd;
    int file_lid = AP_CFG_RDEB_FILE_SN_DATA_LID;
    bool isRead = false;
    int i = 0, rec_sizem, rec_size, rec_num = 0;

    //printf("Waiting NvRam ready...\n");
    if (!checkNvramReady()) {
        printf("checkNvramReady FAIL!!!\n");
        return false;
    }

    sn_nvram_fd = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);
    if (sn_nvram_fd.iFileDesc < 0) {
        printf("Failed to open nvram file!!!\n");
        return false;
    }

    /* Change SN */
    if (rec_size != write(sn_nvram_fd.iFileDesc, pPara, rec_size)) {
        printf("Failed to write nvram file!!!\n");
        return false;
    }

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

bool setSensorDataFromNvRam(SENSOR_DATA_CFG_Struct *pPara)
{
    F_ID sensor_nvram_fd;
    int file_lid = AP_CFG_RDEB_FILE_SENSOR_DATA_LID;
    bool isRead = false;
    int i = 0, rec_sizem, rec_size, rec_num = 0;

    //printf("Waiting NvRam ready...\n");
    if (!checkNvramReady()) {
        printf("checkNvramReady FAIL!!!\n");
        return false;
    }

    sensor_nvram_fd = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);
    if (sensor_nvram_fd.iFileDesc < 0) {
        printf("Failed to open nvram file!!!\n");
        return false;
    }

    /* Change SN */
    if (rec_size != write(sensor_nvram_fd.iFileDesc, pPara, rec_size)) {
        printf("Failed to write nvram file!!!\n");
        return false;
    }

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

bool setHdcpKeyFromNvRam(FILE_CUSTOM_HDCP_KEY_STRUCT *pPara)
{
    F_ID hdcpkey_nvram_fd;
    int file_lid = AP_CFG_RDCL_FILE_HDCP_KEY_LID;
    bool isRead = false;
    int i = 0, rec_sizem, rec_size, rec_num = 0;

    //printf("Waiting NvRam ready...\n");
    if (!checkNvramReady()) {
        printf("checkNvramReady FAIL!!!\n");
        return false;
    }

    hdcpkey_nvram_fd = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);
    if (hdcpkey_nvram_fd.iFileDesc < 0) {
        printf("Failed to open nvram file!!!\n");
        return false;
    }

    /* Change HDCP key */
    if (rec_size != write(hdcpkey_nvram_fd.iFileDesc, pPara, rec_size)) {
        printf("Failed to write nvram file!!!\n");
        return false;
    }

    if(!NVM_CloseFileDesc(hdcpkey_nvram_fd))
        return false;

    return true;
}
#endif
int main(int argc, char *argv[])
{
	int num;
	UINT_8 tmp_mac[6];
	UINT_8 tmp_countrycode[2];
	char file_str[2528] = {'\0'};
	char szSnData[SN_FIELD_LENGTH] = {'\0'};
	unsigned long tmp_data = 0;
	FILE *tmp_fd;
	UINT_32 version;
	long tmp_file_size;
	void* sdl_library;
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
			} else if (!strncmp(argv[2], "setmac", 6)) {
				if (argv[3] != NULL) {
					/* Parse mac address */
					num = sscanf(argv[3], "%02x:%02x:%02x:%02x:%02x:%02x", &tmp_mac[0], &tmp_mac[1], &tmp_mac[2], &tmp_mac[3], &tmp_mac[4], &tmp_mac[5]);
					ret = getWiFiMacFromNvRam(pWiFiCfg_Value);
					if (ret && (num == 6)) {
						memcpy(pWiFiCfg_Value->aucMacAddress, tmp_mac, sizeof(UINT_8)*6);
						ret = setWiFiMacFromNvRam(pWiFiCfg_Value);
						if (!ret)
							printf("FAIL\n");
						else
							printf("SUCCESS\n");
					} else {
						printf("Wrong wifi mac address!!!\n");
						show_help();
					}
				} else
					printf("Need mac address!!!\n");
			} else if (!strncmp(argv[2], "getcountrycode", 14)) {
				ret = getWiFiMacFromNvRam(pWiFiCfg_Value);
				if (ret) {
					//printf("SUCCESS\n");
					printf("%02x,%02x",pWiFiCfg_Value->aucCountryCode[0],pWiFiCfg_Value->aucCountryCode[1]);
				} else {
					printf("Fail\n");
				}
			} else if (!strncmp(argv[2], "setcountrycode", 14)) {
				if (argv[3] != NULL) {
					num = sscanf(argv[3], "%02x,%02x", &tmp_countrycode[0], &tmp_countrycode[1]);
					ret = getWiFiMacFromNvRam(pWiFiCfg_Value);
					if (ret && (num == 2)) {
						memcpy(pWiFiCfg_Value->aucCountryCode, tmp_countrycode, sizeof(UINT_8)*2);
						ret = setWiFiMacFromNvRam(pWiFiCfg_Value);
						if (!ret)
							printf("FAIL\n");
						else
							printf("SUCCESS\n");
					} else {
						printf("Wrong Country code!!!\n");
						show_help();
					}
				} else
					printf("Need Country code!!!\n");

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
			} else if (!strncmp(argv[2], "setmac", 6)) {
				if (argv[3] != NULL) {
					/* Parse mac address */
					num = sscanf(argv[3], "%02x:%02x:%02x:%02x:%02x:%02x", &tmp_mac[0], &tmp_mac[1], &tmp_mac[2], &tmp_mac[3], &tmp_mac[4], &tmp_mac[5]);
					ret = getBTMacFromNvRam(pBTCfg_Value);
					if (ret && (num == 6)) {
						memcpy(pBTCfg_Value->addr, tmp_mac, sizeof(unsigned char)*6);
						ret = setBTMacFromNvRam(pBTCfg_Value);
						if (!ret)
							printf("FAIL\n");
						else
							printf("SUCCESS\n");
					} else {
						printf("Wrong BT mac address!!!\n");
						show_help();
					}
				} else
					printf("Need mac address!!!\n");
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
					//} else if (!strncmp(argv[3], "gyro", 4)) {
					//	printf("%d\n", pSensorCfg_Value->gyro);
					} else if (!strncmp(argv[3], "all", 3)) {
						//printf("SUCCESS\n");
						printf("%s:\n kadc = %d\n offset = %d\n", "ALS_CaliData", pSensorCfg_Value->als_kadc, pSensorCfg_Value->als_offset);
						printf("%s:\n axisx = %x \n axisy = %x\n axisz = %x\n", "GSensor_CaliData", pSensorCfg_Value->acc_x, pSensorCfg_Value->acc_y,pSensorCfg_Value->acc_z);
				//		printf("%s: %d\n", "GSensor_CaliData", pSensorCfg_Value->gsensor);
				//		printf("%s: %d\n", "Gyro_CaliData", pSensorCfg_Value->gyro);
				//printf("%s: %d\n", "Resrv1_Sensor_CaliData", pSensorCfg_Value->resrv1_sensor);
				//printf("%s: %d\n", "Resrv2_Sensor_CaliData", pSensorCfg_Value->resrv2_sensor);
				//printf("%s: %d\n", "Resrv3_Sensor_CaliData", pSensorCfg_Value->resrv3_sensor);
				//printf("%s: %d\n", "Resrv4_Sensor_CaliData", pSensorCfg_Value->resrv4_sensor);
				//printf("%s: %d\n", "Resrv5_Sensor_CaliData", pSensorCfg_Value->resrv5_sensor);
				//printf("%s: %d\n", "Resrv6_Sensor_CaliData", pSensorCfg_Value->resrv6_sensor);
					} else {
						printf("Wrong command for get sensor data!!!\n");
						show_help();
					}
				} else
					printf("FAIL\n");
			} else if (!strncmp(argv[2], "set", 3)) {
				if (argv[3] != NULL) {
					/* Parse SN */
					if(!strncmp(argv[3], "als", 3)) {
						num = sscanf(argv[5], "%d", &tmp_data);
						ret = getSensorDataFromNvRam(pSensorCfg_Value);
					} else {
						num = sscanf(argv[5], "%x", &tmp_data);
						ret = getSensorDataFromNvRam(pSensorCfg_Value);
					}

					//printf("tmp_data %d\n", tmp_data);
					if (ret && (num == 1)) {
						if (!strncmp(argv[3], "als", 3)) {
							if (!strncmp(argv[4], "kadc", 4))
								pSensorCfg_Value->als_kadc = tmp_data;
							else if (!strncmp(argv[4], "offset", 6))
								pSensorCfg_Value->als_offset = tmp_data;
							else {
								printf("Wrong command for set als data!!!\n");
								show_help();
							}
						} else if (!strncmp(argv[3], "gsensor", 7)) {
							if (!strncmp(argv[4], "axisx", 5))
								pSensorCfg_Value->acc_x = tmp_data;
							else if (!strncmp(argv[4], "axisy", 5))
								pSensorCfg_Value->acc_y = tmp_data;
							else if (!strncmp(argv[4], "axisz", 5))
								pSensorCfg_Value->acc_z = tmp_data;
							else {
								printf("Wrong command for set psensor data!!!\n");
								show_help();
							}
						//else if (!strncmp(argv[2]+3, "gyro", 4))
						//	pSensorCfg_Value->gyro = tmp_data;
						} else {
							printf("Wrong command for set sensor data!!!\n");
							show_help();
						}

						ret = setSensorDataFromNvRam(pSensorCfg_Value);
						if (!ret)
							printf("FAIL\n");
						else
							printf("SUCCESS\n");
					} else
						printf("FAIL\n");
				} else
					printf("Please check command and sensor calibration data!!!\n");
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
				} else {
					printf("FAIL\n");
				}
			} else if (!strncmp(argv[2], "set", 3)) {
				if (argc > 3 && argv[3] != NULL) {
					/* Parse SN */
					memset(szSnData, 0, sizeof(szSnData));
					strncpy(szSnData, argv[3], sizeof(szSnData)-1);
					ret = getSNFromNvRam(pSNCfg_Value);
					if (ret) {
						char* cptr = argv[2]+3;
						char* cptrDest = NULL;
						if (strcmp(cptr, "smt") == 0)
							cptrDest = pSNCfg_Value->smt_sn;
						else if (strcmp(cptr, "dev") == 0)
							cptrDest = pSNCfg_Value->dev_sn;
						else if (strcmp(cptr, "fa") == 0)
							cptrDest = pSNCfg_Value->fa_sn;
						else if (strcmp(cptr, "sku") == 0)
							cptrDest = pSNCfg_Value->sku_id;
						else {
							printf("Wrong command for set SN!!!\n");
							show_help();
							free(pSNCfg_Value);
							return 0;
						}
						// copy data
						memcpy(cptrDest, szSnData, sizeof(char)*SN_FIELD_LENGTH);
						ret = setSNFromNvRam(pSNCfg_Value);
						if (!ret)
							printf("FAIL\n");
						else
							printf("SUCCESS\n");
					} else {
						printf("Wrong SN!!!\n");
						show_help();
					}
				} else
					printf("Need SN numbers!!!\n");
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
