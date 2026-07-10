# getbar

`getbar` downloads a URL with HTTP or HTTPS GET (via libcurl) and records a
**bar series** on standard output: either per-block receive times or bytes per
time slice. Optional polynomial fitting, throughput estimates, and gnuplot
charts make it useful for benchmarking mirrors, CDNs, and direct downloads.

```bash
getbar [OPTION]... URL
```

One of **block mode** (`-b`) or **interval mode** (`-i`) is required.

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

Use `-q` to suppress the series and print only the extras.

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
| `-w`, `--window=NUM[um][s]` | Stop after a time limit |
| `-p`, `--polynomial=ORDER` | Fit a polynomial (order 0–7) and print offset + coefficients |
| `-g`, `--gnuplot=FILE` | Write a chart or gnuplot script (see below) |
| `-f`, `--force` | Overwrite an existing gnuplot output file |
| `-e`, `--estimate-bps=NUM[um][s]` | Print estimated B/s measured from `NUM` after data starts |
| `-v`, `--verbose` | Log URL and byte count on stderr |
| `-q`, `--quiet` | Omit the bar series on stdout |
| `-h`, `--help` | Help |
| `--version` | Version |

### Size suffixes (`-b`, `-s`)

`k`/`m`/`g`/`t` (1024-based) or `K`/`M`/`G`/`T` (1000-based), e.g. `64k`, `10M`.

### Time suffixes (`-i`, `-w`, `-e`)

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

- **Interval mode**: delay in seconds (leading zero slices × interval).
- **Block mode**: count of zero-duration blocks after the TTFB bar.

Coefficients describe a polynomial fit over the non-idle part of the series.
When combined with `-g`, the curve is drawn on the chart.

## Gnuplot output (`-g`)

| Extension | Behavior |
|-----------|----------|
| `.png`, `.svg`, `.pdf`, `.eps` | Rendered with `gnuplot` (must be installed) |
| anything else | Gnuplot script only (no `gnuplot` required) |

Charts use box plots for the series. Idle buckets **before** the polynomial
offset are omitted from the plot so the graph focuses on the active transfer.
With `-p`, the fitted polynomial is overlaid as a line.

Optional theming: copy
`share/getbar/gnuplot.rc.example` to
`$XDG_CONFIG_HOME/getbar/gnuplot.rc` (or `~/.config/getbar/gnuplot.rc`).

## Examples

Per-block timings with 64 KiB blocks:

```bash
getbar -b 64k https://example.com/file
```

100 ms slices, estimate speed after 1 s of data, series hidden:

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

Fractional interval, 3 s cap, verbose + force overwrite chart:

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

Block chart with quadratic overlay:

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

Save a script for later editing:

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

## Dependencies

- **Required**: libcurl
- **Recommended**: gnuplot (for image output via `-g`)
- See `getbar(1)` for the manual page.

## See also

- `getbar(1)` — manual page
- `share/getbar/gnuplot.rc.example` — chart theming example
