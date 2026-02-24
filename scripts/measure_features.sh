#!/bin/bash
# Measure .text section size for each feature flag individually.
# Usage: bash measure_features.sh
# Requires: clang (or set CC), size

CC="${CC:-clang}"
SRC="src/ansi_print.c"
TUI_SRC="src/ansi_tui.c"
OUT="build/ansi_measure.o"
TUI_OUT="build/tui_measure.o"
BASE_FLAGS="-Os -ffunction-sections -fdata-sections -std=c99 -I src"
MINIMAL="-DANSI_PRINT_NO_APP_CFG -DANSI_PRINT_MINIMAL"

mkdir -p build

# Get .text size for ansi_print.c only
get_text() {
    $CC $BASE_FLAGS $1 -c "$SRC" -o "$OUT" 2>/dev/null
    size "$OUT" | awk 'NR==2 { print $1 }'
}

# Get combined .text size for ansi_print.c + ansi_tui.c
get_text_tui() {
    $CC $BASE_FLAGS $1 -c "$SRC" -o "$OUT" 2>/dev/null
    $CC $BASE_FLAGS $1 -c "$TUI_SRC" -o "$TUI_OUT" 2>/dev/null
    text1=$(size "$OUT" | awk 'NR==2 { print $1 }')
    text2=$(size "$TUI_OUT" | awk 'NR==2 { print $1 }')
    echo $((text1 + text2))
}

echo "Measuring .text sizes with $CC ..."
echo ""

# ── ansi_print feature flags ──────────────────────────────────

echo "=== ansi_print features ==="
echo ""

# Minimal baseline
min=$(get_text "$MINIMAL")
printf "%-30s %6s B\n" "Minimal baseline" "$min"

# Each feature individually on top of minimal
for feat in ANSI_PRINT_UNICODE ANSI_PRINT_BRIGHT_COLORS ANSI_PRINT_STYLES \
            ANSI_PRINT_EXTENDED_COLORS ANSI_PRINT_EMOJI ANSI_PRINT_BAR \
            ANSI_PRINT_BANNER ANSI_PRINT_GRADIENTS ANSI_PRINT_WINDOW \
            ANSI_PRINT_EXTENDED_EMOJI; do
    extra=""
    # EXTENDED_EMOJI requires EMOJI
    if [ "$feat" = "ANSI_PRINT_EXTENDED_EMOJI" ]; then
        extra="-DANSI_PRINT_EMOJI=1"
    fi
    val=$(get_text "$MINIMAL -D${feat}=1 $extra")
    delta=$((val - min))
    printf "%-30s %6s B  (+%d)\n" "$feat" "$val" "$delta"
done

# Full build
full=$(get_text "-DANSI_PRINT_NO_APP_CFG")
full_delta=$((full - min))
printf "%-30s %6s B  (+%d)\n" "Full build" "$full" "$full_delta"

# Box styles (should be identical — only changes which UTF-8 literals are compiled in)
echo ""
echo "Box style sizes (full build):"
full_flags="-DANSI_PRINT_NO_APP_CFG"
for style in "ANSI_BOX_LIGHT:0" "ANSI_BOX_HEAVY:1" "ANSI_BOX_DOUBLE:2" "ANSI_BOX_ROUNDED:3"; do
    name="${style%%:*}"
    val_num="${style##*:}"
    val=$(get_text "$full_flags -DANSI_PRINT_BOX_STYLE=$val_num")
    printf "  %-20s %6s B\n" "$name" "$val"
done

# BSS
echo ""
echo "BSS (RAM) for ansi_print.c:"
$CC $BASE_FLAGS $MINIMAL -c "$SRC" -o "$OUT" 2>/dev/null
bss_min=$(size "$OUT" | awk 'NR==2 { print $3 }')
$CC $BASE_FLAGS -DANSI_PRINT_NO_APP_CFG -c "$SRC" -o "$OUT" 2>/dev/null
bss_full=$(size "$OUT" | awk 'NR==2 { print $3 }')
echo "  Minimal: $bss_min bytes"
echo "  Full:    $bss_full bytes"

# ── TUI feature flags ─────────────────────────────────────────

echo ""
echo "=== TUI widget features (ansi_print.c + ansi_tui.c combined) ==="
echo ""

# TUI minimal baseline: all ANSI_PRINT features enabled, all TUI widgets disabled
tui_min=$(get_text_tui "-DANSI_PRINT_NO_APP_CFG -DANSI_TUI_FRAME=0 -DANSI_TUI_LABEL=0 -DANSI_TUI_BAR=0 -DANSI_TUI_PBAR=0 -DANSI_TUI_STATUS=0 -DANSI_TUI_TEXT=0 -DANSI_TUI_CHECK=0 -DANSI_TUI_METRIC=0")
printf "%-30s %6s B\n" "TUI baseline (no widgets)" "$tui_min"

# Each TUI widget individually on top of TUI baseline
for feat in ANSI_TUI_FRAME ANSI_TUI_LABEL ANSI_TUI_BAR ANSI_TUI_PBAR \
            ANSI_TUI_STATUS ANSI_TUI_TEXT ANSI_TUI_CHECK ANSI_TUI_METRIC; do
    val=$(get_text_tui "-DANSI_PRINT_NO_APP_CFG -DANSI_TUI_FRAME=0 -DANSI_TUI_LABEL=0 -DANSI_TUI_BAR=0 -DANSI_TUI_PBAR=0 -DANSI_TUI_STATUS=0 -DANSI_TUI_TEXT=0 -DANSI_TUI_CHECK=0 -DANSI_TUI_METRIC=0 -D${feat}=1")
    delta=$((val - tui_min))
    printf "%-30s %6s B  (+%d)\n" "$feat" "$val" "$delta"
done

# Full TUI build (all widgets enabled)
tui_full=$(get_text_tui "-DANSI_PRINT_NO_APP_CFG")
tui_full_delta=$((tui_full - tui_min))
printf "%-30s %6s B  (+%d)\n" "Full TUI build" "$tui_full" "$tui_full_delta"

# TUI BSS
echo ""
echo "BSS (RAM) for ansi_tui.c:"
$CC $BASE_FLAGS $MINIMAL -c "$TUI_SRC" -o "$TUI_OUT" 2>/dev/null
tui_bss_min=$(size "$TUI_OUT" | awk 'NR==2 { print $3 }')
$CC $BASE_FLAGS -DANSI_PRINT_NO_APP_CFG -c "$TUI_SRC" -o "$TUI_OUT" 2>/dev/null
tui_bss_full=$(size "$TUI_OUT" | awk 'NR==2 { print $3 }')
echo "  Minimal: $tui_bss_min bytes"
echo "  Full:    $tui_bss_full bytes"
