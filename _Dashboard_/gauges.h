#ifndef GAUGES_H
#define GAUGES_H

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ── Colour helpers ────────────────────────────────────────────────────────── */
typedef struct { double r, g, b; } Color;

static const Color COL_BG        = {0.067, 0.067, 0.067};   /* #111111 */
static const Color COL_BORDER    = {0.800, 0.000, 0.000};   /* #CC0000 */
static const Color COL_TRACK     = {0.165, 0.165, 0.165};   /* #2A2A2A */
static const Color COL_WHITE     = {0.941, 0.941, 0.941};   /* #F0F0F0 */
static const Color COL_GREEN     = {0.000, 1.000, 0.533};   /* #00FF88 */
static const Color COL_AMBER     = {1.000, 0.533, 0.000};   /* #FF8800 */
static const Color COL_RED       = {1.000, 0.133, 0.133};   /* #FF2222 */
static const Color COL_LABEL_RED = {0.800, 0.000, 0.000};   /* #CC0000 */
static const Color COL_DIM_GREY  = {0.533, 0.533, 0.533};   /* #888888 */
static const Color COL_TICK_GREY = {0.267, 0.267, 0.267};   /* #444444 */

#define ARC_START_DEG  225.0
#define ARC_SPAN_DEG   270.0
#define DEG2RAD(x)     ((x) * M_PI / 180.0)

/* ── Gauge data structures ─────────────────────────────────────────────────── */

typedef struct {
    const char *label;
    const char *unit;
    double min_val, max_val;
    double warn_val, crit_val;
    double value;
    int    reverse;          /* 1 = low is bad (LV battery), 0 = high is bad */
    const char *fmt;         /* printf format e.g. "%.1f" */
} TextGaugeData;

typedef struct {
    const char *label;
    const char *unit;
    double min_val, max_val;
    double warn_val, crit_val;
    double value;
} BatteryGaugeData;

typedef struct {
    const char *label;
    const char *unit;
    double min_val, max_val;
    double warn_val, crit_val;
    double value;
    int    use_fixed_color;
    Color  fixed_color;
    int    reverse;
} ArcGaugeData;

typedef struct {
    const char *label;
    const char *unit;
    double min_val, max_val;
    double warn_val, crit_val;
    double value;
    int    reverse;
} HBarGaugeData;

typedef struct {
    const char *label;
    double value;            /* seconds as float e.g. 74.35 */
} LapTimerData;

/* ── Colour selection ──────────────────────────────────────────────────────── */
static inline Color value_color(double val, double warn, double crit, int reverse)
{
    if (reverse) {
        if (val <= crit) return COL_RED;
        if (val <= warn) return COL_AMBER;
        return COL_GREEN;
    } else {
        if (val >= crit) return COL_RED;
        if (val >= warn) return COL_AMBER;
        return COL_GREEN;
    }
}

static inline void set_color(cairo_t *cr, Color c)
{
    cairo_set_source_rgb(cr, c.r, c.g, c.b);
}

/* ── Draw panel background + top red border ────────────────────────────────── */
static void draw_panel_bg(cairo_t *cr, double x, double y, double w, double h)
{
    /* Fill background */
    set_color(cr, COL_BG);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);

    /* Top red accent line */
    set_color(cr, COL_BORDER);
    cairo_set_line_width(cr, 3.0);
    cairo_move_to(cr, x, y + 1.5);
    cairo_line_to(cr, x + w, y + 1.5);
    cairo_stroke(cr);
}

/* ── Cairo text helper (centred in a box) ──────────────────────────────────── */
static void draw_text_centered(cairo_t *cr,
                                double bx, double by, double bw, double bh,
                                const char *text, double font_size,
                                Color color, int bold)
{
    cairo_text_extents_t ext;
    set_color(cr, color);
    cairo_select_font_face(cr, "Monospace",
                           CAIRO_FONT_SLANT_NORMAL,
                           bold ? CAIRO_FONT_WEIGHT_BOLD
                                : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size);
    cairo_text_extents(cr, text, &ext);
    double tx = bx + (bw - ext.width)  / 2.0 - ext.x_bearing;
    double ty = by + (bh + ext.height) / 2.0;
    cairo_move_to(cr, tx, ty);
    cairo_show_text(cr, text);
}

/* ══════════════════════════════════════════════════════════════════════════════
   TEXT GAUGE
   ══════════════════════════════════════════════════════════════════════════════ */
static void draw_text_gauge(cairo_t *cr, double x, double y, double w, double h,
                             TextGaugeData *g)
{
    draw_panel_bg(cr, x, y, w, h);

    /* Label */
    draw_text_centered(cr, x, y + h * 0.06, w, h * 0.18,
                       g->label, fmax(9, h * 0.09), COL_LABEL_RED, 1);

    /* Value */
    char buf[64];
    snprintf(buf, sizeof(buf), g->fmt, g->value);
    Color col = value_color(g->value, g->warn_val, g->crit_val, g->reverse);
    draw_text_centered(cr, x, y + h * 0.18, w, h * 0.50,
                       buf, fmax(24, h * 0.30), col, 1);

    /* Unit */
    draw_text_centered(cr, x, y + h * 0.66, w, h * 0.14,
                       g->unit, fmax(8, h * 0.085), COL_DIM_GREY, 0);
}

/* ══════════════════════════════════════════════════════════════════════════════
   BATTERY GAUGE
   ══════════════════════════════════════════════════════════════════════════════ */
static void draw_battery_gauge(cairo_t *cr, double x, double y, double w, double h,
                                BatteryGaugeData *g)
{
    draw_panel_bg(cr, x, y, w, h);

    Color col = value_color(g->value, g->warn_val, g->crit_val, 1 /* reverse */);

    /* Label */
    draw_text_centered(cr, x, y + h * 0.06, w, h * 0.18,
                       g->label, fmax(9, h * 0.09), COL_LABEL_RED, 1);

    /* Percentage value */
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f%%", g->value);
    draw_text_centered(cr, x, y + h * 0.18, w, h * 0.50,
                       buf, fmax(24, h * 0.30), col, 1);

    /* Battery bar */
    double bm   = w * 0.08;
    double bw   = w - bm * 2;
    double bh   = fmax(14, h * 0.12);
    double bx   = x + bm;
    double by   = y + h * 0.80;
    double tw   = bh * 0.22;
    double th   = bh * 0.50;

    /* Shell outline */
    set_color(cr, COL_WHITE);
    cairo_set_line_width(cr, 2.0);
    cairo_rectangle(cr, bx, by, bw - tw, bh);
    cairo_stroke(cr);

    /* Nub */
    cairo_rectangle(cr, bx + bw - tw, by + (bh - th) / 2.0, tw, th);
    cairo_stroke(cr);

    /* Fill */
    double frac   = (g->value - g->min_val) / (g->max_val - g->min_val);
    double fill_w = fmax(0.0, (bw - tw - 4) * frac);
    set_color(cr, col);
    cairo_rectangle(cr, bx + 2, by + 2, fill_w, bh - 4);
    cairo_fill(cr);
}

/* ══════════════════════════════════════════════════════════════════════════════
   ARC GAUGE  (RPM)
   ══════════════════════════════════════════════════════════════════════════════ */
static void draw_arc_gauge(cairo_t *cr, double x, double y, double w, double h,
                            ArcGaugeData *g)
{
    draw_panel_bg(cr, x, y, w, h);

    double cx     = x + w / 2.0;
    double cy     = y + h / 2.0;
    double radius = fmin(w, h) * 0.40;
    double pen_w  = fmax(8, radius * 0.13);

    double start_rad = DEG2RAD(180.0 - ARC_START_DEG); /* Cairo: 0=right, CCW */
    double end_rad   = start_rad - DEG2RAD(ARC_SPAN_DEG);  /* sweep CCW */

    /* Track arc */
    set_color(cr, COL_TRACK);
    cairo_set_line_width(cr, pen_w);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_arc_negative(cr, cx, cy, radius, start_rad, end_rad);
    cairo_stroke(cr);

    /* Value arc */
    double frac  = (g->value - g->min_val) / (g->max_val - g->min_val);
    frac = fmax(0.0, fmin(1.0, frac));
    Color col = g->use_fixed_color ? g->fixed_color
              : value_color(g->value, g->warn_val, g->crit_val, g->reverse);
    set_color(cr, col);
    cairo_arc_negative(cr, cx, cy, radius, start_rad,
                       start_rad - DEG2RAD(ARC_SPAN_DEG * frac));
    cairo_stroke(cr);

    /* Value text */
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", (int)g->value);
    draw_text_centered(cr, x, y + h * 0.30, w, h * 0.40,
                       buf, fmax(20, radius * 0.50), COL_WHITE, 1);

    /* Unit */
    draw_text_centered(cr, cx - radius, cy + radius * 0.22,
                       radius * 2, radius * 0.36,
                       g->unit, fmax(8, radius * 0.22), COL_DIM_GREY, 1);

    /* Label */
    draw_text_centered(cr, cx - radius, cy + radius * 0.54,
                       radius * 2, radius * 0.36,
                       g->label, fmax(8, radius * 0.22), COL_LABEL_RED, 1);
}

/* ══════════════════════════════════════════════════════════════════════════════
   HORIZONTAL BAR GAUGE  (Speed)
   ══════════════════════════════════════════════════════════════════════════════ */
static void draw_hbar_gauge(cairo_t *cr, double x, double y, double w, double h,
                             HBarGaugeData *g)
{
    draw_panel_bg(cr, x, y, w, h);

    Color col = value_color(g->value, g->warn_val, g->crit_val, g->reverse);

    /* Label */
    draw_text_centered(cr, x, y + h * 0.06, w, h * 0.18,
                       g->label, fmax(9, h * 0.09), COL_LABEL_RED, 1);

    /* Value */
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", (int)g->value);
    draw_text_centered(cr, x, y + h * 0.18, w, h * 0.50,
                       buf, fmax(24, h * 0.30), col, 1);

    /* Unit */
    draw_text_centered(cr, x, y + h * 0.66, w, h * 0.14,
                       g->unit, fmax(8, h * 0.085), COL_DIM_GREY, 0);

    /* Bar track */
    double bm = w * 0.06;
    double bw = w - bm * 2;
    double bh = fmax(14, h * 0.12);
    double bx = x + bm;
    double by = y + h * 0.80;

    set_color(cr, COL_TRACK);
    cairo_rectangle(cr, bx, by, bw, bh);
    cairo_fill(cr);

    /* Bar fill */
    double frac   = (g->value - g->min_val) / (g->max_val - g->min_val);
    double fill_w = fmax(0.0, bw * frac);
    if (fill_w > 0) {
        set_color(cr, col);
        cairo_rectangle(cr, bx, by, fill_w, bh);
        cairo_fill(cr);
    }

    /* Tick marks */
    set_color(cr, COL_TICK_GREY);
    cairo_set_line_width(cr, 1.0);
    for (int i = 1; i < 10; i++) {
        double tx = bx + bw * i / 10.0;
        cairo_move_to(cr, tx, by);
        cairo_line_to(cr, tx, by + bh);
        cairo_stroke(cr);
    }

    /* Min / max labels */
    char min_buf[16], max_buf[16];
    snprintf(min_buf, sizeof(min_buf), "%d", (int)g->min_val);
    snprintf(max_buf, sizeof(max_buf), "%d", (int)g->max_val);
    double lbl_fs = fmax(7, h * 0.075);
    draw_text_centered(cr, bx, by + bh + 2, bw * 0.25, h * 0.10,
                       min_buf, lbl_fs, COL_TICK_GREY, 0);
    draw_text_centered(cr, bx + bw * 0.75, by + bh + 2, bw * 0.25, h * 0.10,
                       max_buf, lbl_fs, COL_TICK_GREY, 0);
}

/* ══════════════════════════════════════════════════════════════════════════════
   LAP TIMER GAUGE
   ══════════════════════════════════════════════════════════════════════════════ */
static void draw_lap_timer(cairo_t *cr, double x, double y, double w, double h,
                            LapTimerData *g)
{
    draw_panel_bg(cr, x, y, w, h);

    int total_s = (int)g->value;
    int mins    = total_s / 60;
    int secs    = total_s % 60;
    int centis  = (int)((g->value - total_s) * 100);

    char buf[32];
    snprintf(buf, sizeof(buf), "%d:%02d:%02d", mins, secs, centis);

    draw_text_centered(cr, x, y + h * 0.06, w, h * 0.18,
                       g->label, fmax(9, h * 0.09), COL_LABEL_RED, 1);

    draw_text_centered(cr, x, y + h * 0.18, w, h * 0.50,
                       buf, fmax(20, h * 0.26), COL_WHITE, 1);

    draw_text_centered(cr, x, y + h * 0.66, w, h * 0.14,
                       "m : ss : cs", fmax(8, h * 0.085), COL_DIM_GREY, 0);
}

#endif /* GAUGES_H */
