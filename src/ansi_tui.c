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
/* Box-drawing characters (duplicated from ansi_print.c)               */
/* The BOX_* macros in ansi_print.c are file-scoped, so the TUI layer */
/* defines its own set using the same ANSI_PRINT_BOX_STYLE flag.      */
/* ------------------------------------------------------------------ */

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
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

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
 *  by walking the parent frame chain.  NULL parent = no offset. */
static void tui_resolve(const tui_frame_t *parent, int row, int col,
                         int *abs_row, int *abs_col)
{
    while (parent) {
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

/** Emit n spaces at the current cursor position. */
static void tui_pad(int n)
{
    if (n <= 0) return;
    ansi_print("%*s", n, "");
}

/* ------------------------------------------------------------------ */
/* Frame widget                                                        */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/* Label widget                                                        */
/* ------------------------------------------------------------------ */

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

    int ar, ac;
    tui_resolve(w->parent, w->row, w->col, &ar, &ac);
    int iw = label_interior_width(w);

    /* Draw border if requested */
    if (w->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, iw, 1, w->color, 1);

    /* Draw label text at interior position */
    tui_widget_goto(w->parent, w->row, w->col, w->border, NULL, NULL);
    if (w->label) {
        if (w->color)
            ansi_print("[%s]%s: [/]", w->color, w->label);
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
    tui_widget_goto(w->parent, w->row, w->col, w->border, &ir, &ic);
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

    int ar, ac;
    tui_resolve(w->parent, w->row, w->col, &ar, &ac);
    int iw = label_interior_width(w);
    const char *color = enabled ? w->color : "dim";

    if (w->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, iw, 1, color, 1);

    tui_widget_goto(w->parent, w->row, w->col, w->border, NULL, NULL);
    if (w->label) {
        if (enabled && w->color)
            ansi_print("[%s]%s: [/]", w->color, w->label);
        else if (!enabled)
            ansi_print("[dim]%s: [/]", w->label);
        else
            ansi_print("%s: ", w->label);
    }
    /* Blank the value area (clears stale content when disabling) */
    tui_pad(w->width);
}

/* ------------------------------------------------------------------ */
/* Bar widget                                                          */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_BAR

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

    int ar, ac;
    tui_resolve(w->parent, w->row, w->col, &ar, &ac);
    int iw = bar_interior_width(w);

    if (w->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, iw, 1, w->color, 1);

    /* Draw label if present */
    tui_widget_goto(w->parent, w->row, w->col, w->border, NULL, NULL);
    if (w->label) ansi_puts(w->label);

    /* Draw empty bar (value = min = 0) */
    tui_bar_update(w, 0.0, 0.0, 100.0);
}

void tui_bar_update(const tui_bar_t *w,
                         double value, double min, double max)
{
    if (!w || !w->bar_buf) return;
    if (w->state && !w->state->enabled) return;

    if (w->state) {
        w->state->value = value;
        w->state->min   = min;
        w->state->max   = max;
    }

    ansi_bar(w->bar_buf, w->bar_buf_size,
             w->color, w->bar_width, w->track,
             value, min, max);

    /* Position cursor at bar area (after label) */
    int ir, ic;
    tui_widget_goto(w->parent, w->row, w->col, w->border, &ir, &ic);
    int label_len = w->label ? (int)strlen(w->label) : 0;

    tui_goto(ir, ic + label_len);
    ansi_print("%s", w->bar_buf);
}

void tui_bar_enable(const tui_bar_t *w, int enabled)
{
    if (!w || !w->state) return;
    w->state->enabled = enabled;

    int ar, ac;
    tui_resolve(w->parent, w->row, w->col, &ar, &ac);
    int iw = bar_interior_width(w);
    const char *color = enabled ? w->color : "dim";

    if (w->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, iw, 1, color, 1);

    int ir, ic;
    tui_widget_goto(w->parent, w->row, w->col, w->border, &ir, &ic);
    if (w->label) {
        if (!enabled)
            ansi_print("[dim]%s[/]", w->label);
        else
            ansi_puts(w->label);
    }

    if (enabled) {
        /* Re-render the bar with stored values */
        w->state->enabled = 1;   /* allow update through */
        tui_bar_update(w, w->state->value, w->state->min, w->state->max);
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

#endif /* ANSI_PRINT_BAR */

/* ------------------------------------------------------------------ */
/* Status widget                                                       */
/* ------------------------------------------------------------------ */

void tui_status_init(const tui_status_t *w)
{
    if (!w) return;

    if (w->state) w->state->enabled = 1;

    int ar, ac;
    tui_resolve(w->parent, w->row, w->col, &ar, &ac);

    if (w->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, w->width, 1, w->color, 1);

    /* Blank the interior */
    tui_widget_goto(w->parent, w->row, w->col, w->border, NULL, NULL);
    tui_pad(w->width);
}

void tui_status_update(const tui_status_t *w,
                            const char *fmt, ...)
{
    if (!w || !fmt) return;
    if (w->state && !w->state->enabled) return;

    int ir, ic;
    tui_widget_goto(w->parent, w->row, w->col, w->border, &ir, &ic);

    /* Clear interior first, then reposition and write new text */
    tui_pad(w->width);

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

    int ar, ac;
    tui_resolve(w->parent, w->row, w->col, &ar, &ac);
    const char *color = enabled ? w->color : "dim";

    if (w->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, w->width, 1, color, 1);

    /* Blank interior when disabling */
    tui_widget_goto(w->parent, w->row, w->col, w->border, NULL, NULL);
    tui_pad(w->width);
}

/* ------------------------------------------------------------------ */
/* Text widget                                                         */
/* ------------------------------------------------------------------ */

/** Compute the effective width for a text widget.
 *  If width >= 0, returns width as-is.
 *  If width == -1 and parent is set, fills to parent's right edge. */
static int text_effective_width(const tui_text_t *w)
{
    if (w->width >= 0) return w->width;
    if (!w->parent) return 0;
    /* 4 = frame border overhead: border char + space on each side */
    int piw = w->parent->width - 4;
    /* 4 = widget's own border overhead (same: border + space × 2) */
    int bdr = (w->border == ANSI_TUI_BORDER) ? 4 : 0;
    int eff = piw - (w->col - 1) - bdr;
    return eff > 0 ? eff : 0;
}

void tui_text_init(const tui_text_t *w)
{
    if (!w) return;

    if (w->state) w->state->enabled = 1;

    int ew = text_effective_width(w);
    int ar, ac;
    tui_resolve(w->parent, w->row, w->col, &ar, &ac);

    if (w->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, ew, 1, w->color, 1);

    /* Blank the interior */
    tui_widget_goto(w->parent, w->row, w->col, w->border, NULL, NULL);
    tui_pad(ew);
}

void tui_text_update(const tui_text_t *w, const char *fmt, ...)
{
    if (!w || !fmt) return;
    if (w->state && !w->state->enabled) return;

    int ew = text_effective_width(w);
    int ir, ic;
    tui_widget_goto(w->parent, w->row, w->col, w->border, &ir, &ic);

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

    int ew = text_effective_width(w);
    int ar, ac;
    tui_resolve(w->parent, w->row, w->col, &ar, &ac);
    const char *color = enabled ? w->color : "dim";

    if (w->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, ew, 1, color, 1);

    /* Blank interior when disabling */
    tui_widget_goto(w->parent, w->row, w->col, w->border, NULL, NULL);
    tui_pad(ew);
}

/* ------------------------------------------------------------------ */
/* Check widget                                                        */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_EMOJI

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

    int ar, ac;
    tui_resolve(w->parent, w->row, w->col, &ar, &ac);
    int iw = w->width > 0 ? w->width : check_interior_width(w);

    if (w->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, iw, 1, w->color, 1);

    /* Draw indicator + label */
    tui_widget_goto(w->parent, w->row, w->col, w->border, NULL, NULL);
    ansi_puts(state ? "[green]:check:[/]" : "[red]:cross:[/]");
    if (w->label) {
        ansi_puts(" ");
        ansi_puts(w->label);
    }
}

void tui_check_update(const tui_check_t *w, int state)
{
    if (!w) return;
    if (w->state && !w->state->enabled) return;

    if (w->state) w->state->checked = state;

    /* Overwrite just the emoji indicator */
    tui_widget_goto(w->parent, w->row, w->col, w->border, NULL, NULL);
    ansi_puts(state ? "[green]:check:[/]" : "[red]:cross:[/]");
}

void tui_check_toggle(const tui_check_t *w)
{
    if (!w || !w->state) return;
    tui_check_update(w, !w->state->checked);
}

void tui_check_enable(const tui_check_t *w, int enabled)
{
    if (!w || !w->state) return;
    w->state->enabled = enabled;

    int ar, ac;
    tui_resolve(w->parent, w->row, w->col, &ar, &ac);
    int iw = w->width > 0 ? w->width : check_interior_width(w);
    const char *color = enabled ? w->color : "dim";

    if (w->border == ANSI_TUI_BORDER)
        tui_draw_border(ar, ac, iw, 1, color, 1);

    tui_widget_goto(w->parent, w->row, w->col, w->border, NULL, NULL);
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

#endif /* ANSI_PRINT_EMOJI */
