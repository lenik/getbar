# getbar

`getbar` scarica un URL con HTTP o HTTPS GET (tramite libcurl) e registra una
**serie a barre** sull'output standard: tempi di ricezione per blocco oppure byte per
fetta temporale. L'adattamento polinomiale opzionale, le stime di throughput e i
grafici gnuplot lo rendono utile per benchmark di mirror, CDN e download diretti.

```bash
getbar [OPTION]... URL
```

È richiesta una delle modalità **blocco** (`-b`) o **intervallo** (`-i`).

## Traduzioni

| Lingua | README |
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
| Italiano | README-it.md (questo file) |
| Esperanto | [README-eo.md](README-eo.md) |

## Output

### Modalità blocco (`-b`)

I valori sono durate in **microsecondi**, separati da spazi su una riga.

| Posizione | Significato |
|-----------|-------------|
| Primo valore | Tempo di attesa prima dell'arrivo del primo byte (TTFB) |
| Valori successivi | Tempo per ricevere ogni blocco di `-b` byte |

Esempio:

```text
245000 1200 980 1100 1050
```

### Modalità intervallo (`-i`)

I valori sono i **byte ricevuti** in ogni fetta temporale, separati da spazi su una riga.
Gli zeri iniziali indicano che non sono ancora arrivati dati (ritardo di connessione o del server).

Esempio:

```text
0 0 0 0 65536 131072 65536
```

Dopo la riga della serie possono comparire righe aggiuntive opzionali:

- **Polinomio** (`-p`): `offset coef_n … coef_0` (ordine decrescente dopo l'offset)
- **Stima** (`-e`): byte medi al secondo da un punto dopo l'inizio dei dati

Usa `-q` per sopprimere la serie e stampare solo le righe aggiuntive.

La fine del trasferimento avviene al completamento del download, alla scadenza di `-w` / `--window`,
al raggiungimento di `-s` / `--size-max` o in caso di errore della richiesta.

In modalità intervallo, i bucket di zeri finali dopo EOF vengono eliminati (con una breve
finestra di tolleranza di 0,1 s per conservare l'ultima fetta parziale).

## Opzioni

| Opzione | Descrizione |
|---------|-------------|
| `-b`, `--block-size=NUM[kmgKMG]` | Dimensione del blocco per la modalità blocco |
| `-i`, `--interval=NUM[um][s]` | Lunghezza della fetta per la modalità intervallo; `NUM` può essere frazionario (`0.1s`, `100.5ms`) |
| `-s`, `--size-max=NUM[kmgKMG]` | Interrompi dopo aver ricevuto `NUM` byte |
| `-w`, `--window=NUM[um][s]` | Interrompi dopo un limite di tempo |
| `-p`, `--polynomial=ORDER` | Adatta un polinomio (ordine 0–7) e stampa offset + coefficienti |
| `-g`, `--gnuplot=FILE` | Scrive un grafico o uno script gnuplot (vedi sotto) |
| `-f`, `--force` | Sovrascrive un file di output gnuplot esistente |
| `-e`, `--estimate-bps=NUM[um][s]` | Stampa B/s stimati misurati da `NUM` dopo l'inizio dei dati |
| `-v`, `--verbose` | Registra URL e conteggio byte su stderr |
| `-q`, `--quiet` | Omette la serie a barre su stdout |
| `-h`, `--help` | Aiuto |
| `--version` | Versione |

### Suffissi di dimensione (`-b`, `-s`)

`k`/`m`/`g`/`t` (base 1024) o `K`/`M`/`G`/`T` (base 1000), ad es. `64k`, `10M`.

### Suffissi di tempo (`-i`, `-w`, `-e`)

| Suffisso | Significato |
|----------|-------------|
| (nessuno) | secondi |
| `ms` o `m` | millisecondi |
| `us` o `u` | microsecondi |
| `s` | secondi (esplicito) |

I numeri senza suffisso sono in secondi per impostazione predefinita. Esempi: `100ms`, `0.1s`, `0.1`, `500us`.

### Opzioni brevi

Le opzioni raggruppate sono supportate quando il valore è attaccato solo all'**ultima** lettera:

```bash
getbar -vfw3 -i100ms https://example.com/file
# equivalente a: -v -f -w3 -i100ms
```

Non attaccare due valori di opzione in un unico gruppo (usa opzioni separate).

## Adattamento polinomiale (`-p`)

L'**offset** è il periodo di inattività prima dei dati utili:

- **Modalità intervallo**: ritardo in secondi (fette iniziali di zeri × intervallo).
- **Modalità blocco**: conteggio dei blocchi a durata zero dopo la barra TTFB.

I coefficienti descrivono un adattamento polinomiale sulla parte non inattiva della serie.
Combinato con `-g`, la curva viene disegnata sul grafico.

## Output Gnuplot (`-g`)

| Estensione | Comportamento |
|------------|---------------|
| `.png`, `.svg`, `.pdf`, `.eps` | Renderizzato con `gnuplot` (deve essere installato) |
| altro | Solo script gnuplot (`gnuplot` non richiesto) |

I grafici usano diagrammi a barre per la serie. I bucket inattivi **prima** dell'offset
polinomiale sono omessi dal plot, così il grafico si concentra sul trasferimento attivo.
Con `-p`, il polinomio adattato è sovrapposto come linea.

Esempio (modalità intervallo con sovrapposizione polinomiale):

![grafico del throughput getbar](images/chart.png)

Tema opzionale: copia
`share/getbar/gnuplot.rc.example` in
`$XDG_CONFIG_HOME/getbar/gnuplot.rc` (o `~/.config/getbar/gnuplot.rc`).

## Esempi

### Misurazioni di base

Tempi per blocco con blocchi da 64 KiB:

```bash
getbar -b 64k https://example.com/file
```

Fette da 100 ms, stima della velocità dopo 1 s di dati, serie nascosta:

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

Leggere il time-to-first-byte (microsecondi) dal primo campo in modalità blocco:

```bash
getbar -b 64k -s 256k https://example.com/file | awk '{print $1}'
```

### Dimensione o durata del download controllata

Campiona 10 MiB e stampa una stima di velocità a regime (utile per i mirror):

```bash
getbar -i 100ms -s 10M -e 2s -q https://mirror.example/file
```

Esegui per al massimo 30 s indipendentemente dalla dimensione del file (test di carico / edge CDN):

```bash
getbar -i 1s -w 30s -v https://cdn.example/large.bin
```

Limita sia tempo che byte — interrompi al primo limite raggiunto:

```bash
getbar -b 1M -s 50M -w 60s https://example.com/file
```

### Grafici e analisi

Intervallo frazionario, limite di 3 s, log dettagliato, sovrascrittura forzata del grafico:

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

Grafico a blocchi con sovrapposizione polinomiale quadratica:

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

Grafico a intervalli con polinomio (corrisponde alla figura di esempio sopra):

```bash
getbar -i 100ms -p 2 -g chart.png -f https://example.com/file
```

Esporta SVG per report o wiki:

```bash
getbar -i 200ms -p 2 -g report.svg -f https://example.com/file
```

Salva uno script gnuplot per modificarlo in seguito (`gnuplot` non richiesto a runtime):

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

### Confronto di mirror o URL

Stampa una riga stimata in B/s per URL (campione da 5 MiB ciascuno):

```bash
for url in https://mirror-a.example/file https://mirror-b.example/file; do
  printf '%s\t' "$url"
  getbar -i 100ms -s 5M -e 1s -q "$url"
done
```

Ordina i mirror per throughput stimato:

```bash
while read -r url; do
  bps=$(getbar -i 100ms -s 5M -e 1s -q "$url") || continue
  printf '%s\t%s\n' "$bps" "$url"
done < mirrors.txt | sort -nr
```

### Integrazione shell

Registra i dettagli del trasferimento su stderr mantenendo stdout leggibile da macchina:

```bash
getbar -v -i 100ms -s 10M https://example.com/file > series.txt
```

Salva serie e grafico in un'unica esecuzione:

```bash
getbar -i 100ms -s 20M -g run.png -f https://example.com/file | tee series.txt
```

MiB/s approssimativo dalla riga di stima:

```bash
bps=$(getbar -i 100ms -s 10M -e 1s -q https://example.com/file)
awk -v b="$bps" 'BEGIN { printf "~ %.2f MiB/s\n", b / 1024 / 1024 }'
```

## Dipendenze

- **Richiesto**: libcurl
- **Consigliato**: gnuplot (per output immagine tramite `-g`)
- Vedi `getbar(1)` per la pagina di manuale.

## Vedi anche

- `getbar(1)` — pagina di manuale
- `share/getbar/gnuplot.rc.example` — esempio di tema per i grafici
