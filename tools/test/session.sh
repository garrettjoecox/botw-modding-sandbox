#!/bin/bash
# Keeps the user's own game safe around in-game tests. The save in cemu/portable
# is the user's real progress (they play between sessions), so tests run on a
# copy of an early save, and my_mod.cpp - the student's template - goes back
# afterwards.
#
#   session.sh start    back up the live save and my_mod.cpp, then put the test
#                       save in place. Refuses while Cemu runs: it may be the user's.
#   session.sh end      quit Cemu, put the user's save and my_mod.cpp back, and
#                       rebuild so the installed mod matches my_mod.cpp again
#   session.sh status   is a test running, and what will "end" put back
#   session.sh backup   just back up the live save (prints the backup's name)
#
# TEST_SAVE=<name in backups/> picks a different test save for "start".
B="$(cd "$(dirname "$0")/../.." && pwd)"
SAVE="$B/cemu/portable/mlc01/usr/save/00050000/101c9400"
MARK="$B/backups/.test-session"            # holds the name of the backup "end" restores
MOD_COPY="$B/backups/.test-session-my_mod.cpp"
TEST_SAVE="${TEST_SAVE:-save-101c9400-20261012-104050}"   # Great Plateau before the tower; renders reliably
CEMU=Cemu.app/Contents/MacOS/Cemu

cemu_running() { pgrep -f "$CEMU" >/dev/null; }

quit_cemu() {
  pkill -f "$CEMU"
  for _ in $(seq 1 20); do cemu_running || return 0; sleep 0.5; done
  pkill -9 -f "$CEMU"; sleep 1
}

fail() { echo "session.sh: $*" >&2; exit 1; }

# Copies the live save into backups/ and prints the copy's name.
backup() {
  local name
  while :; do   # one backup per second at most, so a name is never reused
    name="save-101c9400-$(date +%Y%m%d-%H%M%S)"
    [ -e "$B/backups/$name" ] || break
    sleep 1
  done
  command cp -Rf "$SAVE" "$B/backups/$name" && diff -rq "$SAVE" "$B/backups/$name" >/dev/null \
    || fail "couldn't back up the save"
  echo "$name"
}

# Makes backups/<name> the live save, and checks the copy matches.
put_save() {
  [ -d "$B/backups/$1" ] || fail "no backups/$1"
  command rm -rf "$SAVE" && command cp -Rf "$B/backups/$1" "$SAVE" && diff -rq "$B/backups/$1" "$SAVE" >/dev/null \
    || fail "couldn't put backups/$1 in place"
}

case "$1" in
  start)
    [ -f "$MARK" ] && fail "a test is already running (the user's save is backups/$(cat "$MARK"))"
    cemu_running && fail "Cemu is running - it may be the user's game. Ask them to close it."
    [ -d "$B/backups/$TEST_SAVE" ] || fail "no test save at backups/$TEST_SAVE"
    name=$(backup) || exit 1
    command cp -f "$B/mod/my_mod.cpp" "$MOD_COPY" || fail "couldn't copy my_mod.cpp"
    echo "$name" >| "$MARK"
    put_save "$TEST_SAVE"
    echo "user's save backed up to backups/$name; test save in place ($TEST_SAVE)" ;;
  end)
    [ -f "$MARK" ] || fail "no test is running"
    name=$(cat "$MARK")
    quit_cemu
    put_save "$name"
    command cp -f "$MOD_COPY" "$B/mod/my_mod.cpp" || fail "couldn't put my_mod.cpp back (copy: $MOD_COPY)"
    command rm -f "$MARK" "$MOD_COPY"
    echo "user's save restored from backups/$name; my_mod.cpp put back"
    if "$B/wxl" build >/dev/null 2>&1; then
      echo "rebuilt: the installed mod matches my_mod.cpp"
    else
      fail "REBUILD FAILED - run ./wxl build before the user plays, or the test build stays installed"
    fi ;;
  status)
    if [ -f "$MARK" ]; then
      echo "test running: the live save is a test save; the user's is backups/$(cat "$MARK")"
    else
      echo "no test running: the live save is the user's"
    fi
    if cemu_running; then echo "Cemu is running"; else echo "Cemu is not running"; fi ;;
  backup)
    [ -f "$MARK" ] && fail "a test is running, so the live save is the test save"
    cemu_running && fail "Cemu is running - close it first"
    backup ;;
  *) sed -n '2,14p' "$0"; exit 1 ;;
esac
