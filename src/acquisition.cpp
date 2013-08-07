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
 * @file acquisition.cpp
 * @brief Definition of Acquisition class.
 * Acquisition methods using libps2000 to drive Picoscope 2000 series HW
 * @version 0.1
 * @date 2012, november 27
 * @author Vincent HERVIEUX    -   11.27.2012   -   initial creation
 */

#include "acquisition.h"
#include "acquisition2000.h"
#include "acquisition3000.h"

#ifndef WIN32
#define Sleep(x) usleep(1000*(x))
enum BOOL {FALSE,TRUE};
#endif

/* static members initialization */
Acquisition *Acquisition::singleton_m = NULL;
const char * Acquisition::known_adc_units[] = { "ADC", "fs", "ps", "ns", "us", "ms"};
const char * Acquisition::unknown_adc_units = "Not Known";

/****************************************************************************
 *
 * constructor
 *
 ****************************************************************************/
Acquisition::Acquisition()
{
    DEBUG( "Acquisition model construction...\n");
    thread_id = 0;
    trigger_slope_m = E_TRIGGER_AUTO;
    trigger_level_m = 0.;

}

/****************************************************************************
 *
 * get_instance
 *
 ****************************************************************************/
Acquisition* Acquisition::get_instance()
{
    device_info_t info;
    if(NULL == Acquisition::singleton_m)
    {
        // TODO Need to choose in a dynamic way between 2000 and 3000 series here..
        do
        {
#ifdef HAVE_LIBPS2000
            Acquisition::singleton_m = Acquisition2000::get_instance();
            memset(&info, 0, sizeof(device_info_t));
            Acquisition::singleton_m->get_device_info(&info);
            if(0 == strncmp( info.device_name, "No device or device not supported", DEVICE_NAME_MAX))
            {
                DEBUG("No Picoscope 2000 series found.\n");
                delete((Acquisition2000*)Acquisition::singleton_m);
                Acquisition::singleton_m = NULL;
            }
            else
            {
                break;
            }
#endif
#ifdef HAVE_LIBPS2000A
            Acquisition::singleton_m = Acquisition2000a::get_instance();
            memset(&info, 0, sizeof(device_info_t));
            Acquisition::singleton_m->get_device_info(&info);
            if(0 == strncmp( info.device_name, "No device or device not supported", DEVICE_NAME_MAX))
            {
                DEBUG("No Picoscope 2000a series found.\n");
                delete((Acquisition2000a*)Acquisition::singleton_m);
                Acquisition::singleton_m = NULL;
            }
            else
            {
                break;
            }
#endif
#ifdef HAVE_LIBPS3000
            Acquisition::singleton_m = Acquisition3000::get_instance();
            memset(&info, 0, sizeof(device_info_t));
            Acquisition::singleton_m->get_device_info(&info);
            if(0 == strncmp( info.device_name, "No device or device not supported", DEVICE_NAME_MAX))
            {
                DEBUG("No Picoscope 3000 series found.\n");
                delete((Acquisition3000*)Acquisition::singleton_m);
                Acquisition::singleton_m = NULL;
            }
            else
            {
                break;
            }
#endif
        }while(0);
    }

    return Acquisition::singleton_m;
}

/****************************************************************************
 *
 * destructor
 *
 ****************************************************************************/
Acquisition::~Acquisition()
{
    if( thread_id )
        stop();
}

/****************************************************************************
 *
 * adc_units
 *
 ****************************************************************************/
const char * Acquisition::adc_units (short time_units)
{
  time_units++;
  switch ( time_units )
    {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
          return known_adc_units[time_units];
    }
  return unknown_adc_units;
}
/****************************************************************************
 *
 * adc_multipliers
 *
 ****************************************************************************/
double Acquisition::adc_multipliers (short time_units)
{
  time_units++;
  switch ( time_units )
    {
    case 1:
      return 1E-15;
    case 2:
      return 1E-12;
    case 3:
      return 1E-9;
    case 4:
      return 1E-6;
    case 5:
      return 1E-3;
    case 0:
    default:
      return 0.;
    }
}

/****************************************************************************
 * Start acquisition thread
 ****************************************************************************/
void Acquisition::start(void)
{
    int ret = 0;
    if(0 == thread_id)
    {
        sem_init(&thread_stop, 0, 0);
        ret = pthread_create(&thread_id, NULL, Acquisition::threadAcquisition, NULL);
        if( 0 != ret )
        {
            ERROR("pthread_create failed and returned %d\n", ret);
            thread_id = 0;
        }
        else
        {
            DEBUG("thread id is %lu\n", thread_id);
        }
    }
}
/****************************************************************************
 * Stop acquisition thread
 ****************************************************************************/
void Acquisition::stop(void)
{
    if( 0 != thread_id )
    {
        DEBUG("thread id is %lu\n", thread_id);
        sem_post(&thread_stop);
        pthread_join(thread_id, NULL);
        thread_id = 0;
    }
}

/****************************************************************************
 * Start acquisition thread
 ****************************************************************************/
void* Acquisition::threadAcquisition(void* arg)
{
    Acquisition *acquisition = Acquisition::get_instance();
    (void)arg;
    if ( NULL != acquisition )
    {
         /* 
          * May not be supported by all devices... 
          * acquisition->collect_streaming();
          */

         /*
          * Acquisition might be triggered or not...
          */
         if(acquisition->trigger_slope_m == E_TRIGGER_AUTO)
         {
             acquisition->collect_block_immediate();
         }
         else
         {
             acquisition->collect_block_triggered(acquisition->trigger_slope_m, acquisition->trigger_level_m);
         }
    }
    else
    {
        ERROR("Failed to retrieve acquisition's instance\n");
    }
   
   pthread_exit(NULL); 
}

/****************************************************************************
 * set trigger
 ****************************************************************************/
void Acquisition::set_trigger (trigger_e trigger_slope, double trigger_level)
{
   trigger_slope_m = trigger_slope;
   trigger_level_m = trigger_level;
}
