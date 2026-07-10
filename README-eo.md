# getbar

`getbar` elŝutas URL per HTTP aŭ HTTPS GET (per libcurl) kaj registras
**stangserion** sur la norma eligo: aŭ ricevotempoj po bloko aŭ bajtoj po
tempa tranĉo. La opcia polinoma adaptado, trafiktaksadoj kaj gnuplot-diagramoj
faras ĝin utila por mezuri spegulojn, CDN-ojn kaj rektajn elŝutojn.

```bash
getbar [OPTION]... URL
```

Unu el **bloka reĝimo** (`-b`) aŭ **intervala reĝimo** (`-i`) estas deviga.

## Tradukoj

| Lingvo | README |
|--------|--------|
| English | [README.md](README.md) |
| 简体中文 | [README-zh_CN.md](README-zh_CN.md) |
| 繁體中文 | [README-zh_TW.md](README-zh_TW.md) |
| 日本語 | [README-ja.md](README-ja.md) |
| 한국어 | [README-ko.md](README-ko.md) |
| ไทย | [README-th.md](README-th.md) |
| Tiếng Việt | [README-vi.md](README-vi.md) |
| Français | [README-fr.md](README-fr.md) |
| Deutsch | [README-de.md](README-de.md) |
| Italiano | [README-it.md](README-it.md) |
| Esperanto | README-eo.md (ĉi tiu dosiero) |

## Eligo

### Bloka reĝimo (`-b`)

La valoroj estas daŭroj en **mikrosekundoj**, apartigitaj per spacoj sur unu linio.

| Pozicio | Signifo |
|---------|---------|
| Unua valoro | Atendotempo antaŭ alveno de la unua bajto (TTFB) |
| Postaj valoroj | Tempo por ricevi ĉiun blokon de `-b` bajtoj |

Ekzemplo:

```text
245000 1200 980 1100 1050
```

### Intervala reĝimo (`-i`)

La valoroj estas **ricevitaj bajtoj** en ĉiu tempa tranĉo, apartigitaj per spacoj sur unu linio.
Komencaj nuloj signifas, ke ankoraŭ ne alvenis datumoj (konekto- aŭ servila prokrasto).

Ekzemplo:

```text
0 0 0 0 65536 131072 65536
```

Post la serilinio povas aperi opciaj aldonaj linioj:

- **Polinomo** (`-p`): `offset coef_n … coef_0` (alta ordo unue post la offset)
- **Taksado** (`-e`): mezumaj bajtoj sekunde de punkto post komenco de datumoj

Uzu `-q` por subpremi la serion kaj presi nur la aldonajn liniojn.

Fino de la transigo estas kiam la elŝuto finiĝas, `-w` / `--window` eksvalidiĝas,
`-s` / `--size-max` estas atingita, aŭ la peto malsukcesas.

En intervala reĝimo, finaj nulaj segmentoj post EOF estas forigitaj (kun mallonga
0,1 s tolerperiodo por konservi la lastan partan tranĉon).

## Opcioj

| Opcio | Priskribo |
|-------|-----------|
| `-b`, `--block-size=NUM[kmgKMG]` | Blokgrandeco por bloka reĝimo |
| `-i`, `--interval=NUM[um][s]` | Tranĉa longo por intervala reĝimo; `NUM` povas esti frakcia (`0.1s`, `100.5ms`) |
| `-s`, `--size-max=NUM[kmgKMG]` | Haltu post ricevo de `NUM` bajtoj |
| `-w`, `--window=NUM[um][s]` | Haltu post tempolimo |
| `-p`, `--polynomial=ORDER` | Adaptu polinomon (ordo 0–7) kaj presu offset + koeficientojn |
| `-g`, `--gnuplot=FILE` | Skribu diagramon aŭ gnuplot-skripton (vidu sube) |
| `-f`, `--force` | Anstataŭigu ekzistantan gnuplot-eligan dosieron |
| `-e`, `--estimate-bps=NUM[um][s]` | Presu takitan B/s mezuritan de `NUM` post komenco de datumoj |
| `-v`, `--verbose` | Protokolu URL kaj bajtkalkulon sur stderr |
| `-q`, `--quiet` | Preterlasu la stangserion sur stdout |
| `-h`, `--help` | Helpo |
| `--version` | Versio |

### Grandecaj sufiksoj (`-b`, `-s`)

`k`/`m`/`g`/`t` (bazo 1024) aŭ `K`/`M`/`G`/`T` (bazo 1000), ekz. `64k`, `10M`.

### Tempaj sufiksoj (`-i`, `-w`, `-e`)

| Sufikso | Signifo |
|---------|---------|
| (neniu) | sekundoj |
| `ms` aŭ `m` | milisekundoj |
| `us` aŭ `u` | mikrosekundoj |
| `s` | sekundoj (eksplicite) |

Nudaj nombroj defaŭlte estas sekundoj. Ekzemploj: `100ms`, `0.1s`, `0.1`, `500us`.

### Mallongaj opcioj

Grupigitaj flagoj estas subtenataj kiam la valoro estas gluita nur al la **lasta** litero:

```bash
getbar -vfw3 -i100ms https://example.com/file
# same kiel: -v -f -w3 -i100ms
```

Ne gluu du opcivalorojn en unu grupon (uzu apartajn opciojn anstataŭe).

## Polinoma adaptado (`-p`)

La **offset** estas la neaktiva periodo antaŭ utilaj datumoj:

- **Intervala reĝimo**: prokrasto en sekundoj (komencaj nulaj tranĉoj × intervalo).
- **Bloka reĝimo**: nombro de blokoj kun nula daŭro post la TTFB-stango.

La koeficientoj priskribas polinoman adaptadon sur la ne-neaktiva parto de la serio.
Kombinite kun `-g`, la kurbo estas desegnita sur la diagramo.

## Gnuplot-eligo (`-g`)

| Etendo | Konduto |
|--------|---------|
| `.png`, `.svg`, `.pdf`, `.eps` | Bildigita per `gnuplot` (devas esti instalita) |
| io alia | Nur gnuplot-skripto (`gnuplot` ne necesas) |

Diagramoj uzas stangajn diagramojn por la serio. Neaktivaj segmentoj **antaŭ** la polinoma
offset estas preterlasitaj en la diagramo por fokusi al la aktiva transigo.
Kun `-p`, la adaptita polinomo estas supermetita kiel linio.

Ekzemplo (intervala reĝimo kun polinoma supermeto):

![getbar-trafikdiagramo](images/chart.png)

Opcia etoso: kopiu
`share/getbar/gnuplot.rc.example` al
`$XDG_CONFIG_HOME/getbar/gnuplot.rc` (aŭ `~/.config/getbar/gnuplot.rc`).

## Ekzemploj

### Bazaj mezuradoj

Tempoj po bloko kun 64 KiB-blokoj:

```bash
getbar -b 64k https://example.com/file
```

100 ms tranĉoj, rapidtaksado post 1 s da datumoj, serio kaŝita:

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

Legu tempon ĝis unua bajto (mikrosekundoj) el la unua kampo en bloka reĝimo:

```bash
getbar -b 64k -s 256k https://example.com/file | awk '{print $1}'
```

### Kontrolita elŝutgrandeco aŭ daŭro

Prenu specimenon de 10 MiB kaj presu takitan stabilan rapidon (utila por speguloj):

```bash
getbar -i 100ms -s 10M -e 2s -q https://mirror.example/file
```

Daŭru maksimume 30 s sendepende de dosiergrandeco (daŭra testo / CDN-rando):

```bash
getbar -i 1s -w 30s -v https://cdn.example/large.bin
```

Limigu kaj tempon kaj bajtojn — haltu je la unua atingita limo:

```bash
getbar -b 1M -s 50M -w 60s https://example.com/file
```

### Diagramoj kaj analizo

Frakcia intervalo, 3 s plafono, detala protokolo, deviga anstataŭigo de diagramo:

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

Bloka diagramo kun kvadrata polinoma supermeto:

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

Intervala diagramo kun polinomo (kongruas kun la ekzempla figuro supre):

```bash
getbar -i 100ms -p 2 -g chart.png -f https://example.com/file
```

Eksportu SVG por raportoj aŭ vikioj:

```bash
getbar -i 200ms -p 2 -g report.svg -f https://example.com/file
```

Konservu gnuplot-skripton por posta redakto (`gnuplot` ne necesas dum rulado):

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

### Komparo de speguloj aŭ URL-oj

Presu unu takitan B/s-linion po URL (po 5 MiB specimenon):

```bash
for url in https://mirror-a.example/file https://mirror-b.example/file; do
  printf '%s\t' "$url"
  getbar -i 100ms -s 5M -e 1s -q "$url"
done
```

Ordigu spegulojn laŭ takita trafiko:

```bash
while read -r url; do
  bps=$(getbar -i 100ms -s 5M -e 1s -q "$url") || continue
  printf '%s\t%s\n' "$bps" "$url"
done < mirrors.txt | sort -nr
```

### Ŝelintegrado

Protokolu detalajn transiginformojn sur stderr dum stdout restas maŝinelegebla:

```bash
getbar -v -i 100ms -s 10M https://example.com/file > series.txt
```

Konservu serion kaj diagramon en unu rulo:

```bash
getbar -i 100ms -s 20M -g run.png -f https://example.com/file | tee series.txt
```

Proksimuma MiB/s el la takadlinio:

```bash
bps=$(getbar -i 100ms -s 10M -e 1s -q https://example.com/file)
awk -v b="$bps" 'BEGIN { printf "~ %.2f MiB/s\n", b / 1024 / 1024 }'
```

## Dependecoj

- **Necesa**: libcurl
- **Rekomendita**: gnuplot (por bilda eligo per `-g`)
- Vidu `getbar(1)` por la manpaĝo.

## Vidu ankaŭ

- `getbar(1)` — manpaĝo
- `share/getbar/gnuplot.rc.example` — ekzemplo de diagrama etoso
