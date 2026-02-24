/**
 * @file ansi_tui.h
 * @brief Positioned TUI widget layer built on ansi_print.
 *
 * Provides draw-once chrome with update-only values for fixed-position
 * terminal widgets.  Uses cursor addressing (ESC[row;colH) to place
 * widgets at absolute screen positions.  No terminal size detection --
 * the caller sizes their terminal to fit the layout.
 *
 * Depends on ansi_print.h for output and Rich markup processing.
 * Call ansi_init() first, then tui_init() before any widget ops.
 *
 * To use: compile ansi_tui.c alongside ansi_print.c.
 * To omit: simply do not compile ansi_tui.c.
 *
 * @section tui_flags Compile-Time Feature Flags
 * Each TUI widget type can be individually enabled or disabled.
 * All default to enabled (1), or disabled (0) when ANSI_PRINT_MINIMAL
 * is defined.  Set via -D flags or app_cfg.h.
 *
 * | Flag             | Default | Description                              |
 * |------------------|---------|------------------------------------------|
 * | ANSI_TUI_FRAME   | 1       | Frame container (border box)             |
 * | ANSI_TUI_LABEL   | 1       | Label widget ("Name: value")             |
 * | ANSI_TUI_BAR     | 1       | Bar graph widget (requires ANSI_PRINT_BAR) |
 * | ANSI_TUI_STATUS  | 1       | Status text field                        |
 * | ANSI_TUI_TEXT    | 1       | Generic text widget                      |
 * | ANSI_TUI_CHECK   | 1       | Check/cross indicator (requires ANSI_PRINT_EMOJI) |
 * | ANSI_TUI_METRIC  | 1       | Threshold-based metric gauge             |
 */

#ifndef ANSI_TUI_H
#define ANSI_TUI_H

#include "ansi_print.h"

/* ------------------------------------------------------------------ */
/* TUI feature flags                                                   */
/* ------------------------------------------------------------------ */

/** @def ANSI_TUI_FRAME
 *  Enable the frame container widget. Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_TUI_FRAME
#  define ANSI_TUI_FRAME    ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_TUI_LABEL
 *  Enable the label widget. Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_TUI_LABEL
#  define ANSI_TUI_LABEL    ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_TUI_BAR
 *  Enable the bar graph widget. Requires ANSI_PRINT_BAR for ansi_bar().
 *  Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_TUI_BAR
#  define ANSI_TUI_BAR      ANSI_PRINT_DEFAULT_
#endif
/* Force off if the underlying bar renderer is disabled */
#if ANSI_TUI_BAR && !ANSI_PRINT_BAR
#  undef  ANSI_TUI_BAR
#  define ANSI_TUI_BAR      0
#endif

/** @def ANSI_TUI_STATUS
 *  Enable the status text widget. Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_TUI_STATUS
#  define ANSI_TUI_STATUS   ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_TUI_TEXT
 *  Enable the generic text widget. Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_TUI_TEXT
#  define ANSI_TUI_TEXT     ANSI_PRINT_DEFAULT_
#endif

/** @def ANSI_TUI_CHECK
 *  Enable the check/cross indicator widget. Requires ANSI_PRINT_EMOJI.
 *  Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_TUI_CHECK
#  define ANSI_TUI_CHECK    ANSI_PRINT_DEFAULT_
#endif
/* Force off if emoji support is disabled */
#if ANSI_TUI_CHECK && !ANSI_PRINT_EMOJI
#  undef  ANSI_TUI_CHECK
#  define ANSI_TUI_CHECK    0
#endif

/** @def ANSI_TUI_METRIC
 *  Enable the threshold-based metric gauge widget.
 *  Default: 1 (0 if ANSI_PRINT_MINIMAL). */
#ifndef ANSI_TUI_METRIC
#  define ANSI_TUI_METRIC   ANSI_PRINT_DEFAULT_
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Screen helpers                                                      */
/* ------------------------------------------------------------------ */

/** Clear the entire screen and move cursor to home (1,1). */
void tui_cls(void);

/** Move cursor to an absolute position (1-based row and col). */
void tui_goto(int row, int col);

/** Hide the terminal cursor (ESC[?25l). */
void tui_cursor_hide(void);

/** Show the terminal cursor (ESC[?25h). */
void tui_cursor_show(void);

/* ------------------------------------------------------------------ */
/* Common types (always available — used by macros and parent ptrs)     */
/* ------------------------------------------------------------------ */

/** Widget border option. */
typedef enum {
    ANSI_TUI_NO_BORDER,    /**< No border drawn. */
    ANSI_TUI_BORDER        /**< Box border using ANSI_PRINT_BOX_STYLE. */
} tui_border_t;

/** Forward declaration for self-referential parent pointer. */
typedef struct tui_frame tui_frame_t;

/**
 * Frame descriptor: a pure border box with no content.
 *
 * Use as an outer container — draw the frame first, then place child
 * widgets inside with relative coordinates.  Child (1,1) maps to the
 * first interior cell.  Frames can nest: set @c parent to the
 * enclosing frame, or NULL for absolute screen positioning.
 * Width and height include the border characters.
 *
 * The struct is always defined because all widget types reference it
 * via their @c parent pointer.  Set ANSI_TUI_FRAME=1 to enable
 * tui_frame_init().
 */
struct tui_frame {
    int         row;    /**< Row (1-based relative to parent; negative = from end). */
    int         col;    /**< Column (1-based relative to parent; negative = from end). */
    int         width;  /**< Total width including border (min 5). */
    int         height; /**< Total height including border (min 3). */
    const char *title;  /**< Optional title on top border, or NULL. */
    const char *color;  /**< Border color name, or NULL. */
    const tui_frame_t *parent; /**< Parent frame, or NULL for absolute. */
};

/**
 * Common positioning fields shared by all content widgets.
 *
 * Every content widget (label, bar, status, text, check, metric) embeds
 * a @c tui_placement_t as its first member.  This lets shared helper
 * functions operate on any widget's placement without knowing the
 * widget type.
 */
typedef struct {
    int                row;    /**< Row (1-based; negative = from end of parent). */
    int                col;    /**< Column (1-based; neg = from end; 0 = center). */
    tui_border_t       border; /**< Border option. */
    const char        *color;  /**< Border/content color name, or NULL. */
    const tui_frame_t *parent; /**< Parent frame, or NULL for absolute. */
} tui_placement_t;

/* ------------------------------------------------------------------ */
/* Layout helper macros                                                */
/* ------------------------------------------------------------------ */

/**
 * Row immediately below a single-line widget.
 *
 * Works with any content widget pointer whose struct embeds a
 * @c tui_placement_t as its @c place member (label, text, bar,
 * status, check, metric).  Bordered widgets occupy 3 rows
 * (top + content + bottom), unbordered occupy 1.
 *
 * @param wp  Pointer to a widget descriptor.
 * @return    Row number immediately below the widget.
 */
#define tui_below(wp) \
    ((wp)->place.row + ((wp)->place.border == ANSI_TUI_BORDER ? 3 : 1))

/**
 * Column immediately to the right of a widget.
 *
 * Works with widget types whose @c width field equals the interior
 * width: text, status, and metric.  Bordered widgets add 4 columns
 * of overhead (border char + padding space on each side).
 *
 * @note Not suitable for label (width is value-area only) or bar
 *       (uses bar_width, not width).  For frame, use
 *       @c (fp)->col + (fp)->width since frame width is already total.
 *
 * @param wp  Pointer to a widget descriptor.
 * @return    Column number immediately to the right of the widget.
 */
#define tui_right(wp) \
    ((wp)->place.col + (wp)->width + \
     ((wp)->place.border == ANSI_TUI_BORDER ? 4 : 0))

/* ------------------------------------------------------------------ */
/* Frame widget API                                                    */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_FRAME
/**
 * @brief Draw a frame border at the specified position.
 *
 * Draws a box outline of the given size.  The interior is not filled —
 * call tui_cls() first to clear the screen.  The descriptor is
 * not modified and may live in flash.
 *
 * @param f  Frame descriptor (const, may live in flash).
 */
void tui_frame_init(const tui_frame_t *f);
#endif

/* ------------------------------------------------------------------ */
/* Label widget                                                        */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_LABEL

/** Mutable state for a label widget (lives in RAM). */
typedef struct {
    int enabled;    /**< Nonzero = active, 0 = disabled (drawn dim). */
} tui_label_state_t;

/**
 * Label widget: "Label: value" at a fixed screen position.
 *
 * @c width is the number of visible characters reserved for the
 * value area (after the "label: " prefix).  On update, the value
 * is padded or truncated to this width.
 */
typedef struct {
    tui_placement_t    place;   /**< Common positioning (row, col, border, color, parent). */
    int                width;   /**< Value area width in visible chars. */
    const char        *label;   /**< Label text (e.g. "CPU"). */
    tui_label_state_t *state;   /**< Mutable state in RAM, or NULL. */
} tui_label_t;

void tui_label_init(const tui_label_t *w);
void tui_label_update(const tui_label_t *w, const char *fmt, ...);
void tui_label_enable(const tui_label_t *w, int enabled);

#endif /* ANSI_TUI_LABEL */

/* ------------------------------------------------------------------ */
/* Bar widget                                                          */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_BAR

/** Mutable state for a bar widget (lives in RAM). */
typedef struct {
    int    enabled; /**< Nonzero = active, 0 = disabled (drawn dim). */
    double value;   /**< Current value. */
    double min;     /**< Current range minimum. */
    double max;     /**< Current range maximum. */
} tui_bar_state_t;

/**
 * Bar widget: positioned bar graph using ansi_bar().
 *
 * The widget calls ansi_bar() into the caller-provided @c bar_buf
 * on every update.  Multiple bar widgets need separate buffers.
 * The const descriptor may live in flash; mutable state is stored
 * via the @c state pointer.
 */
typedef struct {
    tui_placement_t    place;        /**< Common positioning (row, col, border, color, parent). */
    int                bar_width;    /**< Bar width in character cells. */
    const char        *label;        /**< Optional label prefix, or NULL. */
    ansi_bar_track_t   track;        /**< Track character for unfilled cells. */
    char              *bar_buf;      /**< Caller-provided buffer for ansi_bar(). */
    size_t             bar_buf_size; /**< Size of bar_buf in bytes. */
    tui_bar_state_t   *state;        /**< Mutable state in RAM, or NULL. */
} tui_bar_t;

void tui_bar_init(const tui_bar_t *w);
void tui_bar_update(const tui_bar_t *w, double value, double min, double max,
                    int force);
void tui_bar_enable(const tui_bar_t *w, int enabled);

#endif /* ANSI_TUI_BAR */

/* ------------------------------------------------------------------ */
/* Status widget                                                       */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_STATUS

/** Mutable state for a status widget (lives in RAM). */
typedef struct {
    int enabled;    /**< Nonzero = active, 0 = disabled (drawn dim). */
} tui_status_state_t;

/**
 * Status widget: single-line text field at a fixed screen position.
 *
 * On update, the entire width is overwritten (padded with spaces).
 * Supports Rich markup tags in the update text.
 */
typedef struct {
    tui_placement_t      place;  /**< Common positioning (row, col, border, color, parent). */
    int                  width;  /**< Visible chars, or -1 to fill parent. */
    tui_status_state_t  *state;  /**< Mutable state in RAM, or NULL. */
} tui_status_t;

void tui_status_init(const tui_status_t *w);
void tui_status_update(const tui_status_t *w, const char *fmt, ...);
void tui_status_enable(const tui_status_t *w, int enabled);

#endif /* ANSI_TUI_STATUS */

/* ------------------------------------------------------------------ */
/* Text widget                                                         */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_TEXT

/** Mutable state for a text widget (lives in RAM). */
typedef struct {
    int enabled;    /**< Nonzero = active, 0 = disabled (drawn dim). */
} tui_text_state_t;

/**
 * Text widget: single line of text at a fixed screen position.
 *
 * On update, the entire width is overwritten (padded with spaces),
 * then the new text is written.  Supports Rich markup tags.
 * If @c width is -1 and @c parent is non-NULL, the width auto-fills
 * from the widget column to the parent frame's right interior edge.
 */
typedef struct {
    tui_placement_t    place;   /**< Common positioning (row, col, border, color, parent). */
    int                width;   /**< Visible chars, or -1 to fill parent. */
    tui_text_state_t  *state;   /**< Mutable state in RAM, or NULL. */
} tui_text_t;

void tui_text_init(const tui_text_t *w);
void tui_text_update(const tui_text_t *w, const char *fmt, ...);
void tui_text_enable(const tui_text_t *w, int enabled);

#endif /* ANSI_TUI_TEXT */

/* ------------------------------------------------------------------ */
/* Check widget                                                        */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_CHECK

/** Mutable state for a check widget (lives in RAM). */
typedef struct {
    int enabled;    /**< Nonzero = active, 0 = disabled (drawn dim). */
    int checked;    /**< Current boolean state (0 = cross, nonzero = check). */
} tui_check_state_t;

/**
 * Check widget: boolean indicator with a label.
 *
 * Displays ":check:" (green) or ":cross:" (red) followed by a label string.
 * The const descriptor may live in flash; mutable state is stored via the
 * @c state pointer.  If @c state is non-NULL, update/toggle functions store
 * the current value, enabling tui_check_toggle().
 */
typedef struct {
    tui_placement_t    place;   /**< Common positioning (row, col, border, color, parent). */
    int                width;   /**< Total interior width in visible chars. */
    const char        *label;   /**< Label text (e.g. "Current State"). */
    tui_check_state_t *state;   /**< Mutable state in RAM, or NULL. */
} tui_check_t;

void tui_check_init(const tui_check_t *w, int state);
void tui_check_update(const tui_check_t *w, int state, int force);
void tui_check_toggle(const tui_check_t *w);
void tui_check_enable(const tui_check_t *w, int enabled);

#endif /* ANSI_TUI_CHECK */

/* ------------------------------------------------------------------ */
/* Metric widget                                                       */
/* ------------------------------------------------------------------ */

#if ANSI_TUI_METRIC

/** Mutable state for a metric widget (lives in RAM). */
typedef struct {
    int    enabled;  /**< Nonzero = active, 0 = disabled (drawn dim). */
    double value;    /**< Current value (for restore on enable). */
    int    zone;     /**< -1=lo, 0=nom, 1=hi (tracks zone for border redraw). */
} tui_metric_state_t;

/**
 * Metric widget: bordered gauge with threshold-based color coding.
 *
 * Displays a formatted floating-point value centered inside a
 * bordered box.  The border and value text color change based on
 * three zones: below @c thresh_lo (low), between thresholds
 * (nominal), and above @c thresh_hi (high).
 *
 * The widget always draws a border; @c border should be set to
 * @c ANSI_TUI_BORDER for @c tui_below() compatibility.  The title
 * is centered on the top border.
 */
typedef struct {
    tui_placement_t      place;      /**< Common positioning; place.color = nominal color. */
    int                  width;      /**< Interior width in visible chars. */
    const char          *title;      /**< Centered title on top border. */
    const char          *fmt;        /**< printf format, e.g. "%0.2f m/sec". */
    const char          *color_lo;   /**< Border/text color when value < thresh_lo. */
    const char          *color_hi;   /**< Border/text color when value > thresh_hi. */
    double               thresh_lo;  /**< Low threshold. */
    double               thresh_hi;  /**< High threshold. */
    tui_metric_state_t  *state;      /**< Mutable state in RAM, or NULL. */
} tui_metric_t;

void tui_metric_init(const tui_metric_t *w);
void tui_metric_update(const tui_metric_t *w, double value, int force);
void tui_metric_enable(const tui_metric_t *w, int enabled);

#endif /* ANSI_TUI_METRIC */

#ifdef __cplusplus
}
#endif

#endif /* ANSI_TUI_H */
