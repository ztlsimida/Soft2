# Project Overview

This project is an embedded system software based on the C-Sky CK803S (TXW81x) chip platform, mainly designed for IoT devices and smart terminals. It integrates WiFi management (AP/STA/P2P), network configuration, pairing, low-power management, audio/video processing, and peripheral support (such as PSRAM, USB, SD card, LCD, etc.), suitable for customized hardware platforms.

---

## Main Features

- Supports WiFi AP/STA/P2P modes with flexible switching
- Automatic/manual network configuration, supports DHCP/static IP
- Device pairing and secure connection
- Low power and sleep management
- Audio and video stream processing and transmission
- Rich peripheral support (PSRAM, USB, SD card, LCD, etc.)
- Extensible command line and AT command interface
- Various test and demo modes (e.g., speed test, AI demo)

---

## Directory Structure

- `project/`  Main project directory, including main program, configs, scripts, etc.
- `libs/`     Static libraries for drivers and protocol support
- `sdk/`      SDK-related code, including HAL, drivers, OSAL, etc.
- `tools/`    Utility programs (e.g., firmware packaging, config generation)
- `csky/`     C-Sky kernel, algorithms, configs, etc.
- `doc/`      Documentation

---

## Build & Usage

### Environment Setup

1. It is recommended to use C-Sky Studio or a compatible embedded IDE.
2. Install the cross-compilation toolchain (e.g., csky-abiv2-elf-gcc).
3. Set up hardware connections (e.g., JTAG, UART).

### Build

- Open `project/fpv_app_umac4.cdkproj` in the IDE, select the target board/config, and build the firmware.
- Or use command-line scripts (e.g., `makecode.exe`, `prebuild.sh`, `BuildBIN.sh`) for automated build and packaging.

### Flash & Run

- Use `CSKYFlashProgramerConsole.bat` or related tools to flash the firmware to the target hardware.
- Interact with the device via UART or network interface for debugging and feature verification.

---

## Additional Notes

- Configuration files and parameters can be customized in `project/params/`, `project/psramCFG/`, etc.
- Supports various demo and test modes, can be tailored and extended as needed.
- For detailed API and development docs, refer to the `doc/` directory or source code comments.

---

For further assistance, please contact the project maintainer or refer to the documentation.
