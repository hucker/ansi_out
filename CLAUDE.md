# CLAUDE.md — ansi_print project guide

## Project overview

Single-file C99 library (`src/ansi_print.h` + `src/ansi_print.c`) providing Rich-style
`[tag]` markup for colored terminal output. Designed for embedded and PC use with no
dynamic allocation. CLI tool in `src/main.c`. Tests use Unity framework in `test/`.

## Build & test

```bash
make clean && make test        # full build — all features enabled (118 tests)
make test-minimal              # minimal build — all features disabled (42 tests)
make ansiprint                 # CLI only
build/ansiprint --demo         # run feature demo
```

Both `make test` and `make test-minimal` must pass before any checkin.

## Before checkin

1. **All tests pass** — run `make clean && make test && make test-minimal`
2. **README.md updated** — if you changed the API, added features, or changed test counts:
   - Update the feature table and API reference sections
   - Update test count badges (lines 5-6) and the test output section
   - Update version badge if version changed
3. **Version bumped** if the change is user-visible — update in all four locations:
   - `README.md` badge (line 3)
   - `README.md` FetchContent example (`GIT_TAG`)
   - `CMakeLists.txt` project `VERSION`
   - `src/ansi_print.h` `ANSI_PRINT_VERSION_PATCH` / `ANSI_PRINT_VERSION`
4. **Demo updated** — if you added a visible feature, add it to `src/main.c` demo
5. **No compiler warnings** — build uses `-Wall -Wextra`

## Architecture

- **Feature flags**: Compile-time `ANSI_PRINT_*` flags in `ansi_print.h` control optional
  features (emoji, gradients, banners, windows, bars, etc.). All default to 1 (enabled).
  `ANSI_PRINT_MINIMAL` disables everything at once.
- **No dynamic allocation**: All buffers are caller-provided via `ansi_init()` or per-call
  parameters (e.g. `ansi_bar()` takes `buf`/`buf_size`).
- **UTF-8 aware**: Window and bar functions handle multi-byte characters correctly.
  Continuation bytes (0x80-0xBF) are skipped when counting visible characters.

## Key files

| File                   | Purpose                              |
|------------------------|--------------------------------------|
| `src/ansi_print.h`    | Public API, feature flags, types     |
| `src/ansi_print.c`    | Implementation                       |
| `src/main.c`          | CLI tool and `--demo` showcase       |
| `test/test_cprint.c`  | Unity tests (full + minimal)         |
| `Makefile`            | Build system                         |
| `CMakeLists.txt`      | CMake alternative build              |
| `README.md`           | User documentation                   |

## Coding conventions

- C99 standard, no compiler extensions
- Static functions for internal helpers, no external linkage leaks
- Embedded-friendly: no malloc, no stdio in library code (only in main.c)
- `m_` prefix for file-scope static variables in ansi_print.c
- Box-drawing and block characters defined as macros with UTF-8 byte literals
