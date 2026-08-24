#!/usr/bin/env python3
"""Host-side logic check for SETTINGS sleep timeout cycle (TASK-13).

Mirrors main/apps/settings/settings_page.c:
  - find_sleep_idx(): match current timeout to option (default 0)
  - operate SLEEP: next = (idx + 1) % N
  - UP/DOWN selection cycling
"""
import sys

OPTS = [30, 60, 120, 300, 0]
LABELS = ["30s", "1m", "2m", "5m", "never"]
N = len(OPTS)
SEL_COUNT = 3  # BLE / SLEEP / VERSION


def find_idx(cur):
    for i, v in enumerate(OPTS):
        if v == cur:
            return i
    return 0


def next_sleep(cur):
    return OPTS[(find_idx(cur) + 1) % N]


def up(sel):
    return (sel + SEL_COUNT - 1) % SEL_COUNT


def down(sel):
    return (sel + 1) % SEL_COUNT


fails = 0
def check(name, cond):
    global fails
    print(("PASS" if cond else "FAIL"), name)
    if not cond:
        fails += 1

# find_sleep_idx
check("cur=60 -> idx 1", find_idx(60) == 1)
check("cur=0 (never) -> idx 4", find_idx(0) == 4)
check("cur=123 (unknown) -> 0", find_idx(123) == 0)

# sleep cycle
check("30 -> 60", next_sleep(30) == 60)
check("300 -> 0 (never)", next_sleep(300) == 0)
check("0 -> 30 (wrap)", next_sleep(0) == 30)

# selection cycling (UP/DOWN), 3 items
check("down 0->1", down(0) == 1)
check("down 2->0", down(2) == 0)
check("up 0->2", up(0) == 2)

print("\nRESULT:", "PASS" if fails == 0 else f"{fails} FAILED")
sys.exit(0 if fails == 0 else 1)