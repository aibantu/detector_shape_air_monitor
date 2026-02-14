from pathlib import Path
p = Path(r"I:\Eyes_tft-main\Eyes_tft\src\screenDisplay.cpp")
b = p.read_bytes()
keep = {9, 10, 13}
out = bytearray()
removed = 0
for ch in b:
    if ch < 32 and ch not in keep:
        removed += 1
        continue
    out.append(ch)
if removed:
    bak = p.with_suffix(p.suffix + ".bak")
    bak.write_bytes(b)
    p.write_bytes(bytes(out))
print(f"screenDisplay.cpp: removed {removed} control bytes")
