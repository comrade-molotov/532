#!/usr/bin/env bash
# ============================================================================
#  SA-MP Freeroam 0.3.7 - ჩამოტვირთე საჭირო plugin-ები (Linux)
#
#  გამოყენება:   ./download_dependencies.sh
#
#  ამოწმებს და, თუ აკლია, ჩამოტვირთავს შემდეგ ფაილებს plugins/ საქაღალდეში:
#     plugins/mysql.so       BlueG MySQL R41-4 (Debian static build)
#     plugins/streamer.so    Incognito streamer 2.9.6
#     plugins/sscanf.so      Y-Less sscanf 2.13.8
#  და (საჭიროების შემთხვევაში) კომპილატორს pawno/ საქაღალდეში.
#
#  სკრიპტს ინტერნეტი სჭირდება მხოლოდ პირველად. ხელახლა გაშვება უსაფრთხოა.
# ============================================================================
set -u -o pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGINS="$ROOT/plugins"
mkdir -p "$PLUGINS"

MYSQL_URL="https://github.com/pBlueG/SA-MP-MySQL/releases/download/R41-4/mysql-R41-4-Debian-static.tar.gz"
STREAMER_URL="https://github.com/samp-incognito/samp-streamer-plugin/releases/download/v2.9.6/samp-streamer-plugin-2.9.6.zip"
SSCANF_URL="https://github.com/Y-Less/sscanf/releases/download/v2.13.8/sscanf-2.13.8-linux.tar.gz"
PAWN_URL="https://github.com/pawn-lang/compiler/releases/download/v3.10.10/pawnc-3.10.10-linux.tar.gz"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

fail() { echo "[X] $*" >&2; exit 1; }

# ---------------------------------------------------------------- downloader
if command -v curl >/dev/null 2>&1; then
    DL="curl"
elif command -v wget >/dev/null 2>&1; then
    DL="wget"
else
    fail "არც curl, არც wget არ არის დაყენებული. (sudo apt install curl)"
fi

fetch() {
    local url="$1" out="$2"
    echo "  -> $(basename "$out")"
    if [ "$DL" = "curl" ]; then
        curl -fsSL --retry 3 -o "$out" "$url" || fail "ვერ ჩამოვწერე: $url"
    else
        wget -q -O "$out" "$url" || fail "ვერ ჩამოვწერე: $url"
    fi
}

# ---------------------------------------------------------------- unpackers
unpack() {
    local archive="$1" dest="$2"
    mkdir -p "$dest"
    case "$archive" in
        *.zip)
            if command -v unzip >/dev/null 2>&1; then
                unzip -qo "$archive" -d "$dest"
            elif command -v python3 >/dev/null 2>&1; then
                python3 -m zipfile -e "$archive" "$dest"
            else
                fail "zip-ის გასაშლელად საჭიროა unzip ან python3 (sudo apt install unzip)"
            fi
            ;;
        *.tar.gz|*.tgz)
            tar -xzf "$archive" -C "$dest"
            ;;
        *)
            fail "უცნობი არქივის ტიპი: $archive"
            ;;
    esac
}

# 32-bit check: SA-MP Linux server is 32-bit, so every .so must be ELF 32
is_elf32() {
    python3 - "$1" <<'PY' 2>/dev/null
import sys
try:
    with open(sys.argv[1], 'rb') as f:
        h = f.read(18)
    sys.exit(0 if h[:4] == b'\x7fELF' and h[4] == 1 else 1)
except Exception:
    sys.exit(1)
PY
}

install_plugin() {  # <extracted dir> <file name in archive> <target name>
    local dir="$1" name="$2" target="$3"
    local found
    found="$(find "$dir" -type f -name "$name" | head -n 1)"
    [ -n "${found:-}" ] || fail "არქივში ვერ ვიპოვე '$name'"
    cp -f "$found" "$PLUGINS/$target"
    echo "     plugins/$target  OK"
}

echo "=============================================================="
echo " SA-MP Freeroam - dependency downloader (Linux)"
echo "=============================================================="

# ------------------------------------------------------------------- mysql
echo "[1/3] MySQL plugin (BlueG R41-4, static build)"
if [ -f "$PLUGINS/mysql.so" ]; then
    echo "     plugins/mysql.so უკვე არსებობს - გამოტოვებულია"
else
    fetch "$MYSQL_URL" "$TMP/mysql.tar.gz"
    unpack "$TMP/mysql.tar.gz" "$TMP/mysql"
    install_plugin "$TMP/mysql" "*.so" "mysql.so"
fi

# ---------------------------------------------------------------- streamer
echo "[2/3] Streamer plugin (2.9.6)"
if [ -f "$PLUGINS/streamer.so" ]; then
    echo "     plugins/streamer.so უკვე არსებობს - გამოტოვებულია"
else
    fetch "$STREAMER_URL" "$TMP/streamer.zip"
    unpack "$TMP/streamer.zip" "$TMP/streamer"
    install_plugin "$TMP/streamer" "*.so" "streamer.so"
fi

# ------------------------------------------------------------------ sscanf
echo "[3/3] sscanf plugin (2.13.8)"
if [ -f "$PLUGINS/sscanf.so" ]; then
    echo "     plugins/sscanf.so უკვე არსებობს - გამოტოვებულია"
else
    fetch "$SSCANF_URL" "$TMP/sscanf.tar.gz"
    unpack "$TMP/sscanf.tar.gz" "$TMP/sscanf"
    install_plugin "$TMP/sscanf" "*.so" "sscanf.so"
fi

# --------------------------------------------------------- pawn compiler
if [ -x "$ROOT/pawno/pawncc" ]; then
    echo "[+] კომპილატორი: pawno/pawncc უკვე არსებობს"
else
    echo "[+] კომპილატორი: ვაყენებ pawno/pawncc-ს"
    fetch "$PAWN_URL" "$TMP/pawnc.tar.gz"
    unpack "$TMP/pawnc.tar.gz" "$TMP/pawnc"
    PAWNCC="$(find "$TMP/pawnc" -type f -name pawncc | head -n 1)"
    [ -n "$PAWNCC" ] || fail "არქივში pawncc ვერ ვიპოვე"
    cp -f "$PAWNCC" "$ROOT/pawno/pawncc"
    chmod +x "$ROOT/pawno/pawncc"
fi

# -------------------------------------------------------------- validation
echo
echo "შემოწმება:"
status=0
for f in mysql.so streamer.so sscanf.so; do
    if [ -f "$PLUGINS/$f" ]; then
        if is_elf32 "$PLUGINS/$f"; then
            echo "  [OK] plugins/$f   (32-bit)"
        else
            echo "  [!]  plugins/$f   არსებობს, მაგრამ 32-ბიტიანი ELF არ არის"
            status=1
        fi
    else
        echo "  [X]  plugins/$f   აკლია"
        status=1
    fi
done

echo
if [ "$status" -eq 0 ]; then
    echo "ყველაფერი მზადაა: plugins/ სავსეა, კომპილატორი ადგილზეა."
    echo "შემდეგი ნაბიჯი:  ./setup.sh"
else
    echo "რაღაც აკლია - ნახე შეტყობინებები ზემოთ."
fi
exit "$status"
