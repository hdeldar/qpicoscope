/***********************************************************************************************************************************************
*
* Filename:    PS2000Acon.c
*
* Copyright:   Pico Technology Limited 2010
*
* Author:      RPM
*
* Description:
*   This is a console mode program that demonstrates how to use the
*   PicoScope 2000a series API.
*
* Examples:
*    Collect a block of samples immediately
*    Collect a block of samples when a trigger event occurs
*    Collect a stream of data immediately
*    Collect a stream of data when a trigger event occurs
*    Set Signal Generator, using standard or custom signals
* 
* Digital Examples (MSO veriants only): 
*    Collect a block of digital samples immediately
*    Collect a block of digital samples when a trigger event occurs
*    Collect a block of analogue & digital samples when analogue AND digital trigger events occurs
*    Collect a block of analogue & digital samples when analogue OR digital trigger events occurs
*    Collect a stream of digital data immediately
 *   Collect a stream of digital data and show aggregated values
*    
*     
*    
*	To build this application:
*			 Set up a project for a 32-bit console mode application
*			 Add this file to the project
*			 Add PS2000A.lib to the project
*			 Add ps2000aApi.h and picoStatus.h to the project
*			 Build the project
*
*	History
*	22/09/2011		CPY		Modified for PS2000A 
*	12/10/2100		CPY		Mod to BlockCallback & formatting of data.txt files. Added and used g-trigStream & g_trigAtStream
*   19/10/11		CPY		Added support for PS2205 MSO Digital inputs
*
******************************************************************************************************************************************************/
#include <stdio.h>

/* Headers for Windows */
#ifdef _WIN32
#include "windows.h"
#include <conio.h>
#include "..\ps2000aApi.h"
#else
#include "linux_utils.h"
#include "libps2000a-1.0/ps2000aApi.h"
#endif


#define PREF4 __stdcall

int cycles = 0;

#define BUFFER_SIZE 	1024
#define DUAL_SCOPE		2



typedef enum
{
	ANALOGUE,
	DIGITAL,
	AGGREGATED,
	MIXED
}MODE;

typedef struct
{
	short DCcoupled;
	short range;
	short enabled;
}CHANNEL_SETTINGS;

typedef enum
{
	MODEL_NONE		 = 0,
	MODEL_PS2206	 = 2206,
	MODEL_PS2207	 = 2207,
	MODEL_PS2208	 = 2208,
	MODEL_PS2205MSO  = 2205
} MODEL_TYPE;

typedef struct tTriggerDirections
{
	PS2000A_THRESHOLD_DIRECTION channelA;
	PS2000A_THRESHOLD_DIRECTION channelB;
	PS2000A_THRESHOLD_DIRECTION channelC;
	PS2000A_THRESHOLD_DIRECTION channelD;
	PS2000A_THRESHOLD_DIRECTION ext;
	PS2000A_THRESHOLD_DIRECTION aux;
}TRIGGER_DIRECTIONS;

typedef struct tPwq
{
	PS2000A_PWQ_CONDITIONS * conditions;
	short nConditions;
	PS2000A_THRESHOLD_DIRECTION direction;
	unsigned long lower;
	unsigned long upper;
	PS2000A_PULSE_WIDTH_TYPE type;
}PWQ;

typedef struct
{
	short					handle;
	PS2000A_RANGE			firstRange;
	PS2000A_RANGE			lastRange;
	unsigned char			signalGenerator;
	unsigned char			ETS;
	short                   channelCount;
	short					maxValue;
	CHANNEL_SETTINGS		channelSettings [PS2000A_MAX_CHANNELS];
	short					digitalPorts;
}UNIT;

// Global Variables
unsigned long timebase = 8;
short       oversample = 1;
BOOL      scaleVoltages = TRUE;

unsigned short inputRanges [PS2000A_MAX_RANGES] = {	10,
	20,
	50,
	100,
	200,
	500,
	1000,
	2000,
	5000,
	10000,
	20000,
	50000};

BOOL     		g_ready = FALSE;
long long 		g_times [PS2000A_MAX_CHANNELS];
short     		g_timeUnit;
long      		g_sampleCount;
unsigned long	g_startIndex;
short			g_autoStopped;
short			g_trig = 0;
unsigned long	g_trigAt = 0;
short			g_overflow = 0;
char BlockFile[20] = "block.txt";
char StreamFile[20] = "stream.txt";


/****************************************************************************
* Callback
* used by PS2000A data streaimng collection calls, on receipt of data.
* used to set global flags etc checked by user routines
****************************************************************************/
void PREF4 CallBackStreaming(	short handle,
								long noOfSamples,
								unsigned long	startIndex,
								short overflow,
								unsigned long triggerAt,
								short triggered,
								short autoStop,
								void	*pParameter)
{
	// used for streaming
	g_sampleCount = noOfSamples;
	g_startIndex	= startIndex;
	g_autoStopped		= autoStop;

	// flag to say done reading data
	g_ready = TRUE;

	// flags to show if & where a trigger has occurred
	g_trig = triggered;
	g_trigAt = triggerAt;
}

/****************************************************************************
* Callback
* used by PS2000A data block collection calls, on receipt of data.
* used to set global flags etc checked by user routines
****************************************************************************/
void PREF4 CallBackBlock(	short handle,
							PICO_STATUS status,
							void * pParameter)
{
	if (status != PICO_CANCELLED)
		g_ready = TRUE;
}

/****************************************************************************
* CloseDevice 
****************************************************************************/
void CloseDevice(UNIT *unit)
{
	ps2000aCloseUnit(unit->handle);
}


/****************************************************************************
* SetDefaults - restore default settings
****************************************************************************/
void SetDefaults(UNIT * unit)
{
	PICO_STATUS status;
	int i;

	status = ps2000aSetEts(unit->handle, PS2000A_ETS_OFF, 0, 0, NULL); // Turn off ETS

	for (i = 0; i < unit->channelCount; i++) // reset channels to most recent settings
	{
		status = ps2000aSetChannel(unit->handle, (PS2000A_CHANNEL) PS2000A_CHANNEL_A + i,
			unit->channelSettings[PS2000A_CHANNEL_A + i].enabled,
			(PS2000A_COUPLING) unit->channelSettings[PS2000A_CHANNEL_A + i].DCcoupled,
			(PS2000A_RANGE) unit->channelSettings[PS2000A_CHANNEL_A + i].range, 0);
	}
}

/****************************************************************************
* SetDigitals - enable Digital Channels
****************************************************************************/
PICO_STATUS SetDigitals(UNIT *unit)
{
	PICO_STATUS status;

	short enabled = 1;
	short logicLevel;
	float logicVoltage = 1.5;
	short maxLogicVoltage = 5;
	
	short timebase = 1;
	short port;


	// Set logic threshold
	logicLevel =  (short) ((logicVoltage / maxLogicVoltage) * PS2000A_MAX_LOGIC_LEVEL);

	// Enable Digital ports
	for (port = PS2000A_DIGITAL_PORT0; port <= PS2000A_DIGITAL_PORT1; port++)
	{
		status = ps2000aSetDigitalPort(unit->handle, (PS2000A_DIGITAL_PORT)port, enabled, logicLevel);
	}
	return status;
}

/****************************************************************************
* DisableAnalogue - Disable Analogue Channels
****************************************************************************/
PICO_STATUS DisableAnalogue(UNIT *unit)
{
	PICO_STATUS status;
	short ch;
	
// Turn off digital ports
	for (ch = 0; ch < unit->channelCount; ch++)
	{
		if((status = ps2000aSetChannel(unit->handle, (PS2000A_CHANNEL) ch, 0, PS2000A_DC, PS2000A_50MV, 0)) != PICO_OK)
			printf("DisableDigital:ps2000aSetChannel(channel %d) ------ 0x%08lx \n", ch, status);
	}
	return status;
}

/****************************************************************************
* DisableDigital - Disable Digital Channels
****************************************************************************/
PICO_STATUS DisableDigital(UNIT *unit)
{
	PICO_STATUS status;
	short port;
	
// Turn off standard ports
	for (port = 0; port < unit->digitalPorts; port++)
	{
		if((status = ps2000aSetChannel(unit->handle, (PS2000A_CHANNEL)PS2000A_CHANNEL_A + port, 0, PS2000A_DC, PS2000A_50MV, 0)) != PICO_OK)
			printf("DisableDigital:ps2000aSetChannel(port %d) ------ 0x%08lx \n", port, status);
	}
	return status;
}


/****************************************************************************
* adc_to_mv
*
* Convert an 16-bit ADC count into millivolts
****************************************************************************/
int adc_to_mv(long raw, int ch, UNIT * unit)
{
	return (raw * inputRanges[ch]) / unit->maxValue;
}

/****************************************************************************
* mv_to_adc
*
* Convert a millivolt value into a 16-bit ADC count
*
*  (useful for setting trigger thresholds)
****************************************************************************/
short mv_to_adc(short mv, short ch, UNIT * unit)
{
	return (mv * unit->maxValue) / inputRanges[ch];
}

/****************************************************************************
* ClearDataBuffers
*
* stops GetData writing values to memory that has been released
****************************************************************************/
PICO_STATUS ClearDataBuffers(UNIT * unit)
{
	int i;
	PICO_STATUS status;

	for (i = 0; i < unit->channelCount; i++) 
	{
		if((status = ps2000aSetDataBuffers(unit->handle, (short)i, NULL, NULL, 0, 0, PS2000A_RATIO_MODE_NONE)) != PICO_OK)
			printf("ClearDataBuffers:ps2000aSetDataBuffers(channel %d) ------ 0x%08lx \n", i, status);
	}
	
	
	for (i= 0; i < unit->digitalPorts; i++) 
	{
		if((status = ps2000aSetDataBuffer(unit->handle, (PS2000A_CHANNEL) (i + PS2000A_DIGITAL_PORT0), NULL, 0, 0, PS2000A_RATIO_MODE_NONE))!= PICO_OK)
			printf("ClearDataBuffers:ps2000aSetDataBuffer(port 0x%X) ------ 0x%08lx \n", i + PS2000A_DIGITAL_PORT0, status);
	}
	
	return status;
}

/****************************************************************************
* BlockDataHandler
* - Used by all block data routines
* - acquires data (user sets trigger mode before calling), displays 10 items
*   and saves all to data.txt
* Input :
* - unit : the unit to use.
* - text : the text to display before the display of data slice
* - offset : the offset into the data buffer to start the display's slice.
****************************************************************************/
void BlockDataHandler(UNIT * unit, char * text, int offset, MODE mode)
{
	int i, j;
	long timeInterval;
	long sampleCount= BUFFER_SIZE;
	FILE * fp = NULL;
	long maxSamples;
	short * buffers[PS2000A_MAX_CHANNEL_BUFFERS] ;
	short * digiBuffer[PS2000A_MAX_DIGITAL_PORTS];
	long timeIndisposed;
	unsigned short digiValue;
	PICO_STATUS status;

	
	if (mode == ANALOGUE || mode == MIXED)		// Analogue or  (MSO Only) MIXED 
	{
		for (i = 0; i < unit->channelCount; i++) 
		{
			buffers[i * 2] = (short*)malloc(sampleCount * sizeof(short));
			buffers[i * 2 + 1] = (short*)malloc(sampleCount * sizeof(short));
			status = ps2000aSetDataBuffers(unit->handle, (short)i, buffers[i * 2], buffers[i * 2 + 1], sampleCount, 0, PS2000A_RATIO_MODE_NONE);
			printf("BlockDataHandler:ps2000aSetDataBuffers(channel %d) ------ 0x%08lx \n", i, status);
		}
	}

	
	if (mode == DIGITAL || mode == MIXED)		// (MSO Only) Digital or MIXED
	{
		for (i= 0; i < unit->digitalPorts; i++) 
		{
			digiBuffer[i] = (short*)malloc(sampleCount* sizeof(short));
			status = ps2000aSetDataBuffer(unit->handle, (PS2000A_CHANNEL) (i + PS2000A_DIGITAL_PORT0), digiBuffer[i], sampleCount, 0, PS2000A_RATIO_MODE_NONE);
			printf("BlockDataHandler:ps2000aSetDataBuffer(port 0x%X) ------ 0x%08lx \n", i + PS2000A_DIGITAL_PORT0, status);
		}
	}

	

	/*  find the maximum number of samples, the time interval (in timeUnits),
	*		 the most suitable time units, and the maximum oversample at the current timebase*/
	while (ps2000aGetTimebase(unit->handle, timebase, sampleCount, &timeInterval, oversample, &maxSamples, 0))
	{
		timebase++;
	}

	printf("\nTimebase: %lu  SampleInterval: %ldnS  oversample: %hd\n", timebase, timeInterval, oversample);

	/* Start it collecting, then wait for completion*/
	g_ready = FALSE;
	if ((status = ps2000aRunBlock(unit->handle, 0, sampleCount, timebase, oversample,	&timeIndisposed, 0, CallBackBlock, NULL)) != PICO_OK)
		printf("BlockDataHandler:ps2000aRunBlock ------ 0x%08lx \n", status);
	
	printf("Waiting for trigger...Press a key to abort\n");

	while (!g_ready && !_kbhit())
	{
		Sleep(0);
	}


	if(g_ready) 
	{
		if((status = ps2000aGetValues(unit->handle, 0, (unsigned long*) &sampleCount, 1, PS2000A_RATIO_MODE_NONE, 0, NULL)) != PICO_OK)
			printf("BlockDataHandler:ps2000aGetValues ------ 0x%08lx \n", status);

		/* Print out the first 10 readings, converting the readings to mV if required */
		printf("%s\n",text);

		if (mode == ANALOGUE || mode == MIXED)		// if we're doing analogue or MIXED
		{
			printf("Channels are in (%s)\n", ( scaleVoltages ) ? ("mV") : ("ADC Counts"));

			for (j = 0; j < unit->channelCount; j++) 
				printf("Channel%c:\t", 'A' + j);
		}
		
		if (mode == DIGITAL || mode == MIXED)	// if we're doing digital or MIXED
			printf("Digital");
		
		printf("\n");
	
		for (i = offset; i < offset+10; i++) 
		{
			if (mode == ANALOGUE || mode == MIXED)	// if we're doing analogue or MIXED
			{
				for (j = 0; j < unit->channelCount; j++) 
				{
					if (unit->channelSettings[j].enabled) 
					{
						printf("  %6d        ", scaleVoltages ? 
							adc_to_mv(buffers[j * 2][i], unit->channelSettings[PS2000A_CHANNEL_A + j].range, unit)	// If scaleVoltages, print mV value
							: buffers[j * 2][i]);																	// else print ADC Count
					}
				}
			}
			if (mode == DIGITAL || mode == MIXED)	// if we're doing digital or MIXED
			{
				digiValue = 0x00ff & digiBuffer[1][i];
				digiValue <<= 8;
				digiValue |= digiBuffer[0][i];
				printf("0x%04X", digiValue);
			}
			printf("\n");
		}

		if (mode == ANALOGUE || mode == MIXED)		// if we're doing analogue or MIXED
		{
			sampleCount = min(sampleCount, BUFFER_SIZE);

			fopen_s(&fp, BlockFile, "w");
			if (fp != NULL)
			{
				fprintf(fp, "Block Data log\n\n");
				fprintf(fp,"Results shown for each of the %d Channels are......\n",unit->channelCount);
				fprintf(fp,"Maximum Aggregated value ADC Count & mV, Minimum Aggregated value ADC Count & mV\n\n");

				fprintf(fp, "Time  ");
				for (i = 0; i < unit->channelCount; i++) 
					fprintf(fp," Ch   Max ADC  Max mV   Min ADC  Min mV  ");
				fprintf(fp, "\n");

				for (i = 0; i < sampleCount; i++) 
				{
					fprintf(fp, "%5lld ", g_times[0] + (long long)(i * timeInterval));
					for (j = 0; j < unit->channelCount; j++) 
					{
						if (unit->channelSettings[j].enabled) 
						{
							fprintf(	fp,
								"Ch%C  %5d = %+5dmV, %5d = %+5dmV   ",
								(char)('A' + j),
								buffers[j * 2][i],
								adc_to_mv(buffers[j * 2][i], unit->channelSettings[PS2000A_CHANNEL_A + j].range, unit),
								buffers[j * 2 + 1][i],
								adc_to_mv(buffers[j * 2 + 1][i], unit->channelSettings[PS2000A_CHANNEL_A + j].range, unit));
						}
					}
					fprintf(fp, "\n");
				}
			}
			else
			{
				printf(	"Cannot open the file block.txt for writing.\n"
				"Please ensure that you have permission to access.\n");
			}
		}
	} 
	else 
	{
		printf("data collection aborted\n");
		_getch();
	}

	if((status = ps2000aStop(unit->handle)) != PICO_OK)
		printf("BlockDataHandler:ps2000aStop ------ 0x%08lx \n", status);

	if (fp != NULL)
		fclose(fp);

	if (mode == ANALOGUE || mode == MIXED)		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->channelCount * 2; i++) 
		{
			free(buffers[i]);
		}
	}

	if (mode == DIGITAL || mode == MIXED)		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->digitalPorts; i++) 
		{
			free(digiBuffer[i]);
		}
	}
		ClearDataBuffers(unit);
}

/****************************************************************************
* Stream Data Handler
* - Used by the two stream data examples - untriggered and triggered
* Inputs:
* - unit - the unit to sample on
* - preTrigger - the number of samples in the pre-trigger phase 
*					(0 if no trigger has been set)
***************************************************************************/
void StreamDataHandler(UNIT * unit, unsigned long preTrigger, MODE mode)
{
	long i, j;
	unsigned long sampleCount= BUFFER_SIZE * 10; /*  *10 is to make sure buffer large enough */
	FILE * fp = NULL;
	short * buffers[PS2000A_MAX_CHANNEL_BUFFERS];
	short * digiBuffers[PS2000A_MAX_DIGITAL_PORTS];
	PICO_STATUS status;
	unsigned long sampleInterval;
	int index = 0;
	int totalSamples;
	unsigned long postTrigger;
	short autostop;
	unsigned long downsampleRatio;
	unsigned long triggeredAt = 0;
	int bit;
	unsigned short portValue, portValueOR, portValueAND;
	PS2000A_TIME_UNITS timeUnits;
	PS2000A_RATIO_MODE ratioMode;


	if (mode == ANALOGUE)		// Analogue 
	{
		for (i = 0; i < unit->channelCount; i++) 
		{
			buffers[i * 2] = (short*)malloc(sampleCount * sizeof(short));
			buffers[i * 2 + 1] = (short*)malloc(sampleCount * sizeof(short));
			status = ps2000aSetDataBuffers(unit->handle, (short)i, buffers[i * 2], buffers[i * 2 + 1], sampleCount, 0, PS2000A_RATIO_MODE_AGGREGATE);

			printf("StreamDataHandler:ps2000aSetDataBuffers(channel %ld) ------ 0x%08lx \n", i, status);
		}
		
		downsampleRatio = 1000;
		timeUnits = PS2000A_US;
		sampleInterval = 1;
		ratioMode = PS2000A_RATIO_MODE_AGGREGATE;
		postTrigger = 1000000;
		autostop = TRUE;
	}

	
	if (mode == AGGREGATED)		// (MSO Only) AGGREGATED
	{
		for (i= 0; i < unit->digitalPorts; i++) 
		{
		
			digiBuffers[i * 2] = (short*)malloc(sampleCount * sizeof(short));
			digiBuffers[i * 2 + 1] = (short*)malloc(sampleCount * sizeof(short));
			status = ps2000aSetDataBuffers(unit->handle, (PS2000A_CHANNEL) (i + PS2000A_DIGITAL_PORT0), digiBuffers[i * 2], digiBuffers[i * 2 + 1], sampleCount, 0, PS2000A_RATIO_MODE_AGGREGATE);

			printf("StreamDataHandler:ps2000aSetDataBuffer(channel %ld) ------ 0x%08lx \n", i, status);
		}
		
		downsampleRatio = 10;
		timeUnits = PS2000A_MS;
		sampleInterval = 10;
		ratioMode = PS2000A_RATIO_MODE_AGGREGATE;
		postTrigger = 10;
		autostop = FALSE;
	}

	if (mode == DIGITAL)		// (MSO Only) Digital 
	{
		for (i= 0; i < unit->digitalPorts; i++) 
		{
			digiBuffers[i] = (short*)malloc(sampleCount* sizeof(short));
			status = ps2000aSetDataBuffer(unit->handle, (PS2000A_CHANNEL) (i + PS2000A_DIGITAL_PORT0), digiBuffers[i], sampleCount, 0, PS2000A_RATIO_MODE_NONE);

			printf("StreamDataHandler:ps2000aSetDataBuffer(channel %ld) ------ 0x%08lx \n", i, status);
		}
		
		downsampleRatio = 1;
		timeUnits = PS2000A_MS;
		sampleInterval = 10;
		ratioMode = PS2000A_RATIO_MODE_NONE;
		postTrigger = 10;
		autostop = FALSE;
	}

	

	if (autostop)
	{
		printf("\nStreaming Data for %lu samples", postTrigger / downsampleRatio);
		if (preTrigger)							// we pass 0 for preTrigger if we're not setting up a trigger
			printf(" after the trigger occurs\nNote: %lu Pre Trigger samples before Trigger arms\n\n",preTrigger / downsampleRatio);
		else
			printf("\n\n");
	}
	else
		printf("\nStreaming Data continually\n\n");

	g_autoStopped = FALSE;

	status = ps2000aRunStreaming(unit->handle, 
		&sampleInterval, 
		timeUnits,
		preTrigger, 
		postTrigger - preTrigger, 
		autostop,
		downsampleRatio,
		ratioMode,
		sampleCount);

	printf("Streaming data...Press a key to stop\n");

	
	if (mode == ANALOGUE)
	{
		fopen_s(&fp, StreamFile, "w");

		if (fp != NULL)
		{
			fprintf(fp,"For each of the %d Channels, results shown are....\n",unit->channelCount);
			fprintf(fp,"Maximum Aggregated value ADC Count & mV, Minimum Aggregated value ADC Count & mV\n\n");

			for (i = 0; i < unit->channelCount; i++) 
				fprintf(fp,"   Max ADC   Max mV   Min ADC   Min mV");
			fprintf(fp, "\n");
		}
	}

	totalSamples = 0;
	while (!_kbhit() && !g_autoStopped && !g_overflow)
	{
		/* Poll until data is received. Until then, GetStreamingLatestValues wont call the callback */
		Sleep(100);
		g_ready = FALSE;

		status = ps2000aGetStreamingLatestValues(unit->handle, CallBackStreaming, NULL);
		index ++;

		if (g_ready && g_sampleCount > 0) /* can be ready and have no data, if autoStop has fired */
		{
			if (g_trig)
				triggeredAt = totalSamples += g_trigAt;		// calculate where the trigger occurred in the total samples collected

			totalSamples += g_sampleCount;
			printf("\nCollected %3li samples, index = %5lu, Total: %6d samples ", g_sampleCount, g_startIndex, totalSamples);
			
			if (g_trig)
				printf("Trig. at index %lu", triggeredAt);	// show where trigger occurred
			
			
			for (i = g_startIndex; i < (long)(g_startIndex + g_sampleCount); i++) 
			{
				if (mode == ANALOGUE)
				{
					if(fp != NULL)
					{
						for (j = 0; j < unit->channelCount; j++) 
						{
							if (unit->channelSettings[j].enabled) 
							{
								fprintf(	fp,
									"Ch%C  %5d = %+5dmV, %5d = %+5dmV   ",
									(char)('A' + j),
									buffers[j * 2][i],
									adc_to_mv(buffers[j * 2][i], unit->channelSettings[PS2000A_CHANNEL_A + j].range, unit),
									buffers[j * 2 + 1][i],
									adc_to_mv(buffers[j * 2 + 1][i], unit->channelSettings[PS2000A_CHANNEL_A + j].range, unit));
							}
						}
						fprintf(fp, "\n");
					}
					else
						printf("Cannot open the file stream.txt for writing.\n");

				}

				if (mode == DIGITAL)
				{
					portValue = 0x00ff & digiBuffers[1][i];
					portValue <<= 8;
					portValue |= 0x00ff & digiBuffers[0][i];

					printf("\nIndex=%04lu: Value = 0x%04X  =  ",i, portValue);

					for (bit = 0; bit < 16; bit++)
					{
						printf( (0x8000 >> bit) & portValue? "1 " : "0 ");
					}
				}

				if (mode == AGGREGATED)
				{
					portValueOR = 0x00ff & digiBuffers[2][i];
					portValueOR <<= 8;
					portValueOR |= 0x00ff & digiBuffers[0][i];

					portValueAND = 0x00ff & digiBuffers[3][i];
					portValueAND <<= 8;
					portValueAND |= 0x00ff & digiBuffers[1][i];

					printf("\nIndex=%04lu: Bitwise  OR of last %ld readings = 0x%04X ",i,  downsampleRatio, portValueOR);
					printf("\nIndex=%04lu: Bitwise AND of last %ld readings = 0x%04X ",i,  downsampleRatio, portValueAND);
				}
			}
		}
	}

	ps2000aStop(unit->handle);

	if (!g_autoStopped) 
	{
		printf("\ndata collection aborted\n");
		_getch();
	}

	if (g_overflow)
	{
		printf("\nStreaming overflow. Not able to keep up with streaming data rate\n");
	}

	if(fp != NULL) 
		fclose(fp);	


	if (mode == ANALOGUE)		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->channelCount * 2; i++) 
		{
			free(buffers[i]);
		}
	}

	if (mode == DIGITAL) 		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->digitalPorts; i++) 
		{
			free(digiBuffers[i]);
		}

	}

	if (mode == AGGREGATED) 		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->digitalPorts * 2; i++) 
		{
			free(digiBuffers[i]);
		}
	}

	ClearDataBuffers(unit);
}


/****************************************************************************
* SetTrigger
* 
* Parameters
* - *unit               - pointer to the UNIT structure
* - *channelProperties  - pointer to the PS2000A_TRIGGER_CHANNEL_PROPERTIES structure
* - nChannelProperties  - the number of PS2000A_TRIGGER_CHANNEL_PROPERTIES elements in channelProperties
* - *triggerConditions  - pointer to the PS2000A_TRIGGER_CONDITIONS structure
* - nTriggerConditions  - the number of PS2000A_TRIGGER_CONDITIONS elements in triggerConditions
* - *directions         - pointer to the TRIGGER_DIRECTIONS structure
* - *pwq                - pointer to the pwq (Pulse Width Qualifier) structure
* - delay               - Delay time between trigger & first sample
* - auxOutputEnable     - Not used
* - autoTriggerMs       - timeout period if no trigger occurrs
* - *digitalDirections  - pointer to the PS2000A_DIGITAL_CHANNEL_DIRECTIONS structure
* - nDigitalDirections  - the number of PS2000A_DIGITAL_CHANNEL_DIRECTIONS elements in digitalDirections
*
* Returns			    - PICO_STATUS - to show success or if an error occurred
*					
***************************************************************************/
PICO_STATUS SetTrigger(	UNIT * unit,
						PS2000A_TRIGGER_CHANNEL_PROPERTIES * channelProperties,
						short nChannelProperties,
						PS2000A_TRIGGER_CONDITIONS * triggerConditions,
						short nTriggerConditions,
						TRIGGER_DIRECTIONS * directions,
						PWQ * pwq,
						unsigned long delay,
						short auxOutputEnabled,
						long autoTriggerMs,
						PS2000A_DIGITAL_CHANNEL_DIRECTIONS * digitalDirections,
						short nDigitalDirections)
{
	PICO_STATUS status;

	if ((status = ps2000aSetTriggerChannelProperties(unit->handle,
		channelProperties,
		nChannelProperties,
		auxOutputEnabled,
		autoTriggerMs)) != PICO_OK) 
	{
			printf("SetTrigger:ps2000aSetTriggerChannelProperties ------ Ox%8lx \n", status);
			return status;
	}

	if ((status = ps2000aSetTriggerChannelConditions(unit->handle,	triggerConditions, nTriggerConditions)) != PICO_OK) 
	{
			printf("SetTrigger:ps2000aSetTriggerChannelConditions ------ 0x%8lx \n", status);
			return status;
	}

	if ((status = ps2000aSetTriggerChannelDirections(unit->handle,
		directions->channelA,
		directions->channelB,
		directions->channelC,
		directions->channelD,
		directions->ext,
		directions->aux)) != PICO_OK) 
	{
			printf("SetTrigger:ps2000aSetTriggerChannelDirections ------ 0x%08lx \n", status);
			return status;
	}

	if ((status = ps2000aSetTriggerDelay(unit->handle, delay)) != PICO_OK) 
	{
		printf("SetTrigger:ps2000aSetTriggerDelay ------ 0x%08lx \n", status);
		return status;
	}

	if((status = ps2000aSetPulseWidthQualifier(unit->handle,
		pwq->conditions,
		pwq->nConditions, 
		pwq->direction,
		pwq->lower, 
		pwq->upper, 
		pwq->type)) != PICO_OK)
	{
		printf("SetTrigger:ps2000aSetPulseWidthQualifier ------ 0x%08lx \n", status);
		return status;
	}

	if (unit->digitalPorts)					// ps2000aSetTriggerDigitalPortProperties function only applies to MSO	
	{
		if((status = ps2000aSetTriggerDigitalPortProperties(unit->handle,
														digitalDirections, 
														nDigitalDirections)) != PICO_OK) 
		{
			printf("SetTrigger:ps2000aSetTriggerDigitalPortProperties ------ 0x%08lx \n", status);
			return status;
		}
	}
	return status;
}

/****************************************************************************
* CollectBlockImmediate
*  this function demonstrates how to collect a single block of data
*  from the unit (start collecting immediately)
****************************************************************************/
void CollectBlockImmediate(UNIT * unit)
{
	PWQ pulseWidth;
	TRIGGER_DIRECTIONS directions;

	memset(&directions, 0, sizeof(TRIGGER_DIRECTIONS));
	memset(&pulseWidth, 0, sizeof(PWQ));

	printf("Collect block immediate\n");
	printf("Data is written to disk file (%s)\n", BlockFile);
	printf("Press a key to start...\n");
	_getch();

	SetDefaults(unit);

	/* Trigger disabled	*/
	SetTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	BlockDataHandler(unit, "First 10 readings\n", 0, ANALOGUE);
}

/****************************************************************************
* CollectBlockEts
*  this function demonstrates how to collect a block of
*  data using equivalent time sampling (ETS).
****************************************************************************/
void CollectBlockEts(UNIT * unit)
{
	PICO_STATUS status;
	long ets_sampletime;
	short	triggerVoltage = mv_to_adc(1000,	unit->channelSettings[PS2000A_CHANNEL_A].range, unit);
	unsigned long delay = 0;
	struct tPwq pulseWidth;
	struct tTriggerDirections directions;

	PS2000A_TRIGGER_CHANNEL_PROPERTIES sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS2000A_CHANNEL_A,
		PS2000A_LEVEL};

	PS2000A_TRIGGER_CONDITIONS conditions = {	PS2000A_CONDITION_TRUE,
												PS2000A_CONDITION_DONT_CARE,
												PS2000A_CONDITION_DONT_CARE,
												PS2000A_CONDITION_DONT_CARE,
												PS2000A_CONDITION_DONT_CARE,
												PS2000A_CONDITION_DONT_CARE,
												PS2000A_CONDITION_DONT_CARE,
												PS2000A_CONDITION_DONT_CARE};



	memset(&pulseWidth, 0, sizeof(struct tPwq));
	memset(&directions, 0, sizeof(struct tTriggerDirections));
	directions.channelA = PS2000A_RISING;

	printf("Collect ETS block...\n");
	printf("Collects when value rises past %d", scaleVoltages? 
		adc_to_mv(sourceDetails.thresholdUpper,	unit->channelSettings[PS2000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	printf(scaleVoltages? "mV\n" : "ADC Counts\n");
	printf("Press a key to start...\n");
	_getch();

	SetDefaults(unit);

	//Trigger enabled
	//Rising edge
	//Threshold = 1000mV
	status = SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, delay, 0, 0, 0, 0);

	if ((status = ps2000aSetEts(unit->handle, PS2000A_ETS_FAST, 20, 4, &ets_sampletime)) != PICO_OK)
		printf("CollectBlockEts:ps2000aSetEts ------ 0x%08lx \n", status);
	
	printf("ETS Sample Time is: %ld\n", ets_sampletime);

	BlockDataHandler(unit, "Ten readings after trigger\n", BUFFER_SIZE / 10 - 5, ANALOGUE); // 10% of data is pre-trigger
}

/****************************************************************************
* CollectBlockTriggered
*  this function demonstrates how to collect a single block of data from the
*  unit, when a trigger event occurs.
****************************************************************************/
void CollectBlockTriggered(UNIT * unit)
{
	short	triggerVoltage = mv_to_adc(1000, unit->channelSettings[PS2000A_CHANNEL_A].range, unit);

	PS2000A_TRIGGER_CHANNEL_PROPERTIES sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS2000A_CHANNEL_A,
		PS2000A_LEVEL};

	PS2000A_TRIGGER_CONDITIONS conditions = {	PS2000A_CONDITION_TRUE,				// Channel A
												PS2000A_CONDITION_DONT_CARE,		// Channel B 
												PS2000A_CONDITION_DONT_CARE,		// Channel C
												PS2000A_CONDITION_DONT_CARE,		// Channel D
												PS2000A_CONDITION_DONT_CARE,		// external
												PS2000A_CONDITION_DONT_CARE,		// aux
												PS2000A_CONDITION_DONT_CARE,		// PWQ
												PS2000A_CONDITION_DONT_CARE};		// digital

	

	TRIGGER_DIRECTIONS directions = {	PS2000A_RISING,			// Channel A
										PS2000A_NONE,			// Channel B
										PS2000A_NONE,			// Channel C
										PS2000A_NONE,			// Channel D
										PS2000A_NONE,			// ext
										PS2000A_NONE };			// aux

	PWQ pulseWidth;
	memset(&pulseWidth, 0, sizeof(PWQ));

	printf("Collect block triggered\n");
	printf("Data is written to disk file (%s)\n", BlockFile);
	printf("Collects when value rises past %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS2000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	printf(scaleVoltages?"mV\n" : "ADC Counts\n");

	printf("Press a key to start...\n");
	_getch();

	SetDefaults(unit);

	/* Trigger enabled
	* Rising edge
	* Threshold = 1000mV */
	SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	BlockDataHandler(unit, "Ten readings after trigger\n", 0, ANALOGUE);
}

/****************************************************************************
* CollectRapidBlock
*  this function demonstrates how to collect a set of captures using 
*  rapid block mode.
****************************************************************************/
void CollectRapidBlock(UNIT * unit)
{
	unsigned short nCaptures;
	long nMaxSamples;
	unsigned long nSamples = 1000;
	long timeIndisposed;
	short capture, channel;
	short ***rapidBuffers;
	short *overflow;
	PICO_STATUS status;
	short i;
	unsigned long nCompletedCaptures;

	short	triggerVoltage = mv_to_adc(100, unit->channelSettings[PS2000A_CHANNEL_A].range, unit);

	struct tPS2000ATriggerChannelProperties sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS2000A_CHANNEL_A,
		PS2000A_LEVEL};

	struct tPS2000ATriggerConditions conditions = {	PS2000A_CONDITION_TRUE,				// Channel A
													PS2000A_CONDITION_DONT_CARE,		// Channel B
													PS2000A_CONDITION_DONT_CARE,		// Channel C
													PS2000A_CONDITION_DONT_CARE,		// Channel D
													PS2000A_CONDITION_DONT_CARE,		// external
													PS2000A_CONDITION_DONT_CARE,		// aux
													PS2000A_CONDITION_DONT_CARE,		// PWQ
													PS2000A_CONDITION_DONT_CARE};		// digital

	struct tPwq pulseWidth;

	struct tTriggerDirections directions = {	PS2000A_RISING,			// Channel A
												PS2000A_NONE,			// Channel B
												PS2000A_NONE,			// Channel C
												PS2000A_NONE,			// Channel D
												PS2000A_NONE,			// ext
												PS2000A_NONE };			// aux

	memset(&pulseWidth, 0, sizeof(struct tPwq));

	printf("Collect rapid block triggered...\n");
	printf("Collects when value rises past %d",	scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS2000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	printf(scaleVoltages?"mV\n" : "ADC Counts\n");
	printf("Press any key to abort\n");

	SetDefaults(unit);

	// Trigger enabled
	SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	//Set the number of captures
	nCaptures = 10;

	//Segment the memory
	status = ps2000aMemorySegments(unit->handle, nCaptures, &nMaxSamples);

	//Set the number of captures
	status = ps2000aSetNoOfCaptures(unit->handle, nCaptures);

	//Run
	timebase = 160;		//1 MS/s
	status = ps2000aRunBlock(unit->handle, 0, nSamples, timebase, 1, &timeIndisposed, 0, CallBackBlock, NULL);

	//Wait until data ready
	g_ready = 0;
	while(!g_ready && !_kbhit())
	{
		Sleep(0);
	}

	if(!g_ready)
	{
		_getch();
		status = ps2000aStop(unit->handle);
		status = ps2000aGetNoOfCaptures(unit->handle, &nCompletedCaptures);
		printf("Rapid capture aborted. %lu complete blocks were captured\n", nCompletedCaptures);
		printf("\nPress any key...\n\n");
		_getch();

		if(nCompletedCaptures == 0)
			return;
		
		//Only display the blocks that were captured
		nCaptures = (unsigned short)nCompletedCaptures;
	}

	//Allocate memory
	rapidBuffers = calloc(unit->channelCount, sizeof(short*));
	overflow = calloc(unit->channelCount * nCaptures, sizeof(short));

	for (channel = 0; channel < unit->channelCount; channel++) 
	{
		rapidBuffers[channel] = calloc(nCaptures, sizeof(short*));
	}

	for (channel = 0; channel < unit->channelCount; channel++) 
	{	
		if(unit->channelSettings[channel].enabled)
		{
			for (capture = 0; capture < nCaptures; capture++) 
			{
				rapidBuffers[channel][capture] = calloc(nSamples, sizeof(short));
			}
		}
	}

	for (channel = 0; channel < unit->channelCount; channel++) 
	{
		if(unit->channelSettings[channel].enabled)
		{
			for (capture = 0; capture < nCaptures; capture++) 
			{
				status = ps2000aSetDataBuffer(unit->handle, channel, rapidBuffers[channel][capture], nSamples, capture, PS2000A_RATIO_MODE_NONE);
			}
		}
	}

	//Get data
	status = ps2000aGetValuesBulk(unit->handle, &nSamples, 0, nCaptures - 1, 1, PS2000A_RATIO_MODE_NONE, overflow);

	//Stop
	status = ps2000aStop(unit->handle);

	//print first 10 samples from each capture
	for (capture = 0; capture < nCaptures; capture++)
	{
		printf("\nCapture %d\n", capture + 1);
		for (channel = 0; channel < unit->channelCount; channel++) 
		{
			printf("Channel %c\t", 'A' + channel);
		}
		printf("\n");

		for(i = 0; i < 10; i++)
		{
			for (channel = 0; channel < unit->channelCount; channel++) 
			{
				if(unit->channelSettings[channel].enabled)
				{
					printf("%d\t\t", rapidBuffers[channel][capture][i]);
				}
			}
			printf("\n");
		}
	}

	//Free memory
	free(overflow);

	for (channel = 0; channel < unit->channelCount; channel++) 
	{	
		if(unit->channelSettings[channel].enabled)
		{
			for (capture = 0; capture < nCaptures; capture++) 
			{
				free(rapidBuffers[channel][capture]);
			}
		}
	}

	for (channel = 0; channel < unit->channelCount; channel++) 
	{
		free(rapidBuffers[channel]);
	}
	free(rapidBuffers);
}

/****************************************************************************
* Initialise unit' structure with Variant specific defaults
****************************************************************************/
void get_info(UNIT * unit)
{
	char description [6][25]= { "Driver Version",
		"USB Version",
		"Hardware Version",
		"Variant Info",
		"Serial",
		"Error Code" };
	short i, r = 0;
	char line [80];
	int variant;
	PICO_STATUS status = PICO_OK;

	if (unit->handle) 
	{
		for (i = 0; i < 5; i++) 
		{
			status = ps2000aGetUnitInfo(unit->handle, line, sizeof (line), &r, i);
			if (i == 3) 
			{
				variant = atoi(line);
			}
			printf("%s: %s\n", description[i], line);
		}
		switch (variant)
		{

		case MODEL_PS2205MSO:
			unit->signalGenerator	= TRUE;
			unit->ETS				= FALSE;
			unit->firstRange		= PS2000A_50MV;
			unit->lastRange			= PS2000A_20V;
			unit->channelCount		= DUAL_SCOPE;
			unit->digitalPorts      = 2;
			break;

		case MODEL_PS2206:
			unit->signalGenerator	= TRUE;
			unit->ETS				= FALSE;
			unit->firstRange		= PS2000A_50MV;
			unit->lastRange			= PS2000A_20V;
			unit->channelCount		= DUAL_SCOPE;
			unit->digitalPorts      = 0;
			break;

		case MODEL_PS2207:
			unit->signalGenerator	= TRUE;
			unit->ETS				= FALSE;
			unit->firstRange		= PS2000A_50MV;
			unit->lastRange			= PS2000A_20V;
			unit->channelCount		= DUAL_SCOPE;
			unit->digitalPorts      = 0;
			break;

		case MODEL_PS2208:
			unit->signalGenerator	= TRUE;
			unit->ETS				= FALSE;
			unit->firstRange		= PS2000A_50MV;
			unit->lastRange			= PS2000A_20V;
			unit->channelCount		= DUAL_SCOPE;
			unit->digitalPorts      = 0;
			break;

		default:
			break;
		}
	}
}

/****************************************************************************
* Select input voltage ranges for channels
****************************************************************************/
void SetVoltages(UNIT * unit)
{
	int i, ch;
	int count = 0;

	/* See what ranges are available... */
	for (i = unit->firstRange; i <= unit->lastRange; i++) 
	{
		printf("%d -> %d mV\n", i, inputRanges[i]);
	}

	do
	{
		/* Ask the user to select a range */
		printf("Specify voltage range (%d..%d)\n", unit->firstRange, unit->lastRange);
		printf("99 - switches channel off\n");
		for (ch = 0; ch < unit->channelCount; ch++) 
		{
			printf("\n");
			do 
			{
				printf("Channel %c: ", 'A' + ch);
				fflush(stdin);
				scanf_s("%hd", &unit->channelSettings[ch].range);
			} while (unit->channelSettings[ch].range != 99 && (unit->channelSettings[ch].range < unit->firstRange || unit->channelSettings[ch].range > unit->lastRange));

			if (unit->channelSettings[ch].range != 99) 
			{
				printf(" - %d mV\n", inputRanges[unit->channelSettings[ch].range]);
				unit->channelSettings[ch].enabled = TRUE;
				count++;
			} 
			else 
			{
				printf("Channel Switched off\n");
				unit->channelSettings[ch].enabled = FALSE;
			}
		}
		printf(count == 0? "\n** At least 1 channel must be enabled **\n\n":"");
	}
	while(count == 0);	// must have at least one channel enabled

	SetDefaults(unit);	// Put these changes into effect
}

/****************************************************************************
*
* Select timebase, set oversample to on and time units as nano seconds
*
****************************************************************************/
void SetTimebase(UNIT unit)
{
	long timeInterval;
	long maxSamples;

	printf("Specify desired timebase: ");
	fflush(stdin);
	scanf_s("%lud", &timebase);

	while (ps2000aGetTimebase(unit.handle, timebase, BUFFER_SIZE, &timeInterval, 1, &maxSamples, 0))
	{
		timebase++;  // Increase timebase if the one specified can't be used. 
	}

	printf("Timebase %lu used = %ld ns\n", timebase, timeInterval);
	oversample = TRUE;
}

/****************************************************************************
* Sets the signal generator
* - allows user to set frequency and waveform
* - allows for custom waveform (values 0..4192) of up to 8192 samples long
***************************************************************************/
void SetSignalGenerator(UNIT unit)
{
	PICO_STATUS status;
	short waveform;
	long frequency;
	char fileName [128];
	FILE * fp = NULL;
	short arbitraryWaveform [8192];
	short waveformSize = 0;

	memset(&arbitraryWaveform, 0, 8192);

	printf("Enter frequency in Hz: "); // Ask user to enter signal frequency;
	do 
	{
		scanf_s("%lu", &frequency);
	} while (frequency <= 0 || frequency > 10000000);

	if(frequency > 0) // Ask user to enter type of signal
	{
		printf("Signal generator On");
		printf("Enter type of waveform (0..9 or 99)\n");
		printf("0:\tSINE\n");
		printf("1:\tSQUARE\n");
		printf("2:\tTRIANGLE\n");
		printf("99:\tUSER WAVEFORM\n");
		printf("  \t(see manual for more)\n");

		do 
		{
			scanf_s("%hd", &waveform);
		} while (waveform != 99 && (waveform < 0 || waveform >= PS2000A_MAX_WAVE_TYPES));

		if (waveform == 99) // custom waveform selected - user needs to select file
		{
			waveformSize = 0;

			printf("Select a waveform file to load: ");
			scanf_s("%s", fileName, 128);
			if (fopen_s(&fp, fileName, "r") == 0) 
			{ // Having opened file, read in data - one number per line (at most 8192 lines), with values in (0..4095)
				while (EOF != fscanf_s(fp, "%hi", (arbitraryWaveform + waveformSize))&& waveformSize++ < 8192);
				fclose(fp);
				printf("File successfully loaded\n");
			} 
			else 
			{
				printf("Invalid filename\n");
				return;
			}
		}
	} 
	else 
	{
		waveform = 0;
		printf("Signal generator Off");
	}

	if (waveformSize > 0) 
	{
		double delta = ((frequency * waveformSize) / 8192.0) * 4294967296.0 * 8e-9; // delta >= 10

		status = ps2000aSetSigGenArbitrary(	unit.handle, 
			0, 
			1000000, 
			(unsigned long)delta, 
			(unsigned long)delta, 
			0, 
			0, 
			arbitraryWaveform, 
			waveformSize, 
			0,
			0, 
			PS2000A_SINGLE, 
			0, 
			0, 
			PS2000A_SIGGEN_RISING,
			PS2000A_SIGGEN_NONE, 
			0);

		printf("Status of Arbitrary Gen: 0x%08lx \n", status);
	} 
	else 
	{
		status = ps2000aSetSigGenBuiltIn(unit.handle, 0, 1000000, waveform, (float)frequency, (float)frequency, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
}

/****************************************************************************
* CollectStreamingImmediate
*  this function demonstrates how to collect a stream of data
*  from the unit (start collecting immediately)
***************************************************************************/
void CollectStreamingImmediate(UNIT * unit)
{
	struct tPwq pulseWidth;
	struct tTriggerDirections directions;

	memset(&pulseWidth, 0, sizeof(struct tPwq));
	memset(&directions, 0, sizeof(struct tTriggerDirections));

	SetDefaults(unit);

	printf("Collect streaming...\n");
	printf("Data is written to disk file (%s)\n", StreamFile);
	printf("Press a key to start...\n");
	_getch();

	/* Trigger disabled	*/
	SetTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	StreamDataHandler(unit, 0, ANALOGUE);
}

/****************************************************************************
* CollectStreamingTriggered
*  this function demonstrates how to collect a stream of data
*  from the unit (start collecting on trigger)
***************************************************************************/
void CollectStreamingTriggered(UNIT * unit)
{
	short triggerVoltage = mv_to_adc(1000,	unit->channelSettings[PS2000A_CHANNEL_A].range, unit); // ChannelInfo stores ADC counts
	struct tPwq pulseWidth;
	
	struct tPS2000ATriggerChannelProperties sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS2000A_CHANNEL_A,
		PS2000A_LEVEL };

	struct tPS2000ATriggerConditions conditions = {	PS2000A_CONDITION_TRUE,				// Channel A
													PS2000A_CONDITION_DONT_CARE,		// Channel B
													PS2000A_CONDITION_DONT_CARE,		// Channel C
													PS2000A_CONDITION_DONT_CARE,		// Channel D
													PS2000A_CONDITION_DONT_CARE,		// Exteranl
													PS2000A_CONDITION_DONT_CARE,		// aux
													PS2000A_CONDITION_DONT_CARE,		// PWQ
													PS2000A_CONDITION_DONT_CARE };		// digital

	struct tTriggerDirections directions = {	PS2000A_RISING,			// Channel A
												PS2000A_NONE,			// Channel B
												PS2000A_NONE,			// Channel C
												PS2000A_NONE,			// Channel D
												PS2000A_NONE,			// External
												PS2000A_NONE };			// Aux

	memset(&pulseWidth, 0, sizeof(struct tPwq));

	printf("Collect streaming triggered...\n");
	printf("Data is written to disk file (%s)\n", StreamFile);
	printf("Indicates when value rises past %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS2000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	printf(scaleVoltages?"mV\n" : "ADC Counts\n");
	printf("Press a key to start...\n");
	_getch();
	SetDefaults(unit);

	/* Trigger enabled
	* Rising edge
	* Threshold = 1000mV */
	SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	StreamDataHandler(unit, 100000, ANALOGUE);
}


/****************************************************************************
* OpenDevice 
* Parameters 
* - unit        pointer to the UNIT structure, where the handle will be stored
*
* Returns
* - PICO_STATUS to indicate success, or if an error occurred
***************************************************************************/
PICO_STATUS OpenDevice(UNIT *unit)
{
	short value = 0;
	int i;
	PWQ pulseWidth;
	TRIGGER_DIRECTIONS directions;
	PICO_STATUS status = ps2000aOpenUnit(&(unit->handle), NULL);
	printf("Handle: %d\n", unit->handle);
	if (status != PICO_OK) 
	{
		printf("Unable to open device\n");
		printf("Error code : %d\n", (int)status);
		while(!_kbhit());
		exit(99); // exit program
	}

	printf("Device opened successfully, cycle %d\n\n", ++cycles);

	// setup devices
	get_info(unit);
	timebase = 1;

	ps2000aMaximumValue(unit->handle, &value);
	unit->maxValue = value;

	for ( i = 0; i < unit->channelCount; i++) 
	{
		unit->channelSettings[i].enabled = TRUE;
		unit->channelSettings[i].DCcoupled = TRUE;
		unit->channelSettings[i].range = PS2000A_5V;
	}

	memset(&directions, 0, sizeof(TRIGGER_DIRECTIONS));
	memset(&pulseWidth, 0, sizeof(PWQ));

	SetDefaults(unit);

	/* Trigger disabled	*/
	SetTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	return status;
}


/****************************************************************************
* DisplaySettings 
* Displays information about the user configurable settings in this example
* Parameters 
* - unit        pointer to the UNIT structure
*
* Returns       none
***************************************************************************/
void DisplaySettings(UNIT *unit)
{
	int ch;
	int voltage;

	printf("\n\nReadings will be scaled in (%s)\n", (scaleVoltages)? ("mV") : ("ADC counts"));
	
	for (ch = 0; ch < unit->channelCount; ch++)
	{
		voltage = inputRanges[unit->channelSettings[ch].range];
		printf("Channel %c Voltage Range = ", 'A' + ch);

		if (voltage < 1000)
			printf("%dmV\n", voltage);
		else
			printf("%dV\n", voltage / 1000);
	}
	printf("\n");
}


/****************************************************************************
* ANDAnalogueDigital
* This function shows how to collect a block of data from the analogue
* ports and the digital ports at the same time, triggering when the 
* digital conditions AND the analogue conditions are met
*
* Returns       none
***************************************************************************/
void ANDAnalogueDigitalTriggered(UNIT * unit)
{
	short	triggerVoltage = mv_to_adc(1000, unit->channelSettings[PS2000A_CHANNEL_A].range, unit);


	PS2000A_TRIGGER_CHANNEL_PROPERTIES sourceDetails = {	triggerVoltage,			// thresholdUpper
															256 * 10,				// thresholdUpper Hysteresis
															triggerVoltage,			// thresholdLower
															256 * 10,				// thresholdLower Hysteresis
															PS2000A_CHANNEL_A,		// channel
															PS2000A_LEVEL};			// mode

	PS2000A_TRIGGER_CONDITIONS conditions = {	PS2000A_CONDITION_TRUE,					// Channel A
												PS2000A_CONDITION_DONT_CARE,			// Channel B
												PS2000A_CONDITION_DONT_CARE,			// Channel C
												PS2000A_CONDITION_DONT_CARE,			// Channel D
												PS2000A_CONDITION_DONT_CARE,			// external
												PS2000A_CONDITION_DONT_CARE,			// aux 
												PS2000A_CONDITION_DONT_CARE,			// pwq
												PS2000A_CONDITION_TRUE};				// digital

	

	TRIGGER_DIRECTIONS directions = {	PS2000A_RISING,				// Channel A
										PS2000A_NONE,				// Channel B
										PS2000A_NONE,				// Channel C
										PS2000A_NONE,				// Channel D
										PS2000A_NONE,				// external
										PS2000A_NONE };				// aux

	PS2000A_DIGITAL_CHANNEL_DIRECTIONS digDirections[2];		// Array size can be up to 16, an entry for each digital bit

	PWQ pulseWidth;
	memset(&pulseWidth, 0, sizeof(PWQ));

	// Set the Digital trigger so it will trigger when bit 15 is HIGH and bit 13 is HIGH
	// All non-declared bits are taken as PS2000A_DIGITAL_DONT_CARE
	//

	digDirections[0].channel = PS2000A_DIGITAL_CHANNEL_1;
	digDirections[0].direction = PS2000A_DIGITAL_DIRECTION_RISING;

	digDirections[1].channel = PS2000A_DIGITAL_CHANNEL_8;
	digDirections[1].direction = PS2000A_DIGITAL_DIRECTION_HIGH;

  printf("\nCombination Block Triggered\n");
	printf("Collects when value rises past %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS2000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	printf(scaleVoltages?"mV\n" : "ADC Counts\n");

	 printf("AND \n");
   printf("Digital Channel  1   --- Rising\n");
   printf("Digital Channel  8   --- High\n");
   printf("Other Digital Channels - Don't Care\n");
	
	printf("Press a key to start...\n");
	_getch();

	SetDefaults(unit);					// Enable Analogue channels.

	/* Trigger enabled
	* Rising edge
	* Threshold = 100mV */

	if (SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, digDirections, 2) == PICO_OK)
	{
		BlockDataHandler(unit, "First 10 readings\n", 0, MIXED);
	}

	DisableAnalogue(unit);			// Disable Analogue ports when finished;
}


/****************************************************************************
* ORAnalogueDigital
* This function shows how to collect a block of data from the analogue
* ports and the digital ports at the same time, triggering when either the 
* digital conditions OR the analogue conditions are met
*
* Returns       none
***************************************************************************/
void ORAnalogueDigitalTriggered(UNIT * unit)
{
	short	triggerVoltage = mv_to_adc(1000, unit->channelSettings[PS2000A_CHANNEL_A].range, unit);

	PS2000A_TRIGGER_CHANNEL_PROPERTIES sourceDetails = {	triggerVoltage,										// thresholdUpper
																												256 * 10,													// thresholdUpper Hysteresis
																												triggerVoltage,										// thresholdLower
																												256 * 10,													// thresholdLower Hysteresis
																												PS2000A_CHANNEL_A,								// channel
																												PS2000A_LEVEL};										// mode

	
	PS2000A_TRIGGER_CONDITIONS conditions[2];


	TRIGGER_DIRECTIONS directions = {	PS2000A_RISING,			// Channel A
																		PS2000A_NONE,				// Channel B
																		PS2000A_NONE,				// Channel C
																		PS2000A_NONE,				// Channel D
																		PS2000A_NONE,				// external
																		PS2000A_NONE };			// aux

	PS2000A_DIGITAL_CHANNEL_DIRECTIONS digDirections[2];		// Array size can be up to 16, an entry for each digital bit

	PWQ pulseWidth;


	conditions[0].channelA				= PS2000A_CONDITION_TRUE;							// Channel A
	conditions[0].channelB				= PS2000A_CONDITION_DONT_CARE;				// Channel B
	conditions[0].channelC				= PS2000A_CONDITION_DONT_CARE;				// Channel C
	conditions[0].channelD				= PS2000A_CONDITION_DONT_CARE;				// Channel D
	conditions[0].external				= PS2000A_CONDITION_DONT_CARE;				// external
	conditions[0].aux							= PS2000A_CONDITION_DONT_CARE;						// aux
	conditions[0].pulseWidthQualifier	= PS2000A_CONDITION_DONT_CARE;		// pwq
	conditions[0].digital				= PS2000A_CONDITION_DONT_CARE;					// digital


	conditions[1].channelA				= PS2000A_CONDITION_DONT_CARE;				// Channel A
	conditions[1].channelB				= PS2000A_CONDITION_DONT_CARE;				// Channel B
	conditions[1].channelC				= PS2000A_CONDITION_DONT_CARE;				// Channel C
	conditions[1].channelD				= PS2000A_CONDITION_DONT_CARE;				// Channel D
	conditions[1].external				= PS2000A_CONDITION_DONT_CARE;				// external
	conditions[1].aux					= PS2000A_CONDITION_DONT_CARE;						// aux
	conditions[1].pulseWidthQualifier	= PS2000A_CONDITION_DONT_CARE;		// pwq
	conditions[1].digital				= PS2000A_CONDITION_TRUE;								// digital


	
	memset(&pulseWidth, 0, sizeof(PWQ));

	// Set the Digital trigger so it will trigger when bit 15 is HIGH and bit 13 is HIGH
	// All non-declared bits are taken as PS2000A_DIGITAL_DONT_CARE
	//

	digDirections[0].channel = PS2000A_DIGITAL_CHANNEL_3;
	digDirections[0].direction = PS2000A_DIGITAL_DIRECTION_RISING;

	digDirections[1].channel = PS2000A_DIGITAL_CHANNEL_13;
	digDirections[1].direction = PS2000A_DIGITAL_DIRECTION_HIGH;

	printf("\nCombination Block Triggered\n");
	printf("Collects when value rises past %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS2000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	printf(scaleVoltages?"mV\n" : "ADC Counts\n");

	 printf("OR \n");
   printf("Digital Channel  3   --- Rising\n");
   printf("Digital Channel 13   --- High\n");
   printf("Other Digital Channels - Don't Care\n");

	printf("Press a key to start...\n");
	_getch();

	SetDefaults(unit);						// enable analogue ports

	/* Trigger enabled
	* Rising edge
	* Threshold = 1000mV */

	if (SetTrigger(unit, &sourceDetails, 1, conditions, 2, &directions, &pulseWidth, 0, 0, 0, digDirections, 2) == PICO_OK)
	{
		
		BlockDataHandler(unit, "First 10 readings\n", 0, MIXED);
	}
	
	DisableAnalogue(unit);					// Disable Analogue ports when finished;
}

/****************************************************************************
* DigitalBlockTriggered
* This function shows how to collect a block of data from the digital ports
* with triggering enabled
*
* Returns       none
***************************************************************************/

void DigitalBlockTriggered(UNIT * unit)
{
	PWQ pulseWidth;
	TRIGGER_DIRECTIONS directions;
	PS2000A_DIGITAL_CHANNEL_DIRECTIONS digDirections[2];		// Array size can be up to 16, an entry for each digital bit
	
	PS2000A_TRIGGER_CONDITIONS conditions = {	PS2000A_CONDITION_DONT_CARE,		// Channel A
												PS2000A_CONDITION_DONT_CARE,		// Channel B
												PS2000A_CONDITION_DONT_CARE,		// Channel C
												PS2000A_CONDITION_DONT_CARE,		// Channel D
												PS2000A_CONDITION_DONT_CARE,		// external
												PS2000A_CONDITION_DONT_CARE,		// aux
												PS2000A_CONDITION_DONT_CARE,		// pwq
												PS2000A_CONDITION_TRUE};			// digital

	
	printf("\nDigital Block Triggered\n");

	memset(&directions, 0, sizeof(TRIGGER_DIRECTIONS));
	memset(&pulseWidth, 0, sizeof(PWQ));

	printf("Collect block of data when the trigger occurs...\n");
  printf("Digital Channel  3   --- Rising\n");
  printf("Digital Channel 13   --- High\n");
  printf("Other Digital Channels - Don't Care\n");


	digDirections[0].channel = PS2000A_DIGITAL_CHANNEL_13;
	digDirections[0].direction = PS2000A_DIGITAL_DIRECTION_HIGH;

	digDirections[1].channel = PS2000A_DIGITAL_CHANNEL_3;
	digDirections[1].direction = PS2000A_DIGITAL_DIRECTION_RISING;


	if (SetTrigger(unit, NULL, 0, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, digDirections, 2) == PICO_OK)
	{
		printf("Press a key to start...\n");
		_getch();
		BlockDataHandler(unit, "First 10 readings\n", 0, DIGITAL);
	}
}


/****************************************************************************
* DigitalBlockImmediate
* This function shows how to collect a block of data from the digital ports
* with triggering disabled
*
* Returns       none
***************************************************************************/
void DigitalBlockImmediate(UNIT *unit)
{
	PWQ pulseWidth;
	TRIGGER_DIRECTIONS directions;
	PS2000A_DIGITAL_CHANNEL_DIRECTIONS digDirections;

	printf("\nDigital Block Immediate\n");
	memset(&directions, 0, sizeof(TRIGGER_DIRECTIONS));
	memset(&pulseWidth, 0, sizeof(PWQ));
	memset(&digDirections, 0, sizeof(PS2000A_DIGITAL_CHANNEL_DIRECTIONS));

	SetTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, &digDirections, 0);

	printf("Press a key to start...\n");
	_getch();
	
	BlockDataHandler(unit, "First 10 readings\n", 0, DIGITAL);
}


/****************************************************************************
*  DigitalStreamingAggregated
*  this function demonstrates how to collect a stream of Aggregated data
*  from the unit's Digital inputs (start collecting immediately)
***************************************************************************/
void DigitalStreamingAggregated(UNIT * unit)
{
	struct tPwq pulseWidth;
	struct tTriggerDirections directions;

	memset(&pulseWidth, 0, sizeof(struct tPwq));
	memset(&directions, 0, sizeof(struct tTriggerDirections));


	printf("Digital streaming with Aggregation...\n");
	printf("Press a key to start...\n");
	_getch();

	/* Trigger disabled	*/
	SetTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	StreamDataHandler(unit, 0, AGGREGATED);
}


/****************************************************************************
*  DigitalStreamingImmediate
*  this function demonstrates how to collect a stream of data
*  from the unit's Digital inputs (start collecting immediately)
***************************************************************************/
void DigitalStreamingImmediate(UNIT * unit)
{
	struct tPwq pulseWidth;
	struct tTriggerDirections directions;

	memset(&pulseWidth, 0, sizeof(struct tPwq));
	memset(&directions, 0, sizeof(struct tTriggerDirections));

	printf("Digital streaming...\n");
	printf("Press a key to start...\n");
	_getch();

	/* Trigger disabled	*/
	SetTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	StreamDataHandler(unit, 0, DIGITAL);
}


/****************************************************************************
* DigitalMenu 
* Displays digital examples available
* Parameters 
* - unit        pointer to the UNIT structure
*
* Returns       none
***************************************************************************/
void DigitalMenu(UNIT *unit)
{
	char ch;
	short enabled = TRUE;
	short disabled = 0;

	DisableAnalogue(unit);					// Disable Analogue ports;
	SetDigitals(unit);						// Enable Digital ports

	ch = ' ';
	while (ch != 'X')
	{
		printf("\n");
		printf("\nDigital Port Menu\n\n");
		printf("B - Digital Block Immediate\n");
		printf("T - Digital Block Triggered\n");
		printf("A - Analogue 'AND' Digital Triggered Block\n");
		printf("O - Analogue 'OR'  Digital Triggered Block\n");
		printf("S - Digital Streaming Mode\n");
		printf("V - Digital Streaming Aggregated\n");
		printf("X - Return to previous menu\n\n");
		printf("Operation:");

		ch = toupper(_getch());

		printf("\n\n");
		switch (ch) 
		{
			case 'B':
				DigitalBlockImmediate(unit);
				break;

			case 'T':
				DigitalBlockTriggered(unit);
				break;

			case 'A':
				ANDAnalogueDigitalTriggered(unit);
				break;

			case 'O':
				ORAnalogueDigitalTriggered(unit);
				break;

			case 'S':
				DigitalStreamingImmediate(unit);
				break;

			case 'V':
				DigitalStreamingAggregated(unit);
				break;
		}
	}

	DisableDigital(unit);					// Disable Digital ports when finished
}

/****************************************************************************
* main
* 
***************************************************************************/
int main(void)
{
	
	char ch;

	PICO_STATUS status;
	UNIT unit;
	
	printf("PS2000A driver example program\n");
	printf("Version 2.0\n\n");
	printf("\n\nOpening the device...\n");

	status = OpenDevice(&unit);
	
	ch = ' ';
	while (ch != 'X')
	{
		DisplaySettings(&unit);

		printf("\n");
		printf("B - Immediate block                           V - Set voltages\n");
		printf("T - Triggered block                           I - Set timebase\n");
		printf("E - Collect a block of data using ETS         A - ADC counts/mV\n");
		printf("R - Collect set of rapid captures             G - Signal generator\n");
		printf("S - Immediate streaming\n");
		printf("W - Triggered streaming\n");
		printf(unit.digitalPorts? "D - Digital Ports menu\n":"");
		printf("                                              X - Exit\n\n");
		printf("Operation:");

		ch = toupper(_getch());

		printf("\n\n");
		switch (ch) 
		{
		case 'B':
			CollectBlockImmediate(&unit);
			break;

		case 'T':
			CollectBlockTriggered(&unit);
			break;

		case 'R':
			CollectRapidBlock(&unit);
			break;

		case 'S':
			CollectStreamingImmediate(&unit);
			break;

		case 'W':
			CollectStreamingTriggered(&unit);
			break;

		case 'E':
			CollectBlockEts(&unit);
			break;

		case 'G':
			SetSignalGenerator(unit);
			break;

		case 'V':
			SetVoltages(&unit);
			break;

		case 'I':
			SetTimebase(unit);
			break;

		case 'A':
			scaleVoltages = !scaleVoltages;
			break;

		case 'D':
			if (unit.digitalPorts)
				DigitalMenu(&unit);
			break;

		case 'X':
			break;

		default:
			printf("Invalid operation\n");
			break;
		}
	}

	CloseDevice(&unit);

	return 1;
}
