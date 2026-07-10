# getbar

`getbar` lädt eine URL per HTTP- oder HTTPS-GET (über libcurl) herunter und gibt eine
**Balkenreihe** auf der Standardausgabe aus: entweder Empfangszeiten pro Block oder
Bytes pro Zeitabschnitt. Optionales Polynom-Fitting, Durchsatzschätzungen und
gnuplot-Diagramme machen das Tool nützlich zum Benchmarken von Mirrors, CDNs und
direkten Downloads.

```bash
getbar [OPTION]... URL
```

Einer der Modi **Block** (`-b`) oder **Intervall** (`-i`) ist erforderlich.

## Übersetzungen

| Sprache | README |
|---------|--------|
| English | [README.md](README.md) |
| 简体中文 | [README-zh_CN.md](README-zh_CN.md) |
| 繁體中文 | [README-zh_TW.md](README-zh_TW.md) |
| 日本語 | [README-ja.md](README-ja.md) |
| 한국어 | [README-ko.md](README-ko.md) |
| ไทย | [README-th.md](README-th.md) |
| Tiếng Việt | [README-vi.md](README-vi.md) |
| Français | [README-fr.md](README-fr.md) |
| Deutsch | README-de.md (diese Datei) |
| Italiano | [README-it.md](README-it.md) |
| Esperanto | [README-eo.md](README-eo.md) |

## Ausgabe

### Blockmodus (`-b`)

Die Werte sind Dauern in **Mikrosekunden**, durch Leerzeichen getrennt in einer Zeile.

| Position | Bedeutung |
|----------|-----------|
| Erster Wert | Wartezeit bis zum Eintreffen des ersten Bytes (TTFB) |
| Weitere Werte | Zeit zum Empfang jedes Blocks von `-b` Bytes |

Beispiel:

```text
245000 1200 980 1100 1050
```

### Intervallmodus (`-i`)

Die Werte sind **empfangene Bytes** in jedem Zeitabschnitt, durch Leerzeichen getrennt in einer Zeile.
Führende Nullen bedeuten, dass noch keine Daten angekommen sind (Verbindungs- oder Serververzögerung).

Beispiel:

```text
0 0 0 0 65536 131072 65536
```

Nach der Reihenzeile können optionale Zusatzzeilen erscheinen:

- **Polynom** (`-p`): `offset coef_n … coef_0` (hohe Ordnung zuerst nach dem Offset)
- **Schätzung** (`-e`): durchschnittliche Bytes pro Sekunde ab einem Punkt nach Datenbeginn

Mit `-q` wird die Reihe unterdrückt und nur die Zusatzzeilen ausgegeben.

Das Ende des Transfers ist erreicht, wenn der Download abgeschlossen ist, `-w` / `--window` abläuft,
`-s` / `--size-max` erreicht wird oder die Anfrage fehlschlägt.

Im Intervallmodus werden nachlaufende Null-Buckets nach EOF verworfen (mit einem kurzen
0,1-s-Puffer, damit der letzte partielle Abschnitt erhalten bleibt).

## Optionen

| Option | Beschreibung |
|--------|--------------|
| `-b`, `--block-size=NUM[kmgKMG]` | Blockgröße für den Blockmodus |
| `-i`, `--interval=NUM[um][s]` | Abschnittslänge für den Intervallmodus; `NUM` kann gebrochen sein (`0.1s`, `100.5ms`) |
| `-s`, `--size-max=NUM[kmgKMG]` | Stopp nach Empfang von `NUM` Bytes |
| `-w`, `--window=NUM[um][s]` | Stopp nach einer Zeitbegrenzung |
| `-p`, `--polynomial=ORDER` | Polynom anpassen (Ordnung 0–7) und Offset + Koeffizienten ausgeben |
| `-g`, `--gnuplot=FILE` | Diagramm oder gnuplot-Skript schreiben (siehe unten) |
| `-f`, `--force` | Vorhandene gnuplot-Ausgabedatei überschreiben |
| `-e`, `--estimate-bps=NUM[um][s]` | Geschätzte B/s ausgeben, gemessen ab `NUM` nach Datenbeginn |
| `-v`, `--verbose` | URL und Byteanzahl auf stderr protokollieren |
| `-q`, `--quiet` | Balkenreihe auf stdout weglassen |
| `-h`, `--help` | Hilfe |
| `--version` | Version |

### Größensuffixe (`-b`, `-s`)

`k`/`m`/`g`/`t` (Basis 1024) oder `K`/`M`/`G`/`T` (Basis 1000), z. B. `64k`, `10M`.

### Zeitsuffixe (`-i`, `-w`, `-e`)

| Suffix | Bedeutung |
|--------|-----------|
| (keines) | Sekunden |
| `ms` oder `m` | Millisekunden |
| `us` oder `u` | Mikrosekunden |
| `s` | Sekunden (explizit) |

Nackte Zahlen sind standardmäßig Sekunden. Beispiele: `100ms`, `0.1s`, `0.1`, `500us`.

### Kurzoptionen

Gruppierte Flags werden unterstützt, wenn der Wert nur an den **letzten** Buchstaben angehängt ist:

```bash
getbar -vfw3 -i100ms https://example.com/file
# entspricht: -v -f -w3 -i100ms
```

Hängen Sie nicht zwei Optionswerte in einer Gruppe aneinander (stattdessen separate Optionen verwenden).

## Polynom-Fitting (`-p`)

Der **Offset** ist die Leerlaufphase vor nützlichen Daten:

- **Intervallmodus**: Verzögerung in Sekunden (führende Null-Abschnitte × Intervall).
- **Blockmodus**: Anzahl der Blöcke mit Null-Dauer nach dem TTFB-Balken.

Die Koeffizienten beschreiben ein Polynom-Fitting über den nicht-leeren Teil der Reihe.
In Kombination mit `-g` wird die Kurve im Diagramm gezeichnet.

## Gnuplot-Ausgabe (`-g`)

| Erweiterung | Verhalten |
|-------------|-----------|
| `.png`, `.svg`, `.pdf`, `.eps` | Mit `gnuplot` gerendert (muss installiert sein) |
| anderes | Nur gnuplot-Skript (`gnuplot` nicht erforderlich) |

Diagramme verwenden Balkendiagramme für die Reihe. Leerlauf-Buckets **vor** dem Polynom-Offset
werden im Plot weggelassen, damit sich das Diagramm auf den aktiven Transfer konzentriert.
Mit `-p` wird das angepasste Polynom als Linie überlagert.

Beispiel (Intervallmodus mit Polynom-Überlagerung):

![getbar-Durchsatzdiagramm](images/chart.png)\

Optionales Theming: Kopieren Sie
`share/getbar/gnuplot.rc.example` nach
`$XDG_CONFIG_HOME/getbar/gnuplot.rc` (oder `~/.config/getbar/gnuplot.rc`).

## Beispiele

### Grundlegende Messungen

Zeiten pro Block mit 64-KiB-Blöcken:

```bash
getbar -b 64k https://example.com/file
```

100-ms-Abschnitte, Geschwindigkeitsschätzung nach 1 s Daten, Reihe ausgeblendet:

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

Time-to-first-byte (Mikrosekunden) aus dem ersten Feld im Blockmodus auslesen:

```bash
getbar -b 64k -s 256k https://example.com/file | awk '{print $1}'
```

### Kontrollierte Downloadgröße oder -dauer

10 MiB abtasten und eine stationäre Geschwindigkeitsschätzung ausgeben (nützlich für Mirrors):

```bash
getbar -i 100ms -s 10M -e 2s -q https://mirror.example/file
```

Höchstens 30 s laufen lassen, unabhängig von der Dateigröße (Dauerlasttest / CDN-Edge):

```bash
getbar -i 1s -w 30s -v https://cdn.example/large.bin
```

Zeit und Bytes begrenzen — Stopp bei der zuerst erreichten Grenze:

```bash
getbar -b 1M -s 50M -w 60s https://example.com/file
```

### Diagramme und Analyse

Gebrochenes Intervall, 3-s-Obergrenze, ausführliches Protokoll, Diagramm erzwingen überschreiben:

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

Blockdiagramm mit quadratischer Polynom-Überlagerung:

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

Intervall-Diagramm mit Polynom (entspricht der Beispielabbildung oben):

```bash
getbar -i 100ms -p 2 -g chart.png -f https://example.com/file
```

SVG für Berichte oder Wikis exportieren:

```bash
getbar -i 200ms -p 2 -g report.svg -f https://example.com/file
```

gnuplot-Skript zur späteren Bearbeitung speichern (`gnuplot` zur Laufzeit nicht erforderlich):

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

### Mirrors oder URLs vergleichen

Eine geschätzte B/s-Zeile pro URL ausgeben (je 5 MiB Probe):

```bash
for url in https://mirror-a.example/file https://mirror-b.example/file; do
  printf '%s\t' "$url"
  getbar -i 100ms -s 5M -e 1s -q "$url"
done
```

Mirrors nach geschätztem Durchsatz sortieren:

```bash
while read -r url; do
  bps=$(getbar -i 100ms -s 5M -e 1s -q "$url") || continue
  printf '%s\t%s\n' "$bps" "$url"
done < mirrors.txt | sort -nr
```

### Shell-Integration

Ausführliche Transferdetails auf stderr protokollieren, stdout maschinenlesbar halten:

```bash
getbar -v -i 100ms -s 10M https://example.com/file > series.txt
```

Reihe und Diagramm in einem Durchlauf speichern:

```bash
getbar -i 100ms -s 20M -g run.png -f https://example.com/file | tee series.txt
```

Grobe MiB/s aus der Schätzzeile:

```bash
bps=$(getbar -i 100ms -s 10M -e 1s -q https://example.com/file)
awk -v b="$bps" 'BEGIN { printf "~ %.2f MiB/s\n", b / 1024 / 1024 }'
```

## Abhängigkeiten

- **Erforderlich**: libcurl
- **Empfohlen**: gnuplot (für Bildausgabe über `-g`)
- Siehe `getbar(1)` für die Handbuchseite.

## Siehe auch

- `getbar(1)` — Handbuchseite
- `share/getbar/gnuplot.rc.example` — Beispiel für Diagramm-Theming
