#!/usr/bin/env python3
"""Extract an RT_GROUP_ICON from a Windows executable."""

import struct
import sys
from pathlib import Path

import pefile


def resource_data(pe, entry):
    data = entry.directory.entries[0].data.struct
    return pe.get_memory_mapped_image()[data.OffsetToData:data.OffsetToData + data.Size]


def main():
    source = Path(sys.argv[1])
    target = Path(sys.argv[2])
    pe = pefile.PE(str(source))
    types = {entry.id: entry for entry in pe.DIRECTORY_ENTRY_RESOURCE.entries}
    icons = {}
    for entry in types[3].directory.entries:  # RT_ICON
        icons[entry.id] = resource_data(pe, entry)

    requested_group = int(sys.argv[3]) if len(sys.argv) > 3 else None
    groups = []
    for entry in types[14].directory.entries:  # RT_GROUP_ICON
        raw = resource_data(pe, entry)
        count = struct.unpack_from("<H", raw, 4)[0]
        groups.append((entry.id, count, raw))
    if requested_group is None:
        _, _, group = max(groups, key=lambda item: item[1])
    else:
        matches = [item for item in groups if item[0] == requested_group]
        if not matches:
            raise SystemExit(f"RT_GROUP_ICON {requested_group} was not found")
        _, _, group = matches[0]
    count = struct.unpack_from("<H", group, 4)[0]
    images = []
    entries = []
    offset = 6 + count * 16
    for index in range(count):
        width, height, colors, reserved, planes, bits, size, icon_id = \
            struct.unpack_from("<BBBBHHIH", group, 6 + index * 14)
        image = icons[icon_id]
        entries.append(struct.pack("<BBBBHHII", width, height, colors, reserved,
                                   planes, bits, len(image), offset))
        images.append(image)
        offset += len(image)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(struct.pack("<HHH", 0, 1, count) + b"".join(entries) + b"".join(images))
    print(f"Extracted {count} icon sizes to {target}")


if __name__ == "__main__":
    main()
