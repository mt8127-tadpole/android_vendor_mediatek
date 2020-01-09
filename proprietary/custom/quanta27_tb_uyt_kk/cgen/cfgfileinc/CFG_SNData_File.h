#ifndef _CFG_SN_FILE_H
#define _CFG_SN_FILE_H

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

#define SN_FIELD_LENGTH 32

typedef struct
{
    char smt_sn[SN_FIELD_LENGTH]; /* SMT SN */
    char dev_sn[SN_FIELD_LENGTH]; /* Device SN */
    char fa_sn[SN_FIELD_LENGTH]; /* FA SN */
    char sku_id[SN_FIELD_LENGTH]; /* SKU-ID */
/* Reserved IDs */
    char resrv1_id[SN_FIELD_LENGTH];
    char resrv2_id[SN_FIELD_LENGTH];
    char resrv3_id[SN_FIELD_LENGTH];
    char resrv4_id[SN_FIELD_LENGTH];
    char resrv5_id[SN_FIELD_LENGTH];
    char resrv6_id[SN_FIELD_LENGTH];
    char resrv7_id[SN_FIELD_LENGTH];
    char resrv8_id[SN_FIELD_LENGTH];
    char resrv9_id[SN_FIELD_LENGTH];
} SN_CFG_Struct;

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define CFG_FILE_SN_REC_SIZE    sizeof(SN_CFG_Struct)
#define CFG_FILE_SN_REC_TOTAL   1

#endif
