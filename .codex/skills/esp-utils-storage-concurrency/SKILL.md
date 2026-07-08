---
name: esp-utils-storage-concurrency
description: Use this skill when editing code that touches NVS, mutexes, task lifetime, rundown protection, or one-time initialization.
---
# esp-utils Storage And Concurrency Hardening

Use this skill when editing code that touches NVS, mutexes, task lifetime, rundown protection, or one-time initialization.

## Scope

- `src/storage/nvs.cpp` and `include/storage/*.h`
- `src/mutex.cpp` and `include/mutex.h`
- `src/task.cpp` and `include/task.h`
- `src/run_once.cpp`, `src/rundown_protection.cpp`, and related headers
- Tests in `test/main/test_storage_*.cpp`, `test_mutex.cpp`, `test_run_once.cpp`, and `test_rundown_protection.cpp`

## Risk model

- These files are stateful and synchronization-sensitive.
- Small changes can introduce partial initialization, stale handles, double-close, or shutdown races.
- Favor conservative behavior over convenience.

## Review focus

1. Check initialization and teardown symmetry first.
2. Verify ownership transfer rules for handles, buffers, and moved objects.
3. Look for state that must be cleared after failure.
4. Confirm read-only versus read-write transitions behave correctly.
5. Check whether task or event-group lifetime can race with signal or join paths.
6. Preserve or improve assertions without weakening runtime error handling.

## Change checklist

1. Map the full state machine before editing.
2. Keep failure exits centralized if the file already uses that pattern.
3. Do not add logging of key names, blob contents, or other sensitive values.
4. Keep heap allocation off hot paths unless the current design already requires it.
5. Add or update tests that exercise both nominal and failure-adjacent paths.

## Validation ideas

- Storage:
  - missing key behavior
  - read-after-write
  - erase semantics
  - commit behavior
- Concurrency:
  - repeated init/deinit
  - overlapping readers versus writers
  - join/detach ordering
  - one-time execution under contention

## Repository-specific reminders

- `nvsInit()` is global state guarded by a mutex. Treat that path carefully.
- `Task_t` uses an event group plus `RunOnce_t` for shutdown control.
- Unity tests in this repo already model concurrent behavior with FreeRTOS tasks. Extend those tests instead of inventing a new pattern.
