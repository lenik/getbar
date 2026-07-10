# getbar

`getbar` tải URL bằng HTTP hoặc HTTPS GET (qua libcurl) và ghi lại một
**chuỗi cột** trên đầu ra chuẩn: thời gian nhận theo từng khối hoặc số byte
theo từng khoảng thời gian. Tùy chọn fit đa thức, ước tính throughput và biểu
đồ gnuplot giúp đo tốc độ mirror, CDN và tải trực tiếp.

```bash
getbar [OPTION]... URL
```

Phải chỉ định **chế độ khối** (`-b`) hoặc **chế độ khoảng thời gian** (`-i`).

## Ngôn ngữ khác

| Ngôn ngữ | README |
|----------|--------|
| English | [README.md](README.md) |
| 简体中文 | [README-zh_CN.md](README-zh_CN.md) |
| 繁體中文 | [README-zh_TW.md](README-zh_TW.md) |
| 日本語 | [README-ja.md](README-ja.md) |
| 한국어 | [README-ko.md](README-ko.md) |
| ไทย | [README-th.md](README-th.md) |
| Tiếng Việt | README-vi.md (tài liệu này) |
| Français | [README-fr.md](README-fr.md) |
| Deutsch | [README-de.md](README-de.md) |
| Italiano | [README-it.md](README-it.md) |
| Esperanto | [README-eo.md](README-eo.md) |

## Đầu ra

### Chế độ khối (`-b`)

Giá trị là thời gian tính bằng **micro giây**, cách nhau bởi khoảng trắng trên một dòng.

| Vị trí | Ý nghĩa |
|--------|---------|
| Giá trị đầu | Thời gian chờ trước khi byte đầu đến (TTFB) |
| Các giá trị sau | Thời gian nhận mỗi khối `-b` byte |

Ví dụ:

```text
245000 1200 980 1100 1050
```

### Chế độ khoảng thời gian (`-i`)

Giá trị là **byte đã nhận** trong mỗi khoảng thời gian, cách nhau bởi khoảng trắng trên một dòng.
Số 0 đầu nghĩa là chưa có dữ liệu (trễ kết nối hoặc từ máy chủ).

Ví dụ:

```text
0 0 0 0 65536 131072 65536
```

Sau dòng chuỗi có thể có thêm các dòng phụ:

- **Đa thức** (`-p`): `offset coef_n … coef_0` (sau offset là hệ số từ bậc cao đến bậc thấp)
- **Ước tính** (`-e`): byte/giây trung bình từ một điểm sau khi dữ liệu bắt đầu

Dùng `-q` để bỏ qua chuỗi cột và chỉ in các dòng phụ.

Kết thúc truyền khi tải xong, hết `-w` / `--window`, đạt `-s` / `--size-max`,
hoặc yêu cầu thất bại.

Trong chế độ khoảng thời gian, các bucket zero cuối sau EOF bị loại bỏ (có cửa sổ
cho phép 0.1 s ngắn để giữ khoảng thời gian cuối chưa đầy đủ).

## Tùy chọn

| Tùy chọn | Mô tả |
|----------|-------|
| `-b`, `--block-size=NUM[kmgKMG]` | Kích thước khối cho chế độ khối |
| `-i`, `--interval=NUM[um][s]` | Độ dài khoảng thời gian; `NUM` có thể là số thập phân (`0.1s`, `100.5ms`) |
| `-s`, `--size-max=NUM[kmgKMG]` | Dừng sau khi nhận `NUM` byte |
| `-w`, `--window=NUM[um][s]` | Dừng khi hết giới hạn thời gian |
| `-p`, `--polynomial=ORDER` | Fit đa thức (bậc 0–7) và in offset + hệ số |
| `-g`, `--gnuplot=FILE` | Ghi biểu đồ hoặc script gnuplot (xem bên dưới) |
| `-f`, `--force` | Ghi đè tệp đầu ra gnuplot đã tồn tại |
| `-e`, `--estimate-bps=NUM[um][s]` | In B/s ước tính từ `NUM` sau khi dữ liệu bắt đầu |
| `-v`, `--verbose` | Ghi URL và số byte ra stderr |
| `-q`, `--quiet` | Bỏ qua chuỗi cột trên stdout |
| `-h`, `--help` | Trợ giúp |
| `--version` | Phiên bản |

### Hậu tố kích thước (`-b`, `-s`)

`k`/`m`/`g`/`t` (cơ số 1024) hoặc `K`/`M`/`G`/`T` (cơ số 1000), ví dụ `64k`, `10M`.

### Hậu tố thời gian (`-i`, `-w`, `-e`)

| Hậu tố | Ý nghĩa |
|--------|---------|
| (không) | giây |
| `ms` hoặc `m` | milli giây |
| `us` hoặc `u` | micro giây |
| `s` | giây (rõ ràng) |

Số không hậu tố mặc định là giây. Ví dụ: `100ms`, `0.1s`, `0.1`, `500us`.

### Tùy chọn ngắn

Hỗ trợ gộp các cờ ngắn khi giá trị dán vào **chữ cái cuối cùng**:

```bash
getbar -vfw3 -i100ms https://example.com/file
# tương đương: -v -f -w3 -i100ms
```

Không dán hai giá trị tùy chọn trong một cụm (dùng các tùy chọn riêng).

## Fit đa thức (`-p`)

**offset** là khoảng chờ trước khi có dữ liệu hữu ích:

- **Chế độ khoảng thời gian**: độ trễ tính bằng giây (số khoảng zero đầu × khoảng thời gian).
- **Chế độ khối**: số khối có thời gian bằng zero sau cột TTFB.

Hệ số mô tả fit đa thức trên phần không chờ của chuỗi.
Khi kết hợp với `-g`, đường cong được vẽ trên biểu đồ.

## Đầu ra Gnuplot (`-g`)

| Phần mở rộng | Hành vi |
|--------------|---------|
| `.png`, `.svg`, `.pdf`, `.eps` | Render bằng `gnuplot` (phải cài đặt) |
| khác | Chỉ script gnuplot (không cần `gnuplot`) |

Biểu đồ dùng box plot cho chuỗi. Các bucket chờ **trước** offset
đa thức bị bỏ khỏi đồ để tập trung vào giai đoạn truyền hoạt động.
Với `-p`, đường đa thức fit được chồng lên.

Ví dụ (chế độ khoảng thời gian có đường đa thức):

![getbar throughput chart](images/chart.png)\

Tùy chọn giao diện: sao chép
`share/getbar/gnuplot.rc.example` vào
`$XDG_CONFIG_HOME/getbar/gnuplot.rc` (hoặc `~/.config/getbar/gnuplot.rc`).

## Ví dụ

### Đo lường cơ bản

Thời gian theo từng khối với khối 64 KiB:

```bash
getbar -b 64k https://example.com/file
```

Khoảng 100 ms, ước tính tốc độ sau 1 s dữ liệu, ẩn chuỗi cột:

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

Đọc time-to-first-byte (micro giây) từ trường đầu trong chế độ khối:

```bash
getbar -b 64k -s 256k https://example.com/file | awk '{print $1}'
```

### Giới hạn kích thước hoặc thời gian tải

Lấy mẫu 10 MiB và in ước tính tốc độ ổn định (hữu ích cho mirror):

```bash
getbar -i 100ms -s 10M -e 2s -q https://mirror.example/file
```

Chạy tối đa 30 s bất kể kích thước tệp (soak / thử CDN edge):

```bash
getbar -i 1s -w 30s -v https://cdn.example/large.bin
```

Giới hạn cả thời gian và byte — dừng ở giới hạn nào đến trước:

```bash
getbar -b 1M -s 50M -w 60s https://example.com/file
```

### Biểu đồ và phân tích

Khoảng thập phân, giới hạn 3 s, log chi tiết, ghi đè biểu đồ:

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

Biểu đồ khối với đường đa thức bậc hai chồng lên:

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

Biểu đồ khoảng thời gian có đa thức (giống hình mẫu ở trên):

```bash
getbar -i 100ms -p 2 -g chart.png -f https://example.com/file
```

Xuất SVG cho báo cáo hoặc wiki:

```bash
getbar -i 200ms -p 2 -g report.svg -f https://example.com/file
```

Lưu script gnuplot để chỉnh sửa sau (không cần `gnuplot` khi chạy):

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

### So sánh mirror hoặc URL

In một dòng B/s ước tính mỗi URL (lấy mẫu 5 MiB mỗi URL):

```bash
for url in https://mirror-a.example/file https://mirror-b.example/file; do
  printf '%s\t' "$url"
  getbar -i 100ms -s 5M -e 1s -q "$url"
done
```

Sắp xếp mirror theo throughput ước tính:

```bash
while read -r url; do
  bps=$(getbar -i 100ms -s 5M -e 1s -q "$url") || continue
  printf '%s\t%s\n' "$bps" "$url"
done < mirrors.txt | sort -nr
```

### Tích hợp shell

Ghi chi tiết truyền trên stderr, giữ stdout để máy đọc:

```bash
getbar -v -i 100ms -s 10M https://example.com/file > series.txt
```

Lưu chuỗi và biểu đồ trong một lần chạy:

```bash
getbar -i 100ms -s 20M -g run.png -f https://example.com/file | tee series.txt
```

Ước lượng MiB/s từ dòng ước tính:

```bash
bps=$(getbar -i 100ms -s 10M -e 1s -q https://example.com/file)
awk -v b="$bps" 'BEGIN { printf "~ %.2f MiB/s\n", b / 1024 / 1024 }'
```

## Phụ thuộc

- **Bắt buộc**: libcurl
- **Khuyến nghị**: gnuplot (cho đầu ra ảnh qua `-g`)
- Xem `getbar(1)` cho trang hướng dẫn.

## Xem thêm

- `getbar(1)` — trang hướng dẫn
- `share/getbar/gnuplot.rc.example` — ví dụ giao diện biểu đồ
