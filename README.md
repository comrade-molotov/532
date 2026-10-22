# SA-MP 0.3.7 Freeroam (Grand Larceny სტილი) — მზა სერვერი

მარტივი, სტაბილური freeroam სერვერი: ანგარიშები, მანქანები, ტელეპორტი, სკინები,
კაზინო (4 თამაში), drift რეჟიმი და drift დუელი, ადმინ სისტემა ანტიჩიტით,
სტატისტიკა (`/stats`, `/top`).

**სწრაფი დაწყება (2 ბრძანება):**

| OS | ბრძანებები |
|---|---|
| Windows | `setup.bat` → შემდეგ `samp-server.exe` |
| Linux | `./setup.sh` → შემდეგ `./samp03svr` |

`setup` თვითონ ჩამოტვირთავს ყველა plugin-ს, აკომპილირებს gamemode-ს და დაგიტოვებთ
მხოლოდ ბაზის შექმნას + `rcon_password`-ის შეცვლას (იხ. ქვემოთ).

---

## 1. ფაილების სტრუქტურა

```
gamemodes/freeroam.pwn        მთავარი ფაილი: DB კავშირი, callback-ები, ცხრილების შექმნა
includes/playerdata.inc       მოთამაშის მონაცემები (money/score/admin), MySQL handle, /stats, /top
includes/accounts.inc         რეგისტრაცია/ავტორიზაცია + SHA-256 პაროლის ჰეშირება
includes/vehicles.inc         /v, drift რეჟიმი (TextDraw HUD), /duel (drift დუელი)
includes/casino.inc           /casino: blackjack, roulette, slots, poker + casino_log
includes/admin.inc            ადმინ დონეები 1-3, ბანები, ანტიჩიტი
includes/a_samp.inc ...       SA-MP სტანდარტული include-ები
includes/a_mysql.inc          MySQL plugin-ის include (R41-4 API)
includes/sscanf2.inc          sscanf2 include
includes/streamer.inc         streamer include

pawno/pawncc                 კომპილატორი (Linux; Windows-ზე setup ჩამოტვირთავს pawncc.exe-ს)
pawno/include/               კომპილატორის სტანდარტული include-ები (float.inc, string.inc, ...)

plugins/                     აქ უნდა იყოს: mysql, streamer, sscanf (.dll ან .so)
server.cfg                   მზა კონფიგურაცია (plugins-ის ხაზი, gamemode0, rcon_password)
database.sql                 ბაზის სქემა: accounts, casino_log, bans

download_dependencies.sh/.bat   მხოლოდ plugin-ების ჩამოტვირთვა
setup.sh / setup.bat            ჩამოტვირთვა + კომპილაცია (მთავარი ბრძანება)
README.md                       ეს ინსტრუქცია
```

> `plugins/` რეპოზიტორიაში ცარიელია, რადგან plugin-ები ბინარული ფაილებია და
> GitHub-იდან ჩამოიტვირთება. `setup` ამას ერთ ბრძანებაში აკეთებს — ხელით
> ძებნა არ დაგჭირდება. თუ სკრიპტის გარეშე გინდა, იხ. ნაწილი 5.

---

## 2. Windows — ნაბიჯ-ნაბიჯ

### ნაბიჯი 1. SA-MP 0.3.7 სერვერი
ჩამოტვირთე `samp037_svr_R2-1-1_win32.zip`:
<https://www.sa-mp.com/download.php> (აირჩიე **Windows Server**).

გახსენი არქივი და მისი შიგთავსი (`samp-server.exe`, `announce.exe`, `samp-npc.exe`,
`server.cfg`, `npcmodes/`, `filterscripts/`) გადმოიტანე **ამ რეპოზიტორიის ძირში**.
უკვე არსებული `server.cfg` და `gamemodes/` **არ გადააწერო** — ისინი აქ მზადაა.

საბოლოო საქაღალდე უნდა გამოიყურებოდეს ასე:

```
samp-server.exe
samp-npc.exe
announce.exe
server.cfg            <- აქედან
gamemodes/freeroam.amx
gamemodes/freeroam.pwn
plugins/mysql.dll  streamer.dll  sscanf.dll
libmysql.dll          <- mysql plugin-ისთვის
includes/  pawno/  database.sql  README.md
```

### ნაბიჯი 2. ერთი ბრძანება — `setup.bat`
ორჯერ დააწკაპუნე `setup.bat`-ზე (ან `cmd`-ში: `setup.bat`).

ის აკეთებს:
1. `plugins/mysql.dll`, `plugins/streamer.dll`, `plugins/sscanf.dll` + `libmysql.dll`
2. `pawno/pawncc.exe` (თუ არ არის)
3. `gamemodes/freeroam.pwn` → `gamemodes/freeroam.amx`

ბოლოს დაბეჭდავს, რა დარჩა.

### ნაბიჯი 3. MySQL ბაზა
თუ MySQL ჯერ არ გაქვს — დააყენე **XAMPP** (<https://www.apachefriends.org/>) ან
**MySQL Community Server** (<https://dev.mysql.com/downloads/installer/>).
XAMPP-ში უბრალოდ გაუშვი `XAMPP Control Panel` → `MySQL` → **Start**.

ბაზის შექმნა (აირჩიე ერთი):

- **phpMyAdmin** (XAMPP): გახსენი <http://localhost/phpmyadmin> → ჩანართი `SQL` →
  ჩასვი `database.sql`-ის შიგთავსი → `Go`.
- **კონსოლი** (MySQL bin საქაღალდეში, ან სადაც `mysql.exe` არის):
  ```
  mysql -u root -p < database.sql
  ```

### ნაბიჯი 4. კონფიგურაცია
`gamemodes/freeroam.pwn`-ის თავში (სტრიქონები ~21-24):

```
#define DB_HOST "127.0.0.1"
#define DB_USER "root"
#define DB_PASS ""          // XAMPP-ზე ცარიელია; თუ პაროლი გაქვს — ჩაწერე
#define DB_NAME "freeroam"
```

შეცვლის შემდეგ ხელახლა გაუშვი `setup.bat` (რომ `.amx` განახლდეს).

`server.cfg`-ში **აუცილებლად** შეცვალე:
```
rcon_password chmengamgeba    // <- შენი პაროლი
```

### ნაბიჯი 5. გაშვება
ორჯერ დააწკაპუნე `samp-server.exe`-ზე. კონსოლში უნდა დაინახო:

```
[MySQL] Connected to the database.
[MySQL] tables ready.
```

შემდეგ SA-MP-ის კლიენტში დაამატე: `127.0.0.1:7777`.
პირველ შესვლაზე გამოვა **Register** დიალოგი (nickname = username).

---

## 3. Linux — ნაბიჯ-ნაბიჯ

### ნაბიჯი 1. SA-MP 0.3.7 Linux სერვერი
ჩამოტვირთე `samp037svr_R2-1.tar.gz` <https://www.sa-mp.com/download.php>-დან
(აირჩიე **Linux Server**).

```bash
tar -xzf samp037svr_R2-1.tar.gz
cp samp03svr announcement server.cfg.sample /path/to/repo/   # server.cfg-ს არ გადააწერო!
cd /path/to/repo
```

### ნაბიჯი 2. ერთი ბრძანება

```bash
chmod +x setup.sh download_dependencies.sh pawno/pawncc
./setup.sh
```

ის ჩამოტვირთავს `plugins/mysql.so`, `streamer.so`, `sscanf.so`, აკომპილირებს
`gamemodes/freeroam.amx`-ს და დაბეჭდავს დარჩენილ ნაბიჯებს.

საჭირო პაკეტები (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install mysql-server curl unzip
sudo dpkg --add-architecture i386                 # SA-MP სერვერი 32-ბიტიანია
sudo apt install libc6:i386 libstdc++6:i386
```

> MySQL plugin-ის **static** ვერსია იტვირთება (`mysql.so`), ამიტომ 32-ბიტიანი
> `libmysqlclient` არ გჭირდება — საკმარისია `libc6:i386` და `libstdc++6:i386`.

### ნაბიჯი 3. ბაზა და კონფიგურაცია

```bash
sudo mysql < database.sql          # ან: mysql -u root -p < database.sql
```

`gamemodes/freeroam.pwn`-ში შეავსე `DB_USER` / `DB_PASS` (Linux-ზე ხშირად root-ს
პაროლი აქვს), შემდეგ `./setup.sh` ისევ გაუშვი.

`server.cfg`-ში შეცვალე `rcon_password`. Linux-ზე `plugins` ხაზი ასე უნდა იყოს:

```
plugins mysql.so streamer.so sscanf.so
```

> `server.cfg`-ში უკვე `mysql streamer sscanf` წერია (Windows-ის სტილი).
> Linux-ზე `.so` გაფართოება **სავალდებულოა**.

### ნაბიჯი 4. გაშვება

```bash
chmod +x samp03svr
./samp03svr
```

---

## 4. რას აკეთებს `setup` ზუსტად

| ნაბიჯი | Windows | Linux |
|---|---|---|
| plugin-ები | `download_dependencies.ps1` (გამოიძახება `.bat`-იდან) | `download_dependencies.sh` |
| კომპილატორი | `pawno/pawncc.exe` | `pawno/pawncc` (რეპოშია) |
| კომპილაცია | `pawncc gamemodes\freeroam.pwn -ogamemodes\freeroam -iincludes -ipawno\include -d3` | იგივე |
| შედეგი | `gamemodes/freeroam.amx` | `gamemodes/freeroam.amx` |

ხელით კომპილაცია (თუ ოდესმე დაგჭირდა):

```bash
# Linux
./pawno/pawncc gamemodes/freeroam.pwn -ogamemodes/freeroam -iincludes -ipawno/include -d3

# Windows
pawno\pawncc.exe gamemodes\freeroam.pwn -ogamemodes\freeroam -iincludes -ipawno\include -d3
```

---

## 5. ხელით ინსტალაცია (სკრიპტის გარეშე)

ყველა plugin **32-ბიტიანია** (SA-MP სერვერი 32-ბიტიანია).

| Plugin | ვერსია | ფაილი | სად ჩაიდოს |
|---|---|---|---|
| BlueG MySQL | R41-4 | `mysql-R41-4-win32.zip` | `plugins/mysql.dll` + `libmysql.dll` (სერვერის ძირში) |
| BlueG MySQL | R41-4 | `mysql-R41-4-Debian-static.tar.gz` | `plugins/mysql.so` |
| Incognito streamer | 2.9.6 | `samp-streamer-plugin-2.9.6.zip` | `plugins/streamer.dll` / `.so` |
| Y-Less sscanf | 2.13.8 | `sscanf-2.13.8-win32.zip` / `-linux.tar.gz` | `plugins/sscanf.dll` / `.so` |
| Pawn compiler | 3.10.10 | `pawnc-3.10.10-windows.zip` | `pawno/pawncc.exe` |

ბმულები:
- MySQL: <https://github.com/pBlueG/SA-MP-MySQL/releases/tag/R41-4>
- streamer: <https://github.com/samp-incognito/samp-streamer-plugin/releases/tag/v2.9.6>
- sscanf: <https://github.com/Y-Less/sscanf/releases/tag/v2.13.8>
- compiler: <https://github.com/pawn-lang/compiler/releases/tag/v3.10.10>

`includes/`-ში უკვე დევს `a_mysql.inc` (R41-4), `sscanf2.inc`, `streamer.inc` —
ხელახლა არაფერი უნდა ჩამოტვირთო.

---

## 6. ბრძანებები

### ყველა მოთამაშისთვის
| ბრძანება | რას აკეთებს |
|---|---|
| `/v [მოდელი]` | მანქანის გამოძახება და ჩაჯდომა (მაგ. `/v 560`) |
| `/tp` | ტელეპორტის მენიუ (12 ლოკაცია) |
| `/skin` | სკინის მენიუ (15 სკინი) |
| `/drift` | drift რეჟიმის დაწყება (HUD TextDraw-ით) |
| `/stopdrift` | drift-ის დასრულება, ქულის დათვლა |
| `/duel [id] [თანხა]` | drift დუელი მეორე მოთამაშესთან (60 წამი, მინ. $100) |
| `/casino` | კაზინოს მენიუ: Blackjack, Poker, Roulette, Slots |
| `/stats` | საკუთარი სტატისტიკა (money, score, admin level) |
| `/top` | ტოპ-10 ყველაზე მდიდარი მოთამაშე |

### ადმინი (დონე 1–3)
| ბრძანება | დონე | რას აკეთებს |
|---|---|---|
| `/kick [id] [მიზეზი]` | 1 | მოთამაშის გაგდება |
| `/mute [id]` | 1 | ჩატის დადუმება |
| `/goto [id]` | 1 | მოთამაშესთან გადასვლა |
| `/a [ტექსტი]` | 1 | ადმინ ჩატი |
| `/ban [id] [მიზეზი]` | 2 | ბანი (nickname + IP, იწერება `bans` ცხრილში) |
| `/get [id]` | 2 | მოთამაშის თავისთან გამოძახება |
| `/weather [0-20]` | 2 | ამინდი |
| `/time [0-23]` | 2 | საათი |
| `/setscore [id] [ქულა]` | 3 | score-ის დაყენება |
| `/ahelp` | 1 | ბრძანებების სია |

ადმინის დონე იცვლება ბაზაში:
```sql
UPDATE accounts SET admin_level = 3 WHERE username = 'SheniNiki';
```
შემდეგ მოთამაშე ხელახლა უნდა შემოვიდეს სერვერზე.

### კაზინო — თამაშის წესები
- მინიმალური ფსონი **$100**, მაქსიმალური **$100000**; ფსონი არ უნდა აღემატებოდეს ბალანსს.
- **Blackjack**: dealer დგება 17-ზე, natural = 3:2, double მხოლოდ პირველ ორ ბარათზე.
- **Roulette**: წითელი/შავი/ლუწი/კენტი x2, ნული x14.
- **Slots**: წყვილი x2, სამეული x10, სამი `Seven` x50.
- **Poker** (Jacks or Better): წყვილი J+ x1 … Royal Flush x50; ინახავ ბარათებს და აჭერს DRAW-ს.
- ყოველი თამაში იწერება `casino_log` ცხრილში (თამაში, ფსონი, ბალანსი).

### ანტიჩიტი
- სიჩქარე > 500 კმ/სთ → გაფრთხილება
- ფეხით 300 ერთეულზე მეტი ნახტომი ერთ tick-ში → გაფრთხილება
- health/armour > 100 → დაბრუნება 100/0-ზე
- 3 გაფრთხილება → ავტომატური kick

---

## 7. ბაზის ცხრილები

| ცხრილი | დანიშნულება |
|---|---|
| `accounts` | id, username, password (SHA-256), admin_level, money, score, created_at |
| `casino_log` | ყოველი კაზინოს თამაში: account_id, game, amount, balance, created_at |
| `bans` | username, ip, reason, admin, created_at |

ცხრილებს gamemode თვითონაც ქმნის გაშვებისას (`CREATE TABLE IF NOT EXISTS`),
ასე რომ `database.sql` მხოლოდ ბაზის შესაქმნელად არის საჭირო.

---

## 8. ხშირი პრობლემები

**`[MySQL] Connection failed`**
შეამოწმე `DB_HOST/DB_USER/DB_PASS/DB_NAME` და რომ MySQL გაშვებულია.
XAMPP-ზე პაროლი ცარიელია (`DB_PASS ""`). სერვერი ბაზის გარეშე არ ირთვება — ეს განზრახია.

**`Failed to load plugin 'mysql'` / `plugins/mysql.so` not found**
გაუშვი `setup` ხელახლა. Linux-ზე `server.cfg`-ში აუცილებლად `.so` გაფართოებით:
`plugins mysql.so streamer.so sscanf.so`.

**`Script[gamemodes/freeroam.amx]: Run time error` / `.amx` არ არსებობს**
`gamemodes/freeroam.amx` არ არის აწყობილი — გაუშვი `setup.bat` / `./setup.sh`.

**Windows: `libmysql.dll` ვერ მოიძებნა**
ის სერვერის ძირეულ საქაღალდეში უნდა იდოს (`samp-server.exe`-ის გვერდით),
არა `plugins/`-ში. `setup.bat` ამას ავტომატურად აკეთებს.

**Linux: `error while loading shared libraries: libstdc++.so.6`**
`sudo apt install libstdc++6:i386 libc6:i386`.

**`Unknown command`**
ბრძანებამდე მოთამაშე ავტორიზებული უნდა იყოს (`Register`/`Login` დიალოგი).

**ბაზაში ძველი ტესტ-ანგარიშები მინდა წავშალო**
```sql
TRUNCATE TABLE accounts;
TRUNCATE TABLE casino_log;
```

---

## 9. რაც განზრახ გამოვტოვე (სიმარტივისთვის)

- სახლი/ბიზნესი, ინვენტარი, ობიექტები — არ არის
- `/top` მხოლოდ ფულით (არა score-ით/დროით)
- ადმინ დონეები მხოლოდ 1–3
- drift დუელი მხოლოდ 2 მოთამაშეზე, 60 წამი
- ანგარიშის პაროლის აღდგენა ელფოსტით — არ არის (მხოლოდ SHA-256 ჰეში ბაზაში)

---

## 10. მინიმალური მოთხოვნები

- **Windows**: Windows 7+ , MySQL 5.7+ ან XAMPP
- **Linux**: 32-ბიტიანი ბიბლიოთეკები (`libc6:i386`, `libstdc++6:i386`), MySQL 5.7+
- **კლიენტი**: SA-MP 0.3.7 R2/R3
