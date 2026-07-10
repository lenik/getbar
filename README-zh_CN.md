# getbar

`getbar` 通过 HTTP/HTTPS GET（libcurl）下载指定 URL，并在标准输出记录一条
**柱状序列**：按块统计接收耗时，或按时间片统计接收字节数。可选多项式拟合、
吞吐估算和 gnuplot 图表，适合测试镜像站、CDN 和直链下载速度。

```bash
getbar [选项]... URL
```

必须指定 **块模式**（`-b`）或 **间隔模式**（`-i`）之一。

## 输出格式

### 块模式（`-b`）

数值为 **微秒**，空格分隔，输出在一行。

| 位置 | 含义 |
|------|------|
| 第一个值 | 首字节到达前的等待时间（TTFB） |
| 后续值 | 每接收 `-b` 字节所用时间 |

示例：

```text
245000 1200 980 1100 1050
```

### 间隔模式（`-i`）

数值为每个时间片内 **接收的字节数**，空格分隔，输出在一行。
开头的 `0` 表示该时间片内尚未收到数据（连接或服务器延迟）。

示例：

```text
0 0 0 0 65536 131072 65536
```

序列行之后可能还有附加行：

- **多项式**（`-p`）：`offset coef_n … coef_0`（offset 之后为高次到低次系数）
- **估算**（`-e`）：从数据开始一段时间后到结束的平均字节/秒

使用 `-q` 可只输出附加行，不打印序列。

传输结束条件：下载完成、`-w` / `--window` 超时、达到 `-s` / `--size-max`，
或请求失败。

间隔模式下，EOF 之后的尾部零桶会被丢弃（保留约 0.1 s 宽限期，以免漏掉最后一个
不完整时间片）。

## 选项

| 选项 | 说明 |
|------|------|
| `-b`, `--block-size=NUM[kmgKMG]` | 块模式下的块大小 |
| `-i`, `--interval=NUM[um][s]` | 间隔模式下的时间片长度；`NUM` 可为小数（如 `0.1s`、`100.5ms`） |
| `-s`, `--size-max=NUM[kmgKMG]` | 接收满 `NUM` 字节后停止 |
| `-w`, `--window=NUM[um][s]` | 超时后停止 |
| `-p`, `--polynomial=ORDER` | 拟合多项式（阶数 0–7），输出 offset 与系数 |
| `-g`, `--gnuplot=FILE` | 生成图表或 gnuplot 脚本（见下文） |
| `-f`, `--force` | 覆盖已存在的 gnuplot 输出文件 |
| `-e`, `--estimate-bps=NUM[um][s]` | 从数据开始后 `NUM` 时刻起估算字节/秒 |
| `-v`, `--verbose` | 在 stderr 输出 URL 与字节数 |
| `-q`, `--quiet` | 不在 stdout 输出柱状序列 |
| `-h`, `--help` | 帮助 |
| `--version` | 版本 |

### 大小后缀（`-b`、`-s`）

`k`/`m`/`g`/`t`（1024 进制）或 `K`/`M`/`G`/`T`（1000 进制），例如 `64k`、`10M`。

### 时间后缀（`-i`、`-w`、`-e`）

| 后缀 | 含义 |
|------|------|
| （无） | 秒 |
| `ms` 或 `m` | 毫秒 |
| `us` 或 `u` | 微秒 |
| `s` | 秒（显式） |

不写后缀时默认为秒。示例：`100ms`、`0.1s`、`0.1`、`500us`。

### 短选项组合

支持将多个短选项写在一起，**数值只能粘在最后一位选项上**：

```bash
getbar -vfw3 -i100ms https://example.com/file
# 等价于：-v -f -w3 -i100ms
```

不要把两个带参数的选项粘在同一串里（应分开写）。

## 多项式拟合（`-p`）

**offset** 表示有效数据开始前的空闲段：

- **间隔模式**：空闲时间（秒），即前导零时间片数 × 间隔。
- **块模式**：TTFB 之后、持续时间为零的块数量。

系数是对序列有效部分的多项式拟合。与 `-g` 联用时，曲线会画在图上。

## Gnuplot 输出（`-g`）

| 扩展名 | 行为 |
|--------|------|
| `.png`、`.svg`、`.pdf`、`.eps` | 调用 `gnuplot` 渲染（需已安装） |
| 其他 | 仅保存 gnuplot 脚本（无需 `gnuplot`） |

图表用柱状图表示序列。绘图时会去掉多项式 **offset** 之前的空闲桶，突出有效
传输段。指定 `-p` 时叠加拟合曲线。

可选主题：将 `share/getbar/gnuplot.rc.example` 复制到
`$XDG_CONFIG_HOME/getbar/gnuplot.rc`
（或 `~/.config/getbar/gnuplot.rc`）。

## 示例

64 KiB 分块计时：

```bash
getbar -b 64k https://example.com/file
```

100 ms 时间片，数据开始 1 s 后估算速度，隐藏序列：

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

小数间隔、3 秒上限、详细日志并强制覆盖图表：

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

块模式 + 二次多项式叠加：

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

仅保存脚本，稍后手动渲染：

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

## 依赖

- **必需**：libcurl
- **推荐**：gnuplot（`-g` 输出图片时）
- 详见 `getbar(1)` 手册页。

## 参见

- `getbar(1)` — 手册页
- `share/getbar/gnuplot.rc.example` — 图表主题示例
