"""Builds new items into the game's data, for `wxl items`.

The game knows an item from several files, and a new one needs an entry in
each. Everything is made from the user's own dump and written into the
WiiXLaunch graphic pack's content/ folder, which Cemu reads in place of the
game's files; nothing from the game is bundled.

  Actor/Pack/<name>.sbactorpack   the item itself: a copy of an existing weapon's
                                  pack with a few parameter files changed
  Actor/ActorInfo.product.sbyml   the list of every actor, with a summary of each
  Pack/Bootup_<lang>.pack         the item's name and description, per language
  UI/StockItem/<name>.sbitemico   the picture in the inventory
  System/Resource/ResourceSizeTable.product.srsizetable
                                  how much memory each file needs; every new or
                                  bigger file needs its entry, or the game may crash

Only weapons copied from an existing weapon are supported. Each NewWeapon
below describes one.
"""

import struct
from dataclasses import dataclass, field

import botw_files as bf
from botw_files import crc32

# Param names used below (AAMP files store only their crc32).
LINK_TARGET, TAGS = crc32("LinkTarget"), crc32("Tags")
CONTROLLER_INFO = crc32("ControllerInfo")


@dataclass
class NewWeapon:
    name: str              # the new actor name, e.g. "Weapon_Sword_900"
    copy_of: str           # the weapon whose model, physics and sounds it uses
    english_name: str
    description: str
    links: dict = field(default_factory=dict)       # ActorLink users to change: {"AIProgramUser": "EquipItemBase"}
    borrow_from: str = ""  # a weapon whose pack has the files the new links need
    drop_tags: tuple = ()
    params: dict = field(default_factory=dict)      # GeneralParamList: {"Param": value}
    # The model list's MulColor / AddColor tint made no visible difference to
    # the Ice Rod in the game, so only the size is changed.
    scale: tuple = (1.0, 1.0, 1.0)


SPAWNING_WAND = NewWeapon(
    name="Weapon_Sword_900",
    copy_of="Weapon_Sword_061",                     # the Ice Rod
    english_name="Spawning Wand",
    description="A wand that conjures whatever you choose.\n"
                "Press Up or Down to choose, then swing it.",
    # A plain weapon's AI and animations instead of the rod's, so it doesn't shoot.
    links={"AIProgramUser": "EquipItemBase", "ASUser": "EquipItemCommon"},
    borrow_from="Weapon_Sword_001",
    drop_tags=("IceRodSeries", "RodWeapon", "CoolWeapon", "ZukanWeapon"),
    params={"WeaponSubType": ""},                   # swung like a sword, not cast like a rod
    scale=(1.1, 1.1, 1.1),
)

NEW_ITEMS = [SPAWNING_WAND]


# --- reading the dump ---------------------------------------------------------

class Dump:
    """The game's files, the update's copy first."""

    def __init__(self, content_dirs):
        self.dirs = content_dirs

    def read(self, path):
        for d in self.dirs:
            if (d / path).exists():
                return (d / path).read_bytes()
        raise FileNotFoundError(f"{path} isn't in the game files")

    def glob(self, pattern):
        seen = {}
        for d in self.dirs:
            for p in sorted(d.glob(pattern)):
                seen.setdefault(p.name, p)
        return list(seen.values())


def unpack_actor(dump, name):
    return bf.sarc_files(bf.yaz0_decompress(dump.read(f"Actor/Pack/{name}.sbactorpack")))


def find_param(root, param_name):
    """(object, crc) for a param, looking in every object of the file."""
    stack = [root]
    while stack:
        l = stack.pop()
        for o in l.objects.values():
            if crc32(param_name) in o.params:
                return o, crc32(param_name)
        stack.extend(l.lists.values())
    raise KeyError(f"no param {param_name}")


def string(raw):
    return raw.decode("utf-8")


# --- building one weapon ------------------------------------------------------

def build_weapon(dump, item, rstb):
    """{content path: bytes} for the weapon's own files; updates rstb (crc -> size)."""
    files = unpack_actor(dump, item.copy_of)
    link_path = f"Actor/ActorLink/{item.copy_of}.bxml"
    kind, link = bf.read_aamp(files.pop(link_path))
    target = link.objects[LINK_TARGET]
    users = {crc: string(raw) for crc, (t, raw) in target.params.items() if t in bf.AAMP_STRING_TYPES}

    def user(name):
        return users[crc32(name)]

    def set_user(name, value):
        target.params[crc32(name)] = (20, value.encode())

    resized = {}                                # new path -> path of the file it grew from

    # Files named after the item get the new name, so they can't be mixed up
    # with the original's (the game caches files by path).
    gparam_path = f"Actor/GeneralParamList/{user('GParamUser')}.bgparamlist"
    model_path = f"Actor/ModelList/{user('ModelUser')}.bmodellist"
    kind_g, gparam = bf.read_aamp(files.pop(gparam_path))
    kind_m, model = bf.read_aamp(files.pop(model_path))

    for param, value in item.params.items():
        o, crc = find_param(gparam, param)
        t, _ = o.params[crc]
        if t in bf.AAMP_STRING_TYPES:
            o.params[crc] = (t, value.encode())
        elif t == 2:
            o.params[crc] = (t, struct.pack("<i", value))
        elif t == 1:
            o.params[crc] = (t, struct.pack("<f", value))
        elif t == 0:
            o.params[crc] = (t, struct.pack("<I", 1 if value else 0))
        else:
            raise ValueError(f"can't set {param} (type {t})")

    info = model.objects[CONTROLLER_INFO]
    info.params[crc32("BaseScale")] = (4, struct.pack("<3f", *item.scale))

    new_gparam = f"Actor/GeneralParamList/{item.name}.bgparamlist"
    new_model = f"Actor/ModelList/{item.name}.bmodellist"
    files[new_gparam] = bf.write_aamp(kind_g, gparam)
    files[new_model] = bf.write_aamp(kind_m, model)
    resized[new_gparam], resized[new_model] = gparam_path, model_path
    set_user("GParamUser", item.name)
    set_user("ModelUser", item.name)

    # Swap in other users (like a plain AI), taking their files from a weapon
    # that has them, and leave out the files only the old users needed.
    if item.links:
        borrowed = unpack_actor(dump, item.borrow_from)
        folders = {"AIProgramUser": ("Actor/AIProgram/", ".baiprog"),
                   "ASUser": ("Actor/ASList/", ".baslist")}
        for name, value in item.links.items():
            folder, ext = folders[name]
            old = f"{folder}{user(name)}{ext}"
            files.pop(old, None)
            if name == "ASUser":                # an AS list names its AS files
                for path in [p for p in files if p.startswith("Actor/AS/")]:
                    del files[path]
            new = f"{folder}{value}{ext}"
            files[new] = borrowed[new]
            if name == "ASUser":
                files.update({p: d for p, d in borrowed.items() if p.startswith("Actor/AS/")})
            set_user(name, value)

    tags = link.objects[TAGS]
    kept = [raw for _, (t, raw) in tags.params.items() if string(raw) not in item.drop_tags]
    tags.params = {crc32(f"Tag{i}"): (20, raw) for i, raw in enumerate(kept)}
    for o in link.objects.values():             # any other tag object, too
        if o is not tags and o is not target:
            left = [raw for _, (t, raw) in o.params.items() if string(raw) not in item.drop_tags]
            o.params = {crc32(f"Tag{i}"): (20, raw) for i, raw in enumerate(left)}

    new_link = f"Actor/ActorLink/{item.name}.bxml"
    files[new_link] = bf.write_aamp(kind, link)
    resized[new_link] = link_path

    pack = bf.write_sarc(files)
    pack_path = f"Actor/Pack/{item.name}.bactorpack"
    resized[pack_path] = f"Actor/Pack/{item.copy_of}.bactorpack"
    old_pack = bf.yaz0_decompress(dump.read(f"Actor/Pack/{item.copy_of}.sbactorpack"))
    sizes = {pack_path: len(pack), new_link: len(files[new_link]),
             new_gparam: len(files[new_gparam]), new_model: len(files[new_model])}
    old_sizes = {pack_path: len(old_pack), new_link: None, new_gparam: None, new_model: None}
    original = unpack_actor(dump, item.copy_of)
    for path in sizes:
        before = old_sizes[path] or len(original[resized[path]])
        grow_entry(rstb, path, resized[path], sizes[path] - before)

    icon = dump.read(f"UI/StockItem/{item.copy_of}.sbitemico")
    copy_entry(rstb, f"UI/StockItem/{item.name}.bitemico", f"UI/StockItem/{item.copy_of}.bitemico")

    tag_names = [string(raw) for _, raw in tags.params.values()]   # ActorInfo lists these
    return {f"Actor/Pack/{item.name}.sbactorpack": bf.yaz0_compress(pack),
            f"UI/StockItem/{item.name}.sbitemico": icon}, users, tag_names


# --- the size table ------------------------------------------------------------

MARGIN = 0x400   # a little spare on every entry this changes


def grow_entry(rstb, path, like, extra):
    """Gives path the entry of `like`, plus extra bytes (and the margin)."""
    base = rstb.get(crc32(like))
    if base is None:
        raise KeyError(f"no size entry for {like}")
    rstb[crc32(path)] = base + max(extra, 0) + MARGIN


def copy_entry(rstb, path, like):
    if crc32(like) in rstb:
        rstb[crc32(path)] = rstb[crc32(like)]


# --- everything ------------------------------------------------------------------

def build(content_dirs, items=NEW_ITEMS):
    """{content path: bytes} for every file that changes, and a list of what was done."""
    dump = Dump(content_dirs)
    rstb_path = "System/Resource/ResourceSizeTable.product.srsizetable"
    crcs, names = bf.read_rstb(bf.yaz0_decompress(dump.read(rstb_path)))
    out, done = {}, []

    info_path = "Actor/ActorInfo.product.sbyml"
    info = bf.read_byml(bf.yaz0_decompress(dump.read(info_path)))
    actors, hashes = info["Actors"], info["Hashes"]
    by_name = {a["name"]: a for a in actors}

    messages = {}                               # label -> text, for every language
    for item in items:
        files, users, tag_names = build_weapon(dump, item, crcs)
        out.update(files)

        entry = dict(by_name[item.copy_of])
        entry["name"] = item.name
        entry["elink"], entry["slink"], entry["xlink"] = users[crc32("ElinkUser")], users[crc32("SlinkUser")], users[crc32("XlinkUser")]
        entry["tags"] = {f"tag{crc32(t):08x}": bf.U32(crc32(t)) if crc32(t) >= 1 << 31 else crc32(t)
                         for t in tag_names}
        entry.pop("homeArea", None)             # where it's found in the world: nowhere
        h = crc32(item.name)
        if item.name in by_name:
            raise ValueError(f"{item.name} already exists in the game")
        at = next((i for i, x in enumerate(hashes) if int(x) > h), len(hashes))
        hashes.insert(at, bf.U32(h) if h >= 1 << 31 else h)
        actors.insert(at, entry)
        by_name[item.name] = entry

        messages[f"{item.name}_Name"] = item.english_name
        messages[f"{item.name}_Desc"] = item.description
        done.append(f"{item.name}: {item.english_name} (from {item.copy_of})")

    out[info_path] = bf.yaz0_compress(bf.write_byml(info))

    # Names and descriptions: English text goes in every language's file, so
    # the item has a name whichever language the game is in.
    for pack in dump.glob("Pack/Bootup_*.pack"):
        lang = pack.name[len("Bootup_"):-len(".pack")]
        if len(lang) != 4:                      # Bootup_Graphics.pack and others
            continue
        outer = bf.sarc_files(pack.read_bytes())
        msg_path = f"Message/Msg_{lang}.product.ssarc"
        if msg_path not in outer:
            continue
        inner_raw = bf.yaz0_decompress(outer[msg_path])
        inner = bf.sarc_files(inner_raw)
        msbt = "ActorType/WeaponSmallSword.msbt"
        inner[msbt] = bf.msbt_add(inner[msbt], messages)
        new_inner = bf.write_sarc(inner, align=lambda p: 16)
        outer[msg_path] = bf.yaz0_compress(new_inner)
        out[f"Pack/{pack.name}"] = bf.write_sarc(outer, align=lambda p: 0x80)
        rstb_name = f"Message/Msg_{lang}.product.sarc"
        grow_entry(crcs, rstb_name, rstb_name, len(new_inner) - len(inner_raw))

    out[rstb_path] = bf.yaz0_compress(bf.write_rstb(crcs, names))
    return out, done
