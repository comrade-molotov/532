# plugins/ — რა უნდა იდოს აქ

ამ საქაღალდეში plugin-ების ბინარული ფაილებია. ისინი რეპოზიტორიაში
**არ არის** (ბინარები Git-ში არ ინახება) — მათ ავტომატურად ჩამოტვირთავს
`setup.sh` (Linux) ან `setup.bat` (Windows).

საბოლოო შიგთავსი:

| Windows | Linux | Plugin |
|---|---|---|
| `mysql.dll` | `mysql.so` | BlueG MySQL R41-4 |
| `streamer.dll` | `streamer.so` | Incognito streamer 2.9.6 |
| `sscanf.dll` | `sscanf.so` | Y-Less sscanf 2.13.8 |

Windows-ზე დამატებით საჭიროა `libmysql.dll` **სერვერის ძირეულ საქაღალდეში**
(`samp-server.exe`-ის გვერდით), არა აქ.

`server.cfg`-ის შესაბამისი ხაზი:

```
plugins mysql streamer sscanf          ; Windows
plugins mysql.so streamer.so sscanf.so ; Linux  (.so სავალდებულოა!)
```

ხელით ჩამოტვირთვა: იხ. მთავარი `README.md`, ნაწილი 5.
