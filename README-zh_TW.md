# getbar

`getbar` 透過 HTTP/HTTPS GET（libcurl）下載指定 URL，並在標準輸出記錄一條
**柱狀序列**：按區塊統計接收耗時，或按時間片統計接收位元組數。可選多項式擬合、
吞吐估算和 gnuplot 圖表，適合測試鏡像站、CDN 和直連下載速度。

```bash
getbar [選項]... URL
```

必須指定 **區塊模式**（`-b`）或 **間隔模式**（`-i`）之一。

## 其他語言

| 語言 | README |
|------|--------|
| English | [README.md](README.md) |
| 简体中文 | [README-zh_CN.md](README-zh_CN.md) |
| 繁體中文 | README-zh_TW.md（本文） |
| 日本語 | [README-ja.md](README-ja.md) |
| 한국어 | [README-ko.md](README-ko.md) |
| ไทย | [README-th.md](README-th.md) |
| Tiếng Việt | [README-vi.md](README-vi.md) |
| Français | [README-fr.md](README-fr.md) |
| Deutsch | [README-de.md](README-de.md) |
| Italiano | [README-it.md](README-it.md) |
| Esperanto | [README-eo.md](README-eo.md) |

## 輸出格式

### 區塊模式（`-b`）

數值為 **微秒**，空格分隔，輸出在一行。

| 位置 | 含義 |
|------|------|
| 第一個值 | 首位元組到達前的等待時間（TTFB） |
| 後續值 | 每接收 `-b` 位元組所用時間 |

範例：

```text
245000 1200 980 1100 1050
```

### 間隔模式（`-i`）

數值為每個時間片內 **接收的位元組數**，空格分隔，輸出在一行。
開頭的 `0` 表示該時間片內尚未收到資料（連線或伺服器延遲）。

範例：

```text
0 0 0 0 65536 131072 65536
```

序列行之後可能還有附加行：

- **多項式**（`-p`）：`offset coef_n … coef_0`（offset 之後為高次到低次係數）
- **估算**（`-e`）：從資料開始一段時間後到結束的平均位元組/秒

使用 `-q` 可只輸出附加行，不列印序列。

傳輸結束條件：下載完成、`-w` / `--window` 逾時、達到 `-s` / `--size-max`，
或請求失敗。

間隔模式下，EOF 之後的尾部零桶會被捨棄（保留約 0.1 s 寬限期，以免漏掉最後一個
不完整時間片）。

## 選項

| 選項 | 說明 |
|------|------|
| `-b`, `--block-size=NUM[kmgKMG]` | 區塊模式下的區塊大小 |
| `-i`, `--interval=NUM[um][s]` | 間隔模式下的時間片長度；`NUM` 可為小數（如 `0.1s`、`100.5ms`） |
| `-s`, `--size-max=NUM[kmgKMG]` | 接收滿 `NUM` 位元組後停止 |
| `-w`, `--window=NUM[um][s]` | 逾時後停止 |
| `-p`, `--polynomial=ORDER` | 擬合多項式（階數 0–7），輸出 offset 與係數 |
| `-g`, `--gnuplot=FILE` | 產生圖表或 gnuplot 腳本（見下文） |
| `-f`, `--force` | 覆寫已存在的 gnuplot 輸出檔案 |
| `-e`, `--estimate-bps=NUM[um][s]` | 從資料開始後 `NUM` 時刻起估算位元組/秒 |
| `-v`, `--verbose` | 在 stderr 輸出 URL 與位元組數 |
| `-q`, `--quiet` | 不在 stdout 輸出柱狀序列 |
| `-h`, `--help` | 說明 |
| `--version` | 版本 |

### 大小後綴（`-b`、`-s`）

`k`/`m`/`g`/`t`（1024 進位）或 `K`/`M`/`G`/`T`（1000 進位），例如 `64k`、`10M`。

### 時間後綴（`-i`、`-w`、`-e`）

| 後綴 | 含義 |
|------|------|
| （無） | 秒 |
| `ms` 或 `m` | 毫秒 |
| `us` 或 `u` | 微秒 |
| `s` | 秒（顯式） |

不寫後綴時預設為秒。範例：`100ms`、`0.1s`、`0.1`、`500us`。

### 短選項組合

支援將多個短選項寫在一起，**數值只能黏在最後一位選項上**：

```bash
getbar -vfw3 -i100ms https://example.com/file
# 等價於：-v -f -w3 -i100ms
```

不要把兩個帶參數的選項黏在同一串裡（應分開寫）。

## 多項式擬合（`-p`）

**offset** 表示有效資料開始前的空閒段：

- **間隔模式**：空閒時間（秒），即前導零時間片數 × 間隔。
- **區塊模式**：TTFB 之後、持續時間為零的區塊數量。

係數是對序列有效部分的多項式擬合。與 `-g` 聯用時，曲線會畫在圖上。

## Gnuplot 輸出（`-g`）

| 副檔名 | 行為 |
|--------|------|
| `.png`、`.svg`、`.pdf`、`.eps` | 呼叫 `gnuplot` 渲染（需已安裝） |
| 其他 | 僅儲存 gnuplot 腳本（無需 `gnuplot`） |

圖表用柱狀圖表示序列。繪圖時會去掉多項式 **offset** 之前的空閒桶，突出有效
傳輸段。指定 `-p` 時疊加擬合曲線。

範例（間隔模式 + 多項式擬合）：

![getbar 吞吐量圖表](images/chart.png)

可選主題：將 `share/getbar/gnuplot.rc.example` 複製到
`$XDG_CONFIG_HOME/getbar/gnuplot.rc`
（或 `~/.config/getbar/gnuplot.rc`）。

## 範例

### 基本測量

64 KiB 分塊計時：

```bash
getbar -b 64k https://example.com/file
```

100 ms 時間片，資料開始 1 s 後估算速度，隱藏序列：

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

從區塊模式輸出的第一個欄位讀取首位元組時間 TTFB（微秒）：

```bash
getbar -b 64k -s 256k https://example.com/file | awk '{print $1}'
```

### 限制下載量或時長

只拉取 10 MiB 並輸出穩態速度估算（適合測鏡像）：

```bash
getbar -i 100ms -s 10M -e 2s -q https://mirror.example/file
```

最多執行 30 s，不論檔案多大（浸泡測試 / CDN 邊緣節點）：

```bash
getbar -i 1s -w 30s -v https://cdn.example/large.bin
```

同時限制位元組數和時間，先到先停：

```bash
getbar -b 1M -s 50M -w 60s https://example.com/file
```

### 圖表與分析

小數間隔、3 秒上限、詳細日誌並強制覆寫圖表：

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

區塊模式 + 二次多項式疊加：

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

間隔模式 + 多項式（與上文範例圖一致）：

```bash
getbar -i 100ms -p 2 -g chart.png -f https://example.com/file
```

匯出 SVG 用於報告或文件：

```bash
getbar -i 200ms -p 2 -g report.svg -f https://example.com/file
```

僅儲存腳本，稍後手動渲染（執行時無需 `gnuplot`）：

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

### 對比鏡像或 URL

每個 URL 拉 5 MiB 並列印一行估算 B/s：

```bash
for url in https://mirror-a.example/file https://mirror-b.example/file; do
  printf '%s\t' "$url"
  getbar -i 100ms -s 5M -e 1s -q "$url"
done
```

按估算吞吐排序鏡像清單：

```bash
while read -r url; do
  bps=$(getbar -i 100ms -s 5M -e 1s -q "$url") || continue
  printf '%s\t%s\n' "$bps" "$url"
done < mirrors.txt | sort -nr
```

### Shell 整合

stderr 輸出詳細日誌，stdout 保持機器可讀：

```bash
getbar -v -i 100ms -s 10M https://example.com/file > series.txt
```

一次執行儲存序列並產生圖表：

```bash
getbar -i 100ms -s 20M -g run.png -f https://example.com/file | tee series.txt
```

由估算行粗略換算 MiB/s：

```bash
bps=$(getbar -i 100ms -s 10M -e 1s -q https://example.com/file)
awk -v b="$bps" 'BEGIN { printf "~ %.2f MiB/s\n", b / 1024 / 1024 }'
```

## 相依性

- **必要**：libcurl
- **建議**：gnuplot（`-g` 輸出圖片時）
- 詳見 `getbar(1)` 手冊頁。

## 參見

- `getbar(1)` — 手冊頁
- `share/getbar/gnuplot.rc.example` — 圖表主題範例
