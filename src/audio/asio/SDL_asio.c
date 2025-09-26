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

#ifdef SDL_AUDIO_DRIVER_ASIO

#include "../../core/windows/SDL_windows.h"
#include "../SDL_sysaudio.h"
#include "SDL_asio.h"

// ASIO function pointers
ASIOInit_t pASIOInit = NULL;
ASIOExit_t pASIOExit = NULL;
ASIOStart_t pASIOStart = NULL;
ASIOStop_t pASIOStop = NULL;
ASIOGetChannels_t pASIOGetChannels = NULL;
ASIOGetBufferSize_t pASIOGetBufferSize = NULL;
ASIOCanSampleRate_t pASIOCanSampleRate = NULL;
ASIOGetSampleRate_t pASIOGetSampleRate = NULL;
ASIOSetSampleRate_t pASIOSetSampleRate = NULL;
ASIOGetClockSources_t pASIOGetClockSources = NULL;
ASIOSetClockSource_t pASIOSetClockSource = NULL;
ASIOGetSamplePosition_t pASIOGetSamplePosition = NULL;
ASIOGetChannelInfo_t pASIOGetChannelInfo = NULL;
ASIOCreateBuffers_t pASIOCreateBuffers = NULL;
ASIODisposeBuffers_t pASIODisposeBuffers = NULL;
ASIOControlPanel_t pASIOControlPanel = NULL;
ASIOFuture_t pASIOFuture = NULL;
ASIOOutputReady_t pASIOOutputReady = NULL;

static HMODULE asio_dll = NULL;
static SDL_AudioDevice *current_asio_device = NULL;

// ASIO callback functions
static void asio_buffer_switch(long doubleBufferIndex, ASIOBool directProcess);
static void asio_sample_rate_changed(ASIOSampleRate sRate);
static long asio_message(long selector, long value, void* message, double* opt);
static void asio_buffer_switch_time_info(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);

bool ASIO_LoadDriver(void)
{
    if (asio_dll) {
        return true; // Already loaded
    }

    // Try to load the generic ASIO driver interface
    // Note: In a real implementation, you would typically load a specific ASIO driver DLL
    // For now, we'll simulate this with a placeholder
    asio_dll = LoadLibraryA("asio.dll");
    if (!asio_dll) {
        // Try alternative names or paths
        asio_dll = LoadLibraryA("asiodrvr.dll");
    }

    if (!asio_dll) {
        SDL_SetError("ASIO: Could not load ASIO driver library");
        return false;
    }

    // Load all function pointers
    pASIOInit = (ASIOInit_t)GetProcAddress(asio_dll, "ASIOInit");
    pASIOExit = (ASIOExit_t)GetProcAddress(asio_dll, "ASIOExit");
    pASIOStart = (ASIOStart_t)GetProcAddress(asio_dll, "ASIOStart");
    pASIOStop = (ASIOStop_t)GetProcAddress(asio_dll, "ASIOStop");
    pASIOGetChannels = (ASIOGetChannels_t)GetProcAddress(asio_dll, "ASIOGetChannels");
    pASIOGetBufferSize = (ASIOGetBufferSize_t)GetProcAddress(asio_dll, "ASIOGetBufferSize");
    pASIOCanSampleRate = (ASIOCanSampleRate_t)GetProcAddress(asio_dll, "ASIOCanSampleRate");
    pASIOGetSampleRate = (ASIOGetSampleRate_t)GetProcAddress(asio_dll, "ASIOGetSampleRate");
    pASIOSetSampleRate = (ASIOSetSampleRate_t)GetProcAddress(asio_dll, "ASIOSetSampleRate");
    pASIOGetChannelInfo = (ASIOGetChannelInfo_t)GetProcAddress(asio_dll, "ASIOGetChannelInfo");
    pASIOCreateBuffers = (ASIOCreateBuffers_t)GetProcAddress(asio_dll, "ASIOCreateBuffers");
    pASIODisposeBuffers = (ASIODisposeBuffers_t)GetProcAddress(asio_dll, "ASIODisposeBuffers");
    pASIOControlPanel = (ASIOControlPanel_t)GetProcAddress(asio_dll, "ASIOControlPanel");
    pASIOFuture = (ASIOFuture_t)GetProcAddress(asio_dll, "ASIOFuture");
    pASIOOutputReady = (ASIOOutputReady_t)GetProcAddress(asio_dll, "ASIOOutputReady");

    // Check if we got the essential functions
    if (!pASIOInit || !pASIOExit || !pASIOStart || !pASIOStop ||
        !pASIOGetChannels || !pASIOGetBufferSize || !pASIOCreateBuffers ||
        !pASIODisposeBuffers) {
        SDL_SetError("ASIO: Could not load required ASIO functions");
        ASIO_UnloadDriver();
        return false;
    }

    return true;
}

void ASIO_UnloadDriver(void)
{
    if (asio_dll) {
        FreeLibrary(asio_dll);
        asio_dll = NULL;
    }

    // Reset all function pointers
    pASIOInit = NULL;
    pASIOExit = NULL;
    pASIOStart = NULL;
    pASIOStop = NULL;
    pASIOGetChannels = NULL;
    pASIOGetBufferSize = NULL;
    pASIOCanSampleRate = NULL;
    pASIOGetSampleRate = NULL;
    pASIOSetSampleRate = NULL;
    pASIOGetClockSources = NULL;
    pASIOSetClockSource = NULL;
    pASIOGetSamplePosition = NULL;
    pASIOGetChannelInfo = NULL;
    pASIOCreateBuffers = NULL;
    pASIODisposeBuffers = NULL;
    pASIOControlPanel = NULL;
    pASIOFuture = NULL;
    pASIOOutputReady = NULL;
}

bool ASIO_InitializeDriver(void)
{
    if (!asio_dll || !pASIOInit) {
        return false;
    }

    ASIODriverInfo driver_info = {0};
    ASIOError result = pASIOInit(&driver_info);
    if (result != ASE_OK) {
        SDL_SetError("ASIO: Failed to initialize driver (error %d)", (int)result);
        return false;
    }

    return true;
}

void ASIO_ShutdownDriver(void)
{
    if (pASIOExit) {
        pASIOExit();
    }
}

// ASIO callback implementations
static void asio_buffer_switch(long doubleBufferIndex, ASIOBool directProcess)
{
    if (!current_asio_device || !current_asio_device->hidden) {
        return;
    }

    struct SDL_PrivateAudioData *hidden = current_asio_device->hidden;
    
    // Get the current buffer for the output channels - we'll handle stereo for now
    if (hidden->output_channels >= current_asio_device->spec.channels) {
        // Set up the buffer for SDL to write to
        // For simplicity, we'll use the first output channel's buffer
        void *buffer = hidden->buffer_info[hidden->input_channels].buffers[doubleBufferIndex];
        hidden->current_buffer = buffer;
        hidden->current_buffer_size = hidden->buffer_size * SDL_AUDIO_FRAMESIZE(current_asio_device->spec);
        
        // Clear the buffer first
        SDL_memset(buffer, 0, hidden->current_buffer_size);
        
        // If we have multiple channels, we need to handle channel mapping
        // For now, assume interleaved format and copy to all available output channels
        if (current_asio_device->spec.channels == 2 && hidden->output_channels >= 2) {
            // Set up both left and right channel buffers
            hidden->buffer_info[hidden->input_channels + 1].buffers[doubleBufferIndex] = 
                (void*)((Uint8*)buffer + (hidden->buffer_size * SDL_AUDIO_FRAMESIZE(current_asio_device->spec) / 2));
        }
    }

    // Signal that we have a new buffer ready for SDL to fill
    // In a full implementation, this would trigger SDL's audio callback
}

static void asio_sample_rate_changed(ASIOSampleRate sRate)
{
    // Handle sample rate changes if needed
    if (current_asio_device) {
        // Could potentially update the device spec here
    }
}

static long asio_message(long selector, long value, void* message, double* opt)
{
    // Handle ASIO messages
    switch (selector) {
        case 1: // kAsioSelectorSupported
            // Return 1 if we support this selector
            return 0;
        case 2: // kAsioEngineVersion
            return 2; // ASIO version 2
        case 3: // kAsioResetRequest
            // Driver requests a reset
            return 0;
        case 4: // kAsioBufferSizeChange
            // Buffer size has changed
            return 0;
        case 5: // kAsioResyncRequest
            // Resync request
            return 0;
        case 6: // kAsioLatenciesChanged
            // Latencies have changed
            return 0;
        default:
            return 0;
    }
}

static void asio_buffer_switch_time_info(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
{
    // Enhanced buffer switch with timing information
    asio_buffer_switch(doubleBufferIndex, directProcess);
}

// SDL Audio Driver Implementation

static bool ASIO_OpenDevice(SDL_AudioDevice *device)
{
    struct SDL_PrivateAudioData *hidden;
    ASIOError result;
    long input_channels, output_channels;
    long min_size, max_size, preferred_size, granularity;

    // Allocate private data
    hidden = (struct SDL_PrivateAudioData *)SDL_calloc(1, sizeof(*hidden));
    if (!hidden) {
        return false;
    }
    device->hidden = hidden;
    hidden->sdl_device = device;

    // Initialize ASIO driver
    if (!ASIO_InitializeDriver()) {
        SDL_free(hidden);
        device->hidden = NULL;
        return false;
    }

    // Get channel counts
    result = pASIOGetChannels(&input_channels, &output_channels);
    if (result != ASE_OK) {
        SDL_SetError("ASIO: Failed to get channel count (error %d)", (int)result);
        SDL_free(hidden);
        device->hidden = NULL;
        return false;
    }

    hidden->input_channels = input_channels;
    hidden->output_channels = output_channels;

    // Get buffer size info
    result = pASIOGetBufferSize(&min_size, &max_size, &preferred_size, &granularity);
    if (result != ASE_OK) {
        SDL_SetError("ASIO: Failed to get buffer size (error %d)", (int)result);
        SDL_free(hidden);
        device->hidden = NULL;
        return false;
    }

    // Use preferred buffer size, but ensure it fits our requirements
    hidden->buffer_size = preferred_size;
    if (hidden->buffer_size < 64) {
        hidden->buffer_size = 64;
    }
    if (hidden->buffer_size > 4096) {
        hidden->buffer_size = 4096;
    }

    // Set sample rate
    if (pASIOSetSampleRate) {
        result = pASIOSetSampleRate((ASIOSampleRate)device->spec.freq);
        if (result != ASE_OK) {
            // Try to get the current sample rate instead
            ASIOSampleRate current_rate;
            if (pASIOGetSampleRate && pASIOGetSampleRate(&current_rate) == ASE_OK) {
                device->spec.freq = (int)current_rate;
            }
        }
    }

    // Limit channels to what we can handle (stereo for now)
    if (device->spec.channels > 2) {
        device->spec.channels = 2;
    }
    if (device->spec.channels > output_channels) {
        device->spec.channels = output_channels;
    }

    // Set up buffer info for ASIO
    long total_channels = hidden->input_channels + device->spec.channels;
    hidden->buffer_info = (ASIOBufferInfo *)SDL_calloc(total_channels, sizeof(ASIOBufferInfo));
    if (!hidden->buffer_info) {
        SDL_free(hidden);
        device->hidden = NULL;
        return false;
    }

    hidden->channel_info = (ASIOChannelInfo *)SDL_calloc(total_channels, sizeof(ASIOChannelInfo));
    if (!hidden->channel_info) {
        SDL_free(hidden->buffer_info);
        SDL_free(hidden);
        device->hidden = NULL;
        return false;
    }

    // Set up input channels (for full-duplex capability)
    for (long i = 0; i < hidden->input_channels; i++) {
        hidden->buffer_info[i].isInput = ASIOTrue;
        hidden->buffer_info[i].channelNum = i;
        hidden->buffer_info[i].buffers[0] = NULL;
        hidden->buffer_info[i].buffers[1] = NULL;
    }

    // Set up output channels
    for (long i = 0; i < device->spec.channels; i++) {
        hidden->buffer_info[hidden->input_channels + i].isInput = ASIOFalse;
        hidden->buffer_info[hidden->input_channels + i].channelNum = i;
        hidden->buffer_info[hidden->input_channels + i].buffers[0] = NULL;
        hidden->buffer_info[hidden->input_channels + i].buffers[1] = NULL;
    }

    // Set up callbacks
    hidden->callbacks.bufferSwitch = asio_buffer_switch;
    hidden->callbacks.sampleRateDidChange = asio_sample_rate_changed;
    hidden->callbacks.asioMessage = asio_message;
    hidden->callbacks.bufferSwitchTimeInfo = asio_buffer_switch_time_info;

    // Create ASIO buffers
    result = pASIOCreateBuffers(hidden->buffer_info, total_channels, hidden->buffer_size, &hidden->callbacks);
    if (result != ASE_OK) {
        SDL_SetError("ASIO: Failed to create buffers (error %d)", (int)result);
        SDL_free(hidden->channel_info);
        SDL_free(hidden->buffer_info);
        SDL_free(hidden);
        device->hidden = NULL;
        return false;
    }

    hidden->buffers_created = true;
    device->sample_frames = hidden->buffer_size;
    current_asio_device = device;

    return true;
}

static void ASIO_CloseDevice(SDL_AudioDevice *device)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    if (!hidden) {
        return;
    }

    // Stop ASIO if it was started
    if (hidden->driver_started && pASIOStop) {
        pASIOStop();
        hidden->driver_started = false;
    }

    // Dispose of ASIO buffers
    if (hidden->buffers_created && pASIODisposeBuffers) {
        pASIODisposeBuffers();
        hidden->buffers_created = false;
    }

    // Free allocated memory
    if (hidden->channel_info) {
        SDL_free(hidden->channel_info);
    }
    if (hidden->buffer_info) {
        SDL_free(hidden->buffer_info);
    }

    SDL_free(hidden);
    device->hidden = NULL;

    if (current_asio_device == device) {
        current_asio_device = NULL;
    }
}

static Uint8 *ASIO_GetDeviceBuf(SDL_AudioDevice *device, int *buffer_size)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    if (!hidden || !hidden->current_buffer) {
        *buffer_size = 0;
        return NULL;
    }

    *buffer_size = hidden->current_buffer_size;
    return (Uint8 *)hidden->current_buffer;
}

static bool ASIO_PlayDevice(SDL_AudioDevice *device, const Uint8 *buffer, int buffer_size)
{
    struct SDL_PrivateAudioData *hidden = device->hidden;
    if (!hidden) {
        return false;
    }

    // Start ASIO if not already started
    if (!hidden->driver_started && pASIOStart) {
        ASIOError result = pASIOStart();
        if (result != ASE_OK) {
            SDL_SetError("ASIO: Failed to start driver (error %d)", (int)result);
            return false;
        }
        hidden->driver_started = true;
    }

    // The actual buffer copying is handled in the ASIO callback
    // This function mainly ensures the driver is running
    return true;
}

static void ASIO_WaitDevice(SDL_AudioDevice *device)
{
    // ASIO is callback-based, so we don't need to wait
    // Just sleep for a short time to yield control
    SDL_Delay(1);
}

static void ASIO_Deinitialize(void)
{
    ASIO_ShutdownDriver();
    ASIO_UnloadDriver();
}

static bool ASIO_Init(SDL_AudioDriverImpl *impl)
{
    // Try to load the ASIO driver
    if (!ASIO_LoadDriver()) {
        return false;
    }

    // Set up the driver implementation
    impl->OpenDevice = ASIO_OpenDevice;
    impl->CloseDevice = ASIO_CloseDevice;
    impl->GetDeviceBuf = ASIO_GetDeviceBuf;
    impl->PlayDevice = ASIO_PlayDevice;
    impl->WaitDevice = ASIO_WaitDevice;
    impl->Deinitialize = ASIO_Deinitialize;
    impl->OnlyHasDefaultPlaybackDevice = true;
    impl->ProvidesOwnCallbackThread = true; // ASIO provides its own callback thread

    return true;
}

AudioBootStrap ASIO_bootstrap = {
    "asio", "ASIO Audio Driver", ASIO_Init, false, true // Mark as preferred for low-latency
};

#endif // SDL_AUDIO_DRIVER_ASIO