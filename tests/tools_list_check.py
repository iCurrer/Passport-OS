#!/usr/bin/env python3
"""Host-side logic check for TOOLS list selection cycling (TASK-08).

Mirrors tools_key UP/DOWN cursor math in main/apps/tools/tools.c:
  UP   : sel = (sel + COUNT - 1) % COUNT
  DOWN : sel = (sel + 1) % COUNT
"""
import sys

COUNT = 4


def up(sel):
    return (sel + COUNT - 1) % COUNT


def down(sel):
    return (sel + 1) % COUNT


fails = 0
def check(name, cond):
    global fails
    print(("PASS" if cond else "FAIL"), name)
    if not cond:
        fails += 1

check("DOWN from 0 -> 1", down(0) == 1)
check("DOWN from 3 -> 0 (wrap)", down(3) == 0)
check("UP from 3 -> 2", up(3) == 2)
check("UP from 0 -> 3 (wrap)", up(0) == 3)
check("DOWN from 1 -> 2", down(1) == 2)
check("UP from 2 -> 1", up(2) == 1)

# full loop returns to start
sel = 0
for _ in range(COUNT):
    sel = down(sel)
check("4x DOWN returns to 0", sel == 0)
sel = 3
for _ in range(COUNT):
    sel = up(sel)
check("4x UP from 3 returns to 3", sel == 3)

print("\nRESULT:", "PASS" if fails == 0 else f"{fails} FAILED")
sys.exit(0 if fails == 0 else 1)