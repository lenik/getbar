# getbar

`getbar`는 HTTP/HTTPS GET(libcurl)으로 URL을 다운로드하고 표준 출력에
**막대 시리즈**를 기록합니다. 블록별 수신 시간 또는 시간 슬라이스별 수신 바이트 수입니다.
선택적 다항식 피팅, 처리량 추정, gnuplot 차트로 미러, CDN, 직접 다운로드 속도
벤치마크에 유용합니다.

```bash
getbar [OPTION]... URL
```

**블록 모드**(`-b`) 또는 **간격 모드**(`-i`) 중 하나가 필수입니다.

## 다른 언어

| 언어 | README |
|------|--------|
| English | [README.md](README.md) |
| 简体中文 | [README-zh_CN.md](README-zh_CN.md) |
| 繁體中文 | [README-zh_TW.md](README-zh_TW.md) |
| 日本語 | [README-ja.md](README-ja.md) |
| 한국어 | README-ko.md (이 문서) |
| ไทย | [README-th.md](README-th.md) |
| Tiếng Việt | [README-vi.md](README-vi.md) |
| Français | [README-fr.md](README-fr.md) |
| Deutsch | [README-de.md](README-de.md) |
| Italiano | [README-it.md](README-it.md) |
| Esperanto | [README-eo.md](README-eo.md) |

## 출력

### 블록 모드(`-b`)

값은 **마이크로초**이며, 한 줄에 공백으로 구분되어 출력됩니다.

| 위치 | 의미 |
|------|------|
| 첫 번째 값 | 첫 바이트 도착 전 대기 시간(TTFB) |
| 이후 값 | `-b` 바이트 블록을 수신하는 데 걸린 시간 |

예:

```text
245000 1200 980 1100 1050
```

### 간격 모드(`-i`)

값은 각 시간 슬라이스에서 **수신한 바이트 수**이며, 한 줄에 공백으로 구분되어 출력됩니다.
앞의 `0`은 해당 슬라이스에 아직 데이터가 없음을 의미합니다(연결 또는 서버 지연).

예:

```text
0 0 0 0 65536 131072 65536
```

시리즈 행 뒤에 추가 행이 나타날 수 있습니다:

- **다항식**(`-p`): `offset coef_n … coef_0`(offset 뒤에 고차에서 저차 계수)
- **추정**(`-e`): 데이터 시작 후 특정 시점부터 종료까지의 평균 바이트/초

`-q`로 시리즈를 생략하고 추가 행만 출력할 수 있습니다.

전송 종료 조건: 다운로드 완료, `-w` / `--window` 만료, `-s` / `--size-max` 도달,
또는 요청 실패.

간격 모드에서는 EOF 이후의 후행 0 버킷이 제거됩니다(마지막 불완전 슬라이스를
유지하기 위해 약 0.1 s 유예 기간이 있습니다).

## 옵션

| 옵션 | 설명 |
|------|------|
| `-b`, `--block-size=NUM[kmgKMG]` | 블록 모드의 블록 크기 |
| `-i`, `--interval=NUM[um][s]` | 간격 모드의 슬라이스 길이; `NUM`은 소수 가능(`0.1s`, `100.5ms` 등) |
| `-s`, `--size-max=NUM[kmgKMG]` | `NUM` 바이트 수신 후 중지 |
| `-w`, `--window=NUM[um][s]` | 시간 제한 후 중지 |
| `-p`, `--polynomial=ORDER` | 다항식 피팅(차수 0–7) 후 offset과 계수 출력 |
| `-g`, `--gnuplot=FILE` | 차트 또는 gnuplot 스크립트 작성(아래 참조) |
| `-f`, `--force` | 기존 gnuplot 출력 파일 덮어쓰기 |
| `-e`, `--estimate-bps=NUM[um][s]` | 데이터 시작 후 `NUM` 시점부터 바이트/초 추정 출력 |
| `-v`, `--verbose` | stderr에 URL과 바이트 수 로그 |
| `-q`, `--quiet` | stdout에 막대 시리즈 출력 안 함 |
| `-h`, `--help` | 도움말 |
| `--version` | 버전 |

### 크기 접미사(`-b`, `-s`)

`k`/`m`/`g`/`t`(1024 기준) 또는 `K`/`M`/`G`/`T`(1000 기준). 예: `64k`, `10M`.

### 시간 접미사(`-i`, `-w`, `-e`)

| 접미사 | 의미 |
|--------|------|
| (없음) | 초 |
| `ms` 또는 `m` | 밀리초 |
| `us` 또는 `u` | 마이크로초 |
| `s` | 초(명시) |

접미사 없는 숫자는 초가 기본값입니다. 예: `100ms`, `0.1s`, `0.1`, `500us`.

### 짧은 옵션

여러 짧은 옵션을 묶어 쓸 수 있으며, **값은 마지막 글자에만** 붙입니다:

```bash
getbar -vfw3 -i100ms https://example.com/file
# 다음과 동일: -v -f -w3 -i100ms
```

인수가 있는 두 옵션을 한 클러스터에 붙이지 마세요(별도로 지정).

## 다항식 피팅(`-p`)

**offset**은 유효 데이터 시작 전 유휴 구간입니다:

- **간격 모드**: 유휴 시간(초). 선행 0 슬라이스 수 × 간격.
- **블록 모드**: TTFB 막대 이후 지속 시간이 0인 블록 수.

계수는 시리즈의 비유휴 부분에 대한 다항식 피팅을 나타냅니다.
`-g`와 함께 사용하면 곡선이 차트에 그려집니다.

## Gnuplot 출력(`-g`)

| 확장자 | 동작 |
|--------|------|
| `.png`, `.svg`, `.pdf`, `.eps` | `gnuplot`으로 렌더링(설치 필요) |
| 그 외 | gnuplot 스크립트만(실행 시 `gnuplot` 불필요) |

차트는 시리즈를 막대 그래프로 표시합니다. 다항식 **offset** 이전의 유휴 버킷은
플롯에서 제외되어 활성 전송 구간에 초점을 맞춥니다.
`-p` 지정 시 피팅된 다항식이 선으로 겹쳐집니다.

예(간격 모드 + 다항식 오버레이):

![getbar throughput chart](images/chart.png)

선택적 테마: `share/getbar/gnuplot.rc.example`을
`$XDG_CONFIG_HOME/getbar/gnuplot.rc`
(또는 `~/.config/getbar/gnuplot.rc`)에 복사합니다.

## 예제

### 기본 측정

64 KiB 블록별 타이밍:

```bash
getbar -b 64k https://example.com/file
```

100 ms 슬라이스, 데이터 시작 1 s 후 속도 추정, 시리즈 숨김:

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

블록 모드 출력의 첫 필드에서 time-to-first-byte(마이크로초) 읽기:

```bash
getbar -b 64k -s 256k https://example.com/file | awk '{print $1}'
```

### 다운로드 크기 또는 시간 제어

10 MiB만 샘플링하고 정상 상태 속도 추정 출력(미러 측정에 유용):

```bash
getbar -i 100ms -s 10M -e 2s -q https://mirror.example/file
```

파일 크기와 관계없이 최대 30 s 실행(소크 테스트 / CDN 엣지):

```bash
getbar -i 1s -w 30s -v https://cdn.example/large.bin
```

바이트와 시간 모두 제한 — 먼저 도달하는 한도에서 중지:

```bash
getbar -b 1M -s 50M -w 60s https://example.com/file
```

### 차트와 분석

소수 간격, 3초 상한, 상세 로그, 차트 강제 덮어쓰기:

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

블록 차트에 2차 다항식 오버레이:

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

간격 차트에 다항식(위 샘플 그림과 동일):

```bash
getbar -i 100ms -p 2 -g chart.png -f https://example.com/file
```

보고서나 위키용 SVG보내기:

```bash
getbar -i 200ms -p 2 -g report.svg -f https://example.com/file
```

나중에 편집할 gnuplot 스크립트 저장(실행 시 `gnuplot` 불필요):

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

### 미러 또는 URL 비교

URL마다 5 MiB를 가져와 추정 B/s를 한 줄씩 출력:

```bash
for url in https://mirror-a.example/file https://mirror-b.example/file; do
  printf '%s\t' "$url"
  getbar -i 100ms -s 5M -e 1s -q "$url"
done
```

추정 처리량으로 미러 정렬:

```bash
while read -r url; do
  bps=$(getbar -i 100ms -s 5M -e 1s -q "$url") || continue
  printf '%s\t%s\n' "$bps" "$url"
done < mirrors.txt | sort -nr
```

### 셸 통합

stderr에 상세 로그를 출력하고 stdout은 기계 가독성 유지:

```bash
getbar -v -i 100ms -s 10M https://example.com/file > series.txt
```

한 번의 실행으로 시리즈와 차트 저장:

```bash
getbar -i 100ms -s 20M -g run.png -f https://example.com/file | tee series.txt
```

추정 행에서 대략적인 MiB/s 계산:

```bash
bps=$(getbar -i 100ms -s 10M -e 1s -q https://example.com/file)
awk -v b="$bps" 'BEGIN { printf "~ %.2f MiB/s\n", b / 1024 / 1024 }'
```

## 의존성

- **필수**: libcurl
- **권장**: gnuplot(`-g`로 이미지 출력 시)
- 자세한 내용은 `getbar(1)` 매뉴얼 페이지를 참조하세요.

## 참고

- `getbar(1)` — 매뉴얼 페이지
- `share/getbar/gnuplot.rc.example` — 차트 테마 예제
