/*
 * dashboard.c — FS Electric Race Dashboard
 * GTK3 + Cairo port of the PyQt5 dashboard
 *
 * Compile:
 *   gcc dashboard.c -o dashboard \
 *       $(pkg-config --cflags --libs gtk+-3.0) -lm
 *
 * Run:
 *   ./dashboard
 */

#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gauges.h"

/* ── Telemetry JSON reader (same format as Python version) ─────────────────── */
typedef struct {
    double speed;
    double rpm;
    double accum_voltage;
    double lv_battery;
    double soc;
    double coolant_temp;
    double lap_time;
} Telemetry;

static Telemetry g_telem = {0};
static double    g_lap_seconds = 74.35;

/* Very minimal JSON parser — reads flat key:value pairs */
static void parse_telemetry_json(const char *path, Telemetry *t)
{
    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        double v;
        if (sscanf(line, " \"speed\" : %lf", &v))           t->speed         = v;
        if (sscanf(line, " \"rpm\" : %lf", &v))             t->rpm           = v;
        if (sscanf(line, " \"accum_voltage\" : %lf", &v))   t->accum_voltage = v;
        if (sscanf(line, " \"lv_battery\" : %lf", &v))      t->lv_battery    = v;
        if (sscanf(line, " \"soc\" : %lf", &v))             t->soc           = v;
        if (sscanf(line, " \"coolant_temp\" : %lf", &v))    t->coolant_temp  = v;
        if (sscanf(line, " \"lap_time\" : %lf", &v))        t->lap_time      = v;
    }
    fclose(f);
}

/* ── Gauge data instances ───────────────────────────────────────────────────── */
static TextGaugeData    g_coolant   = {"COOLANT",       "°C", 0,  120, 80,  100,  0, 0, "%.1f"};
static BatteryGaugeData g_soc       = {"ACCUM SoC",     "%",  0,  100, 50,  20,   0};
static ArcGaugeData     g_rpm       = {"MOTOR RPM",     "rpm",0,  12000,9000,11000,0, 1,{1,1,1},0};
static HBarGaugeData    g_speed     = {"SPEED",         "km/h",0, 150, 80,  120,  0};
static LapTimerData     g_lap       = {"LAP TIME",      74.35};
static TextGaugeData    g_lv        = {"LV BATTERY",    "V",  10, 14.6,13,  10.8, 0, 1, "%.2f"};
static TextGaugeData    g_accv      = {"ACCUM VOLTAGE", "V",  0,  600, 300, 500,  0, 0, "%.1f"};

/* ── Drawing area + layout ─────────────────────────────────────────────────── */

/*
 * Layout (matches Python dashboard):
 *
 *  Row 0 (h=1):  Coolant (cols 0-2)  |  SoC (cols 3-5)
 *  Row 1 (h=1):  RPM (cols 0-1)  |  Speed (cols 2-3)  |  Lap (cols 4-5)
 *  Row 2 (h=1):  LV Battery (cols 0-2)  |  Accum Voltage (cols 3-5)
 *
 * Each col = total_w / 6
 * Each row = total_h / 3
 */

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    (void)data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    double W = alloc.width;
    double H = alloc.height;

    /* Fill window background */
    cairo_set_source_rgb(cr, 0.039, 0.039, 0.039); /* #0A0A0A */
    cairo_paint(cr);

    double col  = W / 6.0;
    double row  = H / 3.0;
    double pad  = 8.0;

    /* Helper macro for panel x, y, w, h with padding */
    #define CELL(c0, r0, cspan, rspan) \
        (c0)*col + pad, (r0)*row + pad, \
        (cspan)*col - pad*2, (rspan)*row - pad*2

    /* Row 0 */
    draw_text_gauge   (cr, CELL(0, 0, 3, 1), &g_coolant);
    draw_battery_gauge(cr, CELL(3, 0, 3, 1), &g_soc);

    /* Row 1 */
    draw_arc_gauge    (cr, CELL(0, 1, 2, 1), &g_rpm);
    draw_hbar_gauge   (cr, CELL(2, 1, 2, 1), &g_speed);
    draw_lap_timer    (cr, CELL(4, 1, 2, 1), &g_lap);

    /* Row 2 */
    draw_text_gauge   (cr, CELL(0, 2, 3, 1), &g_lv);
    draw_text_gauge   (cr, CELL(3, 2, 3, 1), &g_accv);

    #undef CELL

    return FALSE;
}

/* ── Telemetry update timer (200 ms = 5 Hz) ────────────────────────────────── */
static GtkWidget *g_canvas = NULL;

static gboolean update_telemetry(gpointer data)
{
    (void)data;

    parse_telemetry_json("telemetry.json", &g_telem);

    /* If file has data use it, otherwise increment lap timer as placeholder */
    if (g_telem.speed > 0 || g_telem.rpm > 0) {
        g_speed.value   = g_telem.speed;
        g_rpm.value     = g_telem.rpm;
        g_accv.value    = g_telem.accum_voltage;
        g_lv.value      = g_telem.lv_battery;
        g_soc.value     = g_telem.soc;
        g_coolant.value = g_telem.coolant_temp;
        g_lap.value     = g_telem.lap_time;
    } else {
        g_lap_seconds  += 0.2;
        g_lap.value     = g_lap_seconds;
    }

    /* Trigger redraw */
    if (g_canvas)
        gtk_widget_queue_draw(g_canvas);

    return G_SOURCE_CONTINUE;
}

/* ── Main ───────────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    /* Window */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "FS Electric Dashboard");
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);
    gtk_window_fullscreen(GTK_WINDOW(window));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Drawing area */
    g_canvas = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), g_canvas);
    g_signal_connect(g_canvas, "draw", G_CALLBACK(on_draw), NULL);

    /* Start 200ms update timer */
    g_timeout_add(200, update_telemetry, NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
