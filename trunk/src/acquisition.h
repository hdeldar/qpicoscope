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
 * @file acquisition.h
 * @brief Declaration of Acquisition class.
 * Acquisition methods using libps2000 to drive Picoscope 2000 series HW
 * @version 0.1
 * @date 2012, november 27
 * @author Vincent HERVIEUX    -   11.27.2012   -   initial creation
 */
#ifndef ACQUISITION_H
#define ACQUISITION_H

#include <string>
#include <vector>

#include "oscilloscope.h"
#include "drawdata.h"

#ifdef WIN32
/* Headers for Windows */
#include "windows.h"
#include <conio.h>

#else
/* Headers for Linux */
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

/* Definition of PS2000 driver routines on Linux */
//#define DYNLINK
#define __stdcall
/* End of Linux-specific definitions */
#endif

#define BUFFER_SIZE           1024
#define BUFFER_SIZE_STREAMING 100000
#define MAX_CHANNELS          4

#define DEVICE_NAME_MAX       80
#define CHANNEL_OFF           99

class Acquisition{
public:
    /**
     * @brief public typedef declarations
     */
    typedef enum
    {
        E_WAVE_TYPE_SINE = 0,
        E_WAVE_TYPE_SQUARE,
        E_WAVE_TYPE_TRIANGLE,
        E_WAVE_TYPE_RAMP_UP,
        E_WAVE_TYPE_RAMP_DOWN,
        E_WAVE_TYPE_DC_VOLTAGE
    }e_wave_type;

    typedef struct
    {
        double value;
        std::string name;
    }volt_item_t;

    typedef struct
    {
        char    device_name[DEVICE_NAME_MAX];
        uint8_t nb_channels;
    }device_info_t;

    typedef enum
    {
        CHANNEL_A = 0,
        CHANNEL_B,
        CHANNEL_C,
        CHANNEL_D,
        CHANNEL_MAX
    }channel_e;

    /** @brief get singleton instance */
    static Acquisition* get_instance();
    /** @brief destructor */
    virtual ~Acquisition();
    /**
     * @brief set input voltage range
     * @param[in] : the channel index (0 for channel A, 1 for channel B, etc)
     * @param[in] : volts per division caliber
     */
    virtual void set_voltages (channel_e channel_index, double volts_per_division) = 0;
    /**
     * @brief set input time base
     * @param[in] : time per division valiber
     */
    virtual void set_timebase (double time_per_division) = 0;
    /**
     * @brief set input trigger type
     * @param[in] : trigger slope, or AUTO
     * @param[in] : trigger level
     */
    void set_trigger (trigger_e trigger_slope, double trigger_level);
    /**
     * @brief set signal generator (2200 series only) 
     * @param[in] : waveform type 
     * @param[in] : frequency in Hertz
     */
    virtual void set_sig_gen (e_wave_type waveform, long frequency) = 0;
    /**
     * @brief set signal generator arbitrary (2200 series only) 
     * @param[in] : frequency in Hertz
     */
    virtual void set_sig_gen_arb (long int frequency) = 0;
    /**
     * @brief get device informations 
     */
    virtual void get_device_info(device_info_t* info) = 0;
    /**
     * @brief start acquisition thread
     */
    void start(void);
    /**
     * @brief stop acquisition thread
     */
    void stop(void);
    /**
     * @brief set DrawData Class
     */
    void setDrawData(DrawData *drawdata) { draw = drawdata; }
protected:
    /**
     * @brief protected methods declarations
     */
    Acquisition();
    static const char * known_adc_units[]/* = { "ADC", "fs", "ps", "ns", "us", "ms"}*/;
    static const char * unknown_adc_units/* = "Not Known"*/;
    const char * adc_units (short time_units);
    double adc_multipliers (short time_units);
    virtual int adc_to_mv (long raw, int ch) = 0;
    virtual short mv_to_adc (short mv, short ch) = 0;
    virtual void get_info (void) = 0;
    virtual void set_defaults (void) = 0;
    virtual void set_trigger_advanced(void) = 0;
    virtual void collect_block_immediate (void) = 0;
    virtual void collect_block_triggered (trigger_e trigger_slope, double trigger_level) = 0;
    virtual void collect_block_advanced_triggered (void) = 0;
    virtual void collect_block_ets (void) = 0;
    virtual void collect_streaming (void) = 0;
    virtual void collect_fast_streaming (void) = 0;
    virtual void collect_fast_streaming_triggered (void) = 0;
    /**
     * @brief protected members declarations
     */

    sem_t thread_stop;
    DrawData *draw;
    trigger_e trigger_slope_m;
    double trigger_level_m;
private:
    /**
     * @brief private typedef declarations
     */


    /**
     * @brief private methods declarations
     */
    static void* threadAcquisition(void *arg);
    /**
     * Store singleton output from the factory
     */
    static Acquisition *singleton_m;
    pthread_t thread_id;
};

#endif // ACQUISITION_H
