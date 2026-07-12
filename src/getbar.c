/*
 * Copyright (C) 2026 Lenik <getbar@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * getbar - HTTP download throughput bar series
 */

#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <bas/locale/i18n.h>
#include <bas/log/deflog.h>
#include <bas/proc/env.h>

#include <curl/curl.h>

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

define_logger();

enum { OPT_VERSION = 256 };

#define SERIES_INIT_CAP 64
#define POLY_MAX_ORDER 8
#define CONFIG_PATH_MAX 4096
#define INTERVAL_EOF_SAFE_US 100000ULL

struct poly_result {
    bool requested;
    bool has_fit;
    int order;
    double x_origin;
    double x_step;
    size_t fit_count;
    double offset;
    double coef[POLY_MAX_ORDER];
};

struct series {
    uint64_t *values;
    size_t len;
    size_t cap;
};

struct transfer {
    const char *url;
    uint64_t block_size;
    uint64_t interval_us;
    uint64_t size_max;
    uint64_t window_us;
    int poly_order;
    uint64_t estimate_delay_us;
    bool interval_mode;
    bool quiet;
    bool count;
    bool delay_mode;
    bool verbose;
    bool force;
    char *gnuplot_output;

    struct series series;
    uint64_t start_us;
    uint64_t delay_timeout_us;
    uint64_t data_origin_us;
    uint64_t first_byte_us;
    uint64_t block_start_us;
    uint64_t end_us;
    uint64_t total_bytes;
    uint64_t bytes_in_block;
    uint64_t interval_bytes;
    uint64_t next_interval_us;
    uint64_t last_data_us;
    uint64_t eof_us;
    uint64_t interval_flush_limit_us;
    uint64_t estimate_bytes_base;
    uint64_t estimate_time_us;
    bool estimate_set;
    bool stopped;
    bool interval_closed;
    size_t series_printed_idx;
    CURLcode curl_result;
};

static void die(const char *msg) {
    fprintf(stderr, "%s: %s\n", self_exe(), _(msg));
    exit(1);
}

static void die_errno(const char *msg) {
    fprintf(stderr, "%s: %s: %s\n", self_exe(), _(msg), strerror(errno));
    exit(1);
}

static uint64_t now_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        die_errno("clock_gettime");
    }
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

static int series_init(struct series *s) {
    s->values = malloc(SERIES_INIT_CAP * sizeof(uint64_t));
    if (!s->values) {
        return -1;
    }
    s->len = 0;
    s->cap = SERIES_INIT_CAP;
    return 0;
}

static void series_free(struct series *s) {
    free(s->values);
    s->values = NULL;
    s->len = 0;
    s->cap = 0;
}

static int series_push(struct series *s, uint64_t value) {
    if (s->len >= s->cap) {
        size_t new_cap = s->cap * 2;
        uint64_t *next = realloc(s->values, new_cap * sizeof(uint64_t));
        if (!next) {
            return -1;
        }
        s->values = next;
        s->cap = new_cap;
    }
    s->values[s->len++] = value;
    return 0;
}

static int series_push_live(struct transfer *t, uint64_t value);
static void emit_new_series_values(struct transfer *t);

static int parse_size(const char *text, uint64_t *out) {
    char *end = NULL;
    errno = 0;
    unsigned long long whole = strtoull(text, &end, 10);
    if (errno != 0 || end == text) {
        return -1;
    }

    uint64_t mult = 1;
    if (*end != '\0') {
        switch (*end) {
        case 'k':
            mult = 1024ULL;
            break;
        case 'm':
            mult = 1024ULL * 1024ULL;
            break;
        case 'g':
            mult = 1024ULL * 1024ULL * 1024ULL;
            break;
        case 't':
            mult = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
            break;
        case 'K':
            mult = 1000ULL;
            break;
        case 'M':
            mult = 1000000ULL;
            break;
        case 'G':
            mult = 1000000000ULL;
            break;
        case 'T':
            mult = 1000000000000ULL;
            break;
        default:
            return -1;
        }
        if (end[1] != '\0') {
            return -1;
        }
    }

    if (whole > UINT64_MAX / mult) {
        return -1;
    }
    *out = (uint64_t)whole * mult;
    return 0;
}

static int parse_time_us(const char *text, uint64_t *out) {
    char *end = NULL;
    errno = 0;
    const double value = strtod(text, &end);

    if (errno != 0 || end == text || value < 0.0) {
        return -1;
    }

    uint64_t mult = 1000000ULL;
    if (*end == '\0') {
        /* bare number means seconds */
    } else if (end[0] == 'u' && (end[1] == 's' || end[1] == '\0')) {
        mult = 1;
    } else if ((end[0] == 'm' && end[1] == 's') || (end[0] == 'm' && end[1] == '\0')) {
        mult = 1000ULL;
    } else if (end[0] == 's' && end[1] == '\0') {
        mult = 1000000ULL;
    } else {
        return -1;
    }

    if ((end[0] == 'u' && end[1] == 's' && end[2] != '\0') || (end[0] == 'm' && end[1] == 's' && end[2] != '\0')) {
        return -1;
    }
    if (end[0] == 'u' && end[1] == '\0') {
        mult = 1;
    }

    const double us = value * (double)mult;
    if (us > (double)UINT64_MAX) {
        return -1;
    }
    *out = (uint64_t)(us + 0.5);
    return 0;
}

static uint64_t interval_flush_cap(const struct transfer *t, uint64_t now) {
    if (t->interval_closed && t->interval_flush_limit_us > 0) {
        return t->interval_flush_limit_us;
    }
    if (t->interval_flush_limit_us > 0 && now > t->interval_flush_limit_us) {
        return t->interval_flush_limit_us;
    }
    if (t->last_data_us > 0 && now > t->last_data_us + INTERVAL_EOF_SAFE_US) {
        return t->last_data_us + INTERVAL_EOF_SAFE_US;
    }
    if (t->eof_us > 0 && now > t->eof_us + INTERVAL_EOF_SAFE_US) {
        return t->eof_us + INTERVAL_EOF_SAFE_US;
    }
    return now;
}

static void flush_intervals(struct transfer *t, uint64_t now, bool live_emit) {
    if (t->interval_closed) {
        return;
    }

    now = interval_flush_cap(t, now);
    while (now >= t->next_interval_us) {
        if (live_emit) {
            if (series_push_live(t, t->interval_bytes) != 0) {
                die("out of memory");
            }
        } else if (series_push(&t->series, t->interval_bytes) != 0) {
            die("out of memory");
        }
        t->interval_bytes = 0;
        t->next_interval_us += t->interval_us;
    }
}

static void trim_trailing_zero_intervals(struct transfer *t) {
    if (t->last_data_us == 0) {
        return;
    }
    while (t->series.len > 0 && t->series.values[t->series.len - 1] == 0) {
        t->series.len--;
    }
}

static bool delay_active(const struct transfer *t) {
    return t->delay_mode;
}

static bool data_phase_started(const struct transfer *t) {
    return t->data_origin_us > 0;
}

static void begin_data_phase(struct transfer *t, uint64_t now) {
    if (t->data_origin_us > 0) {
        return;
    }
    t->data_origin_us = now;
    if (t->interval_mode) {
        t->next_interval_us = now + t->interval_us;
        t->interval_bytes = 0;
    } else {
        t->block_start_us = now;
        t->bytes_in_block = 0;
    }
}

static void maybe_force_data_phase(struct transfer *t, uint64_t now) {
    if (!delay_active(t) || t->data_origin_us > 0 || t->delay_timeout_us == 0) {
        return;
    }
    const uint64_t deadline = t->start_us + t->delay_timeout_us;
    if (now >= deadline) {
        begin_data_phase(t, deadline);
    }
}

static uint64_t measure_origin_us(const struct transfer *t) {
    if (delay_active(t) && t->data_origin_us > 0) {
        return t->data_origin_us;
    }
    return t->start_us;
}

static bool window_expired(const struct transfer *t, uint64_t now) {
    if (t->window_us == 0) {
        return false;
    }
    return now - t->start_us >= t->window_us;
}

static void note_estimate(struct transfer *t, uint64_t now) {
    if (t->estimate_delay_us == 0 || t->estimate_set || t->first_byte_us == 0) {
        return;
    }
    if (now < t->first_byte_us + t->estimate_delay_us) {
        return;
    }
    t->estimate_bytes_base = t->total_bytes;
    t->estimate_time_us = now;
    t->estimate_set = true;
}

static int progress_cb(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                       curl_off_t ulnow) {
    (void)dltotal;
    (void)dlnow;
    (void)ultotal;
    (void)ulnow;
    struct transfer *t = userdata;
    const uint64_t now = now_us();

    if (t->stopped) {
        return 1;
    }
    maybe_force_data_phase(t, now);
    if (window_expired(t, now)) {
        t->stopped = true;
        return 1;
    }
    if (t->interval_mode && (!delay_active(t) || data_phase_started(t))) {
        flush_intervals(t, now, true);
    }
    note_estimate(t, now);
    return 0;
}

#if LIBCURL_VERSION_NUM < 0x072000
static int legacy_progress_cb(void *userdata, double dltotal, double dlnow, double ultotal,
                              double ulnow) {
    return progress_cb(userdata, (curl_off_t)dltotal, (curl_off_t)dlnow, (curl_off_t)ultotal,
                       (curl_off_t)ulnow);
}
#endif

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    (void)ptr;
    struct transfer *t = userdata;
    const size_t n = size * nmemb;
    const uint64_t now = now_us();

    if (t->stopped) {
        return 0;
    }
    maybe_force_data_phase(t, now);
    if (window_expired(t, now)) {
        t->stopped = true;
        return 0;
    }
    if (t->size_max > 0 && t->total_bytes + n > t->size_max) {
        t->stopped = true;
        return 0;
    }

    note_estimate(t, now);

    if (t->first_byte_us == 0 && n > 0) {
        t->first_byte_us = now;
        if (t->data_origin_us == 0) {
            begin_data_phase(t, now);
        }
        if (!t->interval_mode && !delay_active(t)) {
            if (series_push_live(t, now - t->start_us) != 0) {
                die("out of memory");
            }
            t->block_start_us = now;
            t->bytes_in_block = 0;
        } else if (!t->interval_mode) {
            t->block_start_us = now;
            t->bytes_in_block = 0;
        }
    }

    if (t->interval_mode) {
        if (!delay_active(t) || data_phase_started(t)) {
            flush_intervals(t, now, true);
        }
        t->interval_bytes += (uint64_t)n;
    } else {
        size_t left = n;
        while (left > 0) {
            const uint64_t need = t->block_size - t->bytes_in_block;
            const size_t chunk = (left < need) ? left : (size_t)need;
            t->bytes_in_block += (uint64_t)chunk;
            left -= chunk;
            if (t->bytes_in_block >= t->block_size) {
                if (series_push_live(t, now - t->block_start_us) != 0) {
                    die("out of memory");
                }
                t->block_start_us = now;
                t->bytes_in_block = 0;
            }
        }
    }

    t->total_bytes += (uint64_t)n;
    if (n > 0) {
        t->last_data_us = now;
    }
    note_estimate(t, now);
    return n;
}

static void finish_interval_series(struct transfer *t) {
    const uint64_t now = now_us();

    t->eof_us = now;
    if (t->last_data_us > 0) {
        t->interval_flush_limit_us = t->last_data_us + INTERVAL_EOF_SAFE_US;
    } else {
        t->interval_flush_limit_us = t->eof_us + INTERVAL_EOF_SAFE_US;
    }

    flush_intervals(t, now, false);

    if (t->interval_bytes > 0 && t->next_interval_us <= t->interval_flush_limit_us) {
        if (series_push(&t->series, t->interval_bytes) != 0) {
            die("out of memory");
        }
        t->interval_bytes = 0;
    }

    trim_trailing_zero_intervals(t);
    t->interval_closed = true;
}

static void finish_transfer(struct transfer *t) {
    t->end_us = now_us();
    maybe_force_data_phase(t, t->end_us);
    if (t->interval_mode) {
        finish_interval_series(t);
    } else if (t->bytes_in_block > 0 && t->block_start_us > 0) {
        if (series_push_live(t, t->end_us - t->block_start_us) != 0) {
            die("out of memory");
        }
    }
}

static size_t count_leading_zeros(const struct series *s, size_t start) {
    size_t n = 0;
    for (size_t i = start; i < s->len; i++) {
        if (s->values[i] != 0) {
            break;
        }
        n++;
    }
    return n;
}

static size_t series_data_start(const struct transfer *t) {
    if (!delay_active(t)) {
        return 0;
    }
    return count_leading_zeros(&t->series, 0);
}

static void emit_new_series_values(struct transfer *t) {
    if (t->quiet) {
        return;
    }

    const size_t start = series_data_start(t);
    if (t->series_printed_idx < start) {
        t->series_printed_idx = start;
    }
    while (t->series_printed_idx < t->series.len) {
        if (t->series_printed_idx > start) {
            putchar(' ');
        }
        printf("%" PRIu64, t->series.values[t->series_printed_idx]);
        fflush(stdout);
        t->series_printed_idx++;
    }
}

static void finish_series_output(const struct transfer *t) {
    if (t->quiet || t->series_printed_idx <= series_data_start(t)) {
        return;
    }
    putchar('\n');
    fflush(stdout);
}

static int series_push_live(struct transfer *t, uint64_t value) {
    if (series_push(&t->series, value) != 0) {
        return -1;
    }
    emit_new_series_values(t);
    return 0;
}

static int solve_linear(int n, double a[], double b[]) {
    for (int i = 0; i < n; i++) {
        int pivot = i;
        for (int r = i + 1; r < n; r++) {
            if (fabs(a[r * n + i]) > fabs(a[pivot * n + i])) {
                pivot = r;
            }
        }
        if (fabs(a[pivot * n + i]) < 1e-12) {
            return -1;
        }
        if (pivot != i) {
            for (int c = 0; c < n; c++) {
                const double tmp = a[i * n + c];
                a[i * n + c] = a[pivot * n + c];
                a[pivot * n + c] = tmp;
            }
            const double tb = b[i];
            b[i] = b[pivot];
            b[pivot] = tb;
        }
        const double div = a[i * n + i];
        for (int c = i; c < n; c++) {
            a[i * n + c] /= div;
        }
        b[i] /= div;
        for (int r = 0; r < n; r++) {
            if (r == i) {
                continue;
            }
            const double factor = a[r * n + i];
            for (int c = i; c < n; c++) {
                a[r * n + c] -= factor * a[i * n + c];
            }
            b[r] -= factor * b[i];
        }
    }
    return 0;
}

static int poly_fit(const double *x, const double *y, size_t n, int order, double *coef) {
    const int m = order + 1;
    double ata[POLY_MAX_ORDER * POLY_MAX_ORDER];
    double aty[POLY_MAX_ORDER];

    if (order < 0 || order > POLY_MAX_ORDER - 1 || n < (size_t)m) {
        return -1;
    }

    memset(ata, 0, sizeof(ata));
    memset(aty, 0, sizeof(aty));

    for (size_t i = 0; i < n; i++) {
        double xp[POLY_MAX_ORDER];
        xp[0] = 1.0;
        for (int j = 1; j < m; j++) {
            xp[j] = xp[j - 1] * x[i];
        }
        for (int r = 0; r < m; r++) {
            aty[r] += xp[r] * y[i];
            for (int c = 0; c < m; c++) {
                ata[r * m + c] += xp[r] * xp[c];
            }
        }
    }

    if (solve_linear(m, ata, aty) != 0) {
        return -1;
    }
    for (int i = 0; i < m; i++) {
        coef[i] = aty[i];
    }
    return 0;
}

static void print_polynomial(const struct transfer *t, const struct poly_result *poly) {
    if (!poly->requested) {
        return;
    }

    if (t->interval_mode || delay_active(t)) {
        printf("%g", poly->offset);
    } else {
        printf("%.0f", poly->offset);
    }

    if (!poly->has_fit) {
        putchar('\n');
        return;
    }

    for (int i = poly->order; i >= 0; i--) {
        printf(" %g", poly->coef[i]);
    }
    putchar('\n');
}

static int compute_poly(const struct transfer *t, struct poly_result *poly) {
    double *xs = NULL;
    double *ys = NULL;
    size_t n = 0;
    size_t start = 0;
    size_t lead = 0;

    memset(poly, 0, sizeof(*poly));
    poly->requested = t->poly_order >= 0;
    if (!poly->requested) {
        return 0;
    }

    poly->order = t->poly_order;
    xs = malloc(t->series.len * sizeof(double));
    ys = malloc(t->series.len * sizeof(double));
    if (!xs || !ys) {
        free(xs);
        free(ys);
        return -1;
    }

    if (t->interval_mode) {
        poly->x_step = (double)t->interval_us / 1000000.0;
        if (delay_active(t)) {
            lead = count_leading_zeros(&t->series, 0);
            start = lead;
            n = t->series.len - start;
            if (t->data_origin_us > t->start_us) {
                poly->offset = (double)(t->data_origin_us - t->start_us) / 1000000.0;
            } else {
                poly->offset = (double)lead * poly->x_step;
            }
            poly->x_origin = 0.0;
            for (size_t i = 0; i < n; i++) {
                xs[i] = (double)i * poly->x_step;
                ys[i] = (double)t->series.values[start + i];
            }
        } else {
            start = 0;
            n = t->series.len;
            poly->offset = 0.0;
            poly->x_origin = 0.0;
            for (size_t i = 0; i < n; i++) {
                xs[i] = (double)i * poly->x_step;
                ys[i] = (double)t->series.values[i];
            }
        }
    } else {
        if (t->series.len < 2) {
            free(xs);
            free(ys);
            return 0;
        }
        if (delay_active(t)) {
            lead = count_leading_zeros(&t->series, 0);
            start = lead;
            n = t->series.len - start;
            if (t->data_origin_us > t->start_us) {
                poly->offset = (double)(t->data_origin_us - t->start_us) / 1000000.0;
            } else {
                poly->offset = (double)lead;
            }
            poly->x_origin = 0.0;
        } else {
            start = 1;
            n = t->series.len - start;
            poly->offset = 0.0;
            poly->x_origin = 0.0;
        }
        poly->x_step = 1.0;
        for (size_t i = 0; i < n; i++) {
            xs[i] = (double)i;
            ys[i] = (double)t->series.values[start + i] / 1000000.0;
        }
    }

    poly->fit_count = n;
    if (n < (size_t)poly->order + 1) {
        free(xs);
        free(ys);
        return 0;
    }

    if (poly_fit(xs, ys, n, poly->order, poly->coef) == 0) {
        poly->has_fit = true;
    }

    free(xs);
    free(ys);
    return 0;
}

static int get_config_dir(char *out, size_t out_len) {
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");

    if (xdg != NULL && xdg[0] != '\0') {
        return snprintf(out, out_len, "%s/getbar", xdg) >= (int)out_len ? -1 : 0;
    }
    if (home != NULL && home[0] != '\0') {
        return snprintf(out, out_len, "%s/.config/getbar", home) >= (int)out_len ? -1 : 0;
    }
    return -1;
}

static bool have_gnuplot(void) {
    return access("/usr/bin/gnuplot", X_OK) == 0 || access("/bin/gnuplot", X_OK) == 0;
}

static void gnuplot_escape(char *out, size_t out_len, const char *in) {
    size_t j = 0;
    for (size_t i = 0; in[i] != '\0' && j + 2 < out_len; i++) {
        if (in[i] == '\'') {
            out[j++] = '\\';
        }
        out[j++] = in[i];
    }
    out[j] = '\0';
}

static bool gnuplot_path_is_image(const char *path) {
    const char *dot = strrchr(path, '.');
    if (dot == NULL || dot[1] == '\0') {
        return false;
    }
    dot++;
    if (strcasecmp(dot, "png") == 0) {
        return true;
    }
    if (strcasecmp(dot, "svg") == 0) {
        return true;
    }
    if (strcasecmp(dot, "pdf") == 0) {
        return true;
    }
    if (strcasecmp(dot, "eps") == 0) {
        return true;
    }
    return false;
}

static const char *gnuplot_terminal(const char *path) {
    const char *dot = strrchr(path, '.');
    if (dot == NULL) {
        return "pngcairo";
    }
    dot++;
    if (strcasecmp(dot, "png") == 0) {
        return "pngcairo";
    }
    if (strcasecmp(dot, "svg") == 0) {
        return "svg";
    }
    if (strcasecmp(dot, "pdf") == 0) {
        return "pdfcairo";
    }
    if (strcasecmp(dot, "eps") == 0) {
        return "epscairo";
    }
    return "pngcairo";
}

static void fprintf_poly_curve(FILE *out, const struct poly_result *poly) {
    const double origin = poly->x_origin;
    fputs("(", out);
    for (int p = poly->order; p >= 0; p--) {
        if (p < poly->order) {
            fputc('+', out);
        }
        fprintf(out, "%g", poly->coef[p]);
        if (p == 0) {
            continue;
        }
        fprintf(out, "*(x-%g)", origin);
        if (p > 1) {
            fprintf(out, "**%d", p);
        }
    }
    fputc(')', out);
}

static void write_gnuplot_script(FILE *gp, const struct transfer *t, const struct poly_result *poly,
                                 bool render_image, const char *image_path) {
    char config_dir[CONFIG_PATH_MAX];
    char rc_path[CONFIG_PATH_MAX];
    char rc_escaped[CONFIG_PATH_MAX * 2];
    char out_escaped[CONFIG_PATH_MAX * 2];

    if (render_image) {
        gnuplot_escape(out_escaped, sizeof(out_escaped), image_path);
        fprintf(gp, "set terminal %s enhanced\n", gnuplot_terminal(image_path));
        fprintf(gp, "set output '%s'\n", out_escaped);
    } else {
        fputs("# getbar gnuplot script\n", gp);
    }

    if (get_config_dir(config_dir, sizeof(config_dir)) == 0) {
        if (snprintf(rc_path, sizeof(rc_path), "%s/gnuplot.rc", config_dir) >= (int)sizeof(rc_path)) {
            rc_path[0] = '\0';
        }
        if (rc_path[0] != '\0' && access(rc_path, R_OK) == 0) {
            gnuplot_escape(rc_escaped, sizeof(rc_escaped), rc_path);
            fprintf(gp, "load '%s'\n", rc_escaped);
        }
    }

    if (t->interval_mode) {
        fputs("set xlabel 'time (s)'\nset ylabel 'bytes'\n", gp);
    } else {
        fputs("set xlabel 'block'\nset ylabel 'duration (s)'\n", gp);
    }
    fputs("set autoscale\n", gp);
    fputs("set key top right\n", gp);
    fputs("set style fill solid 0.55 border rgb 'black'\n", gp);
    fputs("set boxwidth 0.8 relative\n", gp);

    fputs("plot '-' using 1:2 with boxes title 'bars'", gp);
    if (poly->has_fit) {
        fputs(", ", gp);
        fprintf_poly_curve(gp, poly);
        fputs(" with lines linewidth 2 title 'polynomial'", gp);
    }
    fputc('\n', gp);

    if (t->interval_mode) {
        const double step = (double)t->interval_us / 1000000.0;
        const size_t plot_start = series_data_start(t);

        for (size_t j = 0; j + plot_start < t->series.len; j++) {
            const size_t i = plot_start + j;
            const double x = delay_active(t) ? (double)j * step : (double)i * step;

            fprintf(gp, "%g %g\n", x, (double)t->series.values[i]);
        }
    } else {
        const size_t plot_start = series_data_start(t);

        for (size_t j = 0; j + plot_start < t->series.len; j++) {
            const size_t i = plot_start + j;
            const double x = delay_active(t) ? (double)j : (double)i;

            fprintf(gp, "%g %g\n", x, (double)t->series.values[i] / 1000000.0);
        }
    }
    fputs("e\n", gp);
}

static void render_gnuplot(const struct transfer *t, const struct poly_result *poly) {
    const bool render_image = gnuplot_path_is_image(t->gnuplot_output);

    if (!t->force && access(t->gnuplot_output, F_OK) == 0) {
        die("gnuplot output file already exists (use --force to overwrite)");
    }

    if (render_image) {
        FILE *gp = NULL;

        if (!have_gnuplot()) {
            die("gnuplot is required to render image output");
        }

        gp = popen("gnuplot", "w");
        if (gp == NULL) {
            die_errno("gnuplot");
        }

        write_gnuplot_script(gp, t, poly, true, t->gnuplot_output);

        if (pclose(gp) != 0) {
            die("gnuplot failed");
        }

        if (t->verbose) {
            fprintf(stderr, _("%s: wrote chart %s\n"), self_exe(), t->gnuplot_output);
        }
        return;
    }

    FILE *script = fopen(t->gnuplot_output, "w");
    if (script == NULL) {
        die_errno(t->gnuplot_output);
    }

    write_gnuplot_script(script, t, poly, false, NULL);
    if (fclose(script) != 0) {
        die_errno(t->gnuplot_output);
    }

    if (t->verbose) {
        fprintf(stderr, _("%s: wrote gnuplot script %s\n"), self_exe(), t->gnuplot_output);
    }
}

static void print_estimate(const struct transfer *t) {
    if (t->estimate_delay_us == 0) {
        return;
    }
    if (!t->estimate_set || t->end_us <= t->estimate_time_us) {
        printf("0\n");
        return;
    }
    const uint64_t dt = t->end_us - t->estimate_time_us;
    if (dt == 0) {
        printf("0\n");
        return;
    }
    const double bps = (double)(t->total_bytes - t->estimate_bytes_base) * 1000000.0 / (double)dt;
    printf("%.0f\n", bps);
}

static void print_count(const struct transfer *t) {
    const size_t data_start = series_data_start(t);
    const uint64_t origin = measure_origin_us(t);
    const uint64_t duration_us = t->end_us > origin ? t->end_us - origin : 0;
    double bps = 0.0;

    if (duration_us > 0) {
        bps = (double)t->total_bytes * 1000000.0 / (double)duration_us;
    }
    printf("%zu %" PRIu64 " %" PRIu64 " %.0f\n",
           t->series.len - data_start,
           t->total_bytes,
           duration_us,
           bps);
}

static void usage(FILE *out) {
    fputs(_("Usage: getbar [OPTION]... URL\n"
            "Download URL with HTTP(S) GET and record throughput bars.\n"),
          out);
    fputs("\n", out);
    fputs("  -b, --block-size=NUM[kmgKMG]   ", out);
    fputs(_("split by received block size\n"), out);
    fputs("  -i, --interval=NUM[um][s]      ", out);
    fputs(_("split by time interval (NUM may be fractional)\n"), out);
    fputs("  -s, --size-max=NUM[kmgKMG]     ", out);
    fputs(_("stop after receiving NUM bytes\n"), out);
    fputs("  -w, --window=NUM[um][s]        ", out);
    fputs(_("stop after timeout\n"), out);
    fputs("  -p, --polynomial=ORDER         ", out);
    fputs(_("output offset and polynomial coeffs\n"), out);
    fputs("  -g, --gnuplot=FILE             ", out);
    fputs(_("render chart (.png/.svg/.pdf/.eps) or save script\n"), out);
    fputs("  -f, --force                    ", out);
    fputs(_("overwrite gnuplot output file\n"), out);
    fputs("  -e, --estimate-bps=NUM[um][s]  ", out);
    fputs(_("output estimated bytes/sec after delay\n"), out);
    fputs("  -c, --count                    ", out);
    fputs(_("print summary after series: count size duration bps\n"), out);
    fputs("  -d, --delayed=NUM[um][s]       ", out);
    fputs(_("omit pre-data wait; force data phase after timeout (0=infinite)\n"), out);
    fputs("  -v, --verbose                  ", out);
    fputs(_("more logging to stderr\n"), out);
    fputs("  -q, --quiet                    ", out);
    fputs(_("omit the bar series on stdout\n"), out);
    fputs("  -h, --help                     ", out);
    fputs(_("display this help and exit\n"), out);
    fputs("      --version                  ", out);
    fputs(_("output version information and exit\n"), out);
    fputs("\n", out);
    fputs(_("Block mode (-b) prints per-block durations in microseconds. By default\n"
            "the first value is the wait before data arrives. With -d that wait is\n"
            "omitted from the series and derived metrics; -p then reports the delay\n"
            "as offset. Interval mode (-i) includes leading zero intervals by default;\n"
            "with -d they are omitted and -p offset is the idle time in seconds.\n"
            "A -d timeout of 0 waits indefinitely for the first byte; a positive\n"
            "timeout starts the data phase even when no bytes have arrived yet.\n"
            "Polynomial, count, and estimate lines are printed after the series.\n"
            "Optional theming: $XDG_CONFIG_HOME/getbar/gnuplot.rc\n"),
          out);
    fprintf(out, _("Report bugs to: <%s>\n"), PROJECT_EMAIL);
}

static void version(void) {
    printf("getbar %s\n", PROJECT_VERSION);
    printf(_("Copyright (C) %d %s\n"), PROJECT_YEAR, PROJECT_AUTHOR);
    fputs(_("License AGPL-3.0-or-later: <https://www.gnu.org/licenses/agpl-3.0.html>\n"),
          stdout);
    fputs(_("This is free software: you are free to change and redistribute it.\n"), stdout);
    fputs(_("This project opposes AI exploitation and AI hegemony.\n"), stdout);
    fputs(_("This project rejects mindless MIT-style licensing and politically naive "
            "BSD-style licensing.\n"),
          stdout);
    fputs(_("There is NO WARRANTY, to the extent permitted by law.\n"), stdout);
}

#define SHORT_OPTS "b:i:s:w:p:g:fe:cd:vqh"

struct argv_buf {
    char **items;
    int argc;
    int cap;
};

static int argv_buf_push(struct argv_buf *b, char *item) {
    if (b->argc >= b->cap) {
        const int new_cap = b->cap ? b->cap * 2 : 16;
        char **next = realloc(b->items, (size_t)new_cap * sizeof(char *));

        if (!next) {
            free(item);
            return -1;
        }
        b->items = next;
        b->cap = new_cap;
    }
    b->items[b->argc++] = item;
    return 0;
}

static ssize_t last_short_opt_index(const char *cluster) {
    ssize_t last = -1;

    for (size_t i = 0; cluster[i] != '\0'; i++) {
        if (strchr(SHORT_OPTS, cluster[i]) != NULL) {
            last = (ssize_t)i;
        }
    }
    return last;
}

static int count_short_opts(const char *cluster) {
    int count = 0;

    for (size_t i = 0; cluster[i] != '\0'; i++) {
        if (strchr(SHORT_OPTS, cluster[i]) != NULL) {
            count++;
        }
    }
    return count;
}

static bool short_opt_takes_arg(char c) {
    const char *p = strchr(SHORT_OPTS, c);

    return p != NULL && p[1] == ':';
}

static bool needs_glued_split(const char *arg) {
    if (arg[0] != '-' || arg[1] == '\0' || arg[1] == '-') {
        return false;
    }

    const char *cluster = arg + 1;
    const ssize_t last_idx = last_short_opt_index(cluster);

    if (last_idx < 0) {
        return false;
    }

    const char *val = cluster + last_idx + 1;
    const char last_c = cluster[last_idx];

    if (count_short_opts(cluster) < 2 || *val == '\0' || !short_opt_takes_arg(last_c)) {
        return false;
    }

    for (size_t j = 0; j < (size_t)last_idx; j++) {
        if (strchr(SHORT_OPTS, cluster[j]) == NULL) {
            return false;
        }
    }
    return true;
}

/* Split -vf...wVAL into -v -f -wVAL; value may only be glued to the last opt. */
static int expand_glued_short_opts(int *argc, char ***argv) {
    char **old = *argv;
    const int old_argc = *argc;
    bool any = false;

    for (int i = 1; i < old_argc; i++) {
        if (needs_glued_split(old[i])) {
            any = true;
            break;
        }
    }
    if (!any) {
        return 0;
    }

    struct argv_buf out = {NULL, 0, 0};

    if (argv_buf_push(&out, old[0]) != 0) {
        return -1;
    }

    for (int i = 1; i < old_argc; i++) {
        const char *arg = old[i];

        if (!needs_glued_split(arg)) {
            if (argv_buf_push(&out, strdup(arg)) != 0) {
                goto fail;
            }
            continue;
        }

        const char *cluster = arg + 1;
        const ssize_t last_idx = last_short_opt_index(cluster);
        const char *val = cluster + last_idx + 1;
        const char last_c = cluster[last_idx];

        for (size_t j = 0; j < (size_t)last_idx; j++) {
            const char single[3] = {'-', cluster[j], '\0'};

            if (argv_buf_push(&out, strdup(single)) != 0) {
                goto fail;
            }
        }

        char *last = malloc(2 + strlen(val) + 1);
        if (!last) {
            goto fail;
        }
        last[0] = '-';
        last[1] = last_c;
        strcpy(last + 2, val);
        if (argv_buf_push(&out, last) != 0) {
            goto fail;
        }
    }

    *argc = out.argc;
    *argv = out.items;
    return 0;

fail:
    for (int j = 1; j < out.argc; j++) {
        free(out.items[j]);
    }
    free(out.items);
    return -1;
}

static void free_expanded_argv(char **argv, int argc) {
    for (int i = 1; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
}

int main(int argc, char **argv) {
    const char *exe = self_exe();
    init_i18n(LOCALEDIR);

    static const struct option long_opts[] = {
        {"block-size", required_argument, NULL, 'b'},
        {"interval", required_argument, NULL, 'i'},
        {"size-max", required_argument, NULL, 's'},
        {"window", required_argument, NULL, 'w'},
        {"polynomial", required_argument, NULL, 'p'},
        {"gnuplot", required_argument, NULL, 'g'},
        {"force", no_argument, NULL, 'f'},
        {"estimate-bps", required_argument, NULL, 'e'},
        {"count", no_argument, NULL, 'c'},
        {"delayed", required_argument, NULL, 'd'},
        {"verbose", no_argument, NULL, 'v'},
        {"quiet", no_argument, NULL, 'q'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, OPT_VERSION},
        {NULL, 0, NULL, 0},
    };

    struct transfer t;
    memset(&t, 0, sizeof(t));
    t.poly_order = -1;

    if (series_init(&t.series) != 0) {
        die("out of memory");
    }

    char **orig_argv = argv;
    int parsed_argc = argc;

    if (expand_glued_short_opts(&argc, &argv) != 0) {
        die("out of memory");
    }
    parsed_argc = argc;
    optind = 1;
    char **parse_argv = argv;

    for (;;) {
        const int c = getopt_long(argc, argv, SHORT_OPTS, long_opts, NULL);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'b':
            if (parse_size(optarg, &t.block_size) != 0) {
                die("invalid block size");
            }
            t.interval_mode = false;
            break;
        case 'i':
            if (parse_time_us(optarg, &t.interval_us) != 0) {
                die("invalid interval");
            }
            t.interval_mode = true;
            break;
        case 's':
            if (parse_size(optarg, &t.size_max) != 0) {
                die("invalid size max");
            }
            break;
        case 'w':
            if (parse_time_us(optarg, &t.window_us) != 0) {
                die("invalid window");
            }
            break;
        case 'p':
            t.poly_order = atoi(optarg);
            if (t.poly_order < 0 || t.poly_order > POLY_MAX_ORDER - 1) {
                die("invalid polynomial order");
            }
            break;
        case 'g':
            t.gnuplot_output = strdup(optarg);
            if (t.gnuplot_output == NULL) {
                die("out of memory");
            }
            break;
        case 'f':
            t.force = true;
            break;
        case 'e':
            if (parse_time_us(optarg, &t.estimate_delay_us) != 0) {
                die("invalid estimate delay");
            }
            break;
        case 'c':
            t.count = true;
            break;
        case 'd':
            t.delay_mode = true;
            if (parse_time_us(optarg, &t.delay_timeout_us) != 0) {
                die("invalid delay timeout");
            }
            break;
        case 'v':
            t.verbose = true;
            break;
        case 'q':
            t.quiet = true;
            break;
        case 'h':
            usage(stdout);
            series_free(&t.series);
            if (parse_argv != orig_argv) {
                free_expanded_argv(parse_argv, parsed_argc);
            }
            return 0;
        case OPT_VERSION:
            version();
            series_free(&t.series);
            if (parse_argv != orig_argv) {
                free_expanded_argv(parse_argv, parsed_argc);
            }
            return 0;
        default:
            usage(stderr);
            series_free(&t.series);
            if (parse_argv != orig_argv) {
                free_expanded_argv(parse_argv, parsed_argc);
            }
            return 1;
        }
    }

    argc -= optind;
    argv += optind;
    if (argc != 1) {
        usage(stderr);
        series_free(&t.series);
        if (parse_argv != orig_argv) {
            free_expanded_argv(parse_argv, parsed_argc);
        }
        return 1;
    }
    t.url = argv[0];

    if (t.block_size == 0 && t.interval_us == 0) {
        die("one of --block-size or --interval is required");
    }
    if (t.block_size > 0 && t.interval_us > 0) {
        die("use only one of --block-size or --interval");
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        die("curl_global_init failed");
    }

    CURL *easy = curl_easy_init();
    if (!easy) {
        curl_global_cleanup();
        die("curl_easy_init failed");
    }

    t.start_us = now_us();
    t.next_interval_us = t.start_us + t.interval_us;

    curl_easy_setopt(easy, CURLOPT_URL, t.url);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &t);
    curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
#if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(easy, CURLOPT_XFERINFOFUNCTION, progress_cb);
    curl_easy_setopt(easy, CURLOPT_XFERINFODATA, &t);
#else
    curl_easy_setopt(easy, CURLOPT_PROGRESSFUNCTION, legacy_progress_cb);
    curl_easy_setopt(easy, CURLOPT_PROGRESSDATA, &t);
#endif
    curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
    if (t.window_us > 0) {
        const long timeout_ms = (long)((t.window_us + 999) / 1000);
        curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, timeout_ms);
    }

    if (t.verbose) {
        fprintf(stderr, _("%s: GET %s\n"), exe, t.url);
    }

    if (!t.quiet) {
        setvbuf(stdout, NULL, _IONBF, 0);
    }

    t.curl_result = curl_easy_perform(easy);
    finish_transfer(&t);

    if (t.verbose) {
        fprintf(stderr, _("%s: received %" PRIu64 " bytes"), exe, t.total_bytes);
        if (t.curl_result != CURLE_OK) {
            fprintf(stderr, " (%s)", curl_easy_strerror(t.curl_result));
        }
        if (t.stopped) {
            fputs(_(" [stopped early]"), stderr);
        }
        fputc('\n', stderr);
    }

    curl_easy_cleanup(easy);
    curl_global_cleanup();

    emit_new_series_values(&t);
    finish_series_output(&t);
    if (t.count) {
        print_count(&t);
    }

    struct poly_result poly;
    if (compute_poly(&t, &poly) != 0) {
        die("out of memory");
    }
    print_polynomial(&t, &poly);
    print_estimate(&t);
    if (t.gnuplot_output != NULL) {
        render_gnuplot(&t, &poly);
    }

    free(t.gnuplot_output);
    series_free(&t.series);
    if (parse_argv != orig_argv) {
        free_expanded_argv(parse_argv, parsed_argc);
    }
    return (t.curl_result == CURLE_OK || t.curl_result == CURLE_WRITE_ERROR) ? 0 : 1;
}
