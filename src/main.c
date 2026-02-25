#include "ansi_print.h"
#include "ansi_tui.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void my_putc(int ch)  { putchar(ch); }
static void my_flush(void)   { fflush(stdout); }

static char fmt_buf[1024];

static void demo(void)
{
    ansi_puts("[bold underline]ansi_print demo[/]\n");
    ansi_puts("[dim]Rich-style colored text and emoji for PC, embedded, and terminal emulator output[/]\n\n");

    /* --- Standard colors --- */
    ansi_puts("[bold]Standard Colors[/]\n");
    ansi_puts("  [black]black[/] [red]red[/] [green]green[/] [yellow]yellow[/]"
              " [blue]blue[/] [magenta]magenta[/] [cyan]cyan[/] [white]white[/]\n\n");

    /* --- Extended colors --- */
#if ANSI_PRINT_EXTENDED_COLORS
    ansi_puts("[bold]Extended Colors[/]\n");
    ansi_puts("  [orange]orange[/] [pink]pink[/] [purple]purple[/] [brown]brown[/]"
              " [teal]teal[/] [lime]lime[/] [navy]navy[/] [olive]olive[/]"
              " [maroon]maroon[/] [aqua]aqua[/] [silver]silver[/] [gray]gray[/]\n\n");
#endif

    /* --- Bright colors --- */
#if ANSI_PRINT_BRIGHT_COLORS
    ansi_puts("[bold]Bright Colors[/]\n");
    ansi_puts("  [bright_red]bright_red[/] [bright_green]bright_green[/]"
              " [bright_yellow]bright_yellow[/] [bright_blue]bright_blue[/]"
              " [bright_magenta]bright_magenta[/] [bright_cyan]bright_cyan[/]\n\n");
#endif

    /* --- Styles --- */
#if ANSI_PRINT_STYLES
    ansi_puts("[bold]Text Styles[/]\n");
    ansi_puts("  [bold]bold[/] [dim]dim[/] [italic]italic[/]"
              " [underline]underline[/] [invert]invert[/]"
              " [strikethrough]strikethrough[/]\n\n");
#endif

    /* --- Backgrounds --- */
    ansi_puts("[bold]Foreground on Background[/]\n");
    ansi_puts("  [white on red] FAULT [/] [black on yellow] WARN [/]"
              " [white on green] OK [/] [white on blue] INFO [/]\n\n");

    /* --- Numeric 256-color --- */
    ansi_puts("[bold]Numeric 256-Color[/]\n");
    ansi_puts("  [fg:196]fg:196[/] [fg:208]fg:208[/] [fg:226]fg:226[/]"
              " [fg:46]fg:46[/] [fg:51]fg:51[/] [fg:93]fg:93[/]\n\n");

    /* --- Rainbow & gradient --- */
#if ANSI_PRINT_GRADIENTS
    ansi_puts("[bold]Rainbow & Gradient[/]\n");
    ansi_puts("  [bold][rainbow]System initialization complete[/rainbow][/]\n");
    ansi_puts("  [gradient red blue]Gradient: red to blue[/gradient]\n\n");
#endif

    /* --- Emoji --- */
#if ANSI_PRINT_EMOJI
    ansi_puts("[bold]Emoji Shortcodes[/]\n");
    ansi_puts("  :check: check  :cross: cross  :warning: warning"
              "  :fire: fire  :rocket: rocket  :gear: gear\n");
    ansi_puts("  :star: star  :zap: zap  :bug: bug"
              "  :wrench: wrench  :bell: bell  :sparkles: sparkles\n");
#if ANSI_PRINT_EXTENDED_EMOJI
    ansi_puts("  :red_box: :orange_box: :yellow_box: :green_box:"
              " :blue_box: :purple_box: :brown_box: :white_box: :black_box: boxes\n");
#endif
    ansi_puts("\n");
#endif

    /* --- Unicode codepoints --- */
#if ANSI_PRINT_UNICODE
    ansi_puts("[bold]Unicode Codepoints[/]\n");
    ansi_puts("  :U-2714: U-2714  :U-2620: U-2620  :U-2764: U-2764"
              "  :U-1F525: U-1F525  :U-1F680: U-1F680\n\n");
#endif

#if ANSI_PRINT_BANNER
    /* --- Banners --- */
    ansi_puts("[bold]Banners[/]\n");
    ansi_banner("red", 0, ANSI_ALIGN_LEFT,
                "FAULT: Over voltage on rail %s (%.1fV)", "VDD_3V3", 3.6);
    ansi_banner("green", 0, ANSI_ALIGN_LEFT,
                "Self-test passed -- %d/%d checks OK", 17, 17);
    ansi_banner("cyan", 40, ANSI_ALIGN_CENTER,
                "Firmware v%d.%d.%d\n"
                "Build: %s\n"
                "Status: %s",
                2, 4, 1, "Feb 21 2026", "Ready");
    ansi_banner("yellow", 0, ANSI_ALIGN_RIGHT,
                "ADC Channels\n"
                "  CH0: %5.2fV\n"
                "  CH1: %5.2fV\n"
                "  CH2: %5.2fV\n"
                "  CH3: %5.2fV",
                3.29, 1.81, 0.42, 2.50);
    ansi_puts("\n");
#endif

#if ANSI_PRINT_WINDOW
    /* --- Windows --- */
    ansi_puts("[bold]Windows[/]\n");
    ansi_window_start("cyan", 40, ANSI_ALIGN_CENTER, "Sensor Readings");
    ansi_window_line(ANSI_ALIGN_LEFT, "[green]Temperature: %5.1f C[/]", 23.4);
    ansi_window_line(ANSI_ALIGN_LEFT, "[yellow]Humidity:    %5.1f %%[/]", 61.2);
    ansi_window_line(ANSI_ALIGN_LEFT, "[red]Pressure:    %5.1f hPa[/]", 1013.2);
    ansi_window_end();

    ansi_window_start("yellow", 30, ANSI_ALIGN_LEFT, NULL);
    ansi_window_line(ANSI_ALIGN_CENTER, "No title window");
    ansi_window_line(ANSI_ALIGN_CENTER, "Width = %d", 30);
    ansi_window_end();
    ansi_puts("\n");
#endif

#if ANSI_PRINT_BAR
    /* --- Bar Graphs --- */
    ansi_puts("[bold]Bar Graphs[/]\n");
    {
        char bar[128];
        const char *names[]  = {"light", "med  ", "heavy", "dot  ", "line ", "blank"};
        const char *colors[] = {"green", "cyan", "yellow", "blue", "red", "magenta"};
        int loads[]          = {73, 45, 40, 18, 60, 15};
        /* Each bar uses a different track character to showcase the options */
        ansi_bar_track_t tracks[] = {
            ANSI_BAR_LIGHT, ANSI_BAR_MED,  ANSI_BAR_HEAVY,
            ANSI_BAR_DOT,   ANSI_BAR_LINE, ANSI_BAR_BLANK,
        };
        for (int i = 0; i < 6; i++) {
            ansi_print("  %s %s\n",
                       names[i],
                       ansi_bar_percent(bar, sizeof(bar), colors[i], 20,
                                        tracks[i], loads[i]));
        }
    }
    ansi_puts("\n");
#endif

    /* --- Simulated embedded system output --- */
    ansi_puts("[bold underline]Embedded System Boot Log[/]\n\n");

    ansi_print("[dim]%s[/] [bold cyan]BOOT[/]  Hardware rev %d.%d  CPU @ %d MHz\n",
               "[00:00.001]", 3, 2, 168);

    ansi_print("[dim]%s[/] :gear:  [cyan]Peripheral init[/] ",
               "[00:00.010]");
    ansi_puts("[white on green] OK [/]\n");

    ansi_print("[dim]%s[/] :gear:  [cyan]CAN bus[/] ",
               "[00:00.030]");
    ansi_puts("[black on yellow] WARN [/]");
    ansi_puts("  no peers detected\n");

    ansi_print("[dim]%s[/] :gear:  [cyan]USB OTG[/] ",
               "[00:00.032]");
    ansi_puts("[white on red] FAULT [/]");
    ansi_puts("  [red]VBUS not present[/]\n");

    ansi_puts("\n");
    ansi_print("[dim]%s[/] [bold]TASK[/]  Starting scheduler (%d tasks)\n",
               "[00:00.050]", 3);
    ansi_print("[dim]%s[/] [bold]TASK[/]  [green]:check: sensor_read[/]   prio=%d  stk=%d\n",
               "[00:00.051]", 3, 512);
    ansi_print("[dim]%s[/] [bold]TASK[/]  [red]:cross: data_logger[/]   prio=%d  stk=%d",
               "[00:00.054]", 5, 256);
    ansi_puts("  [red]stack overflow[/]\n");
    ansi_print("[dim]%s[/] [bold]TASK[/]  [green]:check: watchdog[/]      prio=%d  stk=%d\n",
               "[00:00.055]", 1, 128);

    ansi_puts("\n");
    ansi_print("[dim]%s[/] :check: [bold green]System ready[/]  "
               "uptime %d ms  free heap %d bytes\n",
               "[00:00.060]", 60, 45312);
}

/* ---- TUI demo: widget declarations (file scope) ---- */

/* Simulated sensor data — 6 frames (initial + 5 updates) */
static const int    cpu_vals[]  = { 73,  82,  65,  91,  58,  77 };
static const int    mem_vals[]  = { 45,  52,  48,  61,  55,  43 };
static const double tmp_vals[]  = { 78.3, 80.1, 76.5, 83.7, 74.2, 79.0 };
static const double vlt_vals[]  = { 3.30, 3.28, 3.25, 3.15, 3.22, 3.31 };
static const double freq_vals[] = { 1200.0, 1800.0, 1500.0, 2100.0, 800.0, 1600.0 };
static const int    check_states[] = { 1, 1, 0, 0, 1, 1 };
static const char  *status_msgs[] = {
    "[green]All systems nominal[/]",
    "[cyan]Sensor calibrating...[/]",
    "[yellow]Temperature rising[/]",
    "[red]Thermal warning![/]",
    "[cyan]Cooling active[/]",
    "[green]All systems nominal[/]",
};
static const char *status_colors[] = {
    "green", "cyan", "yellow", "red", "cyan", "green",
};
static const char *uptime_vals[] = {
    "0:00:00", "0:00:01", "0:00:02", "0:00:03", "0:00:04", "0:00:05",
};
static const char *io_vals[] = {
    "[green]12.4 MB/s[/]", "[yellow]34.7 MB/s[/]", "[green]8.1 MB/s[/]",
    "[red]67.2 MB/s[/]", "[green]15.3 MB/s[/]", "[green]11.9 MB/s[/]",
};
static const char *log_msgs[] = {
    "[dim]sched: idle[/]",
    "[cyan]sensor: calibrating ADC[/]",
    "[yellow]therm: temp rising +2.1[/]",
    "[red]therm: WARNING limit exceeded[/]",
    "[cyan]cool: fan speed 80%%[/]",
    "[green]sched: all tasks nominal[/]",
};

static const char *alert_levels[] = {
    "[green]OK[/]", "[green]OK[/]", "[yellow]WARN[/]",
    "[red]CRIT[/]", "[yellow]WARN[/]", "[green]OK[/]",
};
static const char *alert_msgs[] = {
    "[green]No active alerts[/]",
    "[green]No active alerts[/]",
    "[yellow]Temp approaching limit[/]",
    "[red]Thermal shutdown imminent![/]",
    "[cyan]Recovery in progress[/]",
    "[green]No active alerts[/]",
};
static const int nframes = 6;

/* Frame layout constants */
#define TOP_ROW      1
#define TOP_HEIGHT   14
#define BOT_ROW      (TOP_ROW + TOP_HEIGHT)          /* row below top frames */
#define OUTER_HEIGHT 29
#define BOT_HEIGHT   (OUTER_HEIGHT - BOT_ROW - 2)    /* fill to row -2 */

/* Frames */
static const tui_frame_t outer_frame = {
    .row = 1, .col = 1, .width = 100, .height = OUTER_HEIGHT,
    .title = "ANSI TUI WIDGET DEMO", .color = "blue"
};
static const tui_frame_t sensors_frame = {
    .row = TOP_ROW, .col = 1, .width = 47, .height = TOP_HEIGHT,
    .title = "Sensors", .color = "cyan", .parent = &outer_frame
};
static const tui_frame_t monitors_frame = {
    .row = TOP_ROW, .col = 49, .width = 47, .height = TOP_HEIGHT,
    .title = "Monitors", .color = "green", .parent = &outer_frame
};
static const tui_frame_t system_frame = {
    .row = BOT_ROW, .col = 1, .width = 47, .height = BOT_HEIGHT,
    .title = "System", .color = "yellow", .parent = &outer_frame
};
static const tui_frame_t alerts_frame = {
    .row = BOT_ROW, .col = 49, .width = 47, .height = BOT_HEIGHT,
    .title = "Alerts", .color = "red", .parent = &outer_frame
};

/* Labels */
static tui_label_state_t cpu_label_st, mem_label_st, tmp_label_st;
static const tui_label_t cpu_label = {
    .place = { .row = 1, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "cyan", .parent = &sensors_frame },
    .width = 10, .label = "CPU", .state = &cpu_label_st
};
static const tui_label_t mem_label = {
    .place = { .row = 4, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "cyan", .parent = &sensors_frame },
    .width = 10, .label = "MEM", .state = &mem_label_st
};
static const tui_label_t tmp_label = {
    .place = { .row = 7, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "cyan", .parent = &sensors_frame },
    .width = 10, .label = "TMP", .state = &tmp_label_st
};

/* Percent bar */
#if ANSI_PRINT_BAR
static char cpu_pbar_buf[256];
static tui_pbar_state_t cpu_pbar_st;
static const tui_pbar_t cpu_pbar = {
    .place = { .row = 10, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "green", .parent = &sensors_frame },
    .bar_width = 30, .label = "CPU ",
    .track = ANSI_BAR_LIGHT,
    .bar_buf = cpu_pbar_buf, .bar_buf_size = sizeof(cpu_pbar_buf),
    .state = &cpu_pbar_st
};
#endif

/* Bars */
#if ANSI_PRINT_BAR
static char cpu_bar_buf[256], mem_bar_buf[256];
static tui_bar_state_t cpu_bar_st, mem_bar_st;
static const tui_bar_t cpu_bar = {
    .place = { .row = 1, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "green", .parent = &monitors_frame },
    .bar_width = 35, .label = "CPU ",
    .track = ANSI_BAR_LIGHT,
    .bar_buf = cpu_bar_buf, .bar_buf_size = sizeof(cpu_bar_buf),
    .state = &cpu_bar_st
};
static const tui_bar_t mem_bar = {
    .place = { .row = 4, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "yellow", .parent = &monitors_frame },
    .bar_width = 35, .label = "MEM ",
    .track = ANSI_BAR_LIGHT,
    .bar_buf = mem_bar_buf, .bar_buf_size = sizeof(mem_bar_buf),
    .state = &mem_bar_st
};
#endif

/* Metrics */
static tui_metric_state_t tmp_metric_st, vlt_metric_st;
static const tui_metric_t tmp_metric = {
    .place = { .row = 7, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "green", .parent = &monitors_frame },
    .width = 16, .title = "TEMP", .fmt = "%5.1f \xc2\xb0""F",
    .color_lo = "blue", .color_hi = "red",
    .thresh_lo = 76.0, .thresh_hi = 82.0,
    .state = &tmp_metric_st
};
static const tui_metric_t vlt_metric = {
    .place = { .row = 7, .col = 22, .border = ANSI_TUI_BORDER,
               .color = "green", .parent = &monitors_frame },
    .width = -1, .title = "VDD_3V3", .fmt = "%5.3f V",
    .color_lo = "red", .color_hi = "red",
    .thresh_lo = 3.20, .thresh_hi = 3.40,
    .state = &vlt_metric_st
};

/* Frequency metric (emoji in title, next to CPU label in Sensors) */
static tui_metric_state_t freq_metric_st;
static const tui_metric_t freq_metric = {
    .place = { .row = 1, .col = 20, .border = ANSI_TUI_BORDER,
               .color = "green", .parent = &sensors_frame },
    .width = 20, .title = ":zap: MHz", .fmt = "%4.0f",
    .color_lo = "blue", .color_hi = "red",
    .thresh_lo = 1000.0, .thresh_hi = 2000.0,
    .state = &freq_metric_st
};

/* Check */
static tui_check_state_t sys_check_st;
static const tui_check_t sys_check = {
    .place = { .row = 1, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "green", .parent = &system_frame },
    .width = 0, .label = "System OK", .state = &sys_check_st
};

/* Status (mutable — color changes at runtime) */
static tui_status_t sys_status = {
    .place = { .row = 4, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "green", .parent = &system_frame },
    .width = 36
};

/* Alert label + status */
static tui_label_state_t alert_label_st;
static const tui_label_t alert_label = {
    .place = { .row = 1, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "red", .parent = &alerts_frame },
    .width = 10, .label = "Level", .state = &alert_label_st
};
static tui_status_t alert_status = {
    .place = { .row = 4, .col = 1, .border = ANSI_TUI_BORDER,
               .color = "red", .parent = &alerts_frame },
    .width = 36
};

/* Borderless widgets in Monitors (rows 10-12) */
static tui_label_state_t uptime_label_st;
static const tui_label_t uptime_label = {
    .place = { .row = 10, .col = 1, .border = ANSI_TUI_NO_BORDER,
               .color = NULL, .parent = &monitors_frame },
    .width = 10, .label = "Uptime", .state = &uptime_label_st
};
static tui_label_state_t io_label_st;
static const tui_label_t io_label = {
    .place = { .row = 11, .col = 1, .border = ANSI_TUI_NO_BORDER,
               .color = NULL, .parent = &monitors_frame },
    .width = 15, .label = "I/O", .state = &io_label_st
};
static const tui_text_t log_text = {
    .place = { .row = 12, .col = 1, .border = ANSI_TUI_NO_BORDER,
               .color = NULL, .parent = &monitors_frame },
    .width = -1
};

/* Footer (negative row = from bottom of parent interior) */
static const tui_text_t footer_text = {
    .place = { .row = -1, .col = 1, .border = ANSI_TUI_NO_BORDER,
               .color = NULL, .parent = &outer_frame },
    .width = -1
};
static const tui_text_t tick_text = {
    .place = { .row = -1, .col = 85, .border = ANSI_TUI_NO_BORDER,
               .color = NULL, .parent = &outer_frame },
    .width = 10
};

/*
 * draw_tui() — canonical TUI update pattern.
 *
 * Call with force=1 for initial paint or full repaint (e.g. after
 * terminal resize).  Call with force=0 for incremental updates
 * where only changed widgets redraw.
 */
static void draw_tui(int frame, int tick, int force)
{
    /* Double-buffer the screen for smooth updates (DEC mode 2026).
     * Not all terminals support this; unsupported terminals will
     * silently ignore it and you may see incremental redraws. */
    tui_sync_begin();

    /* Enable/disable MEM widgets during thermal warning */
    int mem_disabled = (frame == 2 || frame == 3);
    tui_label_enable(&mem_label, !mem_disabled);
#if ANSI_PRINT_BAR
    tui_bar_enable(&mem_bar, !mem_disabled);
#endif

    
    /* Color CPU label by load */
    const char *cpu_color = cpu_vals[frame] >= 90 ? "red"
                          : cpu_vals[frame] >= 70 ? "yellow" : "green";
    tui_label_update(&cpu_label, "[%s]%d%%[/]", cpu_color, cpu_vals[frame]);
    tui_label_update(&mem_label, "[yellow]%d%%[/]", mem_vals[frame]);
    tui_label_update(&tmp_label, "[red]%.1f C[/]", tmp_vals[frame]);

#if ANSI_PRINT_BAR
    tui_bar_update(&cpu_bar, (double)cpu_vals[frame], 0.0, 100.0, force);
    tui_bar_update(&mem_bar, (double)mem_vals[frame], 0.0, 100.0, force);
    tui_pbar_update(&cpu_pbar, cpu_vals[frame], force);
#endif

    tui_metric_update(&tmp_metric, tmp_vals[frame], force);
    tui_metric_update(&vlt_metric, vlt_vals[frame], force);
    tui_metric_update(&freq_metric, freq_vals[frame], force);
    tui_check_update(&sys_check, check_states[frame], force);

    sys_status.place.color = status_colors[frame];
    if (force) tui_status_init(&sys_status);
    tui_status_update(&sys_status, "%s", status_msgs[frame]);

    tui_label_update(&alert_label, "%s", alert_levels[frame]);
    tui_status_update(&alert_status, "%s", alert_msgs[frame]);
    tui_label_update(&uptime_label, "[cyan]%s[/]", uptime_vals[frame]);
    tui_label_update(&io_label, "%s", io_vals[frame]);
    tui_text_update(&log_text, "%s", log_msgs[frame]);
    tui_text_update(&tick_text, "[dim]t=%d[/]", tick);

    tui_sync_end();
}

static void tui_demo(void)
{
    tui_cls();
    tui_cursor_hide();

    tui_sync_begin();

    /* Init frames */
    tui_frame_init(&outer_frame);
    tui_frame_init(&sensors_frame);
    tui_frame_init(&monitors_frame);
    tui_frame_init(&system_frame);
    tui_frame_init(&alerts_frame);

    /* Init content widgets */
    tui_label_init(&cpu_label);
    tui_label_init(&mem_label);
    tui_label_init(&tmp_label);
#if ANSI_PRINT_BAR
    tui_bar_init(&cpu_bar);
    tui_bar_init(&mem_bar);
    tui_pbar_init(&cpu_pbar);
#endif
    tui_metric_init(&tmp_metric);
    tui_metric_init(&vlt_metric);
    tui_metric_init(&freq_metric);
    tui_check_init(&sys_check, 1);
    tui_status_init(&sys_status);
    tui_label_init(&alert_label);
    tui_status_init(&alert_status);
    tui_label_init(&uptime_label);
    tui_label_init(&io_label);
    tui_text_init(&log_text);
    tui_text_init(&footer_text);
    tui_text_update(&footer_text, "[dim]Live update demo — %d frames[/]", nframes * 4);
    tui_text_init(&tick_text);

    tui_sync_end();

    /* Draw loop: four full cycles, quarter-second steps */
    int total = nframes * 4;
    for (int i = 0; i < total; i++) {
        draw_tui(i % nframes, i, i == 0 ? 1 : 0);
        my_flush();
        if (i < total - 1) usleep(120000);
    }

    /* Park cursor below the frame */
    tui_goto(OUTER_HEIGHT + 1, 1);
    tui_cursor_show();
}

static void emoji_test(void)
{
    const ansi_emoji_entry_t *table = ansi_emoji_table();
    int count = ansi_emoji_count();
    char line[128];

    ansi_window_start("cyan", 24, ANSI_ALIGN_CENTER, "Emoji Width Test");
    for (int i = 0; i < count; i++) {
        snprintf(line, sizeof(line), ":%s: %-14s", table[i].name, table[i].name);
        ansi_window_line(ANSI_ALIGN_LEFT, "%s", line);
    }
    ansi_window_end();
}

static void quick_start(void)
{
    char bar[128];

    ansi_banner("cyan", 50, ANSI_ALIGN_CENTER,
                ":rocket: Sensor Gateway v2.1\n"
                "Build: %s  :gear: %d cores",
                __DATE__, 4);
    ansi_puts("\n");

    ansi_window_start("green", 50, ANSI_ALIGN_LEFT, "Live Readings");
    ansi_window_line(ANSI_ALIGN_LEFT,
                     ":zap: Voltage  %s %5.2f V",
                     ansi_bar(bar, sizeof(bar), "green", 20,
                              ANSI_BAR_LIGHT, 3.29, 0.0, 5.0),
                     3.29);
    ansi_window_line(ANSI_ALIGN_LEFT,
                     ":fire: Temp     %s %5.1f C",
                     ansi_bar(bar, sizeof(bar), "yellow", 20,
                              ANSI_BAR_LIGHT, 42.7, 0.0, 100.0),
                     42.7);
    ansi_window_line(ANSI_ALIGN_LEFT,
                     ":warning: Load     %s",
                     ansi_bar_percent(bar, sizeof(bar), "red", 20,
                                     ANSI_BAR_LIGHT, 87));
    ansi_window_end();
    ansi_puts("\n");

    ansi_puts(":check: [green]Network[/]   :check: [green]Storage[/]"
              "   :cross: [red]GPS Lock[/]\n");
    ansi_puts("[bold][rainbow]All systems operational[/rainbow][/]\n");
}

int main(int argc, char *argv[])
{
    ansi_init(my_putc, my_flush, fmt_buf, sizeof(fmt_buf));
    ansi_enable();

    if (argc >= 2 && strcmp(argv[1], "--demo") == 0) {
        demo();
        return 0;
    }

    if (argc >= 2 && strcmp(argv[1], "--tui-demo") == 0) {
        tui_demo();
        return 0;
    }

    if (argc >= 2 && strcmp(argv[1], "--quick-start") == 0) {
        quick_start();
        return 0;
    }

    if (argc >= 2 && strcmp(argv[1], "--emoji-test") == 0) {
        emoji_test();
        return 0;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: ansiprint [--demo | --tui-demo | --quick-start | --emoji-test] "
                        "[<markup string> ...]\n");
        fprintf(stderr, "  --demo        Show feature showcase\n");
        fprintf(stderr, "  --tui-demo    Show positioned TUI widget demo\n");
        fprintf(stderr, "  --quick-start Quick start example output\n");
        fprintf(stderr, "  --emoji-test  Show all emoji in a window (width test)\n");
        fprintf(stderr, "Example: ansiprint \"[bold red]Error:[/] something broke\"\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (i > 1) my_putc(' ');
        ansi_puts(argv[i]);
    }
    my_putc('\n');

    return 0;
}
