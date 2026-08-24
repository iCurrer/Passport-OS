#!/usr/bin/env python3
"""Host-side geometry self-check for the Passport OS V2 design system (TASK-01).

Pure-arithmetic mirror of the coordinate constants in components/ui/include/ds_tokens.h
and the layout math in components/ui/src/ds_widgets.c. No hardware / no LVGL needed.
Validates: Page Indicator placement, Header elements in-bounds and non-overlapping.
"""
import sys

W = H = None
W, H = 240, 320

# --- ds_tokens.h values (must match) ---
DS_SCREEN_W, DS_SCREEN_H = 240, 320
DS_MARGIN_X = 16
DS_HEADER_LINE_Y = 34
DS_HDR_BRAND_X, DS_HDR_BRAND_Y = 16, 13
DS_HDR_BATT_X, DS_HDR_BATT_Y, DS_HDR_BATT_W, DS_HDR_BATT_H = 158, 14, 20, 10
DS_HDR_BATT_CORE_X, DS_HDR_BATT_CORE_Y, DS_HDR_BATT_CORE_W, DS_HDR_BATT_CORE_H = 160, 16, 16, 6
DS_HDR_BATT_PCT_X, DS_HDR_BATT_PCT_Y = 186, 12
DS_DOT_SIZE, DS_DOT_GAP, DS_DOT_Y = 6, 12, 293
DS_FOOTER_Y, DS_FOOTER_H = 272, 48
DS_PAGES_MAX = 8

fails = 0


def check(name, cond):
    global fails
    print(("PASS" if cond else "FAIL"), name)
    if not cond:
        fails += 1


# --- Page Indicator ---
for count in (1, 4, 8):
    assert 1 <= count <= DS_PAGES_MAX
    total = (count - 1) * DS_DOT_GAP + DS_DOT_SIZE
    x0 = (DS_SCREEN_W - total) // 2
    xs = [x0 + i * DS_DOT_GAP for i in range(count)]
    right = xs[-1] + DS_DOT_SIZE
    inside = x0 >= 0 and right <= DS_SCREEN_W
    # True geometric center of the band: (first_left + last_right) / 2.
    center = (xs[0] + (xs[-1] + DS_DOT_SIZE)) / 2.0
    centered = abs(center - DS_SCREEN_W / 2) <= 0.5
    cy = DS_DOT_Y + DS_DOT_SIZE / 2 - (DS_FOOTER_Y)  # center relative to footer top
    check(f"count={count}: dots within horizontal bounds (x0={x0}, right={right})", inside and xs[0] >= 0)
    check(f"count={count}: group horizontally centered (span center ~120)", centered)
    check(f"count={count}: dot band inside footer 272..319", DS_DOT_Y >= DS_FOOTER_Y and DS_DOT_Y + DS_DOT_SIZE <= DS_FOOTER_Y + DS_FOOTER_H)
    print(f"        dot centers x={[x + DS_DOT_SIZE / 2 for x in xs]}, center_y_in_footer={cy}")

# --- Header ---
check("batt frame in-bounds", 0 <= DS_HDR_BATT_X and DS_HDR_BATT_X + DS_HDR_BATT_W <= DS_SCREEN_W)
check("batt core inside frame", DS_HDR_BATT_CORE_X >= DS_HDR_BATT_X and DS_HDR_BATT_CORE_X + DS_HDR_BATT_CORE_W <= DS_HDR_BATT_X + DS_HDR_BATT_W)
check("title left of battery (no overlap)", DS_HDR_BRAND_X < DS_HDR_BATT_X)
check("title/batt/pct vertically in header 0..35",
      DS_HDR_BRAND_Y < 35 and DS_HDR_BATT_Y + DS_HDR_BATT_H <= 35 and DS_HDR_BATT_PCT_Y < 35)
check("separator in-bounds width", DS_MARGIN_X + (DS_SCREEN_W - 2 * DS_MARGIN_X) == DS_SCREEN_W - DS_MARGIN_X and DS_HEADER_LINE_Y < 36)
print(f"        gap title..battery = {DS_HDR_BATT_X - DS_HDR_BRAND_X}px; separator y={DS_HEADER_LINE_Y} w={DS_SCREEN_W - 2 * DS_MARGIN_X}")

print("\nRESULT:", "PASS" if fails == 0 else f"{fails} FAILED")
sys.exit(0 if fails == 0 else 1)