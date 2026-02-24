/**
 * @file ansi_print.c
 * @brief ANSI color printing helpers with Rich-inspired inline markup
 *        suitable for embedded C environments.
 *
 * Supports named colors, 256-color codes, and style attributes.
 */

#include "ansi_print.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>         /* _isatty, _fileno */
#elif defined(__unix__) || defined(__APPLE__)
#include <unistd.h>     /* isatty, fileno */
#endif

#define RESET "\x1b[0m"

/* Style codes */
#define BOLD "\x1b[1m"
#define DIM "\x1b[2m"
#define ITALIC "\x1b[3m"
#define UNDERLINE "\x1b[4m"
#define INVERT "\x1b[7m"
#define STRIKETHROUGH "\x1b[9m"

/* Box-drawing characters (UTF-8 byte sequences) for ansi_banner/window.
   Style selected at compile time via ANSI_PRINT_BOX_STYLE. */
#if ANSI_PRINT_BOX_STYLE == ANSI_BOX_LIGHT
#define BOX_TOPLEFT     "\xe2\x94\x8c"  /* U+250C  ┌ */
#define BOX_TOPRIGHT    "\xe2\x94\x90"  /* U+2510  ┐ */
#define BOX_BOTTOMLEFT  "\xe2\x94\x94"  /* U+2514  └ */
#define BOX_BOTTOMRIGHT "\xe2\x94\x98"  /* U+2518  ┘ */
#define BOX_HORZ        "\xe2\x94\x80"  /* U+2500  ─ */
#define BOX_VERT        "\xe2\x94\x82"  /* U+2502  │ */
#define BOX_MIDLEFT     "\xe2\x94\x9c"  /* U+251C  ├ */
#define BOX_MIDRIGHT    "\xe2\x94\xa4"  /* U+2524  ┤ */
#elif ANSI_PRINT_BOX_STYLE == ANSI_BOX_HEAVY
#define BOX_TOPLEFT     "\xe2\x94\x8f"  /* U+250F  ┏ */
#define BOX_TOPRIGHT    "\xe2\x94\x93"  /* U+2513  ┓ */
#define BOX_BOTTOMLEFT  "\xe2\x94\x97"  /* U+2517  ┗ */
#define BOX_BOTTOMRIGHT "\xe2\x94\x9b"  /* U+251B  ┛ */
#define BOX_HORZ        "\xe2\x94\x81"  /* U+2501  ━ */
#define BOX_VERT        "\xe2\x94\x83"  /* U+2503  ┃ */
#define BOX_MIDLEFT     "\xe2\x94\xa3"  /* U+2523  ┣ */
#define BOX_MIDRIGHT    "\xe2\x94\xab"  /* U+252B  ┫ */
#elif ANSI_PRINT_BOX_STYLE == ANSI_BOX_DOUBLE
#define BOX_TOPLEFT     "\xe2\x95\x94"  /* U+2554  ╔ */
#define BOX_TOPRIGHT    "\xe2\x95\x97"  /* U+2557  ╗ */
#define BOX_BOTTOMLEFT  "\xe2\x95\x9a"  /* U+255A  ╚ */
#define BOX_BOTTOMRIGHT "\xe2\x95\x9d"  /* U+255D  ╝ */
#define BOX_HORZ        "\xe2\x95\x90"  /* U+2550  ═ */
#define BOX_VERT        "\xe2\x95\x91"  /* U+2551  ║ */
#define BOX_MIDLEFT     "\xe2\x95\xa0"  /* U+2560  ╠ */
#define BOX_MIDRIGHT    "\xe2\x95\xa3"  /* U+2563  ╣ */
#elif ANSI_PRINT_BOX_STYLE == ANSI_BOX_ROUNDED
#define BOX_TOPLEFT     "\xe2\x95\xad"  /* U+256D  ╭ */
#define BOX_TOPRIGHT    "\xe2\x95\xae"  /* U+256E  ╮ */
#define BOX_BOTTOMLEFT  "\xe2\x95\xb0"  /* U+2570  ╰ */
#define BOX_BOTTOMRIGHT "\xe2\x95\xaf"  /* U+256F  ╯ */
#define BOX_HORZ        "\xe2\x94\x80"  /* U+2500  ─ */
#define BOX_VERT        "\xe2\x94\x82"  /* U+2502  │ */
#define BOX_MIDLEFT     "\xe2\x94\x9c"  /* U+251C  ├ */
#define BOX_MIDRIGHT    "\xe2\x94\xa4"  /* U+2524  ┤ */
#else
#error "Unknown ANSI_PRINT_BOX_STYLE value"
#endif

#if ANSI_PRINT_BAR
/* Block elements for bar rendering (UTF-8 byte sequences) */
#define BAR_1_OF_8  "\xe2\x96\x8f"  /* U+258F  Left One Eighth Block    */
#define BAR_2_OF_8  "\xe2\x96\x8e"  /* U+258E  Left One Quarter Block   */
#define BAR_3_OF_8  "\xe2\x96\x8d"  /* U+258D  Left Three Eighths Block */
#define BAR_4_OF_8  "\xe2\x96\x8c"  /* U+258C  Left Half Block          */
#define BAR_5_OF_8  "\xe2\x96\x8b"  /* U+258B  Left Five Eighths Block  */
#define BAR_6_OF_8  "\xe2\x96\x8a"  /* U+258A  Left Three Quarters Block */
#define BAR_7_OF_8  "\xe2\x96\x89"  /* U+2589  Left Seven Eighths Block */
#define BAR_8_OF_8  "\xe2\x96\x88"  /* U+2588  Full Block               */
/** Block characters indexed by eighths (0 = none, 1–7 = partial, 8 = full) */
static const char * const m_bar_block[] = {
    NULL,        /* 0 -- no block */
    BAR_1_OF_8,  /* 1/8 */
    BAR_2_OF_8,  /* 2/8 */
    BAR_3_OF_8,  /* 3/8 */
    BAR_4_OF_8,  /* 4/8 */
    BAR_5_OF_8,  /* 5/8 */
    BAR_6_OF_8,  /* 6/8 */
    BAR_7_OF_8,  /* 7/8 */
    BAR_8_OF_8,  /* 8/8 */
};

/* Track characters indexed by ansi_bar_track_t.
 * Each entry: { utf8_string, byte_length } */
static const struct { const char *s; int len; } m_bar_track[] = {
    { " ",                 1 },  /* ANSI_BAR_BLANK */
    { "\xe2\x96\x91",     3 },  /* ANSI_BAR_LIGHT  ░ U+2591 */
    { "\xe2\x96\x92",     3 },  /* ANSI_BAR_MED    ▒ U+2592 */
    { "\xe2\x96\x93",     3 },  /* ANSI_BAR_HEAVY  ▓ U+2593 */
    { "\xc2\xb7",         2 },  /* ANSI_BAR_DOT    · U+00B7 */
    { "\xe2\x94\x80",     3 },  /* ANSI_BAR_LINE   ─ U+2500 */
};

#endif /* ANSI_PRINT_BAR */

/* ------------------------------------------------------------------------- */
/* Output plumbing                                                            */
/* ------------------------------------------------------------------------- */

static char   *m_buf      = NULL;
static size_t  m_buf_size = 0;


/* Default no-op functions */
static void ansi_noop_putc(int ch)  { (void)ch; }
static void ansi_noop_flush(void)   { }

static ansi_putc_function  m_putc_function  = ansi_noop_putc;
static ansi_flush_function m_flush_function = ansi_noop_flush;
static int m_color_enabled = 1;
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
static int m_no_color_lock = 0;  /* set by ansi_enable() when NO_COLOR env is present */
#endif

/** Emit a string by calling the user-provided putc function for each character */
static void output_string(const char *s)
{
    if (!s) return;
    while (*s) m_putc_function(*s++);
}

/* ------------------------------------------------------------------------- */
/* Style bitmask                                                              */
/* ------------------------------------------------------------------------- */

typedef uint8_t StyleMask;
#define STYLE_BOLD      (1u << 0)
#define STYLE_DIM       (1u << 1)
#define STYLE_ITALIC    (1u << 2)
#define STYLE_UNDERLINE (1u << 3)
#define STYLE_INVERT    (1u << 4)
#define STYLE_STRIKE    (1u << 5)
#define STYLE_RAINBOW   (1u << 6)
#define STYLE_GRADIENT  (1u << 7)

/* ------------------------------------------------------------------------- */
/* Color & style table                                                        */
/* ------------------------------------------------------------------------- */

typedef struct { uint8_t r, g, b; } RGB;

typedef struct {
    const char *name;
    uint8_t     len;
    const char *fg_code;
    const char *bg_code;
    StyleMask   style; /* 0 for colors, style bitmask for attributes */
    RGB         rgb;   /* for gradient interpolation; {0,0,0} for styles */
} AttrEntry;

/* Pre-compute name length at compile time so lookup can reject on length
   before calling memcmp — avoids O(table_size * name_len) per query. */
#define ATTR(n, fg, bg, sty, r, g, b)  { (n), sizeof(n)-1, (fg), (bg), (sty), {(r),(g),(b)} }

static const AttrEntry ATTRS[] = {
    /* Standard colors */
    ATTR("black",           "\x1b[30m",      "\x1b[40m",      0,    0,   0,   0),
    ATTR("red",             "\x1b[31m",      "\x1b[41m",      0,  255,   0,   0),
    ATTR("green",           "\x1b[32m",      "\x1b[42m",      0,    0, 205,   0),
    ATTR("yellow",          "\x1b[33m",      "\x1b[43m",      0,  255, 255,   0),
    ATTR("blue",            "\x1b[34m",      "\x1b[44m",      0,    0,   0, 255),
    ATTR("magenta",         "\x1b[35m",      "\x1b[45m",      0,  255,   0, 255),
    ATTR("cyan",            "\x1b[36m",      "\x1b[46m",      0,    0, 255, 255),
    ATTR("white",           "\x1b[37m",      "\x1b[47m",      0,  255, 255, 255),

#if ANSI_PRINT_EXTENDED_COLORS
    /* Extended colors */
    ATTR("orange",          "\x1b[38;5;208m", "\x1b[48;5;208m", 0, 255, 135,   0),
    ATTR("pink",            "\x1b[38;5;213m", "\x1b[48;5;213m", 0, 255, 135, 255),
    ATTR("purple",          "\x1b[38;5;93m",  "\x1b[48;5;93m",  0, 135,   0, 255),
    ATTR("brown",           "\x1b[38;5;94m",  "\x1b[48;5;94m",  0, 135,  95,   0),
    ATTR("teal",            "\x1b[38;5;37m",  "\x1b[48;5;37m",  0,   0, 175, 175),
    ATTR("lime",            "\x1b[38;5;118m", "\x1b[48;5;118m", 0, 135, 255,   0),
    ATTR("navy",            "\x1b[38;5;18m",  "\x1b[48;5;18m",  0,   0,   0, 135),
    ATTR("olive",           "\x1b[38;5;100m", "\x1b[48;5;100m", 0, 135, 135,   0),
    ATTR("maroon",          "\x1b[38;5;52m",  "\x1b[48;5;52m",  0,  95,   0,   0),
    ATTR("aqua",            "\x1b[38;5;51m",  "\x1b[48;5;51m",  0,   0, 255, 255),
    ATTR("silver",          "\x1b[38;5;250m", "\x1b[48;5;250m", 0, 188, 188, 188),
    ATTR("gray",            "\x1b[38;5;244m", "\x1b[48;5;244m", 0, 128, 128, 128),
#endif

#if ANSI_PRINT_BRIGHT_COLORS
    /* Bright variants */
    ATTR("bright_black",    "\x1b[90m",      "\x1b[100m",     0,  128, 128, 128),
    ATTR("bright_red",      "\x1b[91m",      "\x1b[101m",     0,  255,  85,  85),
    ATTR("bright_green",    "\x1b[92m",      "\x1b[102m",     0,   85, 255,  85),
    ATTR("bright_yellow",   "\x1b[93m",      "\x1b[103m",     0,  255, 255,  85),
    ATTR("bright_blue",     "\x1b[94m",      "\x1b[104m",     0,   85,  85, 255),
    ATTR("bright_magenta",  "\x1b[95m",      "\x1b[105m",     0,  255,  85, 255),
    ATTR("bright_cyan",     "\x1b[96m",      "\x1b[106m",     0,   85, 255, 255),
    ATTR("bright_white",    "\x1b[97m",      "\x1b[107m",     0,  255, 255, 255),
#endif

#if ANSI_PRINT_STYLES
    /* Styles (rgb unused - gradient rejects via style != 0) */
    ATTR("bold",            BOLD,          NULL, STYLE_BOLD,      0, 0, 0),
    ATTR("dim",             DIM,           NULL, STYLE_DIM,       0, 0, 0),
    ATTR("italic",          ITALIC,        NULL, STYLE_ITALIC,    0, 0, 0),
    ATTR("underline",       UNDERLINE,     NULL, STYLE_UNDERLINE, 0, 0, 0),
    ATTR("invert",          INVERT,        NULL, STYLE_INVERT,    0, 0, 0),
    ATTR("strikethrough",   STRIKETHROUGH, NULL, STYLE_STRIKE,    0, 0, 0),
#endif
#if ANSI_PRINT_GRADIENTS
    ATTR("rainbow",         NULL,          NULL, STYLE_RAINBOW,   0, 0, 0),
#endif
};

/* ------------------------------------------------------------------------- */
/* Emoji table (Rich-style :name: shortcodes)                                 */
/* ------------------------------------------------------------------------- */

#if ANSI_PRINT_EMOJI

typedef struct {
    const char *name;
    uint8_t     len;
    const char *utf8;
} EmojiEntry;

/* Pre-compute name length at compile time via sizeof so lookup can reject
   length mismatches immediately without calling strlen on every entry. */
#define EMOJI(n, u)  { (n), sizeof(n)-1, (u) }

/* Grouped by category for readability, not sorted alphabetically.
   Lookup is linear O(n) — fast enough for this table size. and
   the fact that we won't have that many emojis*/
static const EmojiEntry EMOJIS[] = {
    EMOJI("check",          "\xe2\x9c\x85"      ),  /* ✅ U+2705 */
    EMOJI("cross",          "\xe2\x9d\x8c"      ),  /* ❌ U+274C */
    EMOJI("warning",        "\xe2\x9a\xa0"      ),  /* ⚠  U+26A0 */
    EMOJI("info",           "\xe2\x84\xb9"      ),  /* ℹ  U+2139 */
    EMOJI("arrow",          "\xe2\x9e\xa1"      ),  /* ➡  U+27A1 */
    EMOJI("gear",           "\xe2\x9a\x99"      ),  /* ⚙  U+2699 */
    EMOJI("clock",          "\xe2\x8f\xb0"      ),  /* ⏰ U+23F0 */
    EMOJI("hourglass",      "\xe2\x8c\x9b"      ),  /* ⌛ U+231B */
    EMOJI("thumbs_up",      "\xf0\x9f\x91\x8d"  ),  /* 👍 U+1F44D */
    EMOJI("thumbs_down",    "\xf0\x9f\x91\x8e"  ),  /* 👎 U+1F44E */
    EMOJI("star",           "\xe2\xad\x90"      ),  /* ⭐ U+2B50 */
    EMOJI("fire",           "\xf0\x9f\x94\xa5"  ),  /* 🔥 U+1F525 */
    EMOJI("rocket",         "\xf0\x9f\x9a\x80"  ),  /* 🚀 U+1F680 */
    EMOJI("zap",            "\xe2\x9a\xa1"      ),  /* ⚡ U+26A1 */
    EMOJI("bug",            "\xf0\x9f\x90\x9b"  ),  /* 🐛 U+1F41B */
    EMOJI("wrench",         "\xf0\x9f\x94\xa7"  ),  /* 🔧 U+1F527 */
    EMOJI("bell",           "\xf0\x9f\x94\x94"  ),  /* 🔔 U+1F514 */
    EMOJI("sparkles",       "\xe2\x9c\xa8"      ),  /* ✨ U+2728 */
    EMOJI("package",        "\xf0\x9f\x93\xa6"  ),  /* 📦 U+1F4E6 */
    EMOJI("link",           "\xf0\x9f\x94\x97"  ),  /* 🔗 U+1F517 */
    EMOJI("stop",           "\xf0\x9f\x9b\x91"  ),  /* 🛑 U+1F6D1 */

#if ANSI_PRINT_EXTENDED_EMOJI
    /* -------------------------------------------------------------------- */
    /* Extended emoji                                                        */
    /* -------------------------------------------------------------------- */

    /* Faces & Expressions */
    EMOJI("smile",          "\xf0\x9f\x98\x8a"  ),  /* 😊 U+1F60A */
    EMOJI("grin",           "\xf0\x9f\x98\x81"  ),  /* 😁 U+1F601 */
    EMOJI("laugh",          "\xf0\x9f\x98\x82"  ),  /* 😂 U+1F602 */
    EMOJI("wink",           "\xf0\x9f\x98\x89"  ),  /* 😉 U+1F609 */
    EMOJI("heart_eyes",     "\xf0\x9f\x98\x8d"  ),  /* 😍 U+1F60D */
    EMOJI("cool",           "\xf0\x9f\x98\x8e"  ),  /* 😎 U+1F60E */
    EMOJI("thinking",       "\xf0\x9f\xa4\x94"  ),  /* 🤔 U+1F914 */
    EMOJI("shush",          "\xf0\x9f\xa4\xab"  ),  /* 🤫 U+1F92B */
    EMOJI("cry",            "\xf0\x9f\x98\xa2"  ),  /* 😢 U+1F622 */
    EMOJI("sob",            "\xf0\x9f\x98\xad"  ),  /* 😭 U+1F62D */
    EMOJI("angry",          "\xf0\x9f\x98\xa0"  ),  /* 😠 U+1F620 */
    EMOJI("scream",         "\xf0\x9f\x98\xb1"  ),  /* 😱 U+1F631 */
    EMOJI("sweat",          "\xf0\x9f\x98\x93"  ),  /* 😓 U+1F613 */
    EMOJI("relieved",       "\xf0\x9f\x98\x8c"  ),  /* 😌 U+1F60C */
    EMOJI("sleeping",       "\xf0\x9f\x98\xb4"  ),  /* 😴 U+1F634 */
    EMOJI("skull",          "\xf0\x9f\x92\x80"  ),  /* 💀 U+1F480 */

    /* Hands & Gestures */
    EMOJI("wave",           "\xf0\x9f\x91\x8b"  ),  /* 👋 U+1F44B */
    EMOJI("clap",           "\xf0\x9f\x91\x8f"  ),  /* 👏 U+1F44F */
    EMOJI("pray",           "\xf0\x9f\x99\x8f"  ),  /* 🙏 U+1F64F */
    EMOJI("muscle",         "\xf0\x9f\x92\xaa"  ),  /* 💪 U+1F4AA */
    EMOJI("ok_hand",        "\xf0\x9f\x91\x8c"  ),  /* 👌 U+1F44C */
    EMOJI("victory",        "\xe2\x9c\x8c"      ),  /* ✌  U+270C */
    EMOJI("point_up",       "\xf0\x9f\x91\x86"  ),  /* 👆 U+1F446 */
    EMOJI("point_down",     "\xf0\x9f\x91\x87"  ),  /* 👇 U+1F447 */
    EMOJI("point_left",     "\xf0\x9f\x91\x88"  ),  /* 👈 U+1F448 */
    EMOJI("point_right",    "\xf0\x9f\x91\x89"  ),  /* 👉 U+1F449 */
    EMOJI("raised_hand",    "\xe2\x9c\x8b"      ),  /* ✋ U+270B */
    EMOJI("pinch",          "\xf0\x9f\xa4\x8f"  ),  /* 🤏 U+1F90F */

    /* Check & Cross variants */
    EMOJI("green_check",    "\xe2\x9c\x85"      ),  /* ✅ U+2705 */
    EMOJI("check_box",      "\xe2\x98\x91"      ),  /* ☑  U+2611 */
    EMOJI("ballot_x",       "\xe2\x9c\x97"      ),  /* ✗  U+2717 */
    EMOJI("heavy_x",        "\xe2\x9c\x98"      ),  /* ✘  U+2718 */
    EMOJI("cross_box",      "\xe2\x9d\x8e"      ),  /* ❎ U+274E */

    /* Hearts & Symbols */
    EMOJI("heart",          "\xe2\x9d\xa4"      ),  /* ❤  U+2764 */
    EMOJI("broken_heart",   "\xf0\x9f\x92\x94"  ),  /* 💔 U+1F494 */
    EMOJI("question",       "\xe2\x9d\x93"      ),  /* ❓ U+2753 */
    EMOJI("exclamation",    "\xe2\x9d\x97"      ),  /* ❗ U+2757 */
    EMOJI("bangbang",       "\xe2\x80\xbc"      ),  /* ‼  U+203C */
    EMOJI("plus",           "\xe2\x9e\x95"      ),  /* ➕ U+2795 */
    EMOJI("minus",          "\xe2\x9e\x96"      ),  /* ➖ U+2796 */
    EMOJI("multiply",       "\xe2\x9c\x96"      ),  /* ✖  U+2716 */
    EMOJI("divide",         "\xe2\x9e\x97"      ),  /* ➗ U+2797 */
    EMOJI("infinity",       "\xe2\x99\xbe"      ),  /* ♾  U+267E */
    EMOJI("recycle",        "\xe2\x99\xbb"      ),  /* ♻  U+267B */
    EMOJI("copyright",      "\xc2\xa9"          ),  /* ©  U+00A9 */

    /* Arrows & Navigation */
    EMOJI("arrow_up",       "\xe2\xac\x86"      ),  /* ⬆  U+2B06 */
    EMOJI("arrow_down",     "\xe2\xac\x87"      ),  /* ⬇  U+2B07 */
    EMOJI("arrow_left",     "\xe2\xac\x85"      ),  /* ⬅  U+2B05 */
    EMOJI("arrow_right",    "\xe2\x9e\xa1"      ),  /* ➡  U+27A1 */
    EMOJI("arrow_up_down",  "\xe2\x86\x95"      ),  /* ↕  U+2195 */
    EMOJI("left_right",     "\xe2\x86\x94"      ),  /* ↔  U+2194 */
    EMOJI("back",           "\xf0\x9f\x94\x99"  ),  /* 🔙 U+1F519 */
    EMOJI("forward",        "\xf0\x9f\x94\x9c"  ),  /* 🔜 U+1F51C */
    EMOJI("refresh",        "\xf0\x9f\x94\x84"  ),  /* 🔄 U+1F504 */

    /* Objects & Tools */
    EMOJI("key",            "\xf0\x9f\x94\x91"  ),  /* 🔑 U+1F511 */
    EMOJI("lock",           "\xf0\x9f\x94\x92"  ),  /* 🔒 U+1F512 */
    EMOJI("unlock",         "\xf0\x9f\x94\x93"  ),  /* 🔓 U+1F513 */
    EMOJI("shield",         "\xf0\x9f\x9b\xa1"  ),  /* 🛡  U+1F6E1 */
    EMOJI("bomb",           "\xf0\x9f\x92\xa3"  ),  /* 💣 U+1F4A3 */
    EMOJI("hammer",         "\xf0\x9f\x94\xa8"  ),  /* 🔨 U+1F528 */
    EMOJI("scissors",       "\xe2\x9c\x82"      ),  /* ✂  U+2702 */
    EMOJI("pencil",         "\xe2\x9c\x8f"      ),  /* ✏  U+270F */
    EMOJI("pen",            "\xf0\x9f\x96\x8a"  ),  /* 🖊  U+1F58A */
    EMOJI("magnifier",      "\xf0\x9f\x94\x8d"  ),  /* 🔍 U+1F50D */
    EMOJI("flashlight",     "\xf0\x9f\x94\xa6"  ),  /* 🔦 U+1F526 */
    EMOJI("clipboard",      "\xf0\x9f\x93\x8b"  ),  /* 📋 U+1F4CB */
    EMOJI("calendar",       "\xf0\x9f\x93\x85"  ),  /* 📅 U+1F4C5 */
    EMOJI("envelope",       "\xe2\x9c\x89"      ),  /* ✉  U+2709 */
    EMOJI("phone",          "\xf0\x9f\x93\xb1"  ),  /* 📱 U+1F4F1 */
    EMOJI("laptop",         "\xf0\x9f\x92\xbb"  ),  /* 💻 U+1F4BB */
    EMOJI("desktop",        "\xf0\x9f\x96\xa5"  ),  /* 🖥  U+1F5A5 */
    EMOJI("printer",        "\xf0\x9f\x96\xa8"  ),  /* 🖨  U+1F5A8 */
    EMOJI("folder",         "\xf0\x9f\x93\x81"  ),  /* 📁 U+1F4C1 */
    EMOJI("file",           "\xf0\x9f\x93\x84"  ),  /* 📄 U+1F4C4 */
    EMOJI("trash",          "\xf0\x9f\x97\x91"  ),  /* 🗑  U+1F5D1 */

    /* Nature & Weather */
    EMOJI("sun",            "\xe2\x98\x80"      ),  /* ☀  U+2600 */
    EMOJI("moon",           "\xf0\x9f\x8c\x99"  ),  /* 🌙 U+1F319 */
    EMOJI("cloud",          "\xe2\x98\x81"      ),  /* ☁  U+2601 */
    EMOJI("rain",           "\xf0\x9f\x8c\xa7"  ),  /* 🌧  U+1F327 */
    EMOJI("snow",           "\xe2\x9d\x84"      ),  /* ❄  U+2744 */
    EMOJI("earth",          "\xf0\x9f\x8c\x8d"  ),  /* 🌍 U+1F30D */
    EMOJI("tree",           "\xf0\x9f\x8c\xb3"  ),  /* 🌳 U+1F333 */
    EMOJI("leaf",           "\xf0\x9f\x8d\x83"  ),  /* 🍃 U+1F343 */
    EMOJI("flower",         "\xf0\x9f\x8c\xb8"  ),  /* 🌸 U+1F338 */
    EMOJI("seedling",       "\xf0\x9f\x8c\xb1"  ),  /* 🌱 U+1F331 */

    /* Food & Drink */
    EMOJI("coffee",         "\xe2\x98\x95"      ),  /* ☕ U+2615 */
    EMOJI("beer",           "\xf0\x9f\x8d\xba"  ),  /* 🍺 U+1F37A */
    EMOJI("pizza",          "\xf0\x9f\x8d\x95"  ),  /* 🍕 U+1F355 */
    EMOJI("cake",           "\xf0\x9f\x8e\x82"  ),  /* 🎂 U+1F382 */
    EMOJI("apple",          "\xf0\x9f\x8d\x8e"  ),  /* 🍎 U+1F34E */

    /* Animals */
    EMOJI("dog",            "\xf0\x9f\x90\xb6"  ),  /* 🐶 U+1F436 */
    EMOJI("cat",            "\xf0\x9f\x90\xb1"  ),  /* 🐱 U+1F431 */
    EMOJI("snake",          "\xf0\x9f\x90\x8d"  ),  /* 🐍 U+1F40D */
    EMOJI("bird",           "\xf0\x9f\x90\xa6"  ),  /* 🐦 U+1F426 */
    EMOJI("fish",           "\xf0\x9f\x90\x9f"  ),  /* 🐟 U+1F41F */
    EMOJI("butterfly",      "\xf0\x9f\xa6\x8b"  ),  /* 🦋 U+1F98B */
    EMOJI("bee",            "\xf0\x9f\x90\x9d"  ),  /* 🐝 U+1F41D */
    EMOJI("ant",            "\xf0\x9f\x90\x9c"  ),  /* 🐜 U+1F41C */
    EMOJI("spider",         "\xf0\x9f\x95\xb7"  ),  /* 🕷  U+1F577 */
    EMOJI("unicorn",        "\xf0\x9f\xa6\x84"  ),  /* 🦄 U+1F984 */

    /* Travel & Transport */
    EMOJI("car",            "\xf0\x9f\x9a\x97"  ),  /* 🚗 U+1F697 */
    EMOJI("airplane",       "\xe2\x9c\x88"      ),  /* ✈  U+2708 */
    EMOJI("ship",           "\xf0\x9f\x9a\xa2"  ),  /* 🚢 U+1F6A2 */
    EMOJI("bicycle",        "\xf0\x9f\x9a\xb2"  ),  /* 🚲 U+1F6B2 */
    EMOJI("train",          "\xf0\x9f\x9a\x86"  ),  /* 🚆 U+1F686 */
    EMOJI("fuel",           "\xe2\x9b\xbd"      ),  /* ⛽ U+26FD */

    /* Celebration & Awards */
    EMOJI("trophy",         "\xf0\x9f\x8f\x86"  ),  /* 🏆 U+1F3C6 */
    EMOJI("medal",          "\xf0\x9f\x8f\x85"  ),  /* 🏅 U+1F3C5 */
    EMOJI("crown",          "\xf0\x9f\x91\x91"  ),  /* 👑 U+1F451 */
    EMOJI("gem",            "\xf0\x9f\x92\x8e"  ),  /* 💎 U+1F48E */
    EMOJI("money",          "\xf0\x9f\x92\xb0"  ),  /* 💰 U+1F4B0 */
    EMOJI("gift",           "\xf0\x9f\x8e\x81"  ),  /* 🎁 U+1F381 */
    EMOJI("ribbon",         "\xf0\x9f\x8e\x80"  ),  /* 🎀 U+1F380 */
    EMOJI("balloon",        "\xf0\x9f\x8e\x88"  ),  /* 🎈 U+1F388 */
    EMOJI("party",          "\xf0\x9f\x8e\x89"  ),  /* 🎉 U+1F389 */
    EMOJI("confetti",       "\xf0\x9f\x8e\x8a"  ),  /* 🎊 U+1F38A */

    /* Media & Arts */
    EMOJI("music",          "\xf0\x9f\x8e\xb5"  ),  /* 🎵 U+1F3B5 */
    EMOJI("film",           "\xf0\x9f\x8e\xac"  ),  /* 🎬 U+1F3AC */
    EMOJI("camera",         "\xf0\x9f\x93\xb7"  ),  /* 📷 U+1F4F7 */
    EMOJI("art",            "\xf0\x9f\x8e\xa8"  ),  /* 🎨 U+1F3A8 */
    EMOJI("palette",        "\xf0\x9f\x8e\xa8"  ),  /* 🎨 U+1F3A8 */
    EMOJI("microphone",     "\xf0\x9f\x8e\xa4"  ),  /* 🎤 U+1F3A4 */

    /* Misc Symbols */
    EMOJI("pin",            "\xf0\x9f\x93\x8c"  ),  /* 📌 U+1F4CC */
    EMOJI("paperclip",      "\xf0\x9f\x93\x8e"  ),  /* 📎 U+1F4CE */
    EMOJI("eye",            "\xf0\x9f\x91\x81"  ),  /* 👁  U+1F441 */
    EMOJI("bulb",           "\xf0\x9f\x92\xa1"  ),  /* 💡 U+1F4A1 */
    EMOJI("battery",        "\xf0\x9f\x94\x8b"  ),  /* 🔋 U+1F50B */
    EMOJI("plug",           "\xf0\x9f\x94\x8c"  ),  /* 🔌 U+1F50C */
    EMOJI("satellite",      "\xf0\x9f\x9b\xb0"  ),  /* 🛰  U+1F6F0 */
    EMOJI("flag",           "\xf0\x9f\x9a\xa9"  ),  /* 🚩 U+1F6A9 */
    EMOJI("label",          "\xf0\x9f\x8f\xb7"  ),  /* 🏷  U+1F3F7 */
    EMOJI("memo",           "\xf0\x9f\x93\x9d"  ),  /* 📝 U+1F4DD */

    /* Colored Squares */
    EMOJI("red_box",        "\xf0\x9f\x9f\xa5"  ),  /* 🟥 U+1F7E5 */
    EMOJI("orange_box",     "\xf0\x9f\x9f\xa7"  ),  /* 🟧 U+1F7E7 */
    EMOJI("yellow_box",     "\xf0\x9f\x9f\xa8"  ),  /* 🟨 U+1F7E8 */
    EMOJI("green_box",      "\xf0\x9f\x9f\xa9"  ),  /* 🟩 U+1F7E9 */
    EMOJI("blue_box",       "\xf0\x9f\x9f\xa6"  ),  /* 🟦 U+1F7E6 */
    EMOJI("purple_box",     "\xf0\x9f\x9f\xaa"  ),  /* 🟪 U+1F7EA */
    EMOJI("brown_box",      "\xf0\x9f\x9f\xab"  ),  /* 🟫 U+1F7EB */
    EMOJI("white_box",      "\xe2\xac\x9c"      ),  /* ⬜ U+2B1C */
    EMOJI("black_box",      "\xe2\xac\x9b"      ),  /* ⬛ U+2B1B */
#endif /* ANSI_PRINT_EXTENDED_EMOJI */
};

#define EMOJI_COUNT (sizeof(EMOJIS) / sizeof(EMOJIS[0]))

/** Find emoji entry by shortcode name (linear scan over EMOJIS table) */
static const EmojiEntry *lookup_emoji(const char *s, size_t len)
{
    for (size_t i = 0; i < EMOJI_COUNT; ++i) {
        if (len == EMOJIS[i].len &&
            memcmp(s, EMOJIS[i].name, len) == 0) {
            return &EMOJIS[i];
        }
    }
    return NULL;
}

#endif /* ANSI_PRINT_EMOJI */

/* ------------------------------------------------------------------------- */
/* Tag state                                                                  */
/* ------------------------------------------------------------------------- */

typedef struct {
    const char *fg_code;
    const char *bg_code;
    StyleMask   styles;
} TagState;

static TagState m_tag_state;
static const char *m_default_fg;   /* default fg restored on [/] reset */
static const char *m_default_bg;   /* default bg restored on [/] reset */
static char m_num_fg[16];  /* persistent storage for numeric fg code */
static char m_num_bg[16];  /* persistent storage for numeric bg code */


#if ANSI_PRINT_GRADIENTS
static int  m_rainbow_idx; /* current visible char index for [rainbow] */
static int  m_rainbow_len; /* total visible chars in rainbow span (pre-scan) */

/* Full-spectrum rainbow: red -> yellow -> green -> cyan -> blue -> magenta
   Uses 256-color palette indices from the 6x6x6 color cube. */
static const uint8_t RAINBOW[] = {
    196, 202, 208, 214, 220, 226,   /* red -> yellow       */
    190, 154, 118,  82,  46,        /* yellow -> green      */
     48,  51,                       /* green -> cyan        */
     45,  39,  33,                  /* cyan -> blue         */
     63,  93, 129, 165, 201         /* blue -> magenta      */
};

#define RAINBOW_LEN (sizeof(RAINBOW) / sizeof(RAINBOW[0]))

/* --- Gradient interpolation state ---------------------------------------- */

typedef struct {
    RGB start;
    RGB end;
    int len;    /* total visible chars in gradient span (from pre-scan) */
    int idx;    /* current visible char index */
} GradientState;

static GradientState m_gradient;
#endif /* ANSI_PRINT_GRADIENTS */

/** Find " on " separator in tag content (splits fg from bg) */
static const char *find_on(const char *s, size_t len)
{
    if (!s || len < 4) return NULL;
    for (size_t i = 0; i + 4 <= len; ++i) {
        if (s[i] == ' ' && s[i+1] == 'o' && s[i+2] == 'n' && s[i+3] == ' ')
            return s + i;
    }
    return NULL;
}


/** Re-emit ANSI codes for current fg/bg/styles after a RESET */
static void reapply_state(void)
{
    if (m_tag_state.fg_code) output_string(m_tag_state.fg_code);
    if (m_tag_state.bg_code) output_string(m_tag_state.bg_code);
#if ANSI_PRINT_STYLES
    if (m_tag_state.styles & STYLE_BOLD)        output_string(BOLD);
    if (m_tag_state.styles & STYLE_DIM)         output_string(DIM);
    if (m_tag_state.styles & STYLE_ITALIC)      output_string(ITALIC);
    if (m_tag_state.styles & STYLE_UNDERLINE)   output_string(UNDERLINE);
    if (m_tag_state.styles & STYLE_INVERT)      output_string(INVERT);
    if (m_tag_state.styles & STYLE_STRIKE)      output_string(STRIKETHROUGH);
#endif
}

/* ------------------------------------------------------------------------- */
/* Lookup helper                                                              */
/* ------------------------------------------------------------------------- */

/** Find color/style entry by name (linear scan over ATTRS table) */
static const AttrEntry *lookup_attr(const char *s, size_t len)
{
    for (size_t i = 0; i < sizeof(ATTRS)/sizeof(ATTRS[0]); ++i) {
        if (len == ATTRS[i].len &&
            memcmp(s, ATTRS[i].name, len) == 0) {
            return &ATTRS[i];
        }
    }
    return NULL;
}

#if ANSI_PRINT_UNICODE
/** Parse :U-XXXX: hex codepoint between colons; returns 1 on success, 0 on failure */
static int try_parse_unicode(const char *s, size_t len, uint32_t *out)
{
    if (len < 3 || len > 8 || s[0] != 'U' || s[1] != '-') return 0;
    uint32_t cp = 0;
    for (size_t i = 2; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        uint32_t nibble;
        if (c >= '0' && c <= '9')      nibble = c - '0';
        else if (c >= 'A' && c <= 'F') nibble = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') nibble = c - 'a' + 10;
        else return 0;
        cp = (cp << 4) | nibble;
    }
    if (cp == 0 || cp > 0x10FFFF) return 0;
    if (cp >= 0xD800 && cp <= 0xDFFF) return 0;  /* reject surrogates */
    *out = cp;
    return 1;
}
#endif /* ANSI_PRINT_UNICODE */

/* ------------------------------------------------------------------------- */
/* Markup tokenizer                                                           */
/* ------------------------------------------------------------------------- */

typedef enum {
    TOK_CHAR,        /* regular character (possibly multi-byte UTF-8) */
    TOK_TAG,         /* [tag] content between brackets */
    TOK_EMOJI,       /* :name: resolved emoji UTF-8 string */
    TOK_UNICODE,     /* :U-XXXX: resolved codepoint */
    TOK_ESC_BRACKET, /* [[ or ]] — literal bracket */
    TOK_ESC_COLON,   /* :: — literal colon */
    TOK_END          /* end of string */
} MarkupTokenType;

typedef struct {
    MarkupTokenType type;
    const char *ptr;    /* TOK_TAG: tag content; TOK_CHAR: start of char bytes */
    size_t      len;    /* TOK_TAG: tag length; TOK_CHAR: byte count (1-4) */
    union {
        char        ch;        /* TOK_ESC_BRACKET/COLON: the literal character */
        const char *emoji;     /* TOK_EMOJI: UTF-8 string pointer */
        uint32_t    codepoint; /* TOK_UNICODE: parsed codepoint value */
    } val;
} MarkupToken;

/** Advance *pos past the next markup token and fill tok.
    Returns 1 on success, 0 when the string is exhausted (TOK_END). */
static int next_markup_token(const char **pos, MarkupToken *tok)
{
    const char *p = *pos;
    if (!*p) { tok->type = TOK_END; return 0; }

    /* Escaped brackets: [[ → '[', ]] → ']' */
    if (p[0] == '[' && p[1] == '[') {
        tok->type = TOK_ESC_BRACKET; tok->val.ch = '[';
        *pos = p + 2; return 1;
    }
    if (p[0] == ']' && p[1] == ']') {
        tok->type = TOK_ESC_BRACKET; tok->val.ch = ']';
        *pos = p + 2; return 1;
    }

#if ANSI_PRINT_EMOJI || ANSI_PRINT_UNICODE
    /* Escaped colon: :: → ':' */
    if (p[0] == ':' && p[1] == ':') {
        tok->type = TOK_ESC_COLON; tok->val.ch = ':';
        *pos = p + 2; return 1;
    }
#endif

    /* [tag] */
    if (*p == '[') {
        const char *e = strchr(p + 1, ']');
        if (e) {
            tok->type = TOK_TAG;
            tok->ptr  = p + 1;
            tok->len  = (size_t)(e - (p + 1));
            *pos = e + 1; return 1;
        }
    }

#if ANSI_PRINT_EMOJI || ANSI_PRINT_UNICODE
    /* :name: — try emoji, then unicode */
    if (*p == ':') {
        const char *end = strchr(p + 1, ':');
        if (end && end > p + 1) {
            size_t name_len = (size_t)(end - (p + 1));
#if ANSI_PRINT_EMOJI
            {
                const EmojiEntry *em = lookup_emoji(p + 1, name_len);
                if (em) {
                    tok->type = TOK_EMOJI;
                    tok->val.emoji = em->utf8;
                    *pos = end + 1; return 1;
                }
            }
#endif
#if ANSI_PRINT_UNICODE
            {
                uint32_t cp;
                if (try_parse_unicode(p + 1, name_len, &cp)) {
                    tok->type = TOK_UNICODE;
                    tok->val.codepoint = cp;
                    *pos = end + 1; return 1;
                }
            }
#endif
        }
    }
#endif

    /* Regular character — consume full UTF-8 sequence */
    tok->type = TOK_CHAR;
    tok->ptr  = p;
    tok->val.ch = *p;
    p++;
    /* Drain UTF-8 continuation bytes (0x80-0xBF) */
    while ((unsigned char)*p >= 0x80 && (unsigned char)*p <= 0xBF) p++;
    tok->len = (size_t)(p - tok->ptr);
    *pos = p;
    return 1;
}

/** Emit all bytes of a TOK_CHAR token (handles multi-byte UTF-8) */
static void emit_token_bytes(const MarkupToken *tok)
{
    for (size_t i = 0; i < tok->len; i++)
        m_putc_function(tok->ptr[i]);
}

/* ------------------------------------------------------------------------- */
/* Tag emission                                                               */
/* ------------------------------------------------------------------------- */

#if ANSI_PRINT_GRADIENTS
/** Count visible characters from p until [/name] or [/] is found (for gradient/rainbow pre-scan) */
static int count_effect_chars(const char *p, const char *name, size_t name_len)
{
    int count = 0;
    MarkupToken tok;
    while (next_markup_token(&p, &tok)) {
        switch (tok.type) {
        case TOK_TAG:
            /* Stop at [/] or [/name] (exact match only) */
            if (tok.ptr[0] == '/' && (tok.len == 1 ||
                (tok.len >= name_len + 1 &&
                 memcmp(tok.ptr + 1, name, name_len) == 0 &&
                 (tok.len == name_len + 1 || tok.ptr[name_len + 1] == ' '))))
                return count > 0 ? count : 1;
            break; /* skip other tags */
        case TOK_CHAR:
            if (tok.val.ch != ' ' && tok.val.ch != '\t' && tok.val.ch != '\n')
                count++;
            break;
        default: /* ESC_BRACKET, ESC_COLON, EMOJI, UNICODE all count as 1 */
            count++;
            break;
        }
    }
    return count > 0 ? count : 1; /* avoid division by zero */
}

/** Parse [gradient color1 color2] arguments and activate gradient state */
static void parse_gradient_tag(const char *args, size_t len)
{
    /* Extract first color name */
    while (len && isspace((unsigned char)*args)) { args++; len--; }
    const char *c1 = args;
    while (len && !isspace((unsigned char)*args)) { args++; len--; }
    size_t c1_len = (size_t)(args - c1);

    /* Extract second color name */
    while (len && isspace((unsigned char)*args)) { args++; len--; }
    const char *c2 = args;
    size_t c2_len = 0;
    while (c2_len < len && !isspace((unsigned char)c2[c2_len])) c2_len++;

    const AttrEntry *a1 = lookup_attr(c1, c1_len);
    const AttrEntry *a2 = lookup_attr(c2, c2_len);
    if (!a1 || a1->style || !a2 || a2->style) return; /* unknown or non-color */

    m_gradient.start = a1->rgb;
    m_gradient.end   = a2->rgb;
    m_gradient.idx   = 0;
    m_gradient.len   = 0; /* filled by ansi_emit after pre-scan */
    m_tag_state.styles |= STYLE_GRADIENT;
}
#endif /* ANSI_PRINT_GRADIENTS */

/** Handle [/tag]: clear matching fg/bg/style and reset ANSI state */
static void emit_close_tag(const char *tag, size_t len)
{
    if (len == 0) { /* [/] resets to defaults */
        output_string(RESET);
        m_tag_state.fg_code = m_default_fg;
        m_tag_state.bg_code = m_default_bg;
        m_tag_state.styles  = 0;
        if (m_default_fg || m_default_bg) reapply_state();
        return;
    }

#if ANSI_PRINT_GRADIENTS
    /* [/gradient] or [/gradient ...] */
    if (len >= 8 && memcmp(tag, "gradient", 8) == 0 &&
        (len == 8 || tag[8] == ' ')) {
        m_tag_state.styles &= ~STYLE_GRADIENT;
        output_string(RESET);
        reapply_state();
        return;
    }

    /* [/rainbow] */
    if (len == 7 && memcmp(tag, "rainbow", 7) == 0) {
        m_tag_state.styles &= ~STYLE_RAINBOW;
        m_rainbow_idx = 0;
        m_rainbow_len = 0;
        output_string(RESET);
        reapply_state();
        return;
    }
#endif

    /* Split "[/red bold on yellow]" into fg words ("red bold") and bg ("yellow") */
    const char *on = find_on(tag,len);
    size_t fg_len = on ? (size_t)(on - tag) : len;
    const char *bg = on ? on + 4 : NULL;
    size_t bg_len = on ? (len - (size_t)(bg - tag)) : 0;

    /* Foreground: walk each word and undo its effect if it matches current state */
    const char *p = tag;
    while (p < tag + fg_len) {
        while (p < tag + fg_len && isspace((unsigned char)*p)) p++;
        if (p >= tag + fg_len) break;

        const char *w = p;
        while (p < tag + fg_len && !isspace((unsigned char)*p)) p++;
        size_t wl = (size_t)(p - w);
        if (!wl) continue;

        const AttrEntry *a = lookup_attr(w, wl);
        if (a) {
            if (a->style) m_tag_state.styles &= ~a->style;
            else if (a->fg_code && a->fg_code == m_tag_state.fg_code) {
                m_tag_state.fg_code = m_default_fg;
            }
            continue;
        }

        /* Numeric fg: fg:<num> — only clear if the built code matches current */
        if (wl > 3 && memcmp(w, "fg:", 3) == 0 && m_tag_state.fg_code) {
            char *endptr;
            long val = strtol(w + 3, &endptr, 10);
            if (endptr != w + 3) {
                int code = val < 0 ? 0 : val > 255 ? 255 : (int)val;
                char tmp[16];
                snprintf(tmp, sizeof(tmp), "\x1b[38;5;%dm", code);
                if (strcmp(tmp, m_tag_state.fg_code) == 0)
                    m_tag_state.fg_code = m_default_fg;
            }
        }
    }

    /* Background: clear bg color only if the close tag names the active one */
    if (bg && bg_len && m_tag_state.bg_code) {
        while (bg_len && isspace((unsigned char)*bg)) { bg++; bg_len--; }
        while (bg_len && isspace((unsigned char)bg[bg_len - 1])) bg_len--;

        const AttrEntry *a = lookup_attr(bg, bg_len);
        if (a && !a->style && a->bg_code == m_tag_state.bg_code) {
            m_tag_state.bg_code = m_default_bg;
        } else if (bg_len > 3 && memcmp(bg, "bg:", 3) == 0) {
            char *endptr;
            long val = strtol(bg + 3, &endptr, 10);
            if (endptr != bg + 3) {
                int code = val < 0 ? 0 : val > 255 ? 255 : (int)val;
                char tmp[16];
                snprintf(tmp, sizeof(tmp), "\x1b[48;5;%dm", code);
                if (strcmp(tmp, m_tag_state.bg_code) == 0)
                    m_tag_state.bg_code = m_default_bg;
            }
        }
    }

    output_string(RESET);
    reapply_state();
}

/** Handle [tag]: apply fg/bg colors and styles, or delegate to close/gradient */
static void emit_tag(const char *tag, size_t len)
{
    if (!m_color_enabled || len == 0) return;

    if (tag[0] == '/') { emit_close_tag(tag + 1, len - 1); return; }

#if ANSI_PRINT_GRADIENTS
    /* [gradient color1 color2] - special prefix, not a regular attribute */
    if (len > 9 && memcmp(tag, "gradient ", 9) == 0) {
        parse_gradient_tag(tag + 9, len - 9);
        return;
    }
#endif

    const char *on = find_on(tag, len);
    size_t fg_len = on ? (size_t)(on - tag) : len;
    const char *bg = on ? on + 4 : NULL;
    size_t bg_len = on ? (len - (size_t)(bg - tag)) : 0;

    /* Foreground words */
    const char *p = tag;
    while (p < tag + fg_len) {
        while (p < tag + fg_len && isspace((unsigned char)*p)) p++;
        if (p >= tag + fg_len) break;

        const char *w = p;
        while (p < tag + fg_len && !isspace((unsigned char)*p)) p++;
        size_t wl = (size_t)(p - w);
        if (!wl) continue;

        const AttrEntry *a = lookup_attr(w, wl);
        if (a) {
            if (a->style) { output_string(a->fg_code); m_tag_state.styles |= a->style; }
            else { output_string(a->fg_code); m_tag_state.fg_code = a->fg_code; }
            continue;
        }

        /* Numeric fg: fg:<num> — clamp to 0-255 */
        if (wl > 3 && memcmp(w, "fg:", 3) == 0) {
            char *endptr;
            long val = strtol(w + 3, &endptr, 10);
            if (endptr != w + 3) {
                int code = val < 0 ? 0 : val > 255 ? 255 : (int)val;
                snprintf(m_num_fg, sizeof(m_num_fg), "\x1b[38;5;%dm", code);
                output_string(m_num_fg); m_tag_state.fg_code = m_num_fg;
            }
        }
    }

    /* Background */
    if (bg && bg_len) {
        while (bg_len && isspace((unsigned char)*bg)) { bg++; bg_len--; }
        while (bg_len && isspace((unsigned char)bg[bg_len-1])) bg_len--;

        const AttrEntry *a = lookup_attr(bg, bg_len);
        if (a && !a->style) { output_string(a->bg_code); m_tag_state.bg_code = a->bg_code; }
        else if (bg_len > 3 && memcmp(bg, "bg:", 3) == 0) {
            char *endptr;
            long val = strtol(bg + 3, &endptr, 10);
            if (endptr != bg + 3) {
                int code = val < 0 ? 0 : val > 255 ? 255 : (int)val;
                snprintf(m_num_bg, sizeof(m_num_bg), "\x1b[48;5;%dm", code);
                output_string(m_num_bg); m_tag_state.bg_code = m_num_bg;
            }
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                 */
/* ------------------------------------------------------------------------- */

void ansi_init(ansi_putc_function putc_fn, ansi_flush_function flush_fn,
               char *buf, size_t buf_size)
{
    m_putc_function  = putc_fn  ? putc_fn  : ansi_noop_putc;
    m_flush_function = flush_fn ? flush_fn : ansi_noop_flush;
    m_buf      = buf;
    m_buf_size = buf_size;
    m_color_enabled = 1;
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
    m_no_color_lock = 0;
#endif
}

void ansi_enable(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m;
    if (GetConsoleMode(h, &m)) SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
    /* Respect the NO_COLOR convention (https://no-color.org/).
       Once set, ansi_set_enabled() and ansi_toggle() cannot re-enable color. */
    if (getenv("NO_COLOR") != NULL) {
        m_color_enabled = 0;
        m_no_color_lock = 1;
        return;
    }

    /* Disable color when stdout is not a terminal (e.g. piped to file) */
#if defined(_WIN32)
    m_color_enabled = _isatty(_fileno(stdout));
#else
    m_color_enabled = isatty(fileno(stdout));
#endif
#else
    /* Embedded/freestanding -- no env or tty detection available */
    m_color_enabled = 1;
#endif
}

void ansi_set_enabled(int enabled)
{
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
    if (!m_no_color_lock)
#endif
        m_color_enabled = enabled;
}

int ansi_is_enabled(void)
{
    return m_color_enabled;
}

void ansi_toggle(void)
{
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
    if (!m_no_color_lock)
#endif
        m_color_enabled = !m_color_enabled;
}

void ansi_set_fg(const char *color)
{
    if (!color) {
        m_default_fg = NULL;
        return;
    }
    const AttrEntry *a = lookup_attr(color, strlen(color));
    if (a && !a->style && a->fg_code) {
        m_default_fg = a->fg_code;
        m_tag_state.fg_code = a->fg_code;
        if (m_color_enabled) output_string(a->fg_code);
    }
}

void ansi_set_bg(const char *color)
{
    if (!color) {
        m_default_bg = NULL;
        return;
    }
    const AttrEntry *a = lookup_attr(color, strlen(color));
    if (a && !a->style && a->bg_code) {
        m_default_bg = a->bg_code;
        m_tag_state.bg_code = a->bg_code;
        if (m_color_enabled) output_string(a->bg_code);
    }
}

#if ANSI_PRINT_GRADIENTS
/** Emit per-character gradient or rainbow color code (call before each visible char) */
static void emit_char_color(void)
{
    if ((m_tag_state.styles & STYLE_GRADIENT) && m_color_enabled) {
        int i = m_gradient.idx;
        int n = m_gradient.len > 1 ? m_gradient.len - 1 : 1;
        if (i > n) i = n;
        int r = (int)m_gradient.start.r + ((int)m_gradient.end.r - (int)m_gradient.start.r) * i / n;
        int g = (int)m_gradient.start.g + ((int)m_gradient.end.g - (int)m_gradient.start.g) * i / n;
        int b = (int)m_gradient.start.b + ((int)m_gradient.end.b - (int)m_gradient.start.b) * i / n;
        char buf[24];
        snprintf(buf, sizeof(buf), "\x1b[38;2;%d;%d;%dm", r, g, b);
        output_string(buf);
        m_gradient.idx++;
    } else if ((m_tag_state.styles & STYLE_RAINBOW) && m_color_enabled) {
        int pos = m_rainbow_idx * (int)(RAINBOW_LEN - 1) /
                  (m_rainbow_len > 1 ? m_rainbow_len - 1 : 1);
        if (pos > (int)(RAINBOW_LEN - 1)) pos = (int)(RAINBOW_LEN - 1);
        char buf[16];
        snprintf(buf, sizeof(buf), "\x1b[38;5;%dm", RAINBOW[pos]);
        output_string(buf);
        m_rainbow_idx++;
    }
}
#else
static void emit_char_color(void) { }
#endif /* ANSI_PRINT_GRADIENTS */

#if ANSI_PRINT_UNICODE
/** Encode a Unicode codepoint as UTF-8 and emit via putc function */
static void emit_unicode_codepoint(uint32_t cp)
{
    if (cp <= 0x7F) {
        m_putc_function((int)cp);
    } else if (cp <= 0x7FF) {
        m_putc_function((int)(0xC0 | (cp >> 6)));
        m_putc_function((int)(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        m_putc_function((int)(0xE0 | (cp >> 12)));
        m_putc_function((int)(0x80 | ((cp >> 6) & 0x3F)));
        m_putc_function((int)(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        m_putc_function((int)(0xF0 | (cp >> 18)));
        m_putc_function((int)(0x80 | ((cp >> 12) & 0x3F)));
        m_putc_function((int)(0x80 | ((cp >> 6) & 0x3F)));
        m_putc_function((int)(0x80 | (cp & 0x3F)));
    }
}

#endif /* ANSI_PRINT_UNICODE */

/** Core markup renderer: tokenize Rich-style text and emit with ANSI codes */
static void ansi_emit(const char *p)
{
    if (!p) return;
    m_tag_state.fg_code = NULL;
    m_tag_state.bg_code = NULL;
    m_tag_state.styles  = 0;
#if ANSI_PRINT_GRADIENTS
    m_rainbow_idx = 0;
    m_rainbow_len = 0;
    m_gradient.len = 0;
    m_gradient.idx = 0;
#endif

    MarkupToken tok;
    while (next_markup_token(&p, &tok)) {
        switch (tok.type) {
        /* Always-on: literal escapes and plain characters */
        case TOK_ESC_BRACKET:
        case TOK_ESC_COLON:
            m_putc_function(tok.val.ch);
            break;
        case TOK_TAG:
            emit_tag(tok.ptr, tok.len);
#if ANSI_PRINT_GRADIENTS
            if ((m_tag_state.styles & STYLE_GRADIENT) && m_gradient.len == 0)
                m_gradient.len = count_effect_chars(p, "gradient", 8);
            if ((m_tag_state.styles & STYLE_RAINBOW) && m_rainbow_len == 0)
                m_rainbow_len = count_effect_chars(p, "rainbow", 7);
#endif
            break;
        case TOK_CHAR:
            if (tok.val.ch != ' ' && tok.val.ch != '\t' && tok.val.ch != '\n')
                emit_char_color();
            emit_token_bytes(&tok);
            break;
        /* Feature-gated: only emitted when the corresponding flag is on */
#if ANSI_PRINT_EMOJI
        case TOK_EMOJI:
            emit_char_color();
            output_string(tok.val.emoji);
            break;
#endif
#if ANSI_PRINT_UNICODE
        case TOK_UNICODE:
            emit_char_color();
            emit_unicode_codepoint(tok.val.codepoint);
            break;
#endif
        default: break;
        }
    }

    if (m_color_enabled && (m_tag_state.fg_code||m_tag_state.bg_code||m_tag_state.styles))
        output_string(RESET);

    m_flush_function();
}

/** Emit a pre-built markup string (no formatting) */
void ansi_puts(const char *s) { ansi_emit(s); }

/** Printf into shared buffer via va_list, return pointer (NULL on error) */
static const char *ansi_vformat(const char *fmt, va_list ap)
{
    if (!fmt || !m_buf || !m_buf_size) return NULL;
    vsnprintf(m_buf, m_buf_size, fmt, ap);
    return m_buf;
}

/** Access the shared format buffer */
char *ansi_get_buf(size_t *out_size)
{
    if (out_size) *out_size = m_buf_size;
    return m_buf;
}

/** Printf into shared buffer and return pointer — does not emit */
const char *ansi_format(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const char *result = ansi_vformat(fmt, ap);
    va_end(ap);
    return result;
}

/** Printf into shared buffer, then emit with markup processing */
void ansi_print(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const char *result = ansi_vformat(fmt, ap);
    va_end(ap);
    ansi_emit(result);
}

/** va_list variant of ansi_print — format and emit with markup processing */
void ansi_vprint(const char *fmt, va_list ap)
{
    ansi_emit(ansi_vformat(fmt, ap));
}

#if ANSI_PRINT_BANNER

/** Count visible characters in plain text (skips UTF-8 continuation bytes) */
static int banner_visible_len(const char *p, int byte_len)
{
    int vis = 0;
    for (int i = 0; i < byte_len; i++) {
        if (!((unsigned char)p[i] >= 0x80 && (unsigned char)p[i] <= 0xBF)) /* skip UTF-8 continuation bytes */
            vis++;
    }
    return vis;
}

/** Emit up to max_vis visible characters from plain text, return bytes consumed */
static int banner_emit_vis(const char *p, int byte_len, int max_vis)
{
    int vis = 0, i = 0;
    while (i < byte_len && vis < max_vis) {
        unsigned char c = (unsigned char)p[i];
        m_putc_function(c);
        i++;
        if (!(c >= 0x80 && c <= 0xBF)) /* not a UTF-8 continuation byte */
            vis++;
        /* Drain continuation bytes of the character we just started */
        while (i < byte_len &&
               (unsigned char)p[i] >= 0x80 && (unsigned char)p[i] <= 0xBF) {
            m_putc_function(p[i]);
            i++;
        }
    }
    return i;
}

void ansi_banner(const char *color, int width, ansi_align_t align,
                 const char *fmt, ...)
{
    if (!fmt || !m_buf || !m_buf_size) return;

    /* Format text into buffer */
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m_buf, m_buf_size, fmt, ap);
    va_end(ap);

    /* Compute effective width: if 0, auto-size to longest line (visible chars) */
    if (width <= 0) {
        width = 0;
        const char *p = m_buf;
        while (*p) {
            int byte_len = 0;
            while (p[byte_len] && p[byte_len] != '\n') byte_len++;
            int vis = banner_visible_len(p, byte_len);
            if (vis > width) width = vis;
            p += byte_len;
            if (*p == '\n') p++;
        }
    }
    if (width < 1) width = 1;

    /* Resolve color name to ANSI foreground code */
    const char *fg = NULL;
    if (color) {
        const AttrEntry *a = lookup_attr(color, strlen(color));
        if (a) fg = a->fg_code;
    }

    if (fg && m_color_enabled) output_string(fg);

    /* Top border */
    output_string(BOX_TOPLEFT);
    for (int i = 0; i < width + 2; i++) output_string(BOX_HORZ);
    output_string(BOX_TOPRIGHT);
    m_putc_function('\n');

    /* Walk the buffer line-by-line (split on '\n'), emitting each
       as a bordered row:  ║ <pad> text <pad> ║ */
    const char *p = m_buf;
    do {
        const char *eol = p;
        while (*eol && *eol != '\n') eol++;
        int byte_len = (int)(eol - p);
        int vis_len = banner_visible_len(p, byte_len);

        output_string(BOX_VERT);
        m_putc_function(' ');

        /* Truncate line to box width, then compute alignment padding */
        int out = vis_len > width ? width : vis_len;
        int pad = width - out;
        int pad_left = 0;
        if (align == ANSI_ALIGN_CENTER)      pad_left = pad / 2;
        else if (align == ANSI_ALIGN_RIGHT)  pad_left = pad;
        int pad_right = pad - pad_left;

        for (int i = 0; i < pad_left; i++)  m_putc_function(' ');
        banner_emit_vis(p, byte_len, out);
        for (int i = 0; i < pad_right; i++) m_putc_function(' ');

        m_putc_function(' ');
        output_string(BOX_VERT);
        m_putc_function('\n');

        p = (*eol == '\n') ? eol + 1 : eol;
    } while (*p);

    /* Bottom border */
    output_string(BOX_BOTTOMLEFT);
    for (int i = 0; i < width + 2; i++) output_string(BOX_HORZ);
    output_string(BOX_BOTTOMRIGHT);

    if (fg && m_color_enabled) output_string(RESET);
    m_putc_function('\n');

    m_flush_function();
}
#endif /* ANSI_PRINT_BANNER */

/* ------------------------------------------------------------------------- */
/* Window (streaming boxed text)                                             */
/* ------------------------------------------------------------------------- */

#if ANSI_PRINT_WINDOW

static int         m_window_width = 0;
static const char *m_window_fg    = NULL;  /* border color from start() */

/** Resolve a color name to its ANSI foreground escape code (or NULL) */
static const char *window_resolve_color(const char *color)
{
    if (!color) return NULL;
    const AttrEntry *a = lookup_attr(color, strlen(color));
    return a ? a->fg_code : NULL;
}

/** Count visible characters in Rich markup text (tags are zero-width) */
static int window_count_visible(const char *p)
{
    int count = 0;
    MarkupToken tok;
    while (next_markup_token(&p, &tok)) {
        switch (tok.type) {
        case TOK_TAG: break;                /* tags are invisible */
        default:      count++; break;       /* everything else is visible */
        }
    }
    return count;
}

/* Emit text through the Rich markup parser, stopping after max_vis visible
   characters.  Resets tag state before and after.  Does NOT call flush. */
/** Emit Rich markup text, stopping after max_vis visible characters */
static void window_emit_text(const char *p, int max_vis)
{
    int vis = 0;
    m_tag_state.fg_code = NULL;
    m_tag_state.bg_code = NULL;
    m_tag_state.styles  = 0;
#if ANSI_PRINT_GRADIENTS
    m_rainbow_idx  = 0;
    m_rainbow_len  = 0;
    m_gradient.len = 0;
    m_gradient.idx = 0;
#endif

    MarkupToken tok;
    while (vis < max_vis && next_markup_token(&p, &tok)) {
        switch (tok.type) {
        /* Always-on: literal escapes and plain characters */
        case TOK_ESC_BRACKET:
        case TOK_ESC_COLON:
            m_putc_function(tok.val.ch);
            vis++;
            break;
        case TOK_TAG:
            emit_tag(tok.ptr, tok.len);
#if ANSI_PRINT_GRADIENTS
            if ((m_tag_state.styles & STYLE_GRADIENT) && m_gradient.len == 0)
                m_gradient.len = count_effect_chars(p, "gradient", 8);
            if ((m_tag_state.styles & STYLE_RAINBOW) && m_rainbow_len == 0)
                m_rainbow_len = count_effect_chars(p, "rainbow", 7);
#endif
            break;
        case TOK_CHAR:
            if (tok.val.ch != ' ' && tok.val.ch != '\t' && tok.val.ch != '\n')
                emit_char_color();
            emit_token_bytes(&tok);
            vis++;
            break;
        /* Feature-gated: only emitted when the corresponding flag is on */
#if ANSI_PRINT_EMOJI
        case TOK_EMOJI:
            emit_char_color();
            output_string(tok.val.emoji);
            vis++;
            break;
#endif
#if ANSI_PRINT_UNICODE
        case TOK_UNICODE:
            emit_char_color();
            emit_unicode_codepoint(tok.val.codepoint);
            vis++;
            break;
#endif
        default: break;
        }
    }

    if (m_color_enabled &&
        (m_tag_state.fg_code || m_tag_state.bg_code || m_tag_state.styles))
        output_string(RESET);
}

/* Emit one padded plain-text line between ║ borders (used for title) */
/** Emit one padded plain-text line between box-drawing borders (for title) */
static void window_emit_line(const char *text, int len, ansi_align_t align)
{
    int width     = m_window_width;
    int emit_len  = len > width ? width : len;
    int total_pad = width - emit_len;
    int pad_left  = 0;
    if (align == ANSI_ALIGN_CENTER)      pad_left = total_pad / 2;
    else if (align == ANSI_ALIGN_RIGHT)  pad_left = total_pad;
    int pad_right = total_pad - pad_left;

    if (m_window_fg && m_color_enabled) output_string(m_window_fg);
    output_string(BOX_VERT);
    m_putc_function(' ');
    for (int i = 0; i < pad_left; i++)  m_putc_function(' ');
    for (int i = 0; i < emit_len; i++)  m_putc_function(text[i]);
    for (int i = 0; i < pad_right; i++) m_putc_function(' ');
    m_putc_function(' ');
    output_string(BOX_VERT);
    if (m_window_fg && m_color_enabled) output_string(RESET);
    m_putc_function('\n');
}

void ansi_window_start(const char *color, int width, ansi_align_t align,
                       const char *title)
{
    m_window_width = width < 1 ? 1 : width;
    m_window_fg = window_resolve_color(color);

    /* Top border */
    if (m_window_fg && m_color_enabled) output_string(m_window_fg);
    output_string(BOX_TOPLEFT);
    for (int i = 0; i < m_window_width + 2; i++) output_string(BOX_HORZ);
    output_string(BOX_TOPRIGHT);
    if (m_window_fg && m_color_enabled) output_string(RESET);
    m_putc_function('\n');

    /* Title + separator (if title is non-NULL and non-empty) */
    if (title && *title) {
        window_emit_line(title, (int)strlen(title), align);

        /* Separator */
        if (m_window_fg && m_color_enabled) output_string(m_window_fg);
        output_string(BOX_MIDLEFT);
        for (int i = 0; i < m_window_width + 2; i++) output_string(BOX_HORZ);
        output_string(BOX_MIDRIGHT);
        if (m_window_fg && m_color_enabled) output_string(RESET);
        m_putc_function('\n');
    }
}

void ansi_window_line(ansi_align_t align, const char *fmt, ...)
{
    if (!fmt || !m_buf || !m_buf_size) return;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m_buf, m_buf_size, fmt, ap);
    va_end(ap);

    int visible   = window_count_visible(m_buf);
    int width     = m_window_width;
    int emit_len  = visible > width ? width : visible;
    int total_pad = width - emit_len;
    int pad_left  = 0;
    if (align == ANSI_ALIGN_CENTER)      pad_left = total_pad / 2;
    else if (align == ANSI_ALIGN_RIGHT)  pad_left = total_pad;
    int pad_right = total_pad - pad_left;

    /* Left border in border color */
    if (m_window_fg && m_color_enabled) output_string(m_window_fg);
    output_string(BOX_VERT);
    m_putc_function(' ');
    if (m_window_fg && m_color_enabled) output_string(RESET);

    /* Left padding */
    for (int i = 0; i < pad_left; i++) m_putc_function(' ');

    /* Text with Rich markup processing (truncated to window width) */
    window_emit_text(m_buf, emit_len);

    /* Right padding */
    for (int i = 0; i < pad_right; i++) m_putc_function(' ');

    /* Right border in border color */
    if (m_window_fg && m_color_enabled) output_string(m_window_fg);
    m_putc_function(' ');
    output_string(BOX_VERT);
    if (m_window_fg && m_color_enabled) output_string(RESET);
    m_putc_function('\n');
}

/**
 * @brief Close a window by emitting the bottom border.
 *
 * Emits the bottom border in the color set by ansi_window_start(),
 * then flushes output.
 */
void ansi_window_end(void)
{
    if (m_window_fg && m_color_enabled) output_string(m_window_fg);
    output_string(BOX_BOTTOMLEFT);
    for (int i = 0; i < m_window_width + 2; i++) output_string(BOX_HORZ);
    output_string(BOX_BOTTOMRIGHT);
    if (m_window_fg && m_color_enabled) output_string(RESET);
    m_putc_function('\n');
    m_flush_function();
}

#endif /* ANSI_PRINT_WINDOW */

/* ------------------------------------------------------------------------- */
/* Bar graph                                                                  */
/* ------------------------------------------------------------------------- */

#if ANSI_PRINT_BAR

/*
 * ansi_bar() -- build a bar graph string into a caller-provided buffer.
 *
 * The buffer is passed directly rather than via an init function so that
 * multiple bars can coexist in the same printf argument list:
 *
 *   char b1[128], b2[128];
 *   ansi_print("CPU %s  MEM %s\n",
 *              ansi_bar(b1, sizeof(b1), "green", 15, ANSI_BAR_LIGHT, cpu, 0, 100),
 *              ansi_bar(b2, sizeof(b2), "cyan",  15, ANSI_BAR_LIGHT, mem, 0, 100));
 *
 * Each call writes to its own buffer, so there is no shared state and
 * no ordering dependency between argument evaluations.
 */
const char *ansi_bar(char *buf, size_t buf_size,
                     const char *color, int width, ansi_bar_track_t track,
                     double value, double min, double max)
{
    /* Graceful fallback for NULL or tiny buffers */
    if (!buf || buf_size == 0) return "";
    if (buf_size < 2) {
        buf[0] = '\0';
        return buf;
    }

    char *out = buf;
    char *end = buf + buf_size - 1;

    if (width < 1) { buf[0] = '\0'; return buf; }

    /* Resolve track character -- default to space for unknown values */
    int tk_idx = (int)track;
    if (tk_idx < 0 || tk_idx >= (int)(sizeof(m_bar_track) / sizeof(m_bar_track[0])))
        tk_idx = 0;
    const char *tk_str = m_bar_track[tk_idx].s;
    int         tk_len = m_bar_track[tk_idx].len;

    /* Compute fill fraction, clamped to [0.0, 1.0] */
    double fraction;
    if (max == min) {
        fraction = 1.0;           /* degenerate range -> full bar */
    } else {
        fraction = (value - min) / (max - min);
    }
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;

    /* Convert fraction to 1/8-cell units */
    int eighths     = (int)(fraction * width * 8 + 0.5);
    int filled_cells = (eighths + 7) / 8;  /* ceil(eighths / 8) */
    int empty        = width - filled_cells;

    /* Resolve color name to tag string — only emit if both [color] and [/color]
       fit completely, so we never produce an incomplete tag like "[re" */
    int has_color = 0;
    size_t clen = 0;
    if (color) {
        const AttrEntry *a = lookup_attr(color, strlen(color));
        if (a && a->fg_code) {
            clen = strlen(color);
            size_t need = (clen + 2) + (clen + 3);  /* [color] + [/color] */
            if (out + need <= end) {
                *out++ = '[';
                memcpy(out, color, clen); out += clen;
                *out++ = ']';
                has_color = 1;
            }
        }
    }

    /* When color is active, reserve bytes for [/color] close tag so that
       block-writing loops cannot consume space needed for it. */
    char *blk_end = has_color ? end - (ptrdiff_t)(clen + 3) : end;

    /* Emit filled cells: consume eighths left-to-right, one cell at a time */
    while (eighths > 0 && out + 3 <= blk_end) {
        int fill = eighths >= 8 ? 8 : eighths;
        memcpy(out, m_bar_block[fill], 3);
        out += 3;
        eighths -= fill;
    }

    /* Close only the bar's own color, preserving any surrounding color state */
    if (has_color) {
        *out++ = '[';
        *out++ = '/';
        memcpy(out, color, clen); out += clen;
        *out++ = ']';
    }

    /* Emit empty track */
    for (int i = 0; i < empty && out + tk_len <= end; i++) {
        memcpy(out, tk_str, tk_len); out += tk_len;
    }

    *out = '\0';
    return buf;
}

/*
 * ansi_bar_percent() -- bar graph with " XX%" appended.
 * Range is always 0-100. Calls ansi_bar() then appends the clamped percent.
 */
const char *ansi_bar_percent(char *buf, size_t buf_size,
                             const char *color, int width,
                             ansi_bar_track_t track, int percent)
{
    int pct = percent < 0 ? 0 : percent > 100 ? 100 : percent;
    ansi_bar(buf, buf_size, color, width, track, pct, 0, 100);

    /* Append " XX%" to the bar string */
    size_t len = strlen(buf);
    if (len < buf_size - 1) {
        snprintf(buf + len, buf_size - len, " %d%%", pct);
    }
    return buf;
}

#endif /* ANSI_PRINT_BAR */
