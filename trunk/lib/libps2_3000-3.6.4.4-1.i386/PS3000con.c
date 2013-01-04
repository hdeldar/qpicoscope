/**************************************************************************
 *
 * Filename:    PS3000con.c
 *
 * Copyright:   Pico Technology Limited 2002-8
 *
 * Author:      MAS
 *
 * Description:
 *   This is a console-mode program that demonstrates how to use the
 *   PS3000 driver.
 *
 * Examples:
 *    Collect a block of samples immediately
 *    Collect a block of samples when a trigger event occurs
 *    Collect a block of samples using an advanced trigger
 *			- PS3223, PS3224, PS3423, PS3424, PS3225, PS3425
 *    Collect a block using ETS
 *			- PS3204, PS3205, PS3206
 *    Collect a stream of data
 *    Collect a stream of data using an advanced trigger
 *			- PS3223, PS3224, PS3423, PS3424, PS3225, PS3425
 *
 *	To build this application
 *		Windows: set up a project for a 32-bit console mode application
 *			 add this file to the project
 *			 add PS3000bc.lib to the project (Borland C only)
 *			 add PS3000.lib to the project (Microsoft C only)
 *			 add PS3000.h to the project
 *			 then build the project
 *
 *		Linux: gcc -lps3000 PS3000con.c -oPS3000con
 *		       ./PS3000con
 *
 *
 ***************************************************************************/

#ifdef WIN32
/* Headers for Windows */
#include "windows.h"
#include <conio.h>
#include <stdio.h>

/* Definitions of PS3000 driver routines on Windows*/
#include "PS3000.h"

#else
/* Headers for Linux */
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Definition of PS3000 driver routines on Linux */
#include <libps3000/ps3000.h>

#define __stdcall
#define Sleep(x) usleep(1000*(x))
enum BOOL {FALSE,TRUE};

/* A function to detect a keyboard press on Linux */
int kbhit()
{
	struct termios oldt, newt;
	int bytesWaiting;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	setbuf(stdin, NULL);
	ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return bytesWaiting;
}

/* A function to get a single character on Linux */
int getch()
{
	struct termios oldt, newt;
	int ch;
	int bytesWaiting;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	setbuf(stdin, NULL);
	do {
		ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
		if (bytesWaiting)
			getchar();
	} while (bytesWaiting);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}
/* End of Linux-specific definitions */
#endif


#define BUFFER_SIZE 	1024
#define BUFFER_SIZE_STREAMING 100000
#define MAX_CHANNELS 4

int		scale_to_mv = 1;
short		channel_mv [PS3000_MAX_CHANNELS];
short		timebase = 8;

long 		sig_gen_frequency = 0;
PS3000_WAVE_TYPES sig_gen_wave = PS3000_SINE;

#define QUAD_SCOPE 4
#define DUAL_SCOPE 2



typedef enum {
	MODEL_NONE = 0,
  MODEL_PS3204 = 3204,
	MODEL_PS3205 = 3205,
	MODEL_PS3206 = 3206,
	MODEL_PS3223 = 3223,
	MODEL_PS3423 = 3423,
	MODEL_PS3224 = 3224,
	MODEL_PS3424 = 3424,
	MODEL_PS3225 = 3225,
	MODEL_PS3425 = 3425
} MODEL_TYPE;

typedef struct
{
	THRESHOLD_DIRECTION	channelA;
	THRESHOLD_DIRECTION	channelB;
	THRESHOLD_DIRECTION	channelC;
	THRESHOLD_DIRECTION	channelD;
	THRESHOLD_DIRECTION	ext;
} DIRECTIONS;

typedef struct
{
	PWQ_CONDITIONS					*	conditions;
	short										nConditions;
	THRESHOLD_DIRECTION		  direction;
	unsigned long						lower;
	unsigned long						upper;
	PULSE_WIDTH_TYPE				type;
} PULSE_WIDTH_QUALIFIER;

typedef struct
{
	PS3000_CHANNEL channel;
	float threshold;
	short direction;
	float delay;
} SIMPLE;

typedef struct
{
	short hysterisis;
	DIRECTIONS directions;
	short nProperties;
	TRIGGER_CONDITIONS * conditions;
	TRIGGER_CHANNEL_PROPERTIES * channelProperties;
	PULSE_WIDTH_QUALIFIER pwq;
 	unsigned long totalSamples;
	short autoStop;
	short triggered;
} ADVANCED;


typedef struct 
{
	SIMPLE simple;
	ADVANCED advanced;
} TRIGGER_CHANNEL;



typedef struct {
	short DCcoupled;
	short range;
	short enabled;
	short values [BUFFER_SIZE];
} CHANNEL_SETTINGS;

typedef struct  {
	short handle;
	MODEL_TYPE model;
  	PS3000_RANGE firstRange;
	PS3000_RANGE lastRange;
	char  signalGenerator;
	char  external;
	short timebases;
	short maxTimebases;
	short noOfChannels;
	CHANNEL_SETTINGS channelSettings[MAX_CHANNELS];
	TRIGGER_CHANNEL trigger;
	short				hasAdvancedTriggering;
	short				hasFastStreaming;
	short				hasEts;
} UNIT_MODEL; 

UNIT_MODEL unitOpened;

long times[BUFFER_SIZE];

long input_ranges [PS3000_MAX_RANGES] = {10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 400000};

void  __stdcall ps3000FastStreamingReady( short **overviewBuffers,
															  short overflow,
																unsigned long triggeredAt,
																short triggered,
																short auto_stop,
															  unsigned long nValues)
{
	unitOpened.trigger.advanced.totalSamples += nValues;
	unitOpened.trigger.advanced.autoStop = auto_stop;
}

/****************************************************************************
 *
 * scaled_time_units
 *
 ****************************************************************************/
char * scaled_time_units (short time_units)
  {
  switch ( time_units )
    {
    case 0:
      return "fs";
    case 1:
      return "ps";
    case 2:
      return "ns";
    case 3:
      return "us";
    case 4:
      return "ms";
    }
  return "Not Known";
  }

/****************************************************************************
 * adc_to_mv
 *
 * If the user selects scaling to millivolts,
 * Convert an 12-bit ADC count into millivolts
 ****************************************************************************/
int adc_to_mv (long raw, int ch)
  {
  return ( scale_to_mv ) ? ( raw * input_ranges[ch] ) / 32767 : raw;
  }

/****************************************************************************
 * mv_to_adc
 *
 * Convert a millivolt value into a 12-bit ADC count
 *
 *  (useful for setting trigger thresholds)
 ****************************************************************************/
short mv_to_adc (short mv, short ch)
  {
  return (short) ( ( mv * 32767 ) / input_ranges[ch] );
  }

/****************************************************************************
 * set_defaults - restore default settings
 ****************************************************************************/
void set_defaults (void)
{
	int i;

  ps3000_set_ets ( unitOpened.handle, PS3000_ETS_OFF, 0, 0 );

	for (i = 0; i < unitOpened.noOfChannels; i++)
	{
    ps3000_set_channel ( unitOpened.handle,
			                   PS3000_CHANNEL_A + i,
												 unitOpened.channelSettings[PS3000_CHANNEL_A + i].enabled ,
												 unitOpened.channelSettings[PS3000_CHANNEL_A + i].DCcoupled ,
												 unitOpened.channelSettings[PS3000_CHANNEL_A + i].range);
	}
}

void set_trigger_advanced(void)
{
	short ok = 0;
  short 	auto_trigger_ms = 0;

	// to trigger of more than one channel set this parameter to 2 or more
	// each condition can only have on parameter set to CONDITION_TRUE or CONDITION_FALSE
	// if more than on condition is set then it will trigger off condition one, or condition two etc.
	unitOpened.trigger.advanced.nProperties = 1;
	// set the trigger channel to channel A by using CONDITION_TRUE
	unitOpened.trigger.advanced.conditions = malloc (sizeof (TRIGGER_CONDITIONS) * unitOpened.trigger.advanced.nProperties);
	unitOpened.trigger.advanced.conditions->channelA = CONDITION_TRUE;
	unitOpened.trigger.advanced.conditions->channelB = CONDITION_DONT_CARE;
	unitOpened.trigger.advanced.conditions->channelC = CONDITION_DONT_CARE;
	unitOpened.trigger.advanced.conditions->channelD = CONDITION_DONT_CARE;
	unitOpened.trigger.advanced.conditions->external = CONDITION_DONT_CARE;
	unitOpened.trigger.advanced.conditions->pulseWidthQualifier = CONDITION_DONT_CARE;

	// set channel A to rising
	// the remainder will be ignored as only a condition is set for channel A
	unitOpened.trigger.advanced.directions.channelA = RISING;
	unitOpened.trigger.advanced.directions.channelB = RISING;
	unitOpened.trigger.advanced.directions.channelC = RISING;
	unitOpened.trigger.advanced.directions.channelD = RISING;
	unitOpened.trigger.advanced.directions.ext = RISING;


	unitOpened.trigger.advanced.channelProperties = malloc (sizeof (TRIGGER_CHANNEL_PROPERTIES) * unitOpened.trigger.advanced.nProperties);
	// there is one property for each condition
	// set channel A
	// trigger level 1500 adc counts the trigger point will vary depending on the voltage range
	// hysterisis 4096 adc counts  
	unitOpened.trigger.advanced.channelProperties->channel = (short) PS3000_CHANNEL_A;
	unitOpened.trigger.advanced.channelProperties->thresholdMajor = 1500;
	// not used in level triggering, should be set when in window mode
	unitOpened.trigger.advanced.channelProperties->thresholdMinor = 0;
	// used in level triggering, not used when in window mode
	unitOpened.trigger.advanced.channelProperties->hysteresis = (short) 4096;
	unitOpened.trigger.advanced.channelProperties->thresholdMode = LEVEL;

	ok = ps3000SetAdvTriggerChannelConditions (unitOpened.handle, unitOpened.trigger.advanced.conditions, unitOpened.trigger.advanced.nProperties);
	ok = ps3000SetAdvTriggerChannelDirections (unitOpened.handle,
																				unitOpened.trigger.advanced.directions.channelA,
																				unitOpened.trigger.advanced.directions.channelB,
																				unitOpened.trigger.advanced.directions.channelC,
																				unitOpened.trigger.advanced.directions.channelD,
																				unitOpened.trigger.advanced.directions.ext);
	ok = ps3000SetAdvTriggerChannelProperties (unitOpened.handle,
																				unitOpened.trigger.advanced.channelProperties,
																				unitOpened.trigger.advanced.nProperties,
																				auto_trigger_ms);


	// remove comments to try triggering with a pulse width qualifier
	// add a condition for the pulse width eg. in addition to the channel A or as a replacement
	//unitOpened.trigger.advanced.pwq.conditions = malloc (sizeof (PS2000_PWQ_CONDITIONS));
	//unitOpened.trigger.advanced.pwq.conditions->channelA = CONDITION_TRUE;
	//unitOpened.trigger.advanced.pwq.conditions->channelB = CONDITION_DONT_CARE;
	//unitOpened.trigger.advanced.pwq.conditions->channelC = CONDITION_DONT_CARE;
	//unitOpened.trigger.advanced.pwq.conditions->channelD = CONDITION_DONT_CARE;
	//unitOpened.trigger.advanced.pwq.conditions->external = CONDITION_DONT_CARE;
	//unitOpened.trigger.advanced.pwq.nConditions = 1;

	//unitOpened.trigger.advanced.pwq.direction = RISING;
	//unitOpened.trigger.advanced.pwq.type = PW_TYPE_LESS_THAN;
	//// used when type	PW_TYPE_IN_RANGE,	PW_TYPE_OUT_OF_RANGE
	//unitOpened.trigger.advanced.pwq.lower = 0;
	//unitOpened.trigger.advanced.pwq.upper = 10000;
	//ps3000SetPulseWidthQualifier (unitOpened.handle,
	//															unitOpened.trigger.advanced.pwq.conditions,
	//															unitOpened.trigger.advanced.pwq.nConditions, 
	//															unitOpened.trigger.advanced.pwq.direction,
	//															unitOpened.trigger.advanced.pwq.lower,
	//															unitOpened.trigger.advanced.pwq.upper,
	//															unitOpened.trigger.advanced.pwq.type);

	ok = ps3000SetAdvTriggerDelay (unitOpened.handle, 0, -10); 
}



/****************************************************************************
 * Collect_block_immediate
 *  this function demonstrates how to collect a single block of data
 *  from the unit (start collecting immediately)
 ****************************************************************************/

void collect_block_immediate (void)
{
  int		i, j;
  long 	time_interval;
  short 	time_units;
  short 	oversample;
  long 	no_of_samples = BUFFER_SIZE;
  FILE 	*fp;
  short 	auto_trigger_ms = 0;
  long 	time_indisposed_ms;
  short 	overflow;
  long 	max_samples;
  
  printf ( "Collect block immediate...\n" );
  printf ( "Data is written to disk file (data.txt) \n");
  printf ( "Press a key to start\n" );
  getch ();

  set_defaults ();

  /* Trigger disabled
   */
  ps3000_set_trigger ( unitOpened.handle, PS3000_NONE, 0, PS3000_RISING, 0, auto_trigger_ms );

  /*  find the maximum number of samples, the time interval (in time_units),
   *		 the most suitable time units, and the maximum oversample at the current timebase
   */
	oversample = 1;
  while (!ps3000_get_timebase ( unitOpened.handle,
																timebase,
  					      							no_of_samples,
																&time_interval,
																&time_units,
																oversample,
																&max_samples))
	  timebase++;										

  printf ( "timebase: %hd\toversample:%hd\n", timebase, oversample );
  /* Start it collecting,
   *  then wait for completion
   */
  ps3000_run_block ( unitOpened.handle, no_of_samples, timebase, oversample, &time_indisposed_ms );
  while ( !ps3000_ready ( unitOpened.handle ) )
    {
    Sleep ( 100 );
    }

  ps3000_stop ( unitOpened.handle );

  /* Should be done now...
   *  get the times (in nanoseconds)
   *   and the values (in ADC counts)
   */
  ps3000_get_values ( unitOpened.handle, 
											unitOpened.channelSettings[PS3000_CHANNEL_A].values, 
											unitOpened.channelSettings[PS3000_CHANNEL_B].values,
											unitOpened.channelSettings[PS3000_CHANNEL_C].values,
											unitOpened.channelSettings[PS3000_CHANNEL_D].values,
											&overflow, no_of_samples );
  
  /* Print out the first 10 readings,
  *  converting the readings to mV if required
  */
  printf ( "First 10 readings\n" );
  printf ( "Value\n" );
  for (j = 0; j < unitOpened.noOfChannels; j++)
  {
    if(unitOpened.channelSettings[j].enabled)
    {
      printf ( "(%s)\t", scale_to_mv ? "mV" : "ADC" );				
    }
  }
  printf("\n");

  for ( i = 0; i < 10; i++ )
  {
    for (j = 0; j < unitOpened.noOfChannels; j++)
    {
      if(unitOpened.channelSettings[j].enabled)
      {
        printf ( "%d\t", adc_to_mv ( unitOpened.channelSettings[j].values[i], unitOpened.channelSettings[j].range) );
      }
    }
    printf("\n");
  }

  fp = fopen ( "data.txt","w" );
  if (fp != NULL)
  {
	  for ( i = 0; i < BUFFER_SIZE; i++ )
	  {
	    fprintf ( fp,"%ld ", times[i]);
	    for ( j = 0; j < unitOpened.noOfChannels; j++)
			{
	      if(unitOpened.channelSettings[j].enabled)
			  {
		fprintf ( fp, ",%d, %d,", unitOpened.channelSettings[j].values[i],
					    											adc_to_mv ( unitOpened.channelSettings[j].values[i], unitOpened.channelSettings[PS3000_CHANNEL_A + j].range) );
				}
	    }
			fprintf(fp, "\n");
		}
	  fclose(fp);
  }
  else
	printf("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
}

/****************************************************************************
 * Collect_block_triggered
 *  this function demonstrates how to collect a single block of data from the
 *  unit, when a trigger event occurs.
 ****************************************************************************/

void collect_block_triggered (void)
{
	int		i, j;
	int		trigger_sample;
	long 	time_interval;
	short 	time_units;
	short 	oversample;
	long 	no_of_samples = BUFFER_SIZE;
	FILE 	*fp;
	short 	auto_trigger_ms = 0;
	long 	time_indisposed_ms;
	short 	overflow;
	int 	threshold_mv =100;
	long 	max_samples;

	printf ( "Collect block triggered...\n" );
	printf ( "Collects when value on A rises past %dmV\n", threshold_mv );
	printf ( "Data is written to disk file (data.txt)\n");
	printf ( "Press a key to start...\n" );
	getch ();

	set_defaults ();

	/* Trigger enabled
	* Rising edge
	* Threshold = 100mV
	* 10% pre-trigger  (negative is pre-, positive is post-)
	*/
	unitOpened.trigger.simple.channel = PS3000_CHANNEL_A;
	unitOpened.trigger.simple.delay = -10;
	unitOpened.trigger.simple.direction = PS3000_RISING;
	unitOpened.trigger.simple.threshold = 100;

	ps3000_set_trigger ( unitOpened.handle,
		(short)unitOpened.trigger.simple.channel,
		mv_to_adc ((short)unitOpened.trigger.simple.threshold, unitOpened.channelSettings[unitOpened.trigger.simple.channel].range),
		unitOpened.trigger.simple.direction,
		(short)unitOpened.trigger.simple.delay,
		auto_trigger_ms );


	/*  find the maximum number of samples, the time interval (in time_units),
	*		 the most suitable time units, and the maximum oversample at the current timebase
	*/
	oversample = 1;
	while (!ps3000_get_timebase ( unitOpened.handle,
		timebase,
		no_of_samples,
		&time_interval,
		&time_units,
		oversample,
		&max_samples))
		timebase++;

	/* Start it collecting,
	*  then wait for completion
	*/
	ps3000_run_block ( unitOpened.handle, BUFFER_SIZE, timebase, oversample, &time_indisposed_ms );

	printf ( "Waiting for trigger..." );
	printf ( "Press a key to abort\n" );

	while (( !ps3000_ready ( unitOpened.handle )) && ( !kbhit () ))
	{
		Sleep ( 100 );
	}

	if (kbhit ())
	{
		getch ();

		printf ( "data collection aborted\n" );
	}
	else
	{
		ps3000_stop ( unitOpened.handle );

		/* Get the times (in units specified by time_units)
		*  and the values (in ADC counts)
		*/
		ps3000_get_times_and_values ( unitOpened.handle,
			times,
			unitOpened.channelSettings[PS3000_CHANNEL_A].values,
			unitOpened.channelSettings[PS3000_CHANNEL_B].values,
			unitOpened.channelSettings[PS3000_CHANNEL_C].values,
			unitOpened.channelSettings[PS3000_CHANNEL_D].values,
			&overflow, time_units, BUFFER_SIZE );

		/* Print out the first 10 readings,
		*  converting the readings to mV if required
		*/
		printf ("Ten readings around trigger\n");
		printf ("Time\tValue\n");
		printf ("(%s)\t", scaled_time_units (time_units));
		for (j = 0; j < unitOpened.noOfChannels; j++)
		{
			if(unitOpened.channelSettings[j].enabled)
			{
				printf ( "(%s)\t", scale_to_mv ? "mV" : "ADC" );				
			}
		}
		printf("\n");

		/* This calculation is correct for 10% pre-trigger
		*/
		trigger_sample = BUFFER_SIZE / 10;

		for (i = trigger_sample - 5; i < trigger_sample + 5; i++)
		{
			printf ( "%ld\t", times[i]);
			for (j = 0; j < unitOpened.noOfChannels; j++)
			{
				if(unitOpened.channelSettings[j].enabled)
				{
					printf ( "%d\t", adc_to_mv ( unitOpened.channelSettings[j].values[i], unitOpened.channelSettings[j].range) );
				}
			}
			printf("\n");
		}

		fp = fopen ( "data.txt","w" );
		if (fp != NULL)
		{
			for ( i = 0; i < BUFFER_SIZE; i++ )
			{
				fprintf ( fp,"%ld ", times[i]);
				for ( j = 0; j < unitOpened.noOfChannels; j++)
				{
					if(unitOpened.channelSettings[j].enabled)
					{
						fprintf ( fp, ",%d, %d,", unitOpened.channelSettings[j].values[i],
							adc_to_mv ( unitOpened.channelSettings[j].values[i], unitOpened.channelSettings[j].range) );
					}
				}
				fprintf(fp, "\n");
			}
			fclose(fp);
		}
		else
			printf("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
	}
}

void collect_block_advanced_trigger()
{
  int		i, j;
  int		trigger_sample;
  long 	time_interval;
  short 	time_units;
  short 	oversample;
  long 	no_of_samples = BUFFER_SIZE;
  FILE 	*fp;
  long 	time_indisposed_ms;
  short 	overflow;
  int 	threshold_mv =100;
  long 	max_samples;

  printf ( "Collect block triggered...\n" );
  printf ( "Collects when value on A rises past %dmV\n", threshold_mv );
  printf ( "Data is written to disk file (data.txt) \n");
  printf ( "Press a key to start...\n" );
  getch ();

  set_defaults ();

	set_trigger_advanced ();

  /*  find the maximum number of samples, the time interval (in time_units),
   *		 the most suitable time units, and the maximum oversample at the current timebase
   */
	oversample = 1;
  while (!ps3000_get_timebase ( unitOpened.handle,
                        timebase,
  					      	    no_of_samples,
                        &time_interval,
                        &time_units,
                        oversample,
                        &max_samples))
	  timebase++;

  /* Start it collecting,
   *  then wait for completion
   */
  ps3000_run_block ( unitOpened.handle, BUFFER_SIZE, timebase, oversample, &time_indisposed_ms );

  printf ( "Waiting for trigger..." );
  printf ( "Press a key to abort\n" );

  while (( !ps3000_ready ( unitOpened.handle )) && ( !kbhit () ))
  {
    Sleep ( 100 );
  }

  if (kbhit ())
  {
    getch ();

    printf ( "data collection aborted\n" );
  }
  else
  {
    ps3000_stop ( unitOpened.handle );

    /* Get the times (in units specified by time_units)
     *  and the values (in ADC counts)
     */
    ps3000_get_times_and_values ( unitOpened.handle,
			                            times,
																	unitOpened.channelSettings[PS3000_CHANNEL_A].values,
																	unitOpened.channelSettings[PS3000_CHANNEL_B].values,
																	unitOpened.channelSettings[PS3000_CHANNEL_C].values,
																	unitOpened.channelSettings[PS3000_CHANNEL_D].values,
																	&overflow, time_units, BUFFER_SIZE );

    /* Print out the first 10 readings,
     *  converting the readings to mV if required
     */
    printf ("Ten readings around trigger\n");
    printf ("Time\tValue\n");
    printf ("(%s)\t", scaled_time_units (time_units));
    for (j = 0; j < unitOpened.noOfChannels; j++)
    {
      if(unitOpened.channelSettings[j].enabled)
      {
        printf ( "(%s)\t", scale_to_mv ? "mV" : "ADC" );				
      }
    }
    printf("\n");

    /* This calculation is correct for 10% pre-trigger
     */
    trigger_sample = BUFFER_SIZE / 10;

    for (i = trigger_sample - 5; i < trigger_sample + 5; i++)
    {
      printf ( "%ld\t", times[i]);
  		for (j = 0; j < unitOpened.noOfChannels; j++)
			{
			  if(unitOpened.channelSettings[j].enabled)
				{
          printf ( "%d\t", adc_to_mv ( unitOpened.channelSettings[j].values[i], unitOpened.channelSettings[j].range) );
				}
			}
			printf("\n");
    }
 
    fp = fopen ( "data.txt","w" );
    if (fp != NULL)
    {
	for ( i = 0; i < BUFFER_SIZE; i++ )
	{
			fprintf ( fp,"%ld ", times[i]);
			for ( j = 0; j < unitOpened.noOfChannels; j++)
			{
		if(unitOpened.channelSettings[j].enabled)
			{
						fprintf ( fp, ",%d, %d,", unitOpened.channelSettings[j].values[i],
																			adc_to_mv ( unitOpened.channelSettings[j].values[i], unitOpened.channelSettings[j].range) );
				}
			}
			fprintf(fp, "\n");
	}
	fclose(fp);

     }
	else
		printf("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
   }
}

/****************************************************************************
 * Collect_block_ets
 *  this function demonstrates how to collect a block of
 *  data using equivalent time sampling (ETS).
 ****************************************************************************/

void collect_block_ets (void)
{
	int		i, j;
	int		trigger_sample;
	FILE 	*fp;
	short 	auto_trigger_ms = 0;
	long 	time_indisposed_ms;
	short 	overflow;
	long  	ets_sampletime;

	printf ( "Collect ETS block...\n" );
	printf ( "Collects when value on A rises past 100mV\n" );
	printf ( "Data is written to disk file (data.txt) \n");
	printf ( "Press a key to start...\n" );
	getch ();

	set_defaults ();

	/* Trigger enabled
	* Rising edge
	* Threshold = 1500mV
	* 10% pre-trigger  (negative is pre-, positive is post-)
	*/
	unitOpened.trigger.simple.channel = PS3000_CHANNEL_A;
	unitOpened.trigger.simple.delay  = -10;
	unitOpened.trigger.simple.direction = PS3000_RISING;
	unitOpened.trigger.simple.threshold = 1500;

	ps3000_set_trigger ( unitOpened.handle,
		unitOpened.trigger.simple.channel,
		mv_to_adc (unitOpened.trigger.simple.threshold, unitOpened.channelSettings[unitOpened.trigger.simple.channel].range),
		unitOpened.trigger.simple.direction,
		(short) unitOpened.trigger.simple.delay,
		auto_trigger_ms );

	/* Enable ETS in fast mode,
	* the computer will store 20 cycles
	*  but interleave only 10
	*/
	ets_sampletime = ps3000_set_ets ( unitOpened.handle, PS3000_ETS_FAST, 100, 10 );
	printf ( "ETS Sample Time is: %ld\n", ets_sampletime );
	/* Start it collecting,
	*  then wait for completion
	*/
	ps3000_run_block ( unitOpened.handle, BUFFER_SIZE, timebase, 1, &time_indisposed_ms );

	printf ( "Waiting for trigger..." );
	printf ( "Press a key to abort\n" );

	while ( (!ps3000_ready (unitOpened.handle)) && (!kbhit ()) )
	{
		Sleep (100);
	}

	if ( kbhit () )
	{
		getch ();
		printf ( "data collection aborted\n" );
	}
	else
	{
		/* Get the times (in microseconds)
		*  and the values (in ADC counts)
		*/
		ps3000_get_times_and_values (unitOpened.handle,
			times,
			unitOpened.channelSettings[PS3000_CHANNEL_A].values,
			unitOpened.channelSettings[PS3000_CHANNEL_B].values,
			unitOpened.channelSettings[PS3000_CHANNEL_C].values,
			unitOpened.channelSettings[PS3000_CHANNEL_D].values,
			&overflow,
			1,
			BUFFER_SIZE);   

		/* Print out the first 10 readings,
		*  converting the readings to mV if required
		*/

		printf ("Ten readings around trigger\n");
		printf ("Time\tValue\n");
		printf ("(%s)\t", scaled_time_units (1));
		for (j = 0; j < unitOpened.noOfChannels; j++)
		{
			if(unitOpened.channelSettings[j].enabled)
			{
				printf ( "(%s)\t", scale_to_mv ? "mV" : "ADC" );				
			}
		}
		printf("\n");

		/* This calculation is correct for 10% pre-trigger
		*/
		trigger_sample = BUFFER_SIZE / 10;

		for ( i = trigger_sample - 5; i < trigger_sample + 5; i++ )
		{
			printf ( "%ld\t", times[i]);
			for (j = 0; j < unitOpened.noOfChannels; j++)
			{
				if(unitOpened.channelSettings[j].enabled)
				{
					printf ( "%d\t", adc_to_mv ( unitOpened.channelSettings[j].values[i], unitOpened.channelSettings[j].range) );
				}
			}
			printf("\n");
		}

		fp = fopen ( "data.txt","w" );
		if (fp != NULL)
		{
			for ( i = 0; i < BUFFER_SIZE; i++ )
			{
				fprintf ( fp, "%ld, ", times[i]);
				for (j = 0; j < unitOpened.noOfChannels; j++)
				{
					if(unitOpened.channelSettings[j].enabled)
					{
						fprintf ( fp, "%d, %d\n", unitOpened.channelSettings[j].values[i], adc_to_mv (unitOpened.channelSettings[j].values[i], unitOpened.channelSettings[j].range) );
					}
				}
				fprintf ( fp, "\n");
			}
			fclose( fp );
		}
		else
			printf("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
	}

	// You may now call ps3000_get_times_and_values to get the next set of ets data
	// Once you have finshed collecting data, you need to call ps3000_stop to stop
	// ets mode.
	ps3000_stop(unitOpened.handle);

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
 * Each call to ps3000_get_times_and_values returns the readings since the
 * last call
 *
 * The time is in microseconds: it will wrap around at 2^32 (approx 2,000 seconds)
 * if you don't need the time, you can just call ps3000_get_values
 *
 ****************************************************************************/

void collect_streaming (void)
  {
  int		i, j;
  int		block_no;
  FILE 	*fp;
  int		no_of_values;
  short  overflow;
  int 	ok;

  printf ( "Collect streaming...\n" );
  printf ( "Data is written to disk file (data.txt)\n" );
  printf ( "Press a key to start\n" );
  getch ();

  set_defaults ();

  /* You cannot use simple triggering for the start of the data...
   */
  ps3000_set_trigger ( unitOpened.handle, PS3000_NONE, 0, 0, 0, 0 );

  /* Collect data at 10ms intervals
   * Max BUFFER_SIZE points on each call
   *  (buffer must be big enough for max time between calls
   *
   *  Start it collecting,
   *  then wait for trigger event
   */
  ok = ps3000_run_streaming ( unitOpened.handle, 10, 1000, 0 );
  printf ( "OK: %d\n", ok );

  /* From here on, we can get data whenever we want...
   */
  block_no = 0;
  fp = fopen ( "data.txt", "w" );

  if (fp != NULL)
  {
	for (j = 0; j < unitOpened.noOfChannels; j++)
	{
		if(unitOpened.channelSettings[j].enabled)
		{
			fprintf (fp,  "(%s)\t", scale_to_mv ? "mV" : "ADC" );				
		}
	}
	fprintf (fp,"\n");
  }
  else
 	printf("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");

  while ( !kbhit () )
  {
    no_of_values = ps3000_get_values ( unitOpened.handle, 
																			 unitOpened.channelSettings[PS3000_CHANNEL_A].values,
																			 unitOpened.channelSettings[PS3000_CHANNEL_B].values,
																			 unitOpened.channelSettings[PS3000_CHANNEL_C].values,
																			 unitOpened.channelSettings[PS3000_CHANNEL_D].values,
																			 &overflow,
																			 BUFFER_SIZE );
    printf ( "%d values\n", no_of_values );

    if ( block_no++ > 20 )
      {
      block_no = 0;
      printf ( "Press any key to stop\n" );
      }

    /* Print out the readings
     */
     
    if (fp != NULL)
    {
	for ( i = 0; i < no_of_values; i++ )
	{
	for (j = 0; j < unitOpened.noOfChannels; j++)
	{
		if(unitOpened.channelSettings[j].enabled)
		{
						fprintf (fp, "%d\t", adc_to_mv ( unitOpened.channelSettings[j].values[i], unitOpened.channelSettings[j].range) );
		}
	}
	fprintf (fp, "\n");
	}    
    }
    else
 	printf("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");

    /* Wait 100ms before asking again
     */
    Sleep ( 100 );
    }
	
  if (fp != NULL)
  	fclose ( fp );

  ps3000_stop ( unitOpened.handle );

  getch ();
  }

void collect_fast_streaming (void)
{
	unsigned long	i;
	FILE 	*fp;
	short  overflow;
	int 	ok;
	short ch;
	unsigned long nPreviousValues = 0;
	short values_a[BUFFER_SIZE_STREAMING];
	short values_b[BUFFER_SIZE_STREAMING];
	short values_c[BUFFER_SIZE_STREAMING];
	short values_d[BUFFER_SIZE_STREAMING];
	unsigned long	triggerAt;
	short triggered;
	unsigned long no_of_samples;
	double startTime = 0;


	printf ( "Collect fast streaming...\n" );
	printf ( "Data is written to disk file (data.txt)\n" );
	printf ( "Press a key to start\n" );
	getch ();

	set_defaults ();

	/* You cannot use triggering for the start of the data...
	*/
	ps3000_set_trigger ( unitOpened.handle, PS3000_NONE, 0, 0, 0, 0 );

	unitOpened.trigger.advanced.autoStop = 0;
	unitOpened.trigger.advanced.totalSamples = 0;
	unitOpened.trigger.advanced.triggered = 0;

	/* Collect data at 10us intervals
	* 100000 points with an agregation of 100 : 1
	*	Auto stop after the 100000 samples
	*  Start it collecting,
	*/
	ok = ps3000_run_streaming_ns ( unitOpened.handle, 10, PS3000_US, BUFFER_SIZE_STREAMING, 1, 100, 30000 );
	printf ( "OK: %d\n", ok );

	/* From here on, we can get data whenever we want...
	*/	
	
	while (!unitOpened.trigger.advanced.autoStop)
	{
		ps3000_get_streaming_last_values (unitOpened.handle, ps3000FastStreamingReady);
		if (nPreviousValues != unitOpened.trigger.advanced.totalSamples)
		{
			printf ("Values collected: %ld\n", unitOpened.trigger.advanced.totalSamples - nPreviousValues);
			nPreviousValues = 	unitOpened.trigger.advanced.totalSamples;
		}
		Sleep (0);
	}

	ps3000_stop (unitOpened.handle);

	no_of_samples = ps3000_get_streaming_values_no_aggregation (unitOpened.handle,
																															&startTime, // get samples from the beginning
																															values_a, // set buffer for channel A
																															values_b,	// set buffer for channel B
																															NULL,
																															NULL,
																															&overflow,
																															&triggerAt,
																															&triggered,
																															BUFFER_SIZE_STREAMING);


	// print out the first 20 readings
	for ( i = 0; i < 20; i++ )
	{
		for (ch = 0; ch < unitOpened.noOfChannels; ch++)
		{
			if (unitOpened.channelSettings[ch].enabled)
			{
				switch (ch)
				{
				case 0:
					printf ("%d, ", adc_to_mv (values_a[i], unitOpened.channelSettings[ch].range) );
					break;

				case 1:
					printf ("%d, ", adc_to_mv (values_b[i], unitOpened.channelSettings[ch].range) );
					break;

				case 2:
					printf ("%d, ", adc_to_mv (values_c[i], unitOpened.channelSettings[ch].range) );
					break;

				case 3:
					printf ("%d, ", adc_to_mv (values_d[i], unitOpened.channelSettings[ch].range) );
					break;
				}

			}
		}
			printf ("\n");
	}

	fp = fopen ( "data.txt", "w" );

	if (fp != NULL)
	{
		for ( i = 0; i < no_of_samples; i++ )
		{
			for (ch = 0; ch < unitOpened.noOfChannels; ch++)
			{
				if (unitOpened.channelSettings[ch].enabled)
				{
					switch (ch)
					{
					case 0:
					fprintf ( fp, "%d, ", adc_to_mv (values_a[i], unitOpened.channelSettings[ch].range) );
					break;
	
					case 1:
					fprintf ( fp, "%d, ", adc_to_mv (values_b[i], unitOpened.channelSettings[ch].range) );
					break;
	
					case 2:
					fprintf ( fp, "%d, ", adc_to_mv (values_c[i], unitOpened.channelSettings[ch].range) );
					break;
	
					case 3:
					fprintf ( fp, "%d, ", adc_to_mv (values_d[i], unitOpened.channelSettings[ch].range) );
					break;
					}
				}
			}
			fprintf (fp, "\n");
		}
		fclose ( fp );
	}
  	else
 		printf("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");

	getch ();
}

void collect_fast_streaming_triggered (void)
{
	unsigned long	i;
	FILE 	*fp;
	short  overflow;
	int 	ok;
	short ch;
	unsigned long nPreviousValues = 0;
	short values_a[BUFFER_SIZE_STREAMING];
	short values_b[BUFFER_SIZE_STREAMING];
	unsigned long	triggerAt;
	short triggered;
	unsigned long no_of_samples;
	double startTime = 0;



	printf ( "Collect fast streaming triggered...\n" );
	printf ( "Data is written to disk file (data.txt)\n" );
	printf ( "Press a key to start\n" );
	getch ();

	set_defaults ();

	set_trigger_advanced ();

	unitOpened.trigger.advanced.autoStop = 0;
	unitOpened.trigger.advanced.totalSamples = 0;
	unitOpened.trigger.advanced.triggered = 0;

	/* Collect data at 10us intervals
	* 100000 points with an agregation of 100 : 1
	*	Auto stop after the 100000 samples
	*  Start it collecting,
	*/
	ok = ps3000_run_streaming_ns ( unitOpened.handle, 10, PS3000_US, BUFFER_SIZE_STREAMING, 1, 100, 30000 );
	printf ( "OK: %d\n", ok );

	/* From here on, we can get data whenever we want...
	*/	
	
	while (!unitOpened.trigger.advanced.autoStop)
	{
		ps3000_get_streaming_last_values (unitOpened.handle, ps3000FastStreamingReady);
		if (nPreviousValues != unitOpened.trigger.advanced.totalSamples)
		{
			printf ("Values collected: %ld\n", unitOpened.trigger.advanced.totalSamples - nPreviousValues);
			nPreviousValues = 	unitOpened.trigger.advanced.totalSamples;
		}
		Sleep (0);
	}

	ps3000_stop (unitOpened.handle);

	no_of_samples = ps3000_get_streaming_values_no_aggregation (unitOpened.handle,
																														 &startTime, // get samples from the beginning
																															values_a, // set buffer for channel A
																															values_b,	// set buffer for channel B
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
		for (ch = 0; ch < unitOpened.noOfChannels; ch++)
		{
			if (unitOpened.channelSettings[ch].enabled)
			{
				printf ("%d, ", adc_to_mv ((!ch ? values_a[i] : values_b[i]), unitOpened.channelSettings[ch].range) );
			}
		}
		printf ("\n");
	}

	fp = fopen ( "data.txt", "w" );

	if (fp != NULL)
	{
		for ( i = 0; i < no_of_samples; i++ )
		{
			for (ch = 0; ch < unitOpened.noOfChannels; ch++)
			{
				if (unitOpened.channelSettings[ch].enabled)
				{
					fprintf ( fp, "%d, ", adc_to_mv ((!ch ? values_a[i] : values_b[i]), unitOpened.channelSettings[ch].range) );
				}
			}
			fprintf (fp, "\n");
		}
		fclose ( fp );
	}
  	else
 		printf("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");

	getch ();
}



/****************************************************************************
 *
 *
 ****************************************************************************/
void get_info (void)
  {
  char description [6][25]=  { "Driver Version","USB Version","Hardware Version",
                              "Variant Info","Serial", "Error Code" };
  short 		i;
  char		line [80];
	int    variant;

  if( unitOpened.handle )
  {
    for ( i = 0; i < 5; i++ )
    {
      ps3000_get_unit_info ( unitOpened.handle, line, sizeof (line), i );
			if (i == 3)
			{
			  variant = atoi(line);
			}
      printf ( "%s: %s\n", description[i], line );
    }

  	switch (variant)
		{
		case MODEL_PS3206:
			unitOpened.model = MODEL_PS3206;
			unitOpened.external = TRUE;
			unitOpened.signalGenerator = TRUE;
			unitOpened.firstRange = PS3000_100MV;
			unitOpened.lastRange = PS3000_20V;
			unitOpened.maxTimebases = PS3206_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebases;
			unitOpened.noOfChannels = DUAL_SCOPE;
			unitOpened.hasAdvancedTriggering = FALSE;
			unitOpened.hasEts = TRUE;
			unitOpened.hasFastStreaming = FALSE;
		break;

		case MODEL_PS3205:
			unitOpened.model = MODEL_PS3205;
			unitOpened.external = TRUE;
			unitOpened.signalGenerator = TRUE;
			unitOpened.firstRange = PS3000_100MV;
			unitOpened.lastRange = PS3000_20V;
			unitOpened.maxTimebases = PS3205_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebases;
			unitOpened.noOfChannels = DUAL_SCOPE; 
			unitOpened.hasAdvancedTriggering = FALSE;
			unitOpened.hasEts = TRUE;
			unitOpened.hasFastStreaming = FALSE;
		break;
				
		case MODEL_PS3204:
			unitOpened.model = MODEL_PS3204;
			unitOpened.external = TRUE;
			unitOpened.signalGenerator = TRUE;
	  	unitOpened.firstRange = PS3000_100MV;
			unitOpened.lastRange = PS3000_20V;
			unitOpened.maxTimebases = PS3204_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebases;
			unitOpened.noOfChannels = DUAL_SCOPE;
			unitOpened.hasAdvancedTriggering = FALSE;
			unitOpened.hasEts = TRUE;
			unitOpened.hasFastStreaming = FALSE;
		break;
				
		case MODEL_PS3223:
			unitOpened.model = MODEL_PS3223;
			unitOpened.external = FALSE;
			unitOpened.signalGenerator = FALSE;
			unitOpened.firstRange = PS3000_20MV;
			unitOpened.lastRange = PS3000_20V;
			unitOpened.maxTimebases = PS3224_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebases;
			unitOpened.noOfChannels = DUAL_SCOPE;
			unitOpened.hasAdvancedTriggering = TRUE;
			unitOpened.hasEts = FALSE;
			unitOpened.hasFastStreaming = TRUE;
		break;

		case MODEL_PS3423:
			unitOpened.model = MODEL_PS3423;
			unitOpened.external = FALSE;
			unitOpened.signalGenerator = FALSE;
			unitOpened.firstRange = PS3000_20MV;
			unitOpened.lastRange = PS3000_20V;
			unitOpened.maxTimebases = PS3424_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebases;
			unitOpened.noOfChannels = QUAD_SCOPE;					
			unitOpened.hasAdvancedTriggering = TRUE;
			unitOpened.hasEts = FALSE;
			unitOpened.hasFastStreaming = TRUE;
		break;

		case MODEL_PS3224:
			unitOpened.model = MODEL_PS3224;
			unitOpened.external = FALSE;
			unitOpened.signalGenerator = FALSE;
			unitOpened.firstRange = PS3000_20MV;
			unitOpened.lastRange = PS3000_20V;
			unitOpened.maxTimebases = PS3224_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebases;
			unitOpened.noOfChannels = DUAL_SCOPE;
			unitOpened.hasAdvancedTriggering = TRUE;
			unitOpened.hasEts = FALSE;
			unitOpened.hasFastStreaming = TRUE;
		break;

		case MODEL_PS3424:
			unitOpened.model = MODEL_PS3424;
			unitOpened.external = FALSE;
			unitOpened.signalGenerator = FALSE;
			unitOpened.firstRange = PS3000_20MV;
			unitOpened.lastRange = PS3000_20V;
			unitOpened.maxTimebases = PS3424_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebases;
			unitOpened.noOfChannels = QUAD_SCOPE;		
			unitOpened.hasAdvancedTriggering = TRUE;
			unitOpened.hasEts = FALSE;
			unitOpened.hasFastStreaming = TRUE;
		break;

		case MODEL_PS3225:
			unitOpened.model = MODEL_PS3225;
			unitOpened.external = FALSE;
			unitOpened.signalGenerator = FALSE;
			unitOpened.firstRange = PS3000_100MV;
			unitOpened.lastRange = PS3000_400V;
			unitOpened.maxTimebases = PS3225_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebases;
			unitOpened.noOfChannels = DUAL_SCOPE;
			unitOpened.hasAdvancedTriggering = TRUE;
			unitOpened.hasEts = FALSE;
			unitOpened.hasFastStreaming = TRUE;
		break;

		case MODEL_PS3425:
			unitOpened.model = MODEL_PS3425;
			unitOpened.external = FALSE;
			unitOpened.signalGenerator = FALSE;
			unitOpened.firstRange = PS3000_100MV;
			unitOpened.lastRange = PS3000_400V;
			unitOpened.timebases = PS3425_MAX_TIMEBASE;
			unitOpened.noOfChannels = QUAD_SCOPE;					
			unitOpened.hasAdvancedTriggering = TRUE;
			unitOpened.hasEts = FALSE;
			unitOpened.hasFastStreaming = TRUE;
		break;

 		default:
			printf("Unit not supported");
		}

		unitOpened.channelSettings [PS3000_CHANNEL_A].enabled = 1;
		unitOpened.channelSettings [PS3000_CHANNEL_A].DCcoupled = 1;
		unitOpened.channelSettings [PS3000_CHANNEL_A].range = unitOpened.lastRange;

		unitOpened.channelSettings [PS3000_CHANNEL_B].enabled = 0;
		unitOpened.channelSettings [PS3000_CHANNEL_B].DCcoupled = 1;
		unitOpened.channelSettings [PS3000_CHANNEL_B].range = unitOpened.lastRange;

		unitOpened.channelSettings [PS3000_CHANNEL_C].enabled = 0;
		unitOpened.channelSettings [PS3000_CHANNEL_C].DCcoupled = 1;
		unitOpened.channelSettings [PS3000_CHANNEL_C].range = unitOpened.lastRange;

		unitOpened.channelSettings [PS3000_CHANNEL_D].enabled = 0;
		unitOpened.channelSettings [PS3000_CHANNEL_D].DCcoupled = 1;
		unitOpened.channelSettings [PS3000_CHANNEL_D].range = unitOpened.lastRange;
  }
  else
  {
    printf ( "Unit Not Opened\n" );
    ps3000_get_unit_info ( unitOpened.handle, line, sizeof (line), PS3000_ERROR_CODE );
    printf ( "%s: %s\n", description[5], line );
		unitOpened.model = MODEL_NONE;
		unitOpened.external = TRUE;
		unitOpened.signalGenerator = TRUE;
		unitOpened.firstRange = PS3000_100MV;
		unitOpened.lastRange = PS3000_20V;
		unitOpened.timebases = PS3206_MAX_TIMEBASE;
		unitOpened.noOfChannels = QUAD_SCOPE;	
	}
}

/****************************************************************************
 *
 * Select timebase, set oversample to on and time units as nano seconds
 *
 ****************************************************************************/
void set_timebase (void)
  {
  short	i;
  long   time_interval;
  short  time_units;
  short 	oversample;
  long   max_samples;

  printf ( "Specify timebase: \n" );

  /* See what ranges are available...
   */
	oversample = 1;
  for (i = 0; i < unitOpened.maxTimebases; i++)
    {
    ps3000_get_timebase ( unitOpened.handle, i, BUFFER_SIZE, &time_interval, &time_units, oversample, &max_samples );
    if ( time_interval > 0 )
      {
        printf ( "%d -> %ldns\n", i, time_interval);
      }
    }

  /* Ask the user to select a timebase
   */
  printf ( "Timebase: " );
  do
  {
    fflush( stdin );
    scanf ( "%hd", &timebase );
  } while ( (timebase < 0) || (timebase >= unitOpened.timebases) );
  ps3000_get_timebase ( unitOpened.handle, i, BUFFER_SIZE, &time_interval, &time_units, oversample, &max_samples );
  printf ( "Timebase %d - %ld ns\n", timebase, time_interval );
  }

/****************************************************************************
 * Select input voltage ranges for channels A and B
 ****************************************************************************/
void set_voltages (void)
{
  int		i;
  int		ch;

  /* See what ranges are available...
   */
  for ( i = unitOpened.firstRange; i <= unitOpened.lastRange; i++ )
  {
    printf ( "%d -> %ld mV\n", i, input_ranges[i] );
  }

  /* Ask the user to select a range
   */
	
  printf ( "Specify voltage range (%d..%d)\n", unitOpened.firstRange, (unitOpened.lastRange) );
	printf ( "99 - switches channel off\n");
  for ( ch = 0; ch < unitOpened.noOfChannels; ch++ )
  {
    printf ( "\nChannel %c: ", 'A' + ch );
    do
    {
       fflush( stdin );
       scanf ( "%hd", &unitOpened.channelSettings[ch].range);
    } while ( unitOpened.channelSettings[ch].range != 99 && (unitOpened.channelSettings[ch].range < unitOpened.firstRange || unitOpened.channelSettings[ch].range  > unitOpened.lastRange) );
		if(unitOpened.channelSettings[ch].range != 99)
		{
      printf ( " - %ld mV\n", input_ranges[unitOpened.channelSettings[ch].range]);
      unitOpened.channelSettings[ch].enabled = TRUE;
		}
		else
		{
      printf ( "Channel Switched off\n");
      unitOpened.channelSettings[ch].enabled = FALSE;
    }
  }
}	


/****************************************************************************
 * Toggles the signal generator to 1kHz sine wave or off
 ***************************************************************************/
void set_signal_generator (void)
  {
  short waveform;
  char  sweep;
  long 	sig_gen_finish;
  short repeat, dwell_time, dual_slope;
  float 	increment;

  increment = repeat = dwell_time = dual_slope = 0;

  // Ask user to enter sig frequency;
  printf ( "Enter frequency in Hz: " );
  do
    {
    scanf( "%lu", &sig_gen_frequency );
    } while ( sig_gen_frequency < 0 || sig_gen_frequency > PS3000_MAX_SIGGEN_FREQ );

  if ( sig_gen_frequency > 0 )
    {
    printf ( "Signal generator On" );
    // Ask user to enter type of signal
    printf ( "Enter type of waveform\n" );
    printf ( "0:\tSQUARE\n" );
  	 printf ( "1:\tTRIANGLE\n" );
    printf ( "2:\tSINE\n" );
    do
      {
      scanf ( "%hd", &waveform );
      } while ( waveform < 0 || waveform >= PS3000_MAX_WAVE_TYPES );

    printf ( "Use sweep mode:" );
    fflush ( stdin );
    scanf ( "%c", &sweep );
    fflush ( stdin );
    sweep = toupper ( sweep );

    if (sweep == 'Y')
      {
      printf ( "Enter finish frequency (Hz) " );
      scanf ( "%lu", &sig_gen_finish );
      printf ( "Enter increment " );
      scanf ( "%f", &increment );
      printf ( "Enter dwell time (ns) " );
      scanf ( "%hd", &dwell_time );
      printf ( "Repeat sweep (0-No, 1-Yes) " );

      scanf ( "%hd", &repeat );
      printf ( "Enter dual slope (0-No, 1-Yes) " );

      scanf ( "%hd", &dual_slope );
      }
	 else
      sig_gen_finish = sig_gen_frequency;
    }
  else
    {
    waveform = 0;
    printf ( "Signal generator Off" );
    }

  printf ( "Actual frequency generated is %lu Hz\n", ps3000_set_siggen ( unitOpened.handle,
                                                                        waveform,
  		                                                                  sig_gen_frequency,
                                                                        sig_gen_finish,
                                                                        increment,
                                                                        dwell_time,
                                                                        repeat,
                                                                        dual_slope) );
  }


/****************************************************************************
 *
 *
 ****************************************************************************/

int main (void)
{
	char	ch;

	printf ( "PS3000 driver example program\n" );
	printf ( "Version 1.1\n\n" );

	printf ( "\n\nOpening the device...\n");

	//open unit and show splash screen
	unitOpened.handle = ps3000_open_unit ();
	printf ( "Handle: %d\n", unitOpened.handle );
	if ( unitOpened.handle < 1)
	{
		printf ( "Unable to open device\n" );
		get_info ();
		while( !kbhit() );
		exit ( 99 );
	}
	else
	{
		printf ( "Device opened successfully\n\n" );
		get_info ();

		timebase = 0;
		ch = ' ';
		while ( ch != 'X' )
		{
			printf ( "\n" );
			printf ( "B - immediate block		          V - Set voltages\n" );
			printf ( "T - triggered block	            I - Set timebase\n" );
			printf ( "Y - advanced triggered block    A - ADC counts/mV\n" );
			printf ( "E - ETS block\n" );
			printf ( "S - Streaming\n" );
			printf ( "F - Fast streaming\n");
			printf ( "D - Fast streaming triggered\n" );
			printf ( "G - Toggle signal generator on/off\n" );
			printf ( "X - exit\n" );
			printf ( "Operation:" );

			ch = toupper ( getch () );

			printf ( "\n\n" );
			switch ( ch )
			{
			case 'B':
				collect_block_immediate ();
				break;

			case 'T':
				collect_block_triggered ();
				break;

			case 'Y':
				if (unitOpened.hasAdvancedTriggering)
				{
					collect_block_advanced_trigger ();
				}
				else
				{
					printf ("Not supported by this model\n\n");
				}
				break;


			case 'S':
				collect_streaming ();
				break;

			case 'F':
				if (unitOpened.hasFastStreaming)
				{
					collect_fast_streaming ();
				}
				else
				{
					printf ("Not supported by this model\n\n");
				}
				break;

			case 'D':
				if (unitOpened.hasFastStreaming && unitOpened.hasAdvancedTriggering)
				{
					collect_fast_streaming_triggered ();
				}
				else
				{
					printf ("Not supported by this model\n\n");
				}
				break;

			case 'G':
				set_signal_generator ();
				break;

			case 'E':
				collect_block_ets ();
				break;

			case 'V':
				set_voltages ();
				break;

			case 'I':
				set_timebase ();
				break;

			case 'A':
				scale_to_mv = !scale_to_mv;
				if ( scale_to_mv )
				{
					printf ( "Readings will be scaled in mV\n" );
				}
				else
				{
					printf ( "Readings will be scaled in ADC counts\n" );
				}
				break;

			case 'X':
				/* Handled by outer loop */
				break;

			default:
				printf ( "Invalid operation\n" );
				break;
			}
		}

		ps3000_close_unit ( unitOpened.handle );
	}
	return 1;
}
