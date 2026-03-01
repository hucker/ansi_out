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

#if ANSI_TUI_FRAME

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

#endif /* ANSI_TUI_FRAME */

/* ------------------------------------------------------------------ */
/* Parent-relative positioning tests                                   */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_FRAME

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

#endif /* ANSI_TUI_FRAME */

#if ANSI_TUI_LABEL

void test_label_with_parent(void)
{
    /* Frame at (1,1). Label at (1,1) inside frame.
       Absolute = (1+1, 1+1+1) = (2, 3) = first interior cell. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 40, .height = 10, .color = NULL
    };
    tui_label_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &frame },
        .width = 8, .label = "V"
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
        .place = { .row = 5, .col = 10, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = NULL },
        .width = 8, .label = "X"
    };
    tui_label_init(&w);
    /* Cursor at absolute (5, 10) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[5;10H"));
}

void test_nested_parent(void)
{
    /* Outer frame (1,1), inner frame (3,3) inside outer,
       label (1,1) inside inner.
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &inner },
        .width = 5, .label = "N"
    };
    tui_label_init(&w);
    /* Cursor at absolute (5, 7) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[5;7H"));
}

#endif /* ANSI_TUI_LABEL */

/* ------------------------------------------------------------------ */
/* Negative coordinate tests                                           */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_LABEL

void test_negative_row_in_parent(void)
{
    /* Parent height 12 → interior rows = 12 - 2 = 10.
       Row -1 → last interior row = 10.
       Absolute row = 10 + parent->row(1) = 11. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 40, .height = 12, .color = NULL
    };
    tui_label_t w = {
        .place = { .row = -1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &frame },
        .width = 5, .label = "R"
    };
    tui_label_init(&w);
    /* Row: -1 + (12-2) + 1 = 10, then + parent row 1 = 11 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[11;3H"));
}

void test_negative_col_in_parent(void)
{
    /* Parent width 40 → interior cols = 40 - 4 = 36.
       Col -1 → last interior col = 36.
       Absolute col = 36 + parent->col(1) + 1 = 38. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 40, .height = 10, .color = NULL
    };
    tui_label_t w = {
        .place = { .row = 1, .col = -1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &frame },
        .width = 5, .label = "C"
    };
    tui_label_init(&w);
    /* Col: -1 + (40-4) + 1 = 36, then + parent col 1 + 1 = 38 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2;38H"));
}

void test_negative_both(void)
{
    /* Parent 40×12. Row -3 → 8, col -5 → 32.
       Absolute: row 8+1=9, col 32+1+1=34. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 40, .height = 12, .color = NULL
    };
    tui_label_t w = {
        .place = { .row = -3, .col = -5, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &frame },
        .width = 3, .label = "B"
    };
    tui_label_init(&w);
    /* Row: -3 + 10 + 1 = 8, + 1 = 9.  Col: -5 + 36 + 1 = 32, + 1 + 1 = 34. */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[9;34H"));
}

void test_negative_without_parent(void)
{
    /* No parent — negative coords pass through unchanged. */
    tui_label_t w = {
        .place = { .row = -3, .col = -5, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = NULL },
        .width = 5, .label = "X"
    };
    tui_label_init(&w);
    /* tui_resolve returns (-3, -5) unchanged */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[-3;-5H"));
}

void test_negative_nested(void)
{
    /* Outer (1,1, 60×30). Inner frame at row -5 inside outer.
       Outer interior height = 30-2 = 28. Row -5 → 28+1-5 = 24.
       Inner absolute row = 24 + outer row 1 = 25.
       Label at (1,1) inside inner → absolute row = 1 + 25 = 26. */
    const tui_frame_t outer = {
        .row = 1, .col = 1, .width = 60, .height = 30, .color = NULL
    };
    const tui_frame_t inner = {
        .row = -5, .col = 1, .width = 30, .height = 5,
        .color = NULL, .parent = &outer
    };
    tui_label_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &inner },
        .width = 5, .label = "N"
    };
    tui_label_init(&w);
    /* Inner row: -5 + 28 + 1 = 24, + outer 1 = 25.
       Label row: 1 + 25 = 26.  Label col: 1 + 1 + 1 + 1 + 1 = 5. */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[26;"));
}

#endif /* ANSI_TUI_LABEL */

#if ANSI_TUI_TEXT

void test_text_fill_negative_col(void)
{
    /* Parent width 30, interior = 26.  Widget at col -10.
       Resolved col = -10 + 26 + 1 = 17.
       Effective fill width = 26 - (17-1) - 0 = 10. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 30, .height = 10, .color = NULL
    };
    tui_text_t w = {
        .place = { .row = 1, .col = -10, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &frame },
        .width = -1
    };
    tui_text_init(&w);
    capture_reset();
    tui_text_update(&w, "hello");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "hello"));
    /* Effective width should be 10: count the padding spaces.
       After goto, first thing is 10 spaces (padding), then goto again + text. */
    /* Find the goto to the resolved position */
    /* Col: -10 + 26 + 1 = 17, + parent col 1 + 1 = 19.  Row: 1+1 = 2. */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2;19H"));
}

#endif /* ANSI_TUI_TEXT */

/* ------------------------------------------------------------------ */
/* tui_right tests                                                     */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_TEXT

void test_tui_right_no_border(void)
{
    const tui_text_t w = {
        .place = { .row = 1, .col = 5, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20
    };
    /* Unbordered: next col = 5 + 20 = 25 */
    TEST_ASSERT_EQUAL_INT(25, tui_right(&w));
}

void test_tui_right_negative_col(void)
{
    const tui_text_t w = {
        .place = { .row = 1, .col = -10, .border = ANSI_TUI_BORDER,
                   .color = NULL },
        .width = 6
    };
    /* -10 + 6 + 4 = 0 — arithmetic works for relative chaining */
    TEST_ASSERT_EQUAL_INT(0, tui_right(&w));
}

#endif /* ANSI_TUI_TEXT */

#if ANSI_TUI_STATUS

void test_tui_right_bordered(void)
{
    const tui_status_t w = {
        .place = { .row = 1, .col = 3, .border = ANSI_TUI_BORDER,
                   .color = NULL },
        .width = 10
    };
    /* Bordered: next col = 3 + 10 + 4 = 17 */
    TEST_ASSERT_EQUAL_INT(17, tui_right(&w));
}

#endif /* ANSI_TUI_STATUS */

#if ANSI_TUI_METRIC

void test_tui_right_metric(void)
{
    tui_metric_state_t st;
    const tui_metric_t w = {
        .place = { .row = 1, .col = 5, .border = ANSI_TUI_BORDER,
                   .color = "green" },
        .width = 14,
        .title = "T", .fmt = "%.1f",
        .color_lo = "blue", .color_hi = "red",
        .thresh_lo = 20.0, .thresh_hi = 80.0,
        .state = &st
    };
    /* Bordered metric: next col = 5 + 14 + 4 = 23 */
    TEST_ASSERT_EQUAL_INT(23, tui_right(&w));
}

#endif /* ANSI_TUI_METRIC */

/* ------------------------------------------------------------------ */
/* col=0 centering tests                                               */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_TEXT

void test_text_center_col(void)
{
    /* Parent width 40 → interior = 36.
       Widget width 10, unbordered: total = 10.
       Centered col = (36 - 10) / 2 + 1 = 14. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 40, .height = 10, .color = NULL
    };
    tui_text_t w = {
        .place = { .row = 1, .col = 0, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &frame },
        .width = 10
    };
    tui_text_init(&w);
    /* Absolute col: centered 14, + parent col 1 + 1 = 16 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2;16H"));
}

void test_text_center_bordered(void)
{
    /* Parent width 50 → interior = 46.
       Widget width 12, bordered: total = 12 + 4 = 16.
       Centered col = (46 - 16) / 2 + 1 = 16.
       Absolute col: 16 + parent col 1 + 1 = 18.
       Interior col: 18 + 2 (border offset) = 20. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 50, .height = 10, .color = NULL
    };
    tui_text_t w = {
        .place = { .row = 1, .col = 0, .border = ANSI_TUI_BORDER,
                   .color = NULL, .parent = &frame },
        .width = 12
    };
    tui_text_init(&w);
    /* Border position at (2, 18) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2;18H"));
    /* Interior position at (3, 20) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[3;20H"));
}

void test_center_no_parent(void)
{
    /* col=0 with no parent: col stays 0, effectively absolute col 0.
       This is an edge case — without a parent there's nothing to center in. */
    tui_text_t w = {
        .place = { .row = 1, .col = 0, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = NULL },
        .width = 10
    };
    tui_text_init(&w);
    /* No crash.  col=0 passed through to tui_resolve unchanged. */
    TEST_ASSERT_TRUE(capture_pos > 0);
}

#endif /* ANSI_TUI_TEXT */

#if ANSI_TUI_STATUS

void test_status_center_col(void)
{
    /* Parent width 40 → interior = 36.
       Widget width 20, unbordered: total = 20.
       Centered col = (36 - 20) / 2 + 1 = 9. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 40, .height = 10, .color = NULL
    };
    tui_status_t w = {
        .place = { .row = 1, .col = 0, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &frame },
        .width = 20
    };
    tui_status_init(&w);
    /* Absolute col: centered 9, + parent col 1 + 1 = 11 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2;11H"));
}

#endif /* ANSI_TUI_STATUS */

#if ANSI_TUI_METRIC

void test_metric_center_col(void)
{
    /* Parent width 50 → interior = 46.
       Metric width 14, always bordered: total = 14 + 4 = 18.
       Centered col = (46 - 18) / 2 + 1 = 15.
       Absolute col: 15 + parent col 1 + 1 = 17. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 50, .height = 10, .color = NULL
    };
    tui_metric_state_t st;
    const tui_metric_t w = {
        .place = { .row = 1, .col = 0, .border = ANSI_TUI_BORDER,
                   .color = "green", .parent = &frame },
        .width = 14,
        .title = "V", .fmt = "%.1f V",
        .color_lo = "blue", .color_hi = "red",
        .thresh_lo = 3.0, .thresh_hi = 3.6,
        .state = &st
    };
    tui_metric_init(&w);
    /* Border drawn at (2, 17) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2;17H"));
}

#endif /* ANSI_TUI_METRIC */

/* ------------------------------------------------------------------ */
/* Status width=-1 auto-fill tests                                     */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_STATUS

void test_status_fill_width(void)
{
    /* Parent interior = 30 - 4 = 26.  Widget at col 1, no border.
       Effective width = 26 - 0 - 0 = 26. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 30, .height = 10, .color = NULL
    };
    tui_status_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &frame },
        .width = -1
    };
    tui_status_init(&w);
    capture_reset();

    tui_status_update(&w, "test");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "test"));
    /* Find the goto and count padding spaces — should be 26 */
    const char *goto_pos = strstr(capture_buf, "\x1b[2;3H");
    TEST_ASSERT_NOT_NULL(goto_pos);
    int spaces = 0;
    const char *p = goto_pos + strlen("\x1b[2;3H");
    while (*p == ' ') { spaces++; p++; }
    TEST_ASSERT_EQUAL_INT(26, spaces);
}

void test_status_fill_bordered(void)
{
    /* Parent interior = 30 - 4 = 26.  Widget at col 1, bordered.
       Effective width = 26 - 0 - 4 = 22. */
    const tui_frame_t frame = {
        .row = 1, .col = 1, .width = 30, .height = 10, .color = NULL
    };
    tui_status_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_BORDER,
                   .color = NULL, .parent = &frame },
        .width = -1
    };
    tui_status_init(&w);
    /* Border present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    /* Interior position at (2+1, 3+2) = (3, 5) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[3;5H"));
}

#endif /* ANSI_TUI_STATUS */

/* ------------------------------------------------------------------ */
/* Label widget tests                                                  */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_LABEL

void test_label_init_no_border(void)
{
    tui_label_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 10, .label = "CPU"
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
        .place = { .row = 3, .col = 5, .border = ANSI_TUI_BORDER,
                   .color = "cyan" },
        .width = 8, .label = "MEM"
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 10, .label = "V"
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 10, .label = "V"
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 10, .label = "S"
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
        .place = { .row = 3, .col = 5, .border = ANSI_TUI_BORDER,
                   .color = "cyan" },
        .width = 8, .label = "CPU"
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

#endif /* ANSI_TUI_LABEL */

/* ------------------------------------------------------------------ */
/* Bar widget tests                                                    */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_BAR

void test_bar_init_no_border(void)
{
    char bar_buf[128];
    tui_bar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
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
        .place = { .row = 2, .col = 3, .border = ANSI_TUI_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = "CPU ",
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_bar_init(&w);
    capture_reset();

    tui_bar_update(&w, 100.0, 0.0, 100.0, 1);
    /* Full bar = full blocks (█ = \xe2\x96\x88) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x96\x88"));
    /* Green color code */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m"));
}

void test_bar_update_empty(void)
{
    char bar_buf[128];
    tui_bar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_bar_init(&w);
    capture_reset();

    tui_bar_update(&w, 0.0, 0.0, 100.0, 1);
    /* Empty bar = all track characters (░) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x96\x91"));
}

void test_bar_update_repositions(void)
{
    char bar_buf[128];
    tui_bar_t w = {
        .place = { .row = 5, .col = 10, .border = ANSI_TUI_NO_BORDER,
                   .color = "red" },
        .bar_width = 15, .label = "X ",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_bar_init(&w);
    capture_reset();

    tui_bar_update(&w, 50.0, 0.0, 100.0, 1);
    /* Cursor at (5, 10 + 2 for label "X ") = (5, 12) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[5;12H"));
}

void test_bar_state_tracks_value(void)
{
    char bar_buf[128];
    tui_bar_state_t st;
    const tui_bar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf),
        .state = &st
    };
    tui_bar_init(&w);
    /* Init draws bar at 0/100 — state should reflect that */
    TEST_ASSERT_TRUE(st.value == 0.0);

    tui_bar_update(&w, 75.0, 10.0, 200.0, 1);
    TEST_ASSERT_TRUE(st.value == 75.0);
    TEST_ASSERT_TRUE(st.min   == 10.0);
    TEST_ASSERT_TRUE(st.max   == 200.0);
}

void test_bar_null_widget(void)
{
    tui_bar_init(NULL);
    tui_bar_update(NULL, 50.0, 0.0, 100.0, 1);
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

#endif /* ANSI_TUI_BAR */

/* ------------------------------------------------------------------ */
/* Percent-bar widget tests                                            */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_PBAR

void test_pbar_init_no_border(void)
{
    char bar_buf[128];
    tui_pbar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_pbar_init(&w);
    /* Bar should render track characters (light shade) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x96\x91"));
    /* Percent text " 0%" should appear */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "0%"));
}

void test_pbar_init_bordered(void)
{
    char bar_buf[128];
    tui_pbar_t w = {
        .place = { .row = 2, .col = 3, .border = ANSI_TUI_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = "CPU ",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_pbar_init(&w);
    /* Border present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    /* Label text present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "CPU "));
}

void test_pbar_update_50(void)
{
    char bar_buf[128];
    tui_pbar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_pbar_init(&w);
    capture_reset();

    tui_pbar_update(&w, 50, 1);
    /* Full blocks present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x96\x88"));
    /* Percent text */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "50%"));
    /* Green color code */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m"));
}

void test_pbar_update_100(void)
{
    char bar_buf[128];
    tui_pbar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "cyan" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_pbar_init(&w);
    capture_reset();

    tui_pbar_update(&w, 100, 1);
    /* Percent text "100%" */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "100%"));
}

void test_pbar_update_clamps(void)
{
    char bar_buf[128];
    tui_pbar_state_t st;
    const tui_pbar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf),
        .state = &st
    };
    tui_pbar_init(&w);

    tui_pbar_update(&w, 150, 1);
    TEST_ASSERT_EQUAL_INT(100, st.percent);

    tui_pbar_update(&w, -10, 1);
    TEST_ASSERT_EQUAL_INT(0, st.percent);
}

void test_pbar_update_repositions(void)
{
    char bar_buf[128];
    tui_pbar_t w = {
        .place = { .row = 5, .col = 10, .border = ANSI_TUI_NO_BORDER,
                   .color = "red" },
        .bar_width = 15, .label = "X ",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf)
    };
    tui_pbar_init(&w);
    capture_reset();

    tui_pbar_update(&w, 50, 1);
    /* Cursor at (5, 10 + 2 for label "X ") = (5, 12) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[5;12H"));
}

void test_pbar_state_tracks_percent(void)
{
    char bar_buf[128];
    tui_pbar_state_t st;
    const tui_pbar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf),
        .state = &st
    };
    tui_pbar_init(&w);
    TEST_ASSERT_EQUAL_INT(0, st.percent);

    tui_pbar_update(&w, 73, 1);
    TEST_ASSERT_EQUAL_INT(73, st.percent);
}

void test_pbar_null_widget(void)
{
    tui_pbar_init(NULL);
    tui_pbar_update(NULL, 50, 1);
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

void test_pbar_disable_blocks_update(void)
{
    char bar_buf[128];
    tui_pbar_state_t st;
    const tui_pbar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf),
        .state = &st
    };
    tui_pbar_init(&w);
    tui_pbar_enable(&w, 0);

    capture_reset();
    tui_pbar_update(&w, 75, 1);
    /* Disabled — update should produce no output */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

void test_pbar_enable_restores(void)
{
    char bar_buf[128];
    tui_pbar_state_t st;
    const tui_pbar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf),
        .state = &st
    };
    tui_pbar_init(&w);
    tui_pbar_update(&w, 50, 1);
    tui_pbar_enable(&w, 0);

    capture_reset();
    tui_pbar_enable(&w, 1);
    /* Re-enabled — should re-render with stored value, green color */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "50%"));
}

void test_pbar_force0_skips_same(void)
{
    char bar_buf[128];
    tui_pbar_state_t st;
    const tui_pbar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf), .state = &st
    };
    tui_pbar_init(&w);
    tui_pbar_update(&w, 50, 1);
    capture_reset();
    tui_pbar_update(&w, 50, 0);
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

void test_pbar_force0_redraws_on_change(void)
{
    char bar_buf[128];
    tui_pbar_state_t st;
    const tui_pbar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf), .state = &st
    };
    tui_pbar_init(&w);
    tui_pbar_update(&w, 50, 1);
    capture_reset();
    tui_pbar_update(&w, 60, 0);
    TEST_ASSERT_TRUE(capture_pos > 0);
}

void test_pbar_force1_redraws_same(void)
{
    char bar_buf[128];
    tui_pbar_state_t st;
    const tui_pbar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf), .state = &st
    };
    tui_pbar_init(&w);
    tui_pbar_update(&w, 50, 1);
    capture_reset();
    tui_pbar_update(&w, 50, 1);
    TEST_ASSERT_TRUE(capture_pos > 0);
}

#endif /* ANSI_TUI_PBAR */

/* ------------------------------------------------------------------ */
/* Status widget tests                                                 */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_STATUS

void test_status_init_no_border(void)
{
    tui_status_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20
    };
    tui_status_init(&w);
    /* Cursor positioned at (1,1) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1;1H"));
}

void test_status_init_bordered(void)
{
    tui_status_t w = {
        .place = { .row = 8, .col = 3, .border = ANSI_TUI_BORDER,
                   .color = "yellow" },
        .width = 30
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20
    };
    tui_status_init(&w);
    capture_reset();

    tui_status_update(&w, "System OK");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "System OK"));
}

void test_status_update_pads(void)
{
    tui_status_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20
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
        .place = { .row = 8, .col = 3, .border = ANSI_TUI_BORDER,
                   .color = "yellow" },
        .width = 30
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

#endif /* ANSI_TUI_STATUS */

/* ------------------------------------------------------------------ */
/* Text widget tests                                                   */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_TEXT

void test_text_init_no_border(void)
{
    tui_text_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20
    };
    tui_text_init(&w);
    /* Cursor positioned at (1,1) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[1;1H"));
}

void test_text_init_bordered(void)
{
    tui_text_t w = {
        .place = { .row = 5, .col = 3, .border = ANSI_TUI_BORDER,
                   .color = "cyan" },
        .width = 25
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20
    };
    tui_text_init(&w);
    capture_reset();

    tui_text_update(&w, "Hello World");
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Hello World"));
}

void test_text_update_pads(void)
{
    tui_text_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL, .parent = &frame },
        .width = -1
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_BORDER,
                   .color = NULL, .parent = &frame },
        .width = -1
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20, .state = &st
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

#endif /* ANSI_TUI_TEXT */

/* ------------------------------------------------------------------ */
/* tui_below tests                                                     */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_TEXT

void test_tui_below_no_border(void)
{
    const tui_text_t w = {
        .place = { .row = 5, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20
    };
    /* Unbordered widget occupies 1 row: next row = 5 + 1 = 6 */
    TEST_ASSERT_EQUAL_INT(6, tui_below(&w));
}

#endif /* ANSI_TUI_TEXT */

#if ANSI_TUI_LABEL

void test_tui_below_bordered(void)
{
    const tui_label_t w = {
        .place = { .row = 3, .col = 1, .border = ANSI_TUI_BORDER,
                   .color = NULL },
        .width = 10, .label = "X"
    };
    /* Bordered widget occupies 3 rows: next row = 3 + 3 = 6 */
    TEST_ASSERT_EQUAL_INT(6, tui_below(&w));
}

#endif /* ANSI_TUI_LABEL */

/* ------------------------------------------------------------------ */
/* Color-disabled tests                                                */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_LABEL

void test_label_disabled_strips_color(void)
{
    ansi_set_enabled(0);
    tui_label_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "red" },
        .width = 10, .label = "T"
    };
    tui_label_init(&w);
    /* Label text present, no ANSI color codes (except cursor escapes) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "T: "));
    /* The escape sequences for cursor positioning still present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b["));
}

#endif /* ANSI_TUI_LABEL */

/* ------------------------------------------------------------------ */
/* Check widget                                                        */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_CHECK

static void test_check_init_true(void)
{
    const tui_check_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 0, .label = "Online"
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 0, .label = "Offline"
    };
    tui_check_init(&w, 0);
    /* Should contain cross emoji (U+274C = \xe2\x9d\x8c) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9d\x8c"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Offline"));
}

static void test_check_init_bordered(void)
{
    const tui_check_t w = {
        .place = { .row = 3, .col = 5, .border = ANSI_TUI_BORDER,
                   .color = "cyan" },
        .width = 0, .label = "Ready"
    };
    tui_check_init(&w, 1);
    /* Should have border characters and the check emoji */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "Ready"));
}

static void test_check_update_toggle(void)
{
    const tui_check_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 0, .label = "State"
    };
    tui_check_init(&w, 1);
    /* Starts with check */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));

    capture_reset();
    tui_check_update(&w, 0, 1);
    /* Now has cross, not check */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9d\x8c"));
    TEST_ASSERT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));

    capture_reset();
    tui_check_update(&w, 1, 1);
    /* Back to check */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));
}

static void test_check_state_tracks_value(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 0, .label = "S", .state = &st
    };
    tui_check_init(&w, 1);
    TEST_ASSERT_EQUAL_INT(1, st.checked);

    tui_check_update(&w, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, st.checked);
}

static void test_check_toggle(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 0, .label = "T", .state = &st
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 0, .label = "X"
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
    tui_check_update(NULL, 0, 1);
    tui_check_toggle(NULL);
    /* No crash */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

#endif /* ANSI_TUI_CHECK */

/* ------------------------------------------------------------------ */
/* Emoji bar widget tests                                              */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_EBAR

static const char *m_ebar_emoji[] = { ":star:", ":star:", ":star:" };

void test_ebar_init(void)
{
    tui_ebar_state_t st;
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "yellow" },
        .label = "R ", .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = ":white_circle:",
        .show_value = 0, .state = &st
    };
    tui_ebar_init(&w);
    TEST_ASSERT_EQUAL_INT(1, st.enabled);
    TEST_ASSERT_EQUAL_INT(0, st.value);
}

void test_ebar_init_bordered(void)
{
    tui_ebar_state_t st;
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_BORDER,
                   .color = "cyan" },
        .label = NULL, .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = NULL,
        .show_value = 0, .state = &st
    };
    capture_reset();
    tui_ebar_init(&w);
    /* Border should be drawn */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));  /* ╔ */
}

void test_ebar_update(void)
{
    tui_ebar_state_t st;
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "yellow" },
        .label = NULL, .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = ":white_circle:",
        .show_value = 0, .state = &st
    };
    tui_ebar_init(&w);
    capture_reset();
    tui_ebar_update(&w, 2, 1);
    /* Should contain two star emoji (U+2B50 = \xe2\xad\x90) */
    TEST_ASSERT_EQUAL_INT(2, st.value);
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\xad\x90"));
}

void test_ebar_update_with_value(void)
{
    tui_ebar_state_t st;
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "yellow" },
        .label = NULL, .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = NULL,
        .show_value = 1, .state = &st
    };
    tui_ebar_init(&w);
    capture_reset();
    tui_ebar_update(&w, 2, 1);
    /* Should contain " 2/3" */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, " 2/3"));
}

void test_ebar_update_no_value(void)
{
    tui_ebar_state_t st;
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "yellow" },
        .label = NULL, .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = NULL,
        .show_value = 0, .state = &st
    };
    tui_ebar_init(&w);
    capture_reset();
    tui_ebar_update(&w, 1, 1);
    /* Should NOT contain any digit */
    TEST_ASSERT_NULL(strstr(capture_buf, "1/"));
}

void test_ebar_force0_skips_same(void)
{
    tui_ebar_state_t st;
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "yellow" },
        .label = NULL, .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = NULL,
        .show_value = 0, .state = &st
    };
    tui_ebar_init(&w);
    tui_ebar_update(&w, 2, 1);
    capture_reset();
    tui_ebar_update(&w, 2, 0);
    /* No output — same value, not forced */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

void test_ebar_force1_redraws_same(void)
{
    tui_ebar_state_t st;
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "yellow" },
        .label = NULL, .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = NULL,
        .show_value = 0, .state = &st
    };
    tui_ebar_init(&w);
    tui_ebar_update(&w, 2, 1);
    capture_reset();
    tui_ebar_update(&w, 2, 1);
    /* Output — forced redraw */
    TEST_ASSERT_NOT_EQUAL(0, capture_pos);
}

void test_ebar_null_state_always_draws(void)
{
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "yellow" },
        .label = NULL, .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = NULL,
        .show_value = 0, .state = NULL
    };
    capture_reset();
    tui_ebar_update(&w, 1, 0);
    /* No state → always draws */
    TEST_ASSERT_NOT_EQUAL(0, capture_pos);
}

void test_ebar_disable_blocks(void)
{
    tui_ebar_state_t st;
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "yellow" },
        .label = NULL, .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = NULL,
        .show_value = 0, .state = &st
    };
    tui_ebar_init(&w);
    tui_ebar_enable(&w, 0);
    capture_reset();
    tui_ebar_update(&w, 2, 1);
    /* Disabled — no output */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

void test_ebar_enable_restores(void)
{
    tui_ebar_state_t st;
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "yellow" },
        .label = NULL, .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = NULL,
        .show_value = 0, .state = &st
    };
    tui_ebar_init(&w);
    tui_ebar_update(&w, 2, 1);
    tui_ebar_enable(&w, 0);
    capture_reset();
    tui_ebar_enable(&w, 1);
    /* Re-enabled — should draw stars */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\xad\x90"));
}

void test_ebar_null_widget(void)
{
    tui_ebar_init(NULL);
    tui_ebar_update(NULL, 0, 1);
    tui_ebar_enable(NULL, 1);
    /* No crash */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

void test_ebar_clamps_value(void)
{
    tui_ebar_state_t st;
    const tui_ebar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "yellow" },
        .label = NULL, .emoji = m_ebar_emoji, .count = 3,
        .slot_width = 2, .empty = NULL,
        .show_value = 0, .state = &st
    };
    tui_ebar_init(&w);
    tui_ebar_update(&w, 99, 1);
    TEST_ASSERT_EQUAL_INT(3, st.value);
    tui_ebar_update(&w, -5, 1);
    TEST_ASSERT_EQUAL_INT(0, st.value);
}

#endif /* ANSI_TUI_EBAR */

/* ------------------------------------------------------------------ */
/* Enable/disable tests                                                */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_LABEL

void test_label_disable_blocks_update(void)
{
    tui_label_state_t st;
    const tui_label_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 10, .label = "V", .state = &st
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 10, .label = "V", .state = &st
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
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_BORDER,
                   .color = "cyan" },
        .width = 10, .label = "X", .state = &st
    };
    tui_label_init(&w);

    capture_reset();
    tui_label_enable(&w, 0);
    /* Should contain dim ANSI code (ESC[2m) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[2m"));
}
#endif /* ANSI_PRINT_STYLES */

#endif /* ANSI_TUI_LABEL */

#if ANSI_TUI_STATUS

void test_status_disable_blocks_update(void)
{
    tui_status_state_t st;
    const tui_status_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 20, .state = &st
    };
    tui_status_init(&w);
    TEST_ASSERT_EQUAL_INT(1, st.enabled);

    tui_status_enable(&w, 0);
    capture_reset();
    tui_status_update(&w, "test");
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

#endif /* ANSI_TUI_STATUS */

#if ANSI_TUI_BAR

void test_bar_disable_blocks_update(void)
{
    char bar_buf[128];
    tui_bar_state_t st;
    const tui_bar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf),
        .state = &st
    };
    tui_bar_init(&w);
    tui_bar_enable(&w, 0);

    capture_reset();
    tui_bar_update(&w, 75.0, 0.0, 100.0, 1);
    /* Disabled — update should produce no output */
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

void test_bar_enable_restores(void)
{
    char bar_buf[128];
    tui_bar_state_t st;
    const tui_bar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .label = NULL,
        .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf),
        .state = &st
    };
    tui_bar_init(&w);
    tui_bar_update(&w, 50.0, 0.0, 100.0, 1);
    tui_bar_enable(&w, 0);

    capture_reset();
    tui_bar_enable(&w, 1);
    /* Re-enabled — should re-render with stored value, green color */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m"));
}

#endif /* ANSI_TUI_BAR */

#if ANSI_TUI_CHECK

void test_check_disable_blocks_update(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 0, .label = "S", .state = &st
    };
    tui_check_init(&w, 1);
    tui_check_enable(&w, 0);

    capture_reset();
    tui_check_update(&w, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
    /* State should NOT have changed */
    TEST_ASSERT_EQUAL_INT(1, st.checked);
}

void test_check_enable_restores_state(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = NULL },
        .width = 0, .label = "S", .state = &st
    };
    tui_check_init(&w, 1);
    tui_check_enable(&w, 0);

    capture_reset();
    tui_check_enable(&w, 1);
    /* Re-enabled — should show check (was checked=1 before disable) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x9c\x85"));
}

#endif /* ANSI_TUI_CHECK */

/* ------------------------------------------------------------------ */
/* Metric widget tests                                                 */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_METRIC

/** Helper: create a default metric descriptor for testing. */
static tui_metric_state_t m_metric_st;
static const tui_metric_t m_metric_default = {
    .place = { .row = 5, .col = 10, .border = ANSI_TUI_BORDER,
               .color = "green", .parent = NULL },
    .width = 14,
    .title = "TEMP", .fmt = "%.1f C",
    .color_lo = "blue", .color_hi = "red",
    .thresh_lo = 20.0, .thresh_hi = 80.0,
    .state = &m_metric_st
};

static void test_metric_init(void)
{
    tui_metric_init(&m_metric_default);
    /* Border chars present (double: ╔ = \xe2\x95\x94, ╝ = \xe2\x95\x9d) */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x9d"));
    /* Title present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "TEMP"));
    /* Nominal color used for border */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m")); /* green fg */
    /* No background fill */
    TEST_ASSERT_NULL(strstr(capture_buf, "\x1b[42m"));
    /* State initialized */
    TEST_ASSERT_EQUAL_INT(1, m_metric_st.enabled);
    TEST_ASSERT_EQUAL_INT(0, m_metric_st.zone);
}

static void test_metric_update_nominal(void)
{
    tui_metric_init(&m_metric_default);
    capture_reset();
    tui_metric_update(&m_metric_default, 50.0, 1);
    /* Value text present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "50.0 C"));
    /* Green foreground for value text */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[32m"));
    /* State tracks value */
    TEST_ASSERT_EQUAL_FLOAT(50.0, m_metric_st.value);
    TEST_ASSERT_EQUAL_INT(0, m_metric_st.zone);
}

static void test_metric_update_low(void)
{
    tui_metric_init(&m_metric_default);
    capture_reset();
    tui_metric_update(&m_metric_default, 10.0, 1);
    /* Value text present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "10.0 C"));
    /* Blue foreground for value text */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[34m"));
    /* Zone tracks low */
    TEST_ASSERT_EQUAL_INT(-1, m_metric_st.zone);
}

static void test_metric_update_high(void)
{
    tui_metric_init(&m_metric_default);
    capture_reset();
    tui_metric_update(&m_metric_default, 95.0, 1);
    /* Value text present */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "95.0 C"));
    /* Red foreground for value text */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[31m"));
    TEST_ASSERT_EQUAL_INT(1, m_metric_st.zone);
}

static void test_metric_zone_change(void)
{
    tui_metric_init(&m_metric_default);
    tui_metric_update(&m_metric_default, 50.0, 1); /* nominal */
    capture_reset();
    /* Same zone, force=0 — no border redraw */
    tui_metric_update(&m_metric_default, 60.0, 0);
    TEST_ASSERT_NULL(strstr(capture_buf, "\xe2\x95\x94")); /* no corner = no border */

    capture_reset();
    /* Cross into high zone — border redrawn */
    tui_metric_update(&m_metric_default, 85.0, 0);
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94")); /* corner = border drawn */
}

static void test_metric_null_widget(void)
{
    tui_metric_init(NULL);   /* should not crash */
    tui_metric_update(NULL, 1.0, 1);
    tui_metric_enable(NULL, 0);
}

static void test_metric_disable_blocks(void)
{
    tui_metric_init(&m_metric_default);
    tui_metric_enable(&m_metric_default, 0);
    capture_reset();
    tui_metric_update(&m_metric_default, 99.0, 1);
    /* Output should be empty — update blocked */
    TEST_ASSERT_EQUAL_STRING("", capture_buf);
}

static void test_metric_enable_restores(void)
{
    tui_metric_init(&m_metric_default);
    tui_metric_update(&m_metric_default, 85.0, 1); /* high zone */
    tui_metric_enable(&m_metric_default, 0);
    capture_reset();
    tui_metric_enable(&m_metric_default, 1);
    /* Should redraw with high (red) color */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[31m")); /* red fg */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "85.0 C"));
}

static void test_metric_with_parent(void)
{
    static const tui_frame_t parent = {
        .row = 2, .col = 3, .width = 50, .height = 10,
        .color = "cyan", .parent = NULL
    };
    static tui_metric_state_t st;
    static const tui_metric_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_BORDER,
                   .color = "green", .parent = &parent },
        .width = 14,
        .title = "V", .fmt = "%.2f V",
        .color_lo = "red", .color_hi = "red",
        .thresh_lo = 3.0, .thresh_hi = 3.6,
        .state = &st
    };
    tui_metric_init(&w);
    /* Row offset: parent row 2 + widget row 1 = 3 */
    /* Col offset: parent col 3 + 1 (parent border) + widget col 1 = 5 */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\x1b[3;5H"));
}

static void test_metric_state_tracks(void)
{
    tui_metric_init(&m_metric_default);
    tui_metric_update(&m_metric_default, 15.0, 1);
    TEST_ASSERT_EQUAL_FLOAT(15.0, m_metric_st.value);
    TEST_ASSERT_EQUAL_INT(-1, m_metric_st.zone);

    tui_metric_update(&m_metric_default, 50.0, 1);
    TEST_ASSERT_EQUAL_FLOAT(50.0, m_metric_st.value);
    TEST_ASSERT_EQUAL_INT(0, m_metric_st.zone);

    tui_metric_update(&m_metric_default, 90.0, 1);
    TEST_ASSERT_EQUAL_FLOAT(90.0, m_metric_st.value);
    TEST_ASSERT_EQUAL_INT(1, m_metric_st.zone);
}

#endif /* ANSI_TUI_METRIC */

/* ------------------------------------------------------------------ */
/* force=0 change detection                                            */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_CHECK
static void test_check_force0_skips_same(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER },
        .width = 0, .label = "S", .state = &st
    };
    tui_check_init(&w, 1);
    capture_reset();
    tui_check_update(&w, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

static void test_check_force0_redraws_on_change(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER },
        .width = 0, .label = "S", .state = &st
    };
    tui_check_init(&w, 1);
    capture_reset();
    tui_check_update(&w, 0, 0);
    TEST_ASSERT_TRUE(capture_pos > 0);
}

static void test_check_force1_redraws_same(void)
{
    tui_check_state_t st;
    const tui_check_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER },
        .width = 0, .label = "S", .state = &st
    };
    tui_check_init(&w, 1);
    capture_reset();
    tui_check_update(&w, 1, 1);
    TEST_ASSERT_TRUE(capture_pos > 0);
}

static void test_check_force0_null_state_redraws(void)
{
    const tui_check_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER },
        .width = 0, .label = "S"
    };
    tui_check_init(&w, 1);
    capture_reset();
    tui_check_update(&w, 1, 0);
    TEST_ASSERT_TRUE(capture_pos > 0);
}
#endif

#if ANSI_TUI_BAR
static void test_bar_force0_skips_same(void)
{
    char bar_buf[128];
    tui_bar_state_t st;
    const tui_bar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf), .state = &st
    };
    tui_bar_init(&w);
    tui_bar_update(&w, 50.0, 0.0, 100.0, 1);
    capture_reset();
    tui_bar_update(&w, 50.0, 0.0, 100.0, 0);
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

static void test_bar_force0_redraws_on_change(void)
{
    char bar_buf[128];
    tui_bar_state_t st;
    const tui_bar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf), .state = &st
    };
    tui_bar_init(&w);
    tui_bar_update(&w, 50.0, 0.0, 100.0, 1);
    capture_reset();
    tui_bar_update(&w, 60.0, 0.0, 100.0, 0);
    TEST_ASSERT_TRUE(capture_pos > 0);
}

static void test_bar_force0_redraws_on_range_change(void)
{
    char bar_buf[128];
    tui_bar_state_t st;
    const tui_bar_t w = {
        .place = { .row = 1, .col = 1, .border = ANSI_TUI_NO_BORDER,
                   .color = "green" },
        .bar_width = 10, .track = ANSI_BAR_LIGHT,
        .bar_buf = bar_buf, .bar_buf_size = sizeof(bar_buf), .state = &st
    };
    tui_bar_init(&w);
    tui_bar_update(&w, 50.0, 0.0, 100.0, 1);
    capture_reset();
    tui_bar_update(&w, 50.0, 0.0, 200.0, 0);
    TEST_ASSERT_TRUE(capture_pos > 0);
}
#endif

#if ANSI_TUI_METRIC
static void test_metric_force0_skips_same(void)
{
    tui_metric_init(&m_metric_default);
    tui_metric_update(&m_metric_default, 50.0, 1);
    capture_reset();
    tui_metric_update(&m_metric_default, 50.0, 0);
    TEST_ASSERT_EQUAL_INT(0, capture_pos);
}

static void test_metric_force0_redraws_on_change(void)
{
    tui_metric_init(&m_metric_default);
    tui_metric_update(&m_metric_default, 50.0, 1);
    capture_reset();
    tui_metric_update(&m_metric_default, 55.0, 0);
    TEST_ASSERT_TRUE(capture_pos > 0);
}

static void test_metric_force1_redraws_border(void)
{
    tui_metric_init(&m_metric_default);
    tui_metric_update(&m_metric_default, 50.0, 1);
    capture_reset();
    tui_metric_update(&m_metric_default, 55.0, 1);
    /* force=1 should redraw border even though zone didn't change */
    TEST_ASSERT_NOT_NULL(strstr(capture_buf, "\xe2\x95\x94"));
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
    printf("  TUI flags:");
    printf(" FRAME=%d", ANSI_TUI_FRAME);
    printf(" LABEL=%d", ANSI_TUI_LABEL);
    printf(" BAR=%d", ANSI_TUI_BAR);
    printf(" PBAR=%d", ANSI_TUI_PBAR);
    printf(" STATUS=%d", ANSI_TUI_STATUS);
    printf(" TEXT=%d", ANSI_TUI_TEXT);
    printf(" CHECK=%d", ANSI_TUI_CHECK);
    printf(" METRIC=%d", ANSI_TUI_METRIC);
    printf(" EBAR=%d", ANSI_TUI_EBAR);
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
#if ANSI_TUI_FRAME
    RUN_TEST(test_frame_init_basic);
    RUN_TEST(test_frame_init_colored);
    RUN_TEST(test_frame_null_widget);
    RUN_TEST(test_frame_min_size);
    RUN_TEST(test_frame_too_small);
    RUN_TEST(test_frame_title);
    RUN_TEST(test_frame_null_title);
    RUN_TEST(test_frame_empty_title);
#endif

    /* Parent-relative positioning */
#if ANSI_TUI_FRAME
    RUN_TEST(test_frame_with_parent);
#endif
#if ANSI_TUI_LABEL
    RUN_TEST(test_label_with_parent);
    RUN_TEST(test_label_null_parent);
    RUN_TEST(test_nested_parent);
#endif

    /* Negative coordinates */
#if ANSI_TUI_LABEL
    RUN_TEST(test_negative_row_in_parent);
    RUN_TEST(test_negative_col_in_parent);
    RUN_TEST(test_negative_both);
    RUN_TEST(test_negative_without_parent);
    RUN_TEST(test_negative_nested);
#endif
#if ANSI_TUI_TEXT
    RUN_TEST(test_text_fill_negative_col);
#endif

    /* tui_right */
#if ANSI_TUI_TEXT
    RUN_TEST(test_tui_right_no_border);
    RUN_TEST(test_tui_right_negative_col);
#endif
#if ANSI_TUI_STATUS
    RUN_TEST(test_tui_right_bordered);
#endif
#if ANSI_TUI_METRIC
    RUN_TEST(test_tui_right_metric);
#endif

    /* col=0 centering */
#if ANSI_TUI_TEXT
    RUN_TEST(test_text_center_col);
    RUN_TEST(test_text_center_bordered);
    RUN_TEST(test_center_no_parent);
#endif
#if ANSI_TUI_STATUS
    RUN_TEST(test_status_center_col);
#endif
#if ANSI_TUI_METRIC
    RUN_TEST(test_metric_center_col);
#endif

    /* Status width=-1 auto-fill */
#if ANSI_TUI_STATUS
    RUN_TEST(test_status_fill_width);
    RUN_TEST(test_status_fill_bordered);
#endif

    /* Label widget */
#if ANSI_TUI_LABEL
    RUN_TEST(test_label_init_no_border);
    RUN_TEST(test_label_init_bordered);
    RUN_TEST(test_label_update_basic);
    RUN_TEST(test_label_update_pads_to_width);
    RUN_TEST(test_label_update_with_markup);
    RUN_TEST(test_label_update_bordered);
    RUN_TEST(test_label_null_widget);
#endif

    /* Bar widget */
#if ANSI_TUI_BAR
    RUN_TEST(test_bar_init_no_border);
    RUN_TEST(test_bar_init_bordered);
    RUN_TEST(test_bar_update_full);
    RUN_TEST(test_bar_update_empty);
    RUN_TEST(test_bar_update_repositions);
    RUN_TEST(test_bar_state_tracks_value);
    RUN_TEST(test_bar_null_widget);
#endif

    /* Percent-bar widget */
#if ANSI_TUI_PBAR
    RUN_TEST(test_pbar_init_no_border);
    RUN_TEST(test_pbar_init_bordered);
    RUN_TEST(test_pbar_update_50);
    RUN_TEST(test_pbar_update_100);
    RUN_TEST(test_pbar_update_clamps);
    RUN_TEST(test_pbar_update_repositions);
    RUN_TEST(test_pbar_state_tracks_percent);
    RUN_TEST(test_pbar_null_widget);
    RUN_TEST(test_pbar_disable_blocks_update);
    RUN_TEST(test_pbar_enable_restores);
#endif

    /* Status widget */
#if ANSI_TUI_STATUS
    RUN_TEST(test_status_init_no_border);
    RUN_TEST(test_status_init_bordered);
    RUN_TEST(test_status_update_basic);
    RUN_TEST(test_status_update_pads);
    RUN_TEST(test_status_update_with_markup);
    RUN_TEST(test_status_update_bordered);
    RUN_TEST(test_status_null_widget);
#endif

    /* Check widget */
#if ANSI_TUI_CHECK
    RUN_TEST(test_check_init_true);
    RUN_TEST(test_check_init_false);
    RUN_TEST(test_check_init_bordered);
    RUN_TEST(test_check_update_toggle);
    RUN_TEST(test_check_state_tracks_value);
    RUN_TEST(test_check_toggle);
    RUN_TEST(test_check_toggle_null_state);
    RUN_TEST(test_check_null_widget);
#endif

    /* Emoji bar widget */
#if ANSI_TUI_EBAR
    RUN_TEST(test_ebar_init);
    RUN_TEST(test_ebar_init_bordered);
    RUN_TEST(test_ebar_update);
    RUN_TEST(test_ebar_update_with_value);
    RUN_TEST(test_ebar_update_no_value);
    RUN_TEST(test_ebar_null_state_always_draws);
    RUN_TEST(test_ebar_null_widget);
    RUN_TEST(test_ebar_clamps_value);
#endif

    /* Text widget */
#if ANSI_TUI_TEXT
    RUN_TEST(test_text_init_no_border);
    RUN_TEST(test_text_init_bordered);
    RUN_TEST(test_text_update_basic);
    RUN_TEST(test_text_update_pads);
    RUN_TEST(test_text_fill_width);
    RUN_TEST(test_text_fill_bordered);
    RUN_TEST(test_text_null_widget);
    RUN_TEST(test_text_disable_blocks_update);
#endif

    /* tui_below */
#if ANSI_TUI_TEXT
    RUN_TEST(test_tui_below_no_border);
#endif
#if ANSI_TUI_LABEL
    RUN_TEST(test_tui_below_bordered);
#endif

    /* Color disabled */
#if ANSI_TUI_LABEL
    RUN_TEST(test_label_disabled_strips_color);
#endif

    /* Enable/disable */
#if ANSI_TUI_LABEL
    RUN_TEST(test_label_disable_blocks_update);
    RUN_TEST(test_label_enable_allows_update);
#if ANSI_PRINT_STYLES
    RUN_TEST(test_label_disable_draws_dim);
#endif
#endif
#if ANSI_TUI_STATUS
    RUN_TEST(test_status_disable_blocks_update);
#endif
#if ANSI_TUI_BAR
    RUN_TEST(test_bar_disable_blocks_update);
    RUN_TEST(test_bar_enable_restores);
#endif
#if ANSI_TUI_CHECK
    RUN_TEST(test_check_disable_blocks_update);
    RUN_TEST(test_check_enable_restores_state);
#endif
#if ANSI_TUI_EBAR
    RUN_TEST(test_ebar_disable_blocks);
    RUN_TEST(test_ebar_enable_restores);
#endif

    /* Metric widget */
#if ANSI_TUI_METRIC
    RUN_TEST(test_metric_init);
    RUN_TEST(test_metric_update_nominal);
    RUN_TEST(test_metric_update_low);
    RUN_TEST(test_metric_update_high);
    RUN_TEST(test_metric_zone_change);
    RUN_TEST(test_metric_null_widget);
    RUN_TEST(test_metric_disable_blocks);
    RUN_TEST(test_metric_enable_restores);
    RUN_TEST(test_metric_with_parent);
    RUN_TEST(test_metric_state_tracks);
#endif

    /* force=0 change detection */
#if ANSI_TUI_CHECK
    RUN_TEST(test_check_force0_skips_same);
    RUN_TEST(test_check_force0_redraws_on_change);
    RUN_TEST(test_check_force1_redraws_same);
    RUN_TEST(test_check_force0_null_state_redraws);
#endif
#if ANSI_TUI_BAR
    RUN_TEST(test_bar_force0_skips_same);
    RUN_TEST(test_bar_force0_redraws_on_change);
    RUN_TEST(test_bar_force0_redraws_on_range_change);
#endif
#if ANSI_TUI_PBAR
    RUN_TEST(test_pbar_force0_skips_same);
    RUN_TEST(test_pbar_force0_redraws_on_change);
    RUN_TEST(test_pbar_force1_redraws_same);
#endif
#if ANSI_TUI_METRIC
    RUN_TEST(test_metric_force0_skips_same);
    RUN_TEST(test_metric_force0_redraws_on_change);
    RUN_TEST(test_metric_force1_redraws_border);
#endif
#if ANSI_TUI_EBAR
    RUN_TEST(test_ebar_force0_skips_same);
    RUN_TEST(test_ebar_force1_redraws_same);
#endif

    return UNITY_END();
}
