/**
 * @file ansi_print.h
 * @brief ANSI color printing helpers with Rich-inspired inline markup.
 *
 * Inspired by the Python Rich library's inline text markup syntax (e.g.,
 * "[red]error[/]"). Only the inline color/style formatting is implemented;
 * ansi_print is not terminal-aware and does not support advanced Rich features
 * such as markdown rendering, tables, layouts, or auto-detection of
 * terminal capabilities.
 *
 * @section features Features
 * - Named foreground and background colors (8 standard, 12 extended, 8 bright)
 * - Text styles: bold, dim, italic, underline, invert, strikethrough
 * - Rainbow and gradient effects (24-bit true color)
 * - Emoji shortcodes: :fire:, :check:, :rocket:, etc. (~150 total)
 * - Unicode codepoint escapes: :U-2714:, :U-1F525:, etc.
 * - Runtime enable/disable of color output (emoji/unicode always emitted)
 * - Platform-independent output via injected function pointers
 * - No dynamic memory allocation (caller provides buffer for formatting)
 * - NOT THREAD SAFE (intended for single-threaded/cooperative use)
 *
 * @section compile_flags Compile-Time Feature Flags
 * All features default to enabled (1). Set to 0 via -D flags or app_cfg.h
 * to reduce code size for constrained environments.
 *
 * | Flag                        | Default | Description                          |
 * |-----------------------------|---------|--------------------------------------|
 * | ANSI_PRINT_EMOJI            | 1       | Core emoji (~21 shortcodes)          |
 * | ANSI_PRINT_EXTENDED_EMOJI   | 1       | Extended emoji (~130 more)           |
 * | ANSI_PRINT_EXTENDED_COLORS  | 1       | orange, pink, purple, teal, etc.     |
 * | ANSI_PRINT_BRIGHT_COLORS    | 1       | bright_red, bright_green, etc.       |
 * | ANSI_PRINT_STYLES           | 1       | bold, dim, italic, underline, etc.   |
 * | ANSI_PRINT_GRADIENTS        | 1       | rainbow and gradient effects          |
 * | ANSI_PRINT_UNICODE          | 1       | :U-XXXX: codepoint escapes           |
 * | ANSI_PRINT_BANNER           | 1       | ansi_banner() boxed text output      |
 * | ANSI_PRINT_WINDOW           | 1       | ansi_window_start/line/end() streams |
 * | ANSI_PRINT_BAR              | 1       | ansi_bar() inline bar graphs         |
 *
 * @section setup Setup
 * @code
 * // Initialize output (required before first ansi_print)
 * static void my_putc(int ch) { putchar(ch); }
 * static void my_flush(void)  { fflush(stdout); }
 * static char buf[512];
 * ansi_init(my_putc, my_flush, buf, sizeof(buf));
 *
 * // On embedded (UART, no flush needed)
 * static char buf[256];
 * ansi_init(uart_put_char, NULL, buf, sizeof(buf));
 * @endcode
 *
 * @section usage Usage Examples
 * @code
 * // Simple colored output
 * ansi_print("[red]Error: %s[/]\n", error_msg);
 * ansi_print("[green]Success![/]\n");
 *
 * // Combined styles and colors
 * ansi_print("[bold yellow on black]System Status: %s[/]\n", status);
 *
 * // Emoji shortcodes
 * ansi_print(":check: [green]All tests passed[/]\n");
 * ansi_print(":cross: [red]Build failed[/]\n");
 *
 * // Unicode codepoint escapes
 * ansi_print(":U-2714: done\n");       // ✔ done
 * ansi_print(":U-1F525: hot\n");       // 🔥 hot
 *
 * // Toggle color on/off (emoji/unicode still emitted)
 * ansi_toggle();
 * ansi_print("[red]Plain text[/] :check:\n");  // "Plain text ✔"
 * @endcode
 *
 * @section colors Supported Colors
 * Standard: black, red, green, yellow, blue, magenta, cyan, white
 * Extended (ANSI_PRINT_EXTENDED_COLORS): orange, pink, purple, brown,
 *          teal, lime, navy, olive, maroon, aqua, silver, gray
 * Bright (ANSI_PRINT_BRIGHT_COLORS): bright_black, bright_red,
 *         bright_green, bright_yellow, bright_blue, bright_magenta,
 *         bright_cyan, bright_white
 */

#ifndef ANSI_PRINT_H
#define ANSI_PRINT_H

/** @name Library version */
/** @{ */
#define ANSI_PRINT_VERSION_MAJOR  1
#define ANSI_PRINT_VERSION_MINOR  3
#define ANSI_PRINT_VERSION_PATCH  0
#define ANSI_PRINT_VERSION        "1.3.0"
/** @} */

/* Provide actual #define targets for doxygen's @def parser */
#ifdef __DOXYGEN__
/** @def ANSI_PRINT_NO_APP_CFG
 *  Define to skip the automatic @c app_cfg.h include. Useful when feature
 *  flags are set entirely via compiler @c -D flags. */
#define ANSI_PRINT_NO_APP_CFG
/** @def ANSI_PRINT_MINIMAL
 *  When defined, all optional features default to disabled (0) instead of
 *  enabled (1). Individual flags can still be set to 1 to selectively
 *  re-enable features in a minimal build. */
#define ANSI_PRINT_MINIMAL
#endif

#if !defined(ANSI_PRINT_NO_APP_CFG)
#  if defined(__has_include)
#    if __has_include("app_cfg.h")
#      include "app_cfg.h"
#    endif
#  endif
#endif

#ifdef ANSI_PRINT_MINIMAL
#  define ANSI_PRINT_DEFAULT_  0
#else
#  define ANSI_PRINT_DEFAULT_  1
#endif

/** @def ANSI_PRINT_EMOJI
 *  Enable core emoji shortcodes (21 emoji). Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_PRINT_EMOJI
#  define ANSI_PRINT_EMOJI            ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_PRINT_EXTENDED_EMOJI
 *  Enable extended emoji set (~130 additional shortcodes). Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_PRINT_EXTENDED_EMOJI
#  define ANSI_PRINT_EXTENDED_EMOJI   ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_PRINT_EXTENDED_COLORS
 *  Enable 12 extra named colors: orange, pink, purple, brown, teal, lime, navy,
 *  olive, maroon, aqua, silver, gray. Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_PRINT_EXTENDED_COLORS
#  define ANSI_PRINT_EXTENDED_COLORS  ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_PRINT_BRIGHT_COLORS
 *  Enable 8 bright color variants (bright_red, bright_green, etc.).
 *  Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_PRINT_BRIGHT_COLORS
#  define ANSI_PRINT_BRIGHT_COLORS    ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_PRINT_STYLES
 *  Enable text styles: bold, dim, italic, underline, invert, strikethrough.
 *  Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_PRINT_STYLES
#  define ANSI_PRINT_STYLES           ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_PRINT_GRADIENTS
 *  Enable [rainbow] and [gradient] tag effects.
 *  Requires 24-bit true color support. Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_PRINT_GRADIENTS
#  define ANSI_PRINT_GRADIENTS        ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_PRINT_UNICODE
 *  Enable :U-XXXX: codepoint escapes (U+0001 to U+10FFFF).
 *  Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_PRINT_UNICODE
#  define ANSI_PRINT_UNICODE          ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_PRINT_BANNER
 *  Enable ansi_banner() for boxed text output with Unicode box-drawing borders.
 *  Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_PRINT_BANNER
#  define ANSI_PRINT_BANNER           ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_PRINT_WINDOW
 *  Enable ansi_window_start/line/end() for streaming boxed text output with
 *  optional titled header.  Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_PRINT_WINDOW
#  define ANSI_PRINT_WINDOW           ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_PRINT_BAR
 *  Enable ansi_bar() for inline horizontal bar graph rendering using
 *  Unicode block elements (1/8 resolution).
 *  Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_PRINT_BAR
#  define ANSI_PRINT_BAR              ANSI_PRINT_DEFAULT_
#endif

/** Box style constants for ANSI_PRINT_BOX_STYLE selection. */
#define ANSI_BOX_LIGHT    0   /* ┌─┐│└─┘├┤  single line   */
#define ANSI_BOX_HEAVY    1   /* ┏━┓┃┗━┛┣┫  thick line    */
#define ANSI_BOX_DOUBLE   2   /* ╔═╗║╚═╝╠╣  double line   */
#define ANSI_BOX_ROUNDED  3   /* ╭─╮│╰─╯├┤  rounded corner */

/** @def ANSI_PRINT_BOX_STYLE
 *  Select the box-drawing character set used by ansi_banner() and
 *  ansi_window_*(). Set to one of ANSI_BOX_LIGHT, ANSI_BOX_HEAVY,
 *  ANSI_BOX_DOUBLE, or ANSI_BOX_ROUNDED. Default: ANSI_BOX_DOUBLE. */
#ifndef ANSI_PRINT_BOX_STYLE
#  define ANSI_PRINT_BOX_STYLE        ANSI_BOX_DOUBLE
#endif

#include <stdarg.h>     // va_list
#include <stddef.h>     // size_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function pointer type for character output.
 *
 * Matches the standard putchar() convention: takes an int (character).
 * On desktop, wrap putchar(); on embedded, wrap uart_put_char() or equivalent.
 *
 * @param ch Character to output.
 */
typedef void (*ansi_putc_function)(int ch);

/**
 * @brief Function pointer type for output flush.
 *
 * Called after completing a print operation to ensure all characters
 * are transmitted. On desktop, wrap fflush(stdout); on embedded,
 * pass NULL if flush is not needed (replaced with internal no-op).
 */
typedef void (*ansi_flush_function)(void);

/**
 * @brief Initialize ansi_print with platform-specific output functions and buffer.
 *
 * Must be called before any ansi_print output functions (ansi_print, etc.).
 * Follows the same dependency injection pattern as feq_init().
 *
 * @param putc_fn  Character output function, or NULL to suppress all output.
 * @param flush_fn Flush function, or NULL for no-op flush.
 * @param buf      Caller-owned buffer for ansi_print string formatting.
 * @param buf_size Size of buf in bytes (512 is typical for line-based output).
 *
 * @code
 * // Desktop (stdio)
 * static void my_putc(int ch) { putchar(ch); }
 * static void my_flush(void)  { fflush(stdout); }
 * static char ansi_buf[512];
 * ansi_init(my_putc, my_flush, ansi_buf, sizeof(ansi_buf));
 *
 * // Embedded (UART, no flush needed)
 * static void uart_putc(int ch) { uart_put_char((char)ch); }
 * static char ansi_buf[256];
 * ansi_init(uart_putc, NULL, ansi_buf, sizeof(ansi_buf));
 * @endcode
 */
void ansi_init(ansi_putc_function putc_fn, ansi_flush_function flush_fn,
               char *buf, size_t buf_size);

/**
 * @brief Enable ANSI output with automatic terminal detection.
 *
 * Performs platform-specific console setup and auto-detects whether color
 * output should be enabled:
 *
 * 1. **Windows console setup** -- sets the output codepage to UTF-8 and
 *    enables Virtual Terminal Processing for ANSI escape sequences.
 * 2. **NO_COLOR** -- if the `NO_COLOR` environment variable is set
 *    (any value), color output is disabled. See https://no-color.org/.
 * 3. **isatty** -- if stdout is not a terminal (e.g. piped to a file),
 *    color output is disabled (POSIX/Windows only).
 *
 * On embedded/freestanding targets this function is not needed -- color
 * output is enabled by default after ansi_init(). Use ansi_set_enabled()
 * for explicit control.
 *
 * @note Should be called once at program startup before any color printing.
 */
void ansi_enable(void);

/**
 * @brief Enable or disable color output globally.
 *
 * When disabled, all color printing functions output plain text without
 * ANSI escape codes.
 *
 * If ansi_enable() detected the NO_COLOR environment variable, this
 * function has no effect -- color remains permanently disabled.
 *
 * @param enabled Non-zero to enable color, zero to disable.
 */
void ansi_set_enabled(int enabled);

/**
 * @brief Check if color output is currently enabled.
 *
 * @return Non-zero if color is enabled, zero if disabled.
 */
int ansi_is_enabled(void);

/**
 * @brief Toggle color enable/disable state.
 *
 * Flips the current color enable state. If enabled, becomes disabled.
 * If disabled, becomes enabled.
 *
 * If ansi_enable() detected the NO_COLOR environment variable, this
 * function has no effect -- color remains permanently disabled.
 */
void ansi_toggle(void);

/**
 * @brief Set the default foreground color.
 *
 * Sets a baseline foreground color that is restored whenever formatting is
 * reset with `[/]` or when a selective close tag removes the active
 * foreground (e.g. `[/red]`). Also immediately applies the color.
 *
 * @param color  Color name (e.g. "white", "green"), or NULL to clear
 *               the default and revert to terminal default on reset.
 *
 * @code
 * ansi_set_fg("white");
 * ansi_set_bg("blue");
 * ansi_print("[red]Error[/] back to white on blue\n");
 * @endcode
 */
void ansi_set_fg(const char *color);

/**
 * @brief Set the default background color.
 *
 * Sets a baseline background color that is restored whenever formatting is
 * reset with `[/]` or when a selective close tag removes the active
 * background. Also immediately applies the color.
 *
 * @param color  Color name (e.g. "red", "blue"), or NULL to clear
 *               the default and revert to terminal default on reset.
 */
void ansi_set_bg(const char *color);

/**
 * @brief Rich-style printf with inline markup tags.
 *
 * Printf-style function that supports Python Rich-like markup for colors,
 * backgrounds, and text styles. Uses the caller-provided buffer from ansi_init().
 *
 * **Inspired by Python Rich library** (https://github.com/Textualize/rich)
 * Adapted for embedded C with no dynamic allocation and sequential tag model.
 *
 * @section ansi_print_syntax Tag Syntax
 *
 * **Colors** (foreground):
 * - Standard: black, red, green, yellow, blue, magenta, cyan, white
 * - Extended: orange, pink, purple, brown, teal, lime, navy, olive,
 *             maroon, aqua, silver, gray
 * - Bright: bright_black, bright_red, bright_green, bright_yellow,
 *           bright_blue, bright_magenta, bright_cyan, bright_white
 *
 * **Styles**:
 * - bold, dim, italic, underline, invert, strikethrough
 *
 * **Special effects**:
 * - [rainbow] - Per-character rainbow stretched across text (21-step palette)
 * - [gradient color1 color2] - Per-character linear interpolation between
 *   two named colors using 24-bit true color. Pre-scans text length so the
 *   gradient spans exactly to [/gradient] or [/].
 *   Example: [gradient red blue]smooth transition[/gradient]
 *
 * **Emoji** (Rich-style :name: shortcodes):
 * Core (21): check, cross, warning, info, arrow, gear, clock, hourglass,
 *   thumbs_up, thumbs_down, star, fire, rocket, zap, bug, wrench, bell,
 *   sparkles, package, link, stop
 * Extended (ANSI_PRINT_EXTENDED_EMOJI, ~130 more):
 *   Faces: smile, grin, laugh, wink, heart_eyes, cool, thinking, cry, skull
 *   Hands: wave, clap, pray, muscle, victory, point_up, point_down
 *   Symbols: heart, question, exclamation, plus, minus, infinity, recycle
 *   Arrows: arrow_up, arrow_down, arrow_left, arrow_right, back, forward
 *   Objects: key, lock, unlock, shield, bomb, hammer, scissors, pencil,
 *     magnifier, clipboard, calendar, envelope, phone, laptop, folder, file
 *   Nature: sun, moon, cloud, snow, earth, tree, coffee, beer
 *   Awards: trophy, medal, crown, gem, money, gift, party, confetti
 *   Colored boxes: red_box, orange_box, yellow_box, green_box, blue_box,
 *     purple_box, brown_box, white_box, black_box
 * - Emoji are Unicode characters, not ANSI codes -- always emitted even
 *   when color is disabled via ansi_set_enabled(0).
 * - Unknown :names: pass through as literal text.
 * - Use :: to produce a literal colon (like [[ produces a literal bracket).
 *
 * **Unicode codepoint escapes** (ANSI_PRINT_UNICODE):
 * - Syntax: :U-XXXX: where XXXX is 1-6 hex digits (case-insensitive)
 * - Range: U+0001 to U+10FFFF (full Unicode)
 * - Examples: :U-2714: (✔), :U-1F525: (🔥), :U-41: (A)
 * - Invalid codepoints pass through as literal text.
 * - Always emitted regardless of color enable/disable state.
 *
 * **Backgrounds** (use "on" keyword):
 * - Same color names as foreground
 *
 * **Close tags**:
 * - [/] - Reset all formatting (clears all colors and styles)
 * - [/red] - Close only red foreground color (selective close)
 * - [/bold] - Close only bold style (selective close)
 * - [/red on black] - Close both red foreground and black background (selective)
 *
 * **Selective close behavior**:
 * When you close a specific tag (e.g., [/red]), only that formatting is removed.
 * Other active formatting (e.g., bold) remains active. This matches Python Rich behavior.
 *
 * @section ansi_print_limitations CRITICAL Limitation: No Tag Nesting
 *
 * **IMPORTANT**: This implementation does NOT support tag nesting or a tag stack.
 * Unlike some markup systems, you cannot nest tags within tags and expect inner
 * tags to close before outer tags in the typical nesting sense.
 *
 * **What this means**:
 * - Tags are tracked by ACTIVE STATE, not by a stack
 * - When you open [cyan], cyan becomes the active foreground color
 * - When you open [red] while cyan is active, red REPLACES cyan (not nested inside it)
 * - Closing [/red] removes red from active state, but does NOT restore cyan
 * - After [/red], there is NO active foreground color (reset to terminal default)
 *
 * **Pattern that WORKS** (sequential tags):
 * @code
 * ansi_print("[red]Error:[/red] [cyan]Additional info[/cyan]\n");
 * ansi_print("[bold][green]Status: OK[/green] - [red]Some errors[/red][/bold]\n");
 * @endcode
 *
 * **Pattern that DOES NOT WORK** (apparent nesting):
 * @code
 * ansi_print("[cyan]Outer [red]inner[/red] still cyan[/cyan]\n");
 * // After [/red], cyan is NOT restored - text prints in default color!
 * @endcode
 *
 * **Workaround for "nesting" effect**:
 * If you need to change colors mid-text and return to the previous color,
 * close and re-open the outer tag:
 * @code
 * ansi_print("[cyan]Before [/cyan][red]different[/red][cyan] after[/cyan]\n");
 * @endcode
 *
 * This limitation keeps the implementation simple and efficient for embedded
 * systems, with no dynamic allocation and minimal state tracking.
 *
 * @section ansi_print_examples Examples
 * @code
 * // Simple color
 * ansi_print("[red]Error: %s[/]\n", error_msg);
 *
 * // Style + color
 * ansi_print("[bold red]Warning: %d[/]\n", count);
 *
 * // Foreground + background
 * ansi_print("[white on red]Critical![/]\n");
 *
 * // Everything combined
 * ansi_print("[bold yellow on black]System Status: %s[/]\n", status);
 *
 * // Multiple sections with complete reset
 * ansi_print("[green]Success:[/] Processed [bold]%d[/] items\n", count);
 *
 * // Selective close - close only color, keep bold
 * ansi_print("[bold][red]Error:[/red] still bold text[/bold]\n");
 *
 * // Nested formatting with selective close
 * ansi_print("[bold][green]Success[/green] and [red]Error[/red] messages[/bold]\n");
 *
 * // Close everything with [/]
 * ansi_print("[bold red on black]Critical[/] back to normal\n");
 *
 * // Invert foreground/background colors
 * ansi_print("[invert]Inverted text[/]\n");
 * ansi_print("[red][invert]Red bg with default fg[/][/]\n");
 *
 * // Strikethrough text
 * ansi_print("[strikethrough]Deprecated function[/]\n");
 * ansi_print("[dim strikethrough]Removed feature[/]\n");
 *
 * // Rainbow effect (cycles through 21-color palette)
 * ansi_print("[bold rainbow]Rainbow text[/]\n");
 *
 * // Gradient between two named colors (24-bit true color)
 * ansi_print("[gradient red blue]Smooth transition[/gradient]\n");
 * ansi_print("[bold][gradient cyan magenta]Styled gradient[/gradient][/bold]\n");
 *
 * // Emoji in status messages
 * ansi_print(":check: [green]All tests passed[/]\n");
 * ansi_print(":warning: [yellow]Low memory[/]\n");
 * ansi_print(":cross: [red]Build failed[/]\n");
 *
 * // Emoji with color (inherits active style)
 * ansi_print("[green]:check:[/] Success\n");
 *
 * // Colored boxes for status indicators
 * ansi_print(":green_box: Pass  :red_box: Fail\n");
 *
 * // Unicode codepoint escapes
 * ansi_print(":U-2714: done\n");       // ✔ done
 * ansi_print(":U-1F525: on fire\n");   // 🔥 on fire
 *
 * // Literal colon (escaped)
 * ansi_print("Time:: 12::30\n");  // outputs: Time: 12:30
 * @endcode
 *
 * @param fmt Printf-style format string with markup tags
 * @param ... Variable arguments for printf formatting
 *
 * @note Maximum output length is limited by the buffer passed to ansi_init()
 * @note Respects global color enable/disable state
 * @note Output is automatically flushed via injected flush function
 * @note Unknown tags are silently ignored
 * @note No dynamic memory allocation - uses caller-provided static buffer
 */
void ansi_print(const char *fmt, ...);

/**
 * @brief va_list variant of ansi_print().
 *
 * Identical to ansi_print() but accepts a va_list instead of variadic
 * arguments.  Useful for wrapper functions that forward format strings.
 *
 * @param fmt  Printf-style format string with optional markup tags.
 * @param ap   Argument list initialized with va_start().
 */
void ansi_vprint(const char *fmt, va_list ap);

/**
 * @brief Format a string with Rich-style inline markup but do not emit it.
 *
 * Performs printf-style formatting into the internal buffer and returns a
 * pointer to the result.  The markup tags are left intact (not processed),
 * making this useful for logging to files or passing markup strings to
 * other functions.
 *
 * @param fmt  Printf-style format string with optional markup tags.
 * @return Pointer to the internal format buffer, or NULL on error.
 *
 * @warning The returned pointer is only valid until the next call to
 *          ansi_format(), ansi_print(), or ansi_banner().  All three
 *          share the same internal buffer.
 */
const char *ansi_format(const char *fmt, ...);

/**
 * @brief Access the shared format buffer used by ansi_print/ansi_format.
 *
 * Returns a pointer to the internal format buffer and its size.
 * The caller may write into this buffer (e.g. with snprintf) and then
 * pass the result to ansi_puts().  The buffer is only valid until the
 * next call to ansi_print(), ansi_format(), or ansi_banner().
 *
 * @param out_size  If non-NULL, receives the buffer size in bytes.
 * @return Pointer to the internal buffer, or NULL if not initialized.
 */
char *ansi_get_buf(size_t *out_size);

/**
 * @brief Output a static string with Rich-style inline markup tags.
 *
 * Lightweight alternative to ansi_print() for strings that contain no printf-style
 * format specifiers. Bypasses vsnprintf entirely, processing markup tags
 * directly from the input string. This avoids the format buffer copy and
 * parse overhead, making it faster for fixed status messages, labels, and
 * headings.
 *
 * Supports the same tag syntax as ansi_print(): colors, styles, backgrounds,
 * selective close tags, and bracket escaping ([[, ]]).
 *
 * @param s  Null-terminated string with optional markup tags.
 *           Must NOT contain printf format specifiers (%d, %s, etc.).
 *
 * @note Does not use the format buffer passed to ansi_init()
 * @note Respects global color enable/disable state
 * @note Output is automatically flushed via injected flush function
 *
 * @code
 * ansi_puts("[bold green]System Ready[/]\n");
 * ansi_puts("[red]Error:[/] out of memory\n");
 * @endcode
 *
 * @see ansi_print() for format-string variant with %d, %s, etc.
 */
void ansi_puts(const char *s);

#if ANSI_PRINT_BANNER || ANSI_PRINT_WINDOW
/**
 * @brief Text alignment for ansi_banner() and ansi_window functions.
 */
typedef enum {
    ANSI_ALIGN_LEFT,    /**< Left-align text (default). */
    ANSI_ALIGN_CENTER,  /**< Center text within the box. */
    ANSI_ALIGN_RIGHT    /**< Right-align text within the box. */
} ansi_align_t;
#endif

#if ANSI_PRINT_BANNER
/**
 * @brief Print text inside a colored Unicode box border.
 *
 * Draws a box around the formatted text using the character set selected
 * by ANSI_PRINT_BOX_STYLE (default: double-line ╔═╗║╚═╝), with both
 * the border and text rendered in the specified foreground color. Useful
 * for prominent status messages, error banners, and pass/fail indicators.
 *
 * The text is printf-formatted first. Embedded newlines create additional
 * rows inside the box. Markup tags are NOT processed in the text.
 *
 * @param color  Color name (e.g. "red", "green", "cyan") from the standard,
 *               extended, or bright color tables. NULL or unknown names
 *               produce an uncolored banner.
 * @param width  Interior width in visible characters. Pass 0 to
 *               auto-size to the longest line. Lines longer than width are
 *               truncated; shorter lines are padded with spaces. Multi-byte
 *               UTF-8 characters count as one visible character.
 * @param align  Text alignment: ANSI_ALIGN_LEFT, ANSI_ALIGN_CENTER, or
 *               ANSI_ALIGN_RIGHT.
 * @param fmt    Printf-style format string for the banner text.
 * @param ...    Variable arguments for printf formatting.
 *
 * @warning The entire formatted string (all lines) must fit in the buffer
 *          provided to ansi_init(). Text that exceeds the buffer is silently
 *          truncated, which may lose trailing lines. For large multi-line
 *          content, prefer ansi_window_line() which formats one line at a time.
 *
 * @warning Uses the same internal buffer as ansi_format() and ansi_print().
 *          A pointer returned by ansi_format() is invalidated by calling
 *          this function.
 *
 * Example: `ansi_banner("red", 0, ANSI_ALIGN_LEFT, "Line1\nLine2");`
 * produces a ╔═══╗ / ║ … ║ / ╚═══╝ box with one row per line.
 */
void ansi_banner(const char *color, int width, ansi_align_t align,
                 const char *fmt, ...);
#endif

#if ANSI_PRINT_WINDOW
/**
 * @brief Begin a boxed window with an optional title.
 *
 * Emits the top border and, if a non-NULL non-empty title is provided,
 * a title line followed by a horizontal separator. Subsequent calls to
 * ansi_window_line() add content rows. Call ansi_window_end() to close.
 *
 * @param color  Color name for ALL borders (e.g. "cyan", "green").
 *               Applied to top/bottom borders, separator, and side ║ chars.
 *               NULL or unknown names produce an uncolored border.
 *               This color is also used by ansi_window_end().
 * @param width  Interior width in visible characters. Must be >= 1.
 * @param align  Alignment of the title text within the box.
 * @param title  Title string (plain text, no markup), or NULL / "" to omit.
 *
 * @warning Uses the same internal buffer as ansi_format() and ansi_print().
 *
 * @code
 * ansi_window_start("cyan", 40, ANSI_ALIGN_CENTER, "Status Report");
 * ansi_window_line(ANSI_ALIGN_LEFT, "[green]Uptime: %d sec[/]", uptime);
 * ansi_window_line(ANSI_ALIGN_LEFT, "[red]Errors: %d[/]", err_count);
 * ansi_window_end();
 * @endcode
 */
void ansi_window_start(const char *color, int width, ansi_align_t align,
                       const char *title);

/**
 * @brief Emit one content line inside a window.
 *
 * Must be called between ansi_window_start() and ansi_window_end().
 * The text is printf-formatted into the shared buffer, then processed
 * through the Rich markup parser — all tag syntax supported by
 * ansi_print() works here (colors, styles, emoji, gradients, etc.).
 *
 * The side borders (║) are drawn in the border color set by
 * ansi_window_start(). Visible character count (excluding markup tags
 * and ANSI codes) is used for width/padding/truncation calculations.
 *
 * @param align  Alignment of this line within the box.
 * @param fmt    Printf-style format string with optional Rich markup tags.
 * @param ...    Variable arguments for printf formatting.
 */
void ansi_window_line(ansi_align_t align, const char *fmt, ...);

/**
 * @brief Close a window by emitting the bottom border.
 *
 * Emits the bottom border in the color set by ansi_window_start(),
 * then flushes output.
 */
void ansi_window_end(void);
#endif

#if ANSI_PRINT_BAR

/** Track character for the unfilled portion of ansi_bar(). */
typedef enum {
    ANSI_BAR_BLANK,   /**< Space -- no visible track         */
    ANSI_BAR_LIGHT,   /**< ░ U+2591  light shade             */
    ANSI_BAR_MED,     /**< ▒ U+2592  medium shade            */
    ANSI_BAR_HEAVY,   /**< ▓ U+2593  dark shade              */
    ANSI_BAR_DOT,     /**< · U+00B7  middle dot              */
    ANSI_BAR_LINE,    /**< ─ U+2500  horizontal line         */
} ansi_bar_track_t;

/**
 * @brief Generate an inline horizontal bar graph string.
 *
 * Builds a string of Rich markup and Unicode block characters (█▉▊▋▌▍▎▏)
 * into the caller-provided buffer. The returned string can be used as a
 * %%s argument to ansi_print(), ansi_window_line(), or any printf-style
 * function.
 *
 * The buffer is passed directly (not via an init function) so that multiple
 * bars can coexist in the same printf argument list -- each call writes to
 * its own buffer with no shared state.
 *
 * The filled portion uses 1/8-character-cell resolution (8 steps per cell),
 * so a 13-character-wide bar provides over 100 discrete steps.
 * The @c track parameter selects the character used for unfilled cells.
 *
 * A buffer of 128 bytes supports bars up to ~30 characters wide.
 * The formula is: width * 3 + 20 (for color tag overhead).
 *
 * @param buf       Pointer to caller-provided output buffer.
 * @param buf_size  Size of the buffer in bytes.
 * @param color     Color name for the filled portion (e.g. "green", "red").
 *                  NULL or unknown names produce uncolored blocks.
 * @param width     Total bar width in character cells. Must be >= 1.
 * @param track     Character for unfilled cells (e.g. ANSI_BAR_LIGHT).
 * @param value     Current value to display.
 * @param min       Minimum of the value range.
 * @param max       Maximum of the value range.
 * @return Pointer to buf (for direct use in printf %%s).
 *
 * @code
 * char bar[128];
 * ansi_bar(bar, sizeof(bar), "green", 20, ANSI_BAR_LIGHT, cpu, 0, 100);
 * ansi_bar(bar, sizeof(bar), "green", 20, ANSI_BAR_BLANK, cpu, 0, 100);
 * @endcode
 */
const char *ansi_bar(char *buf, size_t buf_size,
                     const char *color, int width, ansi_bar_track_t track,
                     double value, double min, double max);

/**
 * @brief Bar graph with " XX%%" appended.
 *
 * Convenience wrapper around ansi_bar() with a fixed 0-100 range.
 * Appends the percentage as an integer (e.g. " 73%") after the bar.
 * The percent value is clamped to [0, 100].
 *
 * @param buf       Pointer to caller-provided output buffer.
 * @param buf_size  Size of the buffer in bytes.
 * @param color     Color name for the filled portion (NULL for uncolored).
 * @param width     Total bar width in character cells.
 * @param track     Character for unfilled cells (e.g. ANSI_BAR_LIGHT).
 * @param percent   Fill percentage (0-100, clamped).
 * @return Pointer to buf.
 *
 * @code
 * char bar[128];
 * ansi_print("CPU %s\n",
 *            ansi_bar_percent(bar, sizeof(bar), "green", 20,
 *                             ANSI_BAR_LIGHT, 73));
 * // Output: CPU ██████████████▌░░░░░ 73%
 * @endcode
 */
const char *ansi_bar_percent(char *buf, size_t buf_size,
                             const char *color, int width,
                             ansi_bar_track_t track, int percent);
#endif

#ifdef __cplusplus
}
#endif

#endif // ANSI_PRINT_H
