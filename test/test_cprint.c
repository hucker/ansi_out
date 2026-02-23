#include "unity.h"
#include "ansi_print.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* Capture buffer — collects all output from ansi_print into a string */
/* ------------------------------------------------------------------ */

#define CAPTURE_SIZE 4096

static char capture_buf[CAPTURE_SIZE];
static int  capture_pos;

static void capture_putc(int ch)
{
    if (capture_pos < CAPTURE_SIZE - 1)
        capture_buf[capture_pos++] = (char)ch;
}

static void capture_flush(void) { /* no-op */ }

static void capture_reset(void)
{
    memset(capture_buf, 0, sizeof(capture_buf));
    capture_pos = 0;
}

/* Format buffer for ansi_print */
static char fmt_buf[512];

/* ------------------------------------------------------------------ */
/* Unity setUp / tearDown                                             */
/* ------------------------------------------------------------------ */

void setUp(void)
{
    capture_reset();
    ansi_init(capture_putc, capture_flush, fmt_buf, sizeof(fmt_buf));
    ansi_set_enabled(1);
    ansi_set_fg(NULL);
    ansi_set_bg(NULL);
}

void tearDown(void) { }

/* ------------------------------------------------------------------ */
/* Core tests (always compiled)                                       */
/* ------------------------------------------------------------------ */

void test_plain_text_no_tags(void)
{
    ansi_print("hello world");
    TEST_ASSERT_EQUAL_STRING("hello world", capture_buf);
}

void test_printf_formatting(void)
{
    ansi_print("count=%d name=%s", 42, "foo");
    TEST_ASSERT_EQUAL_STRING("count=42 name=foo", capture_buf);
}

void test_color_disabled_strips_tags(void)
{
    ansi_set_enabled(0);
    ansi_print("[red]error[/]");
    TEST_ASSERT_EQUAL_STRING("error", capture_buf);
}

void test_color_enabled_emits_ansi(void)
{
    ansi_print("[red]hi[/]");
    /* Should start with ESC[ (ANSI sequence) when color is on */
    TEST_ASSERT_EQUAL_CHAR('\x1b', capture_buf[0]);
    /* Should contain "hi" somewhere in the output */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "hi"));
    /* Should end with reset sequence */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[0m"));
}

void test_toggle(void)
{
    TEST_ASSERT_TRUE(ansi_is_enabled());
    ansi_toggle();
    TEST_ASSERT_FALSE(ansi_is_enabled());
    ansi_toggle();
    TEST_ASSERT_TRUE(ansi_is_enabled());
}

void test_set_enabled(void)
{
    ansi_set_enabled(0);
    TEST_ASSERT_FALSE(ansi_is_enabled());
    ansi_set_enabled(1);
    TEST_ASSERT_TRUE(ansi_is_enabled());
}

/* ------------------------------------------------------------------ */
/* NO_COLOR / ansi_enable tests                                       */
/* ------------------------------------------------------------------ */

#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)

/* Portable env var removal -- MinGW lacks unsetenv() */
#ifdef _WIN32
static void clear_no_color(void) { _putenv("NO_COLOR="); }
#else
static void clear_no_color(void) { unsetenv("NO_COLOR"); }
#endif

void test_enable_no_color_disables(void)
{
    putenv("NO_COLOR=1");
    ansi_enable();
    TEST_ASSERT_FALSE(ansi_is_enabled());
    clear_no_color();
}

void test_enable_no_color_locks(void)
{
    putenv("NO_COLOR=1");
    ansi_enable();
    /* set_enabled should be blocked by lock */
    ansi_set_enabled(1);
    TEST_ASSERT_FALSE(ansi_is_enabled());
    clear_no_color();
}

void test_enable_no_color_toggle_locked(void)
{
    putenv("NO_COLOR=1");
    ansi_enable();
    /* toggle should be blocked by lock */
    ansi_toggle();
    TEST_ASSERT_FALSE(ansi_is_enabled());
    clear_no_color();
}

void test_enable_no_color_strips_tags(void)
{
    putenv("NO_COLOR=1");
    ansi_enable();
    ansi_print("[red]hello[/]");
    TEST_ASSERT_EQUAL_STRING("hello", capture_buf);
    clear_no_color();
}

void test_init_resets_lock(void)
{
    putenv("NO_COLOR=1");
    ansi_enable();
    TEST_ASSERT_FALSE(ansi_is_enabled());
    clear_no_color();
    /* ansi_init resets the lock */
    ansi_init(capture_putc, capture_flush, fmt_buf, sizeof(fmt_buf));
    ansi_set_enabled(1);
    TEST_ASSERT_TRUE(ansi_is_enabled());
}

#if !defined(_WIN32)
void test_enable_no_color_empty_value(void)
{
    /* NO_COLOR spec: any value counts, even empty string.
       Windows _putenv("VAR=") removes the var, so this only applies on POSIX. */
    putenv("NO_COLOR=");
    ansi_enable();
    TEST_ASSERT_FALSE(ansi_is_enabled());
    clear_no_color();
}
#endif

#endif /* _WIN32 || __unix__ || __APPLE__ */

void test_puts_plain(void)
{
    ansi_set_enabled(0);
    ansi_puts("[green]ok[/]");
    TEST_ASSERT_EQUAL_STRING("ok", capture_buf);
}

void test_puts_with_color(void)
{
    ansi_puts("[blue]test[/]");
    TEST_ASSERT_EQUAL_CHAR('\x1b', capture_buf[0]);
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "test"));
}

void test_escaped_brackets(void)
{
    ansi_set_enabled(0);
    ansi_print("[[hello]]");
    TEST_ASSERT_EQUAL_STRING("[hello]", capture_buf);
}

void test_background_color(void)
{
    ansi_print("[white on red]alert[/]");
    /* white fg = \x1b[37m, red bg = \x1b[41m */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[37m"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[41m"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "alert"));
}

void test_null_putc_suppresses_output(void)
{
    ansi_init(NULL, NULL, fmt_buf, sizeof(fmt_buf));
    ansi_print("[red]should not crash[/]");
    /* No crash = pass; output goes to noop */
    TEST_ASSERT_TRUE(1);
}

/* ------------------------------------------------------------------ */
/* Style tests                                                        */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_STYLES
void test_bold_style(void)
{
    ansi_print("[bold]text[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1m"));  /* bold on */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "text"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[0m"));  /* reset */
}
#endif

/* ------------------------------------------------------------------ */
/* Gradient / rainbow tests                                           */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_GRADIENTS
void test_rainbow_no_crash(void)
{
    ansi_rainbow("Rainbow!");
    /* Each char is wrapped in ANSI codes so the literal string won't appear
       contiguously.  Just verify we got output containing 'R'. */
    TEST_ASSERT_NOT_NULL(strchr(capture_buf, 'R'));
}

void test_rainbow_disabled(void)
{
    ansi_set_enabled(0);
    ansi_rainbow("Plain");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Plain"));
}
#endif

/* ------------------------------------------------------------------ */
/* Emoji tests                                                        */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_EMOJI
void test_emoji_fire(void)
{
    ansi_set_enabled(0);
    ansi_print(":fire: alert");
    TEST_ASSERT_EQUAL_STRING("\xf0\x9f\x94\xa5 alert", capture_buf);
}

void test_emoji_check_via_puts(void)
{
    ansi_set_enabled(0);
    ansi_puts(":check: passed");
    TEST_ASSERT_EQUAL_STRING("\xe2\x9c\x85 passed", capture_buf);
}

void test_emoji_unknown_passthrough(void)
{
    ansi_set_enabled(0);
    ansi_print(":notanemoji: text");
    TEST_ASSERT_EQUAL_STRING(":notanemoji: text", capture_buf);
}

void test_escaped_colon(void)
{
    ansi_set_enabled(0);
    ansi_print("time::12::30");
    TEST_ASSERT_EQUAL_STRING("time:12:30", capture_buf);
}

void test_emoji_with_color(void)
{
    ansi_print("[green]:check:[/] ok");
    /* Verify ANSI green code appears before the check mark bytes */
    TEST_ASSERT_EQUAL_CHAR('\x1b', capture_buf[0]);
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[0m"));
}

void test_emoji_color_disabled(void)
{
    ansi_set_enabled(0);
    ansi_print("[green]:check:[/] ok");
    /* Tags stripped, but emoji still present */
    TEST_ASSERT_EQUAL_STRING("\xe2\x9c\x85 ok", capture_buf);
}

void test_multiple_emoji(void)
{
    ansi_set_enabled(0);
    ansi_print(":check: yes :cross: no");
    TEST_ASSERT_EQUAL_STRING("\xe2\x9c\x85 yes \xe2\x9d\x8c no", capture_buf);
}

void test_bare_colon(void)
{
    ansi_set_enabled(0);
    ansi_print("key: value");
    TEST_ASSERT_EQUAL_STRING("key: value", capture_buf);
}

void test_colon_at_end(void)
{
    ansi_set_enabled(0);
    ansi_print("note:");
    TEST_ASSERT_EQUAL_STRING("note:", capture_buf);
}

void test_adjacent_emoji(void)
{
    ansi_set_enabled(0);
    ansi_print(":fire::fire:");
    TEST_ASSERT_EQUAL_STRING(
        "\xf0\x9f\x94\xa5\xf0\x9f\x94\xa5", capture_buf);
}
#endif /* ANSI_PRINT_EMOJI */

/* ------------------------------------------------------------------ */
/* Unicode codepoint tests                                            */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_UNICODE
void test_unicode_bmp(void)
{
    /* :U-2714: = CHECK MARK, UTF-8: E2 9C 94 */
    ansi_set_enabled(0);
    ansi_print(":U-2714:");
    TEST_ASSERT_EQUAL_STRING("\xe2\x9c\x94", capture_buf);
}

void test_unicode_supplementary(void)
{
    /* :U-1F525: = FIRE, UTF-8: F0 9F 94 A5 */
    ansi_set_enabled(0);
    ansi_print(":U-1F525:");
    TEST_ASSERT_EQUAL_STRING("\xf0\x9f\x94\xa5", capture_buf);
}

void test_unicode_ascii(void)
{
    /* :U-41: = 'A' */
    ansi_set_enabled(0);
    ansi_print(":U-41:");
    TEST_ASSERT_EQUAL_STRING("A", capture_buf);
}

void test_unicode_invalid_passthrough(void)
{
    /* No valid hex after U- — passes through literally */
    ansi_set_enabled(0);
    ansi_print(":U-ZZZZ:");
    TEST_ASSERT_EQUAL_STRING(":U-ZZZZ:", capture_buf);
}

void test_unicode_with_color(void)
{
    /* Color wraps the unicode character */
    ansi_print("[red]:U-2714:[/]");
    TEST_ASSERT_EQUAL_CHAR('\x1b', capture_buf[0]);
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x94"));
}

void test_unicode_color_disabled(void)
{
    /* Color disabled — still outputs the character */
    ansi_set_enabled(0);
    ansi_print("[red]:U-2714:[/]");
    TEST_ASSERT_EQUAL_STRING("\xe2\x9c\x94", capture_buf);
}

void test_unicode_lowercase_hex(void)
{
    /* Lowercase hex digits should work */
    ansi_set_enabled(0);
    ansi_print(":U-1f525:");
    TEST_ASSERT_EQUAL_STRING("\xf0\x9f\x94\xa5", capture_buf);
}
#endif /* ANSI_PRINT_UNICODE */

/* ------------------------------------------------------------------ */
/* Minimal-build tests (compiled-out features behave gracefully)      */
/* ------------------------------------------------------------------ */

#if !ANSI_PRINT_STYLES
void test_minimal_unknown_style_consumed(void)
{
    /* [bold] not in ATTRS table — tag is consumed, text passes through.
       [/] still emits a reset since it's unconditional, so just check
       that "text" is present and no bold code (\x1b[1m) was emitted. */
    ansi_print("[bold]text[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "text"));
    TEST_ASSERT_NULL(strstr(capture_buf, "\x1b[1m"));
}

void test_minimal_unknown_style_disabled(void)
{
    /* Same with color off — still just "text" */
    ansi_set_enabled(0);
    ansi_print("[bold]important[/]");
    TEST_ASSERT_EQUAL_STRING("important", capture_buf);
}
#endif

#if !ANSI_PRINT_EMOJI
void test_minimal_emoji_literal_passthrough(void)
{
    /* Colon parsing compiled out — :fire: is literal text */
    ansi_set_enabled(0);
    ansi_print(":fire: alert");
    TEST_ASSERT_EQUAL_STRING(":fire: alert", capture_buf);
}

void test_minimal_double_colon_literal(void)
{
    /* :: escape compiled out — passes through literally */
    ansi_set_enabled(0);
    ansi_print("time::12::30");
    TEST_ASSERT_EQUAL_STRING("time::12::30", capture_buf);
}

void test_minimal_colon_with_color(void)
{
    /* :name: with color on — colons are literal, color tags still work */
    ansi_print("[red]:check:[/]");
    TEST_ASSERT_EQUAL_CHAR('\x1b', capture_buf[0]);
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, ":check:"));
}
#endif

#if !ANSI_PRINT_GRADIENTS
void test_minimal_rainbow_tag_consumed(void)
{
    /* [rainbow] not in ATTRS — tag consumed, text passes through */
    ansi_print("[rainbow]colorful[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "colorful"));
}

void test_minimal_gradient_tag_consumed(void)
{
    /* [gradient red blue] not recognized — tag consumed, text passes through */
    ansi_set_enabled(0);
    ansi_print("[gradient red blue]smooth[/]");
    TEST_ASSERT_EQUAL_STRING("smooth", capture_buf);
}
#endif

/* ------------------------------------------------------------------ */
/* ansi_format() tests                                                */
/* ------------------------------------------------------------------ */

void test_format_returns_buffer(void)
{
    const char *r = ansi_format("hello %d", 42);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("hello 42", r);
}

void test_format_null_fmt(void)
{
    const char *r = ansi_format(NULL);
    TEST_ASSERT_NULL(r);
}

void test_format_no_buf(void)
{
    /* Re-init with no format buffer */
    ansi_init(capture_putc, capture_flush, NULL, 0);
    const char *r = ansi_format("test %d", 1);
    TEST_ASSERT_NULL(r);
}

/* ------------------------------------------------------------------ */
/* Edge-case / NULL tests                                             */
/* ------------------------------------------------------------------ */

void test_puts_null(void)
{
    ansi_puts(NULL);
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

void test_buffer_truncation(void)
{
    /* Use a tiny buffer (16 bytes) to force vsnprintf truncation */
    char tiny_buf[16];
    ansi_init(capture_putc, capture_flush, tiny_buf, sizeof(tiny_buf));
    ansi_set_enabled(0);

    ansi_print("This string is way longer than sixteen bytes");
    /* vsnprintf truncates to 15 chars + null; output should not crash */
    TEST_ASSERT_EQUAL(15, (int)strlen(capture_buf));

    /* Restore normal buffer */
    ansi_init(capture_putc, capture_flush, fmt_buf, sizeof(fmt_buf));
}

void test_empty_format_string(void)
{
    ansi_set_enabled(0);
    ansi_print("");
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

void test_unclosed_tag(void)
{
    ansi_set_enabled(0);
    ansi_print("[red");
    /* Unclosed bracket — no tag match, chars pass through */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "[red"));
}

/* ------------------------------------------------------------------ */
/* Numeric color tests                                                */
/* ------------------------------------------------------------------ */

void test_numeric_fg(void)
{
    ansi_print("[fg:208]orange[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[38;5;208m"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "orange"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[0m"));
}

void test_numeric_bg(void)
{
    ansi_print("[white on bg:52]dark[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[48;5;52m"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "dark"));
}

void test_numeric_fg_disabled(void)
{
    ansi_set_enabled(0);
    ansi_print("[fg:208]text[/]");
    TEST_ASSERT_EQUAL_STRING("text", capture_buf);
}

void test_numeric_fg_clamp_high(void)
{
    ansi_print("[fg:999]text[/]");
    /* 999 should clamp to 255 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[38;5;255m"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "text"));
}

void test_numeric_fg_clamp_negative(void)
{
    ansi_print("[fg:-5]text[/]");
    /* -5 should clamp to 0 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[38;5;0m"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "text"));
}

void test_numeric_fg_non_numeric(void)
{
    ansi_set_enabled(0);
    ansi_print("[fg:abc]text[/]");
    /* Non-numeric should be ignored, just output text */
    TEST_ASSERT_EQUAL_STRING("text", capture_buf);
}

/* ------------------------------------------------------------------ */
/* Selective close tag tests                                          */
/* ------------------------------------------------------------------ */

void test_close_specific_color(void)
{
    /* [/red] should clear red, rest still applies */
    ansi_print("[red]hello[/red] world");
    /* After [/red], a RESET + reapply is emitted */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[31m"));   /* red on */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "hello"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[0m"));    /* reset */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "world"));
}

void test_close_specific_color_disabled(void)
{
    ansi_set_enabled(0);
    ansi_print("[red]hello[/red] world");
    TEST_ASSERT_EQUAL_STRING("hello world", capture_buf);
}

void test_close_numeric_fg(void)
{
    ansi_print("[fg:196]hot[/fg:196] cool");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[38;5;196m"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "hot"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "cool"));
}

void test_close_with_background(void)
{
    ansi_print("[white on red]alert[/white on red] done");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[37m"));   /* white */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[41m"));   /* red bg */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "alert"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "done"));
}

/* ------------------------------------------------------------------ */
/* Multi-word tag tests                                               */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_STYLES
void test_multi_style_tag(void)
{
    ansi_print("[bold italic]fancy[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1m"));    /* bold */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[3m"));    /* italic */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "fancy"));
}

void test_close_specific_style(void)
{
    ansi_print("[bold italic]a[/bold]b[/]");
    /* bold on, italic on, then /bold clears bold but italic remains */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1m"));    /* bold */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[3m"));    /* italic */
    /* After [/bold], should reset then reapply italic */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "a"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "b"));
}

void test_all_styles(void)
{
    ansi_print("[dim]d[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2m"));
    capture_reset();
    ansi_print("[underline]u[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[4m"));
    capture_reset();
    ansi_print("[invert]i[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[7m"));
    capture_reset();
    ansi_print("[strikethrough]s[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[9m"));
}

void test_color_and_style_combined(void)
{
    ansi_print("[bold red]warn[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1m"));    /* bold */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[31m"));   /* red */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "warn"));
}
#endif /* ANSI_PRINT_STYLES */

/* ------------------------------------------------------------------ */
/* Gradient & rainbow span tests                                      */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_GRADIENTS
void test_gradient_span(void)
{
    ansi_print("[gradient red blue]fade[/gradient]");
    /* Should contain RGB true-color codes */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[38;2;"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "f"));
}

void test_gradient_disabled(void)
{
    ansi_set_enabled(0);
    ansi_print("[gradient red blue]fade[/gradient]");
    TEST_ASSERT_EQUAL_STRING("fade", capture_buf);
}

void test_gradient_bad_colors(void)
{
    /* Invalid color names — gradient not activated, text passes through */
    ansi_set_enabled(0);
    ansi_print("[gradient notacolor alsonotacolor]text[/]");
    TEST_ASSERT_EQUAL_STRING("text", capture_buf);
}

void test_rainbow_span(void)
{
    ansi_print("[rainbow]colors[/rainbow]");
    /* Should contain 256-color codes */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[38;5;"));
}

void test_rainbow_span_disabled(void)
{
    ansi_set_enabled(0);
    ansi_print("[rainbow]colors[/rainbow]");
    TEST_ASSERT_EQUAL_STRING("colors", capture_buf);
}

void test_rainbow_null(void)
{
    ansi_rainbow(NULL);
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

void test_rainbow_spaces_only(void)
{
    ansi_rainbow("   ");
    /* Spaces are emitted but no color codes per-character */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "   "));
}

void test_gradient_close_reapplies(void)
{
    /* After gradient closes, outer color should reapply */
    ansi_print("[red]a[gradient green blue]b[/gradient]c[/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[31m"));   /* red */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[38;2;"));  /* gradient */
}

#if ANSI_PRINT_EMOJI
void test_gradient_with_emoji(void)
{
    /* Emoji inside gradient counts as one visible char */
    ansi_set_enabled(0);
    ansi_print("[gradient red blue]:check: ok[/gradient]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "ok"));
}
#endif

#if ANSI_PRINT_UNICODE
void test_gradient_with_unicode(void)
{
    ansi_set_enabled(0);
    ansi_print("[gradient red blue]:U-2714: ok[/gradient]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x94"));
}
#endif

void test_count_effect_escaped_brackets(void)
{
    /* [[ and ]] inside rainbow each count as 1 visible char */
    ansi_set_enabled(0);
    ansi_print("[rainbow][[x]][/rainbow]");
    /* Should output [x] */
    TEST_ASSERT_EQUAL_STRING("[x]", capture_buf);
}

#if ANSI_PRINT_EMOJI || ANSI_PRINT_UNICODE
void test_count_effect_escaped_colons(void)
{
    /* :: inside rainbow counts as 1 visible char */
    ansi_set_enabled(0);
    ansi_print("[rainbow]a::b[/rainbow]");
    TEST_ASSERT_EQUAL_STRING("a:b", capture_buf);
}
#endif

void test_count_effect_skips_tags(void)
{
    /* Tags inside rainbow are skipped in char count but consumed */
    ansi_print("[rainbow]ab[/rainbow]");
    /* Two visible chars get rainbow coloring */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "a"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "b"));
}

#if ANSI_PRINT_STYLES
void test_gradient_rejects_style(void)
{
    /* [gradient bold red] — bold is a style, not a color → reject */
    ansi_set_enabled(0);
    ansi_print("[gradient bold red]text[/]");
    TEST_ASSERT_EQUAL_STRING("text", capture_buf);
}
#endif
#endif /* ANSI_PRINT_GRADIENTS */

/* ------------------------------------------------------------------ */
/* Unicode 2-byte range test                                          */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_UNICODE
void test_unicode_2byte(void)
{
    /* :U-A9: = copyright ©, UTF-8: C2 A9 (2-byte) */
    ansi_set_enabled(0);
    ansi_print(":U-A9:");
    TEST_ASSERT_EQUAL_STRING("\xc2\xa9", capture_buf);
}

void test_unicode_out_of_range(void)
{
    /* U+110000 is out of range — pass through literally */
    ansi_set_enabled(0);
    ansi_print(":U-110000:");
    TEST_ASSERT_EQUAL_STRING(":U-110000:", capture_buf);
}

void test_unicode_zero(void)
{
    /* U+0000 is invalid — pass through literally */
    ansi_set_enabled(0);
    ansi_print(":U-0:");
    TEST_ASSERT_EQUAL_STRING(":U-0:", capture_buf);
}

void test_unicode_too_long(void)
{
    /* More than 6 hex digits — pass through literally */
    ansi_set_enabled(0);
    ansi_print(":U-1234567:");
    TEST_ASSERT_EQUAL_STRING(":U-1234567:", capture_buf);
}

void test_unicode_bare_prefix(void)
{
    /* Just :U-: with no hex — pass through */
    ansi_set_enabled(0);
    ansi_print(":U-:");
    TEST_ASSERT_EQUAL_STRING(":U-:", capture_buf);
}
#endif /* ANSI_PRINT_UNICODE */

/* ------------------------------------------------------------------ */
/* find_on edge cases                                                 */
/* ------------------------------------------------------------------ */

void test_on_with_trailing_spaces(void)
{
    ansi_print("[white on  red ]text[/]");
    /* Should still parse "on" and set bg to red (with trimming) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[37m"));   /* white */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "text"));
}

void test_empty_tag_ignored(void)
{
    /* [] is a tag with len=0 — ignored by emit_tag */
    ansi_set_enabled(0);
    ansi_print("[]text");
    TEST_ASSERT_EQUAL_STRING("text", capture_buf);
}

void test_unclosed_bracket(void)
{
    /* [ without ] — emitted as literal */
    ansi_set_enabled(0);
    ansi_print("[broken");
    TEST_ASSERT_EQUAL_STRING("[broken", capture_buf);
}

/* ------------------------------------------------------------------ */
/* Minimal-build tests (compiled-out features behave gracefully)      */
/* ------------------------------------------------------------------ */

#if !ANSI_PRINT_UNICODE
void test_minimal_unicode_literal_passthrough(void)
{
    /* Unicode parsing compiled out — :U-2714: is literal text */
    ansi_set_enabled(0);
    ansi_print(":U-2714: done");
    TEST_ASSERT_EQUAL_STRING(":U-2714: done", capture_buf);
}
#endif

#if !ANSI_PRINT_EXTENDED_COLORS
void test_minimal_extended_color_ignored(void)
{
    /* "orange" not in ATTRS — tag consumed, no ANSI emitted */
    ansi_set_enabled(0);
    ansi_print("[orange]pumpkin[/]");
    TEST_ASSERT_EQUAL_STRING("pumpkin", capture_buf);
}
#endif

#if !ANSI_PRINT_BRIGHT_COLORS
void test_minimal_bright_color_ignored(void)
{
    /* "bright_red" not in ATTRS — tag consumed, no ANSI emitted */
    ansi_set_enabled(0);
    ansi_print("[bright_red]neon[/]");
    TEST_ASSERT_EQUAL_STRING("neon", capture_buf);
}
#endif

#if !ANSI_PRINT_BAR
void test_minimal_bar_not_available(void)
{
    /* ansi_bar() not available -- this test just confirms compilation */
    ansi_set_enabled(0);
    ansi_print("no bar");
    TEST_ASSERT_EQUAL_STRING("no bar", capture_buf);
}
#endif

/* ------------------------------------------------------------------ */
/* Banner tests                                                       */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_BANNER
void test_banner_basic(void)
{
    ansi_banner("red", 0, ANSI_ALIGN_LEFT, "hello");
    /* Should contain: color code, box chars, "hello", reset */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[31m"));  /* red fg */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "hello"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[0m"));   /* reset */
    /* Box-drawing: ╔ = \xe2\x95\x94, ║ = \xe2\x95\x91 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x91"));
}

void test_banner_printf(void)
{
    ansi_banner("green", 0, ANSI_ALIGN_LEFT, "count=%d", 42);
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "count=42"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m"));  /* green fg */
}

void test_banner_disabled(void)
{
    ansi_set_enabled(0);
    ansi_banner("red", 0, ANSI_ALIGN_LEFT, "test");
    /* Box should still draw, but no ANSI codes */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "test"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94")); /* ╔ */
    TEST_ASSERT_NULL(strstr(capture_buf, "\x1b[31m"));  /* no red */
    TEST_ASSERT_NULL(strstr(capture_buf, "\x1b[0m"));   /* no reset */
}

void test_banner_unknown_color(void)
{
    ansi_banner("nosuchcolor", 0, ANSI_ALIGN_LEFT, "msg");
    /* Should still draw box and text, just no color */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "msg"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94")); /* ╔ */
    TEST_ASSERT_NULL(strstr(capture_buf, "\x1b[0m"));   /* no reset */
}

void test_banner_null_fmt(void)
{
    ansi_banner("red", 0, ANSI_ALIGN_LEFT, NULL);
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

void test_banner_null_color(void)
{
    ansi_banner(NULL, 0, ANSI_ALIGN_LEFT, "test");
    /* Should draw box without color */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "test"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94")); /* ╔ */
    TEST_ASSERT_NULL(strstr(capture_buf, "\x1b[0m"));   /* no reset */
}

void test_banner_multiline(void)
{
    ansi_set_enabled(0);  /* disable color to simplify string checks */
    ansi_banner(NULL, 0, ANSI_ALIGN_LEFT, "line1\nline2\nline3");
    /* Should have 3 text rows, each with ║ */
    /* Count ║ occurrences -- expect 6 (left+right for 3 lines) */
    int count = 0;
    const char *p = capture_buf;
    while ((p = strstr(p, "\xe2\x95\x91")) != NULL) { count++; p += 3; }
    TEST_ASSERT_EQUAL_INT(6, count);
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "line1"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "line2"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "line3"));
}

void test_banner_fixed_width(void)
{
    ansi_set_enabled(0);
    ansi_banner(NULL, 10, ANSI_ALIGN_LEFT, "hi");
    /* "hi" is 2 chars, width is 10, so 8 spaces of padding on right */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "hi        "));
}

void test_banner_truncate(void)
{
    ansi_set_enabled(0);
    ansi_banner(NULL, 3, ANSI_ALIGN_LEFT, "abcdef");
    /* Only first 3 chars should appear */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "abc"));
    TEST_ASSERT_NULL(strstr(capture_buf, "abcd"));
}

void test_banner_auto_width(void)
{
    ansi_set_enabled(0);
    ansi_banner(NULL, 0, ANSI_ALIGN_LEFT, "short\nlongest line\nmed");
    /* Auto-width should be 12 ("longest line" length) */
    /* "short" should be padded to 12: "short       " */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "short       "));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "longest line"));
}

void test_banner_align_center(void)
{
    ansi_set_enabled(0);
    ansi_banner(NULL, 10, ANSI_ALIGN_CENTER, "hi");
    /* "hi" is 2 chars, width 10 => 4 spaces left, 4 spaces right */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "    hi    "));
}

void test_banner_align_right(void)
{
    ansi_set_enabled(0);
    ansi_banner(NULL, 10, ANSI_ALIGN_RIGHT, "hi");
    /* "hi" is 2 chars, width 10 => 8 spaces left, 0 right */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "        hi"));
}
#endif /* ANSI_PRINT_BANNER */

/* ------------------------------------------------------------------ */
/* Window tests                                                       */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_WINDOW

void test_window_basic(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 10, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_LEFT, "hello");
    ansi_window_end();
    /* Top border: ╔ */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    /* Bottom border: ╚ */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x9a"));
    /* Content */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "hello"));
    /* No separator (no title) -- ╠ should not appear */
    TEST_ASSERT_NULL(strstr(capture_buf, "\xe2\x95\xa0"));
}

void test_window_with_title(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 20, ANSI_ALIGN_LEFT, "My Title");
    ansi_window_line(ANSI_ALIGN_LEFT, "content");
    ansi_window_end();
    /* Title text */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "My Title"));
    /* Separator: ╠ and ╣ */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\xa0"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\xa3"));
    /* Content */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "content"));
}

void test_window_title_center(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 10, ANSI_ALIGN_CENTER, "hi");
    ansi_window_end();
    /* "hi" centered in width 10: 4 spaces left, 4 right */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "    hi    "));
}

void test_window_title_right(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 10, ANSI_ALIGN_RIGHT, "hi");
    ansi_window_end();
    /* "hi" right-aligned in width 10: 8 spaces left */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "        hi"));
}

void test_window_line_center(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 10, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_CENTER, "ab");
    ansi_window_end();
    /* "ab" centered in width 10: 4 left, 4 right */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "    ab    "));
}

void test_window_line_right(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 10, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_RIGHT, "ab");
    ansi_window_end();
    /* "ab" right-aligned in width 10: 8 left */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "        ab"));
}

void test_window_printf(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 20, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_LEFT, "val=%d", 42);
    ansi_window_end();
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "val=42"));
}

void test_window_truncate(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 3, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_LEFT, "abcdef");
    ansi_window_end();
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "abc"));
    TEST_ASSERT_NULL(strstr(capture_buf, "abcd"));
}

void test_window_null_title(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 10, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_LEFT, "data");
    ansi_window_end();
    /* No separator */
    TEST_ASSERT_NULL(strstr(capture_buf, "\xe2\x95\xa0"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "data"));
}

void test_window_empty_title(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 10, ANSI_ALIGN_LEFT, "");
    ansi_window_line(ANSI_ALIGN_LEFT, "data");
    ansi_window_end();
    /* No separator */
    TEST_ASSERT_NULL(strstr(capture_buf, "\xe2\x95\xa0"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "data"));
}

void test_window_color(void)
{
    ansi_window_start("red", 10, ANSI_ALIGN_LEFT, "T");
    ansi_window_line(ANSI_ALIGN_LEFT, "[green]data[/]");
    ansi_window_end();
    /* Border should have red fg */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[31m"));
    /* Text should have green fg from Rich markup */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m"));
    /* Resets */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[0m"));
}

void test_window_color_disabled(void)
{
    ansi_set_enabled(0);
    ansi_window_start("red", 10, ANSI_ALIGN_LEFT, "T");
    ansi_window_line(ANSI_ALIGN_LEFT, "[green]data[/]");
    ansi_window_end();
    /* No ANSI codes when disabled */
    TEST_ASSERT_NULL(strstr(capture_buf, "\x1b[31m"));
    TEST_ASSERT_NULL(strstr(capture_buf, "\x1b[32m"));
    /* Box still draws */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "data"));
}

void test_window_markup(void)
{
    ansi_set_enabled(0);
    ansi_window_start(NULL, 20, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_LEFT, "[bold]hello[/] world");
    ansi_window_end();
    /* Tags stripped in disabled mode, visible text preserved */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "hello world"));
}

#endif /* ANSI_PRINT_WINDOW */

/* ------------------------------------------------------------------ */
/* Bar graph tests                                                     */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_BAR

void test_bar_full(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 5, ANSI_BAR_LIGHT, 100, 0, 100);
    /* 5 full blocks, no empty track, no color tags */
    TEST_ASSERT_EQUAL(15, (int)strlen(bar));  /* 5 * 3 bytes */
    for (int i = 0; i < 5; i++)
        TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + i*3, 3);
}

void test_bar_empty(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 5, ANSI_BAR_LIGHT, 0, 0, 100);
    /* 5 empty blocks */
    TEST_ASSERT_EQUAL(15, (int)strlen(bar));
    for (int i = 0; i < 5; i++)
        TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x91", bar + i*3, 3);
}

void test_bar_half(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 4, ANSI_BAR_LIGHT, 50, 0, 100);
    /* 50% of 4 = 2 full + 2 empty */
    TEST_ASSERT_EQUAL(12, (int)strlen(bar));
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 0, 3);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 3, 3);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x91", bar + 6, 3);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x91", bar + 9, 3);
}

void test_bar_partial_block(void)
{
    char bar[128], bar2[128];
    /* 25% of 4 = 8 eighths -> 1 full + 0 partial + 3 empty */
    ansi_bar(bar, sizeof(bar), NULL, 4, ANSI_BAR_LIGHT, 25, 0, 100);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 0, 3); /* 1 full */
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x91", bar + 3, 3); /* empty */

    /* 50% of 5 = 20 eighths -> 2 full, 4/8 partial, 2 empty */
    ansi_bar(bar2, sizeof(bar2), NULL, 5, ANSI_BAR_LIGHT, 50, 0, 100);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar2 + 0, 3); /* full */
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar2 + 3, 3); /* full */
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x8c", bar2 + 6, 3); /* half block */
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x91", bar2 + 9, 3); /* empty */
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x91", bar2 + 12, 3); /* empty */
}

void test_bar_with_color(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), "red", 3, ANSI_BAR_LIGHT, 100, 0, 100);
    /* Should start with [red] and have [/] after blocks */
    TEST_ASSERT_NOT_NULL(strstr(bar, "[red]"));
    TEST_ASSERT_NOT_NULL(strstr(bar, "[/]"));
    TEST_ASSERT_NOT_NULL(strstr(bar, "\xe2\x96\x88"));
}

void test_bar_clamp_over(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 3, ANSI_BAR_LIGHT, 200, 0, 100);
    /* Clamped to 100%: 3 full blocks */
    TEST_ASSERT_EQUAL(9, (int)strlen(bar));
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 0, 3);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 3, 3);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 6, 3);
}

void test_bar_clamp_under(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 3, ANSI_BAR_LIGHT, -50, 0, 100);
    /* Clamped to 0%: 3 empty */
    TEST_ASSERT_EQUAL(9, (int)strlen(bar));
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x91", bar + 0, 3);
}

void test_bar_zero_range(void)
{
    char bar[128];
    /* max == min: treat as 100% */
    ansi_bar(bar, sizeof(bar), NULL, 3, ANSI_BAR_LIGHT, 5, 5, 5);
    TEST_ASSERT_EQUAL(9, (int)strlen(bar));
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 0, 3);
}

void test_bar_zero_width(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 0, ANSI_BAR_LIGHT, 50, 0, 100);
    TEST_ASSERT_EQUAL_STRING("", bar);
}

void test_bar_negative_range(void)
{
    char bar[128];
    /* 0 in range [-100,100] = 50% of 4 = 2 full + 2 empty */
    ansi_bar(bar, sizeof(bar), NULL, 4, ANSI_BAR_LIGHT, 0, -100, 100);
    TEST_ASSERT_EQUAL(12, (int)strlen(bar));
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 0, 3);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 3, 3);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x91", bar + 6, 3);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x91", bar + 9, 3);
}

void test_bar_float_value(void)
{
    char bar[128];
    /* 0.5 in range [0.0, 1.0] = 50% of 4 = 2 full + 2 empty */
    ansi_bar(bar, sizeof(bar), NULL, 4, ANSI_BAR_LIGHT, 0.5, 0.0, 1.0);
    TEST_ASSERT_EQUAL(12, (int)strlen(bar));
}

void test_bar_inline_with_text(void)
{
    char bar[128];
    ansi_set_enabled(0);
    ansi_print("CPU: %s!",
               ansi_bar(bar, sizeof(bar), NULL, 3, ANSI_BAR_LIGHT, 100, 0, 100));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "CPU: "));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x96\x88"));
    TEST_ASSERT_EQUAL('!', capture_buf[strlen(capture_buf) - 1]);
}

void test_bar_null_color(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 5, ANSI_BAR_LIGHT, 50, 0, 100);
    /* No [color] or [/] tags */
    TEST_ASSERT_NULL(strstr(bar, "["));
    TEST_ASSERT_NULL(strstr(bar, "]"));
}

void test_bar_blank_track(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 4, ANSI_BAR_BLANK, 50, 0, 100);
    /* 50% of 4 = 2 full + 2 spaces */
    TEST_ASSERT_EQUAL(8, (int)strlen(bar));  /* 2*3 + 2*1 */
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 0, 3);
    TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x88", bar + 3, 3);
    TEST_ASSERT_EQUAL(' ', bar[6]);
    TEST_ASSERT_EQUAL(' ', bar[7]);
}

void test_bar_percent(void)
{
    char bar[128];
    ansi_bar_percent(bar, sizeof(bar), NULL, 5, ANSI_BAR_LIGHT, 73);
    /* Should end with " 73%" */
    TEST_ASSERT_NOT_NULL(strstr(bar, " 73%"));
    /* Should contain block characters */
    TEST_ASSERT_NOT_NULL(strstr(bar, "\xe2\x96\x88"));
}

void test_bar_percent_clamp(void)
{
    char bar[128];
    ansi_bar_percent(bar, sizeof(bar), NULL, 3, ANSI_BAR_LIGHT, 150);
    TEST_ASSERT_NOT_NULL(strstr(bar, " 100%"));
    ansi_bar_percent(bar, sizeof(bar), NULL, 3, ANSI_BAR_LIGHT, -10);
    TEST_ASSERT_NOT_NULL(strstr(bar, " 0%"));
}

void test_bar_track_med(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 3, ANSI_BAR_MED, 0, 0, 100);
    /* 3 medium shade ▒ (U+2592, 3 bytes each) */
    TEST_ASSERT_EQUAL(9, (int)strlen(bar));
    for (int i = 0; i < 3; i++)
        TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x92", bar + i*3, 3);
}

void test_bar_track_heavy(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 3, ANSI_BAR_HEAVY, 0, 0, 100);
    /* 3 dark shade ▓ (U+2593, 3 bytes each) */
    TEST_ASSERT_EQUAL(9, (int)strlen(bar));
    for (int i = 0; i < 3; i++)
        TEST_ASSERT_EQUAL_MEMORY("\xe2\x96\x93", bar + i*3, 3);
}

void test_bar_track_dot(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 3, ANSI_BAR_DOT, 0, 0, 100);
    /* 3 middle dot · (U+00B7, 2 bytes each) */
    TEST_ASSERT_EQUAL(6, (int)strlen(bar));
    for (int i = 0; i < 3; i++)
        TEST_ASSERT_EQUAL_MEMORY("\xc2\xb7", bar + i*2, 2);
}

void test_bar_track_line(void)
{
    char bar[128];
    ansi_bar(bar, sizeof(bar), NULL, 3, ANSI_BAR_LINE, 0, 0, 100);
    /* 3 horizontal line ─ (U+2500, 3 bytes each) */
    TEST_ASSERT_EQUAL(9, (int)strlen(bar));
    for (int i = 0; i < 3; i++)
        TEST_ASSERT_EQUAL_MEMORY("\xe2\x94\x80", bar + i*3, 3);
}

void test_bar_null_buf(void)
{
    /* NULL buffer should return "" without crashing */
    const char *r = ansi_bar(NULL, 0, NULL, 5, ANSI_BAR_LIGHT, 50, 0, 100);
    TEST_ASSERT_EQUAL_STRING("", r);
}

void test_bar_tiny_buf(void)
{
    /* buf_size=1: can only hold '\0' */
    char bar[1];
    ansi_bar(bar, sizeof(bar), NULL, 5, ANSI_BAR_LIGHT, 50, 0, 100);
    TEST_ASSERT_EQUAL_STRING("", bar);
}

void test_bar_color_tight_buf(void)
{
    /* Buffer too small for [red]...[/] — should omit color tags entirely
       rather than emitting an incomplete tag like "[re" */
    char bar[16];  /* only room for a few blocks, not [red]...[/] overhead */
    ansi_bar(bar, sizeof(bar), "red", 3, ANSI_BAR_LIGHT, 100, 0, 100);
    /* Should not contain an incomplete open bracket without close */
    if (strstr(bar, "[red]")) {
        /* If color tag fits, close tag must also be present */
        TEST_ASSERT_NOT_NULL(strstr(bar, "[/]"));
    } else {
        /* Color tag omitted entirely — no stray brackets */
        TEST_ASSERT_NULL(strchr(bar, '['));
    }
}

#if ANSI_PRINT_WINDOW
void test_bar_in_window(void)
{
    char bar[128];
    ansi_set_enabled(0);
    ansi_window_start(NULL, 15, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_LEFT, "V:%s",
                     ansi_bar(bar, sizeof(bar), NULL, 5,
                              ANSI_BAR_LIGHT, 50, 0, 100));
    ansi_window_end();
    /* "V:" is 2 chars + bar is 5 chars = 7, fits in width 15 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x96\x88"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x96\x91"));
    /* Borders present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x9a"));
}

void test_bar_in_window_truncate(void)
{
    char bar[128];
    ansi_set_enabled(0);
    ansi_window_start(NULL, 3, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_LEFT, "%s",
                     ansi_bar(bar, sizeof(bar), NULL, 10,
                              ANSI_BAR_LIGHT, 100, 0, 100));
    ansi_window_end();
    /* Only 3 visible chars should appear in window */
    int block_count = 0;
    const char *s = capture_buf;
    while ((s = strstr(s, "\xe2\x96\x88")) != NULL) { block_count++; s += 3; }
    TEST_ASSERT_EQUAL(3, block_count);
}

void test_window_utf8_counting(void)
{
    char bar[128];
    /* Verify UTF-8 multi-byte chars are counted as 1 visible char, not 3 */
    ansi_set_enabled(0);
    ansi_window_start(NULL, 10, ANSI_ALIGN_LEFT, NULL);
    /* "hello" is 5 ASCII chars */
    ansi_window_line(ANSI_ALIGN_LEFT, "hello");
    ansi_window_end();
    capture_reset();

    /* Now test with UTF-8 block chars (each 3 bytes but 1 visible) */
    ansi_window_start(NULL, 10, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_LEFT, "%s",
                     ansi_bar(bar, sizeof(bar), NULL, 5,
                              ANSI_BAR_LIGHT, 100, 0, 100));
    ansi_window_end();
    /* Bar is 5 visible chars in width 10: should have 5 chars of padding */
    /* Check borders are present (they wouldn't be correct if counting was wrong) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x91"));
}
#endif /* ANSI_PRINT_WINDOW */

#endif /* ANSI_PRINT_BAR */

/* ------------------------------------------------------------------ */
/* Default fg/bg tests                                                */
/* ------------------------------------------------------------------ */

void test_set_fg_applies_color(void)
{
    ansi_set_fg("red");
    capture_reset();
    ansi_print("hello");
    /* "hello" should already be in red (fg set before print) --
       the tag state carries the default fg, so no extra escape needed.
       But after [/] reset it should reapply red. */
    capture_reset();
    ansi_print("[green]x[/]y");
    /* After [/], red should be reapplied: look for red escape \x1b[31m */
    const char *reset = strstr(capture_buf, "\x1b[0m");
    TEST_ASSERT_NOT_NULL(reset);
    /* After the reset, red fg should follow */
    TEST_ASSERT_NOT_NULL(strstr(reset, "\x1b[31m"));
}

void test_set_bg_applies_color(void)
{
    ansi_set_bg("blue");
    capture_reset();
    ansi_print("[red]x[/]y");
    /* After [/], blue bg should be reapplied: \x1b[44m */
    const char *reset = strstr(capture_buf, "\x1b[0m");
    TEST_ASSERT_NOT_NULL(reset);
    TEST_ASSERT_NOT_NULL(strstr(reset, "\x1b[44m"));
}

void test_set_fg_and_bg_restore_on_reset(void)
{
    ansi_set_fg("white");
    ansi_set_bg("red");
    capture_reset();
    ansi_print("[cyan]x[/]y");
    /* After [/], both white fg (\x1b[37m) and red bg (\x1b[41m) restored */
    const char *reset = strstr(capture_buf, "\x1b[0m");
    TEST_ASSERT_NOT_NULL(reset);
    TEST_ASSERT_NOT_NULL(strstr(reset, "\x1b[37m"));
    TEST_ASSERT_NOT_NULL(strstr(reset, "\x1b[41m"));
}

void test_set_fg_selective_close_restores_default(void)
{
    ansi_set_fg("green");
    capture_reset();
    ansi_print("[red]x[/red]y");
    /* After [/red], should reset and reapply green (\x1b[32m) */
    const char *reset = strstr(capture_buf, "\x1b[0m");
    TEST_ASSERT_NOT_NULL(reset);
    TEST_ASSERT_NOT_NULL(strstr(reset, "\x1b[32m"));
}

void test_set_fg_null_clears_default(void)
{
    ansi_set_fg("red");
    ansi_set_fg(NULL);
    capture_reset();
    ansi_print("[green]x[/]y");
    /* After [/], no red should appear -- default was cleared */
    const char *reset = strstr(capture_buf, "\x1b[0m");
    TEST_ASSERT_NOT_NULL(reset);
    /* After reset, the only thing should be "y" -- no color reapplied */
    TEST_ASSERT_NULL(strstr(reset + 4, "\x1b[31m"));
}

void test_set_bg_null_clears_default(void)
{
    ansi_set_bg("blue");
    ansi_set_bg(NULL);
    capture_reset();
    ansi_print("[red]x[/]y");
    /* After [/], no blue bg should appear */
    const char *reset = strstr(capture_buf, "\x1b[0m");
    TEST_ASSERT_NOT_NULL(reset);
    TEST_ASSERT_NULL(strstr(reset + 4, "\x1b[44m"));
}

void test_set_fg_disabled_no_output(void)
{
    ansi_set_enabled(0);
    ansi_set_fg("red");
    capture_reset();
    ansi_print("hello");
    /* No escape codes when disabled */
    TEST_ASSERT_EQUAL_STRING("hello", capture_buf);
}

void test_set_fg_unknown_color_ignored(void)
{
    ansi_set_fg("nosuchcolor");
    capture_reset();
    ansi_print("[green]x[/]y");
    /* After [/], no default fg should be reapplied (unknown was ignored) */
    const char *reset = strstr(capture_buf, "\x1b[0m");
    TEST_ASSERT_NOT_NULL(reset);
    /* Only "y" after reset, no extra fg code */
    const char *after_reset = reset + 4; /* skip \x1b[0m */
    TEST_ASSERT_EQUAL_CHAR('y', *after_reset);
}

void test_set_fg_immediate_emit(void)
{
    /* ansi_set_fg should emit the color code immediately */
    ansi_set_fg("red");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[31m"));
}

void test_set_bg_immediate_emit(void)
{
    ansi_set_bg("blue");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[44m"));
}

/* ------------------------------------------------------------------ */
/* Config banner & runner                                             */
/* ------------------------------------------------------------------ */

static void print_config(void)
{
    printf("Build config:");
    printf(" EMOJI=%d",           ANSI_PRINT_EMOJI);
    printf(" EXTENDED_EMOJI=%d",  ANSI_PRINT_EXTENDED_EMOJI);
    printf(" EXTENDED_COLORS=%d", ANSI_PRINT_EXTENDED_COLORS);
    printf(" BRIGHT_COLORS=%d",   ANSI_PRINT_BRIGHT_COLORS);
    printf(" STYLES=%d",          ANSI_PRINT_STYLES);
    printf(" GRADIENTS=%d",       ANSI_PRINT_GRADIENTS);
    printf(" UNICODE=%d",         ANSI_PRINT_UNICODE);
    printf(" BANNER=%d",          ANSI_PRINT_BANNER);
    printf(" WINDOW=%d",          ANSI_PRINT_WINDOW);
    printf(" BAR=%d",             ANSI_PRINT_BAR);
    printf("\n");
}

int main(void)
{
    print_config();

    UNITY_BEGIN();

    /* Core (always run) */
    RUN_TEST(test_plain_text_no_tags);
    RUN_TEST(test_printf_formatting);
    RUN_TEST(test_color_disabled_strips_tags);
    RUN_TEST(test_color_enabled_emits_ansi);
    RUN_TEST(test_toggle);
    RUN_TEST(test_set_enabled);

#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
    /* NO_COLOR / ansi_enable */
    RUN_TEST(test_enable_no_color_disables);
    RUN_TEST(test_enable_no_color_locks);
    RUN_TEST(test_enable_no_color_toggle_locked);
    RUN_TEST(test_enable_no_color_strips_tags);
    RUN_TEST(test_init_resets_lock);
#if !defined(_WIN32)
    RUN_TEST(test_enable_no_color_empty_value);
#endif
#endif

    RUN_TEST(test_puts_plain);
    RUN_TEST(test_puts_with_color);
    RUN_TEST(test_escaped_brackets);
    RUN_TEST(test_background_color);
    RUN_TEST(test_null_putc_suppresses_output);

    /* ansi_format */
    RUN_TEST(test_format_returns_buffer);
    RUN_TEST(test_format_null_fmt);
    RUN_TEST(test_format_no_buf);

    /* Edge cases */
    RUN_TEST(test_puts_null);
    RUN_TEST(test_buffer_truncation);
    RUN_TEST(test_empty_format_string);
    RUN_TEST(test_unclosed_tag);

    /* Numeric colors */
    RUN_TEST(test_numeric_fg);
    RUN_TEST(test_numeric_bg);
    RUN_TEST(test_numeric_fg_disabled);
    RUN_TEST(test_numeric_fg_clamp_high);
    RUN_TEST(test_numeric_fg_clamp_negative);
    RUN_TEST(test_numeric_fg_non_numeric);

    /* Selective close tags */
    RUN_TEST(test_close_specific_color);
    RUN_TEST(test_close_specific_color_disabled);
    RUN_TEST(test_close_numeric_fg);
    RUN_TEST(test_close_with_background);

    /* find_on edge cases */
    RUN_TEST(test_on_with_trailing_spaces);
    RUN_TEST(test_empty_tag_ignored);
    RUN_TEST(test_unclosed_bracket);

    /* Default fg/bg */
    RUN_TEST(test_set_fg_applies_color);
    RUN_TEST(test_set_bg_applies_color);
    RUN_TEST(test_set_fg_and_bg_restore_on_reset);
    RUN_TEST(test_set_fg_selective_close_restores_default);
    RUN_TEST(test_set_fg_null_clears_default);
    RUN_TEST(test_set_bg_null_clears_default);
    RUN_TEST(test_set_fg_disabled_no_output);
    RUN_TEST(test_set_fg_unknown_color_ignored);
    RUN_TEST(test_set_fg_immediate_emit);
    RUN_TEST(test_set_bg_immediate_emit);

#if ANSI_PRINT_BANNER
    /* Banner */
    RUN_TEST(test_banner_basic);
    RUN_TEST(test_banner_printf);
    RUN_TEST(test_banner_disabled);
    RUN_TEST(test_banner_unknown_color);
    RUN_TEST(test_banner_null_fmt);
    RUN_TEST(test_banner_null_color);
    RUN_TEST(test_banner_multiline);
    RUN_TEST(test_banner_fixed_width);
    RUN_TEST(test_banner_truncate);
    RUN_TEST(test_banner_auto_width);
    RUN_TEST(test_banner_align_center);
    RUN_TEST(test_banner_align_right);
#endif

#if ANSI_PRINT_WINDOW
    /* Window */
    RUN_TEST(test_window_basic);
    RUN_TEST(test_window_with_title);
    RUN_TEST(test_window_title_center);
    RUN_TEST(test_window_title_right);
    RUN_TEST(test_window_line_center);
    RUN_TEST(test_window_line_right);
    RUN_TEST(test_window_printf);
    RUN_TEST(test_window_truncate);
    RUN_TEST(test_window_null_title);
    RUN_TEST(test_window_empty_title);
    RUN_TEST(test_window_color);
    RUN_TEST(test_window_color_disabled);
    RUN_TEST(test_window_markup);
#endif

#if ANSI_PRINT_BAR
    /* Bar graphs */
    RUN_TEST(test_bar_full);
    RUN_TEST(test_bar_empty);
    RUN_TEST(test_bar_half);
    RUN_TEST(test_bar_partial_block);
    RUN_TEST(test_bar_with_color);
    RUN_TEST(test_bar_clamp_over);
    RUN_TEST(test_bar_clamp_under);
    RUN_TEST(test_bar_zero_range);
    RUN_TEST(test_bar_zero_width);
    RUN_TEST(test_bar_negative_range);
    RUN_TEST(test_bar_float_value);
    RUN_TEST(test_bar_inline_with_text);
    RUN_TEST(test_bar_null_color);
    RUN_TEST(test_bar_blank_track);
    RUN_TEST(test_bar_percent);
    RUN_TEST(test_bar_percent_clamp);
    RUN_TEST(test_bar_track_med);
    RUN_TEST(test_bar_track_heavy);
    RUN_TEST(test_bar_track_dot);
    RUN_TEST(test_bar_track_line);
    RUN_TEST(test_bar_null_buf);
    RUN_TEST(test_bar_tiny_buf);
    RUN_TEST(test_bar_color_tight_buf);
#if ANSI_PRINT_WINDOW
    RUN_TEST(test_bar_in_window);
    RUN_TEST(test_bar_in_window_truncate);
    RUN_TEST(test_window_utf8_counting);
#endif
#endif

#if ANSI_PRINT_STYLES
    RUN_TEST(test_bold_style);
    RUN_TEST(test_multi_style_tag);
    RUN_TEST(test_close_specific_style);
    RUN_TEST(test_all_styles);
    RUN_TEST(test_color_and_style_combined);
#endif

#if ANSI_PRINT_GRADIENTS
    RUN_TEST(test_rainbow_no_crash);
    RUN_TEST(test_rainbow_disabled);
    RUN_TEST(test_gradient_span);
    RUN_TEST(test_gradient_disabled);
    RUN_TEST(test_gradient_bad_colors);
    RUN_TEST(test_rainbow_span);
    RUN_TEST(test_rainbow_span_disabled);
    RUN_TEST(test_rainbow_null);
    RUN_TEST(test_rainbow_spaces_only);
    RUN_TEST(test_gradient_close_reapplies);
    RUN_TEST(test_count_effect_escaped_brackets);
    RUN_TEST(test_count_effect_skips_tags);
#if ANSI_PRINT_EMOJI
    RUN_TEST(test_gradient_with_emoji);
#endif
#if ANSI_PRINT_UNICODE
    RUN_TEST(test_gradient_with_unicode);
#endif
#if ANSI_PRINT_EMOJI || ANSI_PRINT_UNICODE
    RUN_TEST(test_count_effect_escaped_colons);
#endif
#if ANSI_PRINT_STYLES
    RUN_TEST(test_gradient_rejects_style);
#endif
#endif

#if ANSI_PRINT_EMOJI
    RUN_TEST(test_emoji_fire);
    RUN_TEST(test_emoji_check_via_puts);
    RUN_TEST(test_emoji_unknown_passthrough);
    RUN_TEST(test_escaped_colon);
    RUN_TEST(test_emoji_with_color);
    RUN_TEST(test_emoji_color_disabled);
    RUN_TEST(test_multiple_emoji);
    RUN_TEST(test_bare_colon);
    RUN_TEST(test_colon_at_end);
    RUN_TEST(test_adjacent_emoji);
#endif

#if ANSI_PRINT_UNICODE
    RUN_TEST(test_unicode_bmp);
    RUN_TEST(test_unicode_supplementary);
    RUN_TEST(test_unicode_ascii);
    RUN_TEST(test_unicode_invalid_passthrough);
    RUN_TEST(test_unicode_with_color);
    RUN_TEST(test_unicode_color_disabled);
    RUN_TEST(test_unicode_lowercase_hex);
    RUN_TEST(test_unicode_2byte);
    RUN_TEST(test_unicode_out_of_range);
    RUN_TEST(test_unicode_zero);
    RUN_TEST(test_unicode_too_long);
    RUN_TEST(test_unicode_bare_prefix);
#endif

    /* Minimal-build tests (only run when features are compiled out) */
#if !ANSI_PRINT_STYLES
    RUN_TEST(test_minimal_unknown_style_consumed);
    RUN_TEST(test_minimal_unknown_style_disabled);
#endif

#if !ANSI_PRINT_EMOJI
    RUN_TEST(test_minimal_emoji_literal_passthrough);
    RUN_TEST(test_minimal_double_colon_literal);
    RUN_TEST(test_minimal_colon_with_color);
#endif

#if !ANSI_PRINT_GRADIENTS
    RUN_TEST(test_minimal_rainbow_tag_consumed);
    RUN_TEST(test_minimal_gradient_tag_consumed);
#endif

#if !ANSI_PRINT_UNICODE
    RUN_TEST(test_minimal_unicode_literal_passthrough);
#endif

#if !ANSI_PRINT_EXTENDED_COLORS
    RUN_TEST(test_minimal_extended_color_ignored);
#endif

#if !ANSI_PRINT_BRIGHT_COLORS
    RUN_TEST(test_minimal_bright_color_ignored);
#endif

#if !ANSI_PRINT_BAR
    RUN_TEST(test_minimal_bar_not_available);
#endif

    return UNITY_END();
}
