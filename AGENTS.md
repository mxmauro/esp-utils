# esp-utils Agent Notes

## Scope and skill routing

These instructions apply to the whole repository. Preserve the component split between public headers in `include/`, implementation in `src/`, and tests in `test/`.

- Use the canonical engineering, security, review, repository-text, and embedded C/C++ skills as applicable.
- Use canonical `esp-idf-guidance` and local component-maintainer, lightstd/buffer, and storage/concurrency skills for their specialized areas.

## Component constraints

- Keep public headers focused on exported APIs; do not expose implementation details without an intentional API change.
- Preserve the mixed API design: C-facing types/functions follow their established suffix/prefix patterns, while C++ wrappers retain their PascalCase interfaces.
- Extend tests in `test/main/` for changes to synchronization, storage, conversion, or buffer behavior.
- Keep Linux-simulator exclusions, component registration, and dependency declarations aligned with `CMakeLists.txt` and `idf_component.yml`.
