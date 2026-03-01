/* emoji_std.inc — default emoji table for standard terminals
 *
 * Capitalized first letter = 1 terminal cell, lowercase = 2 cells.
 * Lookup is case-insensitive, so :warning: matches "Warning".
 *
 * Selected by: ANSI_PRINT_EMOJI_FONT == ANSI_EMOJI_FONT_STD (default)
 */

    /* --- Core emoji (always present when ANSI_PRINT_EMOJI=1) --- */
    EMOJI("check",          "\xe2\x9c\x85"      ),  /* ✅ U+2705 */
    EMOJI("cross",          "\xe2\x9d\x8c"      ),  /* ❌ U+274C */
    EMOJI("Warning",        "\xe2\x9a\xa0"      ),  /* ⚠  U+26A0  (1-cell) */
    EMOJI("Info",           "\xe2\x84\xb9"      ),  /* ℹ  U+2139  (1-cell) */
    EMOJI("Arrow",          "\xe2\x9e\xa1"      ),  /* ➡  U+27A1  (1-cell) */
    EMOJI("Gear",           "\xe2\x9a\x99"      ),  /* ⚙  U+2699  (1-cell) */
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
    EMOJI("cry",            "\xf0\x9f\x98\xa2"  ),  /* 😢 U+1F622 */
    EMOJI("angry",          "\xf0\x9f\x98\xa0"  ),  /* 😠 U+1F620 */
    EMOJI("scream",         "\xf0\x9f\x98\xb1"  ),  /* 😱 U+1F631 */
    EMOJI("skull",          "\xf0\x9f\x92\x80"  ),  /* 💀 U+1F480 */

    /* Hands & Gestures */
    EMOJI("wave",           "\xf0\x9f\x91\x8b"  ),  /* 👋 U+1F44B */
    EMOJI("clap",           "\xf0\x9f\x91\x8f"  ),  /* 👏 U+1F44F */
    EMOJI("pray",           "\xf0\x9f\x99\x8f"  ),  /* 🙏 U+1F64F */
    EMOJI("muscle",         "\xf0\x9f\x92\xaa"  ),  /* 💪 U+1F4AA */
    EMOJI("ok_hand",        "\xf0\x9f\x91\x8c"  ),  /* 👌 U+1F44C */
    EMOJI("Victory",        "\xe2\x9c\x8c"      ),  /* ✌  U+270C */
    EMOJI("point_up",       "\xf0\x9f\x91\x86"  ),  /* 👆 U+1F446 */
    EMOJI("point_down",     "\xf0\x9f\x91\x87"  ),  /* 👇 U+1F447 */
    EMOJI("point_left",     "\xf0\x9f\x91\x88"  ),  /* 👈 U+1F448 */
    EMOJI("point_right",    "\xf0\x9f\x91\x89"  ),  /* 👉 U+1F449 */
    EMOJI("raised_hand",    "\xe2\x9c\x8b"      ),  /* ✋ U+270B */

    /* Check & Cross variants */
    EMOJI("Check_box",      "\xe2\x98\x91"      ),  /* ☑  U+2611 */
    EMOJI("Ballot_x",       "\xe2\x9c\x97"      ),  /* ✗  U+2717 */
    EMOJI("Heavy_x",        "\xe2\x9c\x98"      ),  /* ✘  U+2718 */
    EMOJI("cross_box",      "\xe2\x9d\x8e"      ),  /* ❎ U+274E */

    /* Hearts & Symbols */
    EMOJI("Heart",          "\xe2\x9d\xa4"      ),  /* ❤  U+2764 */
    EMOJI("broken_heart",   "\xf0\x9f\x92\x94"  ),  /* 💔 U+1F494 */
    EMOJI("question",       "\xe2\x9d\x93"      ),  /* ❓ U+2753 */
    EMOJI("exclamation",    "\xe2\x9d\x97"      ),  /* ❗ U+2757 */
    EMOJI("Bangbang",       "\xe2\x80\xbc"      ),  /* ‼  U+203C  (1-cell) */
    EMOJI("plus",           "\xe2\x9e\x95"      ),  /* ➕ U+2795 */
    EMOJI("minus",          "\xe2\x9e\x96"      ),  /* ➖ U+2796 */
    EMOJI("Multiply",       "\xe2\x9c\x96"      ),  /* ✖  U+2716 */
    EMOJI("divide",         "\xe2\x9e\x97"      ),  /* ➗ U+2797 */
    EMOJI("Infinity",       "\xe2\x99\xbe"      ),  /* ♾  U+267E */
    EMOJI("Recycle",        "\xe2\x99\xbb"      ),  /* ♻  U+267B */
    EMOJI("Copyright",      "\xc2\xa9"          ),  /* ©  U+00A9  (1-cell) */

    /* Arrows & Navigation */
    EMOJI("Arrow_up",       "\xe2\xac\x86"      ),  /* ⬆  U+2B06 */
    EMOJI("Arrow_down",     "\xe2\xac\x87"      ),  /* ⬇  U+2B07 */
    EMOJI("Arrow_left",     "\xe2\xac\x85"      ),  /* ⬅  U+2B05 */
    EMOJI("Arrow_right",    "\xe2\x9e\xa1"      ),  /* ➡  U+27A1  (1-cell) */
    EMOJI("Arrow_up_down",  "\xe2\x86\x95"      ),  /* ↕  U+2195  (1-cell) */
    EMOJI("Left_right",     "\xe2\x86\x94"      ),  /* ↔  U+2194  (1-cell) */
    EMOJI("back",           "\xf0\x9f\x94\x99"  ),  /* 🔙 U+1F519 */
    EMOJI("forward",        "\xf0\x9f\x94\x9c"  ),  /* 🔜 U+1F51C */
    EMOJI("refresh",        "\xf0\x9f\x94\x84"  ),  /* 🔄 U+1F504 */

    /* Objects & Tools */
    EMOJI("key",            "\xf0\x9f\x94\x91"  ),  /* 🔑 U+1F511 */
    EMOJI("lock",           "\xf0\x9f\x94\x92"  ),  /* 🔒 U+1F512 */
    EMOJI("unlock",         "\xf0\x9f\x94\x93"  ),  /* 🔓 U+1F513 */
    EMOJI("Shield",         "\xf0\x9f\x9b\xa1"  ),  /* 🛡  U+1F6E1 */
    EMOJI("bomb",           "\xf0\x9f\x92\xa3"  ),  /* 💣 U+1F4A3 */
    EMOJI("hammer",         "\xf0\x9f\x94\xa8"  ),  /* 🔨 U+1F528 */
    EMOJI("Scissors",       "\xe2\x9c\x82"      ),  /* ✂  U+2702 */
    EMOJI("Pencil",         "\xe2\x9c\x8f"      ),  /* ✏  U+270F */
    EMOJI("Pen",            "\xf0\x9f\x96\x8a"  ),  /* 🖊  U+1F58A */
    EMOJI("magnifier",      "\xf0\x9f\x94\x8d"  ),  /* 🔍 U+1F50D */
    EMOJI("flashlight",     "\xf0\x9f\x94\xa6"  ),  /* 🔦 U+1F526 */
    EMOJI("clipboard",      "\xf0\x9f\x93\x8b"  ),  /* 📋 U+1F4CB */
    EMOJI("calendar",       "\xf0\x9f\x93\x85"  ),  /* 📅 U+1F4C5 */
    EMOJI("Envelope",       "\xe2\x9c\x89"      ),  /* ✉  U+2709 */
    EMOJI("phone",          "\xf0\x9f\x93\xb1"  ),  /* 📱 U+1F4F1 */
    EMOJI("laptop",         "\xf0\x9f\x92\xbb"  ),  /* 💻 U+1F4BB */
    EMOJI("Desktop",        "\xf0\x9f\x96\xa5"  ),  /* 🖥  U+1F5A5 */
    EMOJI("Printer",        "\xf0\x9f\x96\xa8"  ),  /* 🖨  U+1F5A8 */
    EMOJI("folder",         "\xf0\x9f\x93\x81"  ),  /* 📁 U+1F4C1 */
    EMOJI("file",           "\xf0\x9f\x93\x84"  ),  /* 📄 U+1F4C4 */
    EMOJI("Trash",          "\xf0\x9f\x97\x91"  ),  /* 🗑  U+1F5D1 */

    /* Nature & Weather */
    EMOJI("Sun",            "\xe2\x98\x80"      ),  /* ☀  U+2600 */
    EMOJI("moon",           "\xf0\x9f\x8c\x99"  ),  /* 🌙 U+1F319 */
    EMOJI("Cloud",          "\xe2\x98\x81"      ),  /* ☁  U+2601 */
    EMOJI("Rain",           "\xf0\x9f\x8c\xa7"  ),  /* 🌧  U+1F327 */
    EMOJI("Snow",           "\xe2\x9d\x84"      ),  /* ❄  U+2744 */
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
    EMOJI("Spider",         "\xf0\x9f\x95\xb7"  ),  /* 🕷  U+1F577 */
    EMOJI("unicorn",        "\xf0\x9f\xa6\x84"  ),  /* 🦄 U+1F984 */

    /* Travel & Transport */
    EMOJI("car",            "\xf0\x9f\x9a\x97"  ),  /* 🚗 U+1F697 */
    EMOJI("Airplane",       "\xe2\x9c\x88"      ),  /* ✈  U+2708 */
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
    EMOJI("microphone",     "\xf0\x9f\x8e\xa4"  ),  /* 🎤 U+1F3A4 */

    /* Misc Symbols */
    EMOJI("pin",            "\xf0\x9f\x93\x8c"  ),  /* 📌 U+1F4CC */
    EMOJI("paperclip",      "\xf0\x9f\x93\x8e"  ),  /* 📎 U+1F4CE */
    EMOJI("Eye",            "\xf0\x9f\x91\x81"  ),  /* 👁  U+1F441 */
    EMOJI("bulb",           "\xf0\x9f\x92\xa1"  ),  /* 💡 U+1F4A1 */
    EMOJI("battery",        "\xf0\x9f\x94\x8b"  ),  /* 🔋 U+1F50B */
    EMOJI("plug",           "\xf0\x9f\x94\x8c"  ),  /* 🔌 U+1F50C */
    EMOJI("Satellite",      "\xf0\x9f\x9b\xb0"  ),  /* 🛰  U+1F6F0 */
    EMOJI("flag",           "\xf0\x9f\x9a\xa9"  ),  /* 🚩 U+1F6A9 */
    EMOJI("Label",          "\xf0\x9f\x8f\xb7"  ),  /* 🏷  U+1F3F7 */
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

    /* Colored Circles */
    EMOJI("red_circle",     "\xf0\x9f\x94\xb4"  ),  /* 🔴 U+1F534 */
    EMOJI("orange_circle",  "\xf0\x9f\x9f\xa0"  ),  /* 🟠 U+1F7E0 */
    EMOJI("yellow_circle",  "\xf0\x9f\x9f\xa1"  ),  /* 🟡 U+1F7E1 */
    EMOJI("green_circle",   "\xf0\x9f\x9f\xa2"  ),  /* 🟢 U+1F7E2 */
    EMOJI("blue_circle",    "\xf0\x9f\x94\xb5"  ),  /* 🔵 U+1F535 */
    EMOJI("purple_circle",  "\xf0\x9f\x9f\xa3"  ),  /* 🟣 U+1F7E3 */
    EMOJI("brown_circle",   "\xf0\x9f\x9f\xa4"  ),  /* 🟤 U+1F7E4 */
    EMOJI("Black_circle",   "\xe2\x9a\xab"      ),  /* ⚫ U+26AB  (1-cell) */
    EMOJI("White_circle",   "\xe2\x9a\xaa"      ),  /* ⚪ U+26AA  (1-cell) */
#endif /* ANSI_PRINT_EXTENDED_EMOJI */
