#!/usr/bin/env python3
"""Host-side logic check for STATUS cycle (TASK-05).

Mirrors the pure parts of main/apps/status/status.c:
  - find_index(): current status -> preset index (or -1)
  - status_cycle(): next preset selection (wrap; legacy non-preset -> AVAILABLE)
"""
import sys

PRESETS = ["AVAILABLE", "FOCUS", "BUSY", "DND", "OFFLINE"]
COUNT = len(PRESETS)


def find_index(st):
    for i, p in enumerate(PRESETS):
        if st and st == p:
            return i
    return -1


def next_index(cur):
    i = find_index(cur)
    return 0 if i < 0 else (i + 1) % COUNT


fails = 0
def check(name, cond):
    global fails
    print(("PASS" if cond else "FAIL"), name)
    if not cond:
        fails += 1

# find_index
check("find AVAILABLE -> 0", find_index("AVAILABLE") == 0)
check("find OFFLINE -> 4", find_index("OFFLINE") == 4)
check("find legacy '自由' -> -1", find_index("自由") == -1)
check("find empty -> -1", find_index("") == -1)
check("find None -> -1", find_index(None) == -1)

# cycle next
check("AVAILABLE -> FOCUS", next_index("AVAILABLE") == 1)
check("FOCUS -> BUSY", next_index("FOCUS") == 2)
check("BUSY -> DND", next_index("BUSY") == 3)
check("DND -> OFFLINE", next_index("DND") == 4)
check("OFFLINE -> AVAILABLE (wrap)", next_index("OFFLINE") == 0)
check("legacy '自由' -> AVAILABLE(0)", next_index("自由") == 0)
check("empty -> AVAILABLE(0)", next_index("") == 0)

# full 5-step loop returns to start
cur = "AVAILABLE"
for _ in range(COUNT):
    cur = PRESETS[next_index(cur)]
check("5-step cycle returns to AVAILABLE", cur == "AVAILABLE")

print("\nRESULT:", "PASS" if fails == 0 else f"{fails} FAILED")
sys.exit(0 if fails == 0 else 1)