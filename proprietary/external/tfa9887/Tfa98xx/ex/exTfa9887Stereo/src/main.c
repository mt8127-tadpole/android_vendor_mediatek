#include <Tfa98xx.h>
#include <Tfa98xx_Registers.h>
#include <assert.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <NXP_I2C.h>
#ifndef WIN32
// need PIN access
#include <inttypes.h>
#include <lxScribo.h>
#endif


#ifndef WIN32

#define Sleep(ms) usleep((ms)*1000)
#define _fileno fileno
#define _GNU_SOURCE   /* to avoid link issues with sscanf on NxDI? */
#endif
#define I2C_ADDRESS  0x6c

#ifdef WIN32
// cwd = dir where vcxproj is
#define LOCATION_FILES ".\\"
#else
// cwd = linux dir
#define LOCATION_FILES "../../../settings/"
#endif
#define PATCH_FILENAME "TFA9887_N1D2_2_4_1.patch"

/* the base speaker file, containing tCoef */
#define SPEAKER_FILENAME "KS_13X18_DUMBO_tCoef.speaker"
#define PRESET_FILENAME "HQ_KS_13X18_DUMBO.preset"
#define CONFIG_FILENAME "TFA9887_N1D2.config"
#define EQ_FILENAME "HQ_KS_13X18_DUMBO.eq"
//#define EQ_FILENAME "Rock.eq"

static float tCoefFromSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes)
{
	int iCoef;

	/* tCoef(A) is the last parameter of the speaker */
	iCoef = (speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-3]<<16) + (speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-2]<<8) + speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-1];

	return (float)iCoef/(1<<23);
}

static void tCoefToSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes, float tCoef)
{
	int iCoef;

	iCoef =(int)(tCoef*(1<<23));

	speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-3] = (iCoef>>16)&0xFF;
	speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-2] = (iCoef>>8)&0xFF;
	speakerBytes[TFA98XX_SPEAKERPARAMETER_LENGTH-1] = (iCoef)&0xFF;
}

static void muteAmplifier(Tfa98xx_handle_t handle)
{
	Tfa98xx_Error_t err;
	unsigned short status;

	/* signal the TFA98xx to mute plop free and turn off the amplifier */
	err = Tfa98xx_SetMute(handle, Tfa98xx_Mute_Amplifier);
	assert(err == Tfa98xx_Error_Ok);

	/* now wait for the amplifier to turn off */
	err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
	assert(err == Tfa98xx_Error_Ok);
	while ( (status & TFA98XX_STATUS_SWS) == TFA98XX_STATUS_SWS)
	{
		err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);
		assert(err == Tfa98xx_Error_Ok);
	}
}

static void loadSpeakerFile(const char* fileName, Tfa98xx_SpeakerParameters_t speakerBytes)
{
	int ret;
	FILE* f;

	printf("using speaker %s\n", fileName);

	f=fopen(fileName, "rb");
	assert(f!=NULL);
	ret = fread(speakerBytes, 1, sizeof(Tfa98xx_SpeakerParameters_t), f);
	assert(ret == sizeof(Tfa98xx_SpeakerParameters_t));
	fclose(f);
}

/* save the current speaker model to a file, for future use */
static void saveSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes, const char* fileName)
{
	int ret;
	FILE* f;

	f=fopen(fileName, "wb");
	assert(f!=NULL);
	ret = fwrite(speakerBytes, 1, sizeof(Tfa98xx_SpeakerParameters_t), f);
	assert(ret == sizeof(Tfa98xx_SpeakerParameters_t));
	fclose(f);
}

static void setConfig(Tfa98xx_handle_t handle, const char* fileName)
{
	Tfa98xx_Error_t err;
	Tfa98xx_Config_t config;
	int ret;
	FILE* f;

	printf("using config %s\n", fileName);

	f=fopen(fileName, "rb");
	assert(f!=NULL);
	ret = fread(config, 1, sizeof(config), f);
	assert(ret == TFA98XX_CONFIG_LENGTH);
	err = Tfa98xx_DspWriteConfig(handle, TFA98XX_CONFIG_LENGTH, config);
	assert(err == Tfa98xx_Error_Ok);
	fclose(f);
}

/* load a preset from a file, as generated by the GUI, can be done at runtime */
static void setPreset(Tfa98xx_handle_t handle, const char* fileName)
{
	int ret;
	int presetSize;
	unsigned char* buffer;
	FILE* f;
	struct stat st;
	Tfa98xx_Error_t err;
	
	printf("using preset %s\n", fileName);

	f=fopen(fileName, "rb");
	assert(f!=NULL);
	ret = fstat(_fileno(f), &st);
	assert(ret == 0);
	presetSize = st.st_size;
	assert(presetSize == TFA98XX_PRESET_LENGTH);
	buffer = (unsigned char*)malloc(presetSize);
	assert(buffer != NULL);
	ret = fread(buffer, 1, presetSize, f);
	assert(ret == presetSize);
	err = Tfa98xx_DspWritePreset( handle, TFA98XX_PRESET_LENGTH, buffer);
	assert(err == Tfa98xx_Error_Ok);
	fclose(f);
	free(buffer);
}

/* load a set of EQ settings from a file, as generated by the GUI, can be done at runtime */
static void setEQ(Tfa98xx_handle_t handle, const char* fileName)
{
	int ret;
	FILE* f;
	Tfa98xx_Error_t err;
	int ind; /* biquad index */
	float b0, b1, b2, a1, a2; /* the coefficients */
	int line = 1;
	char buffer[256];

	printf("using EQ %s\n", fileName);

	f=fopen(fileName, "rb");
	assert(f!=NULL);

	while (!feof(f))
	{
		if (NULL == fgets(buffer, sizeof(buffer)-1, f) )
		{
			break;
		}
		ret = sscanf(buffer, "%d %f %f %f %f %f", &ind, &b0, &b1, &b2, &a1, &a2);
		if (ret == 6)
		{
			if ((b0 != 1) || (b1 != 0) || (b2 != 0) || (a1 != 0) || (a2 != 0)) {
				err = Tfa98xx_DspBiquad_SetCoeff(handle, ind, b0, b1, b2, a1, a2);
				assert(err == Tfa98xx_Error_Ok);
				printf("Loaded biquad %d\n", ind);
      } else {
        err = Tfa98xx_DspBiquad_Disable(handle, ind);
				assert(err == Tfa98xx_Error_Ok);
				printf("Disabled biquad %d\n", ind);
			}
		}
		else {
			printf("error parsing file, line %d\n", line);
		}
		line++;
	}
	fclose(f);
}

/* load a DSP ROM code patch from file */
static Tfa98xx_Error_t dspPatch(Tfa98xx_handle_t handle, const char* fileName)
{
	int ret;
	int fileSize;
	unsigned char* buffer;
	FILE* f;
	struct stat st;
	Tfa98xx_Error_t err;

	f=fopen(fileName, "rb");
	assert(f!=NULL);
	ret = fstat(_fileno(f), &st);
	assert(ret == 0);
	fileSize = st.st_size;
	buffer = (unsigned char*)malloc(fileSize);
	assert(buffer != NULL);
	ret = fread(buffer, 1, fileSize, f);
	assert(ret == fileSize);
	err = Tfa98xx_DspPatch(handle, fileSize, buffer);
	assert(err == Tfa98xx_Error_Ok);
	fclose(f);
	free(buffer);

   return err;
}

static char* stateFlagsStr(int stateFlags)
{
	static char flags[10];
	
	flags[0] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_Activity)) ? 'A':'a';
	flags[1] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_S_Ctrl)) ? 'S':'s';
	flags[2] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_Muted)) ? 'M':'m';
	flags[3] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_X_Ctrl)) ? 'X':'x';
	flags[4] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_T_Ctrl)) ? 'T':'t';
	flags[5] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_NewModel)) ? 'L':'l';
	flags[6] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_VolumeRdy)) ? 'V':'v';
	flags[7] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_Damaged)) ? 'D':'d';
	flags[8] = (stateFlags & (0x1<<Tfa98xx_SpeakerBoost_SignalClipping)) ? 'C':'c';

	flags[9] = 0;
	return flags;
}

static void dump_state_info(Tfa98xx_StateInfo_t* pState)
{
  printf("state: flags %s, agcGain %2.1f\tlimGain %2.1f\tsMax %2.1f\tT %d\tX1 %2.1f\tX2 %2.1f\tRe %2.2f\tshortOnMips %d\n", 
				stateFlagsStr(pState->statusFlag),
				pState->agcGain,
				pState->limGain,
				pState->sMax,
				pState->T,
				pState->X1,
				pState->X2,
				pState->Re,
				pState->shortOnMips);
}

static void load_all_settings(Tfa98xx_handle_t handle, Tfa98xx_SpeakerParameters_t speakerBytes, const char* configFile, const char* presetFile, const char* eqFile)
{
	Tfa98xx_Error_t err;

	/* load fullmodel */
	err = Tfa98xx_DspWriteSpeakerParameters(handle, TFA98XX_SPEAKERPARAMETER_LENGTH, speakerBytes);
	assert(err == Tfa98xx_Error_Ok);

	/* load the settings */
	setConfig(handle, configFile);
	/* load a preset */
	setPreset(handle, presetFile);
	/* set the equalizer to Rock mode */
	setEQ(handle, eqFile);
}

static void waitCalibration(Tfa98xx_handle_t handle, int *calibrateDone)
{
	Tfa98xx_Error_t err;
	int tries = 0;
	unsigned short mtp;
#define WAIT_TRIES 1000

	err = Tfa98xx_ReadRegister16(handle, TFA98XX_MTP, &mtp);

	/* in case of calibrate once wait for MTPEX */
	if ( mtp & TFA98XX_MTP_MTPOTC) {
		while ( (*calibrateDone == 0) && (tries < TFA98XX_API_WAITRESULT_NTRIES))
		{	// TODO optimise with wait estimation
			err = Tfa98xx_ReadRegister16(handle, TFA98XX_MTP, &mtp);
			*calibrateDone = ( mtp & TFA98XX_MTP_MTPEX);	/* check MTP bit1 (MTPEX) */
			tries++;
		}
	} else /* poll xmem for calibrate always */
	{
		while ((*calibrateDone == 0) && (tries<WAIT_TRIES) )
		{	// TODO optimise with wait estimation
			err = Tfa98xx_DspReadMem(handle, 231, 1, calibrateDone);
			tries++;
		}
		if(tries==WAIT_TRIES)
			printf("calibrateDone 231 timedout\n");
	}

}

#ifdef SPDIF_AUDIO
typedef int  (__stdcall *SetPin_type)(unsigned char pinNumber, unsigned short value);
static SetPin_type SetPin;

void InitSpdifAudio()
{
  HMODULE hDll;
	unsigned char i2c_UDAmode2[] = {0x34, 0x00, 0x28, 0x2E};
	unsigned char i2c_UDAI2SSpdif[] = {0x34, 0x50, 0x01, 0x51};

	/* copied the relevant code from C:\Program Files\NXP\I2C\C\CrdI2c.c
	 */
	hDll = LoadLibrary(L"Scribo.dll");
	if (hDll == 0) {
		fprintf(stderr, "Could not open Scribo.dll\n");
		return ;
	}

	SetPin = (SetPin_type) GetProcAddress(hDll, "SetPin");
	if (SetPin == NULL) {
		FreeLibrary(hDll);
		return; // function not found in library
	}

	SetPin(4, 0x1); /* Weak pull up on PA4 powers up the UDA1355 */
	NXP_I2C_Write(sizeof(i2c_UDAmode2), i2c_UDAmode2);
	NXP_I2C_Write(sizeof(i2c_UDAI2SSpdif), i2c_UDAI2SSpdif);
}
#endif
#ifdef USB_AUDIO
#ifdef WIN32
typedef int  (__stdcall *SetPin_type)(unsigned char pinNumber, unsigned short value);
static SetPin_type SetPin;

void InitUsbAudio()
{
    HMODULE hDll;
	int ret;

	/* copied the relevant code from C:\Program Files\NXP\I2C\C\CrdI2c.c
	 */
	hDll = LoadLibrary(L"Scribo.dll");
	if (hDll == 0) {
		fprintf(stderr, "Could not open Scribo.dll\n");
		return ;
	}

	SetPin = (SetPin_type) GetProcAddress(hDll, "SetPin");
	if (SetPin == NULL) {
		FreeLibrary(hDll);
		return; // function not found in library
	}

	ret = SetPin(4, 0x8000); /* Active low on PA4 switches off UDA1355. */
}
#else
void InitUsbAudio()
{
	int fd;
	fd = lxScriboGetFd();
	lxScriboSetPin(fd, 4, 0x8000);
}
#endif
#endif /* USB_AUDIO */

Tfa98xx_Error_t	calculateSpeakertCoefA(
										int handle_cnt, Tfa98xx_handle_t handles[],
										Tfa98xx_SpeakerParameters_t loadedSpeaker[] )
{
	Tfa98xx_Error_t err;
	float re25, tCoef, tCoefA;
	int Tcal; /* temperature at which the calibration happened */
	int T0;
   int h = 0;
   int calibrateDone = 0;

	for (h=0;h<handle_cnt;h++)
   {
      err = Tfa98xx_DspGetCalibrationImpedance(handles[h], &re25);
 	   assert(err == Tfa98xx_Error_Ok);
	   assert(fabs(re25) < 0.1); /* no calibration done yet */
      tCoef = tCoefFromSpeaker(loadedSpeaker[h]);
      /* use dummy tCoefA, also eases the calculations, because tCoefB=re25 */
	   tCoefToSpeaker(loadedSpeaker[h], 0.0f);
      load_all_settings(handles[h], loadedSpeaker[h], LOCATION_FILES CONFIG_FILENAME, LOCATION_FILES PRESET_FILENAME, LOCATION_FILES EQ_FILENAME);

      /* start calibration and wait for result */
      err = Tfa98xx_SetConfigured(handles[h]);
	   assert(err == Tfa98xx_Error_Ok);

      waitCalibration(handles[h], &calibrateDone);
      if (calibrateDone)
      {
         Tfa98xx_DspGetCalibrationImpedance(handles[h],&re25);
      }
      else
      {
         printf("Calibration is not done");
         re25 = 0;
      }
      /* Reading the device temperature of the calibration after the calibration is ready*/
	   err = Tfa98xx_DspReadMem(handles[h], 232, 1, &Tcal);
	   assert(err == Tfa98xx_Error_Ok);
	   printf("Resistance of speaker is %2.2f ohm @ %d degrees\n", re25, Tcal);

	   /* calculate the tCoefA */
	   T0 = 25; /* definition of temperature for Re0 */
	   tCoefA = tCoef * re25 / (tCoef * (Tcal - T0)+1); /* TODO: need Rapp influence */
	   printf("Calculated tCoefA %1.5f\n", tCoefA);

	   /* update the speaker model */
	   tCoefToSpeaker(loadedSpeaker[h], tCoefA);
   }
	/* !!! The value is only written to the speaker file “loadspeaker” located in the memory of 
    * the host and not in the physical file itself.
    * The physical file will always contain tCoef. The host needs to save tCoefA in this “loadedSpeaker” as it is needed 
    * after the next cold boot to write the tCoefA value into MTP !!! */


	return err;
}



static void resetMtpEx(Tfa98xx_handle_t handle)
{
	Tfa98xx_Error_t err;
	unsigned short mtp;
	unsigned short status;

	/* reset MTPEX bit because calibration happened with wrong tCoefA */
	err = Tfa98xx_ReadRegister16(handle, TFA98XX_MTP, &mtp);
	assert(err == Tfa98xx_Error_Ok);
	/* all settings loaded, signal the DSP to start calibration, only needed once after cold boot */

	/* reset MTPEX bit if needed */
	if ( (mtp & TFA98XX_MTP_MTPOTC) && (mtp & TFA98XX_MTP_MTPEX)) 
	{
		err = Tfa98xx_WriteRegister16(handle, 0x0B, 0x5A); /* unlock key2 */
		assert(err == Tfa98xx_Error_Ok);

		err = Tfa98xx_WriteRegister16(handle, TFA98XX_MTP, 1); /* MTPOTC=1, MTPEX=0 */
		assert(err == Tfa98xx_Error_Ok);
		err = Tfa98xx_WriteRegister16(handle, 0x62, 1<<11); /* CIMTP=1 */
		assert(err == Tfa98xx_Error_Ok);
	}
#if 0
	Sleep(13*16); /* need to wait until all parameters are copied into MTP */
#else
	do
	{
		Sleep(10);
		err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUS, &status);
		assert(err == Tfa98xx_Error_Ok);
		
	} while ( (status & TFA98XX_STATUS_MTPB) == TFA98XX_STATUS_MTPB); /*TODO: add timeout*/
	assert( (status & TFA98XX_STATUS_MTPB) == 0);
#endif

}

static void coldStartup(Tfa98xx_handle_t handle)
{
	Tfa98xx_Error_t err;
	unsigned short status = 0;
   unsigned short value = 0;
   int tries = 0;

	/* load the optimal TFA98XX in HW settings */
	err = Tfa98xx_Init(handle);
	assert(err == Tfa98xx_Error_Ok);

	err = Tfa98xx_SetSampleRate(handle, 44100);
	assert(err == Tfa98xx_Error_Ok);

	err = Tfa98xx_Powerdown(handle, 0);
	assert(err == Tfa98xx_Error_Ok);
	
   printf("Waiting for AMPS and CLKS to start up\n"); // TODO make a clock wait in runtime
	err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUS, &status);

   while ( ( (status & TFA98XX_STATUS_CLKS) == 0) && 
            (tries < TFA98XX_API_WAITRESULT_NTRIES) )
	{
		/* not ok yet */
		err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUS, &status);
		assert(err == Tfa98xx_Error_Ok);
      tries++;
	}

   if (tries >= TFA98XX_API_WAITRESULT_NTRIES)
   {
      printf("CLKS start up timed-out\n");
   }
   tries = 0;

   while ( ( (status & TFA98XX_STATUS_AMPS) == 0) && 
            (tries < TFA98XX_API_WAITRESULT_NTRIES) )
	{
		/* not ok yet */
		err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUS, &status);
		assert(err == Tfa98xx_Error_Ok);
      tries++;
	}

   if (tries >= TFA98XX_API_WAITRESULT_NTRIES)
   {
      printf("AMPS start up timed-out\n");
   }
   tries = 0;

   /* some other registers must be set for optimal amplifier behaviour */
   if (Tfa98xx_Error_Ok == err) 
   {
      err = Tfa98xx_WriteRegister16(handle, TFA98XX_CF_CONTROLS, 0x01);
   }

   /* Setting Cool flux enable */
   err = Tfa98xx_ReadRegister16(handle, TFA98XX_SYS_CTRL, &value);
   assert(err == Tfa98xx_Error_Ok);

   value |= TFA98XX_SYSTEM_CONTROL_CFE;/*(1<<2)*/
  
   err = Tfa98xx_WriteRegister16(handle, TFA98XX_SYS_CTRL, value);
   assert(err == Tfa98xx_Error_Ok);

   err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUS, &status);
   assert(err == Tfa98xx_Error_Ok);

   while ( ( (status & TFA98XX_STATUS_AMPS) == 1) && 
            (tries < TFA98XX_API_WAITRESULT_NTRIES) )
	{
		/* not ok yet */
	   err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUS, &status);
	   assert(err == Tfa98xx_Error_Ok);
      tries++;
	}
   tries = 0;

   /* Wait for Cool flux to be ready to be released */
   Sleep(10);

   /* Load cold-boot patch for the first time cold start-up*/
   err = dspPatch(handle, LOCATION_FILES "coldboot.patch");

   err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUSREG, &status);

	if ( !(status & TFA98XX_STATUS_ACS))
   {
      printf("Not Cold booted");
      return;  /* ensure cold booted */
   }

   /* Enabling the amplifier */
   value |= TFA98XX_SYSTEM_CONTROL_AMPE; /*(1<<3)*/
  
	err = Tfa98xx_WriteRegister16(handle, TFA98XX_SYS_CTRL, value);

	/* cold boot, need to load all parameters and patches */
	/* patch the ROM code */
   /* Tfa9887 does not have patch at this moment */
	err = dspPatch(handle, LOCATION_FILES PATCH_FILENAME);
}

static void setOtc(Tfa98xx_handle_t handle, unsigned short otcOn)
{
	Tfa98xx_Error_t err;
	unsigned short mtp;
	unsigned short status;
	int mtpChanged = 0;

	err = Tfa98xx_ReadRegister16(handle, TFA98XX_MTP, &mtp);
	assert(err == Tfa98xx_Error_Ok);
	
	assert((otcOn == 0) || (otcOn == 1) );

	/* set reset MTPEX bit if needed */
	if ( (mtp & TFA98XX_MTP_MTPOTC) != otcOn) 
	{
		/* need to change the OTC bit, set MTPEX=0 in any case */
		err = Tfa98xx_WriteRegister16(handle, 0x0B, 0x5A); /* unlock key2 */
		assert(err == Tfa98xx_Error_Ok);

		err = Tfa98xx_WriteRegister16(handle, TFA98XX_MTP, otcOn); /* MTPOTC=otcOn, MTPEX=0 */
		assert(err == Tfa98xx_Error_Ok);
		err = Tfa98xx_WriteRegister16(handle, 0x62, 1<<11); /* CIMTP=1 */
		assert(err == Tfa98xx_Error_Ok);
		
		mtpChanged =1;
		
	}
	//Sleep(13*16); /* need to wait until all parameters are copied into MTP */
	do
	{
		Sleep(10);
		err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUS, &status);
		assert(err == Tfa98xx_Error_Ok);
		
	} while ( (status & TFA98XX_STATUS_MTPB) == TFA98XX_STATUS_MTPB);
	assert( (status & TFA98XX_STATUS_MTPB) == 0);
	if (mtpChanged)
	{
		/* ensure the DSP restarts after this to read out the new value */
		err = Tfa98xx_WriteRegister16(handle, 0x70, 0x1); /* DSP reset */
		assert(err == Tfa98xx_Error_Ok);
	}
}

static void statusCheck(Tfa98xx_handle_t handle)
{
	Tfa98xx_Error_t err;
	unsigned short status;

   /* Check status from register 0*/
	err = Tfa98xx_ReadRegister16(handle, TFA98XX_STATUS, &status);
   if (status & TFA98XX_STATUS_WDS)
   {
      printf("DSP watchDog triggerd");
      return;
   }
}

static int checkMTPEX(Tfa98xx_handle_t handle)
{
	unsigned short mtp;
	Tfa98xx_Error_t err;	
	err = Tfa98xx_ReadRegister16(handle, TFA98XX_MTP, &mtp);
    assert(err == Tfa98xx_Error_Ok);
  
    if ( mtp & (1<<1))	/* check MTP bit1 (MTPEX) */
		return 1;					/* MTPEX is 1, calibration is done */
	else 
		return 0;					/* MTPEX is 0, calibration is not done yet */
}

int main(/* int argc, char* argv[] */)
{
	Tfa98xx_Error_t err;
	Tfa98xx_handle_t handles[3];
	unsigned char h;
   int i;
	Tfa98xx_Mute_t mute;
	Tfa98xx_SpeakerParameters_t lsModel;
   Tfa98xx_SpeakerParameters_t loadedSpeaker[2] = {0};
	Tfa98xx_StateInfo_t stateInfo;
	float re25;
	float tCoefA;
   int calibrateDone = 0;

   //NXP_I2C_EnableLogging(1);
	for (h=0; h<2; h++)
	{
		/* create handle */
		err = Tfa98xx_Open(I2C_ADDRESS+2*h, &handles[h]);
		assert(err == Tfa98xx_Error_Ok);
   }
#ifdef SPDIF_AUDIO
	   InitSpdifAudio();
#endif
#ifdef USB_AUDIO
	InitUsbAudio();
#endif

   /* When using "tCoefA patch" tCoefA, the loudspeaker model does not need to be saved into by the host anymore*/
	/* tCoefA will be saved into MTP automatically by CoolFlux */
	/* cold boot, need to load all parameters and patches */
   for (h=0; h<2; h++)
	{
      coldStartup(handles[h]);

      /*Set to calibration once*/
      setOtc(handles[h], 1);

      loadSpeakerFile(LOCATION_FILES SPEAKER_FILENAME, loadedSpeaker[h]);
   }
   
   /* Check if MTPEX bit is set for calibration once mode
   /* Calibration always would always do the complete 2 step calibration*/
   if( (checkMTPEX(handles[0]) == 0) && (checkMTPEX(handles[1]) == 0) )
   {
	   printf("DSP not yet calibrated. 2 step calibration will start.\n");
        
	   for (h=0; h<2; h++)
	   {
         /* ensure no audio during special calibration */
	      err = Tfa98xx_SetMute(handles[h], Tfa98xx_Mute_Digital);
		   assert(err == Tfa98xx_Error_Ok);
      }

		err = calculateSpeakertCoefA(2, handles, loadedSpeaker);
		assert(err == Tfa98xx_Error_Ok);

      for (h=0; h<2; h++)
	   {
		   /* if we were in one-time calibration (OTC) mode, clear the calibration results 
		   from MTP so next time 2nd calibartion step can start. */
		   resetMtpEx(handles[h]);

		   /* force recalibration now with correct tCoefA */
		   muteAmplifier(handles[h]); /* clean shutdown to avoid plop */
		   coldStartup(handles[h]);
      }
	}
	else
	{
	   printf("DSP already calibrated. Calibration skipped and previous calibration results loaded from MTP.\n");
	}

   for (h=0; h<2; h++)
	{
      load_all_settings(handles[h], loadedSpeaker[h], LOCATION_FILES CONFIG_FILENAME, LOCATION_FILES PRESET_FILENAME, LOCATION_FILES EQ_FILENAME);
   }

   /* Select stereo channels*/
   err = Tfa98xx_SelectChannel(handles[0], Tfa98xx_Channel_L);
	assert(err == Tfa98xx_Error_Ok);
	err = Tfa98xx_SelectChannel(handles[1], Tfa98xx_Channel_R);
	assert(err == Tfa98xx_Error_Ok);

	/* ensure stereo routing is correct: in this example we use
	* gain is on L channel from 1->2
	* gain is on R channel from 2->1
	* on the other channel of DATAO we put Isense
	*/

	err = Tfa98xx_SelectI2SOutputLeft(handles[0], Tfa98xx_I2SOutputSel_DSP_Gain);
	assert(err == Tfa98xx_Error_Ok);
	err = Tfa98xx_SelectStereoGainChannel(handles[1], Tfa98xx_StereoGainSel_Left);
	assert(err == Tfa98xx_Error_Ok);

	err = Tfa98xx_SelectI2SOutputRight(handles[1], Tfa98xx_I2SOutputSel_DSP_Gain);
	assert(err == Tfa98xx_Error_Ok);
	err = Tfa98xx_SelectStereoGainChannel(handles[0], Tfa98xx_StereoGainSel_Right);
	assert(err == Tfa98xx_Error_Ok);

	err = Tfa98xx_SelectI2SOutputRight(handles[0], Tfa98xx_I2SOutputSel_CurrentSense);
	assert(err == Tfa98xx_Error_Ok);
	err = Tfa98xx_SelectI2SOutputLeft(handles[1], Tfa98xx_I2SOutputSel_CurrentSense);
	assert(err == Tfa98xx_Error_Ok);

   for (h=0; h<2; h++)
	{
	   /* do calibration (again), if needed */
	   err = Tfa98xx_SetConfigured(handles[h]); /* the real calibration started*/ 
	   assert(err == Tfa98xx_Error_Ok);
      /* Wait until the calibration is done. 
      * The MTPEX bit would be set and remain as 1 if MTPOTC is set to 1 */
      waitCalibration(handles[h], &calibrateDone);
      if (calibrateDone)
      {
         Tfa98xx_DspGetCalibrationImpedance(handles[h],&re25);
      }
      else
      {
         printf("Calibration is not done");
         re25 = 0;
      }
      printf("Calibration value is %2.2f ohm\n", re25);
   
      /*Checking the current status for DSP status and DCPVP */
      statusCheck(handles[h]);
   }

   for (h=0; h<2; h++)
	{
	   err = Tfa98xx_SetMute(handles[h], Tfa98xx_Mute_Digital);
	   assert(err == Tfa98xx_Error_Ok);
	   err = Tfa98xx_GetMute(handles[h], &mute);
	   assert(err == Tfa98xx_Error_Ok);

	   assert(mute == Tfa98xx_Mute_Digital);

	   /* Start playing music here */
	   //DebugBreak();

	   err = Tfa98xx_SetMute(handles[h], Tfa98xx_Mute_Off);
	   assert(err == Tfa98xx_Error_Ok);
   }

   /*Starting to retrieve the live data info for 100 loop*/
   for (i=0; i<100; ++i)
	{
	   for (h=0; h<2; h++)
	   {
         err = (Tfa98xx_Error_t)Tfa98xx_DspGetStateInfo(handles[h], &stateInfo);
		   assert(err == Tfa98xx_Error_Ok);
         printf("device %d:\n", h);
		   dump_state_info(&stateInfo);
		   Sleep(500);
	   }
   }

	/* playing with the volume: first down, then up again */
	for (i=2; i<5; i++)
	{
	   float vol;
      for (h=0; h<2; h++)
	   {
		   printf("Setting volume to %3.1f dB\n", -6.0f*i);
		   err = Tfa98xx_SetVolume(handles[h], -6.0f*i);
		   assert(err == Tfa98xx_Error_Ok);
		   err = Tfa98xx_GetVolume(handles[h], &vol);
		   assert(err == Tfa98xx_Error_Ok);
		   assert( fabs(-6.0f*i - vol) < 0.001) ;
		   err = (Tfa98xx_Error_t)Tfa98xx_DspGetStateInfo(handles[h], &stateInfo);
		   assert(err == Tfa98xx_Error_Ok);
		   dump_state_info(&stateInfo);
		   Sleep(1000);
	   }
   }

	for (i=5; i>=0; i--)
	{
	   for (h=0; h<2; h++)
	   {
         printf("Setting volume to %3.1f dB\n", -6.0f*i);
		   err = Tfa98xx_SetVolume(handles[h], -6.0f*i);
		   assert(err == Tfa98xx_Error_Ok);
		   dump_state_info(&stateInfo);
		   Sleep(1000);
	   }
   }

	Sleep(5000);

   for (h=0; h<2; h++)
	{
	   /* check LS model */
	   err = Tfa98xx_DspReadSpeakerParameters(handles[h], TFA98XX_SPEAKERPARAMETER_LENGTH, lsModel);
	   assert(err == Tfa98xx_Error_Ok);
      /*Remark: tCoefA value could be get by using the function below to verify the tCoefA value*/
      tCoefA = tCoefFromSpeaker(lsModel);

	   err = Tfa98xx_SetMute(handles[h], Tfa98xx_Mute_Amplifier);
	   assert(err == Tfa98xx_Error_Ok);

	   err = Tfa98xx_Powerdown(handles[h], 1);
	   assert(err == Tfa98xx_Error_Ok);

	   err = Tfa98xx_Close(handles[h]);
	   assert(err == Tfa98xx_Error_Ok);
   }
   return 0;

}
