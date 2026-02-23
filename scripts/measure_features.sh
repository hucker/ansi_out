#!/bin/bash
# Measure .text section size for each feature flag individually.
# Usage: bash measure_features.sh
# Requires: clang (or set CC), size

CC="${CC:-clang}"
SRC="src/ansi_print.c"
OUT="build/ansi_measure.o"
BASE_FLAGS="-Os -ffunction-sections -fdata-sections -std=c99 -I src"
MINIMAL="-DANSI_PRINT_NO_APP_CFG -DANSI_PRINT_MINIMAL"

mkdir -p build

get_text() {
    $CC $BASE_FLAGS $1 -c "$SRC" -o "$OUT" 2>/dev/null
    size "$OUT" | awk 'NR==2 { print $1 }'
}

echo "Measuring .text sizes with $CC ..."
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
echo "BSS (RAM):"
$CC $BASE_FLAGS $MINIMAL -c "$SRC" -o "$OUT" 2>/dev/null
bss_min=$(size "$OUT" | awk 'NR==2 { print $3 }')
$CC $BASE_FLAGS -DANSI_PRINT_NO_APP_CFG -c "$SRC" -o "$OUT" 2>/dev/null
bss_full=$(size "$OUT" | awk 'NR==2 { print $3 }')
echo "  Minimal: $bss_min bytes"
echo "  Full:    $bss_full bytes"
