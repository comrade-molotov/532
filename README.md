# SA-MP Freeroam (GL სტილი) — 0.3.7

მინიმალური, სტაბილური freeroam სერვერი. feature-ები ემატება ეტაპობრივად,
მხოლოდ წინა ეტაპის დადასტურების შემდეგ.

**მიმდინარე სტატუსი: ეტაპი 1 — მონაცემთა ბაზა + ანგარიშის სისტემა (დასრულებულია)**

---

## 1. ფაილების სტრუქტურა

```
gamemodes/freeroam.pwn      მთავარი ფაილი: DB კავშირი, ცხრილის შექმნა, SA-MP callback-ები
includes/playerdata.inc     მოთამაშის მონაცემები (money/score/admin), MySQL handle, save
includes/accounts.inc       რეგისტრაცია/ავტორიზაცია + SHA256 (პაროლის ჰეშირება)
database.sql                SQL სქემა (ამ ეტაპზე მხოლოდ accounts ცხრილი)
README.md                   ეს ინსტრუქცია
```

შემდეგ ეტაპებზე დაემატება: `includes/vehicles.inc`, `includes/casino.inc`,
`includes/admin.inc` — მეტი ფაილი არ იქნება.

---

## 2. რა მუშაობს ამ ეტაპზე

- MySQL კავშირი გაშვებისას + `accounts` ცხრილის ავტომატური შექმნა
- ავტორიზაცია მხოლოდ **username + password**-ით (ავტომატურად nick-იდან)
- 2 დიალოგი: `Login` და `Register` (DIALOG_STYLE_PASSWORD)
- პაროლი ინახება **SHA256** ჰეშად (64 hex სიმბოლო), არასდროს plain text-ად
- ავტორიზაციამდე მოთამაშე spectate-ზეა — ვერ იძვრება და ვერ ჩნდება
- წარმატებული login-ის შემდეგ: spawn, money/score/admin_level ჩატვირთვა
- დისკონექტზე: money/score ინახება ბაზაში
- ახალი ანგარიშის საწყისი მდგომარეობა: **$5000**, score 0, admin 0
  (იცვლება `includes/playerdata.inc`-ში — `START_MONEY` / `START_SCORE`)

---

## 3. რა ფაილები გჭირდება და საიდან

### ა) SA-MP 0.3.7 სერვერი
https://www.sa-mp.com/download.php → `samp037_svr_R2-1-1_win32.zip` (Windows) ან
`samp037svr_R2-1.tar.gz` (Linux).
ამ ეტაპზე plugin-ებიდან მხოლოდ MySQL გჭირდება.

### ბ) MySQL plugin — BlueG R41-4 (აუცილებელი)
რელიზის გვერდი: https://github.com/pBlueG/SA-MP-MySQL/releases/tag/R41-4

| OS | ფაილი | რას შეიცავს |
|---|---|---|
| Windows | `mysql-R41-4-win32.zip` | `plugins/mysql.dll` + `pawno/include/a_mysql.inc` |
| Linux | `mysql-R41-4-Debian-static.tar.gz` | `plugins/mysql_static.so` (libmysqlclient არ სჭირდება) |
| Linux | `mysql-R41-4-Debian.tar.gz` | `plugins/mysql.so` (სჭირდება 32-ბიტიანი libmysqlclient) |
| Linux | `mysql-R41-4-CentOS7.tar.gz` | CentOS 7-ის ვარიანტი |

> მნიშვნელოვანი: პლაგინი **32-ბიტიანია** (SA-MP სერვერი 32-ბიტიანია).
> ანგარიშის ჰეშირება სკრიპტშია (`accounts.inc`), ამიტომ `a_mysql.inc`-ის
> ძველი/ახალი ვერსია არაფერს ცვლის — მთავარია R41-4 იყოს.

### გ) კომპილატორი
- **Windows:** `pawno/pawncc.exe` (მოყვება SA-MP სერვერის პაკეტს, Pawn 3.10.10).
- **Linux:** `.amx`-ის კომპილაცია ყველაზე მარტივია Windows-ზე pawno-თი და
  მიღებული `freeroam.amx`-ის ატვირთვა. თუ Linux-ზე გინდა კომპილაცია:
  https://github.com/pawn-lang/compiler/releases (Pawn 3.10.10, `pawncc`).

ამ ეტაპზე **არ** გჭირდება: sscanf2, streamer, YSI. ისინი დაემატება მხოლოდ
მაშინ, როცა რეალურად იქნება საჭირო (stage 3+).

---

## 4. MySQL-ის მომზადება

1. დააინსტალირე MySQL 5.7 ან MariaDB (რეკომენდებულია R41-4-ისთვის).
   MySQL 8-ზე შესაძლოა ავტორიზაციის შეცდომა მიიღო — მაშინ შექმენი იუზერი
   ძველი ავტორიზაციით:
   ```sql
   CREATE USER 'freeroam'@'localhost' IDENTIFIED WITH mysql_native_password BY 'შენი_პაროლი';
   GRANT ALL PRIVILEGES ON freeroam.* TO 'freeroam'@'localhost';
   FLUSH PRIVILEGES;
   ```
2. გაუშვი `database.sql` (ქმნის ბაზას და `accounts` ცხრილს).
   `accounts` ცხრილს სკრიპტიც თვითონ შექმნის გაშვებისას, მაგრამ ბაზა
   (`CREATE DATABASE`) ხელით უნდა არსებობდეს.

---

## 5. კონფიგურაცია

`gamemodes/freeroam.pwn` — ზედა ნაწილი:

```pawn
#define DB_HOST "127.0.0.1"
#define DB_USER "root"
#define DB_PASS ""
#define DB_NAME "freeroam"
```

თუ ბაზას ვერ დაუკავშირდა, სერვერი ლოგში ჩაწერს მიზეზს და თვითონ გაითიშება
(რომ "ცარიელი" სერვერი არ დარჩეს). ეს განზრახ ასეა.

---

## 6. კომპილაცია (Windows / pawno)

1. `includes/` საქაღალდე უნდა იყოს სერვერის root-ში, `gamemodes/`-ის გვერდით.
2. გახსენი `pawno/pawno.exe` → `gamemodes/freeroam.pwn` → **F5**.
3. უნდა გამოვიდეს: `1 Errors` → **0 errors, 0 warnings**.
   (კოდი შემოწმებულია Pawn 3.10.10-ზე — 0 error, 0 warning.)

`#include "../includes/...` ფარდობითი გზაა, ამიტომ pawno-ს სამუშაო
საქაღალდეს მნიშვნელობა არ აქვს — იმუშავებს ისე, როგორც არის.

---

## 7. Windows-ზე გაშვება

1. `mysql-R41-4-win32.zip`-იდან:
   - `mysql.dll` → `plugins/mysql.dll`
   - `a_mysql.inc` → `pawno/include/a_mysql.inc`
2. `server.cfg`:
   ```
   echo Executing Server Config...
   lanmode 0
   rcon_password changeme
   maxplayers 50
   port 7777
   hostname Freeroam
   gamemode0 freeroam 1
   plugins mysql
   announce 0
   query 1
   weburl www.sa-mp.com
   onfoot_rate 40
   incar_rate 40
   weapon_rate 40
   stream_distance 300.0
   stream_rate 1000
   maxnpc 0
   logtimeformat [%H:%M:%S]
   language English
   ```
3. თუ plugin არ ჩაიტვირთა — დააყენე **Microsoft Visual C++ 2015 Redistributable (x86)**
   და, საჭიროების შემთხვევაში, 2010/2012 x86 ვერსიებიც (R41-4-ის README-შია).
4. გაუშვი `samp-server.exe`.

---

## 8. Linux-ზე გაშვება

```bash
# 1. 32-ბიტიანი ბიბლიოთეკები SA-MP სერვერისთვის (Debian/Ubuntu)
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install libc6:i386 libstdc++6:i386 zlib1g:i386

# 2. პლაგინი: უმარტივესი გზაა სტატიკური ვერსია (libmysqlclient არ სჭირდება)
tar xzf mysql-R41-4-Debian-static.tar.gz
cp plugins/mysql_static.so /path/to/server/plugins/

# 3. server.cfg-ში:
#    plugins mysql_static.so

# 4. გაშვება
chmod +x samp03svr announce
./samp03svr
```

**თუ არასტატიკურ ვერსიას იყენებ (`mysql.so`):** საჭიროა 32-ბიტიანი MySQL client
ბიბლიოთეკა, თორემ plugin ჩაიტვირთება შეცდომით
(`libmysqlclient.so.18: cannot open shared object file`):

```bash
sudo apt install libmysqlclient-dev:i386      # ან libmariadb-dev:i386
# ძველ დისტრიბუტივებზე: libmysqlclient18:i386
```

სხვა ხშირი შეცდომა: `version GLIBCXX_3.4.15 not found` → ძალიან ძველი GCC-ის
ბიბლიოთეკაა, განაახლე სისტემა ან გამოიყენე სტატიკური `.so`.

---

## 9. ტესტის ჩეკლისტი (ამ ეტაპისთვის)

1. სერვერის ლოგში ჩანს:
   `[MySQL] Connected to the database.` და `[MySQL] accounts table ready.`
2. პირველად შესვლისას გამოდის **Register** დიალოგი → პაროლი 3–32 სიმბოლო.
3. რეგისტრაციის შემდეგ მოთამაშე spawn-დება, ჩნდება `Welcome ... Money: $5000`.
4. გათიშვის და ხელახლა შესვლისას გამოდის **Login** დიალოგი.
5. არასწორი პაროლი → `Wrong password.` და დიალოგი ისევ ჩნდება.
6. `accounts` ცხრილში `password` არის 64-სიმბოლოიანი hex (მაგ.
   `5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8` = "password").
7. დისკონექტის შემდეგ ბაზაში money/score განახლებულია.

---

## 10. რაც გამოვტოვე სიმარტივისთვის (გითხრი, თუ გინდა დავამატო)

- **პაროლის დადასტურება რეგისტრაციისას** (მეორე ველი) — ერთი დიალოგით უფრო სუფთაა.
- **ცდების ლიმიტი / anti-bruteforce** — ამ ეტაპზე არ არის.
- **ავტომატური save ტაიმერით** — ახლა ინახება მხოლოდ დისკონექტზე.
- **`/stats`, `/top money`** — მოდის სტატისტიკის ეტაპზე (მონაცემები უკვე მზადაა).
- **`bans` ცხრილი** — მოდის ადმინის ეტაპზე.

---

## 11. შემდეგი ეტაპი

ეტაპი 2/3: `/v [id]`, `/tp` მენიუ, `/skin`, ადმინის weather/time — ამის შემდეგ
მოდის Blackjack. დაელოდე დადასტურებას, რომ ეს ეტაპი მუშაობს.
