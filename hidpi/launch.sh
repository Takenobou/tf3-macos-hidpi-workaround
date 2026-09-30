#!/bin/sh
set -eu
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
steam_dir=${TPF3_STEAM_DIR:-"$HOME/Library/Application Support/Steam"}
game_dir=${TPF3_GAME_DIR:-"$steam_dir/steamapps/common/Transport Fever 3"}

if [ ! -x "$game_dir/TransportFever3" ] || [ ! -f "$game_dir/MoltenVK_icd.json" ]; then
    printf '%s\n' "Transport Fever 3 was not found at: $game_dir" \
        'Set TPF3_GAME_DIR to your Transport Fever 3 installation folder.' >&2
    exit 1
fi
if pgrep -x TransportFever3 >/dev/null; then
    printf '%s\n' 'Transport Fever 3 is already running. Quit it before using this launcher.' >&2
    exit 1
fi
if [ ! -f "$script_dir/libhidpi.dylib" ]; then
    sh "$script_dir/build.sh"
fi
# Steam userdata stays under the Steam folder even when the game is installed
# in another library. Back up TF3 settings/logs for each local Steam account.
backup_dir=''
for userdata in "$steam_dir"/userdata/*/3493540/local; do
    [ -f "$userdata/settings.lua" ] || continue
    if [ -z "$backup_dir" ]; then
        backup_dir=$(mktemp -d "$script_dir/test-backup.XXXXXX")
    fi
    account_dir=$(dirname "$(dirname "$userdata")")
    account_id=$(basename "$account_dir")
    mkdir -p "$backup_dir/$account_id"
    cp "$userdata/settings.lua" "$backup_dir/$account_id/settings.lua"
    if [ -f "$userdata/crash_dump/stdout.txt" ]; then
        cp "$userdata/crash_dump/stdout.txt" "$backup_dir/$account_id/stdout.txt"
    fi
done
cd "$game_dir"
export VK_ICD_FILENAMES="$game_dir/MoltenVK_icd.json"
export DYLD_LIBRARY_PATH="$game_dir"
export DYLD_INSERT_LIBRARIES="$script_dir/libhidpi.dylib"
export SteamAppId=3493540
exec ./TransportFever3 "$@"
