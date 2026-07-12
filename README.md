# getbar

`getbar` downloads a URL with HTTP or HTTPS GET (via libcurl) and records a
**bar series** on standard output: either per-block receive times or bytes per
time slice. Optional polynomial fitting, throughput estimates, and gnuplot
charts make it useful for benchmarking mirrors, CDNs, and direct downloads.

```bash
getbar [OPTION]... URL
```

One of **block mode** (`-b`) or **interval mode** (`-i`) is required.

## Translations

| Language | README |
|----------|--------|
| English | README.md (this file) |
| Chinese (Simplified) | [README-zh_CN.md](README-zh_CN.md) |
| Chinese (Traditional) | [README-zh_TW.md](README-zh_TW.md) |
| Japanese | [README-ja.md](README-ja.md) |
| Korean | [README-ko.md](README-ko.md) |
| Thai | [README-th.md](README-th.md) |
| Vietnamese | [README-vi.md](README-vi.md) |
| French | [README-fr.md](README-fr.md) |
| German | [README-de.md](README-de.md) |
| Italian | [README-it.md](README-it.md) |
| Esperanto | [README-eo.md](README-eo.md) |

## Output

### Block mode (`-b`)

Values are durations in **microseconds**, space-separated on one line.

| Position | Meaning |
|----------|---------|
| First value | Time waited before the first byte arrives (TTFB) |
| Later values | Time to receive each block of `-b` bytes |

Example:

```text
245000 1200 980 1100 1050
```

### Interval mode (`-i`)

Values are **bytes received** in each time slice, space-separated on one line.
Leading zeros mean no data yet (connection or server delay).

Example:

```text
0 0 0 0 65536 131072 65536
```

After the series line, optional extra lines may appear:

- **Polynomial** (`-p`): `offset coef_n … coef_0` (high order first after offset)
- **Estimate** (`-e`): average bytes per second from a point after data starts
- **Count** (`-c`): `series-count size-bytes duration-usec bytes-per-second`

Use `-q` to suppress the series and print only the extras.

The bar series is streamed to stdout as each value is recorded (unbuffered,
flushed immediately). Pipelines and live dashboards see data without waiting
for the download to finish.

By default the full timeline is recorded, including connection and server delay
(TTFB in block mode, leading zero intervals in interval mode). With
**`-d` / `--delayed=TIMEOUT`**, pre-data wait is omitted from the series;
`-p` offset reports the stripped delay in seconds. `TIMEOUT` is required:
`-d0` waits indefinitely for the first byte; a positive value (e.g. `-d5s`)
starts the data phase after that limit even when no bytes have arrived yet.

End of the transfer is when the download finishes, `-w` / `--window` expires,
`-s` / `--size-max` is reached, or the request fails.

In interval mode, trailing zero buckets after EOF are dropped (with a short
0.1 s grace window so the last partial slice is kept).

## Options

| Option | Description |
|--------|-------------|
| `-b`, `--block-size=NUM[kmgKMG]` | Block size for block mode |
| `-i`, `--interval=NUM[um][s]` | Slice length for interval mode; `NUM` may be fractional (`0.1s`, `100.5ms`) |
| `-s`, `--size-max=NUM[kmgKMG]` | Stop after receiving `NUM` bytes |
| `-w`, `--window=NUM[um][s]` | Stop after a time limit from connection start |
| `-p`, `--polynomial=ORDER` | Fit a polynomial (order 0–7) and print offset + coefficients |
| `-g`, `--gnuplot=FILE` | Write a chart or gnuplot script (see below) |
| `-f`, `--force` | Overwrite an existing gnuplot output file |
| `-e`, `--estimate-bps=NUM[um][s]` | Print estimated B/s measured from `NUM` after data starts |
| `-c`, `--count` | Print summary line: bar count, bytes, duration µs, average B/s |
| `-d`, `--delayed=NUM[um][s]` | Omit pre-data wait from series; `-d0` waits forever for first byte |
| `-v`, `--verbose` | Log URL and byte count on stderr |
| `-q`, `--quiet` | Omit the bar series on stdout |
| `-h`, `--help` | Help |
| `--version` | Version |

### Size suffixes (`-b`, `-s`)

`k`/`m`/`g`/`t` (1024-based) or `K`/`M`/`G`/`T` (1000-based), e.g. `64k`, `10M`.

### Time suffixes (`-i`, `-w`, `-e`, `-d`)

| Suffix | Meaning |
|--------|---------|
| (none) | seconds |
| `ms` or `m` | milliseconds |
| `us` or `u` | microseconds |
| `s` | seconds (explicit) |

Bare numbers default to seconds. Examples: `100ms`, `0.1s`, `0.1`, `500us`.

### Short options

Clustered flags are supported when the value is glued to the **last** letter only:

```bash
getbar -vfw3 -i100ms https://example.com/file
# same as: -v -f -w3 -i100ms
```

Do not glue two option values in one cluster (use separate options instead).

## Polynomial fit (`-p`)

The **offset** is the idle period before useful data:

- **Without `-d`**: offset is zero; block mode still records TTFB as the first bar.
- **With `-d`**: offset is the stripped pre-data delay in **seconds** (from
  connection start to the data phase, whether triggered by the first byte or
  by the `-d` timeout).

Coefficients describe a polynomial fit over the non-idle part of the series.
When combined with `-g`, the curve is drawn on the chart.

## Delayed data phase (`-d`)

```bash
getbar -i 100ms -d0 -p 2 https://example.com/file    # wait forever for first byte
getbar -i 100ms -d5s -w 30s https://example.com/file # start data phase after 5 s idle
getbar -b 64k -d0 -s 256k https://example.com/file   # block mode without TTFB bar
```

`-d` requires a timeout value. Use `-d0` (not bare `-d`) to wait indefinitely.
A positive timeout forces the data phase when no bytes have arrived yet, so
interval mode may emit zero buckets and `-p` offset reflects the configured wait.

## Gnuplot output (`-g`)

| Extension | Behavior |
|-----------|----------|
| `.png`, `.svg`, `.pdf`, `.eps` | Rendered with `gnuplot` (must be installed) |
| anything else | Gnuplot script only (no `gnuplot` required) |

Charts use box plots for the series. Idle buckets **before** the polynomial
offset are omitted from the plot so the graph focuses on the active transfer.
With `-p`, the fitted polynomial is overlaid as a line.

Example (interval mode with polynomial overlay):

![getbar throughput chart](images/chart.png)\

Optional theming: copy
`share/getbar/gnuplot.rc.example` to
`$XDG_CONFIG_HOME/getbar/gnuplot.rc` (or `~/.config/getbar/gnuplot.rc`).

## Examples

### Basic measurements

Per-block timings with 64 KiB blocks:

```bash
getbar -b 64k https://example.com/file
```

100 ms slices, estimate speed after 1 s of data, series hidden:

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

Summary line after the series (bar count, bytes, duration, B/s):

```bash
getbar -i 100ms -c https://example.com/file
```

Read time-to-first-byte (microseconds) from the first field in block mode
(without `-d`):

```bash
getbar -b 64k -s 256k https://example.com/file | awk '{print $1}'
```

### Controlled download size or duration

Sample 10 MiB and print a steady-state speed estimate (useful for mirrors):

```bash
getbar -i 100ms -s 10M -e 2s -q https://mirror.example/file
```

Run for at most 30 s regardless of file size (soak / CDN edge test):

```bash
getbar -i 1s -w 30s -v https://cdn.example/large.bin
```

Cap both time and bytes — stop at whichever limit comes first:

```bash
getbar -b 1M -s 50M -w 60s https://example.com/file
```

### Charts and analysis

Fractional interval, 3 s cap, verbose log, force overwrite chart:

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

Block chart with quadratic polynomial overlay:

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

Interval chart with polynomial (matches the sample figure above):

```bash
getbar -i 100ms -p 2 -g chart.png -f https://example.com/file
```

Export SVG for reports or wikis:

```bash
getbar -i 200ms -p 2 -g report.svg -f https://example.com/file
```

Save a gnuplot script for later editing (no `gnuplot` required at run time):

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

### Comparing mirrors or URLs

Print one estimated B/s line per URL (5 MiB sample each):

```bash
for url in https://mirror-a.example/file https://mirror-b.example/file; do
  printf '%s\t' "$url"
  getbar -i 100ms -s 5M -e 1s -q "$url"
done
```

Sort mirrors by estimated throughput:

```bash
while read -r url; do
  bps=$(getbar -i 100ms -s 5M -e 1s -q "$url") || continue
  printf '%s\t%s\n' "$bps" "$url"
done < mirrors.txt | sort -nr
```

### Shell integration

Log verbose transfer details on stderr while keeping stdout machine-readable:

```bash
getbar -v -i 100ms -s 10M https://example.com/file > series.txt
```

Store series and chart in one run:

```bash
getbar -i 100ms -s 20M -g run.png -f https://example.com/file | tee series.txt
```

Rough MiB/s from the estimate line:

```bash
bps=$(getbar -i 100ms -s 10M -e 1s -q https://example.com/file)
awk -v b="$bps" 'BEGIN { printf "~ %.2f MiB/s\n", b / 1024 / 1024 }'
```

## Dependencies

- **Required**: libcurl
- **Recommended**: gnuplot (for image output via `-g`)
- See `getbar(1)` for the manual page.

## See also

- `getbar(1)` — manual page
- `share/getbar/gnuplot.rc.example` — chart theming example
