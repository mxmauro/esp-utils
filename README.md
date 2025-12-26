# esp-utils

`esp-utils` is a reusable [ESP-IDF](https://github.com/espressif/esp-idf) component that provides abstractions for common tasks including hardware control, data structures, synchronization primitives, and storage management.

## Features

- **Hardware Control**: On-board LED management and reset button handling.
- **Data Structures**: Growable buffers and custom containers (lightstd namespace).
- **Synchronization**: Rundown protection and one-time execution helpers.
- **Task Management**: Simplified task creation and management.
- **Storage**: NVS (Non-Volatile Storage) abstraction layer.
- **Utilities**: Time helpers, FNV hashing, type conversion, and profiling tools.
- **Modern C++**: Provides lightweight alternatives to standard library components for resource-constrained environments.

## Components

| Component            | Description                                                        |
|----------------------|--------------------------------------------------------------------|
| `onboard_led`        | Control built-in LED with PWM support                              |
| `reset_button`       | Handle reset button events and debouncing                          |
| `rundown_protection` | Graceful shutdown coordination mechanism                           |
| `run_once`           | Execute code blocks exactly once                                   |
| `growable_buffer`    | Dynamic memory buffer with automatic resizing                      |
| `task`               | Simplified FreeRTOS task wrapper                                   |
| `time`               | Timing and delay utilities                                         |
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
    version: "*"   # You can also specify a tag, branch or commit hash
```

Save the changes and run the following command:

```bash
idf.py reconfigure   # or idf.py build
```

This will download the components into the `managed_components` directory of your project.

## Requirements

- ESP-IDF v5.5 or later.
