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
 */

#ifndef ANSI_TUI_H
#define ANSI_TUI_H

#include "ansi_print.h"

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

/**
 * Row immediately below a single-line widget.
 *
 * Works with any widget pointer whose struct has @c row and @c border
 * fields (label, text, bar, status, check).  Bordered widgets occupy
 * 3 rows (top + content + bottom), unbordered occupy 1.
 *
 * @param wp  Pointer to a widget descriptor.
 * @return    Row number immediately below the widget.
 */
#define tui_below(wp) \
    ((wp)->row + ((wp)->border == ANSI_TUI_BORDER ? 3 : 1))

/* ------------------------------------------------------------------ */
/* Frame widget                                                        */
/* ------------------------------------------------------------------ */

/** Forward declaration for self-referential parent pointer. */
typedef struct tui_frame tui_frame_t;

/**
 * Frame widget: a pure border box with no content.
 *
 * Use as an outer container — draw the frame first, then place child
 * widgets inside with relative coordinates.  Child (1,1) maps to the
 * first interior cell.  Frames can nest: set @c parent to the
 * enclosing frame, or NULL for absolute screen positioning.
 * Width and height include the border characters.
 */
struct tui_frame {
    int         row;    /**< Row position (1-based, relative to parent). */
    int         col;    /**< Column position (1-based, relative to parent). */
    int         width;  /**< Total width including border (min 5). */
    int         height; /**< Total height including border (min 3). */
    const char *title;  /**< Optional title on top border, or NULL. */
    const char *color;  /**< Border color name, or NULL. */
    const tui_frame_t *parent; /**< Parent frame, or NULL for absolute. */
};

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

/* ------------------------------------------------------------------ */
/* Widget types                                                        */
/* ------------------------------------------------------------------ */

/** Widget border option. */
typedef enum {
    ANSI_TUI_NO_BORDER,    /**< No border drawn. */
    ANSI_TUI_BORDER        /**< Box border using ANSI_PRINT_BOX_STYLE. */
} tui_border_t;

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
    int                row;     /**< Row (1-based, relative to parent). */
    int                col;     /**< Column (1-based, relative to parent). */
    int                width;   /**< Value area width in visible chars. */
    tui_border_t  border;  /**< Border option. */
    const char        *label;   /**< Label text (e.g. "CPU"). */
    const char        *color;   /**< Border/label color name, or NULL. */
    tui_label_state_t *state; /**< Mutable state in RAM, or NULL. */
    const tui_frame_t *parent; /**< Parent frame, or NULL for absolute. */
} tui_label_t;

#if ANSI_PRINT_BAR
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
    int                row;          /**< Row (1-based, relative to parent). */
    int                col;          /**< Column (1-based, relative to parent). */
    int                bar_width;    /**< Bar width in character cells. */
    tui_border_t  border;       /**< Border option. */
    const char        *label;        /**< Optional label prefix, or NULL. */
    const char        *color;        /**< Bar fill color name. */
    ansi_bar_track_t   track;        /**< Track character for unfilled cells. */
    char              *bar_buf;      /**< Caller-provided buffer for ansi_bar(). */
    size_t             bar_buf_size; /**< Size of bar_buf in bytes. */
    tui_bar_state_t *state;     /**< Mutable state in RAM, or NULL. */
    const tui_frame_t *parent;  /**< Parent frame, or NULL for absolute. */
} tui_bar_t;
#endif

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
    int                row;     /**< Row (1-based, relative to parent). */
    int                col;     /**< Column (1-based, relative to parent). */
    int                width;   /**< Interior width in visible chars. */
    tui_border_t  border;  /**< Border option. */
    const char        *color;   /**< Border color name, or NULL. */
    tui_status_state_t *state; /**< Mutable state in RAM, or NULL. */
    const tui_frame_t *parent; /**< Parent frame, or NULL for absolute. */
} tui_status_t;

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
    int                row;     /**< Row (1-based, relative to parent). */
    int                col;     /**< Column (1-based, relative to parent). */
    int                width;   /**< Visible chars, or -1 to fill parent. */
    tui_border_t       border;  /**< Border option. */
    const char        *color;   /**< Border color name, or NULL. */
    tui_text_state_t  *state;   /**< Mutable state in RAM, or NULL. */
    const tui_frame_t *parent;  /**< Parent frame, or NULL for absolute. */
} tui_text_t;

#if ANSI_PRINT_EMOJI
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
    int                row;     /**< Row (1-based, relative to parent). */
    int                col;     /**< Column (1-based, relative to parent). */
    int                width;   /**< Total interior width in visible chars. */
    tui_border_t  border;  /**< Border option. */
    const char        *label;   /**< Label text (e.g. "Current State"). */
    const char        *color;   /**< Border color name, or NULL. */
    tui_check_state_t *state; /**< Mutable state in RAM, or NULL. */
    const tui_frame_t *parent; /**< Parent frame, or NULL for absolute. */
} tui_check_t;
#endif

/* ------------------------------------------------------------------ */
/* Label widget API                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Draw a label widget's chrome (border and label text).
 *
 * Draws the optional border and "Label: " prefix.  The value area
 * is filled with spaces.  Call tui_label_update() to set values.
 * The widget descriptor is not modified — it may be declared const
 * (e.g. in flash on embedded targets).
 *
 * @param w  Widget descriptor (const, may live in flash).
 */
void tui_label_init(const tui_label_t *w);

/**
 * @brief Update only the value portion of a label widget.
 *
 * Positions the cursor at the value area and overwrites with new
 * text, padded to width to clear any previous content.  The format
 * string may contain Rich markup tags.
 *
 * @param w    Widget descriptor.
 * @param fmt  printf-style format string (may contain Rich markup).
 */
void tui_label_update(const tui_label_t *w,
                           const char *fmt, ...);

/**
 * @brief Enable or disable a label widget.
 *
 * When disabled, the widget redraws in dim and ignores updates.
 * Requires w->state to be non-NULL.
 *
 * @param w        Widget descriptor.
 * @param enabled  Nonzero to enable, 0 to disable.
 */
void tui_label_enable(const tui_label_t *w, int enabled);

/* ------------------------------------------------------------------ */
/* Bar widget API                                                      */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_BAR
/**
 * @brief Draw a bar widget's chrome (border, label, empty track).
 *
 * The widget descriptor is not modified — it may be declared const
 * (e.g. in flash on embedded targets).
 *
 * @param w  Widget descriptor (const, may live in flash).
 */
void tui_bar_init(const tui_bar_t *w);

/**
 * @brief Update a bar widget with a new value.
 *
 * Calls ansi_bar() into the widget's own buffer, then positions the
 * cursor and renders the bar at the widget's screen location.
 *
 * @param w      Widget descriptor.
 * @param value  Current value to display.
 * @param min    Range minimum.
 * @param max    Range maximum.
 */
void tui_bar_update(const tui_bar_t *w,
                         double value, double min, double max);

/**
 * @brief Enable or disable a bar widget.
 *
 * When disabled, the widget redraws in dim and ignores updates.
 * Requires w->state to be non-NULL.
 *
 * @param w        Widget descriptor.
 * @param enabled  Nonzero to enable, 0 to disable.
 */
void tui_bar_enable(const tui_bar_t *w, int enabled);
#endif

/* ------------------------------------------------------------------ */
/* Status widget API                                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Draw a status widget's chrome (border and empty interior).
 *
 * The widget descriptor is not modified — it may be declared const
 * (e.g. in flash on embedded targets).
 *
 * @param w  Widget descriptor (const, may live in flash).
 */
void tui_status_init(const tui_status_t *w);

/**
 * @brief Update a status widget with new text.
 *
 * Positions cursor at the interior and overwrites with new text,
 * padded to width.  The format string may contain Rich markup tags.
 *
 * @param w    Widget descriptor.
 * @param fmt  printf-style format string (may contain Rich markup).
 */
void tui_status_update(const tui_status_t *w,
                            const char *fmt, ...);

/**
 * @brief Enable or disable a status widget.
 *
 * When disabled, the widget redraws in dim and ignores updates.
 * Requires w->state to be non-NULL.
 *
 * @param w        Widget descriptor.
 * @param enabled  Nonzero to enable, 0 to disable.
 */
void tui_status_enable(const tui_status_t *w, int enabled);

/* ------------------------------------------------------------------ */
/* Text widget API                                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Draw a text widget's chrome (border and empty interior).
 *
 * The widget descriptor is not modified — it may be declared const
 * (e.g. in flash on embedded targets).
 *
 * @param w  Widget descriptor (const, may live in flash).
 */
void tui_text_init(const tui_text_t *w);

/**
 * @brief Update a text widget with new text.
 *
 * Positions cursor at the interior and overwrites with new text,
 * padded to width.  The format string may contain Rich markup tags.
 *
 * @param w    Widget descriptor.
 * @param fmt  printf-style format string (may contain Rich markup).
 */
void tui_text_update(const tui_text_t *w, const char *fmt, ...);

/**
 * @brief Enable or disable a text widget.
 *
 * When disabled, the widget redraws in dim and ignores updates.
 * Requires w->state to be non-NULL.
 *
 * @param w        Widget descriptor.
 * @param enabled  Nonzero to enable, 0 to disable.
 */
void tui_text_enable(const tui_text_t *w, int enabled);

/* ------------------------------------------------------------------ */
/* Check widget API                                                    */
/* ------------------------------------------------------------------ */

#if ANSI_PRINT_EMOJI
/**
 * @brief Draw a check widget's chrome and set its initial state.
 *
 * Draws the optional border, then renders the check/cross indicator
 * followed by the label text.  The label comes from w->label.
 * If w->width is 0, the interior width is auto-computed from the label.
 *
 * The widget descriptor is not modified — it may be declared const
 * (e.g. in flash on embedded targets).
 *
 * @param w       Widget descriptor (const, may live in flash).
 * @param state   Initial boolean state (true = check, false = cross).
 */
void tui_check_init(const tui_check_t *w, int state);

/**
 * @brief Update a check widget's boolean state.
 *
 * Repositions the cursor and overwrites the indicator emoji.
 * The label text is not redrawn.  If w->state is non-NULL, the
 * new value is stored there.
 *
 * @param w       Widget descriptor.
 * @param state   New boolean state (true = check, false = cross).
 */
void tui_check_update(const tui_check_t *w, int state);

/**
 * @brief Toggle a check widget's boolean state.
 *
 * Requires w->state to be non-NULL.  Flips the stored value and
 * redraws the indicator.
 *
 * @param w  Widget descriptor.
 */
void tui_check_toggle(const tui_check_t *w);

/**
 * @brief Enable or disable a check widget.
 *
 * When disabled, the widget redraws in dim and ignores updates.
 * Requires w->state to be non-NULL.
 *
 * @param w        Widget descriptor.
 * @param enabled  Nonzero to enable, 0 to disable.
 */
void tui_check_enable(const tui_check_t *w, int enabled);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ANSI_TUI_H */
