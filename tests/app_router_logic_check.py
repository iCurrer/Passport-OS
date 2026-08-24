#!/usr/bin/env python3
"""Host-side logic check for the App Router (TASK-02).

Mirrors the pure parts of main/app/app_router.c:
  - app_router_page_cycle(): cyclic page stepping with wrap + clamp
  - map_event():           (btn, ev) -> route intent
No hardware / no LVGL needed.
"""
import sys

APP_PAGE_COUNT = 8
HOME = 0
SETTINGS = APP_PAGE_COUNT - 1

# === app_router_page_cycle mirror ===
def page_cycle(cur, dirn):
    if cur < 0 or cur >= APP_PAGE_COUNT:
        cur = HOME
    return (cur + dirn + APP_PAGE_COUNT) % APP_PAGE_COUNT

# === map_event mirror; bsp_btn_t / bsp_btn_ev_t values from bsp_button.h ===
# 注意:意图枚举用 INT_* 前缀,避免与页面常量 HOME(页码 0) 命名冲突遮蔽。
UP, DOWN, OK = 0, 1, 2
PRESS, CLICK, DOUBLE, LONG = 0, 1, 2, 3
INT_NONE, INT_PREV, INT_NEXT, INT_OK, INT_HOME, INT_TOGGLE = range(6)

def map_event(btn, ev):
    if ev == CLICK:
        if btn == UP: return INT_PREV
        if btn == DOWN: return INT_NEXT
        if btn == OK: return INT_OK
    elif ev == LONG:
        if btn == UP: return INT_HOME
        if btn == OK: return INT_HOME
        if btn == DOWN: return INT_TOGGLE
    return INT_NONE

fails = 0
def check(name, cond):
    global fails
    print(("PASS" if cond else "FAIL"), name)
    if not cond:
        fails += 1

# ---- cycle: wrap both directions and clamp ----
def cy(cur, d):
    return page_cycle(cur, d)

check("cur=0, step +1 -> 1", cy(HOME, +1) == 1)
check("cur=SETTINGS, step +1 -> HOME (wrap)", cy(SETTINGS, +1) == HOME)
check("cur=HOME, step -1 -> SETTINGS (wrap)", cy(HOME, -1) == SETTINGS)
check("cur=3, step +1 -> 4", cy(3, +1) == 4)
check("cur=3, step -1 -> 2", cy(3, -1) == 2)
check("invalid cur(99) clamped to HOME then steps -> 1", cy(99, +1) == 1)
check("invalid cur(-1) -> HOME (clamp)", page_cycle(-1, 0) == HOME)
check("wrap stays in [0,7] across 100 steps",
      all(0 <= page_cycle((i * 7) % 8, (1 if i % 2 else -1)) < APP_PAGE_COUNT for i in range(100)))

# ---- key mapping ----
check("UP+CLICK -> PREV", map_event(UP, CLICK) == INT_PREV)
check("DOWN+CLICK -> NEXT", map_event(DOWN, CLICK) == INT_NEXT)
check("OK+CLICK -> OK_ACTION", map_event(OK, CLICK) == INT_OK)
check("OK+LONG -> HOME", map_event(OK, LONG) == INT_HOME)
check("UP+LONG -> HOME", map_event(UP, LONG) == INT_HOME)
check("DOWN+LONG -> STATUS_TOGGLE", map_event(DOWN, LONG) == INT_TOGGLE)
check("UP+PRESS -> NONE", map_event(UP, PRESS) == INT_NONE)
check("DOWN+DOUBLE -> NONE", map_event(DOWN, DOUBLE) == INT_NONE)
check("OK+PRESS -> NONE", map_event(OK, PRESS) == INT_NONE)

print("\nRESULT:", "PASS" if fails == 0 else f"{fails} FAILED")
sys.exit(0 if fails == 0 else 1)