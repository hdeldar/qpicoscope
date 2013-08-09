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
 * @file acquisition2000a.h
 * @brief Declaration of Acquisition2000a class.
 * Acquisition methods using libps2000a to drive Picoscope 2000a series HW
 * @version 0.1
 * @date 2013, january 4
 * @author Vincent HERVIEUX    -   01.04.2014   -   initial creation
 */
#ifndef ACQUISITION2000A_H
#define ACQUISITION2000A_H

#include "../qpicoscope-config.h"

#ifdef HAVE_LIBPS2000A

#include <string>
#include <vector>

#include "oscilloscope.h"
#include "drawdata.h"
#include "acquisition.h"

#ifdef WIN32
/* Headers for Windows */
#include "windows.h"
#include <conio.h>

/* Definitions of PS2000 driver routines on Windows*/
#include "ps2000aApi.h"

#else
/* Headers for Linux */
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

/* Definition of PS2000a driver routines on Linux */
//#define DYNLINK
#include <libps2000a-1.0/ps2000aApi.h>
#define __stdcall
/* End of Linux-specific definitions */
#endif


class Acquisition2000a : public Acquisition{
public:
    /**
     * @brief public typedef declarations
     */

    typedef struct
    {
        double value;
        std::string name;
    }volt_item_t;

    /** @brief get singleton instance */
    static Acquisition2000a* get_instance();
    /** @brief destructor */
    virtual ~Acquisition2000a();
    /**
     * @brief set input voltage range
     * @param[in] : the channel index (0 for channel A, 1 for channel B, etc)
     * @param[in] : volts per division caliber
     */
    void set_voltages (channel_e channel_index, double volts_per_division); // OK
    /**
     * @brief set input time base
     * @param[in] : time per division valiber
     */
    void set_timebase (double time_per_division); // OK
    /**
     * @brief set AC/DC
     * @param[in] : a current_e value (0 = AC, 1 = DC)
     */
    void set_DC_coupled(current_e coupling); // OK
    /**
     * @brief set signal generator (2200 series only) 
     * @param[in] : waveform type 
     * @param[in] : frequency in Hertz
     */
    void set_sig_gen (e_wave_type waveform, long frequency); // OK
    /**
     * @brief set signal generator arbitrary (2200 series only) 
     * @param[in] : frequency in Hertz
     */
    void set_sig_gen_arb (long int frequency); // OK
    /**
     * @brief get device informations 
     */
    void get_device_info(device_info_t* info); // OK
private:
    /**
     * @brief private typedef declarations
     */
    typedef enum {
        MODEL_NONE = 0,
        MODEL_PS2205MSO = 2205,
        MODEL_PS2206	 = 2206,
        MODEL_PS2207	 = 2207,
        MODEL_PS2208	 = 2208
    } MODEL_TYPE;

    typedef struct
    {
        PS2000A_THRESHOLD_DIRECTION    channelA;
        PS2000A_THRESHOLD_DIRECTION    channelB;
        PS2000A_THRESHOLD_DIRECTION    channelC;
        PS2000A_THRESHOLD_DIRECTION    channelD;
        PS2000A_THRESHOLD_DIRECTION    ext;
        PS2000A_THRESHOLD_DIRECTION    aux;
    } DIRECTIONS;

    typedef struct
    {
        PS2000A_PWQ_CONDITIONS*     conditions;
        short                       nConditions;
        PS2000A_THRESHOLD_DIRECTION direction;
        unsigned long               lower;
        unsigned long               upper;
        PS2000A_PULSE_WIDTH_TYPE    type;
    } PULSE_WIDTH_QUALIFIER;


    typedef struct
    {
        PS2000A_CHANNEL channel;
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
        PS2000A_RANGE firstRange;
        PS2000A_RANGE lastRange;
        TRIGGER_CHANNEL trigger;
        short maxTimebase;
        short timebases;
        short noOfChannels;
	    short maxValue;
        CHANNEL_SETTINGS channelSettings[PS2000A_MAX_CHANNELS];
        short                hasAdvancedTriggering;
        short                hasFastStreaming;
        short                hasEts;
        short                hasSignalGenerator;
    } UNIT_MODEL;
    /**
     * @brief private methods declarations
     */
    Acquisition2000a();                   // OK
    int adc_to_mv (long raw, int ch);     // OK
    short mv_to_adc (short mv, short ch); // OK
    void get_info (void);
    void set_defaults (void); // OK
    PICO_STATUS set_trigger(PS2000A_TRIGGER_CHANNEL_PROPERTIES * channelProperties,
                            short nChannelProperties,
                            PS2000A_TRIGGER_CONDITIONS * triggerConditions,
                            short nTriggerConditions,
                            TRIGGER_DIRECTIONS * directions,
                            PWQ * pwq,
                            unsigned long delay,
                            short auxOutputEnabled,
                            long autoTriggerMs,
                            PS2000A_DIGITAL_CHANNEL_DIRECTIONS * digitalDirections,
                            short nDigitalDirections);
    void set_trigger_advanced(void);
    void collect_block_immediate (void);
    void collect_block_triggered (trigger_e trigger_slope, double trigger_level);    // OK
    void collect_block_advanced_triggered ();
    void collect_block_ets (void);
    void collect_streaming (void);
    void collect_fast_streaming (void);
    void collect_fast_streaming_triggered (void);
    static void  __stdcall ps2000FastStreamingReady( short **overviewBuffers,
                                                     short overflow,
                                                     unsigned long triggeredAt,
                                                     short triggered,
                                                     short auto_stop,
                                                     unsigned long nValues);
    /**
     * @brief private instances declarations
     */
    UNIT_MODEL unitOpened_m;
    static Acquisition2000a *singleton_m;
    short timebase;
    double time_per_division_m;
    long times[BUFFER_SIZE];
    static const short input_ranges [PS2000A_MAX_RANGES] /*= {10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000}*/;
};

#endif // HAVE_LIBPS2000A
#endif // ACQUISITION2000A_H
