#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#define attr_public __attribute__((visibility("default")))
#define attr_module_hidden __attribute__((weak)) __attribute__((visibility("hidden")))

attr_public const char *g_pluginName = "Vypex-Cr_ck";
attr_public const char *g_pluginDesc = "Vypex background service for GoldHEN";
attr_public const char *g_pluginAuth = "Plutana";
attr_public uint32_t g_pluginVersion = 0x00000100;

static const char *ROOT = "/data/Vypex-Cr_ck";
static const char *ENABLED_FILE = "/data/Vypex-Cr_ck/service.enabled";
static const char *STATUS_FILE = "/data/Vypex-Cr_ck/service.status";

static pthread_t g_worker;
static pthread_mutex_t g_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool set_running(false);
static bool g_worker_started = false;

static void set_running(bool running) {
    (void)pthread_mutex_lock(&g_state_mutex);
    g_running = running;
    (void)pthread_mutex_unlock(&g_state_mutex);
}

static bool worker_running(void) {
    (void)pthread_mutex_lock(&g_state_mutex);
    const bool running = g_running;
    (void)pthread_mutex_unlock(&g_state_mutex);
    return running;
}

static bool service_enabled(void) {
    FILE *file = fopen(ENABLED_FILE, "r");
    if (!file) return true; /* First install defaults to enabled. */
    char value = '1';
    (void)fscanf(file, " %c", &value);
    fclose(file);
    return value != '0';
}

static void write_status(const char *status) {
    FILE *file = fopen(STATUS_FILE, "w");
    if (!file) return;
    (void)fprintf(file, "%s\n", status);
    fclose(file);
}

static void *service_worker(void *unused) {
    (void)unused;
    bool last_enabled = false;
    while (worker_running()) {
        const bool enabled = service_enabled();
        if (enabled != last_enabled) {
            write_status(enabled ? "RUNNING" : "STOPPED_BY_USER");
            last_enabled = enabled;
        }
        sleep(1);
    }
    return NULL;
}

static void stop_worker(void) {
    if (g_worker_started) {
        set_running(false);
        (void)pthread_join(g_worker, NULL);
        g_worker_started = false;
    }
    write_status("UNLOADED");
}

int32_t attr_public plugin_load(int32_t argc, const char *argv[]) {
    (void)argc;
    (void)argv;
    (void)mkdir(ROOT, 0777);
    set_running(true);
    if (pthread_create(&g_worker, NULL, service_worker, NULL) != 0) {
        g_running = false;
        write_status("START_FAILED");
        return -1;
    }
    g_worker_started = true;
    write_status(service_enabled() ? "RUNNING" : "STOPPED_BY_USER");
    return 0;
}

int32_t attr_public plugin_unload(int32_t argc, const char *argv[]) {
    (void)argc;
    (void)argv;
    stop_worker();
    return 0;
}

int32_t attr_module_hidden module_start(int64_t argc, const void *args) {
    (void)argc;
    (void)args;
    return 0;
}

int32_t attr_module_hidden module_stop(int64_t argc, const void *args) {
    (void)argc;
    (void)args;
    stop_worker();
    return 0;
}
