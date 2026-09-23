"""Usage: python3 tools/hiwonder/merge-div.py; validate and merge Arduino build output."""
from pathlib import Path
import hashlib
import json
import struct
import subprocess

root = Path(__file__).resolve().parents[2]
build = root / '.build'
boot = (build / 'ESP32-DIV.ino.bootloader.bin').read_bytes()
partitions = (build / 'ESP32-DIV.ino.partitions.bin').read_bytes()
app = (build / 'ESP32-DIV.ino.bin').read_bytes()
assert boot[0] == app[0] == 0xE9
assert struct.unpack_from('<H', boot, 12)[0] == 9
assert struct.unpack_from('<H', app, 12)[0] == 9
assert partitions[:2] == b'\xaa\x50'
assert len(boot) <= 0x8000 and len(partitions) <= 0x1000
assert boot[3] >> 4 == 4, 'Bootloader flash size must be 16 MB'
entries = []
for index in range(0, len(partitions), 32):
    record = partitions[index:index + 32]
    if record[:2] in (b'\xff\xff', b'\xeb\xeb'):
        break
    magic, kind, subtype, offset, size, label, flags = struct.unpack('<HBBII16sI', record)
    assert magic == 0x50AA and flags == 0
    entries.append((label.rstrip(b'\x00').decode(), kind, subtype, offset, size))
assert entries == [('nvs', 1, 2, 0x9000, 0x5000),
                   ('otadata', 1, 0, 0xE000, 0x2000),
                   ('app0', 0, 0x10, 0x10000, 0x600000),
                   ('spiffs', 1, 0x82, 0x610000, 0x9F0000)], 'Unexpected partition layout'
assert len(app) < 0x600000
# With erased otadata the ESP-IDF bootloader selects the first valid OTA app.
image = bytearray(b'\xff' * (0x10000 + len(app)))
image[:len(boot)] = boot
image[0x8000:0x8000 + len(partitions)] = partitions
image[0x10000:] = app
out = root / 'artifacts' / 'ESP32-DIV-v1.7.2-Hiwonder.bin'
out.write_bytes(image)
try:
    source_commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root,
                                            text=True, stderr=subprocess.DEVNULL).strip()
except (OSError, subprocess.CalledProcessError):
    source_commit = None  # A source archive can build without a Git checkout.
metadata = {'file': out.name, 'bytes': len(image), 'sha256': hashlib.sha256(image).hexdigest(),
            'offset': 0, 'chip': 'ESP32-S3', 'flash': '16MB', 'psram': '8MB OPI',
            'source_commit': source_commit,
            'runtime_verified': False}
(out.parent / 'build.json').write_text(json.dumps(metadata, indent=2) + '\n')
print(json.dumps(metadata, indent=2))
