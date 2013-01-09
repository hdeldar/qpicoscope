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
}

/****************************************************************************
 *
 * get_instance
 *
 ****************************************************************************/
Acquisition* Acquisition::get_instance()
{
    if(NULL == Acquisition::singleton_m)
    {
        // TODO Need to choose in a dynamic way between 2000 and 3000 series here..
        WARNING( "Need to choose in a dynamic way between 2000 and 3000 series here...\n");
        //Acquisition::singleton_m = Acquisition2000::get_instance();
        Acquisition::singleton_m = Acquisition3000::get_instance();
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
    sem_init(&thread_stop, 0, 0);
    ret = pthread_create(&thread_id, NULL, Acquisition::threadAcquisition, NULL);
    if( 0 != thread_id)
    {
        ERROR("pthread_create failed and returned %d\n", ret);
    }
}
/****************************************************************************
 * Stop acquisition thread
 ****************************************************************************/
void Acquisition::stop(void)
{
    sem_post(&thread_stop);
    pthread_join(thread_id, NULL);
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
         acquisition->collect_fast_streaming();
    }
    else
    {
        ERROR("Failed to retrieve acquisition's instance\n");
    }
    
   return NULL;
}
