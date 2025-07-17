# Raspberry Pi Pico SDK

The Raspberry Pi Pico Software Development Kit, hereafter **SDK**, provides the headers, libraries, and build system necessary to write programs for RP-series microcontroller devices such as the Raspberry Pi Pico in C, C++, or assembly language. The SDK is designed to provide an API (Application Programming Interface) and programming environment that's familiar both to both non-embedded and embedded C developers.

A single program runs on the device at a time, with a conventional `main()` method. Standard C and C++ libraries are supported, along with APIs for accessing a microcontroller's hardware, including DMA, IRQs, and wide variety of fixed-function peripherals and PIO (Programmable IO).

Additionally, the SDK provides higher-level libraries for dealing with timers, USB, synchronisation, and multi-core programming, along with high-level functionality built using PIO (such as audio). The SDK can be used to build anything from simple applications to full-fledged runtime environments such as MicroPython, or even low-level software, such as the microcontroller's on-chip boot ROM.

This documentation is generated from the SDK source tree using Doxygen. It provides basic information on the APIs used for each library, but doesn't provide usage information. For more technical information and usage guidance, refer to the Raspberry Pi datasheets.

## SDK Design

The RP-series microcontrollers are powerful chips designed for embedded systems; these chips operate in environments with limited memory (RAM) and storage (program space). As a result, trade-offs between performance and other factors (such as edge-case error handling, runtime versus compile time configuration, and so on) are more apparent than they might be on higher-level platforms (like desktop systems).

The SDK is designed to be both beginner-friendly and powerful for more experienced users. Its features work out-of-the-box with sensible defaults that cover most use cases. At the same time, it gives developers as much control as possible to tweak the application they're building and the libraries they use, if they choose to.

## The Build System

The SDK uses **CMake** to manage the build process. CMake is a widely used build system for C and C++ development, and is supported by many IDEs (Integrated Development Environments). It allows developers to specify build instructions using `CMakeLists.txt` files, from which CMake can generate platform-specific build systems for tools like `make` and `ninja`. These builds can be customised for the intended platform and configuration variables defined by the developer.

Apart from its popularity, CMake is fundamental to how the SDK is structured, and how applications are configured and built. It enables consistent builds across platforms while providing flexibility for more complex embedded projects.

The SDK builds a bare-metal executable, which is a standalone binary that includes all the code needed to run directly on a microcontroller, excluding device-specific low-level functionality, such as floating-point routines and other optimized code contained in a microcontroller's boot ROM.

## Examples

This SDK documentation contains example code fragments. An index of these examples can be found in the [examples page](@ref examples_page). These examples, and any other source code included in this documentation, are Copyright &copy; 2020 Raspberry Pi Ltd and licensed under the [3-Clause BSD](https://opensource.org/licenses/BSD-3-Clause) license.

