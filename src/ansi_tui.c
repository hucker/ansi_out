/**
 * @file ansi_tui.c
 * @brief Positioned TUI widget layer built on ansi_print.
 *
 * All output flows through ansi_puts() / ansi_print() — the TUI layer
 * does not store its own putc function.  Cursor-positioning escape
 * sequences pass through the ansi_print tokenizer unchanged because
 * they contain no ']' to close a markup tag.
 */

#include "ansi_tui.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal helper guard macros                                        */
/* Any TUI widget compiled in (for shared drawing primitives).         */
/* ------------------------------------------------------------------ */

#define ANSI_TUI_ANY_ (ANSI_TUI_FRAME || ANSI_TUI_LABEL || ANSI_TUI_BAR || \
                        ANSI_TUI_PBAR  || ANSI_TUI_STATUS || ANSI_TUI_TEXT || \
                        ANSI_TUI_CHECK || ANSI_TUI_METRIC)

/* Widgets that use tui_widget_goto() (all content widgets except metric) */
#define ANSI_TUI_GOTO_ (ANSI_TUI_LABEL || ANSI_TUI_BAR || ANSI_TUI_PBAR || \
                         ANSI_TUI_STATUS || ANSI_TUI_TEXT || ANSI_TUI_CHECK)

/* Widgets that use tui_pad() */
#define ANSI_TUI_PAD_ (ANSI_TUI_LABEL || ANSI_TUI_PBAR || ANSI_TUI_STATUS || \
                        ANSI_TUI_TEXT || ANSI_TUI_METRIC)

/* Widgets that use tui_center_col() */
#define ANSI_TUI_CENTER_ (ANSI_TUI_TEXT || ANSI_TUI_STATUS || ANSI_TUI_METRIC)

/* ------------------------------------------------------------------ */
/* Box-drawing characters (duplicated from ansi_print.c)               */
/* The BOX_* macros in ansi_print.c are file-scoped, so the TUI layer */
/* defines its own set using the same ANSI_PRINT_BOX_STYLE flag.      */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_ANY_

#if ANSI_PRINT_BOX_STYLE == ANSI_BOX_LIGHT
#define TUI_TL  "\xe2\x94\x8c"  /* U+250C  ┌ */
#define TUI_TR  "\xe2\x94\x90"  /* U+2510  ┐ */
#define TUI_BL  "\xe2\x94\x94"  /* U+2514  └ */
#define TUI_BR  "\xe2\x94\x98"  /* U+2518  ┘ */
#define TUI_HZ  "\xe2\x94\x80"  /* U+2500  ─ */
#define TUI_VT  "\xe2\x94\x82"  /* U+2502  │ */
#elif ANSI_PRINT_BOX_STYLE == ANSI_BOX_HEAVY
#define TUI_TL  "\xe2\x94\x8f"  /* U+250F  ┏ */
#define TUI_TR  "\xe2\x94\x93"  /* U+2513  ┓ */
#define TUI_BL  "\xe2\x94\x97"  /* U+2517  ┗ */
#define TUI_BR  "\xe2\x94\x9b"  /* U+251B  ┛ */
#define TUI_HZ  "\xe2\x94\x81"  /* U+2501  ━ */
#define TUI_VT  "\xe2\x94\x83"  /* U+2503  ┃ */
#elif ANSI_PRINT_BOX_STYLE == ANSI_BOX_DOUBLE
#define TUI_TL  "\xe2\x95\x94"  /* U+2554  ╔ */
#define TUI_TR  "\xe2\x95\x97"  /* U+2557  ╗ */
#define TUI_BL  "\xe2\x95\x9a"  /* U+255A  ╚ */
#define TUI_BR  "\xe2\x95\x9d"  /* U+255D  ╝ */
#define TUI_HZ  "\xe2\x95\x90"  /* U+2550  ═ */
#define TUI_VT  "\xe2\x95\x91"  /* U+2551  ║ */
#elif ANSI_PRINT_BOX_STYLE == ANSI_BOX_ROUNDED
#define TUI_TL  "\xe2\x95\xad"  /* U+256D  ╭ */
#define TUI_TR  "\xe2\x95\xae"  /* U+256E  ╮ */
#define TUI_BL  "\xe2\x95\xb0"  /* U+2570  ╰ */
#define TUI_BR  "\xe2\x95\xaf"  /* U+256F  ╯ */
#define TUI_HZ  "\xe2\x94\x80"  /* U+2500  ─ */
#define TUI_VT  "\xe2\x94\x82"  /* U+2502  │ */
#else
#error "Unknown ANSI_PRINT_BOX_STYLE value"
#endif

#endif /* ANSI_TUI_ANY_ */

/* ------------------------------------------------------------------ */
/* Screen helpers                                                      */
/* ------------------------------------------------------------------ */

void tui_cls(void)
{
    ansi_puts("\x1b[2J\x1b[H");
}

void tui_goto(int row, int col)
{
    char seq[24];
    snprintf(seq, sizeof(seq), "\x1b[%d;%dH", row, col);
    ansi_puts(seq);
}

void tui_cursor_hide(void)
{
    ansi_puts("\x1b[?25l");
}

void tui_cursor_show(void)
{
    ansi_puts("\x1b[?25h");
}

/* ------------------------------------------------------------------ */
/* Internal helpers — shared drawing primitives (any widget)            */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_ANY_

/** Build a horizontal run of box-drawing characters into p.
 *  Returns the number of bytes written (not including NUL). */
static int tui_fill_horz(char *p, size_t avail, int count)
{
    int written = 0;
    for (int i = 0; i < count; i++) {
        int n = snprintf(p + written, avail - (size_t)written, "%s", TUI_HZ);
        if (n < 0 || (size_t)(written + n) >= avail) break;
        written += n;
    }
    return written;
}

/** Resolve a widget's local (row,col) to absolute screen coordinates
 *  by walking the parent frame chain.  NULL parent = no offset.
 *  Negative row/col count from the end of the parent's interior:
 *  -1 = last interior position, -2 = second-to-last, etc. */
static void tui_resolve(const tui_frame_t *parent, int row, int col,
                         int *abs_row, int *abs_col)
{
    while (parent) {
        /* Negative coords count from the end of the parent's interior.
         * height - 2 = interior rows (minus top & bottom border).
         * width  - 4 = interior cols (minus left/right border + padding).
         * + 1 converts from 0-based "from-end" to 1-based position. */
        if (row < 0) row += (parent->height - 2) + 1;
        if (col < 0) col += (parent->width  - 4) + 1;

        row += parent->row;
        col += parent->col + 1;
        parent = parent->parent;
    }
    *abs_row = row;
    *abs_col = col;
}

/** Draw a complete box border at the given position.
 *  @param iw    interior width (chars between the side borders)
 *  @param ih    interior height (rows between top and bottom borders)
 *  @param color border color name, or NULL
 *  @param fill  if nonzero, fill interior rows with spaces */
static void tui_draw_border(int row, int col, int iw, int ih,
                            const char *color, int fill)
{
    size_t buf_size;
    char *buf = ansi_get_buf(&buf_size);
    if (!buf || buf_size < 32) return;

    char *p;
    char *end = buf + buf_size;

    /* --- top border --- */
    tui_goto(row, col);
    p = buf;
    if (color) p += snprintf(p, (size_t)(end - p), "[%s]", color);
    p += snprintf(p, (size_t)(end - p), "%s", TUI_TL);
    p += tui_fill_horz(p, (size_t)(end - p), iw + 2);
    p += snprintf(p, (size_t)(end - p), "%s", TUI_TR);
    if (color) snprintf(p, (size_t)(end - p), "[/]");
    ansi_puts(buf);

    /* --- side rows --- */
    for (int r = 0; r < ih; r++) {
        /* Left border */
        tui_goto(row + 1 + r, col);
        p = buf;
        if (color) p += snprintf(p, (size_t)(end - p), "[%s]", color);
        p += snprintf(p, (size_t)(end - p), "%s", TUI_VT);
        if (color) p += snprintf(p, (size_t)(end - p), "[/]");
        if (fill) {
            int n = iw + 2;
            int avail = (int)(end - p) - 1;
            if (n > avail) n = avail;
            memset(p, ' ', (size_t)n);
            p[n] = '\0';
            p += n;
        }
        if (color) p += snprintf(p, (size_t)(end - p), "[%s]", color);
        if (fill) {
            p += snprintf(p, (size_t)(end - p), "%s", TUI_VT);
            if (color) snprintf(p, (size_t)(end - p), "[/]");
            ansi_puts(buf);
        } else {
            if (color) snprintf(p, (size_t)(end - p), "[/]");
            ansi_puts(buf);
            /* Right border at far column */
            tui_goto(row + 1 + r, col + iw + 3);
            p = buf;
            if (color) p += snprintf(p, (size_t)(end - p), "[%s]", color);
            p += snprintf(p, (size_t)(end - p), "%s", TUI_VT);
            if (color) snprintf(p, (size_t)(end - p), "[/]");
            ansi_puts(buf);
        }
    }

    /* --- bottom border --- */
    tui_goto(row + ih + 1, col);
    p = buf;
    if (color) p += snprintf(p, (size_t)(end - p), "[%s]", color);
    p += snprintf(p, (size_t)(end - p), "%s", TUI_BL);
    p += tui_fill_horz(p, (size_t)(end - p), iw + 2);
    p += snprintf(p, (size_t)(end - p), "%s", TUI_BR);
    if (color) snprintf(p, (size_t)(end - p), "[/]");
    ansi_puts(buf);
}

#endif /* ANSI_TUI_ANY_ */

/* ------------------------------------------------------------------ */
/* Internal helpers — content widget positioning                       */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_GOTO_

/** Compute interior column offset for a bordered widget.
 *  Border adds "║ " = 2 columns of offset. */
static int tui_interior_col(tui_border_t border, int col)
{
    return border == ANSI_TUI_BORDER ? col + 2 : col;
}

/** Compute interior row offset for a bordered widget. */
static int tui_interior_row(tui_border_t border, int row)
{
    return border == ANSI_TUI_BORDER ? row + 1 : row;
}

/** Resolve a widget's parent chain, compute the interior origin,
 *  and move the cursor there.  Returns the interior position via
 *  optional out-params for callers that need further offsets
 *  (e.g. past a label prefix). */
static void tui_widget_goto(const tui_frame_t *parent, int row, int col,
                             tui_border_t border, int *out_ir, int *out_ic)
{
    int ar, ac;
    tui_resolve(parent, row, col, &ar, &ac);
    int ir = tui_interior_row(border, ar);
    int ic = tui_interior_col(border, ac);
    tui_goto(ir, ic);
    if (out_ir) *out_ir = ir;
    if (out_ic) *out_ic = ic;
}

/** Placement-aware wrapper for tui_widget_goto().
 *  @param col  Column override (may differ from p->col after centering). */
static void tui_place_goto(const tui_placement_t *p, int col,
                            int *out_ir, int *out_ic)
{
    tui_widget_goto(p->parent, p->row, col, p->border, out_ir, out_ic);
}

/** Resolve position, draw border (if requested), and goto interior.
 *  Consolidates the resolve + draw_border + goto sequence shared by
 *  all content-widget init and enable functions.
 *  @param col   Column override (may differ from p->col after centering).
 *  @param iw    Interior width for the border box.
 *  @param color Border/content color, or NULL. */
static void tui_widget_chrome(const tui_placement_t *p, int col, int iw,
                               const char *color, int *out_ir, int *out_ic)
{
    int ar, ac;
    tui_resolve(p->parent, p->row, col, &ar, &ac);
    if (p->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, iw, 1, color, 1);
    int ir = tui_interior_row(p->border, ar);
    int ic = tui_interior_col(p->border, ac);
    tui_goto(ir, ic);
    if (out_ir) *out_ir = ir;
    if (out_ic) *out_ic = ic;
}

#endif /* ANSI_TUI_GOTO_ */

/* ------------------------------------------------------------------ */
/* Internal helpers — padding and centering                            */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_PAD_

/** Emit n spaces at the current cursor position. */
static void tui_pad(int n)
{
    if (n <= 0) return;
    ansi_print("%*s", n, "");
}

#endif /* ANSI_TUI_PAD_ */

#if ANSI_TUI_CENTER_

/** Resolve col = 0 (center sentinel) to a centered column within the
 *  parent's interior.  Requires knowing the widget's interior width.
 *  Returns col unchanged if it is not 0 or parent is NULL. */
static int tui_center_col(int col, const tui_frame_t *parent,
                           int iw, tui_border_t border)
{
    if (col != 0 || !parent) return col;
    /* parent interior width (minus left/right border + padding) */
    int piw = parent->width - 4;
    /* total occupied columns including widget border overhead */
    int total = iw + (border == ANSI_TUI_BORDER ? 4 : 0);
    int centered = (piw - total) / 2 + 1;
    return centered > 0 ? centered : 1;
}

#endif /* ANSI_TUI_CENTER_ */

#if ANSI_TUI_STATUS || ANSI_TUI_TEXT

/** Compute effective width for a fill-to-parent widget.
 *  If width >= 0, returns width as-is.
 *  If width == -1 and parent is set, fills to parent's right edge. */
static int tui_effective_width(const tui_placement_t *p, int width)
{
    if (width >= 0) return width;
    if (!p->parent) return 0;
    /* parent interior width (minus left/right border + padding) */
    int piw = p->parent->width - 4;
    /* Resolve negative or zero col against parent interior */
    int c = p->col;
    if (c < 0) c += piw + 1;
    if (c == 0) c = 1;  /* col=0 (center) with width=-1: fill from left */
    /* 4 = widget's own border overhead (border + space x 2) */
    int bdr = (p->border == ANSI_TUI_BORDER) ? 4 : 0;
    int eff = piw - (c - 1) - bdr;
    return eff > 0 ? eff : 0;
}

#endif /* ANSI_TUI_STATUS || ANSI_TUI_TEXT */

/* ------------------------------------------------------------------ */
/* Frame widget                                                        */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_FRAME

void tui_frame_init(const tui_frame_t *f)
{
    if (!f || f->width < 5 || f->height < 3) return;
    int ar, ac;
    tui_resolve(f->parent, f->row, f->col, &ar, &ac);
    tui_draw_border(ar, ac, f->width - 4, f->height - 2, f->color, 0);

    /* Overlay title on the top border row if provided */
    if (f->title && f->title[0]) {
        tui_goto(ar, ac + 1);
        if (f->color)
            ansi_print(" [bold %s]%s[/] ", f->color, f->title);
        else
            ansi_print(" [bold]%s[/] ", f->title);
    }
}

#endif /* ANSI_TUI_FRAME */

/* ------------------------------------------------------------------ */
/* Label widget                                                        */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_LABEL

/** Compute the total interior width for a label (label + ": " + value). */
static int label_interior_width(const tui_label_t *w)
{
    int label_len = w->label ? (int)strlen(w->label) : 0;
    return label_len + 2 + w->width;   /* "Label: " + value area */
}

void tui_label_init(const tui_label_t *w)
{
    if (!w) return;

    if (w->state) w->state->enabled = 1;

    int iw = label_interior_width(w);
    tui_widget_chrome(&w->place, w->place.col, iw, w->place.color, NULL, NULL);
    if (w->label) {
        if (w->place.color)
            ansi_print("[%s]%s: [/]", w->place.color, w->label);
        else
            ansi_print("%s: ", w->label);
    }
    /* Blank the value area */
    tui_pad(w->width);
}

void tui_label_update(const tui_label_t *w, const char *fmt, ...)
{
    if (!w || !fmt) return;
    if (w->state && !w->state->enabled) return;

    /* Position at value area */
    int ir, ic;
    tui_place_goto(&w->place, w->place.col, &ir, &ic);
    int label_len = w->label ? (int)strlen(w->label) : 0;
    int value_col = ic + label_len + 2;  /* after "Label: " */

    /* Clear value area first, then reposition and write new value.
       This avoids over-padding past the right border when the value
       contains Rich markup (non-printing tags inflate strlen). */
    tui_goto(ir, value_col);
    tui_pad(w->width);

    tui_goto(ir, value_col);

    va_list ap;
    va_start(ap, fmt);
    ansi_vprint(fmt, ap);
    va_end(ap);
}

void tui_label_enable(const tui_label_t *w, int enabled)
{
    if (!w || !w->state) return;
    w->state->enabled = enabled;

    int iw = label_interior_width(w);
    const char *color = enabled ? w->place.color : "dim";
    tui_widget_chrome(&w->place, w->place.col, iw, color, NULL, NULL);
    if (w->label) {
        if (enabled && w->place.color)
            ansi_print("[%s]%s: [/]", w->place.color, w->label);
        else if (!enabled)
            ansi_print("[dim]%s: [/]", w->label);
        else
            ansi_print("%s: ", w->label);
    }
    /* Blank the value area (clears stale content when disabling) */
    tui_pad(w->width);
}

#endif /* ANSI_TUI_LABEL */

/* ------------------------------------------------------------------ */
/* Bar widget                                                          */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_BAR

/** Compute interior width for a bar widget (label + bar). */
static int bar_interior_width(const tui_bar_t *w)
{
    int label_len = w->label ? (int)strlen(w->label) : 0;
    return label_len + w->bar_width;
}

void tui_bar_init(const tui_bar_t *w)
{
    if (!w) return;

    if (w->state) w->state->enabled = 1;

    int iw = bar_interior_width(w);
    tui_widget_chrome(&w->place, w->place.col, iw, w->place.color, NULL, NULL);
    if (w->label) ansi_puts(w->label);

    /* Draw empty bar (value = min = 0) */
    tui_bar_update(w, 0.0, 0.0, 100.0, 1);
}

void tui_bar_update(const tui_bar_t *w,
                         double value, double min, double max, int force)
{
    if (!w || !w->bar_buf) return;
    if (w->state && !w->state->enabled) return;

    if (!force && w->state &&
        w->state->value == value &&
        w->state->min   == min   &&
        w->state->max   == max)
        return;

    if (w->state) {
        w->state->value = value;
        w->state->min   = min;
        w->state->max   = max;
    }

    ansi_bar(w->bar_buf, w->bar_buf_size,
             w->place.color, w->bar_width, w->track,
             value, min, max);

    /* Position cursor at bar area (after label) */
    int ir, ic;
    tui_place_goto(&w->place, w->place.col, &ir, &ic);
    int label_len = w->label ? (int)strlen(w->label) : 0;

    tui_goto(ir, ic + label_len);
    ansi_print("%s", w->bar_buf);
}

void tui_bar_enable(const tui_bar_t *w, int enabled)
{
    if (!w || !w->state) return;
    w->state->enabled = enabled;

    int iw = bar_interior_width(w);
    const char *color = enabled ? w->place.color : "dim";
    int ir, ic;
    tui_widget_chrome(&w->place, w->place.col, iw, color, &ir, &ic);
    if (w->label) {
        if (!enabled)
            ansi_print("[dim]%s[/]", w->label);
        else
            ansi_puts(w->label);
    }

    if (enabled) {
        /* Re-render the bar with stored values */
        w->state->enabled = 1;   /* allow update through */
        tui_bar_update(w, w->state->value, w->state->min, w->state->max, 1);
    } else {
        /* Draw a dim empty track */
        if (w->bar_buf) {
            ansi_bar(w->bar_buf, w->bar_buf_size,
                     "dim", w->bar_width, w->track, 0.0, 0.0, 100.0);
            int label_len = w->label ? (int)strlen(w->label) : 0;
            tui_goto(ir, ic + label_len);
            ansi_print("%s", w->bar_buf);
        }
    }
}

#endif /* ANSI_TUI_BAR */

/* ------------------------------------------------------------------ */
/* Percent-bar widget                                                  */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_PBAR

/** Compute interior width for a percent-bar widget (label + bar + " 100%"). */
static int pbar_interior_width(const tui_pbar_t *w)
{
    int label_len = w->label ? (int)strlen(w->label) : 0;
    return label_len + w->bar_width + 5;   /* bar + " 100%" max */
}

void tui_pbar_init(const tui_pbar_t *w)
{
    if (!w) return;

    if (w->state) w->state->enabled = 1;

    int iw = pbar_interior_width(w);
    tui_widget_chrome(&w->place, w->place.col, iw, w->place.color, NULL, NULL);
    if (w->label) ansi_puts(w->label);

    /* Draw empty bar (0%) */
    tui_pbar_update(w, 0, 1);
}

void tui_pbar_update(const tui_pbar_t *w, int percent, int force)
{
    if (!w || !w->bar_buf) return;
    if (w->state && !w->state->enabled) return;

    int pct = percent < 0 ? 0 : percent > 100 ? 100 : percent;

    if (!force && w->state && w->state->percent == pct) return;

    if (w->state) w->state->percent = pct;

    ansi_bar_percent(w->bar_buf, w->bar_buf_size,
                     w->place.color, w->bar_width, w->track, pct);

    /* Position cursor at bar area (after label) */
    int ir, ic;
    tui_place_goto(&w->place, w->place.col, &ir, &ic);
    int label_len = w->label ? (int)strlen(w->label) : 0;
    int bar_col = ic + label_len;

    /* Clear bar + percent area, then rewrite */
    tui_goto(ir, bar_col);
    tui_pad(w->bar_width + 5);

    tui_goto(ir, bar_col);
    ansi_print("%s", w->bar_buf);
}

void tui_pbar_enable(const tui_pbar_t *w, int enabled)
{
    if (!w || !w->state) return;
    w->state->enabled = enabled;

    int iw = pbar_interior_width(w);
    const char *color = enabled ? w->place.color : "dim";
    int ir, ic;
    tui_widget_chrome(&w->place, w->place.col, iw, color, &ir, &ic);
    if (w->label) {
        if (!enabled)
            ansi_print("[dim]%s[/]", w->label);
        else
            ansi_puts(w->label);
    }

    if (enabled) {
        /* Re-render the bar with stored percent */
        w->state->enabled = 1;   /* allow update through */
        tui_pbar_update(w, w->state->percent, 1);
    } else {
        /* Draw a dim empty track */
        if (w->bar_buf) {
            ansi_bar_percent(w->bar_buf, w->bar_buf_size,
                             "dim", w->bar_width, w->track, 0);
            int label_len = w->label ? (int)strlen(w->label) : 0;
            tui_goto(ir, ic + label_len);
            tui_pad(w->bar_width + 5);
            tui_goto(ir, ic + label_len);
            ansi_print("%s", w->bar_buf);
        }
    }
}

#endif /* ANSI_TUI_PBAR */

/* ------------------------------------------------------------------ */
/* Status widget                                                       */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_STATUS

void tui_status_init(const tui_status_t *w)
{
    if (!w) return;

    if (w->state) w->state->enabled = 1;

    int ew = tui_effective_width(&w->place, w->width);
    int col = tui_center_col(w->place.col, w->place.parent, ew, w->place.border);
    tui_widget_chrome(&w->place, col, ew, w->place.color, NULL, NULL);
    tui_pad(ew);
}

void tui_status_update(const tui_status_t *w,
                            const char *fmt, ...)
{
    if (!w || !fmt) return;
    if (w->state && !w->state->enabled) return;

    int ew = tui_effective_width(&w->place, w->width);
    int col = tui_center_col(w->place.col, w->place.parent, ew, w->place.border);
    int ir, ic;
    tui_place_goto(&w->place, col, &ir, &ic);

    /* Clear interior first, then reposition and write new text */
    tui_pad(ew);

    tui_goto(ir, ic);

    va_list ap;
    va_start(ap, fmt);
    ansi_vprint(fmt, ap);
    va_end(ap);
}

void tui_status_enable(const tui_status_t *w, int enabled)
{
    if (!w || !w->state) return;
    w->state->enabled = enabled;

    int ew = tui_effective_width(&w->place, w->width);
    int col = tui_center_col(w->place.col, w->place.parent, ew, w->place.border);
    const char *color = enabled ? w->place.color : "dim";
    tui_widget_chrome(&w->place, col, ew, color, NULL, NULL);
    tui_pad(ew);
}

#endif /* ANSI_TUI_STATUS */

/* ------------------------------------------------------------------ */
/* Text widget                                                         */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_TEXT

void tui_text_init(const tui_text_t *w)
{
    if (!w) return;

    if (w->state) w->state->enabled = 1;

    int ew = tui_effective_width(&w->place, w->width);
    int col = tui_center_col(w->place.col, w->place.parent, ew, w->place.border);
    tui_widget_chrome(&w->place, col, ew, w->place.color, NULL, NULL);
    tui_pad(ew);
}

void tui_text_update(const tui_text_t *w, const char *fmt, ...)
{
    if (!w || !fmt) return;
    if (w->state && !w->state->enabled) return;

    int ew = tui_effective_width(&w->place, w->width);
    int col = tui_center_col(w->place.col, w->place.parent, ew, w->place.border);
    int ir, ic;
    tui_place_goto(&w->place, col, &ir, &ic);

    /* Clear interior first, then reposition and write new text */
    tui_pad(ew);

    tui_goto(ir, ic);

    va_list ap;
    va_start(ap, fmt);
    ansi_vprint(fmt, ap);
    va_end(ap);
}

void tui_text_enable(const tui_text_t *w, int enabled)
{
    if (!w || !w->state) return;
    w->state->enabled = enabled;

    int ew = tui_effective_width(&w->place, w->width);
    int col = tui_center_col(w->place.col, w->place.parent, ew, w->place.border);
    const char *color = enabled ? w->place.color : "dim";
    tui_widget_chrome(&w->place, col, ew, color, NULL, NULL);
    tui_pad(ew);
}

#endif /* ANSI_TUI_TEXT */

/* ------------------------------------------------------------------ */
/* Check widget                                                        */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_CHECK

/** Compute interior width for a check widget (emoji + space + label). */
static int check_interior_width(const tui_check_t *w)
{
    int label_len = w->label ? (int)strlen(w->label) : 0;
    return 2 + 1 + label_len;   /* emoji(2 cols) + space + label */
}

void tui_check_init(const tui_check_t *w, int state)
{
    if (!w) return;

    if (w->state) {
        w->state->enabled = 1;
        w->state->checked = state;
    }

    int iw = w->width > 0 ? w->width : check_interior_width(w);
    tui_widget_chrome(&w->place, w->place.col, iw, w->place.color, NULL, NULL);
    ansi_puts(state ? "[green]:check:[/]" : "[red]:cross:[/]");
    if (w->label) {
        ansi_puts(" ");
        ansi_puts(w->label);
    }
}

void tui_check_update(const tui_check_t *w, int state, int force)
{
    if (!w) return;
    if (w->state && !w->state->enabled) return;

    if (!force && w->state && w->state->checked == state) return;

    if (w->state) w->state->checked = state;

    /* Overwrite just the emoji indicator */
    tui_place_goto(&w->place, w->place.col, NULL, NULL);
    ansi_puts(state ? "[green]:check:[/]" : "[red]:cross:[/]");
}

void tui_check_toggle(const tui_check_t *w)
{
    if (!w || !w->state) return;
    tui_check_update(w, !w->state->checked, 1);
}

void tui_check_enable(const tui_check_t *w, int enabled)
{
    if (!w || !w->state) return;
    w->state->enabled = enabled;

    int iw = w->width > 0 ? w->width : check_interior_width(w);
    const char *color = enabled ? w->place.color : "dim";
    tui_widget_chrome(&w->place, w->place.col, iw, color, NULL, NULL);
    if (enabled) {
        /* Restore from stored state */
        ansi_puts(w->state->checked ? "[green]:check:[/]" : "[red]:cross:[/]");
    } else {
        /* Dim indicator */
        ansi_puts("[dim]:cross:[/]");
    }
    if (w->label) {
        ansi_puts(" ");
        if (!enabled)
            ansi_print("[dim]%s[/]", w->label);
        else
            ansi_puts(w->label);
    }
}

#endif /* ANSI_TUI_CHECK */

/* ------------------------------------------------------------------ */
/* Metric widget                                                       */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_METRIC

/** Determine which zone a value falls in. */
static int metric_zone(const tui_metric_t *w, double value)
{
    if (value < w->thresh_lo) return -1;
    if (value > w->thresh_hi) return  1;
    return 0;
}

/** Return the color string for a given zone. */
static const char *metric_color(const tui_metric_t *w, int zone)
{
    switch (zone) {
    case -1: return w->color_lo;
    case  1: return w->color_hi;
    default: return w->place.color;  /* nominal = place.color */
    }
}

/** Center a title on the top border row of a metric widget. */
static void metric_draw_title(const tui_metric_t *w,
                               int ar, int ac, const char *color)
{
    if (!w->title || !w->title[0]) return;
    int title_len = (int)strlen(w->title);
    int offset = (w->width + 2 - title_len - 2) / 2;  /* center in hz span */
    if (offset < 0) offset = 0;
    tui_goto(ar, ac + 1 + offset);
    if (color)
        ansi_print(" [bold %s]%s[/] ", color, w->title);
    else
        ansi_print(" [bold]%s[/] ", w->title);
}

/** Draw the metric value as colored foreground text, centered in the interior.
 *  Pads to width+2 chars at ac+1 to clear the full span between borders. */
static void metric_draw_value(const tui_metric_t *w, int ar, int ac,
                               double value, const char *zone_color)
{
    char vbuf[64];
    snprintf(vbuf, sizeof(vbuf), w->fmt, value);
    int vlen = (int)strlen(vbuf);
    int fill = w->width + 2;
    int left_pad = (fill - vlen) / 2;
    if (left_pad < 0) left_pad = 0;
    int right_pad = fill - vlen - left_pad;
    if (right_pad < 0) right_pad = 0;

    tui_goto(ar + 1, ac + 1);
    ansi_print("%*s[%s]%s[/]%*s",
               left_pad, "", zone_color, vbuf, right_pad, "");
}

void tui_metric_init(const tui_metric_t *w)
{
    if (!w) return;

    const char *color = w->place.color;
    if (w->state) {
        w->state->enabled = 1;
        w->state->value   = 0.0;
        w->state->zone    = 0;
    }

    int col = tui_center_col(w->place.col, w->place.parent, w->width, w->place.border);
    int ar, ac;
    tui_resolve(w->place.parent, w->place.row, col, &ar, &ac);

    tui_draw_border(ar, ac, w->width, 1, color, 0);
    metric_draw_title(w, ar, ac, color);

    /* Blank the interior */
    tui_goto(ar + 1, ac + 1);
    tui_pad(w->width + 2);
}

void tui_metric_update(const tui_metric_t *w, double value, int force)
{
    if (!w) return;
    if (w->state && !w->state->enabled) return;

    if (!force && w->state && w->state->value == value) return;

    int zone = metric_zone(w, value);
    const char *color = metric_color(w, zone);

    int col = tui_center_col(w->place.col, w->place.parent, w->width, w->place.border);
    int ar, ac;
    tui_resolve(w->place.parent, w->place.row, col, &ar, &ac);

    /* Redraw border + title if zone changed, forced, or no state */
    int need_border = force;
    if (w->state) {
        if (!need_border) need_border = (zone != w->state->zone);
        w->state->zone  = zone;
        w->state->value = value;
    } else {
        need_border = 1;
    }
    if (need_border) {
        tui_draw_border(ar, ac, w->width, 1, color, 0);
        metric_draw_title(w, ar, ac, color);
    }

    metric_draw_value(w, ar, ac, value, color);
}

void tui_metric_enable(const tui_metric_t *w, int enabled)
{
    if (!w || !w->state) return;
    w->state->enabled = enabled;

    int col = tui_center_col(w->place.col, w->place.parent, w->width, w->place.border);
    int ar, ac;
    tui_resolve(w->place.parent, w->place.row, col, &ar, &ac);

    if (enabled) {
        /* Force zone mismatch so update redraws border */
        w->state->zone = -2;
        tui_metric_update(w, w->state->value, 1);
    } else {
        tui_draw_border(ar, ac, w->width, 1, "dim", 0);
        metric_draw_title(w, ar, ac, "dim");
        tui_goto(ar + 1, ac + 1);
        tui_pad(w->width + 2);
    }
}

#endif /* ANSI_TUI_METRIC */
