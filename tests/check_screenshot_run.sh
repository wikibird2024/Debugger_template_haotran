#!/usr/bin/env bash
# A --screenshot run connects and turns Live on by itself, so it must never make
# the real board link, and it must leave the user's settings alone. The program
# under test is the real main.cpp, built with a link that aborts when made.
set -uo pipefail

program="$1"
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
ulimit -c 0

export QT_QPA_PLATFORM=offscreen
# A scratch home: Qt's test mode moves settings to ~/.qttest, away from XDG_CONFIG_HOME.
export HOME="$scratch/home"
export XDG_CONFIG_HOME="$HOME/.config"
settings_file="$XDG_CONFIG_HOME/Debugger template/Debugger template.conf"
mkdir -p "$(dirname "$settings_file")"
printf '[General]\nisDarkTheme=true\n' >"$settings_file"
cp "$settings_file" "$scratch/settings_before"

# Without --screenshot the link must be made: this proves the trap is armed.
timeout 20 "$program" >/dev/null 2>"$scratch/normal_run_errors"
if ! grep -q 'The real board link was made' "$scratch/normal_run_errors"; then
    echo "A normal run did not make the board link, so this check proves nothing."
    exit 1
fi

if ! timeout 60 "$program" --screenshot "$scratch/shots"; then
    echo "The --screenshot run failed or made the real board link."
    exit 1
fi

file_count=$(find "$scratch/shots" -name '*.png' | wc -l)
if [ "$file_count" -eq 0 ]; then
    echo "The --screenshot run saved no pictures."
    exit 1
fi

if ! cmp -s "$settings_file" "$scratch/settings_before"; then
    echo "The --screenshot run changed the user's settings."
    exit 1
fi

other_settings=$(find "$HOME" -name 'Debugger template.conf' ! -path "$settings_file")
if [ -n "$other_settings" ]; then
    echo "The --screenshot run wrote settings into the home folder: $other_settings"
    exit 1
fi
