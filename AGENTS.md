# esp-utils Agent Notes

## Scope
- These instructions apply to the whole repository.
- Treat this as an ESP-IDF component first. Preserve the existing split between public headers in `include/`, implementation in `src/`, and test code in `test/`.
- Do not touch unrelated user changes. This repository may be dirty.

## Shared skills
- Use `embedded-working-principles` for change scope and verification, `embedded-security-expectations` for security-sensitive work, and `esp-idf-guidance` for ESP-IDF and FreeRTOS work.
- Use `embedded-cpp-style` for C/C++ source and headers and `repository-file-style` for CMake, YAML, shell, Markdown, and other text files.

## Naming and API conventions
- Preserve the current mixed C/C++ API design:
- C-facing types and functions use the established suffix/prefix patterns such as `Mutex_t`, `RunOnce_t`, `mutexInit`, `nvsInit`.
- C++ wrappers use PascalCase types and methods such as `Mutex`, `AutoMutex`, `NVSStorage`, `Lock`, `Unlock`.

## Headers, implementation, and tests
- Keep public headers focused on the exported API. Do not leak internal implementation details unless the task requires an API change.
- Match the surrounding naming style instead of renaming identifiers for taste.
- Extend or update tests in `test/main/` when behavior changes, especially for synchronization, storage, conversion, or buffer logic.

## Editing rules for agents
- If a task creates tension between style and security, prioritize security while keeping the diff as small as possible.

## Component integration
- Keep Linux-simulator exclusions, component registration structure, and dependency declarations consistent with the existing `CMakeLists.txt` and `idf_component.yml`.
