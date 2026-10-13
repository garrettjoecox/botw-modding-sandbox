#!/bin/bash
# shot.sh <path-without-.png>  ->  <path>.png (Cemu's game window) and <path>_s.png (1000 px wide, to read)
D="$(cd "$(dirname "$0")" && pwd)"
id=$("$D/bin/winid") || exit 1
screencapture -x -o -l"$id" "$1.png" && sips -Z 1000 "$1.png" --out "$1_s.png" >/dev/null
