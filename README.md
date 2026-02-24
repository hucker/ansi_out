# `ansi_print`

![version](https://img.shields.io/badge/version-1.3.0-blue "Library version")
![license](https://img.shields.io/badge/license-MIT-green "MIT License")
![test-full](https://img.shields.io/badge/test--full-260%20passed-brightgreen "All features enabled")
![test-minimal](https://img.shields.io/badge/test--minimal-69%20passed-brightgreen "All optional features disabled")
![C standard](https://img.shields.io/badge/C-C99-orange "Requires C99 compiler")

A lightweight C library for colored terminal output using
[Python Rich](https://github.com/Textualize/rich)-inspired inline markup.
Built for embedded systems, CLI tools, and log formatting -- anywhere you want
readable, expressive colored text without pulling in a heavy framework. Ideally
used with a UTF-8 terminal for improved display of line based text, status, logs etc.

`ansi_print` borrows Rich's `[tag]` and `:emoji:` syntax but is deliberately
simple: it formats colored text, adds emojis. There is no markdown
rendering, and no nested markup stack. It outputs ANSI
escape codes through a caller-provided putc function, uses no dynamic
allocation, and compiles with any C99 toolchain.

Typical use cases:

- Embedded status displays over UART
- CLI tool output (errors, warnings, progress)
- Colorized log files and diagnostic messages
- Build system and test runner output

## Why Not Manually `#define RED "\033[31m"`?

A handful of escape macros is a perfectly good approach when all you need is a
few colors on a single platform. Use that for as long as it works. `ansi_print`
is for when those macros start turning into your own escape-code generator --
when you need mixed attributes like `[bold yellow on red]` instead of
`"\033[1;33;41m"`, runtime control so piped output stays clean, or features
like emoji and bar graphs that raw escapes become complex to manage.

What you get over hand-rolled macros:

- Readable mixed formatting with named tags
- Automatic reset management -- `[/]` closes whatever is open
- Global enable/disable with `NO_COLOR` and `isatty()` detection
- 28 named colors, 256-color palette, and text styles
- Emoji, Unicode escapes, gradients, banners, windows, and bar graphs
- Portable output via injected putc -- desktop, UART, USB CDC, any byte stream
- Compile-time feature flags so you only pay for what you use

## ansi_print Demo

Output of `ansiprint --demo`, the bundled CLI tool that showcases the main
features of the library: named colors, styles, backgrounds, gradients, emoji,
banners, windows, and bar graphs.

![ansiprint --demo output](img/demo.png)

## TUI Widget Demo

Output of `ansiprint --tui-demo`, showing the TUI widget layer built on top of
ansi_print.  The TUI layer uses ansi_print's markup and output functions to
construct positioned widgets: nested frames, labels, bar graphs, metric gauges
with threshold colors, check indicators, and status fields -- all placed with
cursor addressing and updated in place.

![ansiprint --tui-demo output](img/tui_demo.png)


## Features

- Rich-style `[tag]` markup for colors, styles, and backgrounds
- Emoji shortcodes (`:name:` syntax, same as Python Rich)
- Unicode codepoint escapes (`:U-XXXX:` syntax)
- Rainbow and gradient text effects (24-bit true color)
- 8 standard + 12 extended + 8 bright named colors
- Banner boxes for single-call status messages (`ansi_banner()`)
- Streaming window boxes with titled headers (`ansi_window_start/line/end()`)
- Inline bar graphs with 1/8-cell resolution (`ansi_bar()`)
- Platform-independent output via injected function pointers
- No dynamic memory allocation (caller provides buffer)
- Compile-time feature toggles to minimize code/data size
- Windows console UTF-8 and ANSI escape support

## Limitations

- **UTF-8 terminal required.** Emoji, box-drawing, and bar graph characters are
  UTF-8 encoded. There is no ASCII fallback.
- **No tag nesting.** Tags are tracked by active state, not a stack. Opening
  `[red]` while `[cyan]` is active replaces cyan; closing `[/red]` does not
  restore cyan.
- **Not thread-safe.** Single static state -- intended for single-threaded or
  cooperative use. Wrap calls with a mutex if needed from multiple threads.
- **Single output stream.** One putc destination at a time. Call `ansi_init()`
  to switch targets, but only one is active.
- **Silent truncation.** Output longer than the caller-provided buffer is
  truncated with no error indication.

## Quick Start

```c
#include "ansi_print.h"

#ifdef ON_TERMINAL
#include <stdio.h>
static void my_putc(int ch)  { putchar(ch); }
static void my_flush(void)   { fflush(stdout); }
#else
static void my_putc(int ch)  { uart_putc((char)ch); }
static void my_flush(void)   { /* no flush needed */ }
#endif

static char buf[512];

int main(void)
{
    ansi_init(my_putc, my_flush, buf, sizeof(buf));
#ifdef ON_TERMINAL
    ansi_enable();  /* enables ANSI on Windows; no-op elsewhere */
#endif

    ansi_print("[bold red]Error:[/] file not found\n");
    ansi_print("[green]:check: All tests passed[/]\n");
    ansi_print("[yellow on black]Warning: %d retries[/]\n", 3);

    return 0;
}
```

## Integration

### Files

Copy these into your project:

| File               | Purpose                                  |
| ------------------ | ---------------------------------------- |
| `src/ansi_print.h` | Public API header                        |
| `src/ansi_print.c` | Implementation (single translation unit) |
| `src/ansi_tui.h`   | TUI widget layer header (optional)       |
| `src/ansi_tui.c`   | TUI widget implementation (optional)     |

### CMake

If your project uses CMake, you can add `ansi_print` as a subdirectory:

```cmake
add_subdirectory(ansi_print)
target_link_libraries(my_app PRIVATE ansi_print)
```

Or fetch it directly from a Git repository:

```cmake
include(FetchContent)
FetchContent_Declare(ansi_print
    GIT_REPOSITORY https://github.com/yourname/ansi_print.git
    GIT_TAG        v1.3.0
)
FetchContent_MakeAvailable(ansi_print)
target_link_libraries(my_app PRIVATE ansi_print)
```

Feature toggles work as CMake options:

```console
cmake -B build -DANSI_PRINT_MINIMAL=ON -DANSI_PRINT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

### Embedded Setup

```c
/* UART output -- no flush needed */
static void uart_putc(int ch) { uart_put_char((char)ch); }
static char ansi_buf[256];

void app_init(void)
{
    ansi_init(uart_putc, NULL, ansi_buf, sizeof(ansi_buf));
}
```

### Suppressing Output

Pass `NULL` as the putc function to silently discard all output:

```c
ansi_init(NULL, NULL, buf, sizeof(buf));
```

### Single Instance

The library uses internal static state and supports one output stream at a time.
Call `ansi_init()` to reconfigure the output target (e.g. switch from UART to
USB), but only one destination is active at any moment. This is by design for
embedded systems where a single debug/status output is the norm.

## Configuration

Feature macros control what gets compiled in. By default everything is enabled.
Override them in one of two ways:

1. **app_cfg.h** -- Create this file on your include path. If it exists, the
   library includes it automatically before applying defaults.
2. **Compiler flags** -- Pass `-DANSI_PRINT_STYLES=0` etc. on the command line.

Define `ANSI_PRINT_NO_APP_CFG` to skip the `app_cfg.h` auto-include entirely.

### Minimal Build

Define `ANSI_PRINT_MINIMAL` to disable all optional features at once.
Individual flags can still be set to `1` to selectively re-enable features:

```c
/* app_cfg.h -- minimal build with just emoji */
#define ANSI_PRINT_MINIMAL
#define ANSI_PRINT_EMOJI  1
```

Or on the command line: `-DANSI_PRINT_MINIMAL`

### Feature Macros

| Feature Flag Macro           | Default           | Controls                                            |
| ---------------------------- | ----------------- | --------------------------------------------------- |
| `ANSI_PRINT_MINIMAL`         | (not defined)     | When defined, all features below default to 0       |
| `ANSI_PRINT_EMOJI`           | 1                 | Core emoji shortcodes (21 emoji)                    |
| `ANSI_PRINT_EXTENDED_EMOJI`  | 1                 | Extended emoji set (~130 more)                      |
| `ANSI_PRINT_EXTENDED_COLORS` | 1                 | 12 named colors (orange, pink, etc.)                |
| `ANSI_PRINT_BRIGHT_COLORS`   | 1                 | 8 bright variants (bright_red, etc.)                |
| `ANSI_PRINT_STYLES`          | 1                 | bold, dim, italic, underline, invert, strikethrough |
| `ANSI_PRINT_GRADIENTS`       | 1                 | `[rainbow]` and `[gradient]` tag effects            |
| `ANSI_PRINT_UNICODE`         | 1                 | `:U-XXXX:` codepoint escapes                        |
| `ANSI_PRINT_BANNER`          | 1                 | `ansi_banner()` boxed text output                   |
| `ANSI_PRINT_WINDOW`          | 1                 | `ansi_window_start/line/end()` streaming boxed text |
| `ANSI_PRINT_BAR`             | 1                 | `ansi_bar()` inline horizontal bar graphs           |
| `ANSI_PRINT_BOX_STYLE`       | `ANSI_BOX_DOUBLE` | Box-drawing character set for banner/window borders |

### TUI Feature Macros

The TUI widget layer (`ansi_tui.h` / `ansi_tui.c`) provides positioned terminal
widgets built on top of `ansi_print`.  Each widget type has its own feature flag
following the same pattern — all default to enabled, all disabled by
`ANSI_PRINT_MINIMAL`, and individual flags can be overridden.

| Feature Flag Macro | Default | Controls                                            |
| ------------------ | ------- | --------------------------------------------------- |
| `ANSI_TUI_FRAME`   | 1       | Frame container (border box)                        |
| `ANSI_TUI_LABEL`   | 1       | Label widget ("Name: value")                        |
| `ANSI_TUI_BAR`     | 1       | Bar graph widget (requires `ANSI_PRINT_BAR`)        |
| `ANSI_TUI_PBAR`    | 1       | Percent bar widget (requires `ANSI_PRINT_BAR`)      |
| `ANSI_TUI_STATUS`  | 1       | Status text field                                   |
| `ANSI_TUI_TEXT`    | 1       | Generic text widget                                 |
| `ANSI_TUI_CHECK`   | 1       | Check/cross indicator (requires `ANSI_PRINT_EMOJI`) |
| `ANSI_TUI_METRIC`  | 1       | Threshold-based metric gauge                        |

`ANSI_TUI_BAR` and `ANSI_TUI_PBAR` are forced off when `ANSI_PRINT_BAR=0` (no
underlying bar renderer).  `ANSI_TUI_CHECK` is forced off when
`ANSI_PRINT_EMOJI=0` (no emoji shortcode support).

The `tui_frame_t` struct and `tui_border_t` enum are always defined regardless
of flags, since all widget types reference them via their `parent` pointer.

```c
/* app_cfg.h -- minimal TUI with only frame + metric */
#define ANSI_PRINT_MINIMAL
#define ANSI_TUI_FRAME   1
#define ANSI_TUI_METRIC  1
```

Or on the command line:

```bash
gcc -DANSI_PRINT_MINIMAL -DANSI_TUI_FRAME=1 -DANSI_TUI_METRIC=1 ...
```

#### Box Style Constants

`ANSI_PRINT_BOX_STYLE` selects the box-drawing character set used by
`ansi_banner()` and `ansi_window_*()` at **compile time**.  The choice is
baked into the binary with zero runtime cost — only the selected character
set is compiled in.

```text
Style          Constant          Value   Sample
─────────────  ────────────────  ─────   ──────────────
Light          ANSI_BOX_LIGHT     0      ┌──────┐
                                         │ text │
                                         └──────┘

Heavy          ANSI_BOX_HEAVY     1      ┏━━━━━━┓
                                         ┃ text ┃
                                         ┗━━━━━━┛

Double (default) ANSI_BOX_DOUBLE  2      ╔══════╗
                                         ║ text ║
                                         ╚══════╝

Rounded        ANSI_BOX_ROUNDED   3      ╭──────╮
                                         │ text │
                                         ╰──────╯
```

Configure in `app_cfg.h` (recommended):

```c
/* app_cfg.h */
#define ANSI_PRINT_BOX_STYLE  ANSI_BOX_ROUNDED
```

Or via compiler flag:

```bash
# Command line
gcc -DANSI_PRINT_BOX_STYLE=ANSI_BOX_ROUNDED ...

# CMake
target_compile_definitions(myapp PRIVATE ANSI_PRINT_BOX_STYLE=ANSI_BOX_ROUNDED)
```

Or define before including the header:

```c
#define ANSI_PRINT_BOX_STYLE  ANSI_BOX_LIGHT
#include "ansi_print.h"
```

### Flash Impact per Feature

Each row shows the `.text` section size when a single feature flag is enabled on
top of a minimal build.  Measured with `clang 21 -Os -ffunction-sections
-fdata-sections` on x86-64 using `scripts/measure_features.sh`; absolute sizes will vary
by target architecture but the relative costs are representative.  On 32-bit
embedded targets, sizes will be slightly smaller due to 4-byte pointers (vs 8),
particularly RAM usage (~60-70 B vs ~100 B on 64-bit).

| Feature Flag                 |   .text |    Delta | Notes                             |
| ---------------------------- | ------: | -------: | --------------------------------- |
| *Minimal baseline*           |  3745 B |        — | 8 colors, `fg:/bg:`, tag parsing  |
| `ANSI_PRINT_UNICODE`         |  4150 B |   +405 B | `:U-XXXX:` codepoint escapes      |
| `ANSI_PRINT_BRIGHT_COLORS`   |  4272 B |   +527 B | 8 bright color name entries       |
| `ANSI_PRINT_STYLES`          |  4352 B |   +607 B | 6 style attributes                |
| `ANSI_PRINT_EXTENDED_COLORS` |  4571 B |   +826 B | 12 extra color name entries       |
| `ANSI_PRINT_EMOJI`           |  4729 B |   +984 B | 21 core emoji shortcodes          |
| `ANSI_PRINT_BAR`             |  4810 B |  +1065 B | Bar graphs with block elements    |
| `ANSI_PRINT_BANNER`          |  4946 B |  +1201 B | Boxed text with box-drawing chars |
| `ANSI_PRINT_GRADIENTS`       |  5368 B |  +1623 B | Rainbow + gradient (24-bit color) |
| `ANSI_PRINT_WINDOW`          |  6202 B |  +2457 B | Streaming boxed window output     |
| `ANSI_PRINT_EXTENDED_EMOJI`  |  9804 B |  +6059 B | +130 emoji (requires `EMOJI`)     |
| **Full build (all enabled)** | 18690 B | +14945 B | Everything                        |

> Each delta is measured independently (minimal + one flag).  Deltas overlap
> due to shared code, so individual deltas do not sum to the full build delta.

RAM usage is minimal in all configurations: 82 bytes (BSS) for a minimal build,
118 bytes with all features enabled, plus the caller-provided format buffer.

### TUI Flash Impact per Widget

Each row shows the combined `.text` of `ansi_print.c` + `ansi_tui.c` when a
single TUI widget flag is enabled on top of a full `ansi_print` build with all
TUI widgets disabled.  The baseline includes the full `ansi_print` library plus
TUI screen helpers (`tui_cls`, `tui_goto`, `tui_cursor_hide/show`).  Deltas
overlap because widgets share common infrastructure (border drawing, placement
helpers, etc.), so individual deltas do not sum to the full build delta.

| Component / Flag   |   .text |   Delta | Notes                            |
| ------------------ | ------: | ------: | -------------------------------- |
| *ansi_print lib*   | 18829 B |       — | Full library + screen helpers    |
| `ANSI_TUI_FRAME`   | 20353 B | +1524 B | Border container, no content     |
| `ANSI_TUI_CHECK`   | 20916 B | +2087 B | Boolean indicator with emoji     |
| `ANSI_TUI_LABEL`   | 20970 B | +2141 B | "Name: value" with padding       |
| `ANSI_TUI_STATUS`  | 21039 B | +2210 B | Single-line text field           |
| `ANSI_TUI_TEXT`    | 21039 B | +2210 B | Generic text with fill/center    |
| `ANSI_TUI_BAR`     | 21135 B | +2306 B | Bar graph wrapper (`ansi_bar`)   |
| `ANSI_TUI_PBAR`    | 21232 B | +2403 B | Percent bar (`ansi_bar_percent`) |
| `ANSI_TUI_METRIC`  | 21599 B | +2770 B | Threshold gauge with zone colors |
| **Full TUI build** | 27249 B | +8420 B | All widgets enabled              |

> Content widgets (all except Frame) share drawing primitives (`draw_border`,
> `resolve`, `pad`, `center`), so the first content widget pays for the shared
> code and subsequent widgets add only their unique logic.

`ansi_tui.c` uses zero BSS — all mutable state lives in caller-provided
`_state_t` structs.

### Example app_cfg.h (minimal embedded build)

```c
/* app_cfg.h -- 8 standard colors only, minimal code/data footprint */
#define ANSI_PRINT_MINIMAL
```

## Markup Syntax

### Colors (foreground)

```c
ansi_print("[red]Error message[/]\n");
ansi_print("[cyan]Info: %s[/]\n", msg);
```

### Backgrounds

Use the `on` keyword:

```c
ansi_print("[white on red]CRITICAL[/]\n");
ansi_print("[black on yellow]Warning[/]\n");
```

### Styles

```c
ansi_print("[bold]Important[/]\n");
ansi_print("[dim italic]Fine print[/]\n");
ansi_print("[underline]Underlined[/]\n");
ansi_print("[strikethrough]Deprecated[/]\n");
ansi_print("[invert]Inverted[/]\n");
```

### Combined

```c
ansi_print("[bold yellow on black]System Status: %s[/]\n", status);
```

### Close Tags

| Tag               | Behavior                                 |
| ----------------- | ---------------------------------------- |
| `[/]`             | Reset all formatting                     |
| `[/red]`          | Close only red foreground                |
| `[/bold]`         | Close only bold style                    |
| `[/red on black]` | Close specific foreground and background |

Selective close removes only the named attribute; other active formatting
remains. This matches Python Rich behavior.

```c
/* Bold stays active after closing green */
ansi_print("[bold][green]OK[/green] still bold[/bold]\n");
```

### Default Colors

`ansi_set_fg()` and `ansi_set_bg()` set baseline colors that are restored
whenever `[/]` resets formatting or a selective close removes the active color.
This is useful for applications that run with a fixed color scheme.

```c
ansi_set_fg("white");
ansi_set_bg("blue");
ansi_print("[red]Error:[/] back to white on blue\n");
ansi_print("[bold]Bold[/] still white on blue\n");

ansi_set_fg(NULL);  /* clear -- [/] reverts to terminal default */
```

### Escaped Characters

| Sequence | Output                                      |
| -------- | ------------------------------------------- |
| `[[`     | Literal `[`                                 |
| `]]`     | Literal `]`                                 |
| `::`     | Literal `:` (when emoji or unicode enabled) |

### Numeric Colors (256-color palette)

```c
ansi_print("[fg:208]Orange text[/]\n");      /* foreground */
ansi_print("[fg:82 bg:236]Custom[/]\n");     /* fg + bg */
```

### Important: No Tag Nesting

Tags are tracked by active state, not a stack. Opening a new color replaces the
previous one -- closing it does not restore the earlier color.

```c
/* This does NOT work as expected: */
ansi_print("[cyan]Outer [red]inner[/red] NOT cyan here[/cyan]\n");

/* Workaround -- close and reopen: */
ansi_print("[cyan]Before [/cyan][red]middle[/red][cyan] after[/cyan]\n");
```

## Colors

### Standard (always available)

| Name    | Foreground  | Background         |
| ------- | ----------- | ------------------ |
| black   | `[black]`   | `[... on black]`   |
| red     | `[red]`     | `[... on red]`     |
| green   | `[green]`   | `[... on green]`   |
| yellow  | `[yellow]`  | `[... on yellow]`  |
| blue    | `[blue]`    | `[... on blue]`    |
| magenta | `[magenta]` | `[... on magenta]` |
| cyan    | `[cyan]`    | `[... on cyan]`    |
| white   | `[white]`   | `[... on white]`   |

### Extended (ANSI_PRINT_EXTENDED_COLORS)

orange, pink, purple, brown, teal, lime, navy, olive, maroon, aqua, silver, gray

### Bright (ANSI_PRINT_BRIGHT_COLORS)

bright_black, bright_red, bright_green, bright_yellow, bright_blue,
bright_magenta, bright_cyan, bright_white

## Styles (ANSI_PRINT_STYLES)

| Name          | Effect                         |
| ------------- | ------------------------------ |
| bold          | Bold / bright text             |
| dim           | Reduced brightness             |
| italic        | Italic text                    |
| underline     | Underlined text                |
| invert        | Swap foreground and background |
| strikethrough | Struck-through text            |

## Special Effects (ANSI_PRINT_GRADIENTS)

### Rainbow

Each visible character cycles through a 21-step color palette:

```c
ansi_print("[rainbow]Hello, Rainbow![/]\n");
ansi_print("[bold rainbow]Bold rainbow[/]\n");
```

### Gradient

Per-character interpolation between two named colors using 24-bit true color:

```c
ansi_print("[gradient red blue]Smooth transition[/gradient]\n");
ansi_print("[bold][gradient cyan magenta]Styled gradient[/gradient][/bold]\n");
```

## Emoji (ANSI_PRINT_EMOJI)

Emoji use Rich-style `:name:` shortcodes. They emit Unicode characters (not
ANSI codes) so they appear even when color is disabled.

```c
ansi_print(":check: [green]Tests passed[/]\n");
ansi_print(":warning: [yellow]Low memory[/]\n");
ansi_print(":cross: [red]Build failed[/]\n");
```

### Core Emoji (21)

| Shortcode       | Emoji | Shortcode     | Emoji | Shortcode     | Emoji |
| --------------- | ----- | ------------- | ----- | ------------- | ----- |
| `:check:`       | ✅     | `:cross:`     | ❌     | `:warning:`   | ⚠     |
| `:info:`        | ℹ     | `:arrow:`     | ➡     | `:gear:`      | ⚙     |
| `:clock:`       | ⏰     | `:hourglass:` | ⌛     | `:thumbs_up:` | 👍     |
| `:thumbs_down:` | 👎     | `:star:`      | ⭐     | `:fire:`      | 🔥     |
| `:rocket:`      | 🚀     | `:zap:`       | ⚡     | `:bug:`       | 🐛     |
| `:wrench:`      | 🔧     | `:bell:`      | 🔔     | `:sparkles:`  | ✨     |
| `:package:`     | 📦     | `:link:`      | 🔗     | `:stop:`      | 🛑     |

### Extended Emoji (ANSI_PRINT_EXTENDED_EMOJI, ~140 more)

| Category      | Examples                                                                                          |
| ------------- | ------------------------------------------------------------------------------------------------- |
| Faces         | smile, grin, laugh, wink, cool, thinking, cry, skull                                              |
| Hands         | wave, clap, pray, muscle, ok_hand, victory, point_up                                              |
| Symbols       | heart, broken_heart, question, exclamation, infinity, recycle                                     |
| Arrows        | arrow_up, arrow_down, arrow_left, arrow_right, refresh                                            |
| Objects       | key, lock, unlock, shield, hammer, scissors, pencil, clipboard                                    |
| Nature        | sun, moon, cloud, rain, snow, earth, tree, flower                                                 |
| Food          | coffee, beer, pizza, cake, apple                                                                  |
| Animals       | dog, cat, snake, bird, fish, butterfly, bee, unicorn                                              |
| Transport     | car, airplane, ship, bicycle, train, fuel                                                         |
| Awards        | trophy, medal, crown, gem, money, gift, party, confetti                                           |
| Media         | music, film, camera, art, microphone                                                              |
| Colored Boxes | red_box, orange_box, yellow_box, green_box, blue_box, purple_box, brown_box, white_box, black_box |
| Misc          | pin, paperclip, eye, bulb, battery, plug, satellite, flag, memo                                   |

Unknown `:names:` pass through as literal text.

## Unicode Codepoint Escapes (ANSI_PRINT_UNICODE)

Insert any Unicode character by codepoint using `:U-XXXX:` syntax, where
`XXXX` is 1-6 hex digits (case-insensitive). Supports the full Unicode range
U+0001 to U+10FFFF.

```c
ansi_print(":U-2714: done\n");         // ✔ done
ansi_print(":U-1F525: on fire\n");     // 🔥 on fire
ansi_print(":U-41:\n");                // A
ansi_print(":U-2603: snowman\n");      // ☃ snowman
```

Like emoji, Unicode escapes are content -- they are always emitted regardless of
the color enable/disable state. Invalid codepoints (no hex digits, out of range)
pass through as literal text.

## Banner (ANSI_PRINT_BANNER)

`ansi_banner()` draws a Unicode box border around printf-formatted text
(style selected by `ANSI_PRINT_BOX_STYLE`, default: double-line),
with the border and text rendered in a named color. All text is formatted into
the shared buffer in a single call, so the total content is limited by the
buffer size passed to `ansi_init()`. Because the full text is available up
front, `width=0` can auto-size to the longest line. The `align` argument
controls text alignment within the box.

```c
typedef enum {
    ANSI_ALIGN_LEFT,    /* Left-align text (default) */
    ANSI_ALIGN_CENTER,  /* Center text within the box */
    ANSI_ALIGN_RIGHT    /* Right-align text within the box */
} ansi_align_t;
```

```c
/* Auto-width, left-aligned single line */
ansi_banner("red", 0, ANSI_ALIGN_LEFT,
            "Error: file %s not found", filename);

/* Fixed-width, centered multi-line */
ansi_banner("cyan", 40, ANSI_ALIGN_CENTER,
            "Firmware v%d.%d.%d\n"
            "Build: %s\n"
            "Status: %s",
            2, 4, 1, "Feb 21 2026", "Ready");

/* Right-aligned */
ansi_banner("yellow", 0, ANSI_ALIGN_RIGHT,
            "ADC Channels\n"
            "  CH0: %5.2fV\n"
            "  CH1: %5.2fV",
            3.29, 1.81);
```

*Centered banner (rendered in the specified color):*

```text
╔══════════════════════════════════════════╗
║          Firmware v2.4.1                 ║
║          Build: Feb 21 2026              ║
║          Status: Ready                   ║
╚══════════════════════════════════════════╝
```

Lines longer than the specified width are truncated; shorter lines are padded
according to the alignment setting. Any color name from the standard, extended,
or bright tables can be used. An unknown or NULL color produces an uncolored box.

## Window (ANSI_PRINT_WINDOW)

A streaming box API for displaying boxed text with an optional titled header.
Unlike `ansi_banner()`, which formats all text into the buffer at once, windows
are built one line at a time -- each `ansi_window_line()` call formats and
emits a single row. This means the number of lines is unlimited (not
constrained by the buffer size), though each individual line must still fit in
the buffer. The trade-off is that `width` must be specified up front since the
full text is not available for auto-sizing.

The border color is set once via `ansi_window_start()` and applied to all
border characters (top, bottom, separator, and side `║` bars). Line text
supports full Rich markup — colors, styles, emoji, gradients — the same tag
syntax as `ansi_print()`. Width and padding are calculated from visible
character count, so markup tags and ANSI codes do not affect alignment.

```c
/* Window with cyan borders and per-line markup */
ansi_window_start("cyan", 40, ANSI_ALIGN_CENTER, "Sensor Readings");
ansi_window_line(ANSI_ALIGN_LEFT, "[green]Temperature: %5.1f C[/]", 23.4);
ansi_window_line(ANSI_ALIGN_LEFT, "[yellow]Humidity:    %5.1f %%[/]", 61.2);
ansi_window_line(ANSI_ALIGN_LEFT, "[red]Pressure:    %5.1f hPa[/]", 1013.2);
ansi_window_end();

/* Window without a title */
ansi_window_start("yellow", 30, ANSI_ALIGN_LEFT, NULL);
ansi_window_line(ANSI_ALIGN_CENTER, "Centered content");
ansi_window_line(ANSI_ALIGN_RIGHT, "Right-aligned: %d", 42);
ansi_window_end();
```

*Window with title and separator:*

```text
╔══════════════════════════════════════════╗
║           Sensor Readings                ║
╠══════════════════════════════════════════╣
║ Temperature:  23.4 C                     ║
║ Humidity:     61.2 %                     ║
║ Pressure:  1013.2 hPa                    ║
╚══════════════════════════════════════════╝
```

The title line appears only when a non-NULL, non-empty title is passed to
`ansi_window_start()`. A horizontal separator (`╠═══╣`) is drawn beneath the
title. Each `ansi_window_line()` call has its own alignment parameter.

## Bar Graph (ANSI_PRINT_BAR)

`ansi_bar()` builds a string of Unicode block characters representing a
horizontal bar graph into a caller-provided buffer. The string contains Rich
markup for coloring and can be used as a `%s` argument to `ansi_print()`,
`ansi_window_line()`, or any printf-style function.

Each character cell has 8 sub-steps of resolution using partial block elements
(█▉▊▋▌▍▎▏). The `track` parameter selects the character used for unfilled
cells.

```c
const char *ansi_bar(char *buf, size_t buf_size,
                     const char *color, int width, ansi_bar_track_t track,
                     double value, double min, double max);
```

- `buf`, `buf_size` -- caller-provided output buffer (128 bytes supports ~30-wide bars)
- `color` -- named color for the filled portion (NULL for uncolored)
- `width` -- total bar width in character cells
- `track` -- character for unfilled cells (see table below)
- `value` -- current value to display
- `min`, `max` -- range for scaling (float math, clamped)

### Track Characters

| Value            | Character | Description      |
| ---------------- | --------- | ---------------- |
| `ANSI_BAR_BLANK` | (space)   | No visible track |
| `ANSI_BAR_LIGHT` | ░         | Light shade      |
| `ANSI_BAR_MED`   | ▒         | Medium shade     |
| `ANSI_BAR_HEAVY` | ▓         | Dark shade       |
| `ANSI_BAR_DOT`   | ·         | Middle dot       |
| `ANSI_BAR_LINE`  | ─         | Horizontal line  |

The buffer is passed directly (not via an init function) so that multiple bars
can coexist in the same printf argument list -- each call writes to its own
buffer with no shared state.

```c
/* Single bar with light shade track */
char bar[128];
ansi_print("CPU: %s %d%%\n",
           ansi_bar(bar, sizeof(bar), "green", 20,
                    ANSI_BAR_LIGHT, cpu, 0, 100), cpu);

/* Clean look with no visible track */
ansi_print("CPU: %s %d%%\n",
           ansi_bar(bar, sizeof(bar), "green", 20,
                    ANSI_BAR_BLANK, cpu, 0, 100), cpu);

/* Two bars in one printf -- each gets its own buffer */
char b1[128], b2[128];
ansi_print("CPU %s  MEM %s\n",
           ansi_bar(b1, sizeof(b1), "green", 15,
                    ANSI_BAR_LIGHT, cpu, 0, 100),
           ansi_bar(b2, sizeof(b2), "cyan",  15,
                    ANSI_BAR_LIGHT, mem, 0, 100));

/* Inside a window */
ansi_window_start("cyan", 30, ANSI_ALIGN_LEFT, "Monitors");
ansi_window_line(ANSI_ALIGN_LEFT, "CPU %s %d%%",
                 ansi_bar(bar, sizeof(bar), "green", 15,
                          ANSI_BAR_LIGHT, 73, 0, 100), 73);
ansi_window_line(ANSI_ALIGN_LEFT, "MEM %s %d%%",
                 ansi_bar(bar, sizeof(bar), "cyan",  15,
                          ANSI_BAR_LIGHT, 45, 0, 100), 45);
ansi_window_end();
```

*Bar graphs with light-shade track:*

```text
CPU: ██████████████▌░░░░░ 73%
MEM: █████████░░░░░░░░░░░ 45%
```

### Percentage Shorthand

`ansi_bar_percent()` is a convenience wrapper with a fixed 0-100 range that
appends " XX%" after the bar:

```c
char bar[128];
ansi_print("CPU %s\n",
           ansi_bar_percent(bar, sizeof(bar), "green", 20,
                            ANSI_BAR_LIGHT, 73));
// Output: CPU ██████████████▌░░░░░ 73%
```

## API Reference

```c
/* Initialize -- must be called first */
void ansi_init(ansi_putc_function putc_fn, ansi_flush_function flush_fn,
               char *buf, size_t buf_size);

/* Enable ANSI on Windows console, detect NO_COLOR and isatty */
void ansi_enable(void);

/* Color enable/disable */
void ansi_set_enabled(int enabled);
int  ansi_is_enabled(void);
void ansi_toggle(void);

/* Default foreground/background (restored on [/] reset) */
void ansi_set_fg(const char *color);
void ansi_set_bg(const char *color);

/* Rich-style printf with [tag] markup */
void ansi_print(const char *fmt, ...);

/* va_list variant of ansi_print */
void ansi_vprint(const char *fmt, va_list ap);

/* Printf into buffer without emitting -- returns formatted string */
const char *ansi_format(const char *fmt, ...);

/* Direct access to the format buffer (for custom formatting) */
char *ansi_get_buf(size_t *out_size);

/* Rich-style output for static strings (no printf overhead) */
void ansi_puts(const char *s);

/* Colored banner box around text (ANSI_PRINT_BANNER only) */
void ansi_banner(const char *color, int width, ansi_align_t align,
                 const char *fmt, ...);

/* Streaming boxed text with optional title (ANSI_PRINT_WINDOW only) */
void ansi_window_start(const char *color, int width, ansi_align_t align,
                       const char *title);
void ansi_window_line(ansi_align_t align, const char *fmt, ...);
void ansi_window_end(void);

/* Inline bar graph string (ANSI_PRINT_BAR only) */
const char *ansi_bar(char *buf, size_t buf_size,
                     const char *color, int width, ansi_bar_track_t track,
                     double value, double min, double max);

/* Bar graph with " XX%" appended (ANSI_PRINT_BAR only) */
const char *ansi_bar_percent(char *buf, size_t buf_size,
                             const char *color, int width,
                             ansi_bar_track_t track, int percent);
```

### TUI API

Screen helpers (always available):

```c
void tui_cls(void);                      /* Clear screen */
void tui_goto(int row, int col);         /* Position cursor (1-based) */
void tui_cursor_hide(void);
void tui_cursor_show(void);
```

Widget lifecycle — each widget type follows the same init/update/enable
pattern.  Widgets with numeric or boolean state accept a `force` parameter:
`force=1` always redraws, `force=0` skips redraw when the value is unchanged.

```c
/* Frame container (ANSI_TUI_FRAME) */
void tui_frame_init(const tui_frame_t *f);

/* Label: "Name: value" (ANSI_TUI_LABEL) */
void tui_label_init(const tui_label_t *w);
void tui_label_update(const tui_label_t *w, const char *fmt, ...);
void tui_label_enable(const tui_label_t *w, int enabled);

/* Bar graph widget (ANSI_TUI_BAR, requires ANSI_PRINT_BAR) */
void tui_bar_init(const tui_bar_t *w);
void tui_bar_update(const tui_bar_t *w, double value, double min, double max,
                    int force);
void tui_bar_enable(const tui_bar_t *w, int enabled);

/* Percent bar widget (ANSI_TUI_PBAR, requires ANSI_PRINT_BAR) */
void tui_pbar_init(const tui_pbar_t *w);
void tui_pbar_update(const tui_pbar_t *w, int percent, int force);
void tui_pbar_enable(const tui_pbar_t *w, int enabled);

/* Status text field (ANSI_TUI_STATUS) */
void tui_status_init(const tui_status_t *w);
void tui_status_update(const tui_status_t *w, const char *fmt, ...);
void tui_status_enable(const tui_status_t *w, int enabled);

/* Generic text widget (ANSI_TUI_TEXT) */
void tui_text_init(const tui_text_t *w);
void tui_text_update(const tui_text_t *w, const char *fmt, ...);
void tui_text_enable(const tui_text_t *w, int enabled);

/* Check indicator (ANSI_TUI_CHECK, requires ANSI_PRINT_EMOJI) */
void tui_check_init(const tui_check_t *w, int state);
void tui_check_update(const tui_check_t *w, int state, int force);
void tui_check_toggle(const tui_check_t *w);
void tui_check_enable(const tui_check_t *w, int enabled);

/* Threshold metric gauge (ANSI_TUI_METRIC) */
void tui_metric_init(const tui_metric_t *w);
void tui_metric_update(const tui_metric_t *w, double value, int force);
void tui_metric_enable(const tui_metric_t *w, int enabled);
```

All widgets use `tui_placement_t` for positioning (row, col, border, color,
parent).  Negative row/col values position from the end of the parent frame.
Set `col=0` to center, `width=-1` to fill the parent.

## CLI Tool

The project includes a command-line tool for testing markup from the shell.

### Build

```console
make ansiprint
```

### Usage

```console
ansiprint [--demo | --tui-demo] [<markup string> ...]
```

### Feature Demos

Run `ansiprint --demo` to see the markup features or `ansiprint --tui-demo`
to see the positioned widget layer (output shown at top of this page).

### Examples

```console
$ ansiprint "[bold red]Error:[/] something broke"
Error: something broke

$ ansiprint "[green]:check: All tests passed[/]"
✅ All tests passed

$ ansiprint ":warning: [yellow]Low memory[/]"
⚠ Low memory

$ ansiprint "[white on red] CRITICAL [/] system overheating"
 CRITICAL  system overheating

$ ansiprint "[bold][gradient red blue]Smooth gradient text[/gradient][/bold]"
Smooth gradient text

$ ansiprint ":fire::fire::fire: [bold red]ALERT[/] :fire::fire::fire:"
🔥🔥🔥 ALERT 🔥🔥🔥

$ ansiprint ":green_box: Pass  :red_box: Fail"
🟩 Pass  🟥 Fail

$ ansiprint ":U-2714: done  :U-1F525: hot"
✔ done  🔥 hot

$ ansiprint "[bold rainbow]Rainbow text![/]"
Rainbow text!
```

Note: colors and styles render in the terminal but are shown as plain text above.

## Building & Testing

### Prerequisites

- C99 compiler (gcc, clang, MSVC)
- Make (GNU Make on Linux/macOS, MSYS2 on Windows)

### Targets

| Target              | Description                                  |
| ------------------- | -------------------------------------------- |
| `make all`          | Build CLI tool, run tests, and generate docs |
| `make ansiprint`    | Build CLI executable only                    |
| `make test`         | Build and run tests (all features enabled)   |
| `make test-minimal` | Build and run tests (all features disabled)  |
| `make docs`         | Generate Doxygen HTML documentation          |
| `make clean`        | Remove build artifacts (including docs)      |

### Test Output

Tests print a config banner showing which features are active:

```console
$ make test
>> build/test_cprint
Build config: EMOJI=1 EXTENDED_EMOJI=1 EXTENDED_COLORS=1 BRIGHT_COLORS=1 STYLES=1 GRADIENTS=1 UNICODE=1 BANNER=1 WINDOW=1 BAR=1
...
145 Tests 0 Failures 0 Ignored

>> build/test_tui
Build config: BAR=1 BANNER=1 WINDOW=1 EMOJI=1
  TUI flags: FRAME=1 LABEL=1 BAR=1 PBAR=1 STATUS=1 TEXT=1 CHECK=1 METRIC=1
...
115 Tests 0 Failures 0 Ignored

$ make test-minimal
>> build/test_cprint_minimal
Build config: EMOJI=0 EXTENDED_EMOJI=0 EXTENDED_COLORS=0 BRIGHT_COLORS=0 STYLES=0 GRADIENTS=0 UNICODE=0 BANNER=0 WINDOW=0 BAR=0
...
64 Tests 0 Failures 0 Ignored

>> build/test_tui_minimal
Build config: BAR=0 BANNER=0 WINDOW=0 EMOJI=0
  TUI flags: FRAME=0 LABEL=0 BAR=0 PBAR=0 STATUS=0 TEXT=0 CHECK=0 METRIC=0
...
5 Tests 0 Failures 0 Ignored
```

The minimal build runs fewer tests (features compiled out) plus additional
tests that verify compiled-out features degrade gracefully (unknown tags
consumed silently, emoji shortcodes pass through as literal text, etc.).
TUI minimal runs only screen helper tests since all widget types are disabled.

## Documentation

API documentation is generated from the source headers using [Doxygen](https://www.doxygen.nl/).
All public functions, types, and feature flags are annotated with `@brief`, `@param`,
`@code`/`@endcode` examples, and cross-references.

```bash
make docs                         # generate HTML docs
open build/docs/html/index.html   # view in browser
```

Or invoke doxygen directly:

```bash
doxygen Doxyfile
```

Output is written to `build/docs/html/`.

## License

MIT
