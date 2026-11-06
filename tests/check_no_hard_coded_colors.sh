#!/usr/bin/env bash
# Every color comes from Theme: a color written anywhere else breaks dark mode.
set -uo pipefail

source_folder="$1"
found=$(grep -rnE '#[0-9A-Fa-f]{6}|setStyleSheet|QColor\(|Qt::(red|green|yellow|blue|black|white)' "$source_folder" --include='*.cpp' --include='*.h' | grep -v 'ui/Theme\.')

if [ -n "$found" ]; then
    echo "Hard-coded colors found. Use Theme::color() instead:"
    echo "$found"
    exit 1
fi
