/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "SDL_internal.h"

#ifndef SDL_asio_h_
#define SDL_asio_h_

#ifdef SDL_AUDIO_DRIVER_ASIO

#ifdef __cplusplus
extern "C" {
#endif

#include "../SDL_sysaudio.h"

// ASIO basic types and constants
typedef long ASIOSampleRate;
typedef long ASIOSampleType;
typedef long ASIOBool;
typedef long ASIOError;

// ASIO error codes
#define ASE_OK          0
#define ASE_SUCCESS     0x3f4847a0
#define ASE_NotPresent  -1000
#define ASE_HWMalfunction -999
#define ASE_InvalidParameter -998
#define ASE_InvalidMode -997
#define ASE_SPNotAdvancing -996
#define ASE_NoClock -995
#define ASE_NoMemory -994

// ASIO sample types
#define ASIOSTInt16MSB   0
#define ASIOSTInt24MSB   1    // used for 20 bits as well
#define ASIOSTInt32MSB   2
#define ASIOSTFloat32MSB 3    // IEEE 754 32 bit float
#define ASIOSTFloat64MSB 4    // IEEE 754 64 bit double float
#define ASIOSTInt32MSB16 8    // 32 bit data with 16 bit alignment
#define ASIOSTInt32MSB18 9    // 32 bit data with 18 bit alignment
#define ASIOSTInt32MSB20 10   // 32 bit data with 20 bit alignment
#define ASIOSTInt32MSB24 11   // 32 bit data with 24 bit alignment
#define ASIOSTInt16LSB   16
#define ASIOSTInt24LSB   17   // used for 20 bits as well
#define ASIOSTInt32LSB   18
#define ASIOSTFloat32LSB 19   // IEEE 754 32 bit float, as found on Intel x86 architecture
#define ASIOSTFloat64LSB 20   // IEEE 754 64 bit double float, as found on Intel x86 architecture
#define ASIOSTInt32LSB16 24   // 32 bit data with 16 bit alignment
#define ASIOSTInt32LSB18 25   // 32 bit data with 18 bit alignment
#define ASIOSTInt32LSB20 26   // 32 bit data with 20 bit alignment
#define ASIOSTInt32LSB24 27   // 32 bit data with 24 bit alignment

// ASIO structures
typedef struct ASIODriverInfo {
    long asioVersion;
    long driverVersion;
    char name[32];
    char errorMessage[124];
    void *sysRef;
} ASIODriverInfo;

typedef struct ASIOBufferInfo {
    ASIOBool isInput;
    long channelNum;
    void *buffers[2];
} ASIOBufferInfo;

typedef struct ASIOChannelInfo {
    long channel;
    ASIOBool isInput;
    ASIOBool isActive;
    long channelGroup;
    ASIOSampleType type;
    char name[32];
} ASIOChannelInfo;

typedef struct ASIOClockSource {
    long index;
    long associatedChannel;
    long associatedGroup;
    ASIOBool isCurrentSource;
    char name[32];
} ASIOClockSource;

typedef struct ASIOCallbacks {
    void (*bufferSwitch)(long doubleBufferIndex, ASIOBool directProcess);
    void (*sampleRateDidChange)(ASIOSampleRate sRate);
    long (*asioMessage)(long selector, long value, void* message, double* opt);
    void (*bufferSwitchTimeInfo)(struct ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
} ASIOCallbacks;

typedef struct ASIOTime {
    long reserved[4];
    struct {
        unsigned long hi, lo;
    } timeInfo;
    struct {
        double speed;
        struct {
            unsigned long hi, lo;
        } timeCodeSamples;
        unsigned long flags;
    } timeCode;
} ASIOTime;

// SDL ASIO private data structure
struct SDL_PrivateAudioData {
    ASIODriverInfo driver_info;
    ASIOBufferInfo *buffer_info;
    ASIOChannelInfo *channel_info;
    long input_channels;
    long output_channels;
    long buffer_size;
    ASIOSampleRate sample_rate;
    ASIOCallbacks callbacks;
    bool driver_loaded;
    bool buffers_created;
    bool driver_started;
    void *current_buffer;
    int current_buffer_size;
    SDL_AudioDevice *sdl_device;
};

// ASIO driver interface functions (will be loaded dynamically)
typedef ASIOError (*ASIOInit_t)(ASIODriverInfo *info);
typedef ASIOError (*ASIOExit_t)(void);
typedef ASIOError (*ASIOStart_t)(void);
typedef ASIOError (*ASIOStop_t)(void);
typedef ASIOError (*ASIOGetChannels_t)(long *numInputChannels, long *numOutputChannels);
typedef ASIOError (*ASIOGetBufferSize_t)(long *minSize, long *maxSize, long *preferredSize, long *granularity);
typedef ASIOError (*ASIOCanSampleRate_t)(ASIOSampleRate sampleRate);
typedef ASIOError (*ASIOGetSampleRate_t)(ASIOSampleRate *currentRate);
typedef ASIOError (*ASIOSetSampleRate_t)(ASIOSampleRate sampleRate);
typedef ASIOError (*ASIOGetClockSources_t)(ASIOClockSource *clocks, long *numSources);
typedef ASIOError (*ASIOSetClockSource_t)(long reference);
typedef ASIOError (*ASIOGetSamplePosition_t)(long long *sPos, long long *tStamp);
typedef ASIOError (*ASIOGetChannelInfo_t)(ASIOChannelInfo *info);
typedef ASIOError (*ASIOCreateBuffers_t)(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks *callbacks);
typedef ASIOError (*ASIODisposeBuffers_t)(void);
typedef ASIOError (*ASIOControlPanel_t)(void);
typedef ASIOError (*ASIOFuture_t)(long selector, void *opt);
typedef ASIOError (*ASIOOutputReady_t)(void);

// Function pointers for ASIO driver
extern ASIOInit_t pASIOInit;
extern ASIOExit_t pASIOExit;
extern ASIOStart_t pASIOStart;
extern ASIOStop_t pASIOStop;
extern ASIOGetChannels_t pASIOGetChannels;
extern ASIOGetBufferSize_t pASIOGetBufferSize;
extern ASIOCanSampleRate_t pASIOCanSampleRate;
extern ASIOGetSampleRate_t pASIOGetSampleRate;
extern ASIOSetSampleRate_t pASIOSetSampleRate;
extern ASIOGetClockSources_t pASIOGetClockSources;
extern ASIOSetClockSource_t pASIOSetClockSource;
extern ASIOGetSamplePosition_t pASIOGetSamplePosition;
extern ASIOGetChannelInfo_t pASIOGetChannelInfo;
extern ASIOCreateBuffers_t pASIOCreateBuffers;
extern ASIODisposeBuffers_t pASIODisposeBuffers;
extern ASIOControlPanel_t pASIOControlPanel;
extern ASIOFuture_t pASIOFuture;
extern ASIOOutputReady_t pASIOOutputReady;

// Internal functions
bool ASIO_LoadDriver(void);
void ASIO_UnloadDriver(void);
bool ASIO_InitializeDriver(void);
void ASIO_ShutdownDriver(void);

#ifdef __cplusplus
}
#endif

#endif // SDL_AUDIO_DRIVER_ASIO

#endif // SDL_asio_h_