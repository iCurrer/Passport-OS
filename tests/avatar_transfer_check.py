#!/usr/bin/env python3
"""Host-side logic check for avatar chunked transfer + CRC (TASK-12).

Mirrors main/transport/ble.c on_av_data assembly and CRC verification:
  - chunks appended up to expected size, clamped
  - when full, compute CRC32 (standard poly, == zlib.crc32 == esp_crc32_le)
  - save only if CRC matches
"""
import sys, zlib

AV_SIZE = 80 * 80 * 2  # 12,800
MAX_CHUNK = 244


def assemble(chunks, size):
    """Simulate firmware on_av_data accumulation. Returns (ok, data, consumed_overflow)."""
    buf = bytearray()
    for c in chunks:
        if not c:
            continue
        room = size - len(buf)
        n = min(len(c), room) if room > 0 else 0
        buf += c[:n]
        if len(buf) >= size:
            break
    return bytes(buf)


def verify(buf, expected_crc):
    return zlib.crc32(buf) & 0xFFFFFFFF == expected_crc & 0xFFFFFFFF


fails = 0
def check(name, cond):
    global fails
    print(("PASS" if cond else "FAIL"), name)
    if not cond:
        fails += 1

# deterministic pseudo-random payload (80x80 RGB565)
data = bytes(((i * 7 + i // 3) & 0xFF) for i in range(AV_SIZE))
crc = zlib.crc32(data) & 0xFFFFFFFF

# chunk into MAX_CHUNK pieces
chunks = [data[i:i+MAX_CHUNK] for i in range(0, AV_SIZE, MAX_CHUNK)]

buf = assemble(chunks, AV_SIZE)
check("full payload assembled exactly", len(buf) == AV_SIZE and buf == data)
check("CRC matches (esp_crc32_le==zlib.crc32)", verify(buf, crc))

# corrupt one byte -> CRC mismatch
bad = bytearray(data); bad[5000] ^= 0xFF
check("corrupted payload CRC mismatch", not verify(bytes(bad), crc))

# oversize chunk clamping: send more than size, buffer stops at size
over = chunks + [b'\xAA' * 100]
buf2 = assemble(over, AV_SIZE)
check("oversize clamped to expected size", len(buf2) == AV_SIZE)

# empty/None chunks ignored
buf3 = assemble(chunks + [b'', b'', b''], AV_SIZE)
check("empty chunks skipped, no corruption", buf3 == data)

# count chunks needed
import math
n_chunks = math.ceil(AV_SIZE / MAX_CHUNK)
check(f"chunk count for 12,800B @244 = {n_chunks}", n_chunks == 53)

print("\nRESULT:", "PASS" if fails == 0 else f"{fails} FAILED")
sys.exit(0 if fails == 0 else 1)