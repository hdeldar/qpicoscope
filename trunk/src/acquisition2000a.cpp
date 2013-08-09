/*****************************************************************************
*   Copyright 2012 Vincent HERVIEUX
*
*   This file is part of QPicoscope.
*
*   QPicoscope is free software: you can redistribute it and/or modify
*   it under the terms of the GNU Lesser General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   QPicoscope is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public License
*   along with QPicoscope in files COPYING.LESSER and COPYING.
*   If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
/**
 * @file acquisition2000a.cpp
 * @brief Definition of Acquisition2000a class.
 * Acquisition2000a methods using libps2000a to drive Picoscope 2000a series HW
 * @version 0.1
 * @date 2013, january 4
 * @author Vincent HERVIEUX    -   01.04.2013   -   initial creation
 */

#include "acquisition2000a.h"

#ifdef HAVE_LIBPS2000A

#ifndef WIN32
#define Sleep(x) usleep(1000*(x))
enum BOOL {FALSE,TRUE};
#endif

#define PS2000A_MAX_SIGGEN_FREQ 10000000

/* static members initialization */
Acquisition2000a *Acquisition2000a::singleton_m = NULL;
const short Acquisition2000a::input_ranges [] = {10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};

/****************************************************************************
 *
 * constructor
 *
 ****************************************************************************/
Acquisition2000a::Acquisition2000a() :
    timebase(8)
{
	short value = 0;
	int i;
	PWQ pulseWidth;
	TRIGGER_DIRECTIONS directions;
	PICO_STATUS status = ps2000aOpenUnit(&unitOpened_m.handle, NULL);
	DEBUG ( "Handle: %d\n", unitOpened_m.handle );
	if (status != PICO_OK) 
	{
		DEBUG("Unable to open device\n");
		DEBUG("Error code : %d\n", (int)status);
		unitOpened_m.model = MODEL_NONE;
        return;
	}

	DEBUG("Device opened successfully, cycle %d\n\n", ++cycles);

	// setup devices
	get_info();
	timebase = 1;

	ps2000aMaximumValue(unitOpened_m.handle, &value);
	unitOpened_m.maxValue = value;

	for ( i = 0; i < unitOpened_m.noOfChannels; i++) 
	{
		unitOpened_m.channelSettings[i].enabled = TRUE;
		unitOpened_m.channelSettings[i].DCcoupled = TRUE;
		unitOpened_m.channelSettings[i].range = unitOpened_m.lastRange;
	}

	memset(&directions, 0, sizeof(TRIGGER_DIRECTIONS));
	memset(&pulseWidth, 0, sizeof(PWQ));

	set_defaults();

	/* Trigger disabled	*/
	set_trigger( NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	return status;
}

/****************************************************************************
 *
 * get_instance
 *
 ****************************************************************************/
Acquisition2000a* Acquisition2000a::get_instance()
{
    if(NULL == Acquisition2000a::singleton_m)
    {
        Acquisition2000a::singleton_m = new Acquisition2000a();
    }

    return Acquisition2000a::singleton_m;
}

/****************************************************************************
 *
 * destructor
 *
 ****************************************************************************/
Acquisition2000a::~Acquisition2000a()
{
    DEBUG ( "Device destroyed\n" );
    ps2000aCloseUnit( unitOpened_m.handle );
    Acquisition2000a::singleton_m = NULL;
}

/****************************************************************************
 *
 *
 *
 ****************************************************************************/
/****************************************************************************
* Callback
* used by PS2000A data streaimng collection calls, on receipt of data.
* used to set global flags etc checked by user routines
****************************************************************************/
void __stdcall CallBackStreaming(	short handle,
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
void __stdcall CallBackBlock(	short handle,
							PICO_STATUS status,
							void * pParameter)
{
	if (status != PICO_CANCELLED)
		g_ready = TRUE;
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
		status = ps2000aSetDigitalPort(unitOpened_m.handle, (PS2000A_DIGITAL_PORT)port, enabled, logicLevel);
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
	for (ch = 0; ch < unitOpened_m.channelCount; ch++)
	{
		if((status = ps2000aSetChannel(unitOpened_m.handle, (PS2000A_CHANNEL) ch, 0, PS2000A_DC, PS2000A_50MV, 0)) != PICO_OK)
			DEBUG("DisableDigital:ps2000aSetChannel(channel %d) ------ 0x%08lx \n", ch, status);
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
	for (port = 0; port < unitOpened_m.noOfDigitalPorts; port++)
	{
		if((status = ps2000aSetChannel(unitOpened_m.handle, (PS2000A_CHANNEL)PS2000A_CHANNEL_A + port, 0, PS2000A_DC, PS2000A_50MV, 0)) != PICO_OK)
			DEBUG("DisableDigital:ps2000aSetChannel(port %d) ------ 0x%08lx \n", port, status);
	}
	return status;
}

/****************************************************************************
 * adc_to_mv
 *
 * If the user selects scaling to millivolts,
 * Convert an 12-bit ADC count into millivolts
 ****************************************************************************/
int Acquisition2000a::adc_to_mv (long raw, int ch)
{
      return ( raw * input_ranges[ch] ) / unitOpened_m.maxValue;
}

/****************************************************************************
 * mv_to_adc
 *
 * Convert a millivolt value into a 16-bit ADC count
 *
 *  (useful for setting trigger thresholds)
 ****************************************************************************/
short Acquisition2000a::mv_to_adc (short mv, short ch)
{
  return ( mv * unitOpened_m.maxValue ) / input_ranges[ch];
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

	for (i = 0; i < unitOpened_m.channelCount; i++) 
	{
		if((status = ps2000aSetDataBuffers(unitOpened_m.handle, (short)i, NULL, NULL, 0, 0, PS2000A_RATIO_MODE_NONE)) != PICO_OK)
			DEBUG("ClearDataBuffers:ps2000aSetDataBuffers(channel %d) ------ 0x%08lx \n", i, status);
	}
	
	
	for (i= 0; i < unitOpened_m.noOfDigitalPorts; i++) 
	{
		if((status = ps2000aSetDataBuffer(unitOpened_m.handle, (PS2000A_CHANNEL) (i + PS2000A_DIGITAL_PORT0), NULL, 0, 0, PS2000A_RATIO_MODE_NONE))!= PICO_OK)
			DEBUG("ClearDataBuffers:ps2000aSetDataBuffer(port 0x%X) ------ 0x%08lx \n", i + PS2000A_DIGITAL_PORT0, status);
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
			DEBUG("BlockDataHandler:ps2000aSetDataBuffers(channel %d) ------ 0x%08lx \n", i, status);
		}
	}

	
	if (mode == DIGITAL || mode == MIXED)		// (MSO Only) Digital or MIXED
	{
		for (i= 0; i < unit->noOfDigitalPorts; i++) 
		{
			digiBuffer[i] = (short*)malloc(sampleCount* sizeof(short));
			status = ps2000aSetDataBuffer(unit->handle, (PS2000A_CHANNEL) (i + PS2000A_DIGITAL_PORT0), digiBuffer[i], sampleCount, 0, PS2000A_RATIO_MODE_NONE);
			DEBUG("BlockDataHandler:ps2000aSetDataBuffer(port 0x%X) ------ 0x%08lx \n", i + PS2000A_DIGITAL_PORT0, status);
		}
	}

	

	/*  find the maximum number of samples, the time interval (in timeUnits),
	*		 the most suitable time units, and the maximum oversample at the current timebase*/
	while (ps2000aGetTimebase(unit->handle, timebase, sampleCount, &timeInterval, oversample, &maxSamples, 0))
	{
		timebase++;
	}

	DEBUG("\nTimebase: %lu  SampleInterval: %ldnS  oversample: %hd\n", timebase, timeInterval, oversample);

	/* Start it collecting, then wait for completion*/
	g_ready = FALSE;
	if ((status = ps2000aRunBlock(unit->handle, 0, sampleCount, timebase, oversample,	&timeIndisposed, 0, CallBackBlock, NULL)) != PICO_OK)
		DEBUG("BlockDataHandler:ps2000aRunBlock ------ 0x%08lx \n", status);
	
	DEBUG("Waiting for trigger...Press a key to abort\n");

	while (!g_ready && !_kbhit())
	{
		Sleep(0);
	}


	if(g_ready) 
	{
		if((status = ps2000aGetValues(unit->handle, 0, (unsigned long*) &sampleCount, 1, PS2000A_RATIO_MODE_NONE, 0, NULL)) != PICO_OK)
			DEBUG("BlockDataHandler:ps2000aGetValues ------ 0x%08lx \n", status);

		/* Print out the first 10 readings, converting the readings to mV if required */
		DEBUG("%s\n",text);

		if (mode == ANALOGUE || mode == MIXED)		// if we're doing analogue or MIXED
		{
			DEBUG("Channels are in (%s)\n", ( scaleVoltages ) ? ("mV") : ("ADC Counts"));

			for (j = 0; j < unit->channelCount; j++) 
				DEBUG("Channel%c:\t", 'A' + j);
		}
		
		if (mode == DIGITAL || mode == MIXED)	// if we're doing digital or MIXED
			DEBUG("Digital");
		
		DEBUG("\n");
	
		for (i = offset; i < offset+10; i++) 
		{
			if (mode == ANALOGUE || mode == MIXED)	// if we're doing analogue or MIXED
			{
				for (j = 0; j < unit->channelCount; j++) 
				{
					if (unit->channelSettings[j].enabled) 
					{
						DEBUG("  %6d        ", scaleVoltages ? 
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
				DEBUG("0x%04X", digiValue);
			}
			DEBUG("\n");
		}

		if (mode == ANALOGUE || mode == MIXED)		// if we're doing analogue or MIXED
		{
			sampleCount = min(sampleCount, BUFFER_SIZE);

			fopen_s(&fp, BlockFile, "w");
			if (fp != NULL)
			{
				fDEBUG(fp, "Block Data log\n\n");
				fDEBUG(fp,"Results shown for each of the %d Channels are......\n",unit->channelCount);
				fDEBUG(fp,"Maximum Aggregated value ADC Count & mV, Minimum Aggregated value ADC Count & mV\n\n");

				fDEBUG(fp, "Time  ");
				for (i = 0; i < unit->channelCount; i++) 
					fDEBUG(fp," Ch   Max ADC  Max mV   Min ADC  Min mV  ");
				fDEBUG(fp, "\n");

				for (i = 0; i < sampleCount; i++) 
				{
					fDEBUG(fp, "%5lld ", g_times[0] + (long long)(i * timeInterval));
					for (j = 0; j < unit->channelCount; j++) 
					{
						if (unit->channelSettings[j].enabled) 
						{
							fDEBUG(	fp,
								"Ch%C  %5d = %+5dmV, %5d = %+5dmV   ",
								(char)('A' + j),
								buffers[j * 2][i],
								adc_to_mv(buffers[j * 2][i], unit->channelSettings[PS2000A_CHANNEL_A + j].range, unit),
								buffers[j * 2 + 1][i],
								adc_to_mv(buffers[j * 2 + 1][i], unit->channelSettings[PS2000A_CHANNEL_A + j].range, unit));
						}
					}
					fDEBUG(fp, "\n");
				}
			}
			else
			{
				DEBUG(	"Cannot open the file block.txt for writing.\n"
				"Please ensure that you have permission to access.\n");
			}
		}
	} 
	else 
	{
		DEBUG("data collection aborted\n");
		_getch();
	}

	if((status = ps2000aStop(unit->handle)) != PICO_OK)
		DEBUG("BlockDataHandler:ps2000aStop ------ 0x%08lx \n", status);

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
		for (i = 0; i < unit->noOfDigitalPorts; i++) 
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

			DEBUG("StreamDataHandler:ps2000aSetDataBuffers(channel %ld) ------ 0x%08lx \n", i, status);
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
		for (i= 0; i < unit->noOfDigitalPorts; i++) 
		{
		
			digiBuffers[i * 2] = (short*)malloc(sampleCount * sizeof(short));
			digiBuffers[i * 2 + 1] = (short*)malloc(sampleCount * sizeof(short));
			status = ps2000aSetDataBuffers(unit->handle, (PS2000A_CHANNEL) (i + PS2000A_DIGITAL_PORT0), digiBuffers[i * 2], digiBuffers[i * 2 + 1], sampleCount, 0, PS2000A_RATIO_MODE_AGGREGATE);

			DEBUG("StreamDataHandler:ps2000aSetDataBuffer(channel %ld) ------ 0x%08lx \n", i, status);
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
		for (i= 0; i < unit->noOfDigitalPorts; i++) 
		{
			digiBuffers[i] = (short*)malloc(sampleCount* sizeof(short));
			status = ps2000aSetDataBuffer(unit->handle, (PS2000A_CHANNEL) (i + PS2000A_DIGITAL_PORT0), digiBuffers[i], sampleCount, 0, PS2000A_RATIO_MODE_NONE);

			DEBUG("StreamDataHandler:ps2000aSetDataBuffer(channel %ld) ------ 0x%08lx \n", i, status);
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
		DEBUG("\nStreaming Data for %lu samples", postTrigger / downsampleRatio);
		if (preTrigger)							// we pass 0 for preTrigger if we're not setting up a trigger
			DEBUG(" after the trigger occurs\nNote: %lu Pre Trigger samples before Trigger arms\n\n",preTrigger / downsampleRatio);
		else
			DEBUG("\n\n");
	}
	else
		DEBUG("\nStreaming Data continually\n\n");

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

	DEBUG("Streaming data...Press a key to stop\n");

	
	if (mode == ANALOGUE)
	{
		fopen_s(&fp, StreamFile, "w");

		if (fp != NULL)
		{
			fDEBUG(fp,"For each of the %d Channels, results shown are....\n",unit->channelCount);
			fDEBUG(fp,"Maximum Aggregated value ADC Count & mV, Minimum Aggregated value ADC Count & mV\n\n");

			for (i = 0; i < unit->channelCount; i++) 
				fDEBUG(fp,"   Max ADC   Max mV   Min ADC   Min mV");
			fDEBUG(fp, "\n");
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
			DEBUG("\nCollected %3li samples, index = %5lu, Total: %6d samples ", g_sampleCount, g_startIndex, totalSamples);
			
			if (g_trig)
				DEBUG("Trig. at index %lu", triggeredAt);	// show where trigger occurred
			
			
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
								fDEBUG(	fp,
									"Ch%C  %5d = %+5dmV, %5d = %+5dmV   ",
									(char)('A' + j),
									buffers[j * 2][i],
									adc_to_mv(buffers[j * 2][i], unit->channelSettings[PS2000A_CHANNEL_A + j].range, unit),
									buffers[j * 2 + 1][i],
									adc_to_mv(buffers[j * 2 + 1][i], unit->channelSettings[PS2000A_CHANNEL_A + j].range, unit));
							}
						}
						fDEBUG(fp, "\n");
					}
					else
						DEBUG("Cannot open the file stream.txt for writing.\n");

				}

				if (mode == DIGITAL)
				{
					portValue = 0x00ff & digiBuffers[1][i];
					portValue <<= 8;
					portValue |= 0x00ff & digiBuffers[0][i];

					DEBUG("\nIndex=%04lu: Value = 0x%04X  =  ",i, portValue);

					for (bit = 0; bit < 16; bit++)
					{
						DEBUG( (0x8000 >> bit) & portValue? "1 " : "0 ");
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

					DEBUG("\nIndex=%04lu: Bitwise  OR of last %ld readings = 0x%04X ",i,  downsampleRatio, portValueOR);
					DEBUG("\nIndex=%04lu: Bitwise AND of last %ld readings = 0x%04X ",i,  downsampleRatio, portValueAND);
				}
			}
		}
	}

	ps2000aStop(unit->handle);

	if (!g_autoStopped) 
	{
		DEBUG("\ndata collection aborted\n");
		_getch();
	}

	if (g_overflow)
	{
		DEBUG("\nStreaming overflow. Not able to keep up with streaming data rate\n");
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
		for (i = 0; i < unit->noOfDigitalPorts; i++) 
		{
			free(digiBuffers[i]);
		}

	}

	if (mode == AGGREGATED) 		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->noOfDigitalPorts * 2; i++) 
		{
			free(digiBuffers[i]);
		}
	}

	ClearDataBuffers(unit);
}


/****************************************************************************
* set_trigger
* 
* Parameters
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
PICO_STATUS Acquisition2000a::set_trigger(	PS2000A_TRIGGER_CHANNEL_PROPERTIES * channelProperties,
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

	if ((status = ps2000aSetTriggerChannelProperties(unitOpened_m.handle,
		channelProperties,
		nChannelProperties,
		auxOutputEnabled,
		autoTriggerMs)) != PICO_OK) 
	{
			DEBUG("SetTrigger:ps2000aSetTriggerChannelProperties ------ Ox%8lx \n", status);
			return status;
	}

	if ((status = ps2000aSetTriggerChannelConditions(unitOpened_m.handle, triggerConditions, nTriggerConditions)) != PICO_OK) 
	{
			DEBUG("SetTrigger:ps2000aSetTriggerChannelConditions ------ 0x%8lx \n", status);
			return status;
	}

	if ((status = ps2000aSetTriggerChannelDirections(unitOpened_m.handle,
		directions->channelA,
		directions->channelB,
		directions->channelC,
		directions->channelD,
		directions->ext,
		directions->aux)) != PICO_OK) 
	{
			DEBUG("SetTrigger:ps2000aSetTriggerChannelDirections ------ 0x%08lx \n", status);
			return status;
	}

	if ((status = ps2000aSetTriggerDelay(unitOpened_m.handle, delay)) != PICO_OK) 
	{
		DEBUG("SetTrigger:ps2000aSetTriggerDelay ------ 0x%08lx \n", status);
		return status;
	}

	if((status = ps2000aSetPulseWidthQualifier(unitOpened_m.handle,
		pwq->conditions,
		pwq->nConditions, 
		pwq->direction,
		pwq->lower, 
		pwq->upper, 
		pwq->type)) != PICO_OK)
	{
		DEBUG("SetTrigger:ps2000aSetPulseWidthQualifier ------ 0x%08lx \n", status);
		return status;
	}

	if (unit->noOfDigitalPorts)					// ps2000aSetTriggerDigitalPortProperties function only applies to MSO	
	{
		if((status = ps2000aSetTriggerDigitalPortProperties(unitOpened_m.handle,
														digitalDirections, 
														nDigitalDirections)) != PICO_OK) 
		{
			DEBUG("SetTrigger:ps2000aSetTriggerDigitalPortProperties ------ 0x%08lx \n", status);
			return status;
		}
	}
	return status;
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

	DEBUG("Collect rapid block triggered...\n");
	DEBUG("Collects when value rises past %d",	scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS2000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	DEBUG(scaleVoltages?"mV\n" : "ADC Counts\n");
	DEBUG("Press any key to abort\n");

	set_defaults();

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
		DEBUG("Rapid capture aborted. %lu complete blocks were captured\n", nCompletedCaptures);
		DEBUG("\nPress any key...\n\n");
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
		DEBUG("\nCapture %d\n", capture + 1);
		for (channel = 0; channel < unit->channelCount; channel++) 
		{
			DEBUG("Channel %c\t", 'A' + channel);
		}
		DEBUG("\n");

		for(i = 0; i < 10; i++)
		{
			for (channel = 0; channel < unit->channelCount; channel++) 
			{
				if(unit->channelSettings[channel].enabled)
				{
					DEBUG("%d\t\t", rapidBuffers[channel][capture][i]);
				}
			}
			DEBUG("\n");
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
* Select input voltage ranges for channels
****************************************************************************/
void SetVoltages(UNIT * unit)
{
	int i, ch;
	int count = 0;

	/* See what ranges are available... */
	for (i = unit->firstRange; i <= unit->lastRange; i++) 
	{
		DEBUG("%d -> %d mV\n", i, inputRanges[i]);
	}

	do
	{
		/* Ask the user to select a range */
		DEBUG("Specify voltage range (%d..%d)\n", unit->firstRange, unit->lastRange);
		DEBUG("99 - switches channel off\n");
		for (ch = 0; ch < unit->channelCount; ch++) 
		{
			DEBUG("\n");
			do 
			{
				DEBUG("Channel %c: ", 'A' + ch);
				fflush(stdin);
				scanf_s("%hd", &unit->channelSettings[ch].range);
			} while (unit->channelSettings[ch].range != 99 && (unit->channelSettings[ch].range < unit->firstRange || unit->channelSettings[ch].range > unit->lastRange));

			if (unit->channelSettings[ch].range != 99) 
			{
				DEBUG(" - %d mV\n", inputRanges[unit->channelSettings[ch].range]);
				unit->channelSettings[ch].enabled = TRUE;
				count++;
			} 
			else 
			{
				DEBUG("Channel Switched off\n");
				unit->channelSettings[ch].enabled = FALSE;
			}
		}
		DEBUG(count == 0? "\n** At least 1 channel must be enabled **\n\n":"");
	}
	while(count == 0);	// must have at least one channel enabled

	set_defaults();	// Put these changes into effect
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

	DEBUG("Enter frequency in Hz: "); // Ask user to enter signal frequency;
	do 
	{
		scanf_s("%lu", &frequency);
	} while (frequency <= 0 || frequency > 10000000);

	if(frequency > 0) // Ask user to enter type of signal
	{
		DEBUG("Signal generator On");
		DEBUG("Enter type of waveform (0..9 or 99)\n");
		DEBUG("0:\tSINE\n");
		DEBUG("1:\tSQUARE\n");
		DEBUG("2:\tTRIANGLE\n");
		DEBUG("99:\tUSER WAVEFORM\n");
		DEBUG("  \t(see manual for more)\n");

		do 
		{
			scanf_s("%hd", &waveform);
		} while (waveform != 99 && (waveform < 0 || waveform >= PS2000A_MAX_WAVE_TYPES));

		if (waveform == 99) // custom waveform selected - user needs to select file
		{
			waveformSize = 0;

			DEBUG("Select a waveform file to load: ");
			scanf_s("%s", fileName, 128);
			if (fopen_s(&fp, fileName, "r") == 0) 
			{ // Having opened file, read in data - one number per line (at most 8192 lines), with values in (0..4095)
				while (EOF != fscanf_s(fp, "%hi", (arbitraryWaveform + waveformSize))&& waveformSize++ < 8192);
				fclose(fp);
				DEBUG("File successfully loaded\n");
			} 
			else 
			{
				DEBUG("Invalid filename\n");
				return;
			}
		}
	} 
	else 
	{
		waveform = 0;
		DEBUG("Signal generator Off");
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

		DEBUG("Status of Arbitrary Gen: 0x%08lx \n", status);
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

	set_defaults();

	DEBUG("Collect streaming...\n");
	DEBUG("Data is written to disk file (%s)\n", StreamFile);
	DEBUG("Press a key to start...\n");
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

	DEBUG("Collect streaming triggered...\n");
	DEBUG("Data is written to disk file (%s)\n", StreamFile);
	DEBUG("Indicates when value rises past %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS2000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	DEBUG(scaleVoltages?"mV\n" : "ADC Counts\n");
	DEBUG("Press a key to start...\n");
	_getch();
	set_defaults();

	/* Trigger enabled
	* Rising edge
	* Threshold = 1000mV */
	SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	StreamDataHandler(unit, 100000, ANALOGUE);
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

	DEBUG("\n\nReadings will be scaled in (%s)\n", (scaleVoltages)? ("mV") : ("ADC counts"));
	
	for (ch = 0; ch < unit->channelCount; ch++)
	{
		voltage = inputRanges[unit->channelSettings[ch].range];
		DEBUG("Channel %c Voltage Range = ", 'A' + ch);

		if (voltage < 1000)
			DEBUG("%dmV\n", voltage);
		else
			DEBUG("%dV\n", voltage / 1000);
	}
	DEBUG("\n");
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

  DEBUG("\nCombination Block Triggered\n");
	DEBUG("Collects when value rises past %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS2000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	DEBUG(scaleVoltages?"mV\n" : "ADC Counts\n");

	 DEBUG("AND \n");
   DEBUG("Digital Channel  1   --- Rising\n");
   DEBUG("Digital Channel  8   --- High\n");
   DEBUG("Other Digital Channels - Don't Care\n");
	
	DEBUG("Press a key to start...\n");
	_getch();

	set_defaults();					// Enable Analogue channels.

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

	DEBUG("\nCombination Block Triggered\n");
	DEBUG("Collects when value rises past %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS2000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	DEBUG(scaleVoltages?"mV\n" : "ADC Counts\n");

	 DEBUG("OR \n");
   DEBUG("Digital Channel  3   --- Rising\n");
   DEBUG("Digital Channel 13   --- High\n");
   DEBUG("Other Digital Channels - Don't Care\n");

	DEBUG("Press a key to start...\n");
	_getch();

	set_defaults();						// enable analogue ports

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

	
	DEBUG("\nDigital Block Triggered\n");

	memset(&directions, 0, sizeof(TRIGGER_DIRECTIONS));
	memset(&pulseWidth, 0, sizeof(PWQ));

	DEBUG("Collect block of data when the trigger occurs...\n");
  DEBUG("Digital Channel  3   --- Rising\n");
  DEBUG("Digital Channel 13   --- High\n");
  DEBUG("Other Digital Channels - Don't Care\n");


	digDirections[0].channel = PS2000A_DIGITAL_CHANNEL_13;
	digDirections[0].direction = PS2000A_DIGITAL_DIRECTION_HIGH;

	digDirections[1].channel = PS2000A_DIGITAL_CHANNEL_3;
	digDirections[1].direction = PS2000A_DIGITAL_DIRECTION_RISING;


	if (set_trigger( NULL, 0, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, digDirections, 2) == PICO_OK)
	{
		DEBUG("Press a key to start...\n");
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

	DEBUG("\nDigital Block Immediate\n");
	memset(&directions, 0, sizeof(TRIGGER_DIRECTIONS));
	memset(&pulseWidth, 0, sizeof(PWQ));
	memset(&digDirections, 0, sizeof(PS2000A_DIGITAL_CHANNEL_DIRECTIONS));

	set_trigger( NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, &digDirections, 0);

	DEBUG("Press a key to start...\n");
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


	DEBUG("Digital streaming with Aggregation...\n");
	DEBUG("Press a key to start...\n");
	_getch();

	/* Trigger disabled	*/
	set_trigger( NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

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

	DEBUG("Digital streaming...\n");
	DEBUG("Press a key to start...\n");
	_getch();

	/* Trigger disabled	*/
	set_trigger( NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

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
		DEBUG("\n");
		DEBUG("\nDigital Port Menu\n\n");
		DEBUG("B - Digital Block Immediate\n");
		DEBUG("T - Digital Block Triggered\n");
		DEBUG("A - Analogue 'AND' Digital Triggered Block\n");
		DEBUG("O - Analogue 'OR'  Digital Triggered Block\n");
		DEBUG("S - Digital Streaming Mode\n");
		DEBUG("V - Digital Streaming Aggregated\n");
		DEBUG("X - Return to previous menu\n\n");
		DEBUG("Operation:");

		ch = toupper(_getch());

		DEBUG("\n\n");
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
 *
 * ps2000FastStreamingReady
 *
 ****************************************************************************/
void  __stdcall Acquisition2000a::ps2000FastStreamingReady( short **overviewBuffers,
                                                              short overflow,
                                                                unsigned long triggeredAt,
                                                                short triggered,
                                                                short auto_stop,
                                                              unsigned long nValues)
{
    (void)overviewBuffers;
    (void)overflow;
    (void)triggeredAt;
    (void)triggered;
    Acquisition2000a* instance = Acquisition2000a::get_instance();
    if(NULL != instance)
    {
        instance->unitOpened_m.trigger.advanced.totalSamples += nValues;
        instance->unitOpened_m.trigger.advanced.autoStop = auto_stop;
    }
}

/****************************************************************************
 *
 * get_device_info
 *
 ****************************************************************************/
void Acquisition2000a::get_device_info(device_info_t* info)
{
    if(NULL == info)
    {
        ERROR("%s : invalid pointer given!\n", __FUNCTION__);
        return;
    }

    memset(info, 0, sizeof(device_info_t));

    switch(unitOpened_m.model)
    {
        case MODEL_PS2206:
            snDEBUG(info->device_name, DEVICE_NAME_MAX, "PS2206");
            break;
        case MODEL_PS2207:
            snDEBUG(info->device_name, DEVICE_NAME_MAX, "PS2207"); 
            break;
        case MODEL_PS2208:
            snDEBUG(info->device_name, DEVICE_NAME_MAX, "PS2208"); 
            break;
        case MODEL_PS2205MSO:
            snDEBUG(info->device_name, DEVICE_NAME_MAX, "PS2205MSO"); 
            break;
        case MODEL_NONE:
            snDEBUG(info->device_name, DEVICE_NAME_MAX, "No device or device not supported"); 
            break;
    }
    info->nb_channels = unitOpened_m.noOfChannels;
#ifdef TEST_WITHOUT_HW
    snDEBUG(info->device_name, DEVICE_NAME_MAX, "Tests without HW");
    info->nb_channels = 2;
#endif
}

/****************************************************************************
 * set_defaults - restore default settings
 ****************************************************************************/
void Acquisition2000a::set_defaults (void)
{
	PICO_STATUS status;
	int i;
    ps2000aSetEts( unitOpened_m.handle, PS2000A_ETS_OFF, 0, 0, NULL); // Turn off ETS

    for (i = 0; i < unitOpened_m.noOfChannels; i++) // reset channels to most recent settings
    {
		status = ps2000aSetChannel(unitOpened_m.handle, 
                                   (PS2000A_CHANNEL) PS2000A_CHANNEL_A + i,
                                   unitOpened_m.channelSettings[PS2000A_CHANNEL_A + i].enabled,
                                   (PS2000A_COUPLING) unitOpened_m.channelSettings[PS2000A_CHANNEL_A + i].DCcoupled,
                                   (PS2000A_RANGE) unitOpened_m.channelSettings[PS2000A_CHANNEL_A + i].range, 0);
    }


/****************************************************************************
 * set_trigger_advanced - set advance trigger parameters
 ****************************************************************************/
void Acquisition2000a::set_trigger_advanced(void)
{
    short ok = 0;
    short auto_trigger_ms = 0;

    // to trigger of more than one channel set this parameter to 2 or more
    // each condition can only have on parameter set to PS2000A_CONDITION_TRUE or PS2000A_CONDITION_FALSE
    // if more than on condition is set then it will trigger off condition one, or condition two etc.
    unitOpened_m.trigger.advanced.nProperties = 1;
    // set the trigger channel to channel A by using PS2000A_CONDITION_TRUE
    unitOpened_m.trigger.advanced.conditions = (PS2000A_TRIGGER_CONDITIONS*)malloc (sizeof (PS2000A_TRIGGER_CONDITIONS) * unitOpened_m.trigger.advanced.nProperties);
    unitOpened_m.trigger.advanced.conditions->channelA = PS2000A_CONDITION_TRUE;
    unitOpened_m.trigger.advanced.conditions->channelB = PS2000A_CONDITION_DONT_CARE;
    unitOpened_m.trigger.advanced.conditions->channelC = PS2000A_CONDITION_DONT_CARE;
    unitOpened_m.trigger.advanced.conditions->channelD = PS2000A_CONDITION_DONT_CARE;
    unitOpened_m.trigger.advanced.conditions->external = PS2000A_CONDITION_DONT_CARE;
    unitOpened_m.trigger.advanced.conditions->pulseWidthQualifier = PS2000A_CONDITION_DONT_CARE;

    // set channel A to rising
    // the remainder will be ignored as only a condition is set for channel A
    unitOpened_m.trigger.advanced.directions.channelA = PS2000A_ADV_RISING;
    unitOpened_m.trigger.advanced.directions.channelB = PS2000A_ADV_RISING;
    unitOpened_m.trigger.advanced.directions.channelC = PS2000A_ADV_RISING;
    unitOpened_m.trigger.advanced.directions.channelD = PS2000A_ADV_RISING;
    unitOpened_m.trigger.advanced.directions.ext = PS2000A_ADV_RISING;


    unitOpened_m.trigger.advanced.channelProperties = (PS2000A_TRIGGER_CHANNEL_PROPERTIES*)malloc (sizeof (PS2000A_TRIGGER_CHANNEL_PROPERTIES) * unitOpened_m.trigger.advanced.nProperties);
    // there is one property for each condition
    // set channel A
    // trigger level 1500 adc counts the trigger point will vary depending on the voltage range
    // hysterisis 4096 adc counts
    unitOpened_m.trigger.advanced.channelProperties->channel = (short) PS2000A_CHANNEL_A;
    unitOpened_m.trigger.advanced.channelProperties->thresholdMajor = 1500;
    // not used in level triggering, should be set when in window mode
    unitOpened_m.trigger.advanced.channelProperties->thresholdMinor = 0;
    // used in level triggering, not used when in window mode
    unitOpened_m.trigger.advanced.channelProperties->hysteresis = (short) 4096;
    unitOpened_m.trigger.advanced.channelProperties->thresholdMode = PS2000A_LEVEL;

    ok = ps2000SetAdvTriggerChannelConditions (unitOpened_m.handle, unitOpened_m.trigger.advanced.conditions, unitOpened_m.trigger.advanced.nProperties);
    if ( !ok )
        WARNING("ps2000SetAdvTriggerChannelConditions returned value is not 0.");
    ok = ps2000SetAdvTriggerChannelDirections (unitOpened_m.handle,
                                                                                unitOpened_m.trigger.advanced.directions.channelA,
                                                                                unitOpened_m.trigger.advanced.directions.channelB,
                                                                                unitOpened_m.trigger.advanced.directions.channelC,
                                                                                unitOpened_m.trigger.advanced.directions.channelD,
                                                                                unitOpened_m.trigger.advanced.directions.ext);
    if ( !ok )
        WARNING("ps2000SetAdvTriggerChannelDirections returned value is not 0.");
    ok = ps2000SetAdvTriggerChannelProperties (unitOpened_m.handle,
                                                                                unitOpened_m.trigger.advanced.channelProperties,
                                                                                unitOpened_m.trigger.advanced.nProperties,
                                                                                auto_trigger_ms);
    if ( !ok )
        WARNING("ps2000SetAdvTriggerChannelProperties returned value is not 0.");


    // remove comments to try triggering with a pulse width qualifier
    // add a condition for the pulse width eg. in addition to the channel A or as a replacement
    //unitOpened_m.trigger.advanced.pwq.conditions = malloc (sizeof (PS2000A_PWQ_CONDITIONS));
    //unitOpened_m.trigger.advanced.pwq.conditions->channelA = PS2000A_CONDITION_TRUE;
    //unitOpened_m.trigger.advanced.pwq.conditions->channelB = PS2000A_CONDITION_DONT_CARE;
    //unitOpened_m.trigger.advanced.pwq.conditions->channelC = PS2000A_CONDITION_DONT_CARE;
    //unitOpened_m.trigger.advanced.pwq.conditions->channelD = PS2000A_CONDITION_DONT_CARE;
    //unitOpened_m.trigger.advanced.pwq.conditions->external = PS2000A_CONDITION_DONT_CARE;
    //unitOpened_m.trigger.advanced.pwq.nConditions = 1;

    //unitOpened_m.trigger.advanced.pwq.direction = PS2000A_RISING;
    //unitOpened_m.trigger.advanced.pwq.type = PS2000A_PW_TYPE_LESS_THAN;
    //// used when type    PS2000A_PW_TYPE_IN_RANGE,    PS2000A_PW_TYPE_OUT_OF_RANGE
    //unitOpened_m.trigger.advanced.pwq.lower = 0;
    //unitOpened_m.trigger.advanced.pwq.upper = 10000;
    //ps2000SetPulseWidthQualifier (unitOpened_m.handle,
    //                                                            unitOpened_m.trigger.advanced.pwq.conditions,
    //                                                            unitOpened_m.trigger.advanced.pwq.nConditions,
    //                                                            unitOpened_m.trigger.advanced.pwq.direction,
    //                                                            unitOpened_m.trigger.advanced.pwq.lower,
    //                                                            unitOpened_m.trigger.advanced.pwq.upper,
    //                                                            unitOpened_m.trigger.advanced.pwq.type);

    ok = ps2000SetAdvTriggerDelay (unitOpened_m.handle, 0, -10);
}

/****************************************************************************
 * Collect_block_immediate
 *  this function demonstrates how to collect a single block of data
 *  from the unit (start collecting immediately)
 ****************************************************************************/
void Acquisition2000a::collect_block_immediate (void)
{
    int i = 0;
    long     time_interval;
    short     time_units;
    short     oversample;
    int     no_of_samples = BUFFER_SIZE;
    int nb_of_samples_in_screen = 0;
    short     auto_trigger_ms = 0;
    long     time_indisposed_ms;
    short     overflow;
    long     max_samples;
    short ch = 0;
    double* values_V[CHANNEL_MAX] = {NULL};
    double* time[CHANNEL_MAX] = {NULL};
    double time_multiplier = 0.;
    double time_offset[CHANNEL_MAX] = {0.};
    int index[CHANNEL_MAX] = {0};

    DEBUG( "Collect block immediate...\n" );

    set_defaults();

    /* Trigger disabled
     */
    set_trigger ( NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0 );

    /* TODO */
    BlockDataHandler(&unitOpened_m, "First 10 readings\n", 0, ANALOGUE);
}

/****************************************************************************
 * Collect_block_triggered
 *  this function demonstrates how to collect a single block of data from the
 *  unit, when a trigger event occurs.
 ****************************************************************************/

void Acquisition2000a::collect_block_triggered (trigger_e trigger_slope, double trigger_level)
{
	short	triggerVoltage = mv_to_adc(1000, unitOpened_m.channelSettings[PS2000A_CHANNEL_A].range );

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

	DEBUG("Collect block triggered\n");
	DEBUG("Data is written to disk file (%s)\n", BlockFile);
	DEBUG("Collects when value rises past %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unitOpened_m.channelSettings[PS2000A_CHANNEL_A].range)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	DEBUG(scaleVoltages?"mV\n" : "ADC Counts\n");

	set_defaults();

	/* Trigger enabled
	* Rising edge
	* Threshold = 1000mV */
	set_trigger( &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	BlockDataHandler(&unitOpened_m, "Ten readings after trigger\n", 0, ANALOGUE);
}

void Acquisition2000a::collect_block_advanced_triggered ()
{

}


/****************************************************************************
 * Collect_block_ets
 *  this function demonstrates how to collect a block of
 *  data using equivalent time sampling (ETS).
 ****************************************************************************/

void Acquisition2000a::collect_block_ets (void)
{
	PICO_STATUS status;
	long ets_sampletime;
	short	triggerVoltage = mv_to_adc(1000,	unitOpened_m.channelSettings[PS2000A_CHANNEL_A].range, unit);
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

	DEBUG("Collect ETS block...\n");
	DEBUG("Collects when value rises past %d", scaleVoltages? 
		adc_to_mv(sourceDetails.thresholdUpper,	unitOpened_m.channelSettings[PS2000A_CHANNEL_A].range)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	DEBUG(scaleVoltages? "mV\n" : "ADC Counts\n");

	set_defaults();

	//Trigger enabled
	//Rising edge
	//Threshold = 1000mV
	status = set_trigger(&sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, delay, 0, 0, 0, 0);

	if ((status = ps2000aSetEts(unitOpened_m.handle, PS2000A_ETS_FAST, 20, 4, &ets_sampletime)) != PICO_OK)
		DEBUG("CollectBlockEts:ps2000aSetEts ------ 0x%08lx \n", status);
	
	DEBUG("ETS Sample Time is: %ld\n", ets_sampletime);

	BlockDataHandler(unit, "Ten readings after trigger\n", BUFFER_SIZE / 10 - 5, ANALOGUE); // 10% of data is pre-trigger
}

/****************************************************************************
 *
 * Collect_streaming
 *  this function demonstrates how to use streaming.
 *
 * In this mode, you can collect data continuously.
 *
 * This example writes data to disk...
 * don't leave it running too long or it will fill your disk up!
 *
 * Each call to ps2000_get_times_and_values returns the readings since the
 * last call
 *
 * The time is in microseconds: it will wrap around at 2^32 (approx 2,000 seconds)
 * if you don't need the time, you can just call ps2000_get_values
 *
 ****************************************************************************/

void Acquisition2000a::collect_streaming (void)
{
    int    i = 0;
    int count = 0;
    int    no_of_values;
    short  overflow;
    int    ok;
    short  ch;
    double values_V[BUFFER_SIZE] = {0};
    double time[BUFFER_SIZE] = {0};
    DEBUG ( "Collect streaming...\n" );

    set_defaults ();

    /* You cannot use triggering for the start of the data...
    */
    ps2000_set_trigger ( unitOpened_m.handle, PS2000A_NONE, 0, 0, 0, 0 );

    /* Collect data at time_per_division_m / 100  intervals
    * Max BUFFER_SIZE points on each call
    *  (buffer must be big enough for max time between calls
    *
    *  Start it collecting,
    *  then wait for trigger event
    */
    ok = ps2000_run_streaming ( unitOpened_m.handle, (time_per_division_m * 1000. / 100.), 1000, 0 );
    DEBUG ( "OK: %d\n", ok );

    
    while ( sem_trywait(&thread_stop) )
    {
        no_of_values = ps2000_get_values ( unitOpened_m.handle,
            unitOpened_m.channelSettings[PS2000A_CHANNEL_A].values,
            unitOpened_m.channelSettings[PS2000A_CHANNEL_B].values,
            unitOpened_m.channelSettings[PS2000A_CHANNEL_C].values,
            unitOpened_m.channelSettings[PS2000A_CHANNEL_D].values,
            &overflow,
            BUFFER_SIZE );
        DEBUG ( "%d values, overflow %d\n", no_of_values, overflow );

        for (ch = 0; (ch < unitOpened_m.noOfChannels) && (no_of_values > 0); ch++)
        {
            if (unitOpened_m.channelSettings[ch].enabled)
            {

                for (  i = 0; i < no_of_values; i++, count++ )
                {
                    values_V[count] = 0.001 * adc_to_mv(unitOpened_m.channelSettings[ch].values[i], unitOpened_m.channelSettings[ch].range);
                    // TODO time will be probably wrong here, need to guess how to convert time range to time step...
                    //time[i] = ( i ? time[i-1] : 0) + unitOpened_m.channelSettings[ch].range
                    time[count] = count * 0.01 * time_per_division_m;
                    DEBUG("V: %lf (range %d) T: %lf\n", values_V[count], unitOpened_m.channelSettings[ch].range, time[count]);
                    // 500 points are making a screen:
                    if(count == 500)
                    {
                        count = 0;
                        memset(time, 0, BUFFER_SIZE * sizeof(double));
                        memset(values_V, 0, BUFFER_SIZE * sizeof(double));
                    }
                }
                
                draw->setData(ch+1, time, values_V, count);

            }

        }
        Sleep(100);
    }

    ps2000aStop( unitOpened_m.handle );

}

void Acquisition2000a::collect_fast_streaming (void)
{
    unsigned long    i;
    short  overflow;
    int     ok;
    short ch;
    unsigned long nPreviousValues = 0;
    short values_a[BUFFER_SIZE_STREAMING];
    short values_b[BUFFER_SIZE_STREAMING];
    short *values = NULL;
    double values_mV[BUFFER_SIZE_STREAMING];
    double time[BUFFER_SIZE_STREAMING];
    unsigned long triggerAt;
    short triggered;
    unsigned long no_of_samples;
    double startTime = 0;


    DEBUG ( "Collect fast streaming...\n" );

    set_defaults ();

    /* You cannot use triggering for the start of the data...
    */
    ps2000_set_trigger ( unitOpened_m.handle, PS2000A_NONE, 0, 0, 0, 0 );

    unitOpened_m.trigger.advanced.autoStop = 0;
    unitOpened_m.trigger.advanced.totalSamples = 0;
    unitOpened_m.trigger.advanced.triggered = 0;

    /* Collect data at 10us intervals
    * 100000 points with an agregation of 100 : 1
    *    Auto stop after the 100000 samples
    *  Start it collecting,
    */
    ok = ps2000_run_streaming_ns ( unitOpened_m.handle, 10, PS2000A_US, BUFFER_SIZE_STREAMING, 1, 100, 30000 );
    DEBUG ( "OK: %d\n", ok );

    /* From here on, we can get data whenever we want...
    */

    while (!unitOpened_m.trigger.advanced.autoStop && sem_trywait(&thread_stop))
    {

        ps2000_get_streaming_last_values (unitOpened_m.handle, &Acquisition2000a::ps2000FastStreamingReady);
        if (nPreviousValues != unitOpened_m.trigger.advanced.totalSamples)
        {

            DEBUG ("Values collected: %ld\n", unitOpened_m.trigger.advanced.totalSamples - nPreviousValues);
            nPreviousValues =     unitOpened_m.trigger.advanced.totalSamples;

        }
        Sleep (0);

    }

    ps2000aStop(unitOpened_m.handle);

    no_of_samples = ps2000_get_streaming_values_no_aggregation (unitOpened_m.handle,
                                                                &startTime, // get samples from the beginning
                                                                values_a, // set buffer for channel A
                                                                values_b, // set buffer for channel B
                                                                NULL,
                                                                NULL,
                                                                &overflow,
                                                                &triggerAt,
                                                                &triggered,
                                                                BUFFER_SIZE_STREAMING);


    // print out the first 20 readings
    for ( i = 0; i < 20; i++ )
    {
        for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
        {
            if (unitOpened_m.channelSettings[ch].enabled)
            {
                DEBUG("%d, ", adc_to_mv ((!ch ? values_a[i] : values_b[i]), unitOpened_m.channelSettings[ch].range) );
            }
        }
            DEBUG("\n");
    }

    for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
    {
        if (unitOpened_m.channelSettings[ch].enabled)
        {
            switch(ch)
            {
                case 0:
                    values = values_a;
                break;
                case 1:
                    values = values_b;
                break;
                default:
                    return;
                break;
            }

            for (  i = 0; i < no_of_samples; i++ )
            {
                values_mV[i] = adc_to_mv(values[i], unitOpened_m.channelSettings[ch].range);
                // TODO time will be probably wrong here, need to guess how to convert time range to time step...
                time[i] = ( i ? time[i-1] : 0) + unitOpened_m.channelSettings[ch].range;
            }

            draw->setData(ch+1, values_mV, time, no_of_samples);
        }
               
    }


/*    DEBUG ( "Data is written to disk file (data.txt)\n" );
    fp = fopen ( "data.txt", "w+" );
    if (fp != NULL)
    {
        for ( i = 0; i < no_of_samples; i++ )
        {
            for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
            {
                if (unitOpened_m.channelSettings[ch].enabled)
                {
                    fDEBUG ( fp, "%d, ", adc_to_mv ((!ch ? values_a[i] : values_b[i]), unitOpened_m.channelSettings[ch].range) );
                }
            }
            fDEBUG (fp, "\n");
        }
        fclose ( fp );
    }
    else
        ERROR("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
*/

    //getch ();
}

void Acquisition2000a::collect_fast_streaming_triggered (void)
{
    unsigned long    i;
    FILE     *fp;
    short  overflow;
    int     ok;
    short ch;
    unsigned long nPreviousValues = 0;
    short values_a[BUFFER_SIZE_STREAMING];
    short values_b[BUFFER_SIZE_STREAMING];
    unsigned long    triggerAt;
    short triggered;
    unsigned long no_of_samples;
    double startTime = 0;



    DEBUG ( "Collect fast streaming triggered...\n" );
    DEBUG ( "Data is written to disk file (data.txt)\n" );
    DEBUG ( "Press a key to start\n" );
    //getch ();

    set_defaults ();

    set_trigger_advanced ();

    unitOpened_m.trigger.advanced.autoStop = 0;
    unitOpened_m.trigger.advanced.totalSamples = 0;
    unitOpened_m.trigger.advanced.triggered = 0;

    /* Collect data at 10us intervals
    * 100000 points with an agregation of 100 : 1
    *    Auto stop after the 100000 samples
    *  Start it collecting,
    */
    ok = ps2000_run_streaming_ns ( unitOpened_m.handle, 10, PS2000A_US, BUFFER_SIZE_STREAMING, 1, 100, 30000 );
    DEBUG ( "OK: %d\n", ok );

    /* From here on, we can get data whenever we want...
    */

    while (!unitOpened_m.trigger.advanced.autoStop)
    {
        ps2000_get_streaming_last_values (unitOpened_m.handle, ps2000FastStreamingReady);
        if (nPreviousValues != unitOpened_m.trigger.advanced.totalSamples)
        {
            DEBUG ("Values collected: %ld\n", unitOpened_m.trigger.advanced.totalSamples - nPreviousValues);
            nPreviousValues =     unitOpened_m.trigger.advanced.totalSamples;
        }
        Sleep (0);
    }

    ps2000aStop(unitOpened_m.handle);

    no_of_samples = ps2000_get_streaming_values_no_aggregation (unitOpened_m.handle,
                                                                &startTime, // get samples from the beginning
                                                                values_a, // set buffer for channel A
                                                                values_b,    // set buffer for channel B
                                                                NULL,
                                                                NULL,
                                                                &overflow,
                                                                &triggerAt,
                                                                &triggered,
                                                                BUFFER_SIZE_STREAMING);


    // if the unit triggered print out ten samples either side of the trigger point
    // otherwise print the first 20 readings
    for ( i = (triggered ? triggerAt - 10 : 0) ; i < ((triggered ? triggerAt - 10 : 0) + 20); i++)
    {
        for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
        {
            if (unitOpened_m.channelSettings[ch].enabled)
            {
                DEBUG ("%d, ", adc_to_mv ((!ch ? values_a[i] : values_b[i]), unitOpened_m.channelSettings[ch].range) );
            }
        }
        DEBUG ("\n");
    }

    fp = fopen ( "data.txt", "w" );
    if (fp != NULL)
    {
        for ( i = 0; i < no_of_samples; i++ )
        {
            for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
            {
                if (unitOpened_m.channelSettings[ch].enabled)
                {
                    fDEBUG ( fp, "%d, ", adc_to_mv ((!ch ? values_a[i] : values_b[i]), unitOpened_m.channelSettings[ch].range) );
                }
            }
            fDEBUG (fp, "\n");
        }
        fclose ( fp );
    }
    else
        DEBUG("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
    //getch ();
}


/****************************************************************************
 *
 *
 ****************************************************************************/
void Acquisition2000a::get_info (void)
  {
  char description [6][25]=  {
                               "Driver Version",
                               "USB Version",
                               "Hardware Version",
                               "Variant Info",
                               "Serial",
                               "Error Code"
                              };
  short  i = 0, r = 0;
  char     line [80] = {0};
  int    variant = 0;

  if( unitOpened_m.handle )
  {
    for ( i = 0; i < 5; i++ )
    {
      ps2000aGetUnitInfo( unitOpened_m.handle, line, sizeof (line), &r, i );
            if (i == 3)
            {
              variant = atoi(line);
            }
      DEBUG ( "%s: %s\n", description[i], line );
    }

    switch (variant)
        {
        case MODEL_PS2205MSO:
            unitOpened_m.model = MODEL_PS2205MSO;
            unitOpened_m.hasSignalGenerator = TRUE;
            unitOpened_m.hasEts = FALSE;
            unitOpened_m.hasAdvancedTriggering = FALSE;
            unitOpened_m.hasFastStreaming = FALSE;
            unitOpened_m.firstRange = PS2000A_50MV;
            unitOpened_m.lastRange = PS2000A_20V;
            //unitOpened_m.maxTimebase = PS2104_MAX_TIMEBASE;
            //unitOpened_m.timebases = unitOpened_m.maxTimebase;
            unitOpened_m.noOfChannels = 2;
            unitOpened_m.noOfDigitalPorts = 2;
        break;

        case MODEL_PS2206:
            unitOpened_m.model = MODEL_PS2206;
            unitOpened_m.hasSignalGenerator = TRUE;
            unitOpened_m.hasEts = FALSE;
            unitOpened_m.hasAdvancedTriggering = FALSE;
            unitOpened_m.hasFastStreaming = FALSE;
            unitOpened_m.firstRange = PS2000A_50MV;
            unitOpened_m.lastRange = PS2000A_20V;
            //unitOpened_m.maxTimebase = PS2105_MAX_TIMEBASE;
            //unitOpened_m.timebases = unitOpened_m.maxTimebase;
            unitOpened_m.noOfChannels = 2;
            unitOpened_m.noOfDigitalPorts = 0;
        break;

        case MODEL_PS2207:
            unitOpened_m.model = MODEL_PS2207;
            unitOpened_m.hasSignalGenerator = TRUE;
            unitOpened_m.hasEts = FALSE;
            unitOpened_m.hasAdvancedTriggering = FALSE;
            unitOpened_m.hasFastStreaming = FALSE;
            unitOpened_m.firstRange = PS2000A_50MV;
            unitOpened_m.lastRange = PS2000A_20V;
            //unitOpened_m.maxTimebase = PS2105_MAX_TIMEBASE;
            //unitOpened_m.timebases = unitOpened_m.maxTimebase;
            unitOpened_m.noOfChannels = 2;
            unitOpened_m.noOfDigitalPorts = 0;
        break;

        case MODEL_PS2208:
            unitOpened_m.model = MODEL_PS2208;
            unitOpened_m.hasSignalGenerator = TRUE;
            unitOpened_m.hasEts = FALSE;
            unitOpened_m.hasAdvancedTriggering = FALSE;
            unitOpened_m.hasFastStreaming = FALSE;
            unitOpened_m.firstRange = PS2000A_50MV;
            unitOpened_m.lastRange = PS2000A_20V;
            //unitOpened_m.maxTimebase = PS2105_MAX_TIMEBASE;
            //unitOpened_m.timebases = unitOpened_m.maxTimebase;
            unitOpened_m.noOfChannels = 2;
            unitOpened_m.noOfDigitalPorts = 0;
        break;

        default:
            ERROR("Unit not supported");
            unitOpened_m.model = MODEL_NONE;
            unitOpened_m.firstRange = PS2000A_100MV;
            unitOpened_m.lastRange = PS2000A_20V;
            unitOpened_m.timebases = PS2105_MAX_TIMEBASE;
            unitOpened_m.noOfChannels = 0; 
        }

  }
  else
  {
    ERROR ( "Unit Not Opened\n" );
    ps2000_get_unit_info ( unitOpened_m.handle, line, sizeof (line), PS2000A_ERROR_CODE );
    DEBUG ( "%s: %s\n", description[5], line );
        unitOpened_m.model = MODEL_NONE;
        unitOpened_m.firstRange = PS2000A_100MV;
        unitOpened_m.lastRange = PS2000A_20V;
        unitOpened_m.timebases = PS2105_MAX_TIMEBASE;
        unitOpened_m.noOfChannels = 0;
#ifdef TEST_WITHOUT_HW
        unitOpened_m.noOfChannels = 2;
        unitOpened_m.firstRange = PS2000A_50MV;
        unitOpened_m.lastRange = PS2000A_20V;
#endif
    }
}



void Acquisition2000a::set_sig_gen (e_wave_type waveform, long frequency)
{
	PICO_STATUS status;
	long frequency;
	char fileName [128];
	FILE * fp = NULL;
	short arbitraryWaveform [8192];
	short waveformSize = 0;

	memset(&arbitraryWaveform, 0, 8192);

    if (frequency <= 0 || frequency > PS2000A_MAX_SIGGEN_FREQ)
    {
        ERROR("%s: Invalid frequency setted!\n",__FUNCTION__);
        return;
    }

    if (waveform < 0 || waveform >= E_WAVE_TYPE_DC_VOLTAGE)
    {
        ERROR("%s: Invalid waveform setted!\n",__FUNCTION__);
        return;
    }

		status = ps2000aSetSigGenBuiltIn(unitOpened_m.handle, 0, 1000000, waveform, (float)frequency, (float)frequency, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void Acquisition2000a::set_sig_gen_arb (long int frequency)
{
    unsigned char arbitraryWaveform [8192];
    short waveformSize = 0;
    double delta;

    memset(&arbitraryWaveform, 0, 8192);

    if (frequency < 0 || frequency > 10000000)
    {
        ERROR("invalid frequency %ld\n", frequency);
        return;
    }


    waveformSize = 0;

//    DEBUG("Select a waveform file to load: ");

//    scanf("%s", fileName);
//    if ((fp = fopen(fileName, "r")))
//    { // Having opened file, read in data - one number per line (at most 4096 lines), with values in (0..255)
//        while (EOF != fscanf(fp, "%c", (arbitraryWaveform + waveformSize))&& waveformSize++ < 4096)
//            ;
//        fclose(fp);
//    }
//    else
//    {
//        WARNING("Invalid filename\n");
//        return;
//    }


    delta = ((frequency * waveformSize) / 8192.0) * 4294967296.0 * 8e-9;
    ps2000aSetSigGenArbitrary(unitOpened_m.handle,
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
}

/****************************************************************************
 *
 * Select timebase, set oversample to on and time units as nano seconds
 *
 ****************************************************************************/
void Acquisition2000a::set_timebase(double time_per_division)
  {
  short  i = 0;
  long   time_interval = 0;
  short  oversample = 1;
  long   max_samples = 0;

  DEBUG ( "Specify timebase\n" );
  time_per_division_m = time_per_division;
  
  /* See what ranges are available...
   */
  for (i = 0; i < unitOpened_m.timebases; i++)
  {
      ps2000aGetTimebase( unitOpened_m.handle, i, BUFFER_SIZE, &time_interval, oversample, &max_samples, 0 );
#ifdef TEST_WITHOUT_HW
      switch(i)
      {
      case 0:
          time_interval = 0;
          break;
      case 1:
          time_interval = 0;
          break;
      case 2:
          time_interval = 0;
          break;
      case 3:
          time_interval = 1;
          break;
      case 4:
          time_interval = 10;
          break;
      case 6:
          time_interval = 100;
          break;
      case 8:
          time_interval = 1000;
          break;
      case 10:
          time_interval = 10000;
          break;
      case 11:
          time_interval = 1000000;
          break;
      default:
          time_interval = 0;

          break;
      }


#endif
      if ( time_interval > 0 )
      {
          DEBUG ( "%d -> %ld ns\n", i, time_interval );
          /**
           * we want 100 points per division.
           * Screen has 5 time divisions.
           * So we want 500 points
           */
          if(((double)time_interval * 1E-9) <= (time_per_division * 0.010)){
              timebase = i;
          }
          else if(((double)time_interval * 1E-9)) > (time_per_division * 0.010)){
              break;
          }
      }
  }

#ifndef TEST_WITHOUT_HW
  ps2000aGetTimebase( unitOpened_m.handle, timebase, BUFFER_SIZE, &time_interval, oversample, &max_samples, 0 );
#endif
  DEBUG ( "Timebase %d - %ld ns\n", timebase, time_interval );
  }

/****************************************************************************
 * Select coupling for all channels
 ****************************************************************************/
void Acquisition2000a::set_DC_coupled(current_e coupling)
{
    short ch = 0;
    for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
    {
        unitOpened_m.channelSettings[ch].DCcoupled = coupling;
    }

}
/****************************************************************************
 * Select input voltage ranges for channels A and B
 ****************************************************************************/
void Acquisition2000a::set_voltages (channel_e channel_index, double volts_per_division)
{
    uint8_t i = 0;

    DEBUG("channel index %d, volts/div %lf\n", channel_index, volts_per_division);

    if (channel_index >= unitOpened_m.noOfChannels || channel_index >=CHANNEL_MAX)
    {
        ERROR ( "%s : invalid channel index!\n", __FUNCTION__ );
        return;
    }

    if((5. * volts_per_division) > ((double)input_ranges[unitOpened_m.lastRange] / 1000.)){
        ERROR ( "%s : invalid voltage index!\n", __FUNCTION__ );
        return;
    }

    /* find the first range that includes the voltage caliber */
    for ( i = unitOpened_m.firstRange; i <= unitOpened_m.lastRange; i++ )
    {
        DEBUG ( "trying range %d -> %d mV\n", i, input_ranges[i] );
        if(((double)input_ranges[i] / 1000.) >= (5. * volts_per_division))
        {
            unitOpened_m.channelSettings[channel_index].range = i;
            break;
        }
    }

    if(unitOpened_m.channelSettings[channel_index].range != CHANNEL_OFF)
    {
        DEBUG ( "Channel %c has now range %d mV\n", 'A' + channel_index, input_ranges[unitOpened_m.channelSettings[channel_index].range]);
        unitOpened_m.channelSettings[channel_index].enabled = TRUE;
    }
    else
    {
        DEBUG ( "Channel %c Switched off\n", 'A' + channel_index);
        unitOpened_m.channelSettings[channel_index].enabled = FALSE;
    }

}


#endif // HAVE_LIBPS2000A
