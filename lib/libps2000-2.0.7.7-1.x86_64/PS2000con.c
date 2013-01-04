/**************************************************************************
 *
 * Filename:    PS2000con.c
 *
 * Copyright:   Pico Technology Limited 2006-8
 *
 * Author:      MAS
 *
 * Description:
 *   This is a console-mode program that demonstrates how to use the
 *   PS2000 driver.
 *
 * Examples:
 *    Collect a block of samples immediately
 *    Collect a block of samples when a trigger event occurs
 *    Collect a block of samples using an advanced trigger
 *			- PS2203, PS2204, PS2205 only
 *    Collect a block using ETS
 *			PS2104, PS2105, PS2203, PS2204, and PS2205
 *    Collect a stream of data
 *    Collect a stream of data using an advanced trigger
 *			- PS2203, PS2204, PS2205 only
 *    Set the signal generator with the built in signals 
 *			- PS2203, PS2204, and PS2205 only
 *		Set the signal generator with an arbitrary signal 
 *			- PS2203, PS2204, and PS2205 only
 *		
 *
 *	To build this application
 *		Windows: set up a project for a 32-bit console mode application
 *			 add this file to the project
 *			 add PS2000bc.lib to the project (Borland C only)
 *			 add PS2000.lib to the project (Microsoft C only)
 *			 add PS2000.h to the project
 *			 then build the project
 *
 *		Linux: gcc -lps2000 PS2000con.c -oPS2000con
 *		       ./PS2000con
 *
 *
 ***************************************************************************/

#ifdef WIN32
/* Headers for Windows */
#include "windows.h"
#include <conio.h>
#include <stdio.h>

/* Definitions of PS2000 driver routines on Windows*/
#include "PS2000.h"

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

/* Definition of PS2000 driver routines on Linux */
#include <libps2000/ps2000.h>

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

short  	values_a [BUFFER_SIZE];
short    overflow;
int		scale_to_mv = 1;

short		channel_mv [PS2000_MAX_CHANNELS];
short		timebase = 8;


typedef enum {
	MODEL_NONE = 0,
  MODEL_PS2104 = 2104,
	MODEL_PS2105 = 2105,
	MODEL_PS2202 = 2202,
	MODEL_PS2203 = 2203,
	MODEL_PS2204 = 2204,
	MODEL_PS2205 = 2205
} MODEL_TYPE;

typedef struct
{
	PS2000_THRESHOLD_DIRECTION	channelA;
	PS2000_THRESHOLD_DIRECTION	channelB;
	PS2000_THRESHOLD_DIRECTION	channelC;
	PS2000_THRESHOLD_DIRECTION	channelD;
	PS2000_THRESHOLD_DIRECTION	ext;
} DIRECTIONS;

typedef struct
{
	PS2000_PWQ_CONDITIONS					*	conditions;
	short														nConditions;
	PS2000_THRESHOLD_DIRECTION		  direction;
	unsigned long										lower;
	unsigned long										upper;
	PS2000_PULSE_WIDTH_TYPE					type;
} PULSE_WIDTH_QUALIFIER;


typedef struct
{
	PS2000_CHANNEL channel;
	float threshold;
	short direction;
	float delay;
} SIMPLE;

typedef struct
{
	short hysterisis;
	DIRECTIONS directions;
	short nProperties;
	PS2000_TRIGGER_CONDITIONS * conditions;
	PS2000_TRIGGER_CHANNEL_PROPERTIES * channelProperties;
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
  PS2000_RANGE firstRange;
	PS2000_RANGE lastRange;	
	TRIGGER_CHANNEL trigger;
	short maxTimebase;
	short timebases;
	short noOfChannels;
	CHANNEL_SETTINGS channelSettings[PS2000_MAX_CHANNELS];
	short				hasAdvancedTriggering;
	short				hasFastStreaming;
	short				hasEts;
	short				hasSignalGenerator;
} UNIT_MODEL; 

UNIT_MODEL unitOpened;

long times[BUFFER_SIZE];

short input_ranges [PS2000_MAX_RANGES] = {10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};

void  __stdcall ps2000FastStreamingReady( short **overviewBuffers,
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
 * adc_units
 *
 ****************************************************************************/
char * adc_units (short time_units)
  {
  time_units++;
  printf ( "time unit:  %d\n", time_units ) ;
  switch ( time_units )
    {
    case 0:
      return "ADC";
    case 1:
      return "fs";
    case 2:
      return "ps";
    case 3:
      return "ns";
    case 4:
      return "us";
    case 5:
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
  return ( ( mv * 32767 ) / input_ranges[ch] );
  }

/****************************************************************************
 * set_defaults - restore default settings
 ****************************************************************************/
void set_defaults (void)
{
	short ch = 0;
  ps2000_set_ets ( unitOpened.handle, PS2000_ETS_OFF, 0, 0 );

	for (ch = 0; ch < unitOpened.noOfChannels; ch++)
	{
		ps2000_set_channel ( unitOpened.handle,
			                   ch,
												 unitOpened.channelSettings[ch].enabled ,
												 unitOpened.channelSettings[ch].DCcoupled ,
												 unitOpened.channelSettings[ch].range);
	}
}

void set_trigger_advanced(void)
{
	short ok = 0;
  short 	auto_trigger_ms = 0;

	// to trigger of more than one channel set this parameter to 2 or more
	// each condition can only have on parameter set to PS2000_CONDITION_TRUE or PS2000_CONDITION_FALSE
	// if more than on condition is set then it will trigger off condition one, or condition two etc.
	unitOpened.trigger.advanced.nProperties = 1;
	// set the trigger channel to channel A by using PS2000_CONDITION_TRUE
	unitOpened.trigger.advanced.conditions = malloc (sizeof (PS2000_TRIGGER_CONDITIONS) * unitOpened.trigger.advanced.nProperties);
	unitOpened.trigger.advanced.conditions->channelA = PS2000_CONDITION_TRUE;
	unitOpened.trigger.advanced.conditions->channelB = PS2000_CONDITION_DONT_CARE;
	unitOpened.trigger.advanced.conditions->channelC = PS2000_CONDITION_DONT_CARE;
	unitOpened.trigger.advanced.conditions->channelD = PS2000_CONDITION_DONT_CARE;
	unitOpened.trigger.advanced.conditions->external = PS2000_CONDITION_DONT_CARE;
	unitOpened.trigger.advanced.conditions->pulseWidthQualifier = PS2000_CONDITION_DONT_CARE;

	// set channel A to rising
	// the remainder will be ignored as only a condition is set for channel A
	unitOpened.trigger.advanced.directions.channelA = PS2000_ADV_RISING;
	unitOpened.trigger.advanced.directions.channelB = PS2000_ADV_RISING;
	unitOpened.trigger.advanced.directions.channelC = PS2000_ADV_RISING;
	unitOpened.trigger.advanced.directions.channelD = PS2000_ADV_RISING;
	unitOpened.trigger.advanced.directions.ext = PS2000_ADV_RISING;


	unitOpened.trigger.advanced.channelProperties = malloc (sizeof (PS2000_TRIGGER_CHANNEL_PROPERTIES) * unitOpened.trigger.advanced.nProperties);
	// there is one property for each condition
	// set channel A
	// trigger level 1500 adc counts the trigger point will vary depending on the voltage range
	// hysterisis 4096 adc counts  
	unitOpened.trigger.advanced.channelProperties->channel = (short) PS2000_CHANNEL_A;
	unitOpened.trigger.advanced.channelProperties->thresholdMajor = 1500;
	// not used in level triggering, should be set when in window mode
	unitOpened.trigger.advanced.channelProperties->thresholdMinor = 0;
	// used in level triggering, not used when in window mode
	unitOpened.trigger.advanced.channelProperties->hysteresis = (short) 4096;
	unitOpened.trigger.advanced.channelProperties->thresholdMode = PS2000_LEVEL;

	ok = ps2000SetAdvTriggerChannelConditions (unitOpened.handle, unitOpened.trigger.advanced.conditions, unitOpened.trigger.advanced.nProperties);
	ok = ps2000SetAdvTriggerChannelDirections (unitOpened.handle,
																				unitOpened.trigger.advanced.directions.channelA,
																				unitOpened.trigger.advanced.directions.channelB,
																				unitOpened.trigger.advanced.directions.channelC,
																				unitOpened.trigger.advanced.directions.channelD,
																				unitOpened.trigger.advanced.directions.ext);
	ok = ps2000SetAdvTriggerChannelProperties (unitOpened.handle,
																				unitOpened.trigger.advanced.channelProperties,
																				unitOpened.trigger.advanced.nProperties,
																				auto_trigger_ms);


	// remove comments to try triggering with a pulse width qualifier
	// add a condition for the pulse width eg. in addition to the channel A or as a replacement
	//unitOpened.trigger.advanced.pwq.conditions = malloc (sizeof (PS2000_PWQ_CONDITIONS));
	//unitOpened.trigger.advanced.pwq.conditions->channelA = PS2000_CONDITION_TRUE;
	//unitOpened.trigger.advanced.pwq.conditions->channelB = PS2000_CONDITION_DONT_CARE;
	//unitOpened.trigger.advanced.pwq.conditions->channelC = PS2000_CONDITION_DONT_CARE;
	//unitOpened.trigger.advanced.pwq.conditions->channelD = PS2000_CONDITION_DONT_CARE;
	//unitOpened.trigger.advanced.pwq.conditions->external = PS2000_CONDITION_DONT_CARE;
	//unitOpened.trigger.advanced.pwq.nConditions = 1;

	//unitOpened.trigger.advanced.pwq.direction = PS2000_RISING;
	//unitOpened.trigger.advanced.pwq.type = PS2000_PW_TYPE_LESS_THAN;
	//// used when type	PS2000_PW_TYPE_IN_RANGE,	PS2000_PW_TYPE_OUT_OF_RANGE
	//unitOpened.trigger.advanced.pwq.lower = 0;
	//unitOpened.trigger.advanced.pwq.upper = 10000;
	//ps2000SetPulseWidthQualifier (unitOpened.handle,
	//															unitOpened.trigger.advanced.pwq.conditions,
	//															unitOpened.trigger.advanced.pwq.nConditions, 
	//															unitOpened.trigger.advanced.pwq.direction,
	//															unitOpened.trigger.advanced.pwq.lower,
	//															unitOpened.trigger.advanced.pwq.upper,
	//															unitOpened.trigger.advanced.pwq.type);

	ok = ps2000SetAdvTriggerDelay (unitOpened.handle, 0, -10); 
}

/****************************************************************************
 * Collect_block_immediate
 *  this function demonstrates how to collect a single block of data
 *  from the unit (start collecting immediately)
 ****************************************************************************/
void collect_block_immediate (void)
{
  int		i;
  long 	time_interval;
  short 	time_units;
  short 	oversample;
  long 	no_of_samples = BUFFER_SIZE;
  FILE 	*fp;
  short 	auto_trigger_ms = 0;
  long 	time_indisposed_ms;
  short 	overflow;
  long 	max_samples;
	short ch = 0;

  printf ( "Collect block immediate...\n" );
  printf ( "Press a key to start\n" );
  getch ();

  set_defaults ();

  /* Trigger disabled
   */
  ps2000_set_trigger ( unitOpened.handle, PS2000_NONE, 0, PS2000_RISING, 0, auto_trigger_ms );

  /*  find the maximum number of samples, the time interval (in time_units),
   *		 the most suitable time units, and the maximum oversample at the current timebase
   */
	oversample = 1;
  while (!ps2000_get_timebase ( unitOpened.handle,
                        timebase,
  					      	    no_of_samples,
                        &time_interval,
                        &time_units,
                        oversample,
                        &max_samples))
	  timebase++;										;

  printf ( "timebase: %hd\toversample:%hd\n", timebase, oversample );
  /* Start it collecting,
   *  then wait for completion
   */
  ps2000_run_block ( unitOpened.handle, no_of_samples, timebase, oversample, &time_indisposed_ms );
  while ( !ps2000_ready ( unitOpened.handle ) )
    {
    Sleep ( 100 );
    }

  ps2000_stop ( unitOpened.handle );

  /* Should be done now...
   *  get the times (in nanoseconds)
   *   and the values (in ADC counts)
   */
  ps2000_get_times_and_values ( unitOpened.handle, times,
																unitOpened.channelSettings[PS2000_CHANNEL_A].values, 
																unitOpened.channelSettings[PS2000_CHANNEL_B].values,
																NULL,
																NULL,
																&overflow, time_units, no_of_samples );

  /* Print out the first 10 readings,
   *  converting the readings to mV if required
   */
  printf ( "First 10 readings\n" );
  printf ( "Value\n" );
  printf ( "(%s)\n", adc_units ( time_units ) );

  for ( i = 0; i < 10; i++ )
  {
		for (ch = 0; ch < unitOpened.noOfChannels; ch++)
		{
			if(unitOpened.channelSettings[ch].enabled)
  		{
				printf ( "%d\t", adc_to_mv ( unitOpened.channelSettings[ch].values[i], unitOpened.channelSettings[ch].range) );
			}
		}
		printf("\n");
  }

  fp = fopen ( "data.txt","w" );
  if (fp != NULL)
  {
	for ( i = 0; i < BUFFER_SIZE; i++)
	{
			fprintf ( fp,"%ld ", times[i]);
			for (ch = 0; ch < unitOpened.noOfChannels; ch++)
			{
			if(unitOpened.channelSettings[ch].enabled)
				{
					fprintf ( fp, ",%d, %d,", unitOpened.channelSettings[ch].values[i],
																		adc_to_mv ( unitOpened.channelSettings[ch].values[i],	unitOpened.channelSettings[ch].range) );
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
  int		i;
  int		trigger_sample;
  long 	time_interval;
  short 	time_units;
  short 	oversample;
  long 	no_of_samples = BUFFER_SIZE;
  FILE 	*fp;
  short 	auto_trigger_ms = 0;
  long 	time_indisposed_ms;
  short 	overflow;
  int 	threshold_mv =1500;
  long 	max_samples;
	short ch;

  printf ( "Collect block triggered...\n" );
  printf ( "Collects when value rises past %dmV\n", threshold_mv );
  printf ( "Press a key to start...\n" );
  getch ();

  set_defaults ();

  /* Trigger enabled
	 * ChannelA - to trigger unsing this channel it needs to be enabled using ps2000_set_channel
   * Rising edge
   * Threshold = 100mV
   * 10% pre-trigger  (negative is pre-, positive is post-)
   */
	unitOpened.trigger.simple.channel = PS2000_CHANNEL_A;
	unitOpened.trigger.simple.direction = (short) PS2000_RISING;
	unitOpened.trigger.simple.threshold = 100.f;
	unitOpened.trigger.simple.delay = -10;

  ps2000_set_trigger ( unitOpened.handle,
											 (short) unitOpened.trigger.simple.channel,
											 mv_to_adc (threshold_mv, unitOpened.channelSettings[(short) unitOpened.trigger.simple.channel].range),
											 unitOpened.trigger.simple.direction,
											 (short)unitOpened.trigger.simple.delay,
											 auto_trigger_ms );


  /*  find the maximum number of samples, the time interval (in time_units),
   *		 the most suitable time units, and the maximum oversample at the current timebase
   */
	oversample = 1;
  while (!ps2000_get_timebase ( unitOpened.handle,
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
  ps2000_run_block ( unitOpened.handle, BUFFER_SIZE, timebase, oversample, &time_indisposed_ms );

  printf ( "Waiting for trigger..." );
  printf ( "Press a key to abort\n" );

  while (( !ps2000_ready ( unitOpened.handle )) && ( !kbhit () ))
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
    ps2000_stop ( unitOpened.handle );

    /* Get the times (in units specified by time_units)
     *  and the values (in ADC counts)
     */
    ps2000_get_times_and_values ( unitOpened.handle,
			                            times,
																	unitOpened.channelSettings[PS2000_CHANNEL_A].values,
																	unitOpened.channelSettings[PS2000_CHANNEL_B].values,
																	NULL,
																	NULL,
																	&overflow, time_units, BUFFER_SIZE );

    /* Print out the first 10 readings,
     *  converting the readings to mV if required
     */
    printf ("Ten readings around trigger\n");
    printf ("Time\tValue\n");
    printf ("(ns)\t(%s)\n", adc_units (time_units));

    /* This calculation is correct for 10% pre-trigger
     */
    trigger_sample = BUFFER_SIZE / 10;

    for (i = trigger_sample - 5; i < trigger_sample + 5; i++)
    {
			for (ch = 0; ch < unitOpened.noOfChannels; ch++)
			{
				if(unitOpened.channelSettings[ch].enabled)
				{
					printf ( "%d\t", adc_to_mv ( unitOpened.channelSettings[ch].values[i], unitOpened.channelSettings[ch].range) );
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
				for (ch = 0; ch < unitOpened.noOfChannels; ch++)
				{
					if(unitOpened.channelSettings[ch].enabled)
					{
						fprintf ( fp, ",%d, %d,", unitOpened.channelSettings[ch].values[i],
																			adc_to_mv ( unitOpened.channelSettings[ch].values[i], unitOpened.channelSettings[ch].range) );
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

void collect_block_advanced_triggered ()
{
int		i;
  int		trigger_sample;
  long 	time_interval;
  short 	time_units;
  short 	oversample;
  long 	no_of_samples = BUFFER_SIZE;
  FILE 	*fp;
  long 	time_indisposed_ms;
  short 	overflow;
  int 	threshold_mv =1500;
  long 	max_samples;
	short ch;

  printf ( "Collect block triggered...\n" );
  printf ( "Collects when value rises past %dmV\n", threshold_mv );
  printf ( "Press a key to start...\n" );
  getch ();

  set_defaults ();

	set_trigger_advanced ();


  /*  find the maximum number of samples, the time interval (in time_units),
   *		 the most suitable time units, and the maximum oversample at the current timebase
   */
	oversample = 1;
  while (!ps2000_get_timebase ( unitOpened.handle,
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
  ps2000_run_block ( unitOpened.handle, BUFFER_SIZE, timebase, oversample, &time_indisposed_ms );

  printf ( "Waiting for trigger..." );
  printf ( "Press a key to abort\n" );

  while (( !ps2000_ready ( unitOpened.handle )) && ( !kbhit () ))
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
    ps2000_stop ( unitOpened.handle );

    /* Get the times (in units specified by time_units)
     *  and the values (in ADC counts)
     */
    ps2000_get_times_and_values ( unitOpened.handle,
			                            times,
																	unitOpened.channelSettings[PS2000_CHANNEL_A].values,
																	unitOpened.channelSettings[PS2000_CHANNEL_B].values,
																	NULL,
																	NULL,
																	&overflow, time_units, BUFFER_SIZE );

    /* Print out the first 10 readings,
     *  converting the readings to mV if required
     */
    printf ("Ten readings around trigger\n");
    printf ("Time\tValue\n");
    printf ("(ns)\t(%s)\n", adc_units (time_units));

    /* This calculation is correct for 10% pre-trigger
     */
    trigger_sample = BUFFER_SIZE / 10;

    for (i = trigger_sample - 5; i < trigger_sample + 5; i++)
    {
			for (ch = 0; ch < unitOpened.noOfChannels; ch++)
			{
				if(unitOpened.channelSettings[ch].enabled)
				{
					printf ( "%d\t", adc_to_mv ( unitOpened.channelSettings[ch].values[i], unitOpened.channelSettings[ch].range) );
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
			for (ch = 0; ch < unitOpened.noOfChannels; ch++)
			{
				if(unitOpened.channelSettings[ch].enabled)
				{
					fprintf ( fp, ",%d, %d,", unitOpened.channelSettings[ch].values[i],
																		adc_to_mv ( unitOpened.channelSettings[ch].values[i], unitOpened.channelSettings[ch].range) );
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
	int		i;
	int		trigger_sample;
	FILE 	*fp;
	short 	auto_trigger_ms = 0;
	long 	time_indisposed_ms;
	short 	overflow;
	long  	ets_sampletime;
	short ok;
	short ch;

	printf ( "Collect ETS block...\n" );
	printf ( "Collects when value rises past 1500mV\n" );
	printf ( "Press a key to start...\n" );
	getch ();

	set_defaults ();

	/* Trigger enabled
	* Channel A - to trigger unsing this channel it needs to be enabled using ps2000_set_channel
	* Rising edge
	* Threshold = 1500mV
	* 10% pre-trigger  (negative is pre-, positive is post-)
	*/
	unitOpened.trigger.simple.channel = PS2000_CHANNEL_A;
	unitOpened.trigger.simple.delay = -10.f;
	unitOpened.trigger.simple.direction = PS2000_RISING;
	unitOpened.trigger.simple.threshold = 1500.f;


	ps2000_set_trigger ( unitOpened.handle,
		(short) unitOpened.trigger.simple.channel,
		mv_to_adc (1500, unitOpened.channelSettings[(short) unitOpened.trigger.simple.channel].range),
		unitOpened.trigger.simple.direction ,
		(short) unitOpened.trigger.simple.delay,
		auto_trigger_ms );

	/* Enable ETS in fast mode,
	* the computer will store 60 cycles
	*  but interleave only 4
	*/
	ets_sampletime = ps2000_set_ets ( unitOpened.handle, PS2000_ETS_FAST, 60, 4 );
	printf ( "ETS Sample Time is: %ld\n", ets_sampletime );
	/* Start it collecting,
	*  then wait for completion
	*/
	ok = ps2000_run_block ( unitOpened.handle, BUFFER_SIZE, timebase, 1, &time_indisposed_ms );

	printf ( "Waiting for trigger..." );
	printf ( "Press a key to abort\n" );

	while ( (!ps2000_ready (unitOpened.handle)) && (!kbhit ()) )
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
		ps2000_stop ( unitOpened.handle );
		/* Get the times (in microseconds)
		*  and the values (in ADC counts)
		*/
		ok = (short)ps2000_get_times_and_values ( unitOpened.handle,
																							times,
																							unitOpened.channelSettings[PS2000_CHANNEL_A].values,
																							unitOpened.channelSettings[PS2000_CHANNEL_B].values,
																							NULL,
																							NULL,
																							&overflow,
																							PS2000_PS,
																							BUFFER_SIZE);

		/* Print out the first 10 readings,
		*  converting the readings to mV if required
		*/

		printf ( "Ten readings around trigger\n" );
		printf ( "(ps)\t(mv)\n");

		/* This calculation is correct for 10% pre-trigger
		*/
		trigger_sample = BUFFER_SIZE / 10;

		for ( i = trigger_sample - 5; i < trigger_sample + 5; i++ )
		{
			printf ( "%ld\t", times [i]);
			for (ch = 0; ch < unitOpened.noOfChannels; ch++)
			{
				if (unitOpened.channelSettings[ch].enabled)
				{
					printf ( "%d\t\n", adc_to_mv (unitOpened.channelSettings[ch].values[i], unitOpened.channelSettings[ch].range));
				}
			}
			printf ("\n");
		}

		fp = fopen ( "data.txt","w" );
		if (fp != NULL)
		{
			for ( i = 0; i < BUFFER_SIZE; i++ )
			{
				fprintf ( fp, "%ld,", times[i] );
				for (ch = 0; ch < unitOpened.noOfChannels; ch++)
				{
					if (unitOpened.channelSettings[ch].enabled)
					{
						fprintf ( fp, "%ld, %d, %d", times[i],  unitOpened.channelSettings[ch].values[i], adc_to_mv (unitOpened.channelSettings[ch].values[i], unitOpened.channelSettings[ch].range) );
					}
				}
				fprintf (fp, "\n");
			}
			fclose( fp );
		}
		else
			printf("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
	}
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

void collect_streaming (void)
{
	int		i;
	int		block_no;
	FILE 	*fp;
	int		no_of_values;
	short  overflow;
	int 	ok;
	short ch;

	printf ( "Collect streaming...\n" );
	printf ( "Data is written to disk file (data.txt)\n" );
	printf ( "Press a key to start\n" );
	getch ();

	set_defaults ();

	/* You cannot use triggering for the start of the data...
	*/
	ps2000_set_trigger ( unitOpened.handle, PS2000_NONE, 0, 0, 0, 0 );

	/* Collect data at 10ms intervals
	* Max BUFFER_SIZE points on each call
	*  (buffer must be big enough for max time between calls
	*
	*  Start it collecting,
	*  then wait for trigger event
	*/
	ok = ps2000_run_streaming ( unitOpened.handle, 10, 1000, 0 );
	printf ( "OK: %d\n", ok );

	/* From here on, we can get data whenever we want...
	*/
	block_no = 0;
	fp = fopen ( "data.txt", "w" );
	while ( !kbhit () )
	{
		no_of_values = ps2000_get_values ( unitOpened.handle, 
			unitOpened.channelSettings[PS2000_CHANNEL_A].values,
			unitOpened.channelSettings[PS2000_CHANNEL_B].values,
			NULL,
			NULL,
			&overflow,
			BUFFER_SIZE );
		printf ( "%d values\n", no_of_values );

		if ( block_no++ > 20 )
		{
			block_no = 0;
			printf ( "Press any key to stop\n" );
		}

		/* Print out the first 10 readings
		*/
		if (fp != NULL)
		{
			for ( i = 0; i < no_of_values; i++ )
			{
				for (ch = 0; ch < unitOpened.noOfChannels; ch++)
				{
					if (unitOpened.channelSettings[ch].enabled)
					{
						fprintf ( fp, "%d, ", adc_to_mv (unitOpened.channelSettings[ch].values[i], unitOpened.channelSettings[ch].range) );
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
	fclose ( fp );

	ps2000_stop ( unitOpened.handle );

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
	ps2000_set_trigger ( unitOpened.handle, PS2000_NONE, 0, 0, 0, 0 );

	unitOpened.trigger.advanced.autoStop = 0;
	unitOpened.trigger.advanced.totalSamples = 0;
	unitOpened.trigger.advanced.triggered = 0;

	/* Collect data at 10us intervals
	* 100000 points with an agregation of 100 : 1
	*	Auto stop after the 100000 samples
	*  Start it collecting,
	*/
	ok = ps2000_run_streaming_ns ( unitOpened.handle, 10, PS2000_US, BUFFER_SIZE_STREAMING, 1, 100, 30000 );
	printf ( "OK: %d\n", ok );

	/* From here on, we can get data whenever we want...
	*/	
	
	while (!unitOpened.trigger.advanced.autoStop)
	{
		
		ps2000_get_streaming_last_values (unitOpened.handle, ps2000FastStreamingReady);
		if (nPreviousValues != unitOpened.trigger.advanced.totalSamples)
		{
			
			printf ("Values collected: %ld\n", unitOpened.trigger.advanced.totalSamples - nPreviousValues);
			nPreviousValues = 	unitOpened.trigger.advanced.totalSamples;

		}
		Sleep (0);
		
	}

	ps2000_stop (unitOpened.handle);

	no_of_samples = ps2000_get_streaming_values_no_aggregation (unitOpened.handle,
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
	ok = ps2000_run_streaming_ns ( unitOpened.handle, 10, PS2000_US, BUFFER_SIZE_STREAMING, 1, 100, 30000 );
	printf ( "OK: %d\n", ok );

	/* From here on, we can get data whenever we want...
	*/	
	
	while (!unitOpened.trigger.advanced.autoStop)
	{
		ps2000_get_streaming_last_values (unitOpened.handle, ps2000FastStreamingReady);
		if (nPreviousValues != unitOpened.trigger.advanced.totalSamples)
		{
			printf ("Values collected: %ld\n", unitOpened.trigger.advanced.totalSamples - nPreviousValues);
			nPreviousValues = 	unitOpened.trigger.advanced.totalSamples;
		}
		Sleep (0);
	}

	ps2000_stop (unitOpened.handle);

	no_of_samples = ps2000_get_streaming_values_no_aggregation (unitOpened.handle,
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
      ps2000_get_unit_info ( unitOpened.handle, line, sizeof (line), i );
			if (i == 3)
			{
			  variant = atoi(line);
			}
      printf ( "%s: %s\n", description[i], line );
    }

  	switch (variant)
		{
		case MODEL_PS2104:
			unitOpened.model = MODEL_PS2104;
			unitOpened.firstRange = PS2000_100MV;
			unitOpened.lastRange = PS2000_20V;
			unitOpened.maxTimebase = PS2104_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebase;
			unitOpened.noOfChannels = 1;
			unitOpened.hasAdvancedTriggering = FALSE;
			unitOpened.hasSignalGenerator = FALSE;
			unitOpened.hasEts = TRUE;
			unitOpened.hasFastStreaming = FALSE;
		break;

		case MODEL_PS2105:
			unitOpened.model = MODEL_PS2105;
			unitOpened.firstRange = PS2000_100MV;
			unitOpened.lastRange = PS2000_20V;
			unitOpened.maxTimebase = PS2105_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebase;
			unitOpened.noOfChannels = 1; 
			unitOpened.hasAdvancedTriggering = FALSE;
			unitOpened.hasSignalGenerator = FALSE;
			unitOpened.hasEts = TRUE;
			unitOpened.hasFastStreaming = FALSE;
		break;
				
		case MODEL_PS2202:
			unitOpened.model = MODEL_PS2202;
			unitOpened.firstRange = PS2000_100MV;
			unitOpened.lastRange = PS2000_20V;
			unitOpened.maxTimebase = PS2200_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebase;
			unitOpened.noOfChannels = 2;
			unitOpened.hasAdvancedTriggering = FALSE;
			unitOpened.hasSignalGenerator = FALSE;
			unitOpened.hasEts = FALSE;
			unitOpened.hasFastStreaming = FALSE;
		break;

		case MODEL_PS2203:
			unitOpened.model = MODEL_PS2203;
			unitOpened.firstRange = PS2000_50MV;
			unitOpened.lastRange = PS2000_20V;
			unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebase;
			unitOpened.noOfChannels = 2; 
			unitOpened.hasAdvancedTriggering = TRUE;
			unitOpened.hasSignalGenerator = TRUE;
			unitOpened.hasEts = TRUE;
			unitOpened.hasFastStreaming = TRUE;
		break;

		case MODEL_PS2204:
			unitOpened.model = MODEL_PS2204;
			unitOpened.firstRange = PS2000_50MV;
			unitOpened.lastRange = PS2000_20V;
			unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebase;
			unitOpened.noOfChannels = 2;
			unitOpened.hasAdvancedTriggering = TRUE;
			unitOpened.hasSignalGenerator = TRUE;
			unitOpened.hasEts = TRUE;
			unitOpened.hasFastStreaming = TRUE;
		break;

		case MODEL_PS2205:
			unitOpened.model = MODEL_PS2205;
			unitOpened.firstRange = PS2000_50MV;
			unitOpened.lastRange = PS2000_20V;
			unitOpened.maxTimebase = PS2000_MAX_TIMEBASE;
			unitOpened.timebases = unitOpened.maxTimebase;
			unitOpened.noOfChannels = 2; 
			unitOpened.hasAdvancedTriggering = TRUE;
			unitOpened.hasSignalGenerator = TRUE;
			unitOpened.hasEts = TRUE;
			unitOpened.hasFastStreaming = TRUE;
		break;

 		default:
			printf("Unit not supported");
		}
		unitOpened.channelSettings [PS2000_CHANNEL_A].enabled = 1;
		unitOpened.channelSettings [PS2000_CHANNEL_A].DCcoupled = 1;
		unitOpened.channelSettings [PS2000_CHANNEL_A].range = unitOpened.lastRange;

		unitOpened.channelSettings [PS2000_CHANNEL_B].enabled = 0;
		unitOpened.channelSettings [PS2000_CHANNEL_B].DCcoupled = 1;
		unitOpened.channelSettings [PS2000_CHANNEL_B].range = unitOpened.lastRange;
  }
  else
  {
    printf ( "Unit Not Opened\n" );
    ps2000_get_unit_info ( unitOpened.handle, line, sizeof (line), PS2000_ERROR_CODE );
    printf ( "%s: %s\n", description[5], line );
		unitOpened.model = MODEL_NONE;
		unitOpened.firstRange = PS2000_100MV;
		unitOpened.lastRange = PS2000_20V;
		unitOpened.timebases = PS2105_MAX_TIMEBASE;
		unitOpened.noOfChannels = 1;	
	}
}

				

void set_sig_gen ()
{
	short waveform;
	long frequency;

	printf("Enter frequency in Hz: "); // Ask user to enter signal frequency;
	do 
	{
		scanf("%lu", &frequency);
	} while (frequency < 1000 || frequency > PS2000_MAX_SIGGEN_FREQ);

	printf("Signal generator On");
	printf("Enter type of waveform (0..9 or 99)\n");
	printf("0:\tSINE\n");
	printf("1:\tSQUARE\n");
	printf("2:\tTRIANGLE\n");
	printf("3:\tRAMP UP\n");
	printf("4:\tRAMP DOWN\n");

	do
	{
		scanf("%hd", &waveform);
	} while (waveform < 0 || waveform >= PS2000_DC_VOLTAGE);

	ps2000_set_sig_gen_built_in (unitOpened.handle,
															 0,
															 1000000, // 1 volt
															 waveform,
															 (float)frequency,
															 (float)frequency,
															 0,
															 0,
															 PS2000_UPDOWN, 0);
}

void set_sig_gen_arb (void)
{
	long frequency;
	char fileName [128];
	FILE * fp;
	unsigned char arbitraryWaveform [4096];
	short waveformSize = 0;
	double delta;

	memset(&arbitraryWaveform, 0, 4096);

	printf("Enter frequency in Hz: "); // Ask user to enter signal frequency;
	do {
		scanf("%lu", &frequency);
	} while (frequency < 1000 || frequency > 10000000);

	waveformSize = 0;

	printf("Select a waveform file to load: ");
	
	scanf("%s", fileName);
	if ((fp = fopen(fileName, "r"))) 
	{ // Having opened file, read in data - one number per line (at most 4096 lines), with values in (0..255)
		while (EOF != fscanf(fp, "%c", (arbitraryWaveform + waveformSize))&& waveformSize++ < 4096)
			;
		fclose(fp);
	}
	else
	{
		printf("Invalid filename\n");
		return;
	}


	delta = ((frequency * waveformSize) / 4096) * 4294967296.0 * 20e-9;
	ps2000_set_sig_gen_arbitrary(unitOpened.handle, 0, 2000000, (unsigned long)delta, (unsigned long)delta, 0, 0, arbitraryWaveform, waveformSize, PS2000_UP, 0);
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

  printf ( "Specify timebase\n" );

  /* See what ranges are available...
   */
	oversample = 1;
  for (i = 0; i < unitOpened.timebases; i++)
    {
    ps2000_get_timebase ( unitOpened.handle, i, BUFFER_SIZE, &time_interval, &time_units, oversample, &max_samples );
    if ( time_interval > 0 )
      {
      printf ( "%d -> %ld %s  %hd\n", i, time_interval, adc_units(time_units), time_units );
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
  ps2000_get_timebase ( unitOpened.handle, i, BUFFER_SIZE, &time_interval, &time_units, oversample, &max_samples );
  printf ( "Timebase %d - %ld ns\n", timebase, time_interval );
  }

/****************************************************************************
 * Select input voltage ranges for channels A and B
 ****************************************************************************/
void set_voltages ()
{
  int		i;
	short ch = 0;

  /* See what ranges are available...
   */
  for ( i = unitOpened.firstRange; i <= unitOpened.lastRange; i++ )
  {
    printf ( "%d -> %d mV\n", i, input_ranges[i] );
  }

  /* Ask the user to select a range
   */
	for (ch = 0; ch < unitOpened.noOfChannels; ch++)
	{
		printf ( "Specify voltage range (%d..%d)\n", unitOpened.firstRange, (unitOpened.lastRange) );
		printf ( "99 - switches channel off\n");
		printf ( "\nChannel %c: ", 'A' + ch);
		do
		{
			fflush( stdin );
			scanf ( "%hd", &unitOpened.channelSettings[ch].range);
		} while ( unitOpened.channelSettings[ch].range != 99 && (unitOpened.channelSettings[ch].range < unitOpened.firstRange || unitOpened.channelSettings[ch].range  > unitOpened.lastRange) );
		if(unitOpened.channelSettings[ch].range != 99)
		{
			printf ( " - %d mV\n", input_ranges[unitOpened.channelSettings[ch].range]);
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
 *
 *
 ****************************************************************************/

int main (void)
{
  char	ch;

  printf ( "PS2000 driver example program\n" );
  printf ( "Version 1.0\n\n" );

  printf ( "\n\nOpening the device...\n");

  //open unit and show splash screen
  unitOpened.handle = ps2000_open_unit ();
  printf ( "Handle: %d\n", unitOpened.handle );
  if ( unitOpened.handle < 1 )
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
      printf ( "T - triggered block		          I - Set timebase\n" );
			printf ( "Y - advanced triggered block    A - ADC counts/mV\n" );
      printf ( "E - ETS block\n" );
      printf ( "S - Streaming\n");
      printf ( "F - Fast streaming\n");
			printf ( "D - Fast streaming triggered\n");
			printf ( "G - Signal generator\n");
			printf ( "H - Arbitrary signal generator\n");
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
					collect_block_advanced_triggered ();
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

        case 'E':
				if (unitOpened.hasEts)
				{
	        collect_block_ets ();
				}
				else
				{
					printf ("Not supported by this model\n\n");
				}
        break;

				case 'G':
				if (unitOpened.hasSignalGenerator)
				{
					set_sig_gen ();
				}
				break;

				case 'H':
				if (unitOpened.hasSignalGenerator)
				{
					set_sig_gen_arb ();
				}
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

    ps2000_close_unit ( unitOpened.handle );
    }

	return 1;
  }
