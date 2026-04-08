# esp-utils Agent Notes

## Scope
- These instructions apply to the whole repository.
- Treat this as an ESP-IDF component first. Preserve the existing split between public headers in `include/`, implementation in `src/`, and test code in `test/`.
- Make the smallest change that solves the task. Do not reformat unrelated code.
- Do not touch unrelated user changes. This repository may be dirty.

## File format
- Use LF line endings for text files. The repository enforces this in `.editorconfig`.
- Use UTF-8 or plain ASCII. Do not introduce a different encoding unless the file already requires it.
- Keep trailing whitespace out of handwritten files, except where Markdown or reStructuredText intentionally uses trailing spaces for formatting.
- Preserve the existing spacing and blank-line rhythm in touched files instead of normalizing entire files.

## General coding style
- Indent with 4 spaces in C, C++, headers, and CMake files. Use 2 spaces in YAML files.
- Follow the existing Allman brace style for classes, functions, namespaces, and control-flow blocks.
- Exception: keep the opening `{` on the same line for `struct` and `enum` declarations when following the established C-style form already used in this repository.
- Keep separator comments in the established form when they already help structure a file:

```cpp
// -----------------------------------------------------------------------------
```

- Prefer sparse comments that explain intent or constraints. Do not add narration for obvious statements.
- Keep include order aligned with the surrounding file. In current sources this usually means the local project header first, then C or ESP-IDF headers.

## C and C++ conventions
- Preserve the current mixed C/C++ API design:
- C-facing types and functions use the established suffix/prefix patterns such as `Mutex_t`, `RunOnce_t`, `mutexInit`, `nvsInit`.
- C++ wrappers use PascalCase types and methods such as `Mutex`, `AutoMutex`, `NVSStorage`, `Lock`, `Unlock`.
- Keep `#pragma once` in headers.
- Keep `extern "C"` guards exactly where they are needed for the C API surface.
- Prefer explicit types over `auto` unless the surrounding code already uses `auto` and the deduced type is obvious.
- Preserve `noexcept`, `final`, deleted copy/move operations, and other API-surface qualifiers unless the task requires a real behavioral change.

## ESP-IDF and FreeRTOS guidance
- Prefer established ESP-IDF types and error handling: `esp_err_t`, `ESP_OK`, `ESP_ERR_*`.
- For public or reusable helpers, validate inputs early and keep existing `assert(...)` checks when the file already uses them.
- Preserve current FreeRTOS and ESP-IDF usage patterns instead of replacing them with STL-heavy alternatives.
- Avoid introducing exceptions, RTTI-dependent patterns, or heavyweight standard-library facilities into embedded paths unless the file already depends on them.
- Keep Linux-simulator exclusions, component registration structure, and dependency declarations consistent with the existing `CMakeLists.txt` and `idf_component.yml`.

## Security expectations
- Favor fail-closed behavior. On invalid state or initialization failure, return an error instead of silently continuing.
- Preserve or improve input validation, bounds checks, null checks, and length checks. Do not weaken them for convenience.
- Be careful with storage, buffer, and concurrency code. Any change touching NVS, buffers, atomics, mutexes, or task lifetime should minimize race windows and avoid partial-state exposure.
- Do not log secrets, keys, credentials, tokens, raw device identifiers, or sensitive buffer contents.
- Avoid adding dynamic allocation on hot paths or inside synchronization-sensitive code unless it is already part of the design.
- When clearing or resetting state after failure, keep objects in a safe, reusable state.

## Headers, implementation, and tests
- Keep public headers focused on the exported API. Do not leak internal implementation details unless the task requires an API change.
- Match the surrounding naming style instead of renaming identifiers for taste.
- Extend or update tests in `test/main/` when behavior changes, especially for synchronization, storage, conversion, or buffer logic.
- Do not fix unrelated bugs while editing unless they block the requested change.

## Build files and docs
- In `CMakeLists.txt`, preserve the current style: lowercase commands, 4-space indentation, and grouped argument lists.
- In YAML files, preserve 2-space indentation and existing key ordering where practical.
- Keep Markdown concise and consistent with the repository's current tone.

## Editing rules for agents
- Before changing a file, read the nearby code and match its local style.
- Prefer focused patches over broad cleanup.
- Preserve line endings, indentation, and spacing exactly in untouched regions.
- If a task creates tension between style and security, prioritize security while keeping the diff as small as possible.
