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


# --- AAMP: parameter files (always little-endian, even on Wii U) --------------
# https://zeldamods.org/wiki/AAMP
#
# A tree of lists, which hold lists and objects; objects hold parameters. Every
# name is stored only as its crc32, so read_aamp gives {crc: ...} dicts, and
# AampList / AampObject keep the order they were read in.

AAMP_STRING_TYPES = {7, 8, 15, 20}              # String32, String64, String256, StringRef
AAMP_BUFFER_TYPES = {13, 14, 18, 19}            # buffers of int, f32, u32, bytes
AAMP_SIZES = {0: 4, 1: 4, 2: 4, 3: 8, 4: 12, 5: 16, 6: 16, 9: 0x30, 10: 0x60,
              11: 0x90, 12: 0xC0, 16: 16, 17: 4}


class AampList:
    def __init__(self):
        self.lists = {}     # crc -> AampList
        self.objects = {}   # crc -> AampObject


class AampObject:
    def __init__(self):
        self.params = {}    # crc -> (type, raw bytes); strings without their terminator


def crc32(text):
    import zlib
    return zlib.crc32(text.encode()) & 0xFFFFFFFF


def read_aamp(data):
    """(type string, root AampList) from an AAMP file."""
    if data[:4] != b"AAMP" or struct.unpack("<I", data[4:8])[0] != 2:
        raise ValueError("not a version 2 AAMP file")
    pio_offset = struct.unpack("<I", data[0x14:0x18])[0]
    kind = data[0x30:data.index(b"\0", 0x30)].decode()

    def read_list(at):
        crc, lists_at, n_lists, objs_at, n_objs = struct.unpack("<IHHHH", data[at:at + 12])
        out = AampList()
        for i in range(n_lists):
            p = at + lists_at * 4 + i * 12
            out.lists[struct.unpack("<I", data[p:p + 4])[0]] = read_list(p)
        for i in range(n_objs):
            p = at + objs_at * 4 + i * 8
            out.objects[struct.unpack("<I", data[p:p + 4])[0]] = read_object(p)
        return out

    def read_object(at):
        _crc, params_at, n_params = struct.unpack("<IHH", data[at:at + 8])
        out = AampObject()
        for i in range(n_params):
            p = at + params_at * 4 + i * 8
            crc, word = struct.unpack("<II", data[p:p + 8])
            kind_, value_at = word >> 24, p + (word & 0xFFFFFF) * 4
            if kind_ in AAMP_STRING_TYPES:
                raw = data[value_at:data.index(b"\0", value_at)]
            elif kind_ in AAMP_BUFFER_TYPES:
                count = struct.unpack("<I", data[value_at - 4:value_at])[0]
                raw = data[value_at:value_at + count * (1 if kind_ == 19 else 4)]
            else:
                raw = data[value_at:value_at + AAMP_SIZES[kind_]]
            out.params[crc] = (kind_, bytes(raw))
        return out

    return kind, read_list(0x30 + pio_offset)


# --- BYML: the game's data tables, like ActorInfo (big-endian, version 2) ----
# https://zeldamods.org/wiki/BYML
#
# Read into plain Python: dict, list, str, bool, float and int. A number the
# file stored as unsigned comes back as a U32, so writing it back keeps its type.

class U32(int):
    pass


def read_byml(data):
    if data[:2] != b"BY":
        raise ValueError("not a big-endian BYML file")
    keys_at, strings_at, root_at = struct.unpack(">III", data[4:16])

    def string_table(at):
        if not at:
            return []
        count = struct.unpack(">I", data[at:at + 4])[0] & 0xFFFFFF
        offsets = struct.unpack(f">{count + 1}I", data[at + 4:at + 8 + count * 4])
        return [data[at + o:data.index(b"\0", at + o)].decode("utf-8") for o in offsets[:count]]

    keys, strings = string_table(keys_at), string_table(strings_at)

    def value(kind, word):
        if kind == 0xA0:
            return strings[word]
        if kind in (0xC0, 0xC1):
            return node(word)
        if kind == 0xD0:
            return bool(word)
        if kind == 0xD1:
            return struct.unpack(">i", struct.pack(">I", word))[0]
        if kind == 0xD2:
            return struct.unpack(">f", struct.pack(">I", word))[0]
        if kind == 0xD3:
            return U32(word)
        if kind == 0xFF:
            return None
        raise ValueError(f"BYML node type {kind:#x} isn't handled")

    def node(at):
        kind = data[at]
        count = struct.unpack(">I", data[at:at + 4])[0] & 0xFFFFFF
        if kind == 0xC1:
            out = {}
            for i in range(count):
                p = at + 4 + i * 8
                key = struct.unpack(">I", data[p:p + 4])[0] >> 8
                out[keys[key]] = value(data[p + 3], struct.unpack(">I", data[p + 4:p + 8])[0])
            return out
        if kind == 0xC0:
            values_at = at + 4 + ((count + 3) & ~3)
            return [value(data[at + 4 + i], struct.unpack(">I", data[values_at + i * 4:values_at + i * 4 + 4])[0])
                    for i in range(count)]
        raise ValueError(f"BYML root type {kind:#x} isn't handled")

    return node(root_at) if root_at else None


def write_byml(root):
    keys, strings = set(), set()

    def collect(v):
        if isinstance(v, dict):
            for k, x in v.items():
                keys.add(k)
                collect(x)
        elif isinstance(v, list):
            for x in v:
                collect(x)
        elif isinstance(v, str):
            strings.add(v)

    collect(root)
    keys, strings = sorted(keys, key=str.encode), sorted(strings, key=str.encode)
    key_index = {k: i for i, k in enumerate(keys)}
    string_index = {s: i for i, s in enumerate(strings)}

    def table(items):
        if not items:
            return b""
        head = 4 + (len(items) + 1) * 4
        offsets, body = [], b""
        for s in items:
            offsets.append(head + len(body))
            body += s.encode("utf-8") + b"\0"
        offsets.append(head + len(body))
        out = struct.pack(">I", 0xC2 << 24 | len(items)) + struct.pack(f">{len(offsets)}I", *offsets) + body
        return out + b"\0" * (-len(out) % 4)

    key_table, string_table = table(keys), table(strings)
    out = bytearray(16)
    keys_at = len(out) if key_table else 0
    out += key_table
    strings_at = len(out) if string_table else 0
    out += string_table

    def kind_of(v):
        if v is None:
            return 0xFF
        if isinstance(v, bool):
            return 0xD0
        if isinstance(v, U32):
            return 0xD3
        if isinstance(v, int):
            return 0xD1
        if isinstance(v, float):
            return 0xD2
        if isinstance(v, str):
            return 0xA0
        if isinstance(v, list):
            return 0xC0
        if isinstance(v, dict):
            return 0xC1
        raise TypeError(f"can't write {type(v).__name__} to BYML")

    def inline(v):
        kind = kind_of(v)
        if kind == 0xA0:
            return string_index[v]
        if kind == 0xD2:
            return struct.unpack(">I", struct.pack(">f", v))[0]
        if kind in (0xD0, 0xD1, 0xD3):
            return int(v) & 0xFFFFFFFF
        return 0

    written = {}                                # identical containers are written once

    def shape(v):
        if isinstance(v, dict):
            return ("d", tuple((k, shape(x)) for k, x in sorted(v.items())))
        if isinstance(v, list):
            return ("l", tuple(shape(x) for x in v))
        return (kind_of(v), v)

    def write_node(v):
        """Writes a container (and the ones inside it, after it); gives its offset."""
        key = shape(v)
        if key in written:
            return written[key]
        at = written[key] = len(out)
        items = sorted(v.items(), key=lambda kv: kv[0].encode()) if isinstance(v, dict) else list(enumerate(v))
        if isinstance(v, dict):
            out.extend(struct.pack(">I", 0xC1 << 24 | len(items)))
            slots = []
            for k, x in items:
                slots.append(len(out) + 4)
                out.extend(struct.pack(">I", key_index[k] << 8 | kind_of(x)) + struct.pack(">I", inline(x)))
        else:
            out.extend(struct.pack(">I", 0xC0 << 24 | len(items)))
            out.extend(bytes(kind_of(x) for _, x in items) + b"\0" * (-len(items) % 4))
            slots = []
            for _, x in items:
                slots.append(len(out))
                out.extend(struct.pack(">I", inline(x)))
        for slot, (_, x) in zip(slots, items):
            if isinstance(x, (dict, list)):
                out[slot:slot + 4] = struct.pack(">I", write_node(x))
        return at

    root_at = write_node(root) if root is not None else 0
    out[0:16] = b"BY" + struct.pack(">HIII", 2, keys_at, strings_at, root_at)
    return bytes(out)


# --- writing ------------------------------------------------------------------

def yaz0_compress(data):
    """Yaz0 with a simple greedy match finder: quick, and close enough in size."""
    out = bytearray(b"Yaz0" + struct.pack(">III", len(data), 0, 0))
    recent = {}                                 # 3 bytes -> where they last started
    pos, n = 0, len(data)
    while pos < n:
        code_at = len(out)
        out.append(0)
        code = 0
        for bit in range(8):
            if pos >= n:
                break
            key = data[pos:pos + 3]
            start = recent.get(key, -1)
            recent[key] = pos
            length = 0
            if start >= 0 and pos - start <= 0x1000 and pos + 3 <= n:
                limit = min(0x111, n - pos)
                length = 3
                while length < limit and data[start + length] == data[pos + length]:
                    length += 1
            if length >= 3:
                distance = pos - start - 1
                if length >= 0x12:
                    out += bytes((distance >> 8, distance & 0xFF, length - 0x12))
                else:
                    out += bytes(((length - 2) << 4 | distance >> 8, distance & 0xFF))
                for p in range(pos + 1, min(pos + length, n - 2)):
                    recent[data[p:p + 3]] = p
                pos += length
            else:
                code |= 0x80 >> bit
                out.append(data[pos])
                pos += 1
        out[code_at] = code
    return bytes(out)


def write_sarc(files, align=None):
    """A big-endian SARC holding {path: bytes}. align(path) gives a file's
    alignment (default 4)."""
    def name_hash(name):
        h = 0
        for c in name.encode():
            h = (h * 0x65 + c) & 0xFFFFFFFF
        return h

    entries = sorted(files.items(), key=lambda kv: name_hash(kv[0]))
    names, name_offsets = bytearray(), []
    for path, _ in entries:
        name_offsets.append(len(names))
        names += path.encode() + b"\0"
        names += b"\0" * (-len(names) % 4)
    header_size = 0x14 + 0x0C + len(entries) * 16 + 8 + len(names)
    aligns = [(align(path) if align else 4) for path, _ in entries]
    data_start = header_size + (-header_size % max(aligns + [4]))
    blob, nodes = bytearray(), bytearray()
    for (path, data), a, name_at in zip(entries, aligns, name_offsets):
        blob += b"\0" * (-len(blob) % a)
        nodes += struct.pack(">IIII", name_hash(path), 0x01000000 | name_at // 4, len(blob), len(blob) + len(data))
        blob += data
    out = bytearray(b"SARC" + struct.pack(">HHIIHH", 0x14, 0xFEFF, data_start + len(blob), data_start, 0x100, 0))
    out += b"SFAT" + struct.pack(">HHI", 0x0C, len(entries), 0x65) + nodes
    out += b"SFNT" + struct.pack(">HH", 8, 0) + names
    out += b"\0" * (data_start - len(out))
    return bytes(out + blob)


def write_aamp(kind, root):
    """An AAMP file from read_aamp's (type string, AampList)."""
    lists, objects, params = [], [], []         # (crc, item), laid out in this order

    def gather(crc, l):
        lists.append((crc, l))
    queue = [(0xA4F6CB6C, root)]                # crc32("param_root")
    while queue:                                # lists breadth-first, like Nintendo's
        crc, l = queue.pop(0)
        gather(crc, l)
        queue.extend(l.lists.items())
    for _, l in lists:
        objects.extend(l.objects.items())
    for _, o in objects:
        params.extend(o.params.items())

    kind_bytes = kind.encode() + b"\0"
    kind_bytes += b"\0" * (-len(kind_bytes) % 4)
    lists_at = 0x30 + len(kind_bytes)
    objects_at = lists_at + len(lists) * 12
    params_at = objects_at + len(objects) * 8
    data_at = params_at + len(params) * 8

    data, strings, value_at = bytearray(), bytearray(), []
    for _, (t, raw) in params:
        if t in AAMP_BUFFER_TYPES:
            data += struct.pack("<I", len(raw) // (1 if t == 19 else 4))
            value_at.append(("d", len(data)))
            data += raw + b"\0" * (-len(raw) % 4)
        elif t in AAMP_STRING_TYPES:
            value_at.append(("s", len(strings)))
            strings += raw + b"\0"
            strings += b"\0" * (-len(strings) % 4)
        else:
            value_at.append(("d", len(data)))
            data += raw
    strings_at = data_at + len(data)

    out = bytearray()
    list_index = {id(l): i for i, (_, l) in enumerate(lists)}
    obj_index = {id(o): i for i, (_, o) in enumerate(objects)}
    param_index = {}
    i = 0
    for _, o in objects:
        param_index[id(o)] = i
        i += len(o.params)
    for i, (crc, l) in enumerate(lists):
        here = lists_at + i * 12
        sub = list_index[id(next(iter(l.lists.values())))] if l.lists else 0
        obj = obj_index[id(next(iter(l.objects.values())))] if l.objects else 0
        out += struct.pack("<IHHHH", crc,
                           (lists_at + sub * 12 - here) // 4 if l.lists else 0, len(l.lists),
                           (objects_at + obj * 8 - here) // 4 if l.objects else 0, len(l.objects))
    for i, (crc, o) in enumerate(objects):
        here = objects_at + i * 8
        first = params_at + param_index[id(o)] * 8
        out += struct.pack("<IHH", crc, (first - here) // 4 if o.params else 0, len(o.params))
    for i, (crc, (t, _)) in enumerate(params):
        here = params_at + i * 8
        section, offset = value_at[i]
        target = (data_at if section == "d" else strings_at) + offset
        out += struct.pack("<II", crc, t << 24 | (target - here) // 4)
    body = bytes(out) + bytes(data) + bytes(strings)
    size = 0x30 + len(kind_bytes) + len(body)
    head = b"AAMP" + struct.pack("<IIIIIIIIIII", 2, 3, size, 0, len(kind_bytes), len(lists), len(objects),
                                 len(params), len(data), len(strings), 0)
    return head + kind_bytes + body


def msbt_add(data, entries):
    """data with more {label: text} messages added (UTF-16 MSBT, as BotW's are)."""
    sections = struct.unpack(">H", data[0x0E:0x10])[0]
    at, raw = 0x20, {}
    for _ in range(sections):
        size = struct.unpack(">I", data[at + 4:at + 8])[0]
        raw[data[at:at + 4]] = data[at + 16:at + 16 + size]
        at += 16 + ((size + 15) & ~15)
    if set(raw) != {b"LBL1", b"ATR1", b"TXT2"}:
        raise ValueError(f"unexpected MSBT sections {sorted(raw)}")

    lbl = raw[b"LBL1"]
    buckets = struct.unpack(">I", lbl[:4])[0]
    labels = [[] for _ in range(buckets)]
    for b in range(buckets):
        count, offset = struct.unpack(">II", lbl[4 + b * 8:12 + b * 8])
        p = offset
        for _ in range(count):
            length = lbl[p]
            labels[b].append((lbl[p + 1:p + 1 + length], struct.unpack(">I", lbl[p + 1 + length:p + 5 + length])[0]))
            p += 5 + length

    txt = raw[b"TXT2"]
    count = struct.unpack(">I", txt[:4])[0]
    offsets = list(struct.unpack(f">{count}I", txt[4:4 + count * 4])) + [len(txt)]
    texts = [txt[offsets[i]:offsets[i + 1]] for i in range(count)]

    atr = raw[b"ATR1"]
    atr_count, atr_size = struct.unpack(">II", atr[:8])
    if atr_size != 0 or atr_count != count:
        raise ValueError("MSBT attributes with data aren't handled")

    def bucket_of(label):
        h = 0
        for c in label:
            h = (h * 0x492 + c) & 0xFFFFFFFF
        return h % buckets

    for label, text in entries.items():
        label = label.encode()
        if any(label == l for bucket in labels for l, _ in bucket):
            raise ValueError(f"MSBT already has {label.decode()}")
        labels[bucket_of(label)].append((label, len(texts)))
        texts.append(text.encode("utf-16-be") + b"\0\0")

    new_lbl = bytearray(struct.pack(">I", buckets))
    body = bytearray()
    head = 4 + buckets * 8
    for bucket in labels:
        new_lbl += struct.pack(">II", len(bucket), head + len(body))
        for label, index in bucket:
            body += bytes([len(label)]) + label + struct.pack(">I", index)
    new_lbl += body
    new_txt = bytearray(struct.pack(">I", len(texts)))
    head = 4 + len(texts) * 4
    blob = bytearray()
    for t in texts:
        new_txt += struct.pack(">I", head + len(blob))
        blob += t
    new_txt += blob
    new_atr = struct.pack(">II", len(texts), 0)

    out = bytearray(data[:0x20])
    for magic, body in ((b"LBL1", new_lbl), (b"ATR1", new_atr), (b"TXT2", new_txt)):
        out += magic + struct.pack(">I", len(body)) + b"\0" * 8 + body
        out += b"\xAB" * (-len(out) % 16)
    out[0x12:0x16] = struct.pack(">I", len(out))
    return bytes(out)


# --- RSTB: how much memory the game sets aside for each file ------------------
# https://zeldamods.org/wiki/RSTB
# Entries are keyed by the crc32 of the path without the "s" that marks Yaz0
# ("Actor/Pack/X.bactorpack"); a few paths with clashing crcs are stored by name.

def read_rstb(data):
    """({crc: size}, {name: size})"""
    crc_count, name_count = struct.unpack(">II", data[4:12])
    crcs = dict(struct.unpack(">II", data[12 + i * 8:20 + i * 8]) for i in range(crc_count))
    names, at = {}, 12 + crc_count * 8
    for _ in range(name_count):
        names[data[at:at + 128].split(b"\0")[0].decode()] = struct.unpack(">I", data[at + 128:at + 132])[0]
        at += 132
    return crcs, names


def write_rstb(crcs, names):
    out = bytearray(b"RSTB" + struct.pack(">II", len(crcs), len(names)))
    for crc in sorted(crcs):
        out += struct.pack(">II", crc, crcs[crc])
    for name in sorted(names):
        out += name.encode().ljust(128, b"\0") + struct.pack(">I", names[name])
    return bytes(out)
