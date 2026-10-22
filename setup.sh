#!/usr/bin/env bash
# ============================================================================
#  SA-MP Freeroam 0.3.7 - ერთი ბრძანებით მომზადება (Linux)
#
#  გამოყენება:   ./setup.sh
#
#  რას აკეთებს:
#     1. ამოწმებს/ჩამოტვირთავს plugin-ებს (plugins/mysql.so, streamer.so, sscanf.so)
#     2. ამოწმებს კომპილატორს (pawno/pawncc)
#     3. აკომპილირებს gamemodes/freeroam.pwn -> gamemodes/freeroam.amx
#     4. ბეჭდავს ზუსტ ნაბიჯებს ბაზის შექმნისა და სერვერის გაშვებისთვის
# ============================================================================
set -u -o pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

echo "=============================================================="
echo " SA-MP Freeroam 0.3.7 - setup (Linux)"
echo "=============================================================="

# ------------------------------------------------------------ 1. plugins
echo
echo "[1/4] plugin-ები"
if [ -x ./download_dependencies.sh ] || [ -f ./download_dependencies.sh ]; then
    bash ./download_dependencies.sh || { echo; echo "plugin-ების ჩამოტვირთვა ვერ დასრულდა."; exit 1; }
else
    echo "  [!] download_dependencies.sh ვერ ვიპოვე"
fi

for f in mysql.so streamer.so sscanf.so; do
    [ -f "plugins/$f" ] || { echo "  [X] plugins/$f აკლია"; exit 1; }
done

# Linux-ზე server.cfg-ის plugins ხაზს .so გაფართოება სჭირდება
if [ -f server.cfg ] && grep -qE '^plugins +mysql +streamer +sscanf *$' server.cfg; then
    sed -i 's/^plugins .*/plugins mysql.so streamer.so sscanf.so/' server.cfg
    echo "  [OK] server.cfg: plugins ხაზი გადაკეთდა Linux-ის ფორმატზე"
fi

# --------------------------------------------------------- 2. compiler
echo
echo "[2/4] კომპილატორი"
if [ ! -x pawno/pawncc ]; then
    chmod +x pawno/pawncc 2>/dev/null
fi
[ -x pawno/pawncc ] || { echo "  [X] pawno/pawncc ვერ ვიპოვე"; exit 1; }
echo "  [OK] pawno/pawncc"

# ----------------------------------------------------------- 3. compile
echo
echo "[3/4] კომპილაცია"
OUT="gamemodes/freeroam.amx"
./pawno/pawncc gamemodes/freeroam.pwn -o"gamemodes/freeroam" \
    -iincludes -ipawno/include -d3 '-(+' '-;+' 2>&1 | tail -n 8

if [ -f "$OUT" ]; then
    echo "  [OK] $OUT ($(stat -c%s "$OUT") ბაიტი)"
else
    echo "  [X] კომპილაცია ვერ მოხერხდა - ნახე შეცდომები ზემოთ"
    exit 1
fi

# -------------------------------------------------------- 4. next steps
echo
echo "[4/4] დარჩენილი ნაბიჯები (ერთხელ):"
echo
echo "  ა) შექმენი ბაზა (თუ ჯერ არ გაქვს):"
echo "       mysql -u root -p < database.sql"
echo
echo "  ბ) gamemodes/freeroam.pwn-ის თავში შეავსე DB_HOST / DB_USER / DB_PASS / DB_NAME"
echo "     (ნაგულისხმევი: 127.0.0.1 / root / '' / freeroam) და ხელახლა გაუშვი: ./setup.sh"
echo
echo "  გ) ჩამოტვირთე SA-MP 0.3.7 Linux სერვერი და გადმოიტანე ეს ფაილები:"
echo "       https://www.sa-mp.com/download.php  ->  samp037svr_R2-1.tar.gz"
echo "       (ამ საქაღალდეში დააკოპირე: samp03svr, announcement, server.cfg,"
echo "        gamemodes/, plugins/, scriptfiles/ - დანარჩენი უკვე აქ არის)"
echo "       დეტალები: README.md, ნაწილი 4."
echo
echo "  დ) server.cfg-ში შეცვალე rcon_password"
echo
echo "  ე) გაშვება:"
echo "       ./samp03svr"
echo
echo "  დეტალური ინსტრუქცია: README.md"
echo
