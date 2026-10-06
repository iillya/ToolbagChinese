"""Create a full key/value oracle using Python's independent JSON decoder."""
import json
from pathlib import Path
import struct

root = Path(__file__).resolve().parents[2]
entries = json.loads((root / 'translations/dictionary_zh.json').read_text(encoding='utf-8-sig'))['translations']
entries = {key: value for key, value in entries.items() if key and value}
target = root / 'build/tests/dictionary-corpus.bin'
target.parent.mkdir(parents=True, exist_ok=True)
with target.open('wb') as output:
    output.write(struct.pack('<I', len(entries)))
    for pair in entries.items():
        for value in pair:
            encoded = value.encode('utf-8')
            output.write(struct.pack('<I', len(encoded)))
            output.write(encoded)
print(f'Independent oracle: {len(entries)} key/value pairs')
