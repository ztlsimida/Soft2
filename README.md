# 项目简介 / Project Overview

本项目是基于 C-Sky CK803S (TXW81x) 芯片平台的嵌入式系统软件，主要用于物联网设备、智能终端等场景。项目集成了 WiFi 管理（支持 AP/STA/P2P）、网络配置、配对、低功耗管理、音视频处理、外设支持（如 PSRAM、USB、SD 卡、LCD 等）等功能，适用于定制化硬件平台。

This project is an embedded system software based on the C-Sky CK803S (TXW81x) chip platform, mainly designed for IoT devices and smart terminals. It integrates WiFi management (AP/STA/P2P), network configuration, pairing, low-power management, audio/video processing, and peripheral support (such as PSRAM, USB, SD card, LCD, etc.), suitable for customized hardware platforms.

---

## 主要功能 / Main Features

- 支持 WiFi AP/STA/P2P 模式，灵活切换
- 网络自动/手动配置，支持 DHCP/静态 IP
- 设备配对与安全连接
- 低功耗与休眠管理
- 音频、视频流处理与传输
- 丰富的外设支持（PSRAM、USB、SD 卡、LCD 等）
- 可扩展的命令行与 AT 指令接口
- 多种测试与演示模式（如速率测试、AI Demo 等）

- Supports WiFi AP/STA/P2P modes with flexible switching
- Automatic/manual network configuration, supports DHCP/static IP
- Device pairing and secure connection
- Low power and sleep management
- Audio and video stream processing and transmission
- Rich peripheral support (PSRAM, USB, SD card, LCD, etc.)
- Extensible command line and AT command interface
- Various test and demo modes (e.g., speed test, AI demo)

---

## 目录结构 / Directory Structure

- `project/`  主工程目录，包含主程序、配置、脚本等
- `libs/`     静态库文件，提供底层驱动和协议支持
- `sdk/`      SDK 相关代码，包含 HAL、驱动、OSAL 等
- `tools/`    工具程序（如固件打包、配置生成等）
- `csky/`     C-Sky 相关内核、算法、配置等
- `doc/`      文档目录

- `project/`  Main project directory, including main program, configs, scripts, etc.
- `libs/`     Static libraries for drivers and protocol support
- `sdk/`      SDK-related code, including HAL, drivers, OSAL, etc.
- `tools/`    Utility programs (e.g., firmware packaging, config generation)
- `csky/`     C-Sky kernel, algorithms, configs, etc.
- `doc/`      Documentation

---

## 编译与使用 / Build & Usage

### 环境准备 / Environment Setup

1. 推荐使用 C-Sky Studio 或兼容的嵌入式开发环境。
2. 安装交叉编译工具链（如 csky-abiv2-elf-gcc）。
3. 配置好硬件连接（如 JTAG、串口等）。

1. It is recommended to use C-Sky Studio or a compatible embedded IDE.
2. Install the cross-compilation toolchain (e.g., csky-abiv2-elf-gcc).
3. Set up hardware connections (e.g., JTAG, UART).

### 编译 / Build

- 通过 IDE 打开 `project/fpv_app_umac4.cdkproj` 工程文件，选择目标板卡和配置，编译生成固件。
- 或使用命令行脚本（如 `makecode.exe`、`prebuild.sh`、`BuildBIN.sh` 等）进行自动化编译和打包。

- Open `project/fpv_app_umac4.cdkproj` in the IDE, select the target board/config, and build the firmware.
- Or use command-line scripts (e.g., `makecode.exe`, `prebuild.sh`, `BuildBIN.sh`) for automated build and packaging.

### 烧录与运行 / Flash & Run

- 使用 `CSKYFlashProgramerConsole.bat` 或相关工具，将生成的固件烧录到目标硬件。
- 通过串口或网络接口与设备交互，进行调试和功能验证。

- Use `CSKYFlashProgramerConsole.bat` or related tools to flash the firmware to the target hardware.
- Interact with the device via UART or network interface for debugging and feature verification.

---

## 其他说明 / Additional Notes

- 配置文件和参数可在 `project/params/`、`project/psramCFG/` 等目录下自定义。
- 支持多种演示和测试模式，可根据需求裁剪和扩展。
- 详细 API 和开发文档请参考 `doc/` 目录或源码注释。

- Configuration files and parameters can be customized in `project/params/`, `project/psramCFG/`, etc.
- Supports various demo and test modes, can be tailored and extended as needed.
- For detailed API and development docs, refer to the `doc/` directory or source code comments.

---

如需进一步帮助，请联系项目维护者或查阅相关文档。

For further assistance, please contact the project maintainer or refer to the documentation. 