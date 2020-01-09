#ifndef _CFG_SENSORDATA_FILE_H
#define _CFG_SENSORDATA_FILE_H

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
typedef struct
{
/* Light Sensor */
    unsigned long als_kadc; /* Light Sensor */
    unsigned long als_offset;
/* G Sensor */
    unsigned long acc_x; 
    unsigned long acc_y;
    unsigned long acc_z;
/* Reserved Sensors */
    unsigned long resrv1_sensor;
    unsigned long resrv2_sensor;
    unsigned long resrv3_sensor;
    unsigned long resrv4_sensor;
    unsigned long resrv5_sensor;
    unsigned long resrv6_sensor;
} SENSOR_DATA_CFG_Struct;

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define CFG_FILE_SENSOR_DATA_REC_SIZE    sizeof(SENSOR_DATA_CFG_Struct)
#define CFG_FILE_SENSOR_DATA_REC_TOTAL   1

#endif
