---
name: esp-utils-component-maintainer
description: Use this skill when changing or adding public component APIs in this repository.
---
# esp-utils Component Maintainer

Use this skill when changing or adding public component APIs in this repository.

## Scope

- ESP-IDF component layout in `include/`, `src/`, and `test/`
- Public API additions, removals, or behavior changes
- Component registration updates in `CMakeLists.txt` and packaging checks in `idf_component.yml`

## Repository facts

- This repository is an ESP-IDF component first, not a generic desktop library.
- Public headers live in `include/`.
- Implementations live in `src/`.
- Tests live in `test/main/` as Unity `TEST_CASE` files.
- Linux simulator builds are intentionally excluded in the root `CMakeLists.txt`.
- The published component excludes `test/**/*` via `idf_component.yml`.

## Files to inspect first

- `AGENTS.md`
- `README.md`
- `CMakeLists.txt`
- `idf_component.yml`
- The matching header in `include/`
- The matching implementation in `src/`
- Relevant tests in `test/main/`

## Working rules

1. Keep the diff narrow. Do not reformat unrelated files.
2. Preserve the mixed C/C++ API style already used here.
3. If behavior changes, update or add tests in `test/main/`.
4. If a new `.cpp` implementation file is added, register it in the root `CMakeLists.txt`.
5. If a new public capability is user-facing, update `README.md` only if the change is material.
6. Prefer explicit `esp_err_t` error paths and fail-closed behavior.

## Change checklist

1. Confirm whether the change belongs in a public header or should stay internal.
2. Mirror naming and qualifiers from neighboring APIs.
3. Check include ordering against the local file style.
4. Validate null, size, and state inputs early when the surrounding code does.
5. Keep object state reusable after failure.
6. Add or adjust Unity coverage for success and failure paths.

## Validation

- Read the touched header and source together to confirm signatures still match.
- Re-read the root `CMakeLists.txt` if files were added or moved.
- Re-read `test/RUNNING_TESTS.md` before suggesting how to execute tests.
- If you cannot run ESP-IDF tests locally, state that explicitly.
