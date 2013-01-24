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
 * @file acquisition3000.cpp
 * @brief Definition of Acquisition3000 class.
 * Acquisition3000 methods using libps3000 to drive Picoscope 3000 series HW
 * @version 0.1
 * @date 2013, january 4
 * @author Vincent HERVIEUX    -   01.04.2013   -   initial creation
 */

#include "acquisition3000.h"

#ifdef HAVE_LIBPS3000

#ifndef WIN32
#define Sleep(x) usleep(1000*(x))
enum BOOL {FALSE,TRUE};
#endif

#define QUAD_SCOPE 4
#define DUAL_SCOPE 2

/* static members initialization */
Acquisition3000 *Acquisition3000::singleton_m = NULL;
const short Acquisition3000::input_ranges [] = {10, 20, 50, 100, 200, 500, 1000, 3000, 5000, 10000, 30000, 50000};

/****************************************************************************
 *
 * constructor
 *
 ****************************************************************************/
Acquisition3000::Acquisition3000() :
    scale_to_mv(1),
    timebase(8)
{
    DEBUG( "Opening the device...\n");

    //open unit and show splash screen
    unitOpened_m.handle = ps3000_open_unit ();
    DEBUG ( "Handle: %d\n", unitOpened_m.handle );
    if ( unitOpened_m.handle < 1 )
    {
        DEBUG ( "Unable to open device\n" );
        return;
    }


    if ( unitOpened_m.handle >= 1 )
    {
        DEBUG ( "Device opened successfully\n\n" );
        get_info ();

    }

}

/****************************************************************************
 *
 * get_instance
 *
 ****************************************************************************/
Acquisition3000* Acquisition3000::get_instance()
{
    if(NULL == Acquisition3000::singleton_m)
    {
        Acquisition3000::singleton_m = new Acquisition3000();
    }

    return Acquisition3000::singleton_m;
}

/****************************************************************************
 *
 * destructor
 *
 ****************************************************************************/
Acquisition3000::~Acquisition3000()
{
    ps3000_close_unit ( unitOpened_m.handle );
}

/****************************************************************************
 *
 * ps3000FastStreamingReady
 *
 ****************************************************************************/
void  __stdcall Acquisition3000::ps3000FastStreamingReady( short **overviewBuffers,
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
    Acquisition3000* instance = Acquisition3000::get_instance();
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
void Acquisition3000::get_device_info(device_info_t* info)
{
    if(NULL == info)
    {
        ERROR("%s : invalid pointer given!\n", __FUNCTION__);
        return;
    }

    memset(info, 0, sizeof(device_info_t));

    switch(unitOpened_m.model)
    {
        case MODEL_PS3204:
            snprintf(info->device_name, DEVICE_NAME_MAX, "PS3204");
            break;
        case MODEL_PS3205:
            snprintf(info->device_name, DEVICE_NAME_MAX, "PS3205"); 
            break;
        case MODEL_PS3206:
            snprintf(info->device_name, DEVICE_NAME_MAX, "PS3206"); 
            break;
        case MODEL_PS3223:
            snprintf(info->device_name, DEVICE_NAME_MAX, "PS3223"); 
            break;
        case MODEL_PS3423:
            snprintf(info->device_name, DEVICE_NAME_MAX, "PS3423"); 
            break;
        case MODEL_PS3224:
            snprintf(info->device_name, DEVICE_NAME_MAX, "PS3224"); 
            break;
        case MODEL_PS3424:
            snprintf(info->device_name, DEVICE_NAME_MAX, "PS3424"); 
            break;
        case MODEL_PS3225:
            snprintf(info->device_name, DEVICE_NAME_MAX, "PS3225"); 
            break;
        case MODEL_PS3425:
            snprintf(info->device_name, DEVICE_NAME_MAX, "PS3425"); 
            break;
        case MODEL_NONE:
            snprintf(info->device_name, DEVICE_NAME_MAX, "No device or device not supported"); 
            break;
    }
    info->nb_channels = unitOpened_m.noOfChannels;
#ifdef TEST_WITHOUT_HW
    snprintf(info->device_name, DEVICE_NAME_MAX, "Tests without HW");
    info->nb_channels = 2;
#endif
}

/****************************************************************************
 * set_defaults - restore default settings
 ****************************************************************************/
void Acquisition3000::set_defaults (void)
{
    short ch = 0;
  ps3000_set_ets ( unitOpened_m.handle, PS3000_ETS_OFF, 0, 0 );

    for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
    {
        ps3000_set_channel ( unitOpened_m.handle,
                               ch,
                                                 unitOpened_m.channelSettings[ch].enabled ,
                                                 unitOpened_m.channelSettings[ch].DCcoupled ,
                                                 unitOpened_m.channelSettings[ch].range);
    }
}

/****************************************************************************
 * set_trigger_advanced - set advance trigger parameters
 ****************************************************************************/
void Acquisition3000::set_trigger_advanced(void)
{
    short ok = 0;
    short auto_trigger_ms = 0;

    // to trigger of more than one channel set this parameter to 2 or more
    // each condition can only have on parameter set to PS3000_CONDITION_TRUE or PS3000_CONDITION_FALSE
    // if more than on condition is set then it will trigger off condition one, or condition two etc.
    unitOpened_m.trigger.advanced.nProperties = 1;
    // set the trigger channel to channel A by using PS3000_CONDITION_TRUE
    unitOpened_m.trigger.advanced.conditions = (TRIGGER_CONDITIONS*)malloc (sizeof (TRIGGER_CONDITIONS) * unitOpened_m.trigger.advanced.nProperties);
    unitOpened_m.trigger.advanced.conditions->channelA = CONDITION_TRUE;
    unitOpened_m.trigger.advanced.conditions->channelB = CONDITION_DONT_CARE;
    unitOpened_m.trigger.advanced.conditions->channelC = CONDITION_DONT_CARE;
    unitOpened_m.trigger.advanced.conditions->channelD = CONDITION_DONT_CARE;
    unitOpened_m.trigger.advanced.conditions->external = CONDITION_DONT_CARE;
    unitOpened_m.trigger.advanced.conditions->pulseWidthQualifier = CONDITION_DONT_CARE;

    // set channel A to rising
    // the remainder will be ignored as only a condition is set for channel A
    unitOpened_m.trigger.advanced.directions.channelA = RISING;
    unitOpened_m.trigger.advanced.directions.channelB = RISING;
    unitOpened_m.trigger.advanced.directions.channelC = RISING;
    unitOpened_m.trigger.advanced.directions.channelD = RISING;
    unitOpened_m.trigger.advanced.directions.ext = RISING;


    unitOpened_m.trigger.advanced.channelProperties = (TRIGGER_CHANNEL_PROPERTIES*)malloc (sizeof (TRIGGER_CHANNEL_PROPERTIES) * unitOpened_m.trigger.advanced.nProperties);
    // there is one property for each condition
    // set channel A
    // trigger level 1500 adc counts the trigger point will vary depending on the voltage range
    // hysterisis 4096 adc counts
    unitOpened_m.trigger.advanced.channelProperties->channel = (short) PS3000_CHANNEL_A;
    unitOpened_m.trigger.advanced.channelProperties->thresholdMajor = 1500;
    // not used in level triggering, should be set when in window mode
    unitOpened_m.trigger.advanced.channelProperties->thresholdMinor = 0;
    // used in level triggering, not used when in window mode
    unitOpened_m.trigger.advanced.channelProperties->hysteresis = (short) 4096;
    unitOpened_m.trigger.advanced.channelProperties->thresholdMode = LEVEL;

    ok = ps3000SetAdvTriggerChannelConditions (unitOpened_m.handle, unitOpened_m.trigger.advanced.conditions, unitOpened_m.trigger.advanced.nProperties);
    if ( !ok )
        WARNING("ps3000SetAdvTriggerChannelConditions returned value is not 0.");
    ok = ps3000SetAdvTriggerChannelDirections (unitOpened_m.handle,
                                                                                unitOpened_m.trigger.advanced.directions.channelA,
                                                                                unitOpened_m.trigger.advanced.directions.channelB,
                                                                                unitOpened_m.trigger.advanced.directions.channelC,
                                                                                unitOpened_m.trigger.advanced.directions.channelD,
                                                                                unitOpened_m.trigger.advanced.directions.ext);
    if ( !ok )
        WARNING("ps3000SetAdvTriggerChannelDirections returned value is not 0.");
    ok = ps3000SetAdvTriggerChannelProperties (unitOpened_m.handle,
                                                                                unitOpened_m.trigger.advanced.channelProperties,
                                                                                unitOpened_m.trigger.advanced.nProperties,
                                                                                auto_trigger_ms);
    if ( !ok )
        WARNING("ps3000SetAdvTriggerChannelProperties returned value is not 0.");


    // remove comments to try triggering with a pulse width qualifier
    // add a condition for the pulse width eg. in addition to the channel A or as a replacement
    //unitOpened_m.trigger.advanced.pwq.conditions = malloc (sizeof (PS3000_PWQ_CONDITIONS));
    //unitOpened_m.trigger.advanced.pwq.conditions->channelA = PS3000_CONDITION_TRUE;
    //unitOpened_m.trigger.advanced.pwq.conditions->channelB = PS3000_CONDITION_DONT_CARE;
    //unitOpened_m.trigger.advanced.pwq.conditions->channelC = PS3000_CONDITION_DONT_CARE;
    //unitOpened_m.trigger.advanced.pwq.conditions->channelD = PS3000_CONDITION_DONT_CARE;
    //unitOpened_m.trigger.advanced.pwq.conditions->external = PS3000_CONDITION_DONT_CARE;
    //unitOpened_m.trigger.advanced.pwq.nConditions = 1;

    //unitOpened_m.trigger.advanced.pwq.direction = PS3000_RISING;
    //unitOpened_m.trigger.advanced.pwq.type = PS3000_PW_TYPE_LESS_THAN;
    //// used when type    PS3000_PW_TYPE_IN_RANGE,    PS3000_PW_TYPE_OUT_OF_RANGE
    //unitOpened_m.trigger.advanced.pwq.lower = 0;
    //unitOpened_m.trigger.advanced.pwq.upper = 10000;
    //ps3000SetPulseWidthQualifier (unitOpened_m.handle,
    //                                                            unitOpened_m.trigger.advanced.pwq.conditions,
    //                                                            unitOpened_m.trigger.advanced.pwq.nConditions,
    //                                                            unitOpened_m.trigger.advanced.pwq.direction,
    //                                                            unitOpened_m.trigger.advanced.pwq.lower,
    //                                                            unitOpened_m.trigger.advanced.pwq.upper,
    //                                                            unitOpened_m.trigger.advanced.pwq.type);

    ok = ps3000SetAdvTriggerDelay (unitOpened_m.handle, 0, -10);
}

/****************************************************************************
 * Collect_block_immediate
 *  this function demonstrates how to collect a single block of data
 *  from the unit (start collecting immediately)
 ****************************************************************************/
void Acquisition3000::collect_block_immediate (void)
{
  int        i;
  long     time_interval;
  short     time_units;
  short     oversample;
  long     no_of_samples = BUFFER_SIZE;
  FILE     *fp;
  short     auto_trigger_ms = 0;
  long     time_indisposed_ms;
  short     overflow;
  long     max_samples;
    short ch = 0;

  DEBUG ( "Collect block immediate...\n" );
  DEBUG ( "Press a key to start\n" );
  ////getch ();

  set_defaults ();

  /* Trigger disabled
   */
  ps3000_set_trigger ( unitOpened_m.handle, PS3000_NONE, 0, PS3000_RISING, 0, auto_trigger_ms );

  /*  find the maximum number of samples, the time interval (in time_units),
   *         the most suitable time units, and the maximum oversample at the current timebase
   */
  oversample = 1;
  while (!ps3000_get_timebase ( unitOpened_m.handle,
                                timebase,
                                no_of_samples,
                                &time_interval,
                                &time_units,
                                oversample,
                                &max_samples))
  timebase++;                                        ;

  DEBUG ( "timebase: %hd\toversample:%hd\n", timebase, oversample );
  /* Start it collecting,
   *  then wait for completion
   */
  ps3000_run_block ( unitOpened_m.handle, no_of_samples, timebase, oversample, &time_indisposed_ms );
  while ( !ps3000_ready ( unitOpened_m.handle ) )
  {
    Sleep ( 100 );
  }

  ps3000_stop ( unitOpened_m.handle );

  /* Should be done now...
   *  get the times (in nanoseconds)
   *   and the values (in ADC counts)
   */
  ps3000_get_times_and_values ( unitOpened_m.handle, times,
                                unitOpened_m.channelSettings[PS3000_CHANNEL_A].values,
                                unitOpened_m.channelSettings[PS3000_CHANNEL_B].values,
                                NULL,
                                NULL,
                                &overflow, time_units, no_of_samples );

  /* Print out the first 10 readings,
   *  converting the readings to mV if required
   */
  DEBUG ( "First 10 readings\n" );
  DEBUG ( "Value\n" );
  DEBUG ( "(%s)\n", adc_units ( time_units ) );

  for ( i = 0; i < 10; i++ )
  {
        for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
        {
            if(unitOpened_m.channelSettings[ch].enabled)
            {
                DEBUG ( "%d\t", adc_to_mv ( unitOpened_m.channelSettings[ch].values[i], unitOpened_m.channelSettings[ch].range) );
            }
        }
        DEBUG("\n");
  }

  fp = fopen ( "data.txt","w" );
  if (fp != NULL)
  {
    for ( i = 0; i < BUFFER_SIZE; i++)
    {
            fprintf ( fp,"%ld ", times[i]);
            for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
            {
                if(unitOpened_m.channelSettings[ch].enabled)
                {
                    fprintf ( fp, ",%d, %d,", unitOpened_m.channelSettings[ch].values[i],
                                                                        adc_to_mv ( unitOpened_m.channelSettings[ch].values[i],    unitOpened_m.channelSettings[ch].range) );
                }
            }
            fprintf(fp, "\n");
        }
    fclose(fp);
  }
  else
    ERROR("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
}

/****************************************************************************
 * Collect_block_triggered
 *  this function demonstrates how to collect a single block of data from the
 *  unit, when a trigger event occurs.
 ****************************************************************************/

void Acquisition3000::collect_block_triggered (void)
  {
  int        i;
  int        trigger_sample;
  long     time_interval;
  short     time_units;
  short     oversample;
  long     no_of_samples = BUFFER_SIZE;
  FILE     *fp;
  short     auto_trigger_ms = 0;
  long     time_indisposed_ms;
  short     overflow;
  int     threshold_mv =1500;
  long     max_samples;
    short ch;

  DEBUG ( "Collect block triggered...\n" );
  DEBUG ( "Collects when value rises past %dmV\n", threshold_mv );
  DEBUG ( "Press a key to start...\n" );
  //getch ();

  set_defaults ();

  /* Trigger enabled
     * ChannelA - to trigger unsing this channel it needs to be enabled using ps3000_set_channel
   * Rising edge
   * Threshold = 100mV
   * 10% pre-trigger  (negative is pre-, positive is post-)
   */
    unitOpened_m.trigger.simple.channel = PS3000_CHANNEL_A;
    unitOpened_m.trigger.simple.direction = (short) PS3000_RISING;
    unitOpened_m.trigger.simple.threshold = 100.f;
    unitOpened_m.trigger.simple.delay = -10;

    ps3000_set_trigger ( unitOpened_m.handle,
                         (short) unitOpened_m.trigger.simple.channel,
                         mv_to_adc (threshold_mv, unitOpened_m.channelSettings[(short) unitOpened_m.trigger.simple.channel].range),
                         unitOpened_m.trigger.simple.direction,
                         (short)unitOpened_m.trigger.simple.delay,
                         auto_trigger_ms );


  /*  find the maximum number of samples, the time interval (in time_units),
   *         the most suitable time units, and the maximum oversample at the current timebase
   */
    oversample = 1;
    while (!ps3000_get_timebase ( unitOpened_m.handle,
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
  ps3000_run_block ( unitOpened_m.handle, BUFFER_SIZE, timebase, oversample, &time_indisposed_ms );

  DEBUG ( "Waiting for trigger..." );
  DEBUG ( "Press a key to abort\n" );

  while (( !ps3000_ready ( unitOpened_m.handle )) /*&& ( !kbhit () )*/)
  {
    Sleep ( 100 );
  }

//  if (kbhit ())
//  {
//    getch ();

//    DEBUG ( "data collection aborted\n" );
//  }
//  else
//  {
    ps3000_stop ( unitOpened_m.handle );

    /* Get the times (in units specified by time_units)
     *  and the values (in ADC counts)
     */
    ps3000_get_times_and_values ( unitOpened_m.handle,
                                  times,
                                  unitOpened_m.channelSettings[PS3000_CHANNEL_A].values,
                                  unitOpened_m.channelSettings[PS3000_CHANNEL_B].values,
                                  NULL,
                                  NULL,
                                  &overflow, time_units, BUFFER_SIZE );

    /* Print out the first 10 readings,
     *  converting the readings to mV if required
     */
    DEBUG ("Ten readings around trigger\n");
    DEBUG ("Time\tValue\n");
    DEBUG ("(ns)\t(%s)\n", adc_units (time_units));

    /* This calculation is correct for 10% pre-trigger
     */
    trigger_sample = BUFFER_SIZE / 10;

    for (i = trigger_sample - 5; i < trigger_sample + 5; i++)
    {
            for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
            {
                if(unitOpened_m.channelSettings[ch].enabled)
                {
                    DEBUG ( "%d\t", adc_to_mv ( unitOpened_m.channelSettings[ch].values[i], unitOpened_m.channelSettings[ch].range) );
                }
            }
            DEBUG("\n");
    }

    fp = fopen ( "data.txt","w" );
    if (fp != NULL)
    {
    for ( i = 0; i < BUFFER_SIZE; i++ )
    {
            fprintf ( fp,"%ld ", times[i]);
                for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
                {
                    if(unitOpened_m.channelSettings[ch].enabled)
                    {
                        fprintf ( fp, ",%d, %d,", unitOpened_m.channelSettings[ch].values[i],
                                                                            adc_to_mv ( unitOpened_m.channelSettings[ch].values[i], unitOpened_m.channelSettings[ch].range) );
                    }
                }
            fprintf(fp, "\n");
        }
    fclose(fp);
    }
    else
    ERROR("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
//  }
}

void Acquisition3000::collect_block_advanced_triggered ()
{
int        i;
  int        trigger_sample;
  long     time_interval;
  short     time_units;
  short     oversample;
  long     no_of_samples = BUFFER_SIZE;
  FILE     *fp;
  long     time_indisposed_ms;
  short     overflow;
  int     threshold_mv =1500;
  long     max_samples;
    short ch;

  DEBUG ( "Collect block triggered...\n" );
  DEBUG ( "Collects when value rises past %dmV\n", threshold_mv );
  DEBUG ( "Press a key to start...\n" );
  //getch ();

  set_defaults ();

  set_trigger_advanced ();


  /*  find the maximum number of samples, the time interval (in time_units),
   *         the most suitable time units, and the maximum oversample at the current timebase
   */
  oversample = 1;
  while (!ps3000_get_timebase ( unitOpened_m.handle,
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
  ps3000_run_block ( unitOpened_m.handle, BUFFER_SIZE, timebase, oversample, &time_indisposed_ms );

  DEBUG ( "Waiting for trigger..." );
  DEBUG ( "Press a key to abort\n" );

  while (( !ps3000_ready ( unitOpened_m.handle )) /*&& ( !kbhit () )*/)
  {
    Sleep ( 100 );
  }

//  if (kbhit ())
//  {
//    //getch ();

//    DEBUG ( "data collection aborted\n" );
//  }
//  else
//  {
    ps3000_stop ( unitOpened_m.handle );

    /* Get the times (in units specified by time_units)
     *  and the values (in ADC counts)
     */
    ps3000_get_times_and_values ( unitOpened_m.handle,
                                  times,
                                  unitOpened_m.channelSettings[PS3000_CHANNEL_A].values,
                                  unitOpened_m.channelSettings[PS3000_CHANNEL_B].values,
                                  NULL,
                                  NULL,
                                  &overflow, time_units, BUFFER_SIZE );

    /* Print out the first 10 readings,
     *  converting the readings to mV if required
     */
    DEBUG ("Ten readings around trigger\n");
    DEBUG ("Time\tValue\n");
    DEBUG ("(ns)\t(%s)\n", adc_units (time_units));

    /* This calculation is correct for 10% pre-trigger
     */
    trigger_sample = BUFFER_SIZE / 10;

    for (i = trigger_sample - 5; i < trigger_sample + 5; i++)
    {
            for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
            {
                if(unitOpened_m.channelSettings[ch].enabled)
                {
                    DEBUG ( "%d\t", adc_to_mv ( unitOpened_m.channelSettings[ch].values[i], unitOpened_m.channelSettings[ch].range) );
                }
            }
            DEBUG("\n");
    }

    fp = fopen ( "data.txt","w" );

    if (fp != NULL)
    {
      for ( i = 0; i < BUFFER_SIZE; i++ )
    {
          fprintf ( fp,"%ld ", times[i]);
            for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
            {
                if(unitOpened_m.channelSettings[ch].enabled)
                {
                    fprintf ( fp, ",%d, %d,", unitOpened_m.channelSettings[ch].values[i],
                                                                        adc_to_mv ( unitOpened_m.channelSettings[ch].values[i], unitOpened_m.channelSettings[ch].range) );
                }
            }
          fprintf(fp, "\n");
      }
    fclose(fp);
    }
    else
        ERROR("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
//  }
}


/****************************************************************************
 * Collect_block_ets
 *  this function demonstrates how to collect a block of
 *  data using equivalent time sampling (ETS).
 ****************************************************************************/

void Acquisition3000::collect_block_ets (void)
{
    int       i;
    int       trigger_sample;
    FILE      *fp;
    short     auto_trigger_ms = 0;
    long      time_indisposed_ms;
    short     overflow;
    long      ets_sampletime;
    short     ok = 0;
    short     ch;

    DEBUG ( "Collect ETS block...\n" );
    DEBUG ( "Collects when value rises past 1500mV\n" );
    DEBUG ( "Press a key to start...\n" );
    //getch ();

    set_defaults ();

    /* Trigger enabled
    * Channel A - to trigger unsing this channel it needs to be enabled using ps3000_set_channel
    * Rising edge
    * Threshold = 1500mV
    * 10% pre-trigger  (negative is pre-, positive is post-)
    */
    unitOpened_m.trigger.simple.channel = PS3000_CHANNEL_A;
    unitOpened_m.trigger.simple.delay = -10.f;
    unitOpened_m.trigger.simple.direction = PS3000_RISING;
    unitOpened_m.trigger.simple.threshold = 1500.f;


    ps3000_set_trigger ( unitOpened_m.handle,
        (short) unitOpened_m.trigger.simple.channel,
        mv_to_adc (1500, unitOpened_m.channelSettings[(short) unitOpened_m.trigger.simple.channel].range),
        unitOpened_m.trigger.simple.direction ,
        (short) unitOpened_m.trigger.simple.delay,
        auto_trigger_ms );

    /* Enable ETS in fast mode,
    * the computer will store 60 cycles
    *  but interleave only 4
    */
    ets_sampletime = ps3000_set_ets ( unitOpened_m.handle, PS3000_ETS_FAST, 60, 4 );
    DEBUG ( "ETS Sample Time is: %ld\n", ets_sampletime );
    /* Start it collecting,
    *  then wait for completion
    */
    ok = ps3000_run_block ( unitOpened_m.handle, BUFFER_SIZE, timebase, 1, &time_indisposed_ms );
    if ( !ok )
        WARNING ( "ps3000_run_block return value is not 0." );

    DEBUG ( "Waiting for trigger..." );
    DEBUG ( "Press a key to abort\n" );

    while ( (!ps3000_ready (unitOpened_m.handle)) /*&& (!kbhit ())*/ )
    {
        Sleep (100);
    }

//    if ( kbhit () )
//    {
//        //getch ();
//        DEBUG ( "data collection aborted\n" );
//    }
//    else
//    {
        ps3000_stop ( unitOpened_m.handle );
        /* Get the times (in microseconds)
        *  and the values (in ADC counts)
        */
        ok = (short)ps3000_get_times_and_values ( unitOpened_m.handle,
                                                  times,
                                                  unitOpened_m.channelSettings[PS3000_CHANNEL_A].values,
                                                  unitOpened_m.channelSettings[PS3000_CHANNEL_B].values,
                                                  NULL,
                                                  NULL,
                                                  &overflow,
                                                  PS3000_PS,
                                                  BUFFER_SIZE);

        /* Print out the first 10 readings,
        *  converting the readings to mV if required
        */

        DEBUG ( "Ten readings around trigger\n" );
        DEBUG ( "(ps)\t(mv)\n");

        /* This calculation is correct for 10% pre-trigger
        */
        trigger_sample = BUFFER_SIZE / 10;

        for ( i = trigger_sample - 5; i < trigger_sample + 5; i++ )
        {
            DEBUG ( "%ld\t", times [i]);
            for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
            {
                if (unitOpened_m.channelSettings[ch].enabled)
                {
                    DEBUG ( "%d\t\n", adc_to_mv (unitOpened_m.channelSettings[ch].values[i], unitOpened_m.channelSettings[ch].range));
                }
            }
            DEBUG ("\n");
        }

        fp = fopen ( "data.txt","w" );
        if (fp != NULL)
        {
            for ( i = 0; i < BUFFER_SIZE; i++ )
            {
                fprintf ( fp, "%ld,", times[i] );
                for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
                {
                    if (unitOpened_m.channelSettings[ch].enabled)
                    {
                        fprintf ( fp, "%ld, %d, %d", times[i],  unitOpened_m.channelSettings[ch].values[i], adc_to_mv (unitOpened_m.channelSettings[ch].values[i], unitOpened_m.channelSettings[ch].range) );
                    }
                }
                fprintf (fp, "\n");
            }
            fclose( fp );
        }
        else
            ERROR("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
//    }
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

void Acquisition3000::collect_streaming (void)
{
    int    i;
    FILE   *fp = NULL;
    int    no_of_values;
    short  overflow;
    int    ok;
    short  ch;
    double values_V[BUFFER_SIZE];
    double time[BUFFER_SIZE];
    DEBUG ( "Collect streaming...\n" );
    DEBUG ( "Data is written to disk file (data.txt)\n" );

    set_defaults ();

    /* You cannot use triggering for the start of the data...
    */
    ps3000_set_trigger ( unitOpened_m.handle, PS3000_NONE, 0, 0, 0, 0 );

    /* Collect data at 10ms intervals
    * Max BUFFER_SIZE points on each call
    *  (buffer must be big enough for max time between calls
    *
    *  Start it collecting,
    *  then wait for trigger event
    */
    ok = ps3000_run_streaming ( unitOpened_m.handle, 10, 1000, 0 );
    DEBUG ( "OK: %d\n", ok );

    
    while ( sem_trywait(&thread_stop) )
    {
        no_of_values = ps3000_get_values ( unitOpened_m.handle,
            unitOpened_m.channelSettings[PS3000_CHANNEL_A].values,
            unitOpened_m.channelSettings[PS3000_CHANNEL_B].values,
            unitOpened_m.channelSettings[PS3000_CHANNEL_C].values,
            unitOpened_m.channelSettings[PS3000_CHANNEL_D].values,
            &overflow,
            BUFFER_SIZE );
        DEBUG ( "%d values, overflow %d\n", no_of_values, overflow );

        for (ch = 0; ch < unitOpened_m.noOfChannels; ch++)
        {
            if (unitOpened_m.channelSettings[ch].enabled)
            {

                for (  i = 0; i < no_of_values; i++ )
                {
                    values_V[i] = 0.001 * adc_to_mv(unitOpened_m.channelSettings[ch].values[i], unitOpened_m.channelSettings[ch].range);
                    // TODO time will be probably wrong here, need to guess how to convert time range to time step...
                    //time[i] = ( i ? time[i-1] : 0) + unitOpened_m.channelSettings[ch].range;
                    time[i] = i * 0.010;
                    DEBUG("V: %lf T: %lf\n", values_V[i], time[i]);
                }

                draw->setData(ch+1, values_V, time, no_of_values);
            }

        }
        Sleep(100);
    }
    if (fp != NULL)
        fclose ( fp );

    ps3000_stop ( unitOpened_m.handle );

}

void Acquisition3000::collect_fast_streaming (void)
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
    ps3000_set_trigger ( unitOpened_m.handle, PS3000_NONE, 0, 0, 0, 0 );

    unitOpened_m.trigger.advanced.autoStop = 0;
    unitOpened_m.trigger.advanced.totalSamples = 0;
    unitOpened_m.trigger.advanced.triggered = 0;

    /* Collect data at 10us intervals
    * 100000 points with an agregation of 100 : 1
    *    Auto stop after the 100000 samples
    *  Start it collecting,
    */
    ok = ps3000_run_streaming_ns ( unitOpened_m.handle, 10, PS3000_US, BUFFER_SIZE_STREAMING, 1, 100, 30000 );
    DEBUG ( "OK: %d\n", ok );

    /* From here on, we can get data whenever we want...
    */

    while ( !unitOpened_m.trigger.advanced.autoStop && sem_trywait(&thread_stop))
    {

        ps3000_get_streaming_last_values (unitOpened_m.handle, &Acquisition3000::ps3000FastStreamingReady);
        if (nPreviousValues != unitOpened_m.trigger.advanced.totalSamples)
        {

            DEBUG ("Values collected: %ld\n", unitOpened_m.trigger.advanced.totalSamples - nPreviousValues);
            nPreviousValues =     unitOpened_m.trigger.advanced.totalSamples;

        }
        Sleep (0);

    }

    ps3000_stop (unitOpened_m.handle);

    no_of_samples = ps3000_get_streaming_values_no_aggregation (unitOpened_m.handle,
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
                printf("%d, ", adc_to_mv ((!ch ? values_a[i] : values_b[i]), unitOpened_m.channelSettings[ch].range) );
            }
        }
            printf("\n");
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
                    fprintf ( fp, "%d, ", adc_to_mv ((!ch ? values_a[i] : values_b[i]), unitOpened_m.channelSettings[ch].range) );
                }
            }
            fprintf (fp, "\n");
        }
        fclose ( fp );
    }
    else
        ERROR("Cannot open the file data.txt for writing. \nPlease ensure that you have permission to access. \n");
*/

    //getch ();
}

void Acquisition3000::collect_fast_streaming_triggered (void)
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
    ok = ps3000_run_streaming_ns ( unitOpened_m.handle, 10, PS3000_US, BUFFER_SIZE_STREAMING, 1, 100, 30000 );
    DEBUG ( "OK: %d\n", ok );

    /* From here on, we can get data whenever we want...
    */

    while (!unitOpened_m.trigger.advanced.autoStop)
    {
        ps3000_get_streaming_last_values (unitOpened_m.handle, ps3000FastStreamingReady);
        if (nPreviousValues != unitOpened_m.trigger.advanced.totalSamples)
        {
            DEBUG ("Values collected: %ld\n", unitOpened_m.trigger.advanced.totalSamples - nPreviousValues);
            nPreviousValues =     unitOpened_m.trigger.advanced.totalSamples;
        }
        Sleep (0);
    }

    ps3000_stop (unitOpened_m.handle);

    no_of_samples = ps3000_get_streaming_values_no_aggregation (unitOpened_m.handle,
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
                    fprintf ( fp, "%d, ", adc_to_mv ((!ch ? values_a[i] : values_b[i]), unitOpened_m.channelSettings[ch].range) );
                }
            }
            fprintf (fp, "\n");
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
void Acquisition3000::get_info (void)
  {

  char description [6][25]=  { "Driver Version","USB Version","Hardware Version",
                              "Variant Info","Serial", "Error Code" };
  short         i;
  char      line [80];
  int    variant = MODEL_NONE;


  if( unitOpened_m.handle )
  {
    for ( i = 0; i < 5; i++ )
    {
      ps3000_get_unit_info ( unitOpened_m.handle, line, sizeof (line), i );
            if (i == 3)
            {
              variant = atoi(line);
            }
      printf ( "%s: %s\n", description[i], line );
    }

    switch (variant)
        {
        case MODEL_PS3206:
            unitOpened_m.model = MODEL_PS3206;
            unitOpened_m.external = TRUE;
            unitOpened_m.signalGenerator = TRUE;
            unitOpened_m.firstRange = PS3000_100MV;
            unitOpened_m.lastRange = PS3000_20V;
            unitOpened_m.maxTimebases = PS3206_MAX_TIMEBASE;
            unitOpened_m.timebases = unitOpened_m.maxTimebases;
            unitOpened_m.noOfChannels = DUAL_SCOPE;
            unitOpened_m.hasAdvancedTriggering = FALSE;
            unitOpened_m.hasEts = TRUE;
            unitOpened_m.hasFastStreaming = FALSE;
        break;

        case MODEL_PS3205:
            unitOpened_m.model = MODEL_PS3205;
            unitOpened_m.external = TRUE;
            unitOpened_m.signalGenerator = TRUE;
            unitOpened_m.firstRange = PS3000_100MV;
            unitOpened_m.lastRange = PS3000_20V;
            unitOpened_m.maxTimebases = PS3205_MAX_TIMEBASE;
            unitOpened_m.timebases = unitOpened_m.maxTimebases;
            unitOpened_m.noOfChannels = DUAL_SCOPE; 
            unitOpened_m.hasAdvancedTriggering = FALSE;
            unitOpened_m.hasEts = TRUE;
            unitOpened_m.hasFastStreaming = FALSE;
        break;
                
        case MODEL_PS3204:
            unitOpened_m.model = MODEL_PS3204;
            unitOpened_m.external = TRUE;
            unitOpened_m.signalGenerator = TRUE;
            unitOpened_m.firstRange = PS3000_100MV;
            unitOpened_m.lastRange = PS3000_20V;
            unitOpened_m.maxTimebases = PS3204_MAX_TIMEBASE;
            unitOpened_m.timebases = unitOpened_m.maxTimebases;
            unitOpened_m.noOfChannels = DUAL_SCOPE;
            unitOpened_m.hasAdvancedTriggering = FALSE;
            unitOpened_m.hasEts = TRUE;
            unitOpened_m.hasFastStreaming = FALSE;
        break;
                
        case MODEL_PS3223:
            unitOpened_m.model = MODEL_PS3223;
            unitOpened_m.external = FALSE;
            unitOpened_m.signalGenerator = FALSE;
            unitOpened_m.firstRange = PS3000_20MV;
            unitOpened_m.lastRange = PS3000_20V;
            unitOpened_m.maxTimebases = PS3224_MAX_TIMEBASE;
            unitOpened_m.timebases = unitOpened_m.maxTimebases;
            unitOpened_m.noOfChannels = DUAL_SCOPE;
            unitOpened_m.hasAdvancedTriggering = TRUE;
            unitOpened_m.hasEts = FALSE;
            unitOpened_m.hasFastStreaming = TRUE;
        break;

        case MODEL_PS3423:
            unitOpened_m.model = MODEL_PS3423;
            unitOpened_m.external = FALSE;
            unitOpened_m.signalGenerator = FALSE;
            unitOpened_m.firstRange = PS3000_20MV;
            unitOpened_m.lastRange = PS3000_20V;
            unitOpened_m.maxTimebases = PS3424_MAX_TIMEBASE;
            unitOpened_m.timebases = unitOpened_m.maxTimebases;
            unitOpened_m.noOfChannels = QUAD_SCOPE;                   
            unitOpened_m.hasAdvancedTriggering = TRUE;
            unitOpened_m.hasEts = FALSE;
            unitOpened_m.hasFastStreaming = TRUE;
        break;

        case MODEL_PS3224:
            unitOpened_m.model = MODEL_PS3224;
            unitOpened_m.external = FALSE;
            unitOpened_m.signalGenerator = FALSE;
            unitOpened_m.firstRange = PS3000_20MV;
            unitOpened_m.lastRange = PS3000_20V;
            unitOpened_m.maxTimebases = PS3224_MAX_TIMEBASE;
            unitOpened_m.timebases = unitOpened_m.maxTimebases;
            unitOpened_m.noOfChannels = DUAL_SCOPE;
            unitOpened_m.hasAdvancedTriggering = TRUE;
            unitOpened_m.hasEts = FALSE;
            unitOpened_m.hasFastStreaming = TRUE;
        break;

        case MODEL_PS3424:
            unitOpened_m.model = MODEL_PS3424;
            unitOpened_m.external = FALSE;
            unitOpened_m.signalGenerator = FALSE;
            unitOpened_m.firstRange = PS3000_20MV;
            unitOpened_m.lastRange = PS3000_20V;
            unitOpened_m.maxTimebases = PS3424_MAX_TIMEBASE;
            unitOpened_m.timebases = unitOpened_m.maxTimebases;
            unitOpened_m.noOfChannels = QUAD_SCOPE;       
            unitOpened_m.hasAdvancedTriggering = TRUE;
            unitOpened_m.hasEts = FALSE;
            unitOpened_m.hasFastStreaming = TRUE;
        break;

        case MODEL_PS3225:
            unitOpened_m.model = MODEL_PS3225;
            unitOpened_m.external = FALSE;
            unitOpened_m.signalGenerator = FALSE;
            unitOpened_m.firstRange = PS3000_100MV;
            unitOpened_m.lastRange = PS3000_400V;
            unitOpened_m.maxTimebases = PS3225_MAX_TIMEBASE;
            unitOpened_m.timebases = unitOpened_m.maxTimebases;
            unitOpened_m.noOfChannels = DUAL_SCOPE;
            unitOpened_m.hasAdvancedTriggering = TRUE;
            unitOpened_m.hasEts = FALSE;
            unitOpened_m.hasFastStreaming = TRUE;
        break;

        case MODEL_PS3425:
            unitOpened_m.model = MODEL_PS3425;
            unitOpened_m.external = FALSE;
            unitOpened_m.signalGenerator = FALSE;
            unitOpened_m.firstRange = PS3000_100MV;
            unitOpened_m.lastRange = PS3000_400V;
            unitOpened_m.timebases = PS3425_MAX_TIMEBASE;
            unitOpened_m.noOfChannels = QUAD_SCOPE;                   
            unitOpened_m.hasAdvancedTriggering = TRUE;
            unitOpened_m.hasEts = FALSE;
            unitOpened_m.hasFastStreaming = TRUE;
        break;

        default:
            printf("Unit not supported");
        }

        unitOpened_m.channelSettings [PS3000_CHANNEL_A].enabled = 1;
        unitOpened_m.channelSettings [PS3000_CHANNEL_A].DCcoupled = 1;
        unitOpened_m.channelSettings [PS3000_CHANNEL_A].range = unitOpened_m.lastRange;

        unitOpened_m.channelSettings [PS3000_CHANNEL_B].enabled = 0;
        unitOpened_m.channelSettings [PS3000_CHANNEL_B].DCcoupled = 1;
        unitOpened_m.channelSettings [PS3000_CHANNEL_B].range = unitOpened_m.lastRange;


        unitOpened_m.channelSettings [PS3000_CHANNEL_C].enabled = 0;
        unitOpened_m.channelSettings [PS3000_CHANNEL_C].DCcoupled = 1;
        unitOpened_m.channelSettings [PS3000_CHANNEL_C].range = unitOpened_m.lastRange;

        unitOpened_m.channelSettings [PS3000_CHANNEL_D].enabled = 0;
        unitOpened_m.channelSettings [PS3000_CHANNEL_D].DCcoupled = 1;
        unitOpened_m.channelSettings [PS3000_CHANNEL_D].range = unitOpened_m.lastRange;
  }
  else
  {
    printf ( "Unit Not Opened\n" );
    ps3000_get_unit_info ( unitOpened_m.handle, line, sizeof (line), PS3000_ERROR_CODE );
    printf ( "%s: %s\n", description[5], line );
        unitOpened_m.model = MODEL_NONE;
        unitOpened_m.external = TRUE;
        unitOpened_m.signalGenerator = TRUE;
        unitOpened_m.firstRange = PS3000_100MV;
        unitOpened_m.lastRange = PS3000_20V;
        unitOpened_m.timebases = PS3206_MAX_TIMEBASE;
        unitOpened_m.noOfChannels = QUAD_SCOPE;   
    }
}



void Acquisition3000::set_sig_gen (e_wave_type waveform, long frequency)
{

    PS3000_WAVE_TYPES waveform_ps3000 = PS3000_MAX_WAVE_TYPES;
    if (frequency < 1000 || frequency > PS3000_MAX_SIGGEN_FREQ)
    {
        ERROR("%s: Invalid frequency setted!\n",__FUNCTION__);
        return;
    }

    if (waveform < 0 || waveform >= E_WAVE_TYPE_TRIANGLE)
    {
        ERROR("%s: Invalid waveform setted!\n",__FUNCTION__);
        return;
    }
   
    switch(waveform)
    {
        case E_WAVE_TYPE_SQUARE:
            waveform_ps3000 = PS3000_SQUARE;
            break;
        case E_WAVE_TYPE_TRIANGLE:
            waveform_ps3000 = PS3000_TRIANGLE;
            break;
        case E_WAVE_TYPE_SINE:
        default:
            waveform_ps3000 = PS3000_SINE;
            break;
    }

    ps3000_set_siggen (unitOpened_m.handle,
                       waveform_ps3000,
                       (float)frequency,
                       (float)frequency,
                       0,
                       0, 
                       0,
                       0);
}

void Acquisition3000::set_sig_gen_arb (long int frequency)
{
//    char fileName [128];
//    FILE * fp;
    unsigned char arbitraryWaveform [4096];
    short waveformSize = 0;
    double delta;

    memset(&arbitraryWaveform, 0, 4096);

    if (frequency < 1000 || frequency > 10000000)
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


    delta = ((frequency * waveformSize) / 4096) * 4294967296.0 * 20e-9;
    //ps3000_set_siggen(unitOpened_m.handle, 0, 3000000, (unsigned long)delta, (unsigned long)delta, 0, 0, arbitraryWaveform, waveformSize, PS3000_UP, 0);
}

/****************************************************************************
 *
 * Select timebase, set oversample to on and time units as nano seconds
 *
 ****************************************************************************/
void Acquisition3000::set_timebase (double time_per_division)
  {
  short  i = 0;
  long   time_interval = 0;
  short  time_units = 0;
  short  oversample = 1;
  long   max_samples = 0;

  DEBUG ( "Specify timebase\n" );

  /* See what ranges are available...
   */
  for (i = 0; i < unitOpened_m.timebases; i++)
  {
      ps3000_get_timebase ( unitOpened_m.handle, i, BUFFER_SIZE, &time_interval, &time_units, oversample, &max_samples );
#ifdef TEST_WITHOUT_HW
      switch(i)
      {
      case 0:
          time_interval = 10;
          time_units = 0;
          break;
      case 1:
          time_interval = 10;
          time_units = 1;
          break;
      case 2:
          time_interval = 10;
          time_units = 2;
          break;
      case 3:
          time_interval = 10;
          time_units = 3;
          break;
      case 4:
          time_interval = 10;
          time_units = 4;
          break;
      case 6:
          time_interval = 100;
          time_units = 4;
          break;
      case 8:
          time_interval = 1000;
          time_units = 4;
          break;
      case 10:
          time_interval = 10000;
          time_units = 4;
          break;
      case 11:
          time_interval = 1000000;
          time_units = 4;
          break;
      default:
          time_interval = 0;
          time_units = 4;
          break;
      }


#endif
      if ( time_interval > 0 )
      {
          DEBUG ( "%d -> %ld %s  %hd\n", i, time_interval, adc_units(time_units), time_units );
          if((time_interval * adc_multipliers(time_units)) > (time_per_division * 5.)){
              timebase = i;
              break;
          }
      }
  }

#ifndef TEST_WITHOUT_HW
  ps3000_get_timebase ( unitOpened_m.handle, timebase, BUFFER_SIZE, &time_interval, &time_units, oversample, &max_samples );
#endif
  DEBUG ( "Timebase %d - %ld %s\n", timebase, time_interval, adc_units(time_units) );
  }

/****************************************************************************
 * Select input voltage ranges for channels A and B
 ****************************************************************************/
void Acquisition3000::set_voltages (channel_e channel_index, double volts_per_division)
{
    uint8_t i = 0;

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
/****************************************************************************
 * adc_to_mv
 *
 * If the user selects scaling to millivolts,
 * Convert an 12-bit ADC count into millivolts
 ****************************************************************************/
int Acquisition3000::adc_to_mv (long raw, int ch)
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
short Acquisition3000::mv_to_adc (short mv, short ch)
{
  return ( ( mv * 32767 ) / input_ranges[ch] );
}

#endif // HAVE_LIBPS3000
