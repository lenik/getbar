# getbar

`getbar` télécharge une URL via HTTP ou HTTPS GET (via libcurl) et enregistre une
**série de barres** sur la sortie standard : soit les temps de réception par bloc,
soit les octets par tranche de temps. L'ajustement polynomial optionnel, les
estimations de débit et les graphiques gnuplot en font un outil utile pour
benchmarker des miroirs, des CDN et des téléchargements directs.

```bash
getbar [OPTION]... URL
```

L'un des modes **bloc** (`-b`) ou **intervalle** (`-i`) est requis.

## Traductions

| Langue | README |
|--------|--------|
| English | [README.md](README.md) |
| 简体中文 | [README-zh_CN.md](README-zh_CN.md) |
| 繁體中文 | [README-zh_TW.md](README-zh_TW.md) |
| 日本語 | [README-ja.md](README-ja.md) |
| 한국어 | [README-ko.md](README-ko.md) |
| ไทย | [README-th.md](README-th.md) |
| Tiếng Việt | [README-vi.md](README-vi.md) |
| Français | README-fr.md (ce fichier) |
| Deutsch | [README-de.md](README-de.md) |
| Italiano | [README-it.md](README-it.md) |
| Esperanto | [README-eo.md](README-eo.md) |

## Sortie

### Mode bloc (`-b`)

Les valeurs sont des durées en **microsecondes**, séparées par des espaces sur une ligne.

| Position | Signification |
|----------|---------------|
| Première valeur | Temps d'attente avant l'arrivée du premier octet (TTFB) |
| Valeurs suivantes | Temps pour recevoir chaque bloc de `-b` octets |

Exemple :

```text
245000 1200 980 1100 1050
```

### Mode intervalle (`-i`)

Les valeurs sont les **octets reçus** dans chaque tranche de temps, séparés par des espaces sur une ligne.
Les zéros en tête signifient qu'aucune donnée n'est encore arrivée (retard de connexion ou du serveur).

Exemple :

```text
0 0 0 0 65536 131072 65536
```

Après la ligne de série, des lignes supplémentaires optionnelles peuvent apparaître :

- **Polynomial** (`-p`) : `offset coef_n … coef_0` (ordre décroissant après l'offset)
- **Estimation** (`-e`) : octets moyens par seconde à partir d'un point après le début des données

Utilisez `-q` pour supprimer la série et n'imprimer que les lignes supplémentaires.

La fin du transfert survient lorsque le téléchargement se termine, que `-w` / `--window` expire,
que `-s` / `--size-max` est atteint, ou que la requête échoue.

En mode intervalle, les compartiments de zéros en fin de série après l'EOF sont supprimés (avec une courte
fenêtre de grâce de 0,1 s pour conserver la dernière tranche partielle).

## Options

| Option | Description |
|--------|-------------|
| `-b`, `--block-size=NUM[kmgKMG]` | Taille de bloc pour le mode bloc |
| `-i`, `--interval=NUM[um][s]` | Durée de tranche pour le mode intervalle ; `NUM` peut être fractionnaire (`0.1s`, `100.5ms`) |
| `-s`, `--size-max=NUM[kmgKMG]` | Arrêt après réception de `NUM` octets |
| `-w`, `--window=NUM[um][s]` | Arrêt après une limite de temps |
| `-p`, `--polynomial=ORDER` | Ajuste un polynôme (ordre 0–7) et affiche l'offset + les coefficients |
| `-g`, `--gnuplot=FILE` | Écrit un graphique ou un script gnuplot (voir ci-dessous) |
| `-f`, `--force` | Écrase un fichier de sortie gnuplot existant |
| `-e`, `--estimate-bps=NUM[um][s]` | Affiche le débit estimé en o/s mesuré à partir de `NUM` après le début des données |
| `-v`, `--verbose` | Journalise l'URL et le nombre d'octets sur stderr |
| `-q`, `--quiet` | Omet la série de barres sur stdout |
| `-h`, `--help` | Aide |
| `--version` | Version |

### Suffixes de taille (`-b`, `-s`)

`k`/`m`/`g`/`t` (base 1024) ou `K`/`M`/`G`/`T` (base 1000), par ex. `64k`, `10M`.

### Suffixes de temps (`-i`, `-w`, `-e`)

| Suffixe | Signification |
|---------|---------------|
| (aucun) | secondes |
| `ms` ou `m` | millisecondes |
| `us` ou `u` | microsecondes |
| `s` | secondes (explicite) |

Les nombres sans suffixe sont en secondes par défaut. Exemples : `100ms`, `0.1s`, `0.1`, `500us`.

### Options courtes

Les options groupées sont prises en charge lorsque la valeur est collée uniquement à la **dernière** lettre :

```bash
getbar -vfw3 -i100ms https://example.com/file
# équivalent à : -v -f -w3 -i100ms
```

Ne collez pas deux valeurs d'option dans un même groupe (utilisez des options séparées à la place).

## Ajustement polynomial (`-p`)

L'**offset** est la période d'inactivité avant les données utiles :

- **Mode intervalle** : délai en secondes (tranches de zéros en tête × intervalle).
- **Mode bloc** : nombre de blocs de durée nulle après la barre TTFB.

Les coefficients décrivent un ajustement polynomial sur la partie non inactive de la série.
Combiné avec `-g`, la courbe est tracée sur le graphique.

## Sortie Gnuplot (`-g`)

| Extension | Comportement |
|-----------|--------------|
| `.png`, `.svg`, `.pdf`, `.eps` | Rendu avec `gnuplot` (doit être installé) |
| autre | Script gnuplot uniquement (`gnuplot` non requis) |

Les graphiques utilisent des diagrammes en barres pour la série. Les compartiments inactifs **avant** l'offset
polynomial sont omis du tracé afin que le graphique se concentre sur le transfert actif.
Avec `-p`, le polynôme ajusté est superposé sous forme de ligne.

Exemple (mode intervalle avec superposition polynomiale) :

![graphique de débit getbar](images/chart.png)

Thème optionnel : copiez
`share/getbar/gnuplot.rc.example` vers
`$XDG_CONFIG_HOME/getbar/gnuplot.rc` (ou `~/.config/getbar/gnuplot.rc`).

## Exemples

### Mesures de base

Temps par bloc avec des blocs de 64 KiB :

```bash
getbar -b 64k https://example.com/file
```

Tranches de 100 ms, estimation de vitesse après 1 s de données, série masquée :

```bash
getbar -i 100ms -e 1s -q https://example.com/file
```

Lire le temps jusqu'au premier octet (microsecondes) depuis le premier champ en mode bloc :

```bash
getbar -b 64k -s 256k https://example.com/file | awk '{print $1}'
```

### Taille ou durée de téléchargement contrôlée

Échantillonner 10 MiB et afficher une estimation de vitesse en régime établi (utile pour les miroirs) :

```bash
getbar -i 100ms -s 10M -e 2s -q https://mirror.example/file
```

Exécuter au maximum 30 s quelle que soit la taille du fichier (test de charge / bord CDN) :

```bash
getbar -i 1s -w 30s -v https://cdn.example/large.bin
```

Limiter à la fois le temps et les octets — arrêt à la première limite atteinte :

```bash
getbar -b 1M -s 50M -w 60s https://example.com/file
```

### Graphiques et analyse

Intervalle fractionnaire, plafond de 3 s, journal détaillé, écrasement forcé du graphique :

```bash
getbar -vfw3 -i0.1s -g /tmp/getbar.png -f https://example.com/file
```

Graphique en mode bloc avec superposition polynomiale quadratique :

```bash
getbar -b 64k -p 2 -g /tmp/getbar.png https://example.com/file
```

Graphique en mode intervalle avec polynôme (correspond à la figure d'exemple ci-dessus) :

```bash
getbar -i 100ms -p 2 -g chart.png -f https://example.com/file
```

Exporter en SVG pour rapports ou wikis :

```bash
getbar -i 200ms -p 2 -g report.svg -f https://example.com/file
```

Enregistrer un script gnuplot pour édition ultérieure (`gnuplot` non requis à l'exécution) :

```bash
getbar -b 64k -g /tmp/getbar.gp https://example.com/file
gnuplot /tmp/getbar.gp
```

### Comparaison de miroirs ou d'URL

Afficher une ligne estimée en o/s par URL (échantillon de 5 MiB chacune) :

```bash
for url in https://mirror-a.example/file https://mirror-b.example/file; do
  printf '%s\t' "$url"
  getbar -i 100ms -s 5M -e 1s -q "$url"
done
```

Trier les miroirs par débit estimé :

```bash
while read -r url; do
  bps=$(getbar -i 100ms -s 5M -e 1s -q "$url") || continue
  printf '%s\t%s\n' "$bps" "$url"
done < mirrors.txt | sort -nr
```

### Intégration shell

Journaliser les détails du transfert sur stderr tout en gardant stdout lisible par machine :

```bash
getbar -v -i 100ms -s 10M https://example.com/file > series.txt
```

Enregistrer la série et le graphique en une seule exécution :

```bash
getbar -i 100ms -s 20M -g run.png -f https://example.com/file | tee series.txt
```

MiB/s approximatif à partir de la ligne d'estimation :

```bash
bps=$(getbar -i 100ms -s 10M -e 1s -q https://example.com/file)
awk -v b="$bps" 'BEGIN { printf "~ %.2f MiB/s\n", b / 1024 / 1024 }'
```

## Dépendances

- **Requis** : libcurl
- **Recommandé** : gnuplot (pour la sortie image via `-g`)
- Voir `getbar(1)` pour la page de manuel.

## Voir aussi

- `getbar(1)` — page de manuel
- `share/getbar/gnuplot.rc.example` — exemple de thème de graphique
