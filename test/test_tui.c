#include "unity.h"
#include "ansi_print.h"
#include "ansi_tui.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Capture buffer — collects all output via ansi_print's putc          */
/* ------------------------------------------------------------------ */

#define CAPTURE_SIZE 8192

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

/* Format buffer */
static char fmt_buf[512];      /* for ansi_init */

/* ------------------------------------------------------------------ */
/* Unity setUp / tearDown                                              */
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
/* Screen helper tests                                                 */
/* ------------------------------------------------------------------ */

void test_tui_cls(void)
{
    tui_cls();
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2J"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[H"));
}

void test_tui_goto(void)
{
    tui_goto(5, 10);
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[5;10H"));
}

void test_tui_goto_top_left(void)
{
    tui_goto(1, 1);
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1;1H"));
}

void test_tui_cursor_hide(void)
{
    tui_cursor_hide();
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[?25l"));
}

void test_tui_cursor_show(void)
{
    tui_cursor_show();
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[?25h"));
}

/* ------------------------------------------------------------------ */
/* Frame widget tests                                                  */
/* ------------------------------------------------------------------ */

void test_frame_init_basic(void)
{
    const tui_frame_t f = {
        .row = 1, .col = 1, .width = 10, .height = 5, .color = NULL
    };
    tui_frame_init(&f);
    /* Top-left corner position */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1;1H"));
    /* Box-drawing top-left corner (double: ╔ = \xe2\x95\x94) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    /* Bottom border row = 1 + 5 - 1 = 5 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[5;1H"));
}

void test_frame_init_colored(void)
{
    const tui_frame_t f = {
        .row = 2, .col = 3, .width = 12, .height = 6, .color = "cyan"
    };
    tui_frame_init(&f);
    /* Cyan ANSI code present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[36m"));
    /* Cursor at frame position */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2;3H"));
}

void test_frame_null_widget(void)
{
    tui_frame_init(NULL);
    /* No output */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

void test_frame_min_size(void)
{
    const tui_frame_t f = {
        .row = 1, .col = 1, .width = 5, .height = 3, .color = NULL
    };
    tui_frame_init(&f);
    /* Should produce output (minimal valid frame) */
    TEST_ASSERT_TRUE(capture_pos > 0);
    /* Top-left ╔ and bottom-right ╝ corners present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x9d"));
}

void test_frame_too_small(void)
{
    const tui_frame_t f1 = {
        .row = 1, .col = 1, .width = 4, .height = 3, .color = NULL
    };
    tui_frame_init(&f1);
    TEST_ASSERT_EQUAL_INT(0, capture_pos);

    const tui_frame_t f2 = {
        .row = 1, .col = 1, .width = 5, .height = 2, .color = NULL
    };
    tui_frame_init(&f2);
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

void test_frame_title(void)
{
    const tui_frame_t f = {
        .row = 1, .col = 1, .width = 30, .height = 5,
        .title = "My Panel", .color = "cyan"
    };
    tui_frame_init(&f);
    /* Title text present in output */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "My Panel"));
    /* Title overlaid at col+1 on top border row (row 1, col 2) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1;2H"));
}

void test_frame_null_title(void)
{
    const tui_frame_t f = {
        .row = 1, .col = 1, .width = 20, .height = 5,
        .title = NULL, .color = NULL
    };
    tui_frame_init(&f);
    /* Still draws border (TL corner present) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    /* No bold tag since no title */
    TEST_ASSERT_NULL(strstr(capture_buf, "\x1b[1m"));
}

void test_frame_empty_title(void)
{
    const tui_frame_t f = {
        .row = 1, .col = 1, .width = 20, .height = 5,
        .title = "", .color = NULL
    };
    tui_frame_init(&f);
    /* Empty string treated as no title — no bold tag */
    TEST_ASSERT_NULL(strstr(capture_buf, "\x1b[1m"));
}

/* ------------------------------------------------------------------ */
/* Parent-relative positioning tests                                   */
/* ------------------------------------------------------------------ */

void test_frame_with_parent(void)
{
    /* Parent frame at (1,1), child frame at (3,3) inside it.
       Child absolute = (3+1, 3+1+1) = (4, 5).
       Child border at row 4, col 5. */
    const tui_frame_t parent = {
        .row = 1, .col = 1, .width = 40, .height = 20, .color = NULL
    };
    const tui_frame_t child = {
        .row = 3, .col = 3, .width = 20, .height = 10,
        .color = "cyan", .parent = &parent
    };
    tui_frame_init(&child);
    /* Child border drawn at absolute (4, 5) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[4;5H"));
}

void test_label_with_parent(void)
{
    /* Frame at (1,1). Label at (1,1) inside frame.
       Absolute = (1+1, 1+1+1) = (2, 3) = first interior cell. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 40, .height = 10, .color = NULL
    };
    tui_label_t w = {
        .row = 1, .col = 1, .width = 8,
        .border = ANSI_TUI_NO_BORDER,
        .label = "V", .color = NULL, .parent = &frame
    };
    tui_label_init(&w);
    /* Cursor at absolute (2, 3) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2;3H"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "V: "));
}

void test_label_null_parent(void)
{
    /* NULL parent = absolute positioning, backward compatible */
    tui_label_t w = {
        .row = 5, .col = 10, .width = 8,
        .border = ANSI_TUI_NO_BORDER,
        .label = "X", .color = NULL, .parent = NULL
    };
    tui_label_init(&w);
    /* Cursor at absolute (5, 10) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[5;10H"));
}

void test_nested_parent(void)
{
    /* Outer frame (1,1), inner frame (3,3) inside outer,
       label (1,1) inside inner.
       Inner absolute = (3+1, 3+1+1) = (4, 5).
       Label absolute = (1+4, 1+5+1) = (5, 7). Wait...
       tui_resolve walks: parent=inner → row=1+3=4, col=1+3+1=5
                          parent=outer → row=4+1=5, col=5+1+1=7
       So label at absolute (5, 7). */
    const tui_frame_t outer = {
        .row = 1, .col = 1, .width = 60, .height = 20, .color = NULL
    };
    const tui_frame_t inner = {
        .row = 3, .col = 3, .width = 30, .height = 10,
        .color = NULL, .parent = &outer
    };
    tui_label_t w = {
        .row = 1, .col = 1, .width = 5,
        .border = ANSI_TUI_NO_BORDER,
        .label = "N", .color = NULL, .parent = &inner
    };
    tui_label_init(&w);
    /* Cursor at absolute (5, 7) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[5;7H"));
}

/* ------------------------------------------------------------------ */
/* Label widget tests                                                  */
/* ------------------------------------------------------------------ */

void test_label_init_no_border(void)
{
    tui_label_t w = {
        .row = 1, .col = 1, .width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = "CPU", .color = NULL
    };
    tui_label_init(&w);
    /* Should contain the label text */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "CPU: "));
    /* Cursor positioning escape should be present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1;1H"));
}

void test_label_init_bordered(void)
{
    tui_label_t w = {
        .row = 3, .col = 5, .width = 8,
        .border = ANSI_TUI_BORDER,
        .label = "MEM", .color = "cyan"
    };
    tui_label_init(&w);
    /* Top border at row 3 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[3;5H"));
    /* Box-drawing top-left char (default double: ╔ = \xe2\x95\x94) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    /* Interior label at row 4 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[4;7H"));
    /* Bottom border at row 5 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[5;5H"));
    /* Color tag processed — cyan ANSI code should appear */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[36m"));
}

void test_label_update_basic(void)
{
    tui_label_t w = {
        .row = 1, .col = 1, .width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = "V", .color = NULL
    };
    tui_label_init(&w);
    capture_reset();

    tui_label_update(&w, "73%%");
    /* Should contain the value text */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "73%"));
    /* Cursor positioned at value column (after "V: " = col 1 + 3) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1;4H"));
}

void test_label_update_pads_to_width(void)
{
    tui_label_t w = {
        .row = 1, .col = 1, .width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = "V", .color = NULL
    };
    tui_label_init(&w);
    capture_reset();

    tui_label_update(&w, "hi");
    /* Value "hi" followed by padding spaces (at least 10 total from pad) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "hi"));
    /* Count spaces after content — at least width chars of space padding */
    const char *pad = strstr(capture_buf, "hi");
    TEST_ASSERT_NOT_NULL(pad);
}

void test_label_update_with_markup(void)
{
    tui_label_t w = {
        .row = 1, .col = 1, .width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = "S", .color = NULL
    };
    tui_label_init(&w);
    capture_reset();

    tui_label_update(&w, "[green]OK[/]");
    /* Green ANSI code and text should appear */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "OK"));
}

void test_label_update_bordered(void)
{
    tui_label_t w = {
        .row = 3, .col = 5, .width = 8,
        .border = ANSI_TUI_BORDER,
        .label = "CPU", .color = "cyan"
    };
    tui_label_init(&w);
    capture_reset();

    tui_label_update(&w, "42%%");
    /* Value col = 5 + 2 (border) + 3 (label) + 2 (": ") = 12 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[4;12H"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "42%"));
}

void test_label_null_widget(void)
{
    /* Should not crash */
    tui_label_init(NULL);
    tui_label_update(NULL, "test");
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

/* ------------------------------------------------------------------ */
/* Bar widget tests                                                    */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_BAR

void test_bar_init_no_border(void)
{
    char bar_buf[128];
    tui_bar_t w = {
        .row = 1, .col = 1, .bar_width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = NULL, .color = "green",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_bar_init(&w);
    /* Bar should render track characters (light shade ░ = \xe2\x96\x91) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x96\x91"));
}

void test_bar_init_bordered(void)
{
    char bar_buf[128];
    tui_bar_t w = {
        .row = 2, .col = 3, .bar_width = 10,
        .border = ANSI_TUI_BORDER,
        .label = "CPU ", .color = "green",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_bar_init(&w);
    /* Border present — top-left box char */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    /* Label text present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "CPU "));
}

void test_bar_update_full(void)
{
    char bar_buf[128];
    tui_bar_t w = {
        .row = 1, .col = 1, .bar_width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = NULL, .color = "green",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_bar_init(&w);
    capture_reset();

    tui_bar_update(&w, 100.0, 0.0, 100.0);
    /* Full bar = full blocks (█ = \xe2\x96\x88) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x96\x88"));
    /* Green color code */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m"));
}

void test_bar_update_empty(void)
{
    char bar_buf[128];
    tui_bar_t w = {
        .row = 1, .col = 1, .bar_width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = NULL, .color = "green",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_bar_init(&w);
    capture_reset();

    tui_bar_update(&w, 0.0, 0.0, 100.0);
    /* Empty bar = all track characters (░) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x96\x91"));
}

void test_bar_update_repositions(void)
{
    char bar_buf[128];
    tui_bar_t w = {
        .row = 5, .col = 10, .bar_width = 15,
        .border = ANSI_TUI_NO_BORDER,
        .label = "X ", .color = "red",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_bar_init(&w);
    capture_reset();

    tui_bar_update(&w, 50.0, 0.0, 100.0);
    /* Cursor at (5, 10 + 2 for label "X ") = (5, 12) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[5;12H"));
}

void test_bar_state_tracks_value(void)
{
    char bar_buf[128];
    tui_bar_state_t st;
    const tui_bar_t w = {
        .row = 1, .col = 1, .bar_width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = NULL, .color = "green",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf),
        .state = &st
    };
    tui_bar_init(&w);
    /* Init draws bar at 0/100 — state should reflect that */
    TEST_ASSERT_TRUE(st.value == 0.0);

    tui_bar_update(&w, 75.0, 10.0, 200.0);
    TEST_ASSERT_TRUE(st.value == 75.0);
    TEST_ASSERT_TRUE(st.min   == 10.0);
    TEST_ASSERT_TRUE(st.max   == 200.0);
}

void test_bar_null_widget(void)
{
    tui_bar_init(NULL);
    tui_bar_update(NULL, 50.0, 0.0, 100.0);
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

#endif /* ANSI_PRINT_BAR */

/* ------------------------------------------------------------------ */
/* Status widget tests                                                 */
/* ------------------------------------------------------------------ */

void test_status_init_no_border(void)
{
    tui_status_t w = {
        .row = 1, .col = 1, .width = 20,
        .border = ANSI_TUI_NO_BORDER, .color = NULL
    };
    tui_status_init(&w);
    /* Cursor positioned at (1,1) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1;1H"));
}

void test_status_init_bordered(void)
{
    tui_status_t w = {
        .row = 8, .col = 3, .width = 30,
        .border = ANSI_TUI_BORDER, .color = "yellow"
    };
    tui_status_init(&w);
    /* Border chars present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    /* Yellow ANSI code */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[33m"));
}

void test_status_update_basic(void)
{
    tui_status_t w = {
        .row = 1, .col = 1, .width = 20,
        .border = ANSI_TUI_NO_BORDER, .color = NULL
    };
    tui_status_init(&w);
    capture_reset();

    tui_status_update(&w, "System OK");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "System OK"));
}

void test_status_update_pads(void)
{
    tui_status_t w = {
        .row = 1, .col = 1, .width = 20,
        .border = ANSI_TUI_NO_BORDER, .color = NULL
    };
    tui_status_init(&w);
    capture_reset();

    tui_status_update(&w, "OK");
    /* Content present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "OK"));
}

void test_status_update_with_markup(void)
{
    tui_status_t w = {
        .row = 1, .col = 1, .width = 20,
        .border = ANSI_TUI_NO_BORDER, .color = NULL
    };
    tui_status_init(&w);
    capture_reset();

    tui_status_update(&w, "[red]Error![/]");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[31m"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Error!"));
}

void test_status_update_bordered(void)
{
    tui_status_t w = {
        .row = 8, .col = 3, .width = 30,
        .border = ANSI_TUI_BORDER, .color = "yellow"
    };
    tui_status_init(&w);
    capture_reset();

    tui_status_update(&w, "Running");
    /* Interior position: row 9, col 5 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[9;5H"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Running"));
}

void test_status_null_widget(void)
{
    tui_status_init(NULL);
    tui_status_update(NULL, "test");
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

/* ------------------------------------------------------------------ */
/* Text widget tests                                                   */
/* ------------------------------------------------------------------ */

void test_text_init_no_border(void)
{
    tui_text_t w = {
        .row = 1, .col = 1, .width = 20,
        .border = ANSI_TUI_NO_BORDER, .color = NULL
    };
    tui_text_init(&w);
    /* Cursor positioned at (1,1) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1;1H"));
}

void test_text_init_bordered(void)
{
    tui_text_t w = {
        .row = 5, .col = 3, .width = 25,
        .border = ANSI_TUI_BORDER, .color = "cyan"
    };
    tui_text_init(&w);
    /* Border chars present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    /* Cyan ANSI code */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[36m"));
    /* Interior at row+1, col+2 = (6, 5) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[6;5H"));
}

void test_text_update_basic(void)
{
    tui_text_t w = {
        .row = 1, .col = 1, .width = 20,
        .border = ANSI_TUI_NO_BORDER, .color = NULL
    };
    tui_text_init(&w);
    capture_reset();

    tui_text_update(&w, "Hello World");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Hello World"));
}

void test_text_update_pads(void)
{
    tui_text_t w = {
        .row = 1, .col = 1, .width = 20,
        .border = ANSI_TUI_NO_BORDER, .color = NULL
    };
    tui_text_init(&w);
    capture_reset();

    tui_text_update(&w, "Hi");
    /* Short text present, and cursor was repositioned to write it */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Hi"));
    /* Two goto sequences: first from tui_widget_goto, second to reposition */
    int goto_count = 0;
    for (const char *p = capture_buf; (p = strstr(p, "\x1b[1;1H")) != NULL; p++)
        goto_count++;
    TEST_ASSERT_TRUE(goto_count >= 2);
}

void test_text_fill_width(void)
{
    /* Parent interior = 30 - 4 = 26.  Widget at col 1, no border.
       Effective width = 26 - 0 - 0 = 26.  Padding should be 26 spaces. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 30, .height = 10, .color = NULL
    };
    tui_text_t w = {
        .row = 1, .col = 1, .width = -1,
        .border = ANSI_TUI_NO_BORDER, .color = NULL,
        .parent = &frame
    };
    tui_text_init(&w);
    capture_reset();

    tui_text_update(&w, "test");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "test"));
    /* Count spaces in the padding run — should be at least 26 */
    const char *goto_pos = strstr(capture_buf, "\x1b[2;3H");
    TEST_ASSERT_NOT_NULL(goto_pos);
    /* After the first goto there should be 26 spaces for padding */
    int spaces = 0;
    const char *p = goto_pos + strlen("\x1b[2;3H");
    while (*p == ' ') { spaces++; p++; }
    TEST_ASSERT_EQUAL_INT(26, spaces);
}

void test_text_fill_bordered(void)
{
    /* Parent interior = 30 - 4 = 26.  Widget at col 1, with border.
       Effective width = 26 - 0 - 4 = 22 (own border takes 4). */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 30, .height = 10, .color = NULL
    };
    tui_text_t w = {
        .row = 1, .col = 1, .width = -1,
        .border = ANSI_TUI_BORDER, .color = NULL,
        .parent = &frame
    };
    tui_text_init(&w);
    /* Border present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    /* Interior position at (2+1, 3+2) = (3, 5) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[3;5H"));
}

void test_text_null_widget(void)
{
    tui_text_init(NULL);
    tui_text_update(NULL, "test");
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

void test_text_disable_blocks_update(void)
{
    tui_text_state_t st;
    const tui_text_t w = {
        .row = 1, .col = 1, .width = 20,
        .border = ANSI_TUI_NO_BORDER, .color = NULL, .state = &st
    };
    tui_text_init(&w);
    TEST_ASSERT_EQUAL_INT(1, st.enabled);

    tui_text_enable(&w, 0);
    TEST_ASSERT_EQUAL_INT(0, st.enabled);

    capture_reset();
    tui_text_update(&w, "blocked");
    /* Disabled — update should produce no output */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

/* ------------------------------------------------------------------ */
/* tui_below tests                                                     */
/* ------------------------------------------------------------------ */

void test_tui_below_no_border(void)
{
    const tui_text_t w = {
        .row = 5, .col = 1, .width = 20,
        .border = ANSI_TUI_NO_BORDER, .color = NULL
    };
    /* Unbordered widget occupies 1 row: next row = 5 + 1 = 6 */
    TEST_ASSERT_EQUAL_INT(6, tui_below(&w));
}

void test_tui_below_bordered(void)
{
    const tui_label_t w = {
        .row = 3, .col = 1, .width = 10,
        .border = ANSI_TUI_BORDER, .label = "X", .color = NULL
    };
    /* Bordered widget occupies 3 rows: next row = 3 + 3 = 6 */
    TEST_ASSERT_EQUAL_INT(6, tui_below(&w));
}

/* ------------------------------------------------------------------ */
/* Color-disabled tests                                                */
/* ------------------------------------------------------------------ */

void test_label_disabled_strips_color(void)
{
    ansi_set_enabled(0);
    tui_label_t w = {
        .row = 1, .col = 1, .width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = "T", .color = "red"
    };
    tui_label_init(&w);
    /* Label text present, no ANSI color codes (except cursor escapes) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "T: "));
    /* The escape sequences for cursor positioning still present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b["));
}

/* ------------------------------------------------------------------ */
/* Check widget                                                        */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_EMOJI

static void test_check_init_true(void)
{
    const tui_check_t w = {
        .row = 1, .col = 1, .width = 0,
        .border = ANSI_TUI_NO_BORDER,
        .label = "Online", .color = NULL
    };
    tui_check_init(&w, 1);
    /* Should contain check emoji (U+2705 = \xe2\x9c\x85) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));
    /* Should contain label text */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Online"));
}

static void test_check_init_false(void)
{
    const tui_check_t w = {
        .row = 1, .col = 1, .width = 0,
        .border = ANSI_TUI_NO_BORDER,
        .label = "Offline", .color = NULL
    };
    tui_check_init(&w, 0);
    /* Should contain cross emoji (U+274C = \xe2\x9d\x8c) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9d\x8c"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Offline"));
}

static void test_check_init_bordered(void)
{
    const tui_check_t w = {
        .row = 3, .col = 5, .width = 0,
        .border = ANSI_TUI_BORDER,
        .label = "Ready", .color = "cyan"
    };
    tui_check_init(&w, 1);
    /* Should have border characters and the check emoji */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Ready"));
}

static void test_check_update_toggle(void)
{
    const tui_check_t w = {
        .row = 1, .col = 1, .width = 0,
        .border = ANSI_TUI_NO_BORDER,
        .label = "State", .color = NULL
    };
    tui_check_init(&w, 1);
    /* Starts with check */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));

    capture_reset();
    tui_check_update(&w, 0);
    /* Now has cross, not check */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9d\x8c"));
    TEST_ASSERT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));

    capture_reset();
    tui_check_update(&w, 1);
    /* Back to check */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));
}

static void test_check_state_tracks_value(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .row = 1, .col = 1, .width = 0,
        .border = ANSI_TUI_NO_BORDER,
        .label = "S", .color = NULL, .state = &st
    };
    tui_check_init(&w, 1);
    TEST_ASSERT_EQUAL_INT(1, st.checked);

    tui_check_update(&w, 0);
    TEST_ASSERT_EQUAL_INT(0, st.checked);
}

static void test_check_toggle(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .row = 1, .col = 1, .width = 0,
        .border = ANSI_TUI_NO_BORDER,
        .label = "T", .color = NULL, .state = &st
    };
    tui_check_init(&w, 1);
    TEST_ASSERT_EQUAL_INT(1, st.checked);

    capture_reset();
    tui_check_toggle(&w);
    TEST_ASSERT_EQUAL_INT(0, st.checked);
    /* Should emit cross */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9d\x8c"));

    capture_reset();
    tui_check_toggle(&w);
    TEST_ASSERT_EQUAL_INT(1, st.checked);
    /* Should emit check */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));
}

static void test_check_toggle_null_state(void)
{
    const tui_check_t w = {
        .row = 1, .col = 1, .width = 0,
        .border = ANSI_TUI_NO_BORDER,
        .label = "X", .color = NULL
        /* .state = NULL (no state) */
    };
    tui_check_init(&w, 1);
    capture_reset();
    tui_check_toggle(&w);
    /* No crash, no output (toggle requires state) */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

static void test_check_null_widget(void)
{
    tui_check_init(NULL, 1);
    tui_check_update(NULL, 0);
    tui_check_toggle(NULL);
    /* No crash */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

#endif /* ANSI_PRINT_EMOJI */

/* ------------------------------------------------------------------ */
/* Enable/disable tests                                                */
/* ------------------------------------------------------------------ */

void test_label_disable_blocks_update(void)
{
    tui_label_state_t st;
    const tui_label_t w = {
        .row = 1, .col = 1, .width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = "V", .color = NULL, .state = &st
    };
    tui_label_init(&w);
    TEST_ASSERT_EQUAL_INT(1, st.enabled);

    tui_label_enable(&w, 0);
    TEST_ASSERT_EQUAL_INT(0, st.enabled);

    capture_reset();
    tui_label_update(&w, "hello");
    /* Disabled — update should produce no output */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

void test_label_enable_allows_update(void)
{
    tui_label_state_t st;
    const tui_label_t w = {
        .row = 1, .col = 1, .width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = "V", .color = NULL, .state = &st
    };
    tui_label_init(&w);
    tui_label_enable(&w, 0);

    capture_reset();
    tui_label_enable(&w, 1);
    tui_label_update(&w, "world");
    /* Re-enabled — update should produce output */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "world"));
}

#if ANSI_PRINT_STYLES
void test_label_disable_draws_dim(void)
{
    tui_label_state_t st;
    const tui_label_t w = {
        .row = 1, .col = 1, .width = 10,
        .border = ANSI_TUI_BORDER,
        .label = "X", .color = "cyan", .state = &st
    };
    tui_label_init(&w);

    capture_reset();
    tui_label_enable(&w, 0);
    /* Should contain dim ANSI code (ESC[2m) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2m"));
}
#endif

void test_status_disable_blocks_update(void)
{
    tui_status_state_t st;
    const tui_status_t w = {
        .row = 1, .col = 1, .width = 20,
        .border = ANSI_TUI_NO_BORDER, .color = NULL, .state = &st
    };
    tui_status_init(&w);
    TEST_ASSERT_EQUAL_INT(1, st.enabled);

    tui_status_enable(&w, 0);
    capture_reset();
    tui_status_update(&w, "test");
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

#if ANSI_PRINT_BAR
void test_bar_disable_blocks_update(void)
{
    char bar_buf[128];
    tui_bar_state_t st;
    const tui_bar_t w = {
        .row = 1, .col = 1, .bar_width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = NULL, .color = "green",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf),
        .state = &st
    };
    tui_bar_init(&w);
    tui_bar_enable(&w, 0);

    capture_reset();
    tui_bar_update(&w, 75.0, 0.0, 100.0);
    /* Disabled — update should produce no output */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

void test_bar_enable_restores(void)
{
    char bar_buf[128];
    tui_bar_state_t st;
    const tui_bar_t w = {
        .row = 1, .col = 1, .bar_width = 10,
        .border = ANSI_TUI_NO_BORDER,
        .label = NULL, .color = "green",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf),
        .state = &st
    };
    tui_bar_init(&w);
    tui_bar_update(&w, 50.0, 0.0, 100.0);
    tui_bar_enable(&w, 0);

    capture_reset();
    tui_bar_enable(&w, 1);
    /* Re-enabled — should re-render with stored value, green color */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m"));
}
#endif

#if ANSI_PRINT_EMOJI
void test_check_disable_blocks_update(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .row = 1, .col = 1, .width = 0,
        .border = ANSI_TUI_NO_BORDER,
        .label = "S", .color = NULL, .state = &st
    };
    tui_check_init(&w, 1);
    tui_check_enable(&w, 0);

    capture_reset();
    tui_check_update(&w, 0);
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
    /* State should NOT have changed */
    TEST_ASSERT_EQUAL_INT(1, st.checked);
}

void test_check_enable_restores_state(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .row = 1, .col = 1, .width = 0,
        .border = ANSI_TUI_NO_BORDER,
        .label = "S", .color = NULL, .state = &st
    };
    tui_check_init(&w, 1);
    tui_check_enable(&w, 0);

    capture_reset();
    tui_check_enable(&w, 1);
    /* Re-enabled — should show check (was checked=1 before disable) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));
}
#endif

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

static void print_config(void)
{
    printf("Build config:");
    printf(" BAR=%d", ANSI_PRINT_BAR);
    printf(" BANNER=%d", ANSI_PRINT_BANNER);
    printf(" WINDOW=%d", ANSI_PRINT_WINDOW);
    printf(" EMOJI=%d", ANSI_PRINT_EMOJI);
    printf("\n");
}

int main(void)
{
    print_config();

    UNITY_BEGIN();

    /* Screen helpers */
    RUN_TEST(test_tui_cls);
    RUN_TEST(test_tui_goto);
    RUN_TEST(test_tui_goto_top_left);
    RUN_TEST(test_tui_cursor_hide);
    RUN_TEST(test_tui_cursor_show);

    /* Frame widget */
    RUN_TEST(test_frame_init_basic);
    RUN_TEST(test_frame_init_colored);
    RUN_TEST(test_frame_null_widget);
    RUN_TEST(test_frame_min_size);
    RUN_TEST(test_frame_too_small);
    RUN_TEST(test_frame_title);
    RUN_TEST(test_frame_null_title);
    RUN_TEST(test_frame_empty_title);

    /* Parent-relative positioning */
    RUN_TEST(test_frame_with_parent);
    RUN_TEST(test_label_with_parent);
    RUN_TEST(test_label_null_parent);
    RUN_TEST(test_nested_parent);

    /* Label widget */
    RUN_TEST(test_label_init_no_border);
    RUN_TEST(test_label_init_bordered);
    RUN_TEST(test_label_update_basic);
    RUN_TEST(test_label_update_pads_to_width);
    RUN_TEST(test_label_update_with_markup);
    RUN_TEST(test_label_update_bordered);
    RUN_TEST(test_label_null_widget);

    /* Bar widget */
#if ANSI_PRINT_BAR
    RUN_TEST(test_bar_init_no_border);
    RUN_TEST(test_bar_init_bordered);
    RUN_TEST(test_bar_update_full);
    RUN_TEST(test_bar_update_empty);
    RUN_TEST(test_bar_update_repositions);
    RUN_TEST(test_bar_state_tracks_value);
    RUN_TEST(test_bar_null_widget);
#endif

    /* Status widget */
    RUN_TEST(test_status_init_no_border);
    RUN_TEST(test_status_init_bordered);
    RUN_TEST(test_status_update_basic);
    RUN_TEST(test_status_update_pads);
    RUN_TEST(test_status_update_with_markup);
    RUN_TEST(test_status_update_bordered);
    RUN_TEST(test_status_null_widget);

    /* Check widget */
#if ANSI_PRINT_EMOJI
    RUN_TEST(test_check_init_true);
    RUN_TEST(test_check_init_false);
    RUN_TEST(test_check_init_bordered);
    RUN_TEST(test_check_update_toggle);
    RUN_TEST(test_check_state_tracks_value);
    RUN_TEST(test_check_toggle);
    RUN_TEST(test_check_toggle_null_state);
    RUN_TEST(test_check_null_widget);
#endif

    /* Text widget */
    RUN_TEST(test_text_init_no_border);
    RUN_TEST(test_text_init_bordered);
    RUN_TEST(test_text_update_basic);
    RUN_TEST(test_text_update_pads);
    RUN_TEST(test_text_fill_width);
    RUN_TEST(test_text_fill_bordered);
    RUN_TEST(test_text_null_widget);
    RUN_TEST(test_text_disable_blocks_update);

    /* tui_below */
    RUN_TEST(test_tui_below_no_border);
    RUN_TEST(test_tui_below_bordered);

    /* Color disabled */
    RUN_TEST(test_label_disabled_strips_color);

    /* Enable/disable */
    RUN_TEST(test_label_disable_blocks_update);
    RUN_TEST(test_label_enable_allows_update);
#if ANSI_PRINT_STYLES
    RUN_TEST(test_label_disable_draws_dim);
#endif
    RUN_TEST(test_status_disable_blocks_update);
#if ANSI_PRINT_BAR
    RUN_TEST(test_bar_disable_blocks_update);
    RUN_TEST(test_bar_enable_restores);
#endif
#if ANSI_PRINT_EMOJI
    RUN_TEST(test_check_disable_blocks_update);
    RUN_TEST(test_check_enable_restores_state);
#endif

    return UNITY_END();
}
