# Plugin H264 for Solar2D

A high-performance H.264 video decoder plugin for Solar2D (Corona SDK) with minimal dependencies.

## Overview

This plugin provides:
- H.264 video decoding using OpenH264
- AAC audio decoding using FDK-AAC  
- MP4 container support using MiniMP4
- Full compatibility with existing plugin_movie API
- Cross-platform support (iOS, Android, macOS, Windows)
- Small library size (~2.5MB total)

## Building

### Prerequisites

- CMake 3.10+
- C++11 compatible compiler
- Git (for submodules)

### Setup

```bash
# Clone with submodules
git clone --recursive https://github.com/labolado/plugin_H264.git
cd plugin_H264

# Or if already cloned
git submodule update --init --recursive
```

### macOS Build

```bash
# Build third-party libraries
cd third_party/openh264
make -j8

cd ../fdk-aac
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8

# Build plugin
cd ../../..
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8

# Run tests
make test
```

### Project Structure

```
plugin_H264/
├── docs/                    # Documentation
├── include/                 # Header files
│   ├── decoders/           # Decoder interfaces
│   ├── managers/           # Manager classes
│   └── utils/              # Utility classes
├── src/                    # Source files
├── tests/                  # Unit and integration tests
├── third_party/            # Git submodules
│   ├── openh264/           # OpenH264 codec
│   ├── fdk-aac/           # FDK-AAC codec
│   └── minimp4/           # MP4 demuxer
├── tools/                  # Build tools and scripts
└── validation/            # Technical validation tests
```

## Development Status

Based on comprehensive technical validation completed on 2025-08-29:

- ✅ OpenH264 2.6.0 API validation
- ✅ FDK-AAC 2.0.3 integration  
- ✅ MiniMP4 single-header validation
- ✅ Cross-platform compilation (macOS)
- 🚧 Phase 1: Core decoder implementation (in progress)
- ⏳ Phase 2: Audio-video synchronization
- ⏳ Phase 3: Solar2D integration  
- ⏳ Phase 4: Cross-platform optimization

## License

- Plugin code: MIT License
- OpenH264: BSD 2-Clause License
- FDK-AAC: Custom license (allows commercial use)
- MiniMP4: MIT License

## Contributing

Please follow the established coding standards and ensure all tests pass before submitting pull requests.