"""Reads actor names out of a BotW (Wii U) dump, for `wxl actors`.

Everything comes from the user's own game files; nothing is bundled.

  * Actor names: every actor the game can create has an actor pack, either as a
    loose file in content/Actor/Pack/ or inside TitleBG.pack (the actors that
    are always loaded, like arrows and insects).
  * English names: the game's own text, in Bootup_USen.pack ->
    Message/Msg_USen.product.ssarc -> ActorType/*.msbt, under labels like
    "Enemy_Bokoblin_Junior_Name". Only things the game names on screen have one.

The file formats, all big-endian on Wii U:
  Yaz0  compression        https://zeldamods.org/wiki/Yaz0
  SARC  archive            https://zeldamods.org/wiki/SARC
  MSBT  message text       https://zeldamods.org/wiki/MSBT
"""

import struct
from pathlib import Path


def yaz0_decompress(data):
    if data[:4] != b"Yaz0":
        return data
    size = struct.unpack(">I", data[4:8])[0]
    out = bytearray(size)
    src, dst = 16, 0
    while dst < size:
        code = data[src]
        src += 1
        for bit in range(8):
            if dst >= size:
                break
            if code & (0x80 >> bit):            # a literal byte
                out[dst] = data[src]
                dst += 1
                src += 1
                continue
            b1, b2 = data[src], data[src + 1]   # a copy from earlier output
            src += 2
            distance = ((b1 & 0x0F) << 8 | b2) + 1
            count = b1 >> 4
            if count == 0:
                count = data[src] + 0x12
                src += 1
            else:
                count += 2
            start = dst - distance
            for i in range(count):
                out[dst] = out[start + i]
                dst += 1
    return bytes(out)


def sarc_files(data):
    """{path: bytes} for every named file in a SARC archive."""
    if data[:4] != b"SARC":
        return {}
    data_start = struct.unpack(">I", data[0x0C:0x10])[0]
    count = struct.unpack(">H", data[0x1A:0x1C])[0]
    nodes = 0x20
    names = nodes + count * 16 + 8              # past the nodes and the SFNT header
    files = {}
    for i in range(count):
        _hash, attrs, start, end = struct.unpack(">IIII", data[nodes + i * 16:nodes + i * 16 + 16])
        if not attrs & 0x01000000:              # unnamed file
            continue
        at = names + (attrs & 0xFFFF) * 4
        name = data[at:data.index(b"\0", at)].decode("utf-8", "replace")
        files[name] = data[data_start + start:data_start + end]
    return files


def msbt_texts(data):
    """{label: text} from an MSBT file, with formatting tags left out."""
    if data[:8] != b"MsgStdBn":
        return {}
    sections = struct.unpack(">H", data[0x0E:0x10])[0]
    at = 0x20
    labels, texts = {}, []
    for _ in range(sections):
        magic = data[at:at + 4]
        size = struct.unpack(">I", data[at + 4:at + 8])[0]
        body = at + 16
        if magic == b"LBL1":
            buckets = struct.unpack(">I", data[body:body + 4])[0]
            for b in range(buckets):
                count, offset = struct.unpack(">II", data[body + 4 + b * 8:body + 12 + b * 8])
                p = body + offset
                for _ in range(count):
                    length = data[p]
                    label = data[p + 1:p + 1 + length].decode("utf-8", "replace")
                    labels[label] = struct.unpack(">I", data[p + 1 + length:p + 5 + length])[0]
                    p += 5 + length
        elif magic == b"TXT2":
            count = struct.unpack(">I", data[body:body + 4])[0]
            offsets = [struct.unpack(">I", data[body + 4 + i * 4:body + 8 + i * 4])[0]
                       for i in range(count)]
            for i, offset in enumerate(offsets):
                p = body + offset
                end = body + (offsets[i + 1] if i + 1 < count else size)
                chars = []
                while p + 1 < end:
                    c = struct.unpack(">H", data[p:p + 2])[0]
                    p += 2
                    if c == 0:
                        break
                    if c == 0x0E:                # a tag: group, type, size, payload
                        p += 6 + struct.unpack(">H", data[p + 4:p + 6])[0]
                        continue
                    chars.append(chr(c))
                texts.append("".join(chars))
        at = body + ((size + 15) & ~15)
    return {label: texts[i] for label, i in labels.items() if i < len(texts)}


def actor_names(content_dirs):
    """Every actor with an actor pack, from loose packs and TitleBG.pack."""
    names = set()
    for content in content_dirs:
        for pack in (Path(content) / "Actor" / "Pack").glob("*.sbactorpack"):
            names.add(pack.name[:-len(".sbactorpack")])
        title = Path(content) / "Pack" / "TitleBG.pack"
        if title.exists():
            for path in sarc_files(title.read_bytes()):
                if path.startswith("Actor/Pack/") and path.endswith(".sbactorpack"):
                    names.add(path[len("Actor/Pack/"):-len(".sbactorpack")])
    return names


def english_names(content_dirs):
    """{actor name: English name} from the game's own text (first dump that has it)."""
    for content in content_dirs:
        packs = sorted((Path(content) / "Pack").glob("Bootup_*en.pack"))
        packs.sort(key=lambda p: p.name != "Bootup_USen.pack")   # US English first
        for pack in packs:
            files = sarc_files(pack.read_bytes())
            messages = next((data for path, data in files.items()
                             if path.startswith("Message/Msg_")), None)
            if messages is None:
                continue
            names = {}
            for path, data in sarc_files(yaz0_decompress(messages)).items():
                if path.startswith("ActorType/"):
                    for label, text in msbt_texts(data).items():
                        if label.endswith("_Name"):
                            names[label[:-len("_Name")]] = text
            return names
    return {}
