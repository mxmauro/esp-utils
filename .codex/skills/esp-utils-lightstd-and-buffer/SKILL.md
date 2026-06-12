# esp-utils Lightstd And Buffer Extensions

Use this skill when extending the lightweight containers, allocator wrappers, growable buffer helpers, or conversion/hash utilities.

## Scope

- `include/lightstd/*.h`
- `src/lightstd/allocator.cpp`
- `include/growable_buffer.h` and `src/growable_buffer.cpp`
- `include/convert.h` and `src/convert.cpp`
- `include/fnv.h` and `src/fnv.cpp`
- Matching tests in `test/main/`

## Repository expectations

- This code exists to provide small-footprint alternatives suitable for embedded use.
- Avoid pulling in heavier STL patterns unless the existing file already relies on them.
- Preserve current naming, return-value conventions, and memory ownership style.

## Design rules

1. Prefer explicit capacity and size handling.
2. Keep allocation failure behavior observable to callers.
3. Preserve existing wipe, reset, and clear semantics.
4. Avoid hidden realloc-heavy behavior in hot paths.
5. Extend tests for edge cases such as empty input, boundary sizes, and overwrite/erase behavior.

## Typical tasks

- Add a helper to `GrowableBuffer_t` or its C++ wrapper usage sites
- Extend `lightstd::vector`, `lightstd::string`, or `static_hash_map`
- Add conversion helpers with strict validation
- Add deterministic hash or encoding test vectors

## Change checklist

1. Read the public header and current tests before editing implementation.
2. Match existing API style instead of introducing a parallel interface shape.
3. Keep output buffers in a valid state on failure.
4. For encoding/decoding helpers, test both successful roundtrips and rejection paths.
5. For containers, test size, capacity, iteration, and reset behavior.

## Validation

- Re-read the touched tests to ensure the new behavior is covered.
- Check that any new helper remains suitable for embedded targets.
- If the change affects README feature descriptions, keep the doc update short.
