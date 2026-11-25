# esp-utils

`esp-utils` is a reusable [ESP-IDF](https://github.com/espressif/esp-idf) component that provides abstractions for common tasks including
hardware control, data structures, synchronization primitives, and storage management.

## Features

- **Hardware Control**: On-board LED management and reset button handling.
- **Data Structures**: Growable buffers and custom containers (lightstd namespace).
- **Synchronization**: Rundown protection and one-time execution helpers.
- **Task Management**: Simplified task creation and management.
- **Storage**: NVS (Non-Volatile Storage) abstraction layer.
- **Utilities**: Time helpers, FNV hashing, type conversion, and profiling tools.
- **C and Modern C++**: Provides C APIs plus lightweight C++ RAII wrappers and standard-library alternatives for resource-constrained environments.

## Components

| Component            | Description                                                        |
|----------------------|--------------------------------------------------------------------|
| `onboard_led`        | Control built-in LED                                               |
| `reset_button`       | Handle reset button events and debouncing                          |
| `rundown_protection` | Graceful shutdown coordination mechanism                           |
| `run_once`           | Execute code blocks exactly once                                   |
| `growable_buffer`    | Dynamic memory buffer with automatic resizing                      |
| `task`               | Simplified FreeRTOS task wrapper                                   |
| `time`               | Millisecond uptime helper                                          |
| `fnv`                | FNV hash function implementation                                   |
| `convert`            | Type and data conversion helpers                                   |
| `storage`            | NVS read/write abstractions                                        |
| `lightstd`           | Lightweight standard library alternatives (allocators, containers) |
| `mini_profiler`      | Simple performance profiling utilities                             |

## Installation

Open your project's `idf_component.yml` file and add the `dependencies` section if not present.
Then append the desired component like the following example:

```yaml
dependencies:
  mxmauro/esp_utils:
    git: https://github.com/mxmauro/esp-utils.git
    version: "v0.6.0"   # You can also specify a branch or commit hash
```

Save the changes and run the following command:

```bash
idf.py reconfigure   # or idf.py build
```

This will download the components into the `managed_components` directory of your project.

## Requirements

- ESP-IDF v5.5 or later.
- Target: ESP32 (Linux simulator not supported)

## Onboard LED

Select the LED type and GPIO under **Onboard LED Configuration** in menuconfig.
GPIO LEDs support active-high and active-low wiring. WS2812 LEDs use the RMT
peripheral and support selectable channel order (RGB by default); no external
LED component is required. With no LED selected, the functions do nothing.

Call `onboardLedInit()` once before using `onboardLedSet(bool)`. For WS2812,
on and off mean blue and black respectively, and `onboardLedSetRGB(r, g, b)`
sets an arbitrary 8-bit color. Setters serialize calls from FreeRTOS tasks and
must not be called from an interrupt. Initialization and hardware errors use
ESP-IDF's fatal error handling. The initializer no longer accepts a GPIO
argument; existing applications must move that setting to menuconfig.
