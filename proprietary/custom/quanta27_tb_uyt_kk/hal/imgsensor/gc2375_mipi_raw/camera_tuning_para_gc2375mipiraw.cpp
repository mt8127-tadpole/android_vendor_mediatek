#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

#include "camera_custom_nvram.h"
#include "camera_custom_sensor.h"
#include "image_sensor.h"
#include "kd_imgsensor_define.h"
#include "camera_AE_PLineTable_gc2375mipiraw.h"
#include "camera_info_gc2375mipiraw.h"
#include "camera_custom_AEPlinetable.h"
#include "camera_custom_tsf_tbl.h"
const NVRAM_CAMERA_ISP_PARAM_STRUCT CAMERA_ISP_DEFAULT_VALUE =
{{
    //Version
    Version: NVRAM_CAMERA_PARA_FILE_VERSION,
    //SensorId
    SensorId: SENSOR_ID,
    ISPComm:{
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        }
    },
    ISPPca:{
        #include INCLUDE_FILENAME_ISP_PCA_PARAM
    },
    ISPRegs:{
        #include INCLUDE_FILENAME_ISP_REGS_PARAM
        },
    ISPMfbMixer:{{
        {//00: MFB mixer for ISO 100
            0x00000000, 0x00000000
        },
        {//01: MFB mixer for ISO 200
            0x00000000, 0x00000000
        },
        {//02: MFB mixer for ISO 400
            0x00000000, 0x00000000
        },
        {//03: MFB mixer for ISO 800
            0x00000000, 0x00000000
        },
        {//04: MFB mixer for ISO 1600
            0x00000000, 0x00000000
        },
        {//05: MFB mixer for ISO 2400
            0x00000000, 0x00000000
        },
        {//06: MFB mixer for ISO 3200
            0x00000000, 0x00000000
        }
    }},
    ISPCcmPoly22:{
        68117,    // i4R_AVG
        15414,    // i4R_STD
        94042,    // i4B_AVG
        22219,    // i4B_STD
        {  // i4P00[9]
            5385199, -2189936, -632917, -1046882, 4459806, -855266, -653123, -1289884, 4500660
        },
        {  // i4P10[9]
            211675, -858244, 645533, 261643, -996017, 736222, 660731, 306902, -966596
        },
        {  // i4P01[9]
            -1004027, -165969, 1168252, 231959, -1632668, 1401966, 512407, -398016, -112646
        },
        {  // i4P20[9]
            -21698, -114702, 134260, 74404, 167256, -242894, -170250, -492687, 665076
        },
        {  // i4P11[9]
            1922829, -1248077, -677641, -204692, 1241740, -1037925, -561002, -509935, 1073826
        },
        {  // i4P02[9]
            1464621, -707898, -758190, -113705, 942102, -825202, -473554, -437338, 912359
        }
    }
}};

const NVRAM_CAMERA_3A_STRUCT CAMERA_3A_NVRAM_DEFAULT_VALUE =
{
    NVRAM_CAMERA_3A_FILE_VERSION, // u4Version
    SENSOR_ID, // SensorId

    // AE NVRAM
    {
        // rDevicesInfo
        {
            1024,    // u4MinGain, 1024 base = 1x
            8192,    // u4MaxGain, 16x
            100,    // u4MiniISOGain, ISOxx  
            64,    // u4GainStepUnit, 1x/8 
            26667,    // u4PreExpUnit 
            30,    // u4PreMaxFrameRate
            26667,    // u4VideoExpUnit  
            30,    // u4VideoMaxFrameRate 
            1024,    // u4Video2PreRatio, 1024 base = 1x 
            26667,    // u4CapExpUnit 
            30,    // u4CapMaxFrameRate
            1024,    // u4Cap2PreRatio, 1024 base = 1x
            28,    // u4LensFno, Fno = 2.8
            0    // u4FocusLength_100x
        },
        // rHistConfig
        {
            2,    // u4HistHighThres
            40,    // u4HistLowThres
            2,    // u4MostBrightRatio
            1,    // u4MostDarkRatio
            160,    // u4CentralHighBound
            20,    // u4CentralLowBound
            {240, 230, 220, 210, 200},    // u4OverExpThres[AE_CCT_STRENGTH_NUM] 
            {86, 108, 128, 148, 170},    // u4HistStretchThres[AE_CCT_STRENGTH_NUM] 
            {18, 22, 26, 30, 34}    // u4BlackLightThres[AE_CCT_STRENGTH_NUM] 
        },
        // rCCTConfig
        {
            TRUE,    // bEnableBlackLight
            TRUE,    // bEnableHistStretch
            FALSE,    // bEnableAntiOverExposure
            TRUE,    // bEnableTimeLPF
            FALSE,    // bEnableCaptureThres
            FALSE,    // bEnableVideoThres
            FALSE,    // bEnableStrobeThres
            20,    // u4AETarget
            0,    // u4StrobeAETarget
            20,    // u4InitIndex
            4,    // u4BackLightWeight
            32,    // u4HistStretchWeight
            4,    // u4AntiOverExpWeight
            2,    // u4BlackLightStrengthIndex
            2,    // u4HistStretchStrengthIndex
            2,    // u4AntiOverExpStrengthIndex
            2,    // u4TimeLPFStrengthIndex
            {1, 3, 5, 7, 8},    // u4LPFConvergeTable[AE_CCT_STRENGTH_NUM] 
            90,    // u4InDoorEV = 9.0, 10 base 
            -10,    // i4BVOffset delta BV = value/10 
            64,    // u4PreviewFlareOffset
            64,    // u4CaptureFlareOffset
            5,    // u4CaptureFlareThres
            64,    // u4VideoFlareOffset
            5,    // u4VideoFlareThres
            32,    // u4StrobeFlareOffset
            2,    // u4StrobeFlareThres
            50,    // u4PrvMaxFlareThres
            0,    // u4PrvMinFlareThres
            50,    // u4VideoMaxFlareThres
            0,    // u4VideoMinFlareThres
            18,    // u4FlatnessThres    // 10 base for flatness condition.
            75    // u4FlatnessStrength
        }
    },
    // AWB NVRAM
    {
        // AWB calibration data
        {
            // rUnitGain (unit gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rGoldenGain (golden sample gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rTuningUnitGain (Tuning sample unit gain: 1.0 = 512)
            {
                0,    // i4R
                0,    // i4G
                0    // i4B
            },
            // rD65Gain (D65 WB gain: 1.0 = 512)
            {
                918,    // i4R
                512,    // i4G
                746    // i4B
            }
        },
        // Original XY coordinate of AWB light source
        {
           // Strobe
            {
                0,    // i4X
                0    // i4Y
            },
            // Horizon
            {
                -441,    // i4X
                -314    // i4Y
            },
            // A
            {
                -331,    // i4X
                -351    // i4Y
            },
            // TL84
            {
                -197,    // i4X
                -362    // i4Y
            },
            // CWF
            {
                -119,    // i4X
                -411    // i4Y
            },
            // DNP
            {
                77,    // i4X
                -355    // i4Y
            },
            // D65
            {
                77,    // i4X
                -355    // i4Y
            },
            // DF
            {
                0,    // i4X
                0    // i4Y
            }
        },
        // Rotated XY coordinate of AWB light source
        {
            // Strobe
            {
                0,    // i4X
                0    // i4Y
            },
            // Horizon
            {
                -441,    // i4X
                -314    // i4Y
            },
            // A
            {
                -331,    // i4X
                -351    // i4Y
            },
            // TL84
            {
                -197,    // i4X
                -362    // i4Y
            },
            // CWF
            {
                -119,    // i4X
                -411    // i4Y
            },
            // DNP
            {
                77,    // i4X
                -355    // i4Y
            },
            // D65
            {
                77,    // i4X
                -355    // i4Y
            },
            // DF
            {
                0,    // i4X
                0    // i4Y
            }
        },
        // AWB gain of AWB light source
        {
            // Strobe 
            {
                512,    // i4R
                512,    // i4G
                512    // i4B
            },
            // Horizon 
            {
                512,    // i4R
                608,    // i4G
                1691    // i4B
            },
            // A 
            {
                526,    // i4R
                512,    // i4G
                1289    // i4B
            },
            // TL84 
            {
                641,    // i4R
                512,    // i4G
                1091    // i4B
            },
            // CWF 
            {
                760,    // i4R
                512,    // i4G
                1049    // i4B
            },
            // DNP 
            {
                918,    // i4R
                512,    // i4G
                746    // i4B
            },
            // D65 
            {
                918,    // i4R
                512,    // i4G
                746    // i4B
            },
            // DF 
            {
                512,    // i4R
                512,    // i4G
                512    // i4B
            }
        },
        // Rotation matrix parameter
        {
            0,    // i4RotationAngle
            256,    // i4Cos
            0    // i4Sin
        },
        // Daylight locus parameter
        {
            -118,    // i4SlopeNumerator
            128    // i4SlopeDenominator
        },
        // AWB light area
        {
            // Strobe:FIXME
            {
            0,    // i4RightBound
            0,    // i4LeftBound
            0,    // i4UpperBound
            0    // i4LowerBound
            },
            // Tungsten
            {
            -247,    // i4RightBound
            -897,    // i4LeftBound
            -282,    // i4UpperBound
            -382    // i4LowerBound
            },
            // Warm fluorescent
            {
            -247,    // i4RightBound
            -897,    // i4LeftBound
            -382,    // i4UpperBound
            -502    // i4LowerBound
            },
            // Fluorescent
            {
            27,    // i4RightBound
            -247,    // i4LeftBound
            -278,    // i4UpperBound
            -386    // i4LowerBound
            },
            // CWF
            {
            27,    // i4RightBound
            -247,    // i4LeftBound
            -386,    // i4UpperBound
            -461    // i4LowerBound
            },
            // Daylight
            {
            102,    // i4RightBound
            27,    // i4LeftBound
            -275,    // i4UpperBound
            -435    // i4LowerBound
            },
            // Shade
            {
            462,    // i4RightBound
            102,    // i4LeftBound
            -275,    // i4UpperBound
            -435    // i4LowerBound
            },
            // Daylight Fluorescent
            {
            0,    // i4RightBound
            0,    // i4LeftBound
            0,    // i4UpperBound
            0    // i4LowerBound
            }
        },
        // PWB light area
        {
            // Reference area
            {
            462,    // i4RightBound
            -897,    // i4LeftBound
            -250,    // i4UpperBound
            -502    // i4LowerBound
            },
            // Daylight
            {
            127,    // i4RightBound
            27,    // i4LeftBound
            -275,    // i4UpperBound
            -435    // i4LowerBound
            },
            // Cloudy daylight
            {
            227,    // i4RightBound
            52,    // i4LeftBound
            -275,    // i4UpperBound
            -435    // i4LowerBound
            },
            // Shade
            {
            327,    // i4RightBound
            52,    // i4LeftBound
            -275,    // i4UpperBound
            -435    // i4LowerBound
            },
            // Twilight
            {
            27,    // i4RightBound
            -133,    // i4LeftBound
            -275,    // i4UpperBound
            -435    // i4LowerBound
            },
            // Fluorescent
            {
            127,    // i4RightBound
            -297,    // i4LeftBound
            -305,    // i4UpperBound
            -461    // i4LowerBound
            },
            // Warm fluorescent
            {
            -231,    // i4RightBound
            -431,    // i4LeftBound
            -305,    // i4UpperBound
            -461    // i4LowerBound
            },
            // Incandescent
            {
            -231,    // i4RightBound
            -431,    // i4LeftBound
            -275,    // i4UpperBound
            -435    // i4LowerBound
            },
            // Gray World
            {
            5000,    // i4RightBound
            -5000,    // i4LeftBound
            5000,    // i4UpperBound
            -5000    // i4LowerBound
            }
        },
        // PWB default gain	
        {
            // Daylight
            {
            919,    // i4R
            512,    // i4G
            746    // i4B
            },
            // Cloudy daylight
            {
            1000,    // i4R
            512,    // i4G
            685    // i4B
            },
            // Shade
            {
            1070,    // i4R
            512,    // i4G
            641    // i4B
            },
            // Twilight
            {
            771,    // i4R
            512,    // i4G
            890    // i4B
            },
            // Fluorescent
            {
            766,    // i4R
            512,    // i4G
            965    // i4B
            },
            // Warm fluorescent
            {
            549,    // i4R
            512,    // i4G
            1346    // i4B
            },
            // Incandescent
            {
            529,    // i4R
            512,    // i4G
            1296    // i4B
            },
            // Gray World
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        // AWB preference color	
        {
            // Tungsten
            {
            50,    // i4SliderValue
            4418    // i4OffsetThr
            },
            // Warm fluorescent	
            {
            50,    // i4SliderValue
            4418    // i4OffsetThr
            },
            // Shade
            {
            50,    // i4SliderValue
            841    // i4OffsetThr
            },
            // Daylight WB gain
            {
            919,    // i4R
            512,    // i4G
            746    // i4B
            },
            // Preference gain: strobe
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: tungsten
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: warm fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: CWF
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: shade
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            },
            // Preference gain: daylight fluorescent
            {
            512,    // i4R
            512,    // i4G
            512    // i4B
            }
        },
        {// CCT estimation
            {// CCT
                2300,    // i4CCT[0]
                2850,    // i4CCT[1]
                4100,    // i4CCT[2]
                5100,    // i4CCT[3]
                6500    // i4CCT[4]
            },
            {// Rotated X coordinate
                -518,    // i4RotatedXCoordinate[0]
                -408,    // i4RotatedXCoordinate[1]
                -274,    // i4RotatedXCoordinate[2]
                0,    // i4RotatedXCoordinate[3]
                0    // i4RotatedXCoordinate[4]
            }
        }
    },
    {0}
};

#include INCLUDE_FILENAME_ISP_LSC_PARAM
//};  //  namespace

const CAMERA_TSF_TBL_STRUCT CAMERA_TSF_DEFAULT_VALUE =
{
    #include INCLUDE_FILENAME_TSF_PARA
    #include INCLUDE_FILENAME_TSF_DATA
};


typedef NSFeature::RAWSensorInfo<SENSOR_ID> SensorInfoSingleton_T;


namespace NSFeature {
template <>
UINT32
SensorInfoSingleton_T::
impGetDefaultData(CAMERA_DATA_TYPE_ENUM const CameraDataType, VOID*const pDataBuf, UINT32 const size) const
{
    UINT32 dataSize[CAMERA_DATA_TYPE_NUM] = {sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT),
                                             sizeof(NVRAM_CAMERA_3A_STRUCT),
                                             sizeof(NVRAM_CAMERA_SHADING_STRUCT),
                                             sizeof(NVRAM_LENS_PARA_STRUCT),
                                             sizeof(AE_PLINETABLE_T),
                                             0,
                                             sizeof(CAMERA_TSF_TBL_STRUCT)};

    if (CameraDataType > CAMERA_DATA_TSF_TABLE || NULL == pDataBuf || (size < dataSize[CameraDataType]))
    {
        return 1;
    }

    switch(CameraDataType)
    {
        case CAMERA_NVRAM_DATA_ISP:
            memcpy(pDataBuf,&CAMERA_ISP_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_ISP_PARAM_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_3A:
            memcpy(pDataBuf,&CAMERA_3A_NVRAM_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_3A_STRUCT));
            break;
        case CAMERA_NVRAM_DATA_SHADING:
            memcpy(pDataBuf,&CAMERA_SHADING_DEFAULT_VALUE,sizeof(NVRAM_CAMERA_SHADING_STRUCT));
            break;
        case CAMERA_DATA_AE_PLINETABLE:
            memcpy(pDataBuf,&g_PlineTableMapping,sizeof(AE_PLINETABLE_T));
            break;
        case CAMERA_DATA_TSF_TABLE:
            memcpy(pDataBuf,&CAMERA_TSF_DEFAULT_VALUE,sizeof(CAMERA_TSF_TBL_STRUCT));
            break;
        default:
            break;
    }
    return 0;
}}; // NSFeature


