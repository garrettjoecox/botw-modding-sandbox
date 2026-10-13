#!/bin/bash
# Restarts Cemu and gets into the game fast: taps A every 2 s until the mod's
# first in-game tick ("PlayerTick: my_mod ran") is in the log. About 30 s.
# Only during a test session (session.sh start), since it quits any running
# Cemu and the game autosaves. A is Space (49) in the user's binds; override
# with A_KEY=<keycode>.
B="$(cd "$(dirname "$0")/../.." && pwd)"
D="$(cd "$(dirname "$0")" && pwd)"
A_KEY="${A_KEY:-49}"
[ -f "$B/backups/.test-session" ] || { echo "load.sh: no test session - run tools/test/session.sh start first" >&2; exit 1; }
pkill -f Cemu.app/Contents/MacOS/Cemu; while pgrep -f Cemu.app/Contents/MacOS/Cemu >/dev/null; do sleep 0.5; done
rm -f "$B/cemu/portable/log.txt"
"$B/wxl" play >/dev/null
for i in $(seq 1 75); do
  sleep 2
  if grep -aq "PlayerTick: my_mod ran" "$B/cemu/portable/log.txt" 2>/dev/null; then echo "in game after $((i*2))s"; exit 0; fi
  osascript -e 'tell application id "info.cemu.Cemu" to activate' >/dev/null 2>&1
  "$D/bin/keys" "t:$A_KEY"
done
echo "load.sh: timed out - take a screenshot (shot.sh) to see where it stopped" >&2; exit 1
