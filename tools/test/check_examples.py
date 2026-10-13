#!/usr/bin/env python3
"""Builds every example at once, pasted into my_mod.cpp the way a student would.

The code below each example's "copy from here" line goes above every_frame(),
and the lines its header says to add go inside every_frame() (and
when_game_adds()). Then my_mod.cpp is put back and built again, so the
installed mod matches it.

It starts from the committed my_mod.cpp (git HEAD), the template a student
starts from. It catches examples that don't compile, that clash with each
other, or that cause warnings. It doesn't run anything in the game.

    tools/test/check_examples.py
"""

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MY_MOD = ROOT / "mod" / "my_mod.cpp"
EXAMPLES = ROOT / "mod" / "examples"
MARKER = "// ---- copy from here ----"
INSTRUCTION = re.compile(r"add this line inside (\w+)\(\)", re.IGNORECASE)


def instructions(header):
    """{function: [lines]} from the header's "add this line inside f()" steps."""
    found = {}
    lines = header.splitlines()
    for i, line in enumerate(lines):
        match = INSTRUCTION.search(line)
        if not match:
            continue
        code = []
        for later in lines[i + 1:]:
            text = later.lstrip("/").strip()
            if text:
                code.append(text)
            elif code:
                break   # a blank comment line after the code ends it
        found.setdefault(match.group(1), []).extend(code)
    return found


def paste_all(template):
    """The template with every example pasted in, and what was pasted."""
    pasted, calls, problems = [], {"every_frame": [], "when_game_adds": []}, []
    for path in sorted(EXAMPLES.glob("*.cpp")):
        text = path.read_text()
        if MARKER not in text:
            problems.append(f"{path.name}: no '{MARKER}' line")
            continue
        header, code = text.split(MARKER, 1)
        steps = instructions(header)
        if not steps.get("every_frame"):
            problems.append(f"{path.name}: its header doesn't name a line for every_frame()")
        for function, lines in steps.items():
            if function not in calls:
                problems.append(f"{path.name}: says to add a line inside {function}(), which my_mod.cpp doesn't have")
                continue
            calls[function].extend(lines)
        pasted.append(f"// ===== pasted from examples/{path.name}\n{code.strip()}\n")

    lines = template.splitlines()
    out = []
    in_adds = False
    for line in lines:
        if line.strip().startswith("return amount;") and in_adds:
            out.extend("    " + c for c in calls["when_game_adds"])
            in_adds = False
        out.append(line)
        if "Paste examples here" in line:
            out.append("")
            out.extend(pasted)
        elif line.startswith("void every_frame()"):
            out.extend("    " + c for c in calls["every_frame"])
        elif line.startswith("int when_game_adds("):
            in_adds = True
    if not any("Paste examples here" in line for line in lines):
        problems.append("my_mod.cpp has no 'Paste examples here' line")
    return "\n".join(out) + "\n", len(pasted), problems


def build():
    """Runs wxl build; gives back (ok, output)."""
    result = subprocess.run([sys.executable, str(ROOT / "tools" / "wxl.py"), "build"],
                            cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    return result.returncode == 0, result.stdout


def main():
    original = MY_MOD.read_bytes()
    try:
        template = subprocess.run(["git", "show", "HEAD:mod/my_mod.cpp"], cwd=ROOT,
                                  stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                                  text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError):
        template = original.decode()
    combined, count, problems = paste_all(template)
    for problem in problems:
        print("problem:", problem)

    try:
        MY_MOD.write_text(combined)
        ok, output = build()
    finally:
        MY_MOD.write_bytes(original)

    noise = [l for l in output.splitlines() if "warning:" in l or "error:" in l]
    if ok:
        print(f"all {count} examples built together" + (f", with {len(noise)} warning(s):" if noise else ", no warnings"))
    else:
        print(f"the {count} examples together did NOT build:")
    for line in noise or ([] if ok else output.splitlines()[-25:]):
        print("  " + line)

    restored, restore_output = build()
    print("my_mod.cpp put back and rebuilt" if restored
          else "my_mod.cpp put back, but rebuilding it FAILED - run ./wxl build:\n" + restore_output[-2000:])
    sys.exit(0 if ok and not noise and not problems and restored else 1)


if __name__ == "__main__":
    main()
