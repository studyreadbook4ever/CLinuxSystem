/* myPanel.c
 *
 * 검은 배경 + 작은 monospace 글씨
 * time: 1초마다 갱신
 * uptime/battery: 10초마다 갱신
 *
 * 빌드:
 *   gcc -O2 -pipe myPanel.c -o myPanel $(pkg-config --cflags --libs gtk+-3.0)
 *
 * 실행:
 *   ./myPanel
 */

#include <gtk/gtk.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WINDOW_X 0
#define WINDOW_Y 0

typedef struct {
    GtkWidget *label;
    char time_line[64];
    char uptime_line[64];
    char battery_line[128];
    char battery_dir[PATH_MAX];
    gboolean has_battery;
} AppState;

static gboolean read_first_line(const char *path, char *buf, size_t len) {
    FILE *fp = fopen(path, "r");
    if (!fp) return FALSE;

    if (!fgets(buf, (int)len, fp)) {
        fclose(fp);
        return FALSE;
    }

    fclose(fp);
    buf[strcspn(buf, "\r\n")] = '\0';
    return TRUE;
}

static gboolean detect_battery(AppState *st) {
    DIR *d = opendir("/sys/class/power_supply");
    if (!d) return FALSE;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.')
            continue;

        char type_path[PATH_MAX];
        char present_path[PATH_MAX];
        char type[64];
        char present[16];

        snprintf(type_path, sizeof(type_path),
                 "/sys/class/power_supply/%s/type", ent->d_name);

        if (!read_first_line(type_path, type, sizeof(type)))
            continue;

        if (strcmp(type, "Battery") != 0)
            continue;

        snprintf(present_path, sizeof(present_path),
                 "/sys/class/power_supply/%s/present", ent->d_name);

        if (read_first_line(present_path, present, sizeof(present))) {
            if (strcmp(present, "1") != 0)
                continue;
        }

        snprintf(st->battery_dir, sizeof(st->battery_dir),
                 "/sys/class/power_supply/%s", ent->d_name);

        st->has_battery = TRUE;
        closedir(d);
        return TRUE;
    }

    closedir(d);
    st->has_battery = FALSE;
    st->battery_dir[0] = '\0';
    return FALSE;
}

static void on_window_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;

    GdkWindow *gdk_win = gtk_widget_get_window(widget);
    if (gdk_win)
        gdk_window_set_pass_through(gdk_win, TRUE);
}

static void format_current_time(AppState *st) {
    time_t now = time(NULL);
    struct tm tm_now;

    localtime_r(&now, &tm_now);
    strftime(st->time_line, sizeof(st->time_line),
             "time: %Y-%m-%d %H:%M:%S", &tm_now);
}

static void format_uptime(AppState *st) {
    FILE *fp = fopen("/proc/uptime", "r");
    if (!fp) {
        g_snprintf(st->uptime_line, sizeof(st->uptime_line), "uptime: N/A");
        return;
    }

    double up = 0.0;
    if (fscanf(fp, "%lf", &up) != 1) {
        fclose(fp);
        g_snprintf(st->uptime_line, sizeof(st->uptime_line), "uptime: N/A");
        return;
    }
    fclose(fp);

    long sec = (long)up;
    long days = sec / 86400;
    long hours = (sec % 86400) / 3600;
    long mins = (sec % 3600) / 60;

    if (days > 0) {
        g_snprintf(st->uptime_line, sizeof(st->uptime_line),
                   "uptime: %ldd %02ldh %02ldm", days, hours, mins);
    } else {
        g_snprintf(st->uptime_line, sizeof(st->uptime_line),
                   "uptime: %02ldh %02ldm", hours, mins);
    }
}

static void format_battery(AppState *st) {
    if (!st->has_battery) {
        st->battery_line[0] = '\0';
        return;
    }

    char cap_path[PATH_MAX];
    char stat_path[PATH_MAX];
    char capacity[32];
    char status[64];

    snprintf(cap_path, sizeof(cap_path), "%s/capacity", st->battery_dir);
    snprintf(stat_path, sizeof(stat_path), "%s/status", st->battery_dir);

    if (!read_first_line(cap_path, capacity, sizeof(capacity))) {
        g_snprintf(st->battery_line, sizeof(st->battery_line), "battery: N/A");
        return;
    }

    if (!read_first_line(stat_path, status, sizeof(status))) {
        g_strlcpy(status, "Unknown", sizeof(status));
    }

    g_snprintf(st->battery_line, sizeof(st->battery_line),
               "battery: %s%% (%s)", capacity, status);
}

static void refresh_label(AppState *st) {
    if (st->has_battery && st->battery_line[0] != '\0') {
        char text[320];
        g_snprintf(text, sizeof(text), "%s\n%s\n%s",
                   st->time_line, st->uptime_line, st->battery_line);
        gtk_label_set_text(GTK_LABEL(st->label), text);
    } else {
        char text[256];
        g_snprintf(text, sizeof(text), "%s\n%s",
                   st->time_line, st->uptime_line);
        gtk_label_set_text(GTK_LABEL(st->label), text);
    }
}

static gboolean update_time_cb(gpointer data) {
    AppState *st = (AppState *)data;
    format_current_time(st);
    refresh_label(st);
    return G_SOURCE_CONTINUE;
}

static gboolean update_slow_cb(gpointer data) {
    AppState *st = (AppState *)data;
    format_uptime(st);
    format_battery(st);
    refresh_label(st);
    return G_SOURCE_CONTINUE;
}

static void apply_css(void) {
    static const char *css =
        "#rootbox {"
        "  background-color: #000000;"
        "  padding: 8px 10px;"
        "}"
        "#info-label {"
        "  color: #e8e8e8;"
        "  font-family: monospace;"
        "  font-size: 11px;"
        "}";

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(provider);
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    AppState st = {0};
    detect_battery(&st);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "tinyhud");
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);
    gtk_window_set_focus_on_map(GTK_WINDOW(window), FALSE);
    gtk_window_stick(GTK_WINDOW(window));
    gtk_window_move(GTK_WINDOW(window), WINDOW_X, WINDOW_Y);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "realize", G_CALLBACK(on_window_realize), NULL);


    apply_css();

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(box, "rootbox");

    GtkWidget *label = gtk_label_new("");
    gtk_widget_set_name(label, "info-label");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_yalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_label_set_selectable(GTK_LABEL(label), FALSE);

    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), box);

    st.label = label;

    format_current_time(&st);
    format_uptime(&st);
    format_battery(&st);
    refresh_label(&st);

    g_timeout_add(1000, update_time_cb, &st);
    g_timeout_add_seconds(10, update_slow_cb, &st);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
