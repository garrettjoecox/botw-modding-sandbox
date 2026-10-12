#!/usr/bin/env python3
"""wxl - one entry point for this BotW modding workspace, on macOS and Windows.

    wxl setup [--base DIR] [--update DIR] [--dlc DIR]
                       download Cemu, write its config, install the WiiXLaunch
                       pack, fetch the framework, and (optionally) copy in game files
    wxl build            compile the mod in mod/ and install it
    wxl play             launch BotW in Cemu
    wxl log [TEXT]       show the mod's messages from the last run (optionally filtered)
    wxl actors [TEXT]    list actor names (for spawn) with their English names
    wxl host             rebuild the WiiXLaunch host and refresh pack/ (rarely needed)
    wxl doctor           check what is installed and what is missing

Run it through the wrappers at the workspace root: ./wxl (macOS) or wxl.cmd (Windows).

Toolchain: mods compile with devkitPPC. A native devkitPro install is used when
one is found (DEVKITPPC, C:\\devkitPro, /opt/devkitpro); otherwise the build runs
in Docker (image built from tools/docker). Force either with --native / --docker.
"""

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOLS = ROOT / "tools"
MOD = ROOT / "mod"                    # the one mod: my_mod.cpp, engine, mod.json
EXAMPLES = MOD / "examples"           # reference code to copy from; not built
PACK_SRC = ROOT / "pack" / "WiiXLaunch_BotW"
WXL = ROOT / "WiiXLaunch"

CEMU_DIR = ROOT / "cemu"
PORTABLE = CEMU_DIR / "portable"      # Cemu's portable mode: next to Cemu.exe / Cemu.app
PACK_DST = PORTABLE / "graphicPacks" / "WiiXLaunch_BotW"
PACK_MODS = PACK_DST / "content" / "WiiXLaunch" / "mods"
GAME = CEMU_DIR / "games" / "BotW"
RPX = GAME / "code" / "U-King.rpx"
TITLE_DIR = PORTABLE / "mlc01" / "usr" / "title"
UPDATE_DIR = TITLE_DIR / "0005000e" / "101c9400"
DLC_DIR = TITLE_DIR / "0005000c" / "101c9400"
LOG = PORTABLE / "log.txt"

WXL_REPO = "https://github.com/BladesawStudios/WiiXLaunch.git"
WXL_COMMIT = "30e0502ff2abb76b1816e8271d4126e0a95ac9fc"
WXL_PATCH = TOOLS / "wiixlaunch-local.patch"

DOCKER_IMAGE = "wiixl-dkp"

# Earlier layouts of this workspace installed these: nine separate lesson mods,
# then one combined "lessons" mod. `wxl build` removes them so they don't run
# alongside my_mod.
OLD_MODS = ("hello", "stamina", "hud", "time_warp", "loadout", "unbreakable",
            "bounty", "jetpack", "rupee_hook", "lessons")

IS_WINDOWS = sys.platform == "win32"
IS_MAC = sys.platform == "darwin"

if IS_MAC:
    CEMU_EXE = CEMU_DIR / "Cemu.app"
    _arch = "arm64" if platform.machine() == "arm64" else "x86_64"
    # Newest successful build of Cemu's main branch (has the native Metal renderer).
    CEMU_URL = ("https://nightly.link/cemu-project/Cemu/workflows/build_check/main/"
                f"cemu-bin-macos-{_arch}.zip")
elif IS_WINDOWS:
    CEMU_EXE = CEMU_DIR / "Cemu.exe"
    # The stable release. WiiXLaunch's GUI was developed against 2.6.
    CEMU_URL = "https://github.com/cemu-project/Cemu/releases/download/v2.6/cemu-2.6-windows-x64.zip"
else:
    CEMU_EXE = CEMU_DIR / "Cemu"
    CEMU_URL = None

# Graphics API ids from Cemu's config: 1 = Vulkan, 2 = Metal.
GRAPHICS_API = 2 if IS_MAC else 1
# Cemu log flag bit 17 = CoreinitLogging, which is where WIIXL_LOG's OSReport lands.
LOG_FLAG = 1 << 17


def say(msg):
    print(f"[wxl] {msg}", flush=True)


def die(msg):
    print(f"[wxl] error: {msg}", file=sys.stderr, flush=True)
    sys.exit(1)


def run(cmd, cwd=None, check=True):
    """Run a command, streaming its output."""
    result = subprocess.run(cmd, cwd=cwd)
    if check and result.returncode != 0:
        die(f"command failed ({result.returncode}): {' '.join(str(c) for c in cmd)}")
    return result.returncode


def download(url, dest):
    say(f"downloading {url}")
    with urllib.request.urlopen(url) as resp, open(dest, "wb") as out:
        shutil.copyfileobj(resp, out)


# --- toolchain ---------------------------------------------------------------

def native_devkitppc():
    candidates = [os.environ.get("DEVKITPPC"), r"C:\devkitPro\devkitPPC", "/opt/devkitpro/devkitPPC"]
    gxx = "powerpc-eabi-g++.exe" if IS_WINDOWS else "powerpc-eabi-g++"
    for root in candidates:
        if root and (Path(root) / "bin" / gxx).exists():
            return Path(root)
    return None


def docker_available():
    if not shutil.which("docker"):
        return False
    return subprocess.run(["docker", "info"], stdout=subprocess.DEVNULL,
                          stderr=subprocess.DEVNULL).returncode == 0


def pick_toolchain(args):
    if args.native or (not args.docker and native_devkitppc()):
        if not native_devkitppc():
            die("no native devkitPPC found. Install devkitPro (https://devkitpro.org/wiki/Getting_Started) "
                "with devkitPPC, or use --docker.")
        return "native"
    if not docker_available():
        die("no devkitPPC found and Docker isn't running.\n"
            "  Windows: install devkitPro with the devkitPPC component (simplest).\n"
            "  Any OS:  start Docker Desktop and re-run.")
    return "docker"


def ensure_docker_image():
    if subprocess.run(["docker", "image", "inspect", DOCKER_IMAGE], stdout=subprocess.DEVNULL,
                      stderr=subprocess.DEVNULL).returncode != 0:
        say(f"building Docker image {DOCKER_IMAGE} (first time only)")
        run(["docker", "build", "-t", DOCKER_IMAGE, str(TOOLS / "docker")])


def container_root():
    # macOS/Linux mount the workspace at its own path, so paths in generated files
    # (compile_commands.json) work on the host too. Windows paths can't exist in a
    # Linux container, so mount at /work and translate afterwards.
    return "/work" if IS_WINDOWS else ROOT.as_posix()


def in_docker(shell_cmd, cwd):
    """Run a bash command inside the devkitPPC container, with cwd mapped."""
    ensure_docker_image()
    croot = container_root()
    ccwd = croot + "/" + Path(cwd).resolve().relative_to(ROOT).as_posix()
    return run(["docker", "run", "--rm", "-v", f"{ROOT}:{croot}", "-w", ccwd, DOCKER_IMAGE,
                "bash", "-c", shell_cmd], check=False)


def fix_container_paths(mod_dir):
    """Point editor files written inside Docker at the real (Windows) paths."""
    if not IS_WINDOWS:
        return
    for rel in ("compile_commands.json", ".vscode/c_cpp_properties.json"):
        p = mod_dir / rel
        if p.exists():
            p.write_text(p.read_text().replace("/work", ROOT.as_posix()))


# --- framework ---------------------------------------------------------------

def ensure_wiixlaunch():
    if (WXL / "sdk" / "scripts" / "build_mod.py").exists():
        return
    if not shutil.which("git"):
        die("git is needed to fetch WiiXLaunch (Windows: install Git for Windows).")
    say("fetching WiiXLaunch")
    run(["git", "clone", WXL_REPO, str(WXL)])
    run(["git", "checkout", "-q", WXL_COMMIT], cwd=WXL)
    run(["git", "submodule", "update", "--init", "--recursive"], cwd=WXL)
    say("applying local changes (tools/wiixlaunch-local.patch)")
    run(["git", "apply", "--whitespace=nowarn", str(WXL_PATCH)], cwd=WXL)


# --- Cemu --------------------------------------------------------------------

def ensure_cemu():
    if CEMU_EXE.exists():
        return
    if not CEMU_URL:
        die("automatic Cemu download is only set up for macOS and Windows.")
    CEMU_DIR.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        archive = Path(tmp) / "cemu.zip"
        download(CEMU_URL, archive)
        if IS_WINDOWS:
            with zipfile.ZipFile(archive) as z:
                for member in z.infolist():
                    parts = Path(member.filename).parts
                    if len(parts) < 2 or member.is_dir():
                        continue  # strip the top-level Cemu_2.6/ folder
                    target = CEMU_DIR.joinpath(*parts[1:])
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with z.open(member) as src, open(target, "wb") as dst:
                        shutil.copyfileobj(src, dst)
        else:
            with zipfile.ZipFile(archive) as z:
                z.extractall(tmp)
            dmg = Path(tmp) / "Cemu.dmg"
            mnt = Path(tmp) / "mnt"
            run(["hdiutil", "attach", "-nobrowse", "-readonly", "-mountpoint", str(mnt), str(dmg)])
            try:
                shutil.copytree(mnt / "Cemu.app", CEMU_EXE, symlinks=True)
            finally:
                run(["hdiutil", "detach", str(mnt)], check=False)
            run(["xattr", "-dr", "com.apple.quarantine", str(CEMU_EXE)], check=False)
    say(f"Cemu installed at {CEMU_EXE.relative_to(ROOT)}")


def write_portable_config():
    PORTABLE.mkdir(parents=True, exist_ok=True)
    settings = PORTABLE / "settings.xml"
    if not settings.exists():
        # Cemu stores this path relative to portable/, with the OS separator.
        pack_rules = ("\\" if IS_WINDOWS else "/").join(["graphicPacks", "WiiXLaunch_BotW", "rules.txt"])
        settings.write_text(f"""<?xml version="1.0" encoding="UTF-8"?>
<content>
    <logflag>{LOG_FLAG}</logflag>
    <mlc_path></mlc_path>
    <check_update>false</check_update>
    <gp_download>true</gp_download>
    <macos_disclaimer>true</macos_disclaimer>
    <vk_warning>true</vk_warning>
    <GraphicPack>
        <Entry filename="{pack_rules}"/>
    </GraphicPack>
    <Graphic>
        <api>{GRAPHICS_API}</api>
    </Graphic>
</content>
""")
        say("wrote cemu/portable/settings.xml")
    profile = PORTABLE / "controllerProfiles" / "controller0.xml"
    if not profile.exists():
        profile.parent.mkdir(parents=True, exist_ok=True)
        src = TOOLS / "profiles" / ("keyboard-windows.xml" if IS_WINDOWS else "keyboard-mac.xml")
        shutil.copyfile(src, profile)
        say(f"installed keyboard profile ({src.name})")


def install_pack():
    """Copy the host pack into Cemu, keeping any installed mods."""
    for src in PACK_SRC.rglob("*"):
        if src.is_file():
            dst = PACK_DST / src.relative_to(PACK_SRC)
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(src, dst)


# --- game files --------------------------------------------------------------

def find_title_root(path):
    """A dumped title folder is the one holding code/, content/ and meta/."""
    path = Path(path).expanduser().resolve()
    if not path.is_dir():
        die(f"not a folder: {path}")
    for candidate in [path, *sorted(p for p in path.rglob("*") if p.is_dir())]:
        if (candidate / "meta").is_dir() and ((candidate / "content").is_dir() or (candidate / "code").is_dir()):
            return candidate
    die(f"no code/content/meta folders found under {path}")


def copy_title(src, dst, label):
    root = find_title_root(src)
    say(f"copying {label} from {root} (this can take a while)")
    shutil.copytree(root, dst, dirs_exist_ok=True)


def title_version(dirpath):
    meta = dirpath / "meta" / "meta.xml"
    if not meta.exists():
        return None
    text = meta.read_text(errors="ignore")
    start = text.find("<title_version")
    return text[text.find(">", start) + 1:text.find("<", start + 1)].strip() if start >= 0 else None


# --- commands ----------------------------------------------------------------

def cmd_setup(args):
    ensure_wiixlaunch()
    ensure_cemu()
    write_portable_config()
    install_pack()
    if args.base:
        copy_title(args.base, GAME, "base game")
    if args.update:
        copy_title(args.update, UPDATE_DIR, "update")
    if args.dlc:
        copy_title(args.dlc, DLC_DIR, "DLC")
    cmd_doctor(args)


def cmd_doctor(args):
    def status(ok, what, hint=""):
        print(f"  [{'ok' if ok else '--'}] {what}" + ("" if ok or not hint else f"  -> {hint}"))
    print("workspace:", ROOT)
    status(CEMU_EXE.exists(), f"Cemu ({CEMU_EXE.relative_to(ROOT)})", "wxl setup")
    status((PORTABLE / "settings.xml").exists(), "Cemu portable config", "wxl setup")
    status((PACK_DST / "rules.txt").exists(), "WiiXLaunch graphic pack", "wxl setup")
    status((WXL / "sdk" / "scripts" / "build_mod.py").exists(), "WiiXLaunch framework/SDK", "wxl setup")
    status(RPX.exists(), "base game (cemu/games/BotW)", "wxl setup --base <dump folder>")
    ver = title_version(UPDATE_DIR)
    status(ver == "208", f"update v208 (found: {ver or 'none'})", "wxl setup --update <dump folder>")
    status((DLC_DIR / "content").exists(), "DLC (optional)", "wxl setup --dlc <dump folder>")
    native = native_devkitppc()
    status(bool(native) or docker_available(),
           f"toolchain ({'native ' + str(native) if native else 'Docker' if docker_available() else 'none'})",
           "install devkitPro (devkitPPC) or start Docker Desktop")
    mods = sorted(p.name for p in PACK_MODS.glob("*.wxlm")) if PACK_MODS.exists() else []
    print(f"  installed mods: {', '.join(m[:-5] for m in mods) or 'none'}")


def read_manifest():
    path = MOD / "mod.json"
    if not path.exists():
        die("no mod/mod.json")
    return path, json.loads(path.read_text())


def sync_sources():
    """List every .cpp in mod/ in mod.json, so a new file builds without editing it.

    examples/ is left out: those files are there to copy from, and building them
    all would define every example function a second time."""
    path, manifest = read_manifest()
    entry = manifest.get("entry", "mod.cpp")
    found = []
    for p in sorted(MOD.rglob("*.cpp")):
        rel = p.relative_to(MOD)
        if rel.parts[0] in ("build", "examples") or any(part.startswith(".") for part in rel.parts):
            continue
        if rel.as_posix() != entry:
            found.append(rel.as_posix())
    if manifest.get("sources") != found:
        manifest["sources"] = found
        path.write_text(json.dumps(manifest, indent=2) + "\n")
        say("mod.json: updated the list of source files")
    return manifest


def write_compile_commands(manifest):
    """One entry per source file, for the editor (build_mod.py writes just the entry)."""
    path = MOD / "compile_commands.json"
    try:
        db = json.loads(path.read_text())
    except (OSError, ValueError):
        return
    if not db:
        return
    flags = [a for a in db[0]["arguments"] if not a.endswith(".cpp")]
    files = [manifest.get("entry", "mod.cpp")] + manifest.get("sources", [])
    entries = []
    for rel in files:
        f = (MOD / rel).as_posix()
        entries.append({"directory": MOD.as_posix(), "file": f, "arguments": flags + ["-c", f]})
    path.write_text(json.dumps(entries, indent=2) + "\n")


def cmd_build(args):
    ensure_wiixlaunch()
    manifest = sync_sources()
    mod_id = manifest.get("id", "my_mod")
    toolchain = pick_toolchain(args)
    say(f"building mod/ with {toolchain} devkitPPC")
    if toolchain == "native":
        rc = run([sys.executable, str(WXL / "sdk" / "scripts" / "build_mod.py"), "--source", "mod"],
                 cwd=ROOT, check=False)
    else:
        rc = in_docker("python3 WiiXLaunch/sdk/scripts/build_mod.py --source mod", cwd=ROOT)
        fix_container_paths(MOD)
    write_compile_commands(manifest)
    out = MOD / "build" / f"{mod_id}.wxlm"
    if rc != 0 or not out.exists():
        die("the build failed - the first error above says what went wrong")
    PACK_MODS.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(out, PACK_MODS / out.name)
    for name in OLD_MODS:
        old = PACK_MODS / f"{name}.wxlm"
        if old.exists() and old.name != out.name:
            old.unlink()
            say(f"removed {old.name} from an earlier layout of this workspace")
    say(f"installed {out.name} - restart the game to load it")


def cmd_play(args):
    if not CEMU_EXE.exists():
        die("Cemu isn't installed - run: wxl setup")
    if not RPX.exists():
        die("no game at cemu/games/BotW - run: wxl setup --base <your dump>")
    install_pack()
    say("launching BotW")
    if IS_MAC:
        run(["open", "-n", str(CEMU_EXE), "--args", "-g", str(RPX)])
    else:
        subprocess.Popen([str(CEMU_EXE), "-g", str(RPX)], cwd=CEMU_DIR)


def cmd_log(args):
    if not LOG.exists():
        die("no log yet - run the game first")
    _, manifest = read_manifest()
    prefix = manifest.get("id", "my_mod") + ": "
    problems = ("refused", "fail", "error", "missing", "conflict")
    for line in LOG.read_text(errors="ignore").splitlines():
        if args.all:
            shown = line
        elif "[OSConsole]" in line:
            # "[12:00:00.000] [OSConsole] my_mod: Hello" -> "Hello"
            text = line.split("[OSConsole]", 1)[1].strip()
            if text.startswith(prefix):
                shown = text[len(prefix):]
            elif prefix[:-2] in text and any(p in text.lower() for p in problems):
                shown = "(loader) " + text   # e.g. the mod was refused at load
            else:
                continue
        else:
            continue
        if args.text and args.text.lower() not in shown.lower():
            continue
        print(shown)


def cmd_actors(args):
    """Actor names from the user's own dump, with the game's English names."""
    import botw_files   # tools/botw_files.py, next to this script
    content = [d / "content" for d in (UPDATE_DIR, GAME) if (d / "content").is_dir()]
    if not content:
        die("no game files yet - run: wxl setup --base <dump> --update <dump>")
    names = botw_files.actor_names(content)
    english = botw_files.english_names(content)
    # "_Far" actors are the low-detail stand-ins the game draws in the distance.
    names = sorted(n for n in names if not n.endswith("_Far"))
    text = (args.text or "").lower()
    shown = [n for n in names if text in n.lower() or text in english.get(n, "").lower()]
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(errors="replace")   # a Windows console may not have every character
    for name in shown:
        print(f"{name:<44} {english.get(name, '')}".rstrip())
    if text:
        print(f"[{len(shown)} of {len(names)} actors match \"{args.text}\"]")
    else:
        print(f"[{len(names)} actors. Narrow it down: wxl actors bokoblin]")


def cmd_host(args):
    ensure_wiixlaunch()
    toolchain = pick_toolchain(args)
    steps = "python3 scripts/deploy.py --target botw && python3 scripts/make_sdk.py --host"
    if toolchain == "native":
        build = ["cmd", "/c", "build_cemu.bat"] if IS_WINDOWS else ["bash", "./build_cemu.sh"]
        run(build, cwd=WXL)
        run([sys.executable, "scripts/deploy.py", "--target", "botw"], cwd=WXL)
        run([sys.executable, "scripts/make_sdk.py", "--host"], cwd=WXL)
    elif in_docker(f"./build_cemu.sh && {steps}", cwd=WXL) != 0:
        die("host build failed")
    host = WXL / "build" / "host"
    shutil.rmtree(PACK_SRC, ignore_errors=True)
    shutil.copytree(host, PACK_SRC)
    install_pack()
    say("host rebuilt; pack/ updated and installed")


def main():
    parser = argparse.ArgumentParser(prog="wxl", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    def toolchain_flags(p):
        g = p.add_mutually_exclusive_group()
        g.add_argument("--native", action="store_true", help="use an installed devkitPro")
        g.add_argument("--docker", action="store_true", help="build inside Docker")

    p = sub.add_parser("setup", help="install Cemu, config, pack and framework")
    p.add_argument("--base", help="dumped base game folder (code/content/meta)")
    p.add_argument("--update", help="dumped update v208 folder")
    p.add_argument("--dlc", help="dumped DLC folder")
    p.set_defaults(func=cmd_setup)

    p = sub.add_parser("build", help="compile the mod and install it")
    toolchain_flags(p)
    p.set_defaults(func=cmd_build)

    sub.add_parser("play", help="launch the game").set_defaults(func=cmd_play)

    p = sub.add_parser("log", help="show the mod's messages from the last run")
    p.add_argument("text", nargs="?", help="only lines containing this")
    p.add_argument("--all", action="store_true", help="the whole Cemu log")
    p.set_defaults(func=cmd_log)

    p = sub.add_parser("actors", help="list actor names for spawn(), with English names")
    p.add_argument("text", nargs="?", help="only names containing this (either name)")
    p.set_defaults(func=cmd_actors)

    p = sub.add_parser("host", help="rebuild the WiiXLaunch host")
    toolchain_flags(p)
    p.set_defaults(func=cmd_host)

    sub.add_parser("doctor", help="check the workspace").set_defaults(func=cmd_doctor)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
