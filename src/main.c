#include "ansi_print.h"
#include <stdio.h>
#include <string.h>

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
    ansi_puts("  ");
    ansi_rainbow("System initialization complete");
    ansi_puts("\n");
    ansi_puts("  [gradient red blue]Gradient: red to blue[/gradient]\n");
    ansi_puts("  [gradient green yellow]Gradient: green to yellow[/gradient]\n");
    ansi_puts("  [gradient cyan magenta]Gradient: cyan to magenta[/gradient]\n\n");
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
                "FAULT: Overvoltage on rail %s (%.1fV)", "VDD_3V3", 3.6);
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

    /* --- Simulated embedded system output --- */
    ansi_puts("[bold underline]Embedded System Boot Log[/]\n\n");

    ansi_print("[dim]%s[/] [bold cyan]BOOT[/]  Hardware rev %d.%d\n",
               "[00:00.001]", 3, 2);
    ansi_print("[dim]%s[/] [bold cyan]INIT[/]  CPU @ %d MHz, %d KB SRAM\n",
               "[00:00.003]", 168, 192);
    ansi_print("[dim]%s[/] [bold cyan]INIT[/]  Flash: %d KB (%d%% used)\n",
               "[00:00.005]", 1024, 67);

    ansi_print("[dim]%s[/] :gear:  [cyan]Peripheral init[/] ",
               "[00:00.010]");
    ansi_puts("[white on green] OK [/]\n");

    ansi_print("[dim]%s[/] :gear:  [cyan]UART1 @ %d baud[/] ",
               "[00:00.012]", 115200);
    ansi_puts("[white on green] OK [/]\n");

    ansi_print("[dim]%s[/] :gear:  [cyan]SPI2 @ %d MHz[/] ",
               "[00:00.014]", 42);
    ansi_puts("[white on green] OK [/]\n");

    ansi_print("[dim]%s[/] :gear:  [cyan]I2C1 @ %d kHz[/] ",
               "[00:00.015]", 400);
    ansi_puts("[white on green] OK [/]\n");

    ansi_print("[dim]%s[/] :gear:  [cyan]ADC1 (%d ch)[/] ",
               "[00:00.018]", 4);
    ansi_puts("[white on green] OK [/]\n");

    ansi_print("[dim]%s[/] :gear:  [cyan]Ethernet MAC[/] ",
               "[00:00.025]");
    ansi_puts("[white on green] OK [/]");
    ansi_print("  link [bold green]UP[/] %d Mbps\n", 100);

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
               "[00:00.050]", 5);
    ansi_print("[dim]%s[/] [bold]TASK[/]  [green]:check: sensor_read[/]   prio=%d  stk=%d\n",
               "[00:00.051]", 3, 512);
    ansi_print("[dim]%s[/] [bold]TASK[/]  [green]:check: motor_ctrl[/]    prio=%d  stk=%d\n",
               "[00:00.052]", 2, 1024);
    ansi_print("[dim]%s[/] [bold]TASK[/]  [green]:check: comms_handler[/] prio=%d  stk=%d\n",
               "[00:00.053]", 4, 768);
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

int main(int argc, char *argv[])
{
    ansi_init(my_putc, my_flush, fmt_buf, sizeof(fmt_buf));
    ansi_enable();

    if (argc >= 2 && strcmp(argv[1], "--demo") == 0) {
        demo();
        return 0;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: ansiprint [--demo] [<markup string> ...]\n");
        fprintf(stderr, "  --demo   Show feature showcase\n");
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
