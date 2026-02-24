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

static void tui_demo(void)
{
    /* Simulated sensor data — 6 frames (initial + 5 updates) */
    static const int cpu_vals[] = { 73,  82,  65,  91,  58,  77 };
    static const int mem_vals[] = { 45,  52,  48,  61,  55,  43 };
    static const double tmp_vals[] = { 78.3, 80.1, 76.5, 83.7, 74.2, 79.0 };
    static const char *status_msgs[] = {
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
    static const int check_states[] = { 1, 1, 0, 0, 1, 1 };
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

    tui_cls();
    tui_cursor_hide();

    /* ---- Level 0: outer frame (absolute 1,1) ---- */
    /* Terminal: 100 cols wide */
    static const tui_frame_t outer_frame = {
        .row = 1, .col = 1, .width = 100, .height = 30,
        .title = "ANSI TUI WIDGET DEMO", .color = "blue"
    };
    tui_frame_init(&outer_frame);

    /* ---- Level 1: four child frames in 2x2 grid ---- */

    /* Top-left: Sensors */
    static const tui_frame_t sensors_frame = {
        .row = 2, .col = 1, .width = 47, .height = 14,
        .title = "Sensors", .color = "cyan", .parent = &outer_frame
    };
    tui_frame_init(&sensors_frame);

    /* Top-right: Monitors */
    static const tui_frame_t monitors_frame = {
        .row = 2, .col = 49, .width = 47, .height = 14,
        .title = "Monitors", .color = "green", .parent = &outer_frame
    };
    tui_frame_init(&monitors_frame);

    /* Bottom-left: System */
    static const tui_frame_t system_frame = {
        .row = 16, .col = 1, .width = 47, .height = 9,
        .title = "System", .color = "yellow", .parent = &outer_frame
    };
    tui_frame_init(&system_frame);

    /* Bottom-right: Alerts */
    static const tui_frame_t alerts_frame = {
        .row = 16, .col = 49, .width = 47, .height = 9,
        .title = "Alerts", .color = "red", .parent = &outer_frame
    };
    tui_frame_init(&alerts_frame);

    /* ---- Level 2: widgets inside child frames ---- */

    /* Labels inside sensors frame */
    static tui_label_state_t cpu_label_st, mem_label_st, tmp_label_st;
    static const tui_label_t cpu_label = {
        .row = 1, .col = 1, .width = 10,
        .border = ANSI_TUI_BORDER,
        .label = "CPU", .color = "cyan", .state = &cpu_label_st,
        .parent = &sensors_frame
    };
    tui_label_init(&cpu_label);

    static const tui_label_t mem_label = {
        .row = 4, .col = 1, .width = 10,  /* tui_below(&cpu_label) */
        .border = ANSI_TUI_BORDER,
        .label = "MEM", .color = "cyan", .state = &mem_label_st,
        .parent = &sensors_frame
    };
    tui_label_init(&mem_label);

    static const tui_label_t tmp_label = {
        .row = 7, .col = 1, .width = 10,  /* tui_below(&mem_label) */
        .border = ANSI_TUI_BORDER,
        .label = "TMP", .color = "cyan", .state = &tmp_label_st,
        .parent = &sensors_frame
    };
    tui_label_init(&tmp_label);

#if ANSI_PRINT_BAR
    /* Bars inside monitors frame */
    static char cpu_bar_buf[256], mem_bar_buf[256];
    static tui_bar_state_t cpu_bar_st, mem_bar_st;
    static const tui_bar_t cpu_bar = {
        .row = 1, .col = 1, .bar_width = 35,
        .border = ANSI_TUI_BORDER,
        .label = "CPU ", .color = "green",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = cpu_bar_buf, .bar_buf_size = sizeof(cpu_bar_buf),
        .state = &cpu_bar_st, .parent = &monitors_frame
    };
    tui_bar_init(&cpu_bar);

    static const tui_bar_t mem_bar = {
        .row = 5, .col = 1, .bar_width = 35,
        .border = ANSI_TUI_BORDER,
        .label = "MEM ", .color = "yellow",
        .track = ANSI_BAR_LIGHT,
        .bar_buf = mem_bar_buf, .bar_buf_size = sizeof(mem_bar_buf),
        .state = &mem_bar_st, .parent = &monitors_frame
    };
    tui_bar_init(&mem_bar);
#endif

    /* Check inside system frame */
    static tui_check_state_t sys_check_st;
    static const tui_check_t sys_check = {
        .row = 1, .col = 1, .width = 0,
        .border = ANSI_TUI_BORDER,
        .label = "System OK", .color = "green",
        .state = &sys_check_st, .parent = &system_frame
    };
    tui_check_init(&sys_check, 1);

    /* Status inside system frame */
    tui_status_t sys_status = {
        .row = 4, .col = 1, .width = 36,
        .border = ANSI_TUI_BORDER, .color = "green",
        .parent = &system_frame
    };
    tui_status_init(&sys_status);

    /* Label + status inside alerts frame */
    static tui_label_state_t alert_label_st;
    static const tui_label_t alert_label = {
        .row = 1, .col = 1, .width = 10,
        .border = ANSI_TUI_BORDER,
        .label = "Level", .color = "red", .state = &alert_label_st,
        .parent = &alerts_frame
    };
    tui_label_init(&alert_label);

    tui_status_t alert_status = {
        .row = 4, .col = 1, .width = 36,
        .border = ANSI_TUI_BORDER, .color = "red",
        .parent = &alerts_frame
    };
    tui_status_init(&alert_status);

    /* Footer text widgets inside outer frame */
    static const tui_text_t footer_text = {
        .row = 27, .col = 1, .width = -1,
        .border = ANSI_TUI_NO_BORDER, .color = NULL, .parent = &outer_frame
    };
    tui_text_init(&footer_text);
    tui_text_update(&footer_text, "[dim]Live update demo — 5 seconds[/]");

    static const tui_text_t tick_text = {
        .row = 27, .col = 85, .width = 10,
        .border = ANSI_TUI_NO_BORDER, .color = NULL, .parent = &outer_frame
    };
    tui_text_init(&tick_text);

    /* Animate: initial frame + 5 updates at 1-second intervals */
    for (int i = 0; i < nframes; i++) {
        /* Disable MEM widgets during thermal warning (frames 2-3) */
        int mem_disabled = (i == 2 || i == 3);
        tui_label_enable(&mem_label, !mem_disabled);
#if ANSI_PRINT_BAR
        tui_bar_enable(&mem_bar, !mem_disabled);
#endif

        /* Color CPU label by load */
        const char *cpu_color = cpu_vals[i] >= 90 ? "red"
                              : cpu_vals[i] >= 70 ? "yellow" : "green";
        tui_label_update(&cpu_label, "[%s]%d%%[/]", cpu_color, cpu_vals[i]);
        tui_label_update(&mem_label, "[yellow]%d%%[/]", mem_vals[i]);
        tui_label_update(&tmp_label, "[red]%.1f C[/]", tmp_vals[i]);

#if ANSI_PRINT_BAR
        tui_bar_update(&cpu_bar, (double)cpu_vals[i], 0.0, 100.0);
        tui_bar_update(&mem_bar, (double)mem_vals[i], 0.0, 100.0);
#endif

        tui_check_update(&sys_check, check_states[i]);

        sys_status.color = status_colors[i];
        tui_status_init(&sys_status);
        tui_status_update(&sys_status, "%s", status_msgs[i]);

        /* Update alerts panel */
        tui_label_update(&alert_label, "%s", alert_levels[i]);
        tui_status_update(&alert_status, "%s", alert_msgs[i]);

        tui_text_update(&tick_text, "[dim]t=%d[/]", i);

        my_flush();
        if (i < nframes - 1) sleep(1);
    }

    /* Park cursor below the frame */
    tui_goto(31, 1);
    tui_cursor_show();

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

    if (argc < 2) {
        fprintf(stderr, "Usage: ansiprint [--demo | --tui-demo] [<markup string> ...]\n");
        fprintf(stderr, "  --demo       Show feature showcase\n");
        fprintf(stderr, "  --tui-demo   Show positioned TUI widget demo\n");
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
