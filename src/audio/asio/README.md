# SDL ASIO Audio Driver

This directory contains the ASIO (Audio Stream Input/Output) audio driver for SDL3, providing low-latency audio support on Windows systems.

## Overview

ASIO is a professional audio interface standard developed by Steinberg that provides:
- Ultra-low latency audio (typically 1-10ms)
- Direct hardware access bypassing Windows audio layers
- Sample-accurate timing
- Multi-channel audio support
- Professional audio quality

## Requirements

To use the ASIO driver, you need:

1. **ASIO Driver**: An ASIO-compatible audio driver for your hardware
   - Professional audio interfaces usually include ASIO drivers
   - For consumer audio hardware, you can use universal ASIO drivers like:
     - ASIO4ALL (free universal ASIO driver)
     - FlexASIO (open-source universal ASIO driver)

2. **ASIO SDK**: For production use, download the official ASIO SDK from Steinberg
   - This implementation uses public interface definitions
   - The full SDK includes additional features and documentation

## Implementation Status

This is a basic ASIO driver implementation that provides:

✅ **Implemented:**
- ASIO driver loading and initialization
- Basic stereo audio output
- Low-latency buffer management
- Integration with SDL audio system
- Dynamic driver discovery

⚠️ **Limitations:**
- Simplified audio format handling (16-bit PCM focus)
- Basic channel mapping (stereo output)
- No recording support yet
- Limited error handling and recovery

🔧 **For Production Use:**
- Add COM interface support for proper ASIO driver enumeration
- Implement full sample format support (24-bit, 32-bit, float)
- Add multi-channel audio support
- Implement recording capabilities
- Add comprehensive error handling
- Add ASIO control panel integration

## Usage

The ASIO driver will be automatically selected as the preferred audio driver on Windows systems when:
1. An ASIO driver is installed and available
2. SDL is compiled with ASIO support enabled
3. No other preferred driver is explicitly specified

To force ASIO usage:
```c
SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "asio");
```

## Building

The ASIO driver is enabled by default in Windows builds. To disable:
```bash
cmake -DSDL_ASIO=OFF
```

## Performance

ASIO provides the lowest possible audio latency on Windows:
- Typical latency: 1-10ms (vs. 20-50ms for DirectSound/WASAPI)
- CPU usage: Low (direct hardware access)
- Jitter: Minimal (hardware clock synchronization)

This makes it ideal for:
- Real-time audio applications
- Digital Audio Workstations (DAWs)
- Live audio processing
- Professional audio software
- Games requiring precise audio timing

## Troubleshooting

**Driver not found:**
- Install an ASIO driver (ASIO4ALL is a good universal option)
- Ensure the ASIO driver DLL is in the system PATH

**High latency:**
- Check ASIO driver settings/control panel
- Reduce buffer size in ASIO driver configuration
- Close other audio applications

**Audio dropouts:**
- Increase buffer size slightly
- Check system performance and CPU usage
- Ensure adequate system RAM

## License Notes

This implementation uses publicly available ASIO interface definitions. For production use with the full ASIO SDK, please review Steinberg's license terms at:
https://www.steinberg.net/asiosdk