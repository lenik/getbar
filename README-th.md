# getbar

`getbar` ดาวน์โหลด URL ด้วย HTTP หรือ HTTPS GET (ผ่าน libcurl) และบันทึก
**ชุดแท่ง** บนสแตนดาร์ดเอาต์พุต: ระยะเวลารับต่อบล็อก หรือไบต์ต่อช่วงเวลา
ตามลำดับ การปรับพหุนาม การประมาณ throughput และแผนภูมิ gnuplot
เป็นตัวเลือก จึงเหมาะสำหรับวัดประสิทธิภาพ mirror CDN และการดาวน์โหลดตรง

```bash
getbar [OPTION]... URL
```

ต้องระบุ **โหมดบล็อก** (`-b`) หรือ **โหมดช่วงเวลา** (`-i`) อย่างใดอย่างหนึ่ง

## ภาษาอื่น

| ภาษา | README |
|------|--------|
| English | [README.md](README.md) |
| 简体中文 | [README-zh_CN.md](README-zh_CN.md) |
| 繁體中文 | [README-zh_TW.md](README-zh_TW.md) |
| 日本語 | [README-ja.md](README-ja.md) |
| 한국어 | [README-ko.md](README-ko.md) |
| ไทย | README-th.md (เอกสารนี้) |
| Tiếng Việt | [README-vi.md](README-vi.md) |
| Français | [README-fr.md](README-fr.md) |
| Deutsch | [README-de.md](README-de.md) |
| Italiano | [README-it.md](README-it.md) |
| Esperanto | [README-eo.md](README-eo.md) |

## เอาต์พุต

### โหมดบล็อก (`-b`)

ค่าเป็นระยะเวลาเป็น **ไมโครวินาที** คั่นด้วยช่องว่างในบรรทัดเดียว

| ตำแหน่ง | ความหมาย |
|---------|----------|
| ค่าแรก | เวลารอก่อนไบต์แรกมาถึง (TTFB) |
| ค่าถัดไป | เวลารับแต่ละบล็อกขนาด `-b` ไบต์ |

ตัวอย่าง:

```text
245000 1200 980 1100 1050
```

### โหมดช่วงเวลา (`-i`)

ค่าเป็น **ไบต์ที่รับ** ในแต่ละช่วงเวลา คั่นด้วยช่องว่างในบรรทัดเดียว
ศูนย์นำหน้าหมายถึงยังไม่มีข้อมูล (การเชื่อมต่อหรือความล่าช้าจากเซิร์ฟเวอร์)

ตัวอย่าง:

```text
0 0 0 0 65536 131072 65536
```

หลังบรรทัดชุดข้อมูล อาจมีบรรทัดเพิ่มเติม:

- **พหุนาม** (`-p`): `offset coef_n … coef_0` (หลัง offset เป็นสัมประสิทธิ์จากชั้นสูงไปต่ำ)
- **การประมาณ** (`-e`): ไบต์ต่อวินาทีเฉลี่ยจากจุดหลังข้อมูลเริ่มมา

ใช้ `-q` เพื่อไม่แสดงชุดแท่ง และพิมพ์เฉพาะบรรทัดเพิ่มเติม

การถ่ายโอนจบเมื่อดาวน์โหลดเสร็จ `-w` / `--window` หมดเวลา
ถึง `-s` / `--size-max` หรือคำขอล้มเหลว

ในโหมดช่วงเวลา ถังศูนย์ท้ายหลัง EOF จะถูกตัดทิ้ง (มีช่วงผ่อนผัน 0.1 s
สั้น ๆ เพื่อเก็บช่วงเวลาส่วนที่ยังไม่เต็มครั้งสุดท้าย)

## ตัวเลือก

| ตัวเลือก | คำอธิบาย |
|---------|----------|
| `-b`, `--block-size=NUM[kmgKMG]` | ขนาดบล็อกสำหรับโหมดบล็อก |
| `-i`, `--interval=NUM[um][s]` | ความยาวช่วงเวลาในโหมดช่วงเวลา; `NUM` อาจเป็นทศนิยม (`0.1s`, `100.5ms`) |
| `-s`, `--size-max=NUM[kmgKMG]` | หยุดหลังรับ `NUM` ไบต์ |
| `-w`, `--window=NUM[um][s]` | หยุดเมื่อถึงขีดจำกัดเวลา |
| `-p`, `--polynomial=ORDER` | ปรับพหุนาม (ลำดับ 0–7) และแสดง offset + สัมประสิทธิ์ |
| `-g`, `--gnuplot=FILE` | เขียนแผนภูมิหรือสคริปต์ gnuplot (ดูด้านล่าง) |
| `-f`, `--force` | เขียนทับไฟล์เอาต์พุต gnuplot ที่มีอยู่ |
| `-e`, `--estimate-bps=NUM[um][s]` | แสดง B/s โดยประมาณจาก `NUM` หลังข้อมูลเริ่มมา |
| `-v`, `--verbose` | บันทึก URL และจำนวนไบต์ไป stderr |
| `-q`, `--quiet` | ไม่แสดงชุดแท่งบน stdout |
| `-h`, `--help` | ความช่วยเหลือ |
| `--version` | เวอร์ชัน |

### คำต่อท้ายขนาด (`-b`, `-s`)

`k`/`m`/`g`/`t` (ฐาน 1024) หรือ `K`/`M`/`G`/`T` (ฐาน 1000) เช่น `64k`, `10M`

### คำต่อท้ายเวลา (`-i`, `-w`, `-e`)

| คำต่อท้าย | ความหมาย |
|-----------|----------|
| (ไม่มี) | วินาที |
| `ms` หรือ `m` | มิลลิวินาที |
| `us` หรือ `u` | ไมโครวินาที |
| `s` | วินาที (ระบุชัด) |

ตัวเลขเปล่าเริ่มต้นเป็นวินาที ตัวอย่าง: `100ms`, `0.1s`, `0.1`, `500us`

### ตัวเลือกสั้น

รองรับการรวมตัวเลือกสั้น เมื่อค่าติดกับ **ตัวอักษรตัวสุดท้าย** เท่านั้น:

```bash
getbar -vfw3 -i100ms https://example.com/file
# เท่ากับ: -v -f -w3 -i100ms
```

อย่าติดค่าของสองตัวเลือกในคลัสเตอร์เดียว (ใช้ตัวเลือกแยกแทน)

## การปรับพหุนาม (`-p`)

**offset** คือช่วงว่างก่อนข้อมูลที่มีประโยชน์:

- **โหมดช่วงเวลา**: หน่วงเวลาเป็นวินาที (ช่วงศูนย์นำหน้า × ช่วงเวลา)
- **โหมดบล็อก**: จำนวนบล็อกที่ระยะเวลาเป็นศูนย์หลังแท่ง TTFB

สัมประสิทธิ์อธิบายการปรับพหุนามบนส่วนที่ไม่ว่างของชุดข้อมูล
เมื่อใช้ร่วมกับ `-g` จะวาดเส้นโค้งบนแผนภูมิ

## เอาต์พุต Gnuplot (`-g`)

| นามสกุล | พฤติกรรม |
|---------|----------|
| `.png`, `.svg`, `.pdf`, `.eps` | เรนเดอร์ด้วย `gnuplot` (ต้องติดตั้ง) |
| อื่น ๆ | เฉพาะสคริปต์ gnuplot (ไม่ต้องมี `gnuplot`) |

แผนภูมิใช้ box plot สำหรับชุดข้อมูล ถังว่าง **ก่อน** offset
ของพหุนามจะไม่แสดงในกราฟ เพื่อเน้นช่วงถ่ายโอนที่ใช้งานจริง
เมื่อใช้ `-p` จะซ้อนเส้นพหุนามที่ปรับแล้ว

ตัวอย่าง (โหมดช่วงเวลาพร้อมพหุนามซ้อน):

![getbar throughput chart](images/chart.png)\

ธีมเสริม: คัดลอก
`share/getbar/gnuplot.rc.example` ไปที่
`$XDG_CONFIG_HOME/getbar/gnuplot.rc` (หรือ `~/.config/getbar/gnuplot.rc`)

## ตัวอย่าง

### การวัดพื้นฐาน

วัดเวลาต่อบล็อกด้วยบล็อก 64 KiB:

```bash
getbar -b 64k https://example.com/file
```

ช่วง 100 ms ประมาณความเร็วหลังข้อมูล 1 s ซ่อนชุดแท่ง:

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

อ่าน time-to-first-byte (ไมโครวินาที) จากฟิลด์แรกในโหมดบล็อก:

```bash
getbar -b 64k -s 256k https://example.com/file | awk '{print $1}'
```

### จำกัดขนาดหรือระยะเวลาดาวน์โหลด

สุ่ม 10 MiB และแสดงการประมาณความเร็วสถานะคงที่ (เหมาะกับ mirror):

```bash
getbar -i 100ms -s 10M -e 2s -q https://mirror.example/file
```

รันได้สูงสุด 30 s ไม่ว่าไฟล์จะใหญ่แค่ไหน (ทดสอบ soak / CDN edge):

```bash
getbar -i 1s -w 30s -v https://cdn.example/large.bin
```

จำกัดทั้งเวลาและไบต์ — หยุดที่ขีดจำกัดใดมาก่อน:

```bash
getbar -b 1M -s 50M -w 60s https://example.com/file
```

### แผนภูมิและการวิเคราะห์

ช่วงทศนิยม จำกัด 3 s บันทึกละเอียด บังคับเขียนทับแผนภูมิ:

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

แผนภูมิโหมดบล็อกพร้อมพหุนามกำลังสองซ้อน:

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

แผนภูมิโหมดช่วงเวลาพร้อมพหุนาม (ตรงกับรูปตัวอย่างด้านบน):

```bash
getbar -i 100ms -p 2 -g chart.png -f https://example.com/file
```

ส่งออก SVG สำหรับรายงานหรือ wiki:

```bash
getbar -i 200ms -p 2 -g report.svg -f https://example.com/file
```

บันทึกสคริปต์ gnuplot เพื่อแก้ไขภายหลัง (ไม่ต้องมี `gnuplot` ตอนรัน):

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

### เปรียบเทียบ mirror หรือ URL

พิมพ์บรรทัด B/s โดยประมาณต่อ URL (สุ่ม 5 MiB ต่อ URL):

```bash
for url in https://mirror-a.example/file https://mirror-b.example/file; do
  printf '%s\t' "$url"
  getbar -i 100ms -s 5M -e 1s -q "$url"
done
```

เรียง mirror ตาม throughput โดยประมาณ:

```bash
while read -r url; do
  bps=$(getbar -i 100ms -s 5M -e 1s -q "$url") || continue
  printf '%s\t%s\n' "$bps" "$url"
done < mirrors.txt | sort -nr
```

### การผสานกับ shell

บันทึกรายละเอียดการถ่ายโอนละเอียดไป stderr ขณะ stdout อ่านด้วยเครื่องได้:

```bash
getbar -v -i 100ms -s 10M https://example.com/file > series.txt
```

เก็บชุดข้อมูลและแผนภูมิในครั้งเดียว:

```bash
getbar -i 100ms -s 20M -g run.png -f https://example.com/file | tee series.txt
```

ประมาณ MiB/s จากบรรทัดการประมาณ:

```bash
bps=$(getbar -i 100ms -s 10M -e 1s -q https://example.com/file)
awk -v b="$bps" 'BEGIN { printf "~ %.2f MiB/s\n", b / 1024 / 1024 }'
```

## สิ่งที่ต้องมี

- **จำเป็น**: libcurl
- **แนะนำ**: gnuplot (สำหรับเอาต์พุตรูปภาพผ่าน `-g`)
- ดู `getbar(1)` สำหรับคู่มือ

## ดูเพิ่มเติม

- `getbar(1)` — คู่มือ
- `share/getbar/gnuplot.rc.example` — ตัวอย่างธีมแผนภูมิ
