# Shared setup for the build scripts. Source it; do not run it.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="${WORK:-$ROOT/work}"
DL="$WORK/downloads"
mkdir -p "$WORK" "$DL"

# shellcheck disable=SC1091
. "$ROOT/versions.env"

JOBS="${JOBS:-$(sysctl -n hw.ncpu)}"
# Keep macOS tar from adding AppleDouble (._*) files to the archives.
export COPYFILE_DISABLE=1

# Runs a command as x86_64 (Wine for macOS is x86_64; Apple silicon uses Rosetta 2).
x86() {
    if [ "$(uname -m)" = "arm64" ]; then arch -x86_64 "$@"; else "$@"; fi
}

# fetch <url> <sha256> <file name>: downloads into $DL once and checks the checksum.
fetch() {
    target="$DL/$3"
    if [ ! -f "$target" ]; then
        curl -fL --retry 3 -o "$target.part" "$1"
        mv "$target.part" "$target"
    fi
    actual="$(shasum -a 256 "$target" | cut -d' ' -f1)"
    if [ "$actual" != "$2" ]; then
        echo "SHA-256 mismatch for $3: expected $2, got $actual" >&2
        rm -f "$target"
        exit 1
    fi
    echo "$target"
}

# clone_pinned <repo> <tag> <commit> <dir> [git clone options]: shallow clone, checked against the commit.
clone_pinned() {
    repo=$1 tag=$2 commit=$3 dir=$4
    shift 4
    if [ ! -d "$dir/.git" ]; then
        git clone --depth 1 --branch "$tag" "$@" "$repo" "$dir"
    fi
    actual="$(git -C "$dir" rev-parse HEAD)"
    if [ "$actual" != "$commit" ]; then
        echo "$repo $tag is $actual, expected $commit" >&2
        exit 1
    fi
}
